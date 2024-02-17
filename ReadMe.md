# Worst-fit allocator
The "Worst-fit" is a memory allocation algorithm that aims to assign a memory block to the current request by selecting the largest available block. In this case, we also attempted to implement such type of allocator by leveraging the features already present in the heap_4.c file, such as the coalescence.

In this case, it was necessary to modify both the **pvPortMalloc** and the **prvInsertBlockIntoFreeList** functions, in respect to the vanilla version of the heap_4.c file.

# pvPortMalloc

```c
//...

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
//...

 ```
This code section replaces the original logic for selecting the block to allocate. Instead of selecting the first available block of sufficient size, it iterates through the entire list of free blocks to select the largest block that can accommodate the requested size.
When pxLargestBlock is not NULL,  a suitable free block has been found for allocation. Next, the linked list of free blocks is updated. The block preceding pxLargestBlock is connected to the block following pxLargestBlock, effectively removing pxLargestBlock from the list of free blocks. This ensures that the allocated block is no longer considered part of the free memory pool.

The size of the allocated block is adjusted to match the requested size, and the new free block is inserted into the list of free blocks using the prvInsertBlockIntoFreeList function.
Finally, the pxNextFreeBlock pointer of the allocated block is set to NULL, indicating that it is no longer part of the free block list and the count of successful allocations is incremented to track the number of successful memory allocations.

# prvInsertBlockIntoFreeList

 ```c
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert) PRIVILEGED_FUNCTION {
    BlockLink_t *pxIterator, *pxPrevBlock = NULL;
    size_t xRequiredSize = pxBlockToInsert->xBlockSize;

    // Find the largest free size 
    for (pxIterator = &xStart; pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock) {
        if (pxIterator->pxNextFreeBlock->xBlockSize >= xRequiredSize) {
            pxPrevBlock = pxIterator;
        } else {
            break; 
        }
    }

    // Merge with the previous block if possible
    if (pxPrevBlock != NULL && (pxPrevBlock->pxNextFreeBlock->xBlockSize + xRequiredSize) > heapMINIMUM_BLOCK_SIZE) {
        pxPrevBlock->pxNextFreeBlock->xBlockSize += xRequiredSize;

        // Update the pointer of the block to be inserted to point to the newly merged block
        pxBlockToInsert = pxPrevBlock->pxNextFreeBlock;
    }

    //...


}

 ```
 **prvInsertBlockIntoFreeList**, is responsible for inserting a memory block into the list of free memory blocks. The function then proceeds to search through the list of free memory blocks to identify the largest block. If such a block is found, it updates pxPrevBlock to point to the block preceding it, or it breaks out of the loop if no suitable block is found.

 If a suitable block is found (pxPrevBlock != NULL), and merging the block to be inserted with the previous block does not violate the minimum block size, the size of the previous block is updated, and the pointer of the block to be inserted is updated to point to the merged block.
After the insertion, the function checks also if merging with the next block is feasible.

