/**
 * @file fsm_tuning.c
 * @brief Traffic timing adjustment, validation, and LCD feedback.
 * @details Draft values are committed only when red equals green plus amber.
 */

#include "fsm_tuning.h"
#include "scheduler.h"
#include "lcd.h"
#include "button.h"
#include "traffic_light.h"

#define TUNE_NOTICE_DURATION_MS 1500u

typedef enum
{
    TUNE_NOTICE_NONE = 0,
    TUNE_NOTICE_SAVED,
    TUNE_NOTICE_INVALID
} TuneNotice_t;

int tune_draft_green = 5;
int tune_draft_amber = 2;
int tune_draft_red = 7;

static uint8_t selected_color = 0u; /* 0 = red, 1 = amber, 2 = green. */
static uint8_t tuning_active = 0u;
static uint8_t redraw_requested = 1u;
static TuneNotice_t current_notice = TUNE_NOTICE_NONE;
static uint32_t notice_task_id = NO_TASK_ID;

static int tune_clamp(int value, int minimum, int maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static void tune_load_draft(void)
{
    tune_draft_green = time_green;
    tune_draft_amber = time_amber;
    tune_draft_red = time_red;
}

static uint8_t tune_is_dirty(void)
{
    return ((tune_draft_green != time_green) ||
        (tune_draft_amber != time_amber) ||
        (tune_draft_red != time_red)) ? 1u : 0u;
}

static uint8_t tune_is_valid(void)
{
    if (tune_draft_green < TUNE_GREEN_MIN || tune_draft_green > TUNE_GREEN_MAX)
    {
        return 0u;
    }
    if (tune_draft_amber < TUNE_AMBER_MIN || tune_draft_amber > TUNE_AMBER_MAX)
    {
        return 0u;
    }
    if (tune_draft_red < TUNE_RED_MIN || tune_draft_red > TUNE_RED_MAX)
    {
        return 0u;
    }
    return (tune_draft_red == (tune_draft_green + tune_draft_amber)) ? 1u : 0u;
}

/* One-shot callback clears the save result after 1.5 s. */
static void tune_clear_notice_oneshot(void)
{
    notice_task_id = NO_TASK_ID;
    current_notice = TUNE_NOTICE_NONE;
    if (tuning_active != 0u)
    {
        redraw_requested = 1u;
    }
}

static void tune_cancel_notice_timer(void)
{
    if (notice_task_id != NO_TASK_ID)
    {
        (void)SCH_Delete_Task(notice_task_id);
        notice_task_id = NO_TASK_ID;
    }
    current_notice = TUNE_NOTICE_NONE;
}

static void tune_show_notice(TuneNotice_t notice)
{
    tune_cancel_notice_timer();
    current_notice = notice;
    notice_task_id = SCH_Add_Task(tune_clear_notice_oneshot, TUNE_NOTICE_DURATION_MS, 0u);
}

static void tune_step_value(int delta)
{
    switch (selected_color)
    {
    case 0u:
        tune_draft_red = tune_clamp(tune_draft_red + delta, TUNE_VAL_MIN, TUNE_VAL_MAX);
        break;
    case 1u:
        tune_draft_amber = tune_clamp(tune_draft_amber + delta, TUNE_VAL_MIN, TUNE_VAL_MAX);
        break;
    default:
        tune_draft_green = tune_clamp(tune_draft_green + delta, TUNE_VAL_MIN, TUNE_VAL_MAX);
        break;
    }
}

static uint8_t tune_apply(void)
{
    if (tune_is_valid() == 0u)
    {
        return 0u;
    }
    time_green = tune_draft_green;
    time_amber = tune_draft_amber;
    time_red = tune_draft_red;
    return 1u;
}

static void tune_select_from_status(void)
{
    if (status == TUNE_RED)
    {
        selected_color = 0u;
    }
    else if (status == TUNE_AMBER)
    {
        selected_color = 1u;
    }
    else
    {
        selected_color = 2u;
    }
}

/* Show the selected tuning color on both light modules. */
static void tune_update_lights(void)
{
    uint8_t color;

    switch (selected_color)
    {
    case 1u:
        color = LIGHT_AMBER;
        break;
    case 2u:
        color = LIGHT_GREEN;
        break;
    default:
        color = LIGHT_RED;
        break;
    }

    traffic_set(ROAD_2, color);
    traffic_set(ROAD_1, color);
}

/* Set both roads red before returning to automatic mode. */
static void tune_set_safe_lights(void)
{
    traffic_set(ROAD_1, LIGHT_RED);
    traffic_set(ROAD_2, LIGHT_RED);
}

void fsm_tuning_display(void)
{
    static const char * const labels[3] = { "TUNE RED    ", "TUNE AMBER  ", "TUNE GREEN  " };
    LcdLine_t line;
    int value;

    if (selected_color == 0u)
    {
        value = tune_draft_red;
    }
    else if (selected_color == 1u)
    {
        value = tune_draft_amber;
    }
    else
    {
        value = tune_draft_green;
    }

    /* Line 0 shows the selected color and draft duration. */
    lcd_line_begin(&line);
    lcd_line_str(&line, labels[selected_color]);
    lcd_line_char(&line, ' ');
    lcd_line_int(&line, value, 2u, '0');
    lcd_line_char(&line, 's');
    lcd_show_line(0u, &line);

    /* Line 1 shows save feedback or all three draft durations. */
    if (current_notice == TUNE_NOTICE_INVALID)
    {
        lcd_fill_line(1u, "ERR: R != G + A ");
    }
    else if (current_notice == TUNE_NOTICE_SAVED)
    {
        lcd_fill_line(1u, "SAVED!          ");
    }
    else
    {
        lcd_line_begin(&line);
        lcd_line_str(&line, "R");
        lcd_line_int(&line, tune_draft_red, 2u, '0');
        lcd_line_str(&line, " A");
        lcd_line_int(&line, tune_draft_amber, 2u, '0');
        lcd_line_str(&line, " G");
        lcd_line_int(&line, tune_draft_green, 2u, '0');
        lcd_line_str(&line, (tune_is_dirty() != 0u) ? " *" : "  ");
        lcd_show_line(1u, &line);
    }

    redraw_requested = 0u;
}

void fsm_tuning_enter(void)
{
    tuning_active = 1u;
    tune_cancel_notice_timer();
    tune_load_draft();
    button_clear_flags();
    tune_select_from_status();
    tune_update_lights();
    fsm_tuning_display();
}

void fsm_tuning_exit(void)
{
    tuning_active = 0u;
    tune_cancel_notice_timer();
    tune_load_draft(); /* Discard changes that have not been saved. */
    tune_set_safe_lights();
    status = INIT;
}

uint8_t fsm_tuning_is_active(void)
{
    return tuning_active;
}

void fsm_tuning_run(void)
{
    uint8_t is_tuning_mode = (uint8_t)((status == TUNE_RED) ||
        (status == TUNE_AMBER) ||
        (status == TUNE_GREEN));

    if ((tuning_active == 0u) && (is_tuning_mode != 0u))
    {
        fsm_tuning_enter();
        return;
    }
    if (tuning_active == 0u)
    {
        return;
    }
    if (is_tuning_mode == 0u)
    {
        tune_cancel_notice_timer();
        tuning_active = 0u;
        return;
    }

    tune_update_lights();

    /* Button 1 cycles red, amber, green, then returns to automatic mode. */
    if (isButtonPressed(TUNE_BTN_MODE) != 0u)
    {
        tune_cancel_notice_timer();
        if (status == TUNE_RED)
        {
            status = TUNE_AMBER;
        }
        else if (status == TUNE_AMBER)
        {
            status = TUNE_GREEN;
        }
        else
        {
            button_clear_flags();
            fsm_tuning_exit();
            return;
        }
        tune_select_from_status();
        tune_update_lights();
        redraw_requested = 1u;
    }

    /* Button 2 increments once or repeats while held. */
    if ((isButtonPressed(TUNE_BTN_INC) != 0u) || (isButtonLongPressed(TUNE_BTN_INC) != 0u))
    {
        tune_cancel_notice_timer();
        tune_step_value(1);
        redraw_requested = 1u;
    }

    /* Button 3 decrements once or repeats while held. */
    if ((isButtonPressed(TUNE_BTN_DEC) != 0u) || (isButtonLongPressed(TUNE_BTN_DEC) != 0u))
    {
        tune_cancel_notice_timer();
        tune_step_value(-1);
        redraw_requested = 1u;
    }

    /* Button 4 validates and saves all draft durations. */
    if (isButtonPressed(TUNE_BTN_SET) != 0u)
    {
        if (tune_apply() != 0u)
        {
            tune_show_notice(TUNE_NOTICE_SAVED);
        }
        else
        {
            tune_show_notice(TUNE_NOTICE_INVALID);
        }
        redraw_requested = 1u;
    }

    if (redraw_requested != 0u)
    {
        fsm_tuning_display();
    }
}
