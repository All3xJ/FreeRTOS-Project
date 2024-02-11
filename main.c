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

void executeCommand( char command[] );
static void vCommandlineTask( void *pvParameter );

xQueueHandle xQueueUART;

void vTaskFunctionCmd(void *pvParameters);

void vFullDemoTickHookFunction( void );

static void prvUARTInit( void );
void UART0RX_Handler(void);

void vTaskStartMyScheduler( void );

void Task1(void *pvParameters);
void Task2(void *pvParameters);
void Task3(void *pvParameters);

void append(int *array, int size, int newElement);

int avgWait(int *array, int size);

TickType_t end1, end2, end3;

int finishedTasks[3];



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

	for (int i = 0; i < 3; ++i) {
        finishedTasks[i] = -1;
    }

	xTaskCreate(vCommandlineTask, "Commandline Task", configMINIMAL_STACK_SIZE * 5, NULL, tskIDLE_PRIORITY + 2, NULL);

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

// void vTaskFunction(void *pvParameters) {
//     const char *taskName = pcTaskGetName(NULL);
//     (void)pvParameters; // Ignora l'avviso di parametro non utilizzato
//     printf("La task %s in esecuzione!\r\n", taskName);
//     vTaskDelay(pdMS_TO_TICKS(1000)); // Attendi 1000 millisecondi
// 	vTaskDelete(NULL);
// }

static void vCommandlineTask(void *pvParameters) {
    (void)pvParameters; // ignore unused parameter warning

    char c;
    char inputString[NORMALBUFLEN];

    while (1) {
        int index = 0;

        printf("Select the following:\n\r");
        printf("1 - FCFS\n\r");
        printf("2 - SJF\n\r");
        printf("3 - for example n3\n\r");
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

        switch (choice) {
            case 1:
				printf("Selected FCFS\r\n");
				xTaskCreate(Task1, "Task1", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 1, NULL);
    			xTaskCreate(Task2, "Task2", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 1, NULL);
				xTaskCreate(Task3, "Task3", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 1, NULL);           
                break;
            case 2:
                printf("Selected SJF\r\n");
				xTaskCreate(Task2, "Task2", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 1, NULL);
    			xTaskCreate(Task3, "Task3", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 1, NULL);
				xTaskCreate(Task1, "Task1", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 1, NULL);      
                break;
            case 3:
                printf("\nSelected three\n");
                break;
            case 0:
                printf("\nExiting the cycle\n");
                vTaskDelete(NULL); // delete the task before returning
                break;
            default:
                printf("\nWrong selection\n");
        }
		vTaskDelay(1000);
		printf("avg time: %u\r\n", (end1+end2+end3)/3);
		printf("avg wait: %u\r\n", avgWait(finishedTasks, 3));
		printf("\r\n");
		for (int i = 0; i < 3; ++i) {
			finishedTasks[i] = -1;
		}
    }
}

/* --- List of tasks --- */

void Task1(void *pvParameters) {
	TickType_t start = xTaskGetTickCount();
	(void)pvParameters; // ignore unused parameter warning
	int res = 1;
	for (int i = 1; i < 40000000; ++i) {
		res *= i;
    }
	end1 = xTaskGetTickCount()-start;
	printf("Task1: %u\r\n", end1);
	append(finishedTasks, 3, end1);
	vTaskDelete(NULL); // delete the task before returning
}

// Task 2
void Task2(void *pvParameters) {
	TickType_t start = xTaskGetTickCount();
	(void)pvParameters; // ignore unused parameter warning
	int res = 1;
	for (int i = 1; i < 10000000; ++i) {
		res *= i;
    }
	end2 = xTaskGetTickCount()-start;
	printf("Task2: %u\r\n", end2);
	append(finishedTasks, 3, end2);
	vTaskDelete(NULL); // delete the task before returning
}

// Task 3
void Task3(void *pvParameters) {
	TickType_t start = xTaskGetTickCount();
	(void)pvParameters; // ignore unused parameter warning
	int res = 1;
	for (int i = 1; i < 20000000; ++i) {
		res *= i;
    }
	end3 = xTaskGetTickCount()-start;
	printf("Task3: %u\r\n", end3);
	append(finishedTasks, 3, end3);
	vTaskDelete(NULL); // delete the task before returning
}
/*
	FCFS --> just normal order, no preemption
	SJF --> order can be determined, no preemption

	RR --> no order just preempting a time quantum

	SRT --> like SJF but preemption cause task arrriving late

	RM --> pereempting based on period

	EDF --> gives higher priorites to deadline
*/

void append(int *array, int size, int newElement) {
    for (int i = 0; i < size; ++i) {
        if (array[i] == -1) {
            array[i] = newElement;
            return;
        }
    }
}

int avgWait(int *array, int size) {
	int res = 0;
    for (int i = 0; i < size-1; ++i) {
		res += array[i];
    }
	return res/size-1;
}