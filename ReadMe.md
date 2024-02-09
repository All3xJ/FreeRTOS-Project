# Implementation of Best Fit and Worst Fit Allocator in FreeRTOS

## Modifications Implemented

### Best Fit Allocator

- function **prvInsertBlockIntoFreeList**: 

 ```c
/* Iterate through the list until a block with a larger size is found or the end is reached */
for (pxIterator = &xStart; pxIterator->pxNextFreeBlock->xBlockSize <= pxBlockToInsert->xBlockSize && pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock) {
    /* Nothing to do here, just iterate to the right position. */
}

// ......

/* If the block being inserted plugged a gap, so was merged with the block
 * before and the block after, then its pxNextFreeBlock pointer will have
 * already been set, and should not be set here as that would make it point
 * to itself. */
if (pxIterator != pxBlockToInsert) {
    pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    pxIterator->pxNextFreeBlock = pxBlockToInsert;
} else {
    mtCOVERAGE_TEST_MARKER();
}
 ```






The changes concern only the function prvInsertBlockIntoFreeList, as in the pvPortMalloc function, already in the first fit configuration, there is already the search "Traverse the list from the start (lowest address) block until one of adequate size is found." In this version of the prvInsertBlockIntoFreeList function, the insertion of the free block into the list of free blocks is performed based on the requested size. The largest block that can accommodate the requested size is sought, and a merge of adjacent blocks is performed if possible to form a single larger block. Additionally, the descending order of blocks based on their sizes is maintained to optimize the utilization of available space.

##  Worst Fit Allocator
- The **pvPortMalloc** function has been modified as follows to implement worst fit allocation:

 ```c
pxPreviousBlock = &xStart;
pxBlock = xStart.pxNextFreeBlock;

while (pxBlock != pxEnd)
{
    if (heapBLOCK_IS_ALLOCATED(pxBlock) == 0 && pxBlock->xBlockSize >= xWantedSize)
    {
        if (pxLargestBlock == NULL || pxBlock->xBlockSize > pxLargestBlock->xBlockSize)
        {
            pxLargestBlock = pxBlock;
        }
    }
    pxPreviousBlock = pxBlock;
    pxBlock = pxBlock->pxNextFreeBlock;
}

 ```
This code section replaces the original logic for selecting the block to allocate. Instead of selecting the first available block of sufficient size, it iterates through the entire list of free blocks to select the largest block that can accommodate the requested size.
 ```c
 if (pxLargestBlock != NULL)
            {
                pvReturn = (void *)(((uint8_t *)pxLargestBlock) + xHeapStructSize);
                pxPreviousBlock->pxNextFreeBlock = pxLargestBlock->pxNextFreeBlock;
                if ((pxLargestBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
                {
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
 ```
In this case we remove the block from the list of free blocks, split it if larger than necessary, update the state variables, and return the pointer to the allocated block.

- To implement worst fit, it was also necessary to modify the **prvInsertIntoFreeBlock** function:

 ```c
 size_t xRequiredSize = pxBlockToInsert->xBlockSize;

// Find the worst-fitting block (largest free size that can fit the allocation)
for (pxIterator = &xStart; pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock) {
    if (pxIterator->pxNextFreeBlock->xBlockSize >= xRequiredSize) {
        pxPrevBlock = pxIterator;
    } else {
        break; 
    }
}

 ```
 This part of the function searches for the largest free block that can accommodate the requested allocation size.
Additionally, there are code segments concerning the merging of the new block with the previous one if feasible, or inserting it after the previous block otherwise, as well as handling the merging of the new block with the next one, if possible, after a potential split.

