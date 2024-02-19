# Best-fit allocator
The "Best-fit" is a memory allocation algorithm that aims to assign a memory block to the current request by selecting the smallest available block, able to serve our request. 

We implemented coalescing Best-fit with a memory watchdog feature by modifying the `pvPortMalloc` and `vPortFree` functions in `heap_4.c`.

Moreover, we maintain the same implementation of `prvInsertBlockIntoFreeList` as that of heap_4.c with First-fit allocator, in which we insert a new block into free list, iterating through the list until a block is found that has a higher address than the block that has to be inserted, in order to maintain coalescing.

### Search the Best-fit block:

The function iterates through the free memory blocks, comparing each block's size to the requested size (`xWantedSize`). The algorithm selects the best-fit block by considering both the block's size and if it is smaller than the previously identified best-fit block (`tmpPxBlock`). The iterative process continues until all free blocks have been examined.

```c
void * pvPortMalloc( size_t xWantedSize )
{
    // ... other code

    do
    {
        if(( pxBlock->xBlockSize>=xWantedSize && (tmpPxBlock->xBlockSize > pxBlock->xBlockSize || tmpPxBlock==NULL ))){ // if i-th block can contain wantedsize and it's smaller than previously saved one, then
            tmpPxPreviousBlock = pxPreviousBlock;
            tmpPxBlock = pxBlock;
        }
        pxPreviousBlock = pxBlock;
        pxBlock = pxBlock->pxNextFreeBlock;
    }while(pxBlock->pxNextFreeBlock != NULL );

    pxPreviousBlock = tmpPxPreviousBlock;
    pxBlock = tmpPxBlock;

    // ... other code
```

### Memory Watchdog function

This feature allows us to check at every `pvPortMalloc` call if the wanted-allocated size exceeds a specified threshold, `MIN_FREE_MEMORY_THRESHOLD`, which is the minimum free space in memory. We've also enhanced the `vPortFree` function to handle scenarios where attempts are made to free memory blocks that were never allocated due to the memory watchdog intervention.
If `pv` (representing the return value from `pvPortMalloc`) equals -1, indicating a failed allocation attempt due to watchdog, the `vPortFree` function now ignores the free operation for such blocks. This prevents potential system crashes caused by attempting to free memory that was never allocated.
This modification adds a layer of robustness to memory management, particularly in critical systems, by preventing crashes resulting from programmer oversights or failures to verify successful memory allocations before freeing, or even memory exhaustion attacks.

```c
void * pvPortMalloc( size_t xWantedSize )
{
    // ... other code
    if( heapBLOCK_SIZE_IS_VALID( xWantedSize ) != 0 )
    {
        if(memoryWatchdog(xWantedSize)!=0){
            ( void ) xTaskResumeAll();  // I have to resume scheduler
            return -1;                  // return -1 so skips allocation, to signal that watchdog kicked in so malloc failed
        }
    }
    // ... other code
```

```c
void vPortFree( void * pv )
{
   // ... other code
    

    if( pv != NULL && pv!=-1)   // I both check if pv is not NULL but also if not -1 since can be -1 if set by the memoryWatchdog. this way we avoid that the system crashes if some user is not checking the block address to free
    {
    // ... other code
```

The `memoryWatchdog` kicks in whenever a malloc is called and check if the new allocation of a block would make the remaing free bytes in memory, under a certain treshold.
```c
int memoryWatchdog(int xWantedSize){
    if (xFreeBytesRemaining - xWantedSize <= MIN_FREE_MEMORY_THRESHOLD){
        return -1; // Memory allocation exceeds threshold
    }
    return 0;
}
```

### Fragmentation Testing

To assess memory fragmentation, we've implemented a testing procedure within the `vTask1` function. This function will call two tests:
1) `doRandomMemoryTests`: randomly allocate and deallocate memory blocks of random sizes and monitoring the fragmentation levels.
2) `doFixedMemoryTests`: allocate and deallocate blocks of fixed size, to monitor the fragmentation levels and to check the correct Best-fit functionality.

```c
#define NUM_ALLOCATIONS 20
void doRandomMemoryTests(int iterations){
	int i=0;
	while(i<iterations){
		void *allocations[NUM_ALLOCATIONS] = {NULL};
		int seed = xTaskGetTickCount();
		int action = rand_r(&seed);
		for (int i = 0; i < NUM_ALLOCATIONS; i++) {
			int action = rand_r(&seed) % 2;; // 0 to allocate, 1 to deallocate

			if (action == 0) {
				// Allocation
				size_t size = rand_r(&seed) % 000 + 1;
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
		printf("\nDoing test again..\n");

		// Remaining deallocations
		for (int i = 0; i < NUM_ALLOCATIONS; i++) {
			if (allocations[i] != NULL) {
				vPortFree(allocations[i]);
			}
		}

		i+=1;
	}
}
```

To print fragmentation ratio, we use the formula:small
`Fragmentation = (Total Free Memory - Size of Largest Free Block) / (Total Free Memory)`.
After a small amount of memory operation, we obtain a slightly better fragmentation with Best-fit.