/**
 * @file fsm_automatic.c
 * @brief Automatic traffic-light state machine.
 * @details Timer 0 controls phase changes; timer 1 updates the countdowns.
 */

#include "fsm_automatic.h"
#include "global.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"
#include "display.h"

#define AUTO_PHASE_TIMER 0
#define AUTO_COUNTDOWN_TIMER 1
#define AUTO_SECOND_MS 1000

/* Phase order: red/green -> red/amber -> green/red -> amber/red.
 * Phase expiry is evaluated before the button event in each state.
 */

static void auto_countdown_tick(void)
{
    if (timer_flag[AUTO_COUNTDOWN_TIMER] == 1)
    {
        setTimer(AUTO_COUNTDOWN_TIMER, AUTO_SECOND_MS);
        if (road1_countdown > 1)
        {
            road1_countdown--;
        }
        if (road2_countdown > 1)
        {
            road2_countdown--;
        }
    }
}

static void auto_enter_manual(int manual_state)
{
    clearTimer(AUTO_PHASE_TIMER);
    clearTimer(AUTO_COUNTDOWN_TIMER);
    button_clear_flags();
    status = manual_state;
    display_update();
}

void fsm_automatic_run(void)
{
    switch (status)
    {
    case INIT:
        button_clear_flags();

        status = AUTO_RED_GREEN;
        road1_countdown = time_red;
        road2_countdown = time_green;
        traffic_set(ROAD_1, LIGHT_RED);
        traffic_set(ROAD_2, LIGHT_GREEN);

        setTimer(AUTO_PHASE_TIMER, time_green * AUTO_SECOND_MS);
        setTimer(AUTO_COUNTDOWN_TIMER, AUTO_SECOND_MS);
        display_update();
        break;

    case AUTO_RED_GREEN:
        traffic_set(ROAD_1, LIGHT_RED);
        traffic_set(ROAD_2, LIGHT_GREEN);
        auto_countdown_tick();

        if (timer_flag[AUTO_PHASE_TIMER] == 1)
        {
            status = AUTO_RED_AMBER;
            road1_countdown = time_amber;
            road2_countdown = time_amber;
            traffic_set(ROAD_1, LIGHT_RED);
            traffic_set(ROAD_2, LIGHT_AMBER);
            setTimer(AUTO_PHASE_TIMER, time_amber * AUTO_SECOND_MS);
            setTimer(AUTO_COUNTDOWN_TIMER, AUTO_SECOND_MS);
        }
        if (isButtonPressed(BUTTON_1))
        {
            auto_enter_manual(MAN_RED_GREEN);
        }
        break;

    case AUTO_RED_AMBER:
        traffic_set(ROAD_1, LIGHT_RED);
        traffic_set(ROAD_2, LIGHT_AMBER);
        auto_countdown_tick();

        if (timer_flag[AUTO_PHASE_TIMER] == 1)
        {
            status = AUTO_GREEN_RED;
            road1_countdown = time_green;
            road2_countdown = time_red;
            traffic_set(ROAD_1, LIGHT_GREEN);
            traffic_set(ROAD_2, LIGHT_RED);
            setTimer(AUTO_PHASE_TIMER, time_green * AUTO_SECOND_MS);
            setTimer(AUTO_COUNTDOWN_TIMER, AUTO_SECOND_MS);
        }
        if (isButtonPressed(BUTTON_1))
        {
            auto_enter_manual(MAN_RED_AMBER);
        }
        break;

    case AUTO_GREEN_RED:
        traffic_set(ROAD_1, LIGHT_GREEN);
        traffic_set(ROAD_2, LIGHT_RED);
        auto_countdown_tick();

        if (timer_flag[AUTO_PHASE_TIMER] == 1)
        {
            status = AUTO_AMBER_RED;
            road1_countdown = time_amber;
            road2_countdown = time_amber;
            traffic_set(ROAD_1, LIGHT_AMBER);
            traffic_set(ROAD_2, LIGHT_RED);
            setTimer(AUTO_PHASE_TIMER, time_amber * AUTO_SECOND_MS);
            setTimer(AUTO_COUNTDOWN_TIMER, AUTO_SECOND_MS);
        }
        if (isButtonPressed(BUTTON_1))
        {
            auto_enter_manual(MAN_GREEN_RED);
        }
        break;

    case AUTO_AMBER_RED:
        traffic_set(ROAD_1, LIGHT_AMBER);
        traffic_set(ROAD_2, LIGHT_RED);
        auto_countdown_tick();

        if (timer_flag[AUTO_PHASE_TIMER] == 1)
        {
            status = AUTO_RED_GREEN;
            road1_countdown = time_red;
            road2_countdown = time_green;
            traffic_set(ROAD_1, LIGHT_RED);
            traffic_set(ROAD_2, LIGHT_GREEN);
            setTimer(AUTO_PHASE_TIMER, time_green * AUTO_SECOND_MS);
            setTimer(AUTO_COUNTDOWN_TIMER, AUTO_SECOND_MS);
        }
        if (isButtonPressed(BUTTON_1))
        {
            auto_enter_manual(MAN_AMBER_RED);
        }
        break;

    default:
        break;
    }
}
