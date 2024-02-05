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
#include "semphr.h"

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <SMM_MPS2.h>

/* printf() output uses the UART.  These constants define the addresses of the
required UART registers. */
#define UART0_ADDRESS 	( 0x40004000UL )
#define UART0_DATA		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 0UL ) ) ) )
#define UART0_STATE		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 4UL ) ) ) )
#define UART0_CTRL		( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 8UL ) ) ) )
#define UART0_BAUDDIV	( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 16UL ) ) ) )
#define TX_BUFFER_MASK	( 1UL )
#define UART0_INTSTATUS	( * ( ( ( volatile uint32_t * )( UART0_ADDRESS + 12UL ) ) ) )

#define LED_PORT		(MPS2_SCC->LEDS)	// this is the address of the led port used to control the leds

#define NORMALBUFLEN	50					// size of a "general purpose" buffer

#if (DEBUG_WITH_STATS==1)
	#define DEBUGSTATSBUFLEN 800			// we will use the buffer to enter task stats so it needs to be large enough

#endif

#define DEFAULT_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )	// tskIDLE_PRIORITY=0 which is the lowest priority. We set the default priority to 1, so that IDLE task can never preempt the other tasks.


/*
 * Printf() output is sent to the serial port.  Initialise the serial hardware.
 */
static void prvUARTInit( void );
void UART0RX_Handler(void);
void initializeLED(void);
#if (DEBUG_WITH_STATS==1)
static void initializeTimer0(unsigned int ticks);	// we use timer only for stats, if we don't want stats then it's not used and so we don't need to initialize it.
#endif

#if (DEBUG_WITH_STATS==1) && (configUSE_TICKLESS_IDLE==1)	// if user has modified the FreeRTOSConfig.h file and enabled both (not recommended since it doesn't make any sense to have both enabled)
void busyWait(unsigned int secondsToWait);
#endif

void Switch_Led_On (int ledN);
void Switch_Led_Off (int ledN);
void Switch_All_Led_On();
void Switch_All_Led_Off();
void LEDKnightRider();
void LEDConstantBlink();
void printLEDs ();
static void vLEDTask( void *pvParameter );

void executeCommand( char command[] );
static void vCommandlineTask( void *pvParameter );

xQueueHandle xQueueUART;

TaskHandle_t xHandleLED;	// we need it so that we can control the led task. so that we can resume it (since it pauses every time it completes the Knight Rider animation)

/*-----------------------------------------------------------*/

void main( void )
{
	/* See https://www.freertos.org/freertos-on-qemu-mps2-an385-model.html for
	instructions. */

	/* Hardware initialisation.  printf() output uses the UART for IO. */
	prvUARTInit();
	initializeLED();
	#if (DEBUG_WITH_STATS==1)
	initializeTimer0(2500);	// we use timer only for stats, if we don't want stats then it's not used and so we don't need to initialize it. 2500 is the result of configCPU_CLOCK_HZ/10khz = 25mhz/10khz to obtain a timer interrupt of 10khz because it should be 10x faster than FreeRTOS tick (which is 1khz)
	#endif

	#if (DEBUG_WITH_STATS==1)	// if we want stats, then we need a larger stack to hold the buffer (DEBUGSTATSBUFLEN) + 100 for any other variables used
	xTaskCreate(vCommandlineTask, "Commandline Task", configMINIMAL_STACK_SIZE+DEBUGSTATSBUFLEN+100, NULL, DEFAULT_TASK_PRIORITY, NULL);
	#else
	xTaskCreate(vCommandlineTask, "Commandline Task", configMINIMAL_STACK_SIZE, NULL, DEFAULT_TASK_PRIORITY, NULL);
	#endif

	xTaskCreate(vLEDTask, "Led Task", configMINIMAL_STACK_SIZE, NULL, DEFAULT_TASK_PRIORITY, &xHandleLED);

  	vTaskStartScheduler();

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
	UART0_BAUDDIV = 16;	// we set the divider at minimum possibile=16. so we have max baud rate (less cpu cycle to get character, so faster)
	UART0_CTRL = 11;	// this enables receving, transmitting and also enables RX interrupt
	
	NVIC_SetPriority(UARTRX0_IRQn,configMAX_SYSCALL_INTERRUPT_PRIORITY);	// needed because it is required by an assert in the "port.c" file in the "vPortValidateInterruptPriority" part. From freertos website: "FreeRTOS functions that end in "FromISR" are interrupt safe, but even these functions cannot be called from interrupts that have a logical priority above the priority defined by configMAX_SYSCALL_INTERRUPT_PRIORITY"... and in our uart interrupt handler we execute a xQueueSendToBackFromISR function so we need to respect this rule... also since interrupts have "inverse" priority(in contrast to FreeRTOS' Tasks priorities where numerically higher is logically higher), in this case it must be numerically >= of configMAX_SYSCALL_INTERRUPT_PRIORITY (which is 5).
	NVIC_EnableIRQ(UARTRX0_IRQn);	// with this we are enabling an ISR for UART. in the vector table of "startup_gcc.c" we have set to use "UART0RX_Handler" which is defined here in "main.c" below
	
	xQueueUART = xQueueCreate(NORMALBUFLEN, sizeof(char));	// we create queue which is used to pass uart data received from isr to various tasks
}
/*-----------------------------------------------------------*/

// int __write( int iFile, char *pcString, int iStringLength )
// {
// 	int iNextChar;

// 	/* Avoid compiler warnings about unused parameters. */
// 	( void ) iFile;

// 	/* Output the formatted string to the UART. */
// 	for( iNextChar = 0; iNextChar < iStringLength; iNextChar++ )
// 	{
// 		while( ( UART0_STATE & TX_BUFFER_MASK ) != 0 );
// 		UART0_DATA = *pcString;
// 		pcString++;
// 	}

// 	return iStringLength;
// }
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






void UART0RX_Handler(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (UART0_INTSTATUS == 2){	// if second bit is at 1 it means there was an rx interrupt
		char c = UART0_DATA;	// we read character that was written by the user
		xQueueSendToBackFromISR(xQueueUART, &c, &xHigherPriorityTaskWoken);
		UART0_INTSTATUS = 2;	// we put second bit to 1 to clear the interrupt
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);	// request context-switch to run a task without waiting for next SysTick
}





void initializeLED(void){
	LED_PORT = 0U;	// we point to the contents of the address LED_PORT and set to 0 so that leds are turned off
}

#if (DEBUG_WITH_STATS==1)	// we use timer only for stats, if we don't want stats then it's not used and so we don't need to initialize it.
static void initializeTimer0(unsigned int ticks){
	CMSDK_TIMER0->INTCLEAR =  (1ul <<  0);                   /* clear interrupt */
	CMSDK_TIMER0->RELOAD   =  ticks ;		 				 /* set reload value.... because counter is decremented and when 0 is reached, an interrupt is generated, and then this "reload" value is loaded... ticks = configCPU_CLOCK_HZ/10khz = 25mhz/10khz to obtain a timer interrupt of 10khz because it should be 10x faster than FreeRTOS tick (which is 1khz) */
	CMSDK_TIMER0->CTRL     = ((1ul <<  3) |                  /* enable Timer interrupt */
							 (1ul <<  0) );          	     /* enable Timer */

	NVIC_EnableIRQ(TIMER0_IRQn);                             /* Enable interrupt in NVIC */
}
#endif




#if (DEBUG_WITH_STATS==1) && (configUSE_TICKLESS_IDLE==1)	// if user has modified the FreeRTOSConfig.h file and enabled both (not recommended since it doesn't make any sense to have both enabled)
void busyWait(unsigned int secondsToWait){
	unsigned long stop = secondsToWait*configCPU_CLOCK_HZ;
	for(unsigned long i=0;i<stop;++i);
}
#endif


void Switch_Led_On (int ledN){
	if (ledN <= 7 && ledN >=0){		// we check if we have received correct input
		LED_PORT |=  (1U << ledN);	// we set the LED pin to high level, causing it to turn on
	}
}

void Switch_Led_Off (int ledN){
	if (ledN <= 7 && ledN >=0){		// we check if we have received correct input
		LED_PORT &= ~(1U << ledN);	// we set the led pin low, causing it to turn off
	}
}

void Switch_All_Led_On(){
	// Turns on all LEDs in the last 7 bits of the LED_PORT register
	LED_PORT |= 0xFF;
}

void Switch_All_Led_Off(){
	// Turns off all LEDs in the last 7 bits of the LED_PORT register
	LED_PORT &= ~0xFF;
}

void printLEDs(){	// silly function to draw leds checking if they are powered on.
	printf("╔═══════════════╗\n");
	for (int ledN = 0; ledN <= 7; ++ledN) {	// there is a total of 8 LEDs
		printf("║%s", (LED_PORT & (1 << ledN)) ? "X" : " ");
	}
	printf("║\n");
	printf("╚═══════════════╝\n");
}

#if (DEBUG_WITH_STATS==1) && (configUSE_TICKLESS_IDLE==1)	// if user has modified the FreeRTOSConfig.h file and enabled both (not recommended since it doesn't make any sense to have both enabled)
void LEDKnightRider(){
	for(int i=0;i<=7;++i){
		Switch_Led_On(i);
		printLEDs();
		busyWait(4);
		Switch_Led_Off(i);
	}
	for(int i=7;i>=0;--i){
		Switch_Led_On(i);
		printLEDs();
		busyWait(4);
		Switch_Led_Off(i);
	}
}

void LEDConstantBlink(){
	for(int i=0;i<5;++i){	// we repeat it 5 times
		Switch_All_Led_On();
		printLEDs();
		busyWait(4);
		Switch_All_Led_Off();
		printLEDs();
		busyWait(4);
	}
}
#else
void LEDKnightRider(){
	for(int i=0;i<=7;++i){
		Switch_Led_On(i);
		printLEDs();
		vTaskDelay(1000);
		Switch_Led_Off(i);
	}
	for(int i=7;i>=0;--i){
		Switch_Led_On(i);
		printLEDs();
		vTaskDelay(1000);
		Switch_Led_Off(i);
	}
}

void LEDConstantBlink(){
	for(int i=0;i<5;++i){	// we repeat it 5 times
		Switch_All_Led_On();
		printLEDs();
		vTaskDelay(1000);
		Switch_All_Led_Off();
		printLEDs();
		vTaskDelay(1000);
	}
}
#endif

static void vLEDTask(void *pvParameters) {
	(void)pvParameters;	// ignore unused parameter warning

	while(1){
		vTaskSuspend(NULL);	// task everytime is going to sleep. will be resumed if user enters "led" in commandline
		printf("\n");

		LEDKnightRider();	// simple Knight Rider light effect
		LEDConstantBlink();	// simple Constant Blink light effect

		printf("\n");
	}
}

void executeCommand(char command[]){	// is executed in the vCommandlineTask task, whenever user gives send to a string in the uart
	
	#if (DEBUG_WITH_STATS==1)	// stats accessible only in debugging, as it is a resource-consuming and probably almost useless feature at the time of actual use of an embedded system.
	if (strcmp(command,"stats")==0){
		char buffer[DEBUGSTATSBUFLEN];	// we create a sufficiently large buffer
		vTaskGetRunTimeStats(buffer);	// we take info and save it in the buffer. this function vTaskGetRunTimeStats needs many flags changed in various header files
		printf("\nTask name\tRun time\tPercentage\tTask state\n%s\n",buffer);
	}
	#endif

	if (strcmp(command,"led")==0){
		vTaskResume(xHandleLED);
	}
}

static void vCommandlineTask(void *pvParameters) {
    (void)pvParameters; // ignore unused parameter warning

	char c;
	char inputString[NORMALBUFLEN];  
	while(1){
		int index = 0;
		while (index < NORMALBUFLEN-1) {
			xQueueReceive(xQueueUART, &c, portMAX_DELAY);	// puts the task in Blocked state and wait for the character received from the ISR
			printf("%c",c);				// we echo the character

			if (c == '\r') {			// abort if '\r' is entered, i.e., if enter is given
				break;
			}

			inputString[index] = c;		// add the character to the string
			index++;
   		}
    	inputString[index] = '\0';		// add string terminator
		
		printf("Received string: %s\n", inputString);
		executeCommand(inputString);
	}
}


