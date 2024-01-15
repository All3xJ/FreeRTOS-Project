# FreeRTOS Memory WatchDog Example

The memoryWatchDog code is designed for FreeRTOS V202212.01 and is intended to monitor and manage memory usage in a real-time operating system (RTOS) environment. The code includes two tasks (`vTask1` and `vTask2`) that dynamically allocate memory and a function (`memoryWatchdog`) to handle memory-related events. The primary goal is to ensure that tasks are terminated if free memory falls below a specified threshold.

## Prerequisites

- [FreeRTOS](https://www.freertos.org/): Make sure you have FreeRTOS set up for your target environment.

## Usage

1. Clone the repository or download the source code.
2. Set up FreeRTOS for your target environment.
3. Compile and run the code on your target platform.

## Run with QEMU

### Simulation Setup

To simulate the FreeRTOS Producer-Consumer example using QEMU, follow these steps:

1. Ensure you have [QEMU](https://www.qemu.org/) installed on your system.

2. Open a terminal and navigate to the directory containing your compiled FreeRTOS application binary (`RTOSDemo.out`).

3. Run the following command:
```console
qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -kernel RTOSDemo.out
```

## Code Overview

- Tasks (`vTask1` and `vTask2`):

   `vTask1`: A task that allocates memory, monitors heap statistics, and raises the priority of `vTask2` under certain conditions.
   `vTask2`: A task that allocates memory, monitors heap statistics, and lowers its priority to allow `vTask1` to regain control.

- Functions:


    - `memoryWatchdog`: Monitors free memory and terminates a task if it falls below a specified threshold. Also handles cleanup when all tasks have terminated.

    - `printHeapStats`: Prints heap statistics for a given task, including used heap space, available heap space, and block sizes.

- Constants: `MIN_FREE_MEMORY_THRESHOLD`: The minimum free memory threshold. Tasks are terminated if free memory falls below this value.

```c

#define MIN_FREE_MEMORY_THRESHOLD 59500

void main(void) {
    // Initialization code...

    xTaskCreate( vTask1, "Task 1",configMINIMAL_STACK_SIZE*2, NULL, 6, NULL);
    xTaskCreate( vTask2, "Task 2", configMINIMAL_STACK_SIZE*2, NULL, 5, &xTask2Handle );
    vTaskStartScheduler();

    // Main loop...
}

# Tasks
#vTask1
#The vTask1 allocates memory, monitors heap statistics, and raises the priority of `vTask2` under certain conditions..

void vTask1( void *pvParameters )
{
	UBaseType_t uxPriority;
	

	uxPriority = uxTaskPriorityGet( NULL );
	for( ;; )
	{
		
	/* Print out the name of this task. */
	vPrintString( "Task 1 is running\r\n" );
	
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats, "Task1");

	if (HeapStats.xAvailableHeapSpaceInBytes < MIN_FREE_MEMORY_THRESHOLD)
			memoryWatchdog(NULL, "Task1");

	void *allocatedMemoryTask1 = pvPortMalloc(10);

	


	if (!xTaskTerminated) {
				/* Code to be executed only if Task 2 is still running */
			
				vPrintString( "About to raise the Task 2 priority\r\n" );
				vTaskDelay(pdMS_TO_TICKS(1000));
				vTaskPrioritySet( xTask2Handle, ( uxPriority + 1 ) );
			}


	}
}


#vTask2
#The vTask2 allocates memory, monitors heap statistics, and lowers its priority to allow `vTask1` to regain control..

void vTask2( void *pvParameters )
{
	UBaseType_t uxPriority;
	
	uxPriority = uxTaskPriorityGet( NULL );
	
	for( ;; )
	{
	
	
	vPrintString( "Task 2 is running\r\n" );
	
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats, "Task2");

	if (HeapStats.xAvailableHeapSpaceInBytes < MIN_FREE_MEMORY_THRESHOLD){
		memoryWatchdog(xTask2Handle, "Task2");
	}

	void *allocatedMemoryTask2 = pvPortMalloc(20);
	


	// Lower Task 2 priority
	if(!xTaskTerminated){
		vPrintString( "About to lower the Task 2 priority\r\n" );
		vTaskDelay(pdMS_TO_TICKS(1000));
		
		vTaskPrioritySet( NULL, ( uxPriority - 2 ) );
		}
	}
}

#memoryWatchdog
/* Function to handle memory watchdog and cleanup when free memory is below threshold */
void memoryWatchdog(TaskHandle_t taskHandle, const char *taskName) {
    
	
		xTaskTerminated = pdTRUE;
        vPrintString(taskName);
        vPrintString(": Free memory below threshold. Stopping task.\r\n");

        vTaskDelete(taskHandle); // Terminate the task

    
        taskENTER_CRITICAL();
        {
            ulNumActiveTasks--;
        }
        taskEXIT_CRITICAL();

	if (ulNumActiveTasks == 0) {
        // All tasks have terminated, deallocate memory here
        vPortFree(allocatedMemoryTask1);
		vPortFree(allocatedMemoryTask2);
		
    }
   
    
}

#printHeapStats
//Function to print heap statistics //
void printHeapStats(HeapStats_t *pstats, const char *taskName) {
  printf("Free heap statistics for task '%s':\n", taskName);
  printf(" UsedHeapSpaceInBytes: %u\n", configTOTAL_HEAP_SIZE - pstats->xAvailableHeapSpaceInBytes);
  printf(" AvailableHeapSpaceInBytes: %u\n", pstats->xAvailableHeapSpaceInBytes);
  printf(" SizeOfLargestFreeBlockInBytes: %u\n", pstats->xSizeOfLargestFreeBlockInBytes);
  printf(" SizeOfSmallestFreeBlockInBytes: %u\n", pstats->xSizeOfSmallestFreeBlockInBytes);
  printf(" NumberOfFreeBlocks: %u\n", pstats->xNumberOfFreeBlocks);
  printf(" MinimumEverFreeBytesRemaining: %u\n", pstats->xMinimumEverFreeBytesRemaining);
  printf(" NumberOfSuccessfulAllocations: %u\n", pstats->xNumberOfSuccessfulAllocations);
  printf(" NumberOfSuccessfulFrees: %u\n\n", pstats->xNumberOfSuccessfulFrees);
}


 


```

##General Issues for further implementation
- Se provi a fare i whatch dod periodico, , killa il task che è al dis sotto della soglia, ma anche quando il secondo supera la soglia, questo non viene killato
- Stesso problema si verifica quando vengono analizzate le priorità, o vengono killati i task in base alla priorità 







