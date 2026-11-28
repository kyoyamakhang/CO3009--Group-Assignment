/**
 * @file traffic_light.c
 * @brief Two-signal traffic-light output and cached road colors.
 * @details Signal encoding: red = 11, green = 10, amber = 01, off = 00.
 */

#include "traffic_light.h"
#include "main.h"

static uint8_t traffic_state[3] = {0, 0, 0};

/* Hardware encoding for signals A and B:
 * red = 11, green = 10, amber = 01, off = 00.
 */
static void traffic_write(GPIO_TypeDef *port_a, uint16_t pin_a,
    GPIO_TypeDef *port_b, uint16_t pin_b,
    uint8_t color)
{
    GPIO_PinState signal_a = GPIO_PIN_RESET;
    GPIO_PinState signal_b = GPIO_PIN_RESET;

    switch (color)
    {
    case RED:
        signal_a = GPIO_PIN_SET;
        signal_b = GPIO_PIN_SET;
        break;
    case GREEN:
        signal_a = GPIO_PIN_SET;
        signal_b = GPIO_PIN_RESET;
        break;
    case AMBER:
        signal_a = GPIO_PIN_RESET;
        signal_b = GPIO_PIN_SET;
        break;
    default:
        break; /* OFF */
    }
    HAL_GPIO_WritePin(port_a, pin_a, signal_a);
    HAL_GPIO_WritePin(port_b, pin_b, signal_b);
}

void traffic_set(uint8_t road, uint8_t color)
{
    if (road != ROAD_1 && road != ROAD_2)
    {
        return;
    }
    traffic_state[road] = color;

    if (road == ROAD_1)
    {
        traffic_write(TL1_A_GPIO_Port, TL1_A_Pin, TL1_B_GPIO_Port, TL1_B_Pin, color);
    }
    else
    {
        traffic_write(TL2_A_GPIO_Port, TL2_A_Pin, TL2_B_GPIO_Port, TL2_B_Pin, color);
    }
}

uint8_t traffic_get(uint8_t road)
{
    if (road != ROAD_1 && road != ROAD_2)
    {
        return 0;
    }
    return traffic_state[road];
}
