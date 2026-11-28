/**
 * @file fsm_manual.c
 * @brief Button-driven manual traffic-light state machine.
 * @details Button 2 advances the phase; button 1 enters timing adjustment.
 */

#include "fsm_manual.h"
#include "global.h"
#include "button.h"
#include "traffic_light.h"
#include "display.h"

void fsm_manual_run(void)
{
    if (status != MAN_RED_GREEN && status != MAN_RED_AMBER &&
        status != MAN_GREEN_RED && status != MAN_AMBER_RED)
    {
        return;
    }

    switch (status)
    {
    case MAN_RED_GREEN:
        traffic_set(ROAD_1, RED);
        traffic_set(ROAD_2, GREEN);
        if (isButtonPressed(BUTTON_2))
        {
            status = MAN_RED_AMBER;
            traffic_set(ROAD_1, RED);
            traffic_set(ROAD_2, AMBER);
            display_update();
        }
        break;

    case MAN_RED_AMBER:
        traffic_set(ROAD_1, RED);
        traffic_set(ROAD_2, AMBER);
        if (isButtonPressed(BUTTON_2))
        {
            status = MAN_GREEN_RED;
            traffic_set(ROAD_1, GREEN);
            traffic_set(ROAD_2, RED);
            display_update();
        }
        break;

    case MAN_GREEN_RED:
        traffic_set(ROAD_1, GREEN);
        traffic_set(ROAD_2, RED);
        if (isButtonPressed(BUTTON_2))
        {
            status = MAN_AMBER_RED;
            traffic_set(ROAD_1, AMBER);
            traffic_set(ROAD_2, RED);
            display_update();
        }
        break;

    case MAN_AMBER_RED:
        traffic_set(ROAD_1, AMBER);
        traffic_set(ROAD_2, RED);
        if (isButtonPressed(BUTTON_2))
        {
            status = MAN_RED_GREEN;
            traffic_set(ROAD_1, RED);
            traffic_set(ROAD_2, GREEN);
            display_update();
        }
        break;

    default:
        break;
    }

    /* Button 1 enters adjustment in red, amber, then green order. */
    if (isButtonPressed(BUTTON_1))
    {
        button_clear_flags();
        status = TUNE_RED;
    }
}
