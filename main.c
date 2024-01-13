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
void vTask2( void *pvParameters );
volatile BaseType_t xTaskTerminated = pdFALSE;
TaskHandle_t xTask2Handle = NULL;
HeapStats_t HeapStats;
/*-----------------------------------------------------------*/
#define MIN_FREE_MEMORY_THRESHOLD 59500
volatile uint32_t ulNumActiveTasks = 2;
void *allocatedMemoryTask1 = NULL;
void *allocatedMemoryTask2 = NULL;



/* Function to handle memory watchdog and cleanup when free memory is below threshold */
void memoryWatchdog(TaskHandle_t taskHandle, const char *taskName) {
    
	
		xTaskTerminated = pdTRUE;
        vPrintString(taskName);
        vPrintString(": Free memory below threshold. Stopping task.\r\n");

        vTaskDelete(taskHandle); // Terminate the task

    // Decrement the count of active tasks
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

/*-----------------------------------------------------------*/


void vPrintString(const char *pcString) {
    
    // to output the string to the UART
    printf("%s", pcString);
}



int main( void )
{
	prvUARTInit();
	/* Create the first task at priority 6. The task parameter is not used 
	and set to NULL. The task handle is also not used so is also set to NULL. */
	
	xTaskCreate( vTask1, "Task 1",configMINIMAL_STACK_SIZE*2, NULL, 6, NULL);
	
	/* The task is created at priority 6 ______^. */

	/* Create the second task at priority 5 - which is lower than the priority
	given to Task 1. Again the task parameter is not used so is set to NULL -
	BUT this time the task handle is required so the address of xTask2Handle
	is passed in the last parameter. */
	xTaskCreate( vTask2, "Task 2", configMINIMAL_STACK_SIZE*2, NULL, 5, &xTask2Handle );
	/* The task handle is the last parameter _____^^^^^^^^^^^^^ */

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

void vTask1( void *pvParameters )
{
	UBaseType_t uxPriority;
	/* This task will always run before Task 2 as it is created with the higher 
	priority. Neither Task 1 nor Task 2 ever block so both will always be in 
	either the Running or the Ready state.
	Query the priority at which this task is running - passing in NULL means
	"return the calling task’s priority". */

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

	/* Setting the Task 2 priority above the Task 1 priority will cause
	Task 2 to immediately start running (as then Task 2 will have the higher 
	priority of the two created tasks). Note the use of the handle to task
	2 (xTask2Handle) in the call to vTaskPrioritySet(). Listing 35 shows how
	the handle was obtained. */

	// Raise Task 2 priority if it's still running


	if (!xTaskTerminated) {
				/* Code to be executed only if Task 2 is still running */
			
				vPrintString( "About to raise the Task 2 priority\r\n" );
				vTaskDelay(pdMS_TO_TICKS(1000));
				vTaskPrioritySet( xTask2Handle, ( uxPriority + 1 ) );
			}



	/* Task 1 will only run when it has a priority higher than Task 2.
	Therefore, for this task to reach this point, Task 2 must already have
	executed and set its priority back down to below the priority of this
	task. */
	}
}

void vTask2( void *pvParameters )
{
	UBaseType_t uxPriority;
	/* Task 1 will always run before this task as Task 1 is created with the
	higher priority. Neither Task 1 nor Task 2 ever block so will always be 
	in either the Running or the Ready state.
	Query the priority at which this task is running - passing in NULL means
	"return the calling task’s priority". */
	uxPriority = uxTaskPriorityGet( NULL );
	
	for( ;; )
	{
	/* For this task to reach this point Task 1 must have already run and
	set the priority of this task higher than its own.
	Print out the name of this task. */
	
	vPrintString( "Task 2 is running\r\n" );
	
	vPortGetHeapStats(&HeapStats);
	printHeapStats(&HeapStats, "Task2");

	if (HeapStats.xAvailableHeapSpaceInBytes < MIN_FREE_MEMORY_THRESHOLD){
		memoryWatchdog(xTask2Handle, "Task2");
	}

	void *allocatedMemoryTask2 = pvPortMalloc(20);
	
	/* Set the priority of this task back down to its original value. 
	Passing in NULL as the task handle means "change the priority of the 
	calling task". Setting the priority below that of Task 1 will cause 
	Task 1 to immediately start running again – pre-empting this task. */

	// Lower Task 2 priority
	if(!xTaskTerminated){
		vPrintString( "About to lower the Task 2 priority\r\n" );
		vTaskDelay(pdMS_TO_TICKS(1000));
		
		vTaskPrioritySet( NULL, ( uxPriority - 2 ) );
		}
	}
}

