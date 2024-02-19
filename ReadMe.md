# Worst-fit allocator
The "Worst-fit" is a memory allocation algorithm that aims to assign a memory block to the current request by selecting the largest available block.

In this implementation, we started with the codebase for the best-fit strategy. By understanding the nature of the worst-fit strategy as its specular counterpart, we made specific modifications to adapt the code accordingly. It was necessary to modify both the **pvPortMalloc** and the **prvInsertBlockIntoFreeList** functions.

The following sections highlight the changes made to the original codebase:

### Modification 1: Adjusting Memory Allocation

In the memory allocation function `pvPortMalloc`, we made the following adjustments:
   
```c
void * pvPortMalloc( size_t xWantedSize )
{
    //...
    //other code

    //while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
    // {
    //     pxPreviousBlock = pxBlock;
    //     pxBlock = pxBlock->pxNextFreeBlock;
    // }

    if(pxBlock->xBlockSize < xWantedSize){
        pxBlock = NULL; // If the largest block cannot contain the requested size, set to NULL to avoid entering the if condition and proceed to the else statement.
    }

    //...
    //other code
}
```

We commented out the while loop since in the worst-fit strategy, we only need to take the block at the top of the list, as it's already the largest available block. We don't have to search in the list.


### Modification 2: Updating Block Insertion

In the block insertion function `prvInsertBlockIntoFreeList`, we modified the comparison condition as follows:

```c
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert ) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /* Iterate through the list until a block with a larger size is found or the end is reached */
    for( pxIterator = &xStart; pxIterator->pxNextFreeBlock->xBlockSize > pxBlockToInsert->xBlockSize && pxIterator->pxNextFreeBlock != pxEnd; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    //...
    //other code
}

```

Here, we changed the comparison condition from `pxIterator->pxNextFreeBlock->xBlockSize < pxBlockToInsert->xBlockSize` to `pxIterator->pxNextFreeBlock->xBlockSize > pxBlockToInsert->xBlockSize` since the list is ordered in descending order for the worst-fit strategy.