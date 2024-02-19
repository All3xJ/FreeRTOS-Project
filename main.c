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
#include "queue.h"
#include <SMM_MPS2.h>
#include "list.h"

#include "dictionaries.h" // custom lib to implement dictionaries

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* printf() output uses the UART.  These constants define the addresses of the
required UART registers. */
#define UART0_ADDRESS 	( 0x40004000UL )
#define UART0_DATA		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 0UL ) ) ) )
#define UART0_STATE		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 4UL ) ) ) )
#define UART0_CTRL		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 8UL ) ) ) )
#define UART0_BAUDDIV	( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 16UL ) ) ) )
#define TX_BUFFER_MASK	( 1UL )
#define UART0_INTSTATUS	( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 12UL ) ) ) )

#define NORMALBUFLEN	50					// size of a "general purpose" buffer

static void vCommandlineTask( void *pvParameter );

xQueueHandle xQueueUART;

// void vTaskFunctionCmd(void *pvParameters);

// void vFullDemoTickHookFunction( void );

static void prvUARTInit( void );
void UART0RX_Handler(void);

// Task just do some arithmetics calculation, the longer the number passed, the more complex the computation
void ComputingTask(void *pvParameters);

// Function used to append values to an array (last element defined by -1)
void append(int *array, int size, int newElement);

// Calculate average time of execution of all tasks
int avgTime(int *array, int size);
// Calculate average time waiting time of all tasks
int avgWait(int *array, int size);
// Calculate average turn-around time time of all tasks
int avgTurnaroundTime(int *array, int size);

// Calculates random values between 1000000 and 10000000
int randomCycle(int min, int max);

// Generates the values to pass to initalize tasks
void generateTasksParams(int* array, int size);

// Generates the values to pass to initalize tasks
void generateDeadlines(int* array, int size);

// Generating tasks with array of params as input
void CreateTasks();

void CreatePeriodic();

// Seed for randomness
unsigned int seed = 0;

#define numberOfTasks 10 // Change this value to change the numbers of tasks to run

#define numberOfPeriodicTasks 2

// Array used to store the time each task took to complete, first value, first task to finish
// int finishedTasks[numberOfTasks]; // replaced by dict

// Array used to store the random value to initialize tasks
int tasksParams[numberOfTasks];

// Array used to store the random deadlines to initialize tasks
int tasksDeadlines[numberOfTasks];

// Task for checking if the deadline have been respected
void CheckResTasks();

// Functions that generates params and deadlines given a seed as input
void doAllSeed(int* arrayTasks, int* arrayDeadlines, int size, unsigned int seed);

void ContextSwtichTask(void *pvParameters);

void ComputingTaskContextSwitch(void *pvParameters);

char* result[numberOfPeriodicTasks*10];

void ComputingTaskPeriodic1(void *pvParameters);

void ComputingTaskPeriodic2(void *pvParameters);

int p1 = 240;
int p2 = 330;

int resFlag = 0;

/*
 * Printf() output is sent to the serial port.  Initialise the serial hardware.
 */
/*-----------------------------------------------------------*/

void main( void )
{
	/* See https://www.freertos.org/freertos-on-qemu-mps2-an385-model.html for
	instructions. */

	/* Hardware initialisation.  printf() output uses the UART for IO. */
	prvUARTInit();

	// Initialize finishedTasks and tasksParams setting all values to -1
	// for (int i = 0; i < numberOfTasks; ++i) {
    //     finishedTasks[i] = -1;
    // }

	for (int i = 0; i < numberOfTasks; ++i) {
        tasksParams[i] = -1;
    }

	for (int i = 0; i < numberOfTasks; ++i) {
        tasksDeadlines[i] = -1;
    }

	// This is our command line task
	xTaskCreate(vCommandlineTask, "Commandline Task", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 2, NULL);

  	vTaskStartScheduler();

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

static void prvUARTInit( void ) {
	UART0_BAUDDIV = 16;
	UART0_CTRL = 11;	// this enables receving, transmitting and also enables RX interrupt

	NVIC_SetPriority(UARTRX0_IRQn,configMAX_SYSCALL_INTERRUPT_PRIORITY);	// needed because it is required by an assert in the "port.c" file in the "vPortValidateInterruptPriority" part. ISR interrupts must not have the same priority as FreeRTOS interrupts (so it is required that they are not 0 which is highest priority). so they must be numerically >= of configMAX_SYSCALL_INTERRUPT_PRIORITY (which is 5)
	NVIC_EnableIRQ(UARTRX0_IRQn);	// with this we are enabling an ISR for UART. in the vector table of "startup_gcc.c" we have set to use "UART0RX_Handler" which is defined here in "main.c" below

	xQueueUART = xQueueCreate(NORMALBUFLEN, sizeof(char));	// we create queue which is used to pass uart data received from isr to various tasks
}

void UART0RX_Handler(void) {
	if (UART0_INTSTATUS == 2){	// if second bit is at 1 it means there was an rx interrupt
		char c = UART0_DATA;	// we read character that was written by the user
		xQueueSendToBackFromISR(xQueueUART, &c, NULL);
		UART0_INTSTATUS = 2;	// we put second bit to 1 to clear the interrupt
	}
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

static void vCommandlineTask(void *pvParameters) {
    (void)pvParameters; // ignore unused parameter warning

    char c;
    char inputString[NORMALBUFLEN];

    while (1) {
        int index = 0;

        printf("Select the following:\n\r");
		printf("1 - Generate tasks, deadlines and executes tasks based on a input seed\n\r");
		printf("2 - Print if deadlines got missed or respected\n\r");
		printf("3 - Generate or regenerate the tasks\n\r");
        printf("4 - Generate or regenerate the deadlines\n\r");
        printf("5 - Execute the tasks\n\r");
		printf("6 - Context switch late task test\n\r");
		printf("7 - Periodic tasks example\r\n");
        printf("0 - to exit\n\r");

        while (index < NORMALBUFLEN - 1) {
            if (xQueueReceive(xQueueUART, &c, portMAX_DELAY) == pdTRUE) {
                printf("%c", c); // echo the character

                if (c == '\r') { // abort if '\r' is entered, i.e., if enter is given
                    break;
                }

                inputString[index] = c; // add the character to the string
                index++;
            }
        }

        inputString[index] = '\0'; // add string terminator

        int choice = atoi(inputString); // convert string to integer

		// around 70 ticks every 10000000 cycle

		// example 6
		int *params = (int *)pvPortMalloc(sizeof(int));
		params[0] = 10000000;

		// example 7
		int *params2 = (int *)pvPortMalloc(sizeof(int));
		params2[0] = 10000000;
		int *params3 = (int *)pvPortMalloc(sizeof(int));
		params3[0] = 20000000;

        switch (choice) {
			case 1:
				printf("Provide a seed as input\r\n");
				int indexSeed = 0;
				char d;
    			char inputSeed[NORMALBUFLEN];
				while (index < NORMALBUFLEN - 1) {
					if (xQueueReceive(xQueueUART, &d, portMAX_DELAY) == pdTRUE) {
						printf("%c", d); // echo the character

						if (d == '\r') { // abort if '\r' is entered, i.e., if enter is given
							break;
						}

						inputSeed[indexSeed] = c; // add the character to the string
						indexSeed++;
					}
				}
				sizeStarting = 0;
				sizeFinished = 0;
				doAllSeed(tasksParams, tasksDeadlines, numberOfTasks, (unsigned int)inputSeed);
				CreateTasks();
				resFlag = 2;
				break;
			case 2:
				CheckResTasks();
				break;
			case 3:
				generateTasksParams(tasksParams, numberOfTasks);					
				printf("Params generated\r\n");
				printf("\r\n");
				break;
            case 4:
				generateDeadlines(tasksDeadlines, numberOfTasks);
				printf("Deadlines generated\r\n");
				printf("\r\n");
                break;
            case 5:
				if (tasksParams[0] == -1) {
					printf("First of all generate some tasks with options 1\r\n");
					printf("\r\n");
					break;
				}
				if (tasksDeadlines[0] == -1) {
					printf("First of all generate some deadlines with options 2\r\n");
					printf("\r\n");
					break;
				}
				sizeStarting = 0;
				sizeFinished = 0;
				CreateTasks();  
                break;
			case 6:
				printf("Scheduling task with 200 ticks as deadline\r\n");			
				xTaskCreateDeadline(ComputingTaskContextSwitch, "StartingT", configMINIMAL_STACK_SIZE * 10, params, tskIDLE_PRIORITY + 1, NULL, 200);
				break;
			case 7:
				sizeStarting = 0;
				sizeFinished = 0;
				xTaskCreateDeadline(ComputingTaskPeriodic1, "T1", configMINIMAL_STACK_SIZE * 10, params2, tskIDLE_PRIORITY + 1, NULL, p1*1);
				xTaskCreateDeadline(ComputingTaskPeriodic2, "T2", configMINIMAL_STACK_SIZE * 10, params3, tskIDLE_PRIORITY + 1, NULL, p2*1);
				resFlag = 1;
				break;
            case 0:
                printf("\nExiting the cycle\n");
                vTaskDelete(NULL); // delete the task before returning
                break;
            default:
                printf("\nWrong selection\n");
        }
		vTaskDelay(2200);
    }
}

/* --- List of tasks --- */

void ComputingTask(void *pvParameters) {
    TickType_t start = xTaskGetTickCount();
    int n = *((int*)pvParameters);
	const char *taskName = pcTaskGetName(NULL);
	printf("%s start time: %u\r\n", taskName, (int)start);
    int res = 1;

    for (int i = 1; i <= n; ++i) {
        res *= i;
    }

    TickType_t end = xTaskGetTickCount();
	printf("%s end time: %d\r\n", taskName, (int)end);
    printf("%s elapsed time: %d\r\n", taskName, (int)(end - start));
	insertFinished(taskName, (int)end);
    vTaskDelete(NULL); // delete the task before returning
}

void append(int *array, int size, int newElement) {
    for (int i = 0; i < size; ++i) {
        if (array[i] == -1) {
            array[i] = newElement;
            return;
        }
    }
}

int avgTime(int *array, int size) {
	int res = 0;
    for (int i = 0; i < size; ++i) {
		res += array[i];
    }
	return res/size;
}

int avgWait(int *array, int size) {
	int res = 0;
    for (int i = 0; i < size-1; ++i) {
		res += array[i] * (size-i-1);
    }
	return res/size;
}

int avgTurnaroundTime(int *array, int size) {
	int res = 0;
    for (int i = 0; i < size; ++i) {
		res += array[i] * (size-i);
    }
	return res/size;
}

int randomCycle(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void generateTasksParams(int* array, int size) {
	seed = (unsigned int)xTaskGetTickCount();

    for (int i = 0; i < size; i++) {
        array[i] = 1000000 + rand_r(&seed) % (10000000 - 1000000 + 1); // Parameters to "play" with to change task durations
    }
}

void generateDeadlines(int* array, int size) {
	seed = (unsigned int)xTaskGetTickCount();

    for (int i = 0; i < size; i++) {
        array[i] = 200 + rand_r(&seed) % (500 - 200 + 1); // Parameters to "play" with to change deadlines
    }
}

void CreateTasks() {
	for (int i = 0; i < numberOfTasks; ++i) {
        int *params = (int *)pvPortMalloc(sizeof(int));
        *params = tasksParams[i];

        char taskName[10];
        snprintf(taskName, sizeof(taskName), "Task%d", i + 1);

        xTaskCreateDeadline(ComputingTask, taskName, configMINIMAL_STACK_SIZE * 10, params, tskIDLE_PRIORITY + 1, NULL, tasksDeadlines[i]);
    }
}

void doAllSeed(int* arrayTasks, int* arrayDeadlines, int size, unsigned int seed) {
	for (int i = 0; i < size; i++) {
        arrayTasks[i] = 1000000 + rand_r(&seed) % (10000000 - 1000000 + 1); // Parameters to "play" with to change task durations
    }
	int sum = 0;
	for (int i = 0; i < size; i++) {
        arrayDeadlines[i] = 200 + rand_r(&seed) % (500 - 200 + 1); // Parameters to "play" with to change deadlines
		sum+=arrayDeadlines[i];
		// printf("%d\r\n", sum);
    }
}

void ContextSwtichTask(void *pvParameters) {
	(void)pvParameters;
	const char *taskName = pcTaskGetName(NULL);
	printf("Context switch happend in %s ticks as deadline\r\n", taskName);
	vTaskDelete(NULL);
}

void ComputingTaskContextSwitch(void *pvParameters) {
    TickType_t start = xTaskGetTickCount();
    int n = *((int*)pvParameters);
	const char *taskName = pcTaskGetName(NULL);
	printf("%s start time: %u\r\n", taskName, (int)start);
    int res = 1;

	for (int i = 1; i <= 10000000; ++i) {
        res *= i;
    }

	printf("After some computation, current task deadline: %d\r\n",  200 + (int)start - (int)xTaskGetTickCount());

	int *params = (int *)pvPortMalloc(sizeof(int));
	params[0] = 10000000;
	printf("Scheduling task with 150 ticks as deadline\r\n");
	xTaskCreateDeadline(ContextSwtichTask, "Task - 150", configMINIMAL_STACK_SIZE * 5, params, tskIDLE_PRIORITY + 2, NULL, 150);
	printf("Scheduling task with 10 ticks as deadline\r\n");
	xTaskCreateDeadline(ContextSwtichTask, "Task - 10", configMINIMAL_STACK_SIZE * 5, params, tskIDLE_PRIORITY + 2, NULL, 10);

    for (int i = 1; i <= n; ++i) {
        res *= i;
    }

    TickType_t end = xTaskGetTickCount() - start;
	printf("%s end time: %u\r\n", taskName, (int)xTaskGetTickCount());
    printf("%s elapsed time: %u\r\n", taskName, (int)end);
	insertFinished(taskName, (int)end);
    vTaskDelete(NULL); // delete the task before returning
}

void CreatePeriodic() {
	for (int i = 1; i < 6; i++) {
		for (int j = 0; j < numberOfPeriodicTasks; j++) {
			int *params = (int *)pvPortMalloc(sizeof(int));
        	*params = tasksParams[j];

			char taskName[10];
        	snprintf(taskName, sizeof(taskName), "Task%d - %d", j + 1, i);

			xTaskCreateDeadline(ComputingTask, taskName, configMINIMAL_STACK_SIZE * 10, params, tskIDLE_PRIORITY + 1, NULL, tasksDeadlines[numberOfPeriodicTasks]*i);
		}	
	}
}

void ComputingTaskPeriodic1(void *pvParameters) {
    TickType_t xLastWakeTime;

    // Initialize xLastWakeTime with the current time
    xLastWakeTime = xTaskGetTickCount();

    for (int j = 0; j < 10; j++) {
        TickType_t start = xTaskGetTickCount();
		int n = *((int*)pvParameters);
		const char *taskName = pcTaskGetName(NULL);
		printf("%s start time: %u\r\n", taskName, (int)start);
		int res = 1;

		for (int i = 1; i <= n; ++i) {
			res *= i;
		}

		TickType_t end = xTaskGetTickCount();
		printf("%s end time: %d\r\n", taskName, (int)end);
		printf("%s elapsed time: %d\r\n", taskName, (int)(end - start));
		char taskNameDict[10];
		snprintf(taskNameDict, sizeof(taskNameDict), "Task1-%d", j+1);
		insertFinished(taskNameDict, (int)end);
        vTaskDelayUntil(&xLastWakeTime, 180);
    }
	vTaskDelete(NULL); // delete the task before returning
}

void ComputingTaskPeriodic2(void *pvParameters) {
    TickType_t xLastWakeTime;

    // Initialize xLastWakeTime with the current time
    xLastWakeTime = xTaskGetTickCount();

    for (int j = 0; j < 8; j++) {
        TickType_t start = xTaskGetTickCount();
		int n = *((int*)pvParameters);
		const char *taskName = pcTaskGetName(NULL);
		printf("%s start time: %u\r\n", taskName, (int)start);
		int res = 1;

		for (int i = 1; i <= n; ++i) {
			res *= i;
		}

		TickType_t end = xTaskGetTickCount();
		printf("%s end time: %d\r\n", taskName, (int)end);
		printf("%s elapsed time: %d\r\n", taskName, (int)(end - start));
		char taskNameDict[10];
		snprintf(taskNameDict, sizeof(taskNameDict), "Task2-%d", j+1);
		insertFinished(taskNameDict, (int)end);
        vTaskDelayUntil(&xLastWakeTime, 260);
    }
	vTaskDelete(NULL); // delete the task before returning
}

void CheckResTasks() {
	getAllKeysFinished(result);

	printMapStarting();

	if (resFlag == 1) {
		int ctr_t1 = 0;
		int ctr_t2 = 0;
		for (int i = 0; i < numberOfPeriodicTasks * 10; ++i) {

			const char* taskName = result[i];

			if (strstr(result[i], "Task1-") != NULL) {
				ctr_t1++;
				if ((getFinished(taskName) - getStarting("T1")) > p1 * ctr_t1) {
					printf("%s has missed the deadline of %d\r\n", result[i], p1 * ctr_t1);
				} else {
					printf("%s has respected the deadline of %d\r\n", result[i], p1 * ctr_t1);
				}
			} else if (strstr(result[i], "Task2-") != NULL) {
				ctr_t2++;
				if ((getFinished(taskName) - getStarting("T2")) > p2 * ctr_t2) {
					printf("%s has missed the deadline of %d\r\n", result[i], p2 * ctr_t2);
				} else {
					printf("%s has respected the deadline of %d\r\n", result[i], p2 * ctr_t2);
				}
			}
		}
	} else if (resFlag == 2) {
		for (int i = 0; i < numberOfTasks; ++i) {
			char taskName[20];
			snprintf(taskName, sizeof(taskName), "Task%d", i + 1);

			if ((getFinished(taskName) - getStarting(taskName)) > tasksDeadlines[i])
			{
				printf("%s has missed the deadline\r\n", taskName);
			}
			else
			{
				printf("%s has respected the deadline\r\n", taskName);
			}
		}
	}
}