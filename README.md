# UART Command Line with Interrupts

This code is designed to allow users to execute commands through the
UART console efficiently. The implementation features a UART command
line interface with interrupts, eliminating the need for
resource-intensive polling methods. This approach ensures that when the
user is not actively using the keyboard, the CPU can be utilized for
other tasks or transition to an IDLE state to conserve resources and
energy.

## Interrupt Handling

Upon asynchronous input of a character from the keyboard, the
`UART0RX_Handler` routine is activated to handle the reception. This
routine extracts the character from the UART data register and forwards
it to a FreeRTOS queue. `vCommandlineTask` will process them.

    void UART0RX_Handler(void) {
        // Check if RX interrupt occurred
        if (UART0_INTSTATUS == 2) {
            // Read character and send it to FreeRTOS queue
            char c = UART0_DATA;
            xQueueSendToBackFromISR(xQueueUART, &c, NULL);
            // Clear interrupt status
            UART0_INTSTATUS = 2;
        }
    }

## UART Initialization

The UART initialization function (`prvUARTInit`) handles the
configuration of the UART module, baud rate setup, and initialization of
relevant hardware registers. Additionally, it manages NVIC settings for
the UART RX interrupt and creates a FreeRTOS queue for UART data.

-   **BAUDDIV:** The divider is set to the minimum value (16) to achieve
    the maximum baud rate, ensuring faster character reception with
    fewer CPU cycles.

-   **CTRL:** The value 11 is employed to enable receiving,
    transmitting, and activate the RX interrupt.

-   **NVIC (Nested Vectored Interrupt Controller):** It efficiently
    handles interrupts, closely integrated with the processor core for
    low-latency processing. In the provided code snippet, we enable the
    UART RX handler to execute upon the arrival of a UART RX interrupt
    at the CPU (below we set inside the vector table our handler to be
    executed). The priority is configured to the highest logical value,
    respecting the reserved value designated for OS interrupts.

```{=html}
<!-- -->
```
    void prvUARTInit( void ) {
        // Set baud rate and configure UART control registers
        UART0_BAUDDIV = 16;
        UART0_CTRL = 11;

        // Configure NVIC settings for UART RX interrupt
        NVIC_SetPriority(UARTRX0_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);
        NVIC_EnableIRQ(UARTRX0_IRQn);

        // Create FreeRTOS queue for UART data
        xQueueUART = xQueueCreate(NORMALBUFLEN, sizeof(char));
    }

## Vector Table Update

For the effective handling of UART RX interrupts on our board, we need
to include the address of our `UART0RX_Handler` function in the relevant
row of the `isr_vector` array within the `startup_gcc.c` file. This
addition ensures that our custom handler to be invoked upon UART RX
interrupts.

    ( uint32_t * ) &UART0RX_Handler,

# CPU Usage Statistics

The utilization of CPU usage statistics is controlled by the
`DEBUG_WITH_STATS` flag, providing the flexibility to enable or disable
statistical functionality. This allows efficient resource management,
enabling the toggling of statistical features as needed during real-time
usage by setting the flag to 0. We will use the hardware TIMER0 to
increment a counter with a frequency of 10kHz. This counter will be used
to track the CPU usage of each task.

## Statistics Command Execution

`vCommandlineTask` function reads a string from UART and passes it to
the `executeCommand` function for processing. If the command is
\"stats\", `executeCommand` retrieves and displays runtime statistics.
This task is created with a larger stack when statistics processing is
enabled, to permit to have a sufficiently big buffer to contain all the
statistics.

Here is the implementation:

``` {.objectivec language="C"}
void main( void )
{
    #if (DEBUG_WITH_STATS==1)
    // Create vCommandlinetask with a larger stack for stats processing
    xTaskCreate(vCommandlineTask, "Commandline Task", configMINIMAL_STACK_SIZE+DEBUGSTATSBUFLEN+100, NULL, tskIDLE_PRIORITY + 1, NULL);
    #else
    // If we don't want run time stats, we can have a stack of ordinary size
    xTaskCreate(vCommandlineTask, "Commandline Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    #endif

    // ... 
}

static void vCommandlineTask(void *pvParameters) {
   
    while(1){
    
         // Logic for receiving character UART input and put it in inputString
        
        executeCommand(inputString);
    }
}

void executeCommand(char command[]) {
    #if (DEBUG_WITH_STATS==1)
    if (strcmp(command, "stats") == 0){
        // Create a suffiently BIG buffer for storing runtime statistics
        char buffer[DEBUGSTATSBUFLEN];

        // Retrieve runtime statistics and store in the buffer
        vTaskGetRunTimeStats(buffer);

        // Display task name, run time, and CPU usage
        printf("\nTask name\tRun time\tCPU usage\n%s\n", buffer);
    }
    #endif
}
```

## Timer 0 Initialization

The Timer 0 initialization function (`initializeTimer0`) is executed
when runtime statistics are enabled. The `RELOAD` value is crucial for
determining the interrupt frequency of Timer 0. In our configuration,
since `RELOAD` value is being decremented each clock cycle, if we put a
value equal to $\text{{configCPU\_CLOCK\_HZ}} / 10 \, \text{{kHz}}$, we
achieve a 10 kHz timer interrupt frequency. This is because, for
statistics tracking, it's suggested to have a 10 times faster frequency
than FreeRTOS SysTick (1khz).

``` {.objectivec language="C"}
void main( void )
{
    #if (DEBUG_WITH_STATS==1)
    // Initialize Timer 0 for runtime statistics (10 kHz interrupt frequency)
    initializeTimer0(2500);
    #endif

    // ... 
}

#if (DEBUG_WITH_STATS==1)
// Timer 0 initialization for runtime statistics
static void initializeTimer0(unsigned int ticks){
    // Clear Timer 0 interrupt
    CMSDK_TIMER0->INTCLEAR =  (1ul <<  0);

    // Set the reload value (10 kHz Timer 0 interrupt frequency)
    CMSDK_TIMER0->RELOAD = 2500;

    // Enable Timer 0 interrupt and Timer
    CMSDK_TIMER0->CTRL = ((1ul <<  3) | (1ul <<  0));

    // Enable Timer 0 interrupt in NVIC
    NVIC_EnableIRQ(TIMER0_IRQn);
}
#endif
```

## Timer 0 Handler

We modified the Timer 0 interrupt handler as follows:

``` {.objectivec language="C"}
void TIMER0_Handler( void )
{
    /* Clear interrupt. */
    CMSDK_TIMER0->INTCLEAR = ( 1ul <<  0 );

    ulNestCount++; // Increment the counter used for calculating statistics for each task 
}
```

## Vector Table Update

To instruct our board to execute `TIMER0_Handler` each time the TIMER0
peripheral generates an interrupt (every 10 kHz), it is crucial to
include the handler's address in the appropriate row of the `isr_vector`
array within the `startup_gcc.c` file.

``` {.objectivec language="C"}
#if (DEBUG_WITH_STATS==1)
( uint32_t * ) &TIMER0_Handler, // our handler modified for statistics
#endif
```

# Knight Rider LED Effect in FreeRTOS

The Knight Rider effect is implemented in the `vLEDTask` function, which
is a FreeRTOS task responsible for controlling the LEDs. The task is
suspended and resumed using a message queue and the \"led\" command
entered by the user through the UART console.

    static void vLEDTask(void *pvParameters) {
        (void)pvParameters; // Ignore the unused parameter

        // Knight Rider Effect
        while(1){
            vTaskSuspend(NULL);
            printf("\n");

            for(int i=0; i<=7; ++i){
                Switch_Led_On(i);
                printf("Led n: %d is on\n", i);
                vTaskDelay(100);
                Switch_Led_Off(i);
            }

            // Here we go to the other direction so we decrement
            for(int i=7; i>=0; --i){
                Switch_Led_On(i);
                printf("Led n: %d is on\n", i);
                vTaskDelay(100);
                Switch_Led_Off(i);
            }

            printf("\n");
        }
    }

## LED Control details

To manage the state of LEDs on this board, we use the `LED_PORT`
register. Here's how it's defined:

    #define LED_PORT    (MPS2_SCC->LEDS)

This corresponds to the CFGREG1 register within the Serial Communication
Controller (SCC) interface on the board. In CFGREG1, bits \[7:0\] are
dedicated to controlling the board LEDs, where setting a bit to 1 powers
the corresponding LED on, and setting it to 0 powers it off.

The code uses bitwise operations to control specific LED pins based on
the desired LED number through the `Switch_Led_On` and `Switch_Led_Off`
functions:

    void Switch_Led_On(int ledN){
        if (ledN <= 7 && ledN >= 0){
            LED_PORT |= (1U << ledN);    // Turn on the LED specified by ledN
        }
        // Note: Switch_Led_Off is analogous but with bitwise AND
        // LED_PORT &= ~(1U << ledN);   // Turn off the LED specified by ledN
    }