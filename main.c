/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* printf() output uses the UART.  These constants define the addresses of the
required UART registers. */
#define UART0_ADDRESS 	( 0x40004000UL )
#define UART0_DATA		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 0UL ) ) ) )
#define UART0_STATE		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 4UL ) ) ) )
#define UART0_CTRL		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 8UL ) ) ) )
#define UART0_BAUDDIV	( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 16UL ) ) ) )
#define TX_BUFFER_MASK	( 1UL )

void vFullDemoTickHookFunction( void );


static void prvUARTInit( void );
void vTask1( void *pvParameters );

HeapStats_t HeapStats;
/*-----------------------------------------------------------*/



//Function to print heap statistics //
void printHeapStats(HeapStats_t *pstats) {
	// printf(" UsedHeapSpaceInBytes: %u\n", configTOTAL_HEAP_SIZE - pstats->xAvailableHeapSpaceInBytes);
	// printf(" AvailableHeapSpaceInBytes: %u\n", pstats->xAvailableHeapSpaceInBytes);
	// printf(" SizeOfLargestFreeBlockInBytes: %u\n", pstats->xSizeOfLargestFreeBlockInBytes);
	// printf(" SizeOfSmallestFreeBlockInBytes: %u\n", pstats->xSizeOfSmallestFreeBlockInBytes);
	// printf(" NumberOfFreeBlocks: %u\n", pstats->xNumberOfFreeBlocks);
	// printf(" MinimumEverFreeBytesRemaining: %u\n", pstats->xMinimumEverFreeBytesRemaining);
	// printf(" NumberOfSuccessfulAllocations: %u\n", pstats->xNumberOfSuccessfulAllocations);
	// printf(" NumberOfSuccessfulFrees: %u\n\n", pstats->xNumberOfSuccessfulFrees);

	printf("Free block list:\n");

	typedef struct A_BLOCK_LINK
	{
		struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
		size_t xBlockSize;                     /*<< The size of the free block. */
	} BlockLink_t;

    BlockLink_t *currentBlock = getFirstFreeBlock();
	int blockNum = 0;
    while (currentBlock->xBlockSize != 0) {
        printf("Block %d: Address=%d, Size=%d\n", blockNum, currentBlock, currentBlock->xBlockSize);
        currentBlock = currentBlock->pxNextFreeBlock;
        blockNum++;
    }
	printf("\nFragmentation is: (%d-%d)/(%d)\n",pstats->xAvailableHeapSpaceInBytes,pstats->xSizeOfLargestFreeBlockInBytes,pstats->xAvailableHeapSpaceInBytes);
}

/*-----------------------------------------------------------*/


int main( void )
{
	prvUARTInit();
	

	xTaskCreate( vTask1, "Task 1", configMINIMAL_STACK_SIZE*2, NULL, 4, NULL );


	/* Start the scheduler so the tasks start executing. */
	vTaskStartScheduler();

      

	/* If all is well then main() will never reach here as the scheduler will 
	now be running the tasks. If main() does reach here then it is likely there
	was insufficient heap memory available for the idle task to be created. 
	*/
	for( ;; );



	

}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created using the dynamic allocation (as opposed to
	static allocation) option.  It is also called by various parts of the
	demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
	size of the	heap available to pvPortMalloc() is defined by
	configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
	API function can be used to query the size of free heap space that remains
	(although it does not provide information on how the remaining heap might be
	fragmented).  See http://www.freertos.org/a00111.html for more
	information. */
	printf( "\r\n\r\nMalloc failed\r\n" );
	portDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If application tasks make use of the
	vTaskDelete() API function to delete themselves then it is also important
	that vApplicationIdleHook() is permitted to return to its calling function,
	because it is the responsibility of the idle task to clean up memory
	allocated by the kernel to any task that has since deleted itself. */
	
   
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	printf( "\r\n\r\nStack overflow in %s\r\n", pcTaskName );
	portDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
	/* This function will be called once only, when the daemon task starts to
	execute (sometimes called the timer task).  This is useful if the
	application includes initialisation code that would benefit from executing
	after the scheduler has been started. */
}
/*-----------------------------------------------------------*/

void vAssertCalled( const char *pcFileName, uint32_t ulLine )
{
volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

	/* Called if an assertion passed to configASSERT() fails.  See
	http://www.freertos.org/a00110.html#configASSERT for more information. */

	printf( "ASSERT! Line %d, file %s\r\n", ( int ) ulLine, pcFileName );

 	taskENTER_CRITICAL();
	{
		/* You can step out of this function to debug the assertion by using
		the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
		value. */
		while( ulSetToNonZeroInDebuggerToContinue == 0 )
		{
			__asm volatile( "NOP" );
			__asm volatile( "NOP" );
		}
	}
	taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/

static void prvUARTInit( void )
{
	UART0_BAUDDIV = 16;
	UART0_CTRL = 1;
}
/*-----------------------------------------------------------*/

int __write( int iFile, char *pcString, int iStringLength )
{
	int iNextChar;

	/* Avoid compiler warnings about unused parameters. */
	( void ) iFile;

	/* Output the formatted string to the UART. */
	for( iNextChar = 0; iNextChar < iStringLength; iNextChar++ )
	{
		while( ( UART0_STATE & TX_BUFFER_MASK ) != 0 );
		UART0_DATA = *pcString;
		pcString++;
	}

	return iStringLength;
}
/*-----------------------------------------------------------*/

void *malloc( size_t size )
{
	( void ) size;

	/* This project uses heap_4 so doesn't set up a heap for use by the C
	library - but something is calling the C library malloc().  See
	https://freertos.org/a00111.html for more information. */
	printf( "\r\n\r\nUnexpected call to malloc() - should be usine pvPortMalloc()\r\n" );
	portDISABLE_INTERRUPTS();
	for( ;; );

}


void doFixedMemoryTests(){


	int numB = 1024;
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Malloc of %d\n",numB);
	void* b = pvPortMalloc(numB);
	vTaskDelay(100);


	int numC = 512;
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Malloc of %d\n",numC);
	void* c = pvPortMalloc(numC);
	vTaskDelay(100);

	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Freeing the %d block\n",numB);
	vPortFree(b);
	vTaskDelay(100);

	int numD = 256;
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Malloc of %d\n",numD);
	void* d = pvPortMalloc(numD);
	vTaskDelay(100);

	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Freeing the %d block\n",numC);
	vPortFree(c);
	vTaskDelay(100);

	int numE = 128;
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Malloc of %d\n",numE);
	void* e = pvPortMalloc(numE);
	vTaskDelay(100);

	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Freeing the %d block\n",numD);
	vPortFree(d);
	vTaskDelay(100);

	int numF = 64;
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Malloc of %d\n",numF);
	void* f = pvPortMalloc(numF);
	vTaskDelay(100);

	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Freeing the %d block\n",numE);
	vPortFree(e);
	vTaskDelay(100);

	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats);
	printf("Freeing the %d block\n",numF);
	vPortFree(f);
	vTaskDelay(100);

}


#define NUM_ALLOCATIONS 20
void doRandomMemoryTests(int iterations){
	int i=0;
	while(i<iterations){
		void *allocations[NUM_ALLOCATIONS] = {NULL};
		int seed = xTaskGetTickCount();
		int action = rand_r(&seed);
		//printf("%d\n",action);
		for (int i = 0; i < NUM_ALLOCATIONS; i++) {
			int action = rand_r(&seed) % 2;; // 0 to allocate, 1 to deallocate

			if (action == 0) {
				// Allocation
				size_t size = rand_r(&seed) % 2000 + 1;
				allocations[i] = pvPortMalloc(size);
				printf("Allocated block %d of size %d\n", i, size);
			} else {
				// Random deallocation
				int deallocate_index = rand_r(&seed) % NUM_ALLOCATIONS;
				if (allocations[deallocate_index] != NULL) {
					vPortFree(allocations[deallocate_index]);
					allocations[deallocate_index] = NULL;
					printf("Deallocated block %d\n", deallocate_index);
				}
			}
		}

		vPortGetHeapStats(&HeapStats);
		printHeapStats(&HeapStats);
		vTaskDelay(3000);
		printf("\nDoing test again..\n");

		// Remaining deallocations
		for (int i = 0; i < NUM_ALLOCATIONS; i++) {
			if (allocations[i] != NULL) {
				vPortFree(allocations[i]);
				//printf("Deallocated block %d remaining\n", i);
			}
		}

		i+=1;
	}

}

void vTask1( void *pvParameters )
{
	printf("\n\n\n\nNow starting the random memory test.\n\n");
	vTaskDelay(4000);
	doRandomMemoryTests(5);
	printf("\n\n\n\nNow starting the fixed memory test.\n\n");
	vTaskDelay(4000);
	doFixedMemoryTests();

	vTaskDelete( NULL );
}