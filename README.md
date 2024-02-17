# Repository Introduction

We've organized the repository to have distinct branches for each example or feature implemented. 
Each branch is accompanied by a dedicated readme file, providing a detailed explanation of the specific implementations within that branch. 
Refer to each branch's README for a deeper understanding of the contents and functionalities. 

## Branch Organization

### [cortex_MPS2_Sambataro](https://baltig.polito.it/caos2023/group36/-/tree/cortex_MPS2_Sambataro)
This branch serves as a FreeRTOS Memory WatchDog Example, featuring two tasks dynamically allocating memory. The branch also includes a function designed to handle memory-related events.

### [cortex_MPS2-genova](https://baltig.polito.it/caos2023/group36/-/blob/CORTEX_MPS2-genova)
In this branch, several functionalities have been implemented: a command line for writing and reading data through UART, "CPU Usage Statistics" that provide runtime insights, the "Tickless idle mode" designed to minimize power consumption by allowing the board to enter a low-power state during idle periods, and lastly, an additional feature for LED Control.

### [CORTEX_MPS2-Moscato](https://baltig.polito.it/caos2023/group36/-/blob/CORTEX_MPS2-Cornaggia/-branch)
In this branch the focus is on the utilization of notifications, which serve as a lightweight inter-task communication mechanism provided by FreeRTOS. Notifications enable tasks to interact with each other without the need for a separate communication object.

### [CORTEX_MPS2-Cornaggia](https://baltig.polito.it/caos2023/group36/-/blob/CORTEX_MPS2-Cornaggia/-branch)
In this branch a simple example is implemented to showcase the utilization of Event Groups functionality. The primary objective is to demonstrate how Event Groups can be employed for synchronization between tasks, allowing efficient communication and coordination in the FreeRTOS environment.


### [CORTEX_MPS2_SEMAPHORE_EX]
This branch contains the implementation of an example demonstrating the usage of mutexes in freeRTOS in order to coordinate 2 tasks: producer and consumer

### [CORTEX_MPS2_TASK_SCHEDULING_NO_PREEMPTION]
This branch contains the implementation of two different non-preemptive scheduling algorithms: Longest Job First and Shortest Job First. 
Typically, FreeRTOS employs a First-Come-First-Served (FCFS) algorithm when `configUSE_PREEMPTION` is set to 0.

### [CORTEX_MPS2_TASK_SCHEDULING_PREEMPTION]
This branch contains the implementation of the Earliest Deadline First (EDF) scheduling algorithm.

### [best-fit_heap4]
This branch contains the implementation of two algorithms for memory managment: Worst-Fit and Best-Fit.
(Our base demo uses the heap_4.c, that implements a First fit algoritm).






