# FreeRTOS Producer-Consumer Example

This is a simple example of a Producer-Consumer pattern implemented using FreeRTOS on an embedded system. In this example, two tasks (`producerTask` and `consumerTask`) communicate through a shared buffer protected by a mutex.

## Prerequisites

- [FreeRTOS](https://www.freertos.org/): Make sure you have FreeRTOS set up for your target environment.

## Usage

1. Clone the repository or download the source code.
2. Set up FreeRTOS for your target environment.
3. Compile and run the code on your target platform.

## Code Overview

The `main` function initializes the FreeRTOS environment, creates tasks, and starts the scheduler. The `producerTask` writes data to the shared buffer, and the `consumerTask` reads and prints the buffer value.

```c
void main(void) {
    // Initialization code...

    xTaskCreate(producerTask, "producer", 1000, (void *)42, 3, NULL);
    xTaskCreate(consumerTask, "consumer", 1000, NULL, 1, NULL);
    vTaskStartScheduler();

    // Main loop...
}

# Tasks
#Producer Task
#The 'producerTask' function writes data to the shared buffer.

```code
static void producerTask(void *pvParameters) {
    // Producer task code...

    buffer = argVal;
    printf("I'm the producer, writing %d on the buffer.\n", argVal);

    // More producer task code...

    vTaskDelete(NULL);
}


# Consumer Task
#The consumerTask function reads and prints the buffer value.

static void consumerTask(void *pvParameters) {
    // Consumer task code...

    printf("I'm the consumer, printing the buffer value: %d\n", buffer);

    // More consumer task code...

    vTaskDelete(NULL);
}



