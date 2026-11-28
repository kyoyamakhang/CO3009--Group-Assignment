/**
 * @file i2c_bus.c
 * @brief Shared STM32 HAL I2C bus access.
 * @details Public addresses are 7-bit; the HAL address is shifted left once.
 */

#include "i2c_bus.h"

#define I2C_PROBE_TRIALS         3
#define I2C_PROBE_TIMEOUT_MS     100
#define I2C_FAST_PROBE_TRIALS    1
#define I2C_FAST_PROBE_TIMEOUT_MS 20

extern I2C_HandleTypeDef hi2c1;

/* Convert the 7-bit device address to the STM32 HAL address format. */
uint8_t i2c_bus_probe(uint8_t address)
{
    return (HAL_I2C_IsDeviceReady(&hi2c1, address << 1, I2C_PROBE_TRIALS,
                                  I2C_PROBE_TIMEOUT_MS) == HAL_OK)
        ? I2C_BUS_OK
        : I2C_BUS_ERR;
}

uint8_t i2c_bus_probe_fast(uint8_t address)
{
    return (HAL_I2C_IsDeviceReady(&hi2c1, address << 1, I2C_FAST_PROBE_TRIALS,
                                  I2C_FAST_PROBE_TIMEOUT_MS) == HAL_OK)
        ? I2C_BUS_OK
        : I2C_BUS_ERR;
}

uint8_t i2c_bus_write(uint8_t address, const uint8_t *buffer, uint16_t length, uint32_t timeout_ms)
{
    return (HAL_I2C_Master_Transmit(&hi2c1, address << 1, (uint8_t *)buffer,
                                    length, timeout_ms) == HAL_OK)
        ? I2C_BUS_OK
        : I2C_BUS_ERR;
}

uint8_t i2c_bus_read(uint8_t address, uint8_t *buffer, uint16_t length, uint32_t timeout_ms)
{
    return (HAL_I2C_Master_Receive(&hi2c1, address << 1, buffer, length, timeout_ms) == HAL_OK)
        ? I2C_BUS_OK
        : I2C_BUS_ERR;
}
