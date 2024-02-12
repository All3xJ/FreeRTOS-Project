/*
 * FreeRTOS Kernel V10.5.1
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
    #define configHEAP_CLEAR_MEMORY_ON_FREE    0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* Max value that fits in a size_t type. */
#define heapSIZE_MAX              ( ~( ( size_t ) 0 ) )

/* Check if multiplying a and b will result in overflow. */
#define heapMULTIPLY_WILL_OVERFLOW( a, b )    ( ( ( a ) > 0 ) && ( ( b ) > ( heapSIZE_MAX / ( a ) ) ) )

/* Check if adding a and b will result in overflow. */
#define heapADD_WILL_OVERFLOW( a, b )         ( ( a ) > ( heapSIZE_MAX - ( b ) ) )

/* MSB of the xBlockSize member of an BlockLink_t structure is used to track
 * the allocation status of a block.  When MSB of the xBlockSize member of
 * an BlockLink_t structure is set then the block belongs to the application.
 * When the bit is free the block is still part of the free heap space. */
#define heapBLOCK_ALLOCATED_BITMASK    ( ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 ) )
#define heapBLOCK_SIZE_IS_VALID( xBlockSize )    ( ( ( xBlockSize ) & heapBLOCK_ALLOCATED_BITMASK ) == 0 )
#define heapBLOCK_IS_ALLOCATED( pxBlock )        ( ( ( pxBlock->xBlockSize ) & heapBLOCK_ALLOCATED_BITMASK ) != 0 )
#define heapALLOCATE_BLOCK( pxBlock )            ( ( pxBlock->xBlockSize ) |= heapBLOCK_ALLOCATED_BITMASK )
#define heapFREE_BLOCK( pxBlock )                ( ( pxBlock->xBlockSize ) &= ~heapBLOCK_ALLOCATED_BITMASK )

/*-----------------------------------------------------------*/

/* Allocate the memory for the heap. */
#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )

/* The application writer has already defined the array used for the RTOS
* heap - probably so it can be placed in a special segment or address. */
    extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
    PRIVILEGED_DATA static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert ) PRIVILEGED_FUNCTION;

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void ) PRIVILEGED_FUNCTION;

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
PRIVILEGED_DATA static BlockLink_t xStart;
PRIVILEGED_DATA static BlockLink_t * pxEnd = NULL;

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
PRIVILEGED_DATA static size_t xFreeBytesRemaining = 0U;
PRIVILEGED_DATA static size_t xMinimumEverFreeBytesRemaining = 0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulAllocations = 0;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulFrees = 0;

/*-----------------------------------------------------------*/

void *pvPortMalloc(size_t xWantedSize) {
    BlockLink_t *pxBlock;
    BlockLink_t *pxPreviousBlock;
    BlockLink_t *pxLargestBlock = NULL;  
    size_t xAdditionalRequiredSize;
    void *pvReturn = NULL;

    vTaskSuspendAll();
    {
        if (pxEnd == NULL) {
            prvHeapInit();
        }

        if (xWantedSize > 0) {
            xAdditionalRequiredSize = xHeapStructSize + portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);
            if (heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0) {
                xWantedSize += xAdditionalRequiredSize;
            } else {
                xWantedSize = 0;
            }
        } else {
            mtCOVERAGE_TEST_MARKER();
        }

        if (heapBLOCK_SIZE_IS_VALID(xWantedSize) != 0) {
            pxPreviousBlock = &xStart;
            pxBlock = xStart.pxNextFreeBlock;

            while (pxBlock != pxEnd) {
                if (heapBLOCK_IS_ALLOCATED(pxBlock) == 0 && pxBlock->xBlockSize >= xWantedSize) {
                    if (pxLargestBlock == NULL || pxBlock->xBlockSize > pxLargestBlock->xBlockSize) { 
                        pxLargestBlock = pxBlock;  
                    }
                }
                pxPreviousBlock = pxBlock;
                pxBlock = pxBlock->pxNextFreeBlock;
            }

            if (pxLargestBlock != NULL) { 
                pvReturn = (void *)(((uint8_t *)pxLargestBlock) + xHeapStructSize);
                pxPreviousBlock->pxNextFreeBlock = pxLargestBlock->pxNextFreeBlock;
                if ((pxLargestBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE) {
                    BlockLink_t *pxNewBlockLink = (void *)(((uint8_t *)pxLargestBlock) + xWantedSize);
                    pxNewBlockLink->xBlockSize = pxLargestBlock->xBlockSize - xWantedSize;
                    pxLargestBlock->xBlockSize = xWantedSize;
                    prvInsertBlockIntoFreeList(pxNewBlockLink);
                }
                xFreeBytesRemaining -= pxLargestBlock->xBlockSize;
                heapALLOCATE_BLOCK(pxLargestBlock);
                pxLargestBlock->pxNextFreeBlock = NULL;
                xNumberOfSuccessfulAllocations++;
            }
        }
    }
    (void)xTaskResumeAll();

#if ( configUSE_MALLOC_FAILED_HOOK == 1 )
    if( pvReturn == NULL ) {
        vApplicationMallocFailedHook();
    } else {
        mtCOVERAGE_TEST_MARKER();
    }
#endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    configASSERT(((size_t)pvReturn & (size_t)portBYTE_ALIGNMENT_MASK) == 0);

    return pvReturn;
}

/*-----------------------------------------------------------*/

void vPortFree( void * pv )
{
    uint8_t * puc = ( uint8_t * ) pv;
    BlockLink_t * pxLink;

    if( pv != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;

        configASSERT( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                heapFREE_BLOCK( pxLink );
                #if ( configHEAP_CLEAR_MEMORY_ON_FREE == 1 )
                {
                    ( void ) memset( puc + xHeapStructSize, 0, pxLink->xBlockSize - xHeapStructSize );
                }
                #endif

                vTaskSuspendAll();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
                    xNumberOfSuccessfulFrees++;
                }
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
    /* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

void * pvPortCalloc( size_t xNum,
                     size_t xSize )
{
    void * pv = NULL;

    if( heapMULTIPLY_WILL_OVERFLOW( xNum, xSize ) == 0 )
    {
        pv = pvPortMalloc( xNum * xSize );

        if( pv != NULL )
        {
            ( void ) memset( pv, 0, xNum * xSize );
        }
    }

    return pv;
}
/*-----------------------------------------------------------*/

static void prvHeapInit( void ) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t * pxFirstFreeBlock;
    uint8_t * pucAlignedHeap;
    portPOINTER_SIZE_TYPE uxAddress;
    size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxAddress = ( portPOINTER_SIZE_TYPE ) ucHeap;

    if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
    {
        uxAddress += ( portBYTE_ALIGNMENT - 1 );
        uxAddress &= ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= uxAddress - ( portPOINTER_SIZE_TYPE ) ucHeap;
    }

    pucAlignedHeap = ( uint8_t * ) uxAddress;

    /* xStart is used to hold a pointer to the first item in the list of free
     * blocks.  The void cast is used to prevent compiler warnings. */
    xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
    xStart.xBlockSize = ( size_t ) 0;

    /* pxEnd is used to mark the end of the list of free blocks and is inserted
     * at the end of the heap space. */
    uxAddress = ( ( portPOINTER_SIZE_TYPE ) pucAlignedHeap ) + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK );
    pxEnd = ( BlockLink_t * ) uxAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /* To start with there is a single free block that is sized to take up the
     * entire heap space, minus the space taken by pxEnd. */
    pxFirstFreeBlock = ( BlockLink_t * ) pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = ( size_t ) ( uxAddress - ( portPOINTER_SIZE_TYPE ) pxFirstFreeBlock );
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
}
/*-----------------------------------------------------------*/
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert) PRIVILEGED_FUNCTION {
    BlockLink_t *pxIterator, *pxPrevBlock = NULL;
    size_t xRequiredSize = pxBlockToInsert->xBlockSize;

    // Find the worst-fitting block (largest free size that can fit the allocation)
    for (pxIterator = &xStart; pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock) {
        if (pxIterator->pxNextFreeBlock->xBlockSize >= xRequiredSize) {
            pxPrevBlock = pxIterator;
        } else {
            break; // No larger blocks found, insert here
        }
    }

    // Merge with previous block if possible
    if (pxPrevBlock != NULL && (pxPrevBlock->pxNextFreeBlock->xBlockSize + xRequiredSize) > heapMINIMUM_BLOCK_SIZE) {
        pxPrevBlock->pxNextFreeBlock->xBlockSize += xRequiredSize;
        pxBlockToInsert = pxPrevBlock->pxNextFreeBlock;
    } else {
        // Insert after the previous block or at the beginning
        pxBlockToInsert->pxNextFreeBlock = pxPrevBlock ? pxPrevBlock->pxNextFreeBlock : xStart.pxNextFreeBlock;
        if (pxPrevBlock) {
            pxPrevBlock->pxNextFreeBlock = pxBlockToInsert;
        } else {
            xStart.pxNextFreeBlock = pxBlockToInsert;
        }
    }

    // Merge with next block if possible (after potential split)
    if (pxBlockToInsert->pxNextFreeBlock != pxEnd &&
        (pxBlockToInsert->xBlockSize + pxBlockToInsert->pxNextFreeBlock->xBlockSize) > heapMINIMUM_BLOCK_SIZE) {
        pxBlockToInsert->xBlockSize += pxBlockToInsert->pxNextFreeBlock->xBlockSize;
        pxBlockToInsert->pxNextFreeBlock = pxBlockToInsert->pxNextFreeBlock->pxNextFreeBlock;
    }

    // Maintain descending order of free block sizes
    while (pxBlockToInsert->pxNextFreeBlock && pxBlockToInsert->pxNextFreeBlock->xBlockSize > pxBlockToInsert->xBlockSize) {
        BlockLink_t *temp = pxBlockToInsert->pxNextFreeBlock;
        pxBlockToInsert->pxNextFreeBlock = temp->pxNextFreeBlock;
        temp->pxNextFreeBlock = pxBlockToInsert;
        if (pxBlockToInsert == &xStart) {
            xStart.pxNextFreeBlock = temp;
        } else {
            pxPrevBlock->pxNextFreeBlock = temp;
        }
        pxPrevBlock = temp;
    }
}



/*-----------------------------------------------------------*/

void vPortGetHeapStats( HeapStats_t * pxHeapStats )
{
    BlockLink_t * pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = xStart.pxNextFreeBlock;

        /* pxBlock will be NULL if the heap has not been initialised.  The heap
         * is initialised automatically when the first allocation is made. */
        if( pxBlock != NULL )
        {
            while( pxBlock != pxEnd )
            {
                /* Increment the number of blocks and record the largest block seen
                 * so far. */
                xBlocks++;

                if( pxBlock->xBlockSize > xMaxSize )
                {
                    xMaxSize = pxBlock->xBlockSize;
                }

                if( pxBlock->xBlockSize < xMinSize )
                {
                    xMinSize = pxBlock->xBlockSize;
                }

                /* Move to the next block in the chain until the last block is
                 * reached. */
                pxBlock = pxBlock->pxNextFreeBlock;
            }
        }
    }
    ( void ) xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/
