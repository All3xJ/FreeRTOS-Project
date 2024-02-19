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

The Task Control Block is a structure containing data related to the task as the starting and ending pointer to its stack, its name, its priority. We have also added the attriubte `TickType_t xTaskDeadline` for a task to hold the starting value of its deadline.

When a task get scheduled a portion of the memory get allocated for the stack, we have another allocation for the TCB where ve store the stack pointer `pxNewTCB->pxStack = pxStack;` and the deadline `pxNewTCB->xTaskDeadline = deadline;`. Then the following commands are executed:

```
prvInitialiseNewTask( pxTaskCode, pcName, ( uint32_t ) usStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL );
listSET_LIST_ITEM_VALUE( &( ( pxNewTCB )->xStateListItem ), ( pxNewTCB )->xTaskDeadline + xTickCount); 
insertStarting(pcName, (int)xTaskGetTickCount());
prvAddNewTaskToReadyList( pxNewTCB );
```

`prvInitialiseNewTask` initialize the values in TCB, for example it calculates the starting point of the stack for the given task, it initialize the lists xStateListItem and xEventListItem. No modification are needed in this function.

`listSET_LIST_ITEM_VALUE( &( ( pxNewTCB )->xStateListItem ), ( pxNewTCB )->xTaskDeadline + xTickCount)` is added, in comparison to `xTaskCreate`. Set the attribute `xItemValue` of the list to the deadline of our task.

`insertStarting(pcName, (int)xTaskGetTickCount())` is a function call that we have implemented to a custom implementation of a dictionary, the tuple: Name of the task - Current tick count. This is used to save at which tick a task get scheduled.

`prvAddNewTaskToReadyList( pxNewTCB )` is responsable to adding a new task to the ready list, if there are no task `(xCurrentTCB == NULL)` it makes `pxNewTCB` as the new task. If the scheduler is not running `(xSchedulerRunning == pdFALSE)` it makes the task the current taks if the deadline happens earlier than the actual current task. After this check the call to `prvAddTaskToReadyList( pxNewTCB )` happens, where the task gets added to the correct ReadyList (xReadyTasksListEDF or pxReadyTasksLists). If `configUSE_EDF_SCHEDULER == 1` we have changed the function call to `vListInsert( &(xReadyTasksListEDF), & ((pxTCB)->xStateListItem))` to insert our task in the `xReadyTasksListEDF` list based on its deadline. While if the scheduler is running and a task with earlier deadlines then the current task arrives, context switch happens. This is a critical point that was missing in the thesis mentioned above, because if the context switch doesn't happen, this task will always miss its deadline. We have added the following check that requests a context switch when the condition is met:
```
if( pxCurrentTCB->xStateListItem.xItemValue > pxNewTCB->xStateListItem.xItemValue )
{
    taskYIELD_IF_USING_PREEMPTION();
}
```

In the vTaskSwitchContext function, we have to change the task to select, based on deadline instead of priorities, this happens modifying the code in this manner:

```
 #if (configUSE_EDF_SCHEDULER == 0)
{
    taskSELECT_HIGHEST_PRIORITY_TASK();
}
#else
{
            pxCurrentTCB = (TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( &( xReadyTasksListEDF ) );
}
#endif
```

The same is true for `xTaskIncrementTick`, when a task that awakens from suspended state, it shuold be switch based on deadline and not priority anymore:

```
#if (configUSE_EDF_SCHEDULER == 1)
{
    if( pxTCB->xStateListItem.xItemValue < pxCurrentTCB->xStateListItem.xItemValue )
    {
        xSwitchRequired = pdTRUE;
    }
    #else
    {
        if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
        {
            xSwitchRequired = pdTRUE;
        }

    }
}
#endif
```

## Examples

### 1. Missed deadlines

In this example we are going to generate some random task with some random deadlines and we are going to confront the miss rate when the EDF scheduler is active and when the base scheduler is active.

The tasks that we are going to generate are gonna execute this function that keep on multplying numbers just to keep the task running. The tasks are generated randomly by randomy selecting a value of n (the number of times the for cycle is executed) in an interval from 1000000 to 10000000.
At the same way we assign random deadlines from 200 ticks to 500.
The tasks are scheduled all at the same time.

```
void ComputingTask(void *pvParameters) {
    TickType_t start = xTaskGetTickCount();
    int n = *((int*)pvParameters);
	const char *taskName = pcTaskGetName(NULL);
	printf("%s start time: %u\r\n", taskName, (int)start);
    int res = 1;

    for (int i = 1; i <= n; ++i) {
        res *= i;
    }

    TickType_t end = xTaskGetTickCount();
	printf("%s end time: %d\r\n", taskName, (int)end);
    printf("%s elapsed time: %d\r\n", taskName, (int)(end - start));
	insertFinished(taskName, (int)end);
    vTaskDelete(NULL); // delete the task before returning
}
```

We have to compile and execute the project twitch with the macro `#define configUSE_EDF_SCHEDULER` set to 1 and set to 0.
To make sure the same set of tasks and deadlines are generated for the two execution, we ask the user to insert as input a random seed.

With this settings we obtains the following results (Respected deadlines - Missed deadlines):

| Seed | EDF | RR |
|:----:|:----:|:----:|
| 100  | 10 - 0  | 8 - 2  |
| 200  | 10 - 0  | 7 - 3  |
| 300  | 10 - 0  | 8 - 2  |
| 400  | 10 - 0  | 7 - 3  |
| 500  | 10 - 0  | 7 - 3  |

With this settings we can see that EDF scheduling manages to respect all the deadlines, while the RR algorithm utilized by FreeRTOS misses 2-3 deadlines out of 10 tasks.
This data is extermly teoric as we have a lot of parameters that could be changed to generate different results, the real performance will change based on the context of real program, but we can still observe an approximation of the improvements.

### 2. Context switch when a task with earlierd deadline arrives

This example can be selected with the number 6 of the command line.

In this example we schedule a task that has a deadline of 200 ticks, this task will schedules two other task, one with a deadline of 150 and another one with a deadline of 10 and we observe how context switch happens.

Before scheduling this two task, our starting task will spend sime time doing random computation, after that it will print it's current deadline and then it will schedule the two taks: 
```
After some computation, current task deadline: 118
Scheduling task with 150 ticks as deadline
Scheduling task with 10 ticks as deadline
```

As we can expected context switch happens on the task with 10 edf, then on the starting task and last the tastk with a deadline of 150.

This particular set of task has been choosen to showcase the fact that the deadlines that get confronted are not the starting ones (200 vs 150) but the current ones (118 vs 150).

### 3. Periodic task

This example can be selected with the number 7 of the command line and then verified with the number 2.

Two task are created with the following parameters (we have estimated C in ticks after multiple running of the code):

| Task | C | T |
|:----:|:----:|:----:|
| Task1 | 95 | 280 |
| Task2 | 180 | 360 |

We can see that this set of task is feasable:

95/280 = 0.34
180/360 =  0.50

The sum 0.84 is less or equal than 1

Running the code we can observe that the scheduler has to context switch between the task to not miss their deadlines.