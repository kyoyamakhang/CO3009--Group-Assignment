/**
 * @file display.c
 * @brief LCD presentation for automatic and manual traffic modes.
 * @details Tuning mode owns its LCD updates through fsm_tuning_display().
 */

#include "display.h"
#include "global.h"
#include "traffic_light.h"
#include "dht20.h"
#include "lcd.h"

static char color_to_char(uint8_t color)
{
    switch (color)
    {
    case LIGHT_RED:
        return 'R';
    case LIGHT_GREEN:
        return 'G';
    case LIGHT_AMBER:
        return 'A';
    default:
        return '-';
    }
}

void display_update(void)
{
    /* Tuning mode updates the LCD directly when its state changes. */
    if (status == TUNE_RED || status == TUNE_AMBER || status == TUNE_GREEN)
    {
        return;
    }

    LcdLine_t status_line, environment_line;
    lcd_line_begin(&status_line);
    lcd_line_begin(&environment_line);

    if (status >= MAN_RED_GREEN && status <= MAN_AMBER_RED)
    {
        /* Show the current colors in manual mode. */
        lcd_line_str(&status_line, "MAN  R1:");
        lcd_line_char(&status_line, color_to_char(traffic_get(ROAD_1)));
        lcd_line_str(&status_line, "  R2:");
        lcd_line_char(&status_line, color_to_char(traffic_get(ROAD_2)));
    }
    else
    {
        /* Show each road color and countdown in automatic mode. */
        lcd_line_str(&status_line, "1:");
        lcd_line_char(&status_line, color_to_char(traffic_get(ROAD_1)));
        lcd_line_int(&status_line, road1_countdown, 2u, '0');
        lcd_line_str(&status_line, "s    2:");
        lcd_line_char(&status_line, color_to_char(traffic_get(ROAD_2)));
        lcd_line_int(&status_line, road2_countdown, 2u, '0');
        lcd_line_char(&status_line, 's');
    }

    lcd_line_str(&environment_line, "T:");
    if (dht20_is_valid() != 0u)
    {
        lcd_line_float(&environment_line, temperature, 1u);
        lcd_line_str(&environment_line, "C H:");
        lcd_line_float(&environment_line, humidity, 1u);
        lcd_line_char(&environment_line, '%');
    }
    else
    {
        lcd_line_str(&environment_line, "--.-C H:--.-%");
    }

    lcd_show_line(0u, &status_line);
    lcd_show_line(1u, &environment_line);
}
