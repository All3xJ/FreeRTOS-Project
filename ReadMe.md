# Best-fit allocator
The "Worst-fit" is a memory allocation algorithm that aims to assign a memory block to the current request by selecting the minimum size available block which can contain the requested size.

### Allocator implementation

In our implementation of the best-fit allocator, the **pvPortMalloc** function remains unchanged. Essentially, it conducts a search within the list of free blocks to locate a memory block suitable for the requested size. If a sufficiently large block is found, a pointer to the requested memory is returned. To adhere to the principles of the best-fit allocator, which involve allocating the smallest hole that is large enough, it was necessary to appropriately modify the **prvInsertBlockIntoFreeList** function. This function is ordered by the size of the block, with small blocks positioned at the beginning of the list and large blocks at the end.

```c
/* Iterate through the list until a block with a adequate size is found or the end is reached */
for (pxIterator = &xStart; pxIterator->pxNextFreeBlock->xBlockSize < pxBlockToInsert->xBlockSize && pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock) {
    /* Nothing to do here, just iterate to the right position. */
}

// ......

 ```


In this version of the **prvInsertBlockIntoFreeList** function, the code iterates through the list of free blocks, comparing the size of each block with the size of the block to be inserted (pxBlockToInsert). It stops the iteration when it finds a block that is larger to pxBlockToInsert. 


### Memory Watchdog Implementation
We've integrated a memory watchdog functionality within the `pvPortMalloc` function to ensure that memory allocation doesn't exceed a certain threshold. Here's the implementation:

```c
void * pvPortMalloc( size_t xWantedSize )
{
    //...
    //other code

    if (memoryWatchdog(xWantedSize) != 0) {
        pxBlock = NULL;   // Either if memory watchdog notices that it will exceed the threshold or if the largest block can't contain it: I can't allocate so I set to NULL to not enter the if but instead enter in else
        pvReturn = -1;    // Set it to NOT NULL so that it will not enter in malloc failed hook so does not crash the whole system
    }

    //...
    //other code
```

The `memoryWatchdog` function checks if the remaining free memory after allocation is below a minimum threshold:

```c
int memoryWatchdog(int xWantedSize){
    if (xFreeBytesRemaining - xWantedSize <= MIN_FREE_MEMORY_THRESHOLD) {
        return -1;
    }
    return 0;
}
```

We've enhanced the `vPortFree` function to handle scenarios where attempts are made to free memory blocks that were never allocated due to the memory watchdog intervention.
If `pvReturn` (or `pv`, representing the return value from `malloc`) equals -1, indicating a failed allocation attempt due to watchdog, the `vPortFree` function now ignores the free operation for such blocks. This prevents potential system crashes caused by attempting to free memory that was never allocated.
This modification adds a layer of robustness to memory management, particularly in critical systems, by preventing crashes resulting from programmer oversights or failures to verify successful memory allocations before freeing, or even memory exhaustion attacks.

## Fragmentation Testing

To assess memory fragmentation, we've implemented a testing procedure within the `vTask1` function. This procedure involves allocating and deallocating memory blocks of random sizes and monitoring the fragmentation levels.

```c
void vTask1( void *pvParameters )
{
    int seed = 777777777777;
	int iter = 0;
	while(1){
		printf("\n\nIter n. %d",iter);

		int randNumA = rand_r(&seed) % 10000;
		vPortGetHeapStats(&HeapStats);
		printHeapStats(&HeapStats);
		printf("Allocating %d\n",randNumA+16);
		void* a = pvPortMalloc(randNumA);
		vTaskDelay(100);
        
        // ... same for block B and C

        vPortGetHeapStats(&HeapStats);
		printHeapStats(&HeapStats);
		printf("Freeing %d\n",randNumA+16);
		vPortFree(a);
		vTaskDelay(100);

        // ...same for block B and C
    }
}
```

Inside `printHeapStats` we print information fragmentation ratio, calculated using the formula:
`Fragmentation = (Total Free Memory - Size of Largest Free Block) / (Total Free Memory)`.
We observe that the fragmentation level increases asymptotically towards 100% as more memory is allocated and deallocated over multiple iterations. This testing procedure provides valuable insights into the behavior of memory fragmentation within the system.