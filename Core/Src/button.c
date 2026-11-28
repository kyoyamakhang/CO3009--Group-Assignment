/**
 * @file button.c
 * @brief Debounced button input and long-press events.
 * @details Call button_read() every 10 ms. Event queries consume their flags.
 */

#include "button.h"
#include "main.h"

/* Active-low buttons: pull-up input, pressed to ground. */
#define BUTTON_PRESSED_LEVEL     0
#define BUTTON_RELEASED_LEVEL    (!BUTTON_PRESSED_LEVEL)

#define LONG_PRESS_INITIAL_TICKS 100u  /* Initial repeat delay: 100 x 10 ms = 1 s. */
#define LONG_PRESS_REPEAT_TICKS  20u   /* Repeat interval: 20 x 10 ms = 200 ms. */

static uint8_t press_flags[NO_OF_BUTTONS] = {0};
static uint8_t repeat_flags[NO_OF_BUTTONS] = {0};
static uint16_t hold_ticks[NO_OF_BUTTONS] = {0};

static GPIO_TypeDef *const button_ports[NO_OF_BUTTONS] = {
    BTN_1_GPIO_Port, BTN_2_GPIO_Port, BTN_3_GPIO_Port, BTN_4_GPIO_Port
};
static const uint16_t button_pins[NO_OF_BUTTONS] = {
    BTN_1_Pin, BTN_2_Pin, BTN_3_Pin, BTN_4_Pin
};

static int sample_oldest[NO_OF_BUTTONS] = {
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL,
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL
};
static int sample_previous[NO_OF_BUTTONS] = {
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL,
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL
};
static int sample_current[NO_OF_BUTTONS] = {
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL,
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL
};
static int debounced_level[NO_OF_BUTTONS] = {
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL,
    BUTTON_RELEASED_LEVEL, BUTTON_RELEASED_LEVEL
};

void button_init(void)
{
    for (int button_index = 0; button_index < NO_OF_BUTTONS; button_index++)
    {
        sample_oldest[button_index] = BUTTON_RELEASED_LEVEL;
        sample_previous[button_index] = BUTTON_RELEASED_LEVEL;
        sample_current[button_index] = BUTTON_RELEASED_LEVEL;
        debounced_level[button_index] = BUTTON_RELEASED_LEVEL;
        press_flags[button_index] = 0u;
        repeat_flags[button_index] = 0u;
        hold_ticks[button_index] = 0u;
    }
}

void button_read(void)
{
    for (int button_index = 0; button_index < NO_OF_BUTTONS; button_index++)
    {
        sample_oldest[button_index] = sample_previous[button_index];
        sample_previous[button_index] = sample_current[button_index];
        sample_current[button_index] =
            (HAL_GPIO_ReadPin(button_ports[button_index], button_pins[button_index])
             == GPIO_PIN_SET) ? 1 : 0;

        if ((sample_oldest[button_index] == sample_previous[button_index]) &&
            (sample_previous[button_index] == sample_current[button_index]))
        {
            if (debounced_level[button_index] != sample_current[button_index])
            {
                debounced_level[button_index] = sample_current[button_index];
                if (debounced_level[button_index] == BUTTON_PRESSED_LEVEL)
                {
                    press_flags[button_index] = 1u;
                    repeat_flags[button_index] = 0u;
                    hold_ticks[button_index] = LONG_PRESS_INITIAL_TICKS;
                }
                else
                {
                    repeat_flags[button_index] = 0u;
                    hold_ticks[button_index] = 0u;
                }
            }
            else if (debounced_level[button_index] == BUTTON_PRESSED_LEVEL)
            {
                if (hold_ticks[button_index] > 0u)
                {
                    hold_ticks[button_index]--;
                    if (hold_ticks[button_index] == 0u)
                    {
                        repeat_flags[button_index] = 1u;
                        hold_ticks[button_index] = LONG_PRESS_REPEAT_TICKS;
                    }
                }
            }
        }
    }
}

void button_reading(void)
{
    button_read();
}

uint8_t isButtonPressed(uint8_t index)
{
    if (index >= NO_OF_BUTTONS)
    {
        return 0u;
    }
    if (press_flags[index] != 0u)
    {
        press_flags[index] = 0u;
        return 1u;
    }
    return 0u;
}

uint8_t isButtonLongPressed(uint8_t index)
{
    if (index >= NO_OF_BUTTONS)
    {
        return 0u;
    }
    if (repeat_flags[index] != 0u)
    {
        repeat_flags[index] = 0u;
        return 1u;
    }
    return 0u;
}

uint8_t isButtonHeld(uint8_t index)
{
    if (index >= NO_OF_BUTTONS)
    {
        return 0u;
    }
    return (debounced_level[index] == BUTTON_PRESSED_LEVEL) ? 1u : 0u;
}

void button_clear_flags(void)
{
    for (int button_index = 0; button_index < NO_OF_BUTTONS; button_index++)
    {
        press_flags[button_index] = 0u;
        repeat_flags[button_index] = 0u;
        hold_ticks[button_index] = 0u;
    }
}
