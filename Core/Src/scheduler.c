/**
 * @file scheduler.c
 * @brief Cooperative scheduler with a static delta-delay list.
 * @details SCH_Update() runs in the timer interrupt; dispatch runs in main().
 */

#include "scheduler.h"
#include "software_timer.h"
#include "main.h"

typedef struct
{
    void (*callback)(void);
    uint32_t delay_ticks;
    uint32_t period_ticks;
    uint8_t run_pending;
    uint32_t task_id;
    int8_t next_index; /* Array index for the static linked list. */
} SchedulerTask_t;

static SchedulerTask_t tasks[SCH_MAX_TASKS];
static volatile int8_t head_index = -1;
static uint32_t next_task_id = 1;

static uint32_t sch_enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void sch_exit_critical(uint32_t primask)
{
    if (primask == 0u)
    {
        __enable_irq();
    }
}

/* Insert by relative delay. The caller must hold the critical section. */
static void sch_insert_node(int8_t task_index)
{
    tasks[task_index].next_index = -1;
    if (head_index == -1)
    {
        head_index = task_index;
        return;
    }

    int8_t current_index = head_index;
    int8_t previous_index = -1;

    while (current_index != -1 && tasks[task_index].delay_ticks >= tasks[current_index].delay_ticks)
    {
        tasks[task_index].delay_ticks -= tasks[current_index].delay_ticks;
        previous_index = current_index;
        current_index = tasks[current_index].next_index;
    }

    tasks[task_index].next_index = current_index;
    if (previous_index == -1)
    {
        head_index = task_index;
    }
    else
    {
        tasks[previous_index].next_index = task_index;
    }

    /* Adjust the successor delta to preserve its absolute deadline. */
    if (current_index != -1)
    {
        tasks[current_index].delay_ticks -= tasks[task_index].delay_ticks;
    }
}

void SCH_Init(void)
{
    uint32_t primask = sch_enter_critical();
    head_index = -1;
    next_task_id = 1;
    for (int task_index = 0; task_index < SCH_MAX_TASKS; task_index++)
    {
        tasks[task_index].callback = 0;
        tasks[task_index].delay_ticks = 0u;
        tasks[task_index].period_ticks = 0u;
        tasks[task_index].run_pending = 0u;
        tasks[task_index].task_id = NO_TASK_ID;
        tasks[task_index].next_index = -1;
    }
    sch_exit_critical(primask);
}

/* Add a task using millisecond delays; a zero period creates a one-shot task. */
uint32_t SCH_Add_Task(void (*callback)(void), uint32_t delay_ms, uint32_t period_ms)
{
    if (callback == 0)
    {
        return NO_TASK_ID;
    }

    uint32_t primask = sch_enter_critical();
    int8_t free_index = -1;
    for (int task_index = 0; task_index < SCH_MAX_TASKS; task_index++)
    {
        if (tasks[task_index].callback == 0)
        {
            free_index = (int8_t)task_index;
            break;
        }
    }
    if (free_index == -1)
    {
        sch_exit_critical(primask);
        return NO_TASK_ID;
    }

    uint32_t delay_ticks = (delay_ms == 0u) ? 0u : ((delay_ms + TIMER_TICK - 1u) / TIMER_TICK);
    uint32_t period_ticks = (period_ms == 0u) ? 0u : ((period_ms + TIMER_TICK - 1u) / TIMER_TICK);

    uint32_t task_id = next_task_id++;
    if (next_task_id == NO_TASK_ID)
    {
        next_task_id++;
    }

    tasks[free_index].callback = callback;
    tasks[free_index].delay_ticks = delay_ticks;
    tasks[free_index].period_ticks = period_ticks;
    tasks[free_index].run_pending = (delay_ticks == 0u) ? 1u : 0u;
    tasks[free_index].task_id = task_id;
    tasks[free_index].next_index = -1;

    sch_insert_node(free_index);
    sch_exit_critical(primask);
    return task_id;
}

/* Delete by task ID, including a task that deletes itself while running. */
uint8_t SCH_Delete_Task(uint32_t task_id)
{
    if (task_id == NO_TASK_ID)
    {
        return 0u;
    }

    uint32_t primask = sch_enter_critical();
    int8_t target_index = -1;
    for (int task_index = 0; task_index < SCH_MAX_TASKS; task_index++)
    {
        if (tasks[task_index].callback != 0 && tasks[task_index].task_id == task_id)
        {
            target_index = (int8_t)task_index;
            break;
        }
    }
    if (target_index == -1)
    {
        sch_exit_critical(primask);
        return 0u;
    }

    /* A running task is detached from the waiting list during its callback. */
    int8_t current_index = head_index;
    int8_t previous_index = -1;
    while (current_index != -1 && current_index != target_index)
    {
        previous_index = current_index;
        current_index = tasks[current_index].next_index;
    }

    if (current_index == target_index)
    {
        int8_t next_node = tasks[target_index].next_index;
        if (next_node != -1)
        {
            tasks[next_node].delay_ticks += tasks[target_index].delay_ticks;
        }
        if (previous_index == -1)
        {
            head_index = next_node;
        }
        else
        {
            tasks[previous_index].next_index = next_node;
        }
    }

    tasks[target_index].callback = 0;
    tasks[target_index].delay_ticks = 0u;
    tasks[target_index].period_ticks = 0u;
    tasks[target_index].run_pending = 0u;
    tasks[target_index].task_id = NO_TASK_ID;
    tasks[target_index].next_index = -1;

    sch_exit_critical(primask);
    return 1u;
}

/* Constant-time interrupt update: only the list head is inspected. */
void SCH_Update(void)
{
    int8_t current_head = head_index;
    if (current_head != -1)
    {
        if (tasks[current_head].delay_ticks > 0u)
        {
            tasks[current_head].delay_ticks--;
        }
        if (tasks[current_head].delay_ticks == 0u)
        {
            tasks[current_head].run_pending = 1u;
        }
    }
}

/* Run ready callbacks from the main loop, outside the critical section. */
void SCH_Dispatch_Tasks(void)
{
    while (1)
    {
        uint32_t primask = sch_enter_critical();
        int8_t ready_index = head_index;
        if (ready_index == -1 ||
            tasks[ready_index].delay_ticks != 0u ||
            tasks[ready_index].run_pending == 0u)
        {
            sch_exit_critical(primask);
            break;
        }

        head_index = tasks[ready_index].next_index;
        tasks[ready_index].next_index = -1;
        tasks[ready_index].run_pending = 0u;

        /* Mark the next task ready when it shares the current deadline. */
        if (head_index != -1 && tasks[head_index].delay_ticks == 0u)
        {
            tasks[head_index].run_pending = 1u;
        }

        void (*callback)(void) = tasks[ready_index].callback;
        uint32_t dispatched_task_id = tasks[ready_index].task_id;
        sch_exit_critical(primask);

        if (callback != 0)
        {
            (*callback)();
        }

        primask = sch_enter_critical();
        /* Reinsert only if the callback has not deleted or replaced this task. */
        if (tasks[ready_index].callback != 0 && tasks[ready_index].task_id == dispatched_task_id)
        {
            if (tasks[ready_index].period_ticks > 0u)
            {
                tasks[ready_index].delay_ticks = tasks[ready_index].period_ticks;
                sch_insert_node(ready_index);
            }
            else
            {
                /* Release the completed one-shot task slot. */
                tasks[ready_index].callback = 0;
                tasks[ready_index].delay_ticks = 0u;
                tasks[ready_index].period_ticks = 0u;
                tasks[ready_index].run_pending = 0u;
                tasks[ready_index].task_id = NO_TASK_ID;
                tasks[ready_index].next_index = -1;
            }
        }
        sch_exit_critical(primask);
    }
}
