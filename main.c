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
#include "timers.h"
#include "queue.h"

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SMM_MPS2.h> 


/* printf() output uses the UART.  These constants define the addresses of the
required UART registers. */
#define UART0_ADDRESS (0x40004000UL)
#define UART0_DATA (*(((volatile uint32_t *)(UART0_ADDRESS + 0UL))))
#define UART0_STATE (*(((volatile uint32_t *)(UART0_ADDRESS + 4UL))))
#define UART0_CTRL (*(((volatile uint32_t *)(UART0_ADDRESS + 8UL))))
#define UART0_BAUDDIV (*(((volatile uint32_t *)(UART0_ADDRESS + 16UL))))
#define TX_BUFFER_MASK (1UL)

#define DEFAULT_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

// Definition of tolerance thresholds
#define TEMPERATURE_THRESHOLD 40
#define HUMIDITY_THRESHOLD 65

#define LED_REGISTER (MPS2_SCC->LEDS)

//handle for the notifications
static TaskHandle_t xTemperatureNotificationHandlerTask; //handler for temperature notifications
static TaskHandle_t xHumidityNotificationHandlerTask; //handler for humidity notifications

// Declaration of task function
static void vSensorReadTask(void *pvParameters);
static void vTemperatureNotificationHandlerTask(void *pvParameters); //this task wakes up when temperature value exceed thresholds
static void vHumidityNotificationHandlerTask(void *pvParameters); //this task wakes up when humidity value exceed thresholds
static void vLedTask(void *pvParameters);

void Switch_On_First_Half_Leds();  //called when temperature value exceed thresholds
void Switch_On_Second_Half_Leds(); //called when humidity value exceed thresholds
void printLeds();
void Switch_All_Led_Off(); 

static void prvUARTInit(void);
void initializeLED(void);

void vFullDemoTickHookFunction(void);

/*-----------------------------------------------------------*/

void main(void)
{
	/* See https://www.freertos.org/freertos-on-qemu-mps2-an385-model.html for
	instructions. */

	/* Hardware initialisation.  printf() output uses the UART for IO. */
	prvUARTInit();
	initializeLED(); 


	// Task for handle the notifications
	xTaskCreate(vTemperatureNotificationHandlerTask, "TemperatureNotificationHandler", configMINIMAL_STACK_SIZE, NULL, DEFAULT_TASK_PRIORITY, &xTemperatureNotificationHandlerTask);
	xTaskCreate(vHumidityNotificationHandlerTask, "HumidityNotificationHandler", configMINIMAL_STACK_SIZE, NULL, DEFAULT_TASK_PRIORITY, &xHumidityNotificationHandlerTask);

	// Task that reads sensor (simulated with pseudorandom number)
	xTaskCreate(vSensorReadTask, "SensorReader", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

	xTaskCreate(vLedTask, "LedTask", configMINIMAL_STACK_SIZE, NULL, DEFAULT_TASK_PRIORITY, NULL);

	//Start exec of OS
	vTaskStartScheduler();
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
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
	printf("\r\n\r\nMalloc failed\r\n");
	portDISABLE_INTERRUPTS();
	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
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

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
	(void)pcTaskName;
	(void)pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	printf("\r\n\r\nStack overflow in %s\r\n", pcTaskName);
	portDISABLE_INTERRUPTS();
	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void)
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook(void)
{
	/* This function will be called once only, when the daemon task starts to
	execute (sometimes called the timer task).  This is useful if the
	application includes initialisation code that would benefit from executing
	after the scheduler has been started. */
}
/*-----------------------------------------------------------*/

void vAssertCalled(const char *pcFileName, uint32_t ulLine)
{
	volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

	/* Called if an assertion passed to configASSERT() fails.  See
	http://www.freertos.org/a00110.html#configASSERT for more information. */

	printf("ASSERT! Line %d, file %s\r\n", (int)ulLine, pcFileName);

	taskENTER_CRITICAL();
	{
		/* You can step out of this function to debug the assertion by using
		the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
		value. */
		while (ulSetToNonZeroInDebuggerToContinue == 0)
		{
			__asm volatile("NOP");
			__asm volatile("NOP");
		}
	}
	taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
	/* If the buffers to be provided to the Idle task are declared inside this
	function then they must be declared static - otherwise they will be allocated on
	the stack and so not exists after this function exits. */
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

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
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
	/* If the buffers to be provided to the Timer task are declared inside this
	function then they must be declared static - otherwise they will be allocated on
	the stack and so not exists after this function exits. */
	static StaticTask_t xTimerTaskTCB;
	static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

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
static void prvUARTInit(void)
{
	UART0_BAUDDIV = 16;
	UART0_CTRL = 1;
}
/*-----------------------------------------------------------*/

int __write(int iFile, char *pcString, int iStringLength)
{
	int iNextChar;

	/* Avoid compiler warnings about unused parameters. */
	(void)iFile;

	/* Output the formatted string to the UART. */
	for (iNextChar = 0; iNextChar < iStringLength; iNextChar++)
	{
		while ((UART0_STATE & TX_BUFFER_MASK) != 0)
			;
		UART0_DATA = *pcString;
		pcString++;
	}

	return iStringLength;
}
/*-----------------------------------------------------------*/
void *malloc(size_t size)
{
	(void)size;

	/* This project uses heap_4 so doesn't set up a heap for use by the C
	library - but something is calling the C library malloc().  See
	https://freertos.org/a00111.html for more information. */
	printf("\r\n\r\nUnexpected call to malloc() - should be usine pvPortMalloc()\r\n");
	portDISABLE_INTERRUPTS();
	for (;;)
		;
}

void initializeLED(void)
{
	LED_REGISTER = 0U; //set to 0 the contents of the address LED_REGISTER -> leds are turned off
}


static void vSensorReadTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(5000); //read values every 5 seconds

    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        //Use the tick count as a seed
        unsigned int seed = (unsigned int)xTaskGetTickCount();

        //Generate random numbers for temperature and humidity using rand_r()
        int temperature = (rand_r(&seed) % (50 - 10 + 1)) + 10; //Range between 10 and 50
        printf("The value of temperature is %d\n", temperature);

        int humidity = (rand_r(&seed) % (90 - 20 + 1)) + 20; //Range between 20 and 90
        printf("The value of humidity is %d\n", humidity);

        //Check if values exceed thresholds

        if (temperature > TEMPERATURE_THRESHOLD)
        {
            xTaskNotifyGive(xTemperatureNotificationHandlerTask); //Send temperature notification
        }
		
        if (humidity > HUMIDITY_THRESHOLD)
        {
            xTaskNotifyGive(xHumidityNotificationHandlerTask); //Send humidity notification
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

static void vTemperatureNotificationHandlerTask(void *pvParameters)
{
	(void)pvParameters;

	while(1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	
		printf("The temperature has exceeded the threshold\n");
		Switch_On_First_Half_Leds();
		printLeds();
		
	}

}

static void vHumidityNotificationHandlerTask(void *pvParameters)
{
	(void)pvParameters;

	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		printf("The humidity has exceeded the threshold\n");
		Switch_On_Second_Half_Leds();
		printLeds();

	}
}

void vLedTask(void *pvParameters) {

	(void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10000); //every 10 seconds

    while (1) 
	{
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        Switch_All_Led_Off();
    }
}

static void Switch_On_First_Half_Leds()
{
	//Calculate the mask to turn on only the first half of the LEDs
	uint8_t mask = (1 << 4) - 1;

	//Turn on only the first half of the LEDs
	LED_REGISTER |= mask;
}

void Switch_On_Second_Half_Leds()
{
	//Calculate the mask to turn on only the second half of the LEDs
	uint8_t mask = ((1 << 4) - 1) << (4);

	//Turn on only the second half of the LEDs	
	LED_REGISTER |= mask; 
}

void Switch_All_Led_Off(){
	// Turns off all LEDs in the last 7 bits of the LED_REGISTER register
	LED_REGISTER &= ~0xFF;
}

void printLeds()
{
	printf("╭───────────────╮\n");
	for (int ledN = 0; ledN <= 7; ++ledN)
	{
		printf("│%s", (LED_REGISTER & (1U << ledN)) ? "*" : " ");
	}
	printf("│\n");
	printf("╰───────────────╯\n");
}