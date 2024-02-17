# Best-fit allocator
Heap_4 uses a first fit algorithm to allocate memory. Unlike heap_2, heap_4 combines 
(coalescences) adjacent free blocks of memory into a single larger block, which minimizes the 
risk of memory fragmentation. 
The initial idea was to implement a best-fit allocator, as described in the implementation of heap_2.c, and to additionally leverage the enhancements introduced with heap_4.c.


In our implementation of the best-fit allocator, the **pvPortMalloc** function remains unchanged. Essentially, it conducts a search within the list of free blocks to locate a memory block suitable for the requested size. If a sufficiently large block is found, a pointer to the requested memory is returned. To adhere to the principles of the best-fit allocator, which involve allocating the smallest hole that is large enough, it was necessary to appropriately modify the **prvInsertBlockIntoFreeList** function. This function is ordered by the size of the block, with small blocks positioned at the beginning of the list and large blocks at the end.

```c
/* Iterate through the list until a block with a larger size is found or the end is reached */
for (pxIterator = &xStart; pxIterator->pxNextFreeBlock->xBlockSize < pxBlockToInsert->xBlockSize && pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock) {
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


In this version of the **prvInsertBlockIntoFreeList** function, the code iterates through the list of free blocks, comparing the size of each block with the size of the block to be inserted (pxBlockToInsert). It stops the iteration when it finds a block that is larger to pxBlockToInsert. After determining the correct position to insert the block into the list, the code checks if it's possible to merge the block to be inserted with the previous or next block. If possible, the blocks are merged together to form a larger block. Additionally, After the merge, if the block to be inserted already has a valid pxNextFreeBlock pointer, it is not overwritten to prevent it from pointing to itself.


