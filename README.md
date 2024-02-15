# Scheduling with Earliest Deadline First (EDF)
In this project we have implemented the concept of tasks that have to be executed before a given deadline and we have than implemented the EDF scheduler that executes the task with the earliest deadline.

The concept behind the implementations is taken from the following [thesis](http://beru.univ-brest.fr/cheddar/contribs/examples_of_use/carraro16.pdf), which is improved and tested.

The option to utilize EDF scheduling is active when the macro `configUSE_EDF_SCHEDULER` in `FreeRTOSConfig.h` is set to 1.

Free RTOS utilizes lists for saving the scheduled taks:
`pxReadyTasksLists[i]`is a list that holds the tasks that are in a ready state with the priority `i`.
`xDelayedTaskList1`, `xDelayedTaskList2`, `pxDelayedTaskList`, `pxOverflowDelayedTaskList` holds tasks that are in a blocked state.
`xPendingReadyList` holds tasks in a suspended state.

As pxReadyTasksLists holdes and order tasks based on priority, we have defined a new list called `xReadyTasksListEDF` which holds and order tasks by their deadline in an ascending order.

The function to be called to create a task with a deadline is `xTaskCreateDeadline` which takes the same input as `xTaskCreate` plus a deadline `TickType_t deadline`.

The Task Control Block is a structure containig data related to the task as the starting and ending pointer to its stack, its name, its priority. We have also added the attriubte `TickType_t xTaskDeadline` for a task to hold the starting value of its deadline.

### WIP