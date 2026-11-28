/**
 * @file software_timer.c
 * @brief Software timers driven by elapsed hardware ticks.
 * @details timerTick() records interrupt ticks; timerRun() consumes them.
 */

#include "software_timer.h"
#include <stdint.h>

int timer_flag[MAX_TIMERS] = {0};
int timer_counter[MAX_TIMERS] = {0};

/* Count hardware ticks in the interrupt, then subtract the elapsed ticks
 * when the scheduled timer task runs after short I2C delays.
 */
static volatile uint32_t interrupt_ticks = 0;
static uint32_t processed_ticks = 0;

void timerTick(void)
{
    interrupt_ticks++;
}

void setTimer(int index, int duration_ms)
{
    if (index < 0 || index >= MAX_TIMERS)
    {
        return;
    }
    if (duration_ms <= 0)
    {
        timer_counter[index] = 0;
        timer_flag[index] = 1;
        return;
    }
    uint32_t pending_ticks = interrupt_ticks - processed_ticks;
    int duration_ticks = (duration_ms + TIMER_TICK - 1) / TIMER_TICK;
    timer_counter[index] = duration_ticks + (int)pending_ticks;
    timer_flag[index] = 0;
}

void clearTimer(int index)
{
    if (index < 0 || index >= MAX_TIMERS)
    {
        return;
    }
    timer_counter[index] = 0;
    timer_flag[index] = 0;
}

void timerRun(void)
{
    uint32_t current_ticks = interrupt_ticks;
    uint32_t elapsed_ticks = current_ticks - processed_ticks;
    processed_ticks = current_ticks;

    if (elapsed_ticks == 0u)
    {
        return;
    }

    for (int timer_index = 0; timer_index < MAX_TIMERS; timer_index++)
    {
        if (timer_counter[timer_index] > 0)
        {
            if ((uint32_t)timer_counter[timer_index] <= elapsed_ticks)
            {
                timer_counter[timer_index] = 0;
                timer_flag[timer_index] = 1;
            }
            else
            {
                timer_counter[timer_index] -= (int)elapsed_ticks;
            }
        }
    }
}
