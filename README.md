# Repository Introduction

In the current branch we provided the guide and the report of the project.

We've organized the repository to have distinct branches for each example or feature implemented. 
In each branch there is a dedicated readme file, providing a detailed explanation of the specific implementations within that branch. 
Refer to each branch's README for a deeper understanding of the contents and functionalities. 
Here is the list of branches and a brief description of each of them.

## Branch Organization

### [CORTEX_MPS2_TASK_SCHEDULING_PREEMPTION](https://baltig.polito.it/caos2023/group36/-/tree/CORTEX_MPS2_TASK_SCHEDULING_PREEMPTION)
This branch contains the implementation of the Earliest Deadline First (EDF) scheduling algorithm. Typically, FreeRTOS use Round Robin with priority algorithm when `configUSE_PREEMPTION` is set to 1.

### [CORTEX_MPS2_TASK_SCHEDULING_NO_PREEMPTION](https://baltig.polito.it/caos2023/group36/-/tree/CORTEX_MPS2_TASK_SCHEDULING_NO_PREEMPTION)
This branch contains the implementation of two different non-preemptive scheduling algorithms: Longest Job First and Shortest Job First. 
Typically, FreeRTOS employs a First-Come-First-Served (FCFS) algorithm when `configUSE_PREEMPTION` is set to 0.

### [modified-best-fit](https://baltig.polito.it/caos2023/group36/-/tree/modified-best-fit)
This branch contains the implementation of **Best-fit** algorithm with coalescing, among with the utilization of a **Memory Watchdog**.
There is also example of random and fixed allocations and de-allocations of blocks to simulate a real case scenario, in order to compute and compare memory fragmentation.
Memory Watchdog permits to not allocate memory if exceeds a specific threshold: this also prevents to not crash the whole system in lack of memory situations.

### [modified-worst-fit](https://baltig.polito.it/caos2023/group36/-/tree/modified-worst-fit)
This branch contains the implementation of **Worst-fit** algorithm with coalescing, among with the utilization of a **Memory Watchdog**.
There is also example of random and fixed allocations and de-allocations of blocks to simulate a real case scenario, in order to compute and compare memory fragmentation.
Memory Watchdog permits to not allocate memory if exceeds a specific threshold: this also prevents to not crash the whole system in lack of memory situations.

### [cortex_MPS2-genova](https://baltig.polito.it/caos2023/group36/-/blob/CORTEX_MPS2-genova)
In this branch, several functionalities have been implemented: 
- **UART Command Line**: a command line for writing and reading data through UART, using interrupts
- **CPU Usage Statistics**: provide runtime insights
- **Tickless idle mode**: designed to minimize power consumption by allowing the board to enter a low-power state during idle periods
- Controlling **LED** playing animations.

### [CORTEX_MPS2-Moscato](https://baltig.polito.it/caos2023/group36/-/tree/CORTEX_MPS2-Moscato)
In this branch the focus is on the utilization of **notifications**, which serve as a lightweight inter-task communication mechanism provided by FreeRTOS. Notifications enable tasks to interact with each other without the need for a separate communication object.

### [cortex_MPS2_Sambataro](https://baltig.polito.it/caos2023/group36/-/tree/cortex_MPS2_Sambataro)
FreeRTOS **Memory Watchdog** example: this branch contains the implementation of two tasks that dynamically allocate memory. 
The branch also includes a function designed to handle memory-related events.

### [CORTEX_MPS2-Cornaggia](https://baltig.polito.it/caos2023/group36/-/blob/CORTEX_MPS2-Cornaggia/-branch)
In this branch a simple example is implemented to showcase the utilization of **Event Groups** functionality. 
The primary objective is to demonstrate how Event Groups can be employed for synchronization between tasks, allowing efficient communication and coordination in the FreeRTOS environment.

### [CORTEX_MPS2_SEMAPHORE_EX](https://baltig.polito.it/caos2023/group36/-/tree/CORTEX_MPS2_SEMAPHORE_EX)
This branch contains the implementation of an example demonstrating the usage of **mutexes** in freeRTOS in order to coordinate 2 tasks: producer and consumer.

### [no-coaleshing-best-fit ](https://baltig.polito.it/caos2023/group36/-/tree/no-coaleshing-best-fit)
A **modified Best-fit** version that does not implement coaleshing, so it's faster in finding the block to select for allocation because the largest is the first one in the free block list(which is sorted by block size). The overhead is only given by reordering the list.

### [heap4_best_fit_allocator](https://baltig.polito.it/caos2023/group36/-/tree/heap4_best_fit_allocator)
This branch contains a first and bugged version of the implementation of **Best-Fit algorithm** for memory management. 
This version is an example of Memory WatchDog called by the task.

### [heap4_worst_fit_allocator](https://baltig.polito.it/caos2023/group36/-/tree/heap4_worst_fit_allocator)
This branch contains a first and bugged version of the implementation of **Worst-Fit algorithm** for memory management. 
This version is an example of Memory WatchDog called by the task.




