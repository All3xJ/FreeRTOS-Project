# Scheduling with no preemption

In this project the task scheduler doesn't implement preemption and the tasks are scheduled with the same priority.

`#define configUSE_PREEMPTION			0`

When preemption is disabled the scheduler works in a "First Come, First Served" mode.
This project aims to show the impact that a scheduling algorithm can have in an operative system.
The FCFS scheduling algorith is compared to the "Shortest Job First" and the "Longest Job First" scheduling algorithms.
For providing a correct example we have created the tasks in a way that we give in input a number that determines the duration of the task so that we can order them correctly (as SJF and LJF are theoretical algorithms and cannont be implemented).

A task take as input a number, which is used to choose the number of times the cycle get executed, an higher number implies a longer time for the time to execute

```
void ComputingTask(void *pvParameters) {
    TickType_t start = xTaskGetTickCount();
    int n = *((int*)pvParameters);
	const char *taskName = pcTaskGetName(NULL);
    int res = 1;

    for (int i = 1; i <= n; ++i) {
        res *= i;
    }

    TickType_t end = xTaskGetTickCount() - start;
    printf("%s elapsed time: %u\r\n", taskName, end);
    append(finishedTasks, numberOfTasks, end);
    vTaskDelete(NULL); // delete the task before returning
}
```

When a task starts and ends we take the value of the tick with `xTaskGetTickCount()` so we can calculate the average waiting time and the turn-around time, and we can confront the values obtained in all 3 of the scheduling algorithms.

The number of cycle a task has to execute is selected randomly, the seed used by the function is computed at the time of the selection of the operation through a command line by using the number of tick passed, taken with `xTaskGetTickCount()`.
The task are all scheduled at the same time.

The program utilizes the UART console desribed in the repo [CORTEX_MPS2-genova](https://baltig.polito.it/caos2023/group36/-/tree/CORTEX_MPS2-genova)
When executed it gives this options:

-Select the following:

9 - Generate or regenerate the tasks

1 - FCFS

2 - SJF

3 - LJF

0 - to exit

The option 9 is chosen to generate the array that select the values to pass to the task. The number of tasks is pre-selected using the macro `#define numberOfTasks`. When the values are generated we can then call one of the 3 scheduling algorithms that schedules the tasks in the appropriate order and executes them. Once they are executed the program will print the average duration of the tasks, the average waiting time and the average turn-around time. Than we can choose another scheduling alghoritm that schedules the same tasks with its criteria so that we can have a fair compairison of the results.