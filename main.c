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

/******************************************************************************
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.
 *
 * This file only contains the source code that is specific to the basic demo.
 * Generic functions, such FreeRTOS hook functions, are defined in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 (simulated) milliseconds.
 *
 * The Queue Send Software Timer:
 * The timer is an auto-reload timer with a period of two (simulated) seconds.
 * Its callback function writes the value 200 to the queue.  The callback
 * function is implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "portable.h"

/* Demo includes. */

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define mainQUEUE_SEND_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

/* The rate at which data is sent to the queue.  The times are converted from
milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS pdMS_TO_TICKS(200UL)
#define mainTIMER_SEND_FREQUENCY_MS pdMS_TO_TICKS(2000UL)

/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH (2)

/* The values sent to the queue receive task from the queue send task and the
queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK (100UL)
#define mainVALUE_SENT_FROM_TIMER (200UL)

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask(void *pvParameters);
static void prvQueueSendTask(void *pvParameters);

/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback(TimerHandle_t xTimerHandle);
static void producerTask(void *pvParameters);
static void consumerTask(void *pvParameters);
void writeBuffer(void *arg);

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/

/* The task to be created.  Two instances of this task are created. */
static void prvPrintTask(void *pvParameters);

/* The function that uses a mutex to control access to standard out. */
static void prvNewPrintString(const char *pcString);

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
SemaphoreHandle_t xMutex;
int *buffer;

/* The tasks block for a pseudo random time between 0 and xMaxBlockTime ticks. */
const TickType_t xMaxBlockTimeTicks = 0x20;

void main_blinky(void)
{
	xMutex = xSemaphoreCreateMutex();
	buffer = (int *)pvPortMalloc(sizeof(int));

	if (xMutex != NULL && buffer != NULL)
	{
		xTaskCreate(producerTask, "producer", 1000, (void *)42, 3, NULL);
		xTaskCreate(consumerTask, "consumer", 1000, NULL, 1, NULL);
		vTaskStartScheduler();
	}

	for (;;)
	{
		vApplicationIdleHook();
	};
}

/*-----------------------------------------------------------*/

static void prvQueueSendTask(void *pvParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
	const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for (;;)
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		/* Send to the queue - causing the queue receive task to unblock and
		write to the console.  0 is used as the block time so the send operation
		will not block - it shouldn't need to block as the queue should always
		have at least one space at this point in the code. */
		xQueueSend(xQueue, &ulValueToSend, 0U);
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback(TimerHandle_t xTimerHandle)
{
	const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	(void)xTimerHandle;

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	xQueueSend(xQueue, &ulValueToSend, 0U);
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask(void *pvParameters)
{
	uint32_t ulReceivedValue;

	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;

	for (;;)
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h.  It will not use any CPU time while it is in the
		Blocked state. */
		xQueueReceive(xQueue, &ulReceivedValue, portMAX_DELAY);

		/*  To get here something must have been received from the queue, but
		is it an expected value? */
		if (ulReceivedValue == mainVALUE_SENT_FROM_TASK)
		{
			/* It is normally not good to call printf() from an embedded system,
			although it is ok in this simulated case. */
			printf("Message received from task\r\n");
		}
		else if (ulReceivedValue == mainVALUE_SENT_FROM_TIMER)
		{
			printf("Message received from software timer\r\n");
		}
		else
		{
			printf("Unexpected message\r\n");
		}
	}
}
/*-----------------------------------------------------------*/

static void prvNewPrintString(const char *pcString)
{
	/* The semaphore is created before the scheduler is started so already
	exists by the time this task executes.

	Attempt to take the semaphore, blocking indefinitely if the mutex is not
	available immediately.  The call to xSemaphoreTake() will only return when
	the semaphore has been successfully obtained so there is no need to check the
	return value.  If any other delay period was used then the code must check
	that xSemaphoreTake() returns pdTRUE before accessing the resource (in this
	case standard out. */
	xSemaphoreTake(xMutex, portMAX_DELAY);
	{
		/* The following line will only execute once the semaphore has been
		successfully obtained - so standard out can be accessed freely. */
		printf("%s", pcString);
		fflush(stdout);
		vTaskDelay(1000);
	}
	xSemaphoreGive(xMutex);
	vTaskDelete(NULL);

	/* Allow any key to stop the application running.  A real application that
	actually used the key value should protect access to the keyboard too.  A
	real application is very unlikely to have more than one task processing
	key presses though! */
}
/*-----------------------------------------------------------*/

static void prvPrintTask(void *pvParameters)
{
	char *pcStringToPrint;
	const TickType_t xSlowDownDelay = pdMS_TO_TICKS(5UL);

	/* Two instances of this task are created.  The string printed by the task
	is passed into the task using the task's parameter.  The parameter is cast
	to the required type. */
	pcStringToPrint = (char *)pvParameters;

	for (;;)
	{
		/* Print out the string using the newly defined function. */
		vTaskDelay(1000);
		prvNewPrintString(pcStringToPrint);

		/* Wait a pseudo random time.  Note that rand() is not necessarily
		re-entrant, but in this case it does not really matter as the code does
		not care what value is returned.  In a more secure application a version
		of rand() that is known to be re-entrant should be used - or calls to
		rand() should be protected using a critical section. */
		vTaskDelay(1000);

		/* Just to ensure the scrolling is not too fast! */
		vTaskDelay(xSlowDownDelay);
	}
}

static void consumerTask(void *pvParameters)
{
	xSemaphoreTake(xMutex, portMAX_DELAY);
	{
		vTaskDelay(1000);
		printf("im the consumer, printing the buffer value: %d", buffer);
		vTaskDelay(1000);
	}
	xSemaphoreGive(xMutex);
}

static void producerTask(void *pvParameters)
{
	int argVal;
	argVal = (int)pvParameters;

	xSemaphoreTake(xMutex, portMAX_DELAY);
	{
		vTaskDelay(1000);
		buffer = argVal;
		printf("im the producer, writing \n\r%d\n\ron the buffer \n\r", argVal);
		vTaskDelay(2000);
	}
	xSemaphoreGive(xMutex);

	vTaskDelete(NULL);
}
