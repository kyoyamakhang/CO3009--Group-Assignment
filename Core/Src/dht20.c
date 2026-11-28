/**
 * @file dht20.c
 * @brief Scheduled DHT20 acquisition with CRC validation.
 * @details Initialization blocks; conversion processing is scheduled every 10 ms.
 */

#include "dht20.h"
#include "i2c_bus.h"

/* Measurement frame: status, 20-bit humidity, 20-bit temperature, CRC8.
 * CRC8 uses polynomial 0x31 and initial value 0xFF over the first six bytes.
 * Relative humidity = raw / 2^20 x 100; temperature = raw / 2^20 x 200 - 50.
 */

static uint8_t receive_buffer[7];
static uint8_t conversion_wait_ticks = 0u;
static uint8_t busy_retry_count = 0u;
static uint8_t data_ready_flag = 0u;
static uint8_t data_valid_flag = 0u;

volatile DHT20_State_t dht20_state = DHT20_IDLE;
uint8_t dht20_fail_streak = 0u;
uint32_t dht20_ok_count = 0u;
uint8_t dht20_last_status = 0u;

static uint8_t dht20_crc8(const uint8_t *buffer, uint8_t length)
{
    uint8_t crc = 0xFFu;
    uint8_t byte_index, bit_index;

    for (byte_index = 0u; byte_index < length; byte_index++)
    {
        crc ^= buffer[byte_index];
        for (bit_index = 0u; bit_index < 8u; bit_index++)
        {
            if ((crc & 0x80u) != 0u)
            {
                crc = (uint8_t)((crc << 1) ^ 0x31u);
            }
            else
            {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

static void dht20_fail(void)
{
    if (dht20_fail_streak < 255u)
    {
        dht20_fail_streak++;
    }
    if (dht20_fail_streak >= DHT20_MAX_FAIL)
    {
        data_valid_flag = 0u; /* Invalidate stale data after repeated failures. */
    }
    dht20_state = DHT20_ERR;
}

static void dht20_send_init(void)
{
    uint8_t command[3];

    command[0] = DHT20_CMD_INIT;
    command[1] = DHT20_ARG_INIT_0;
    command[2] = DHT20_ARG_INIT_1;
    (void)i2c_bus_write(DHT20_I2C_ADDRESS, command, 3u, DHT20_I2C_TIMEOUT);
}

/* Initialize before starting the scheduler; HAL_Delay() is used here. */
uint8_t dht20_init(void)
{
    uint8_t sensor_status = 0u;

    HAL_Delay(100); /* Allow the sensor to settle after power-up. */

    conversion_wait_ticks = 0u;
    busy_retry_count = 0u;
    data_ready_flag = 0u;
    data_valid_flag = 0u;
    dht20_state = DHT20_IDLE;

    if (i2c_bus_probe(DHT20_I2C_ADDRESS) != I2C_BUS_OK)
    {
        dht20_state = DHT20_ERR;
        return 0u;
    }
    if (i2c_bus_read(DHT20_I2C_ADDRESS, &sensor_status, 1u, DHT20_I2C_TIMEOUT) != I2C_BUS_OK)
    {
        dht20_state = DHT20_ERR;
        return 0u;
    }
    dht20_last_status = sensor_status;

    if ((sensor_status & DHT20_STATUS_CALIB) == 0u)
    {
        /* Request initialization when the calibration flag is clear. */
        dht20_send_init();
        HAL_Delay(10);
    }
    return 1u;
}

void dht20_start(void)
{
    uint8_t command[3];

    if (dht20_state == DHT20_CONVERTING)
    {
        return; /* Leave the active conversion undisturbed. */
    }

    command[0] = DHT20_CMD_TRIGGER;
    command[1] = DHT20_ARG_TRIGGER_0;
    command[2] = DHT20_ARG_TRIGGER_1;

    if (i2c_bus_write(DHT20_I2C_ADDRESS, command, 3u, DHT20_I2C_TIMEOUT) != I2C_BUS_OK)
    {
        dht20_fail();
        return;
    }

    conversion_wait_ticks = (uint8_t)DHT20_WAIT_TICKS;
    busy_retry_count = 0u;
    dht20_state = DHT20_CONVERTING;
}

void dht20_process(void)
{
    uint32_t raw_humidity;
    uint32_t raw_temperature;

    if (dht20_state != DHT20_CONVERTING)
    {
        return;
    }
    if (conversion_wait_ticks > 0u)
    {
        conversion_wait_ticks--; /* Wait for the configured conversion delay. */
        return;
    }

    if (i2c_bus_read(DHT20_I2C_ADDRESS, receive_buffer, 7u, DHT20_I2C_TIMEOUT) != I2C_BUS_OK)
    {
        dht20_fail();
        return;
    }
    dht20_last_status = receive_buffer[0];

    if ((receive_buffer[0] & DHT20_STATUS_BUSY) != 0u)
    {
        /* Retry on a later scheduler tick while the sensor is busy. */
        if (busy_retry_count < DHT20_BUSY_RETRY)
        {
            busy_retry_count++;
            return;
        }
        dht20_fail();
        return;
    }

    if ((receive_buffer[0] & DHT20_STATUS_CALIB) == 0u)
    {
        /* Reinitialize and discard this uncalibrated measurement. */
        dht20_send_init();
        dht20_fail();
        return;
    }

    if (dht20_crc8(receive_buffer, 6u) != receive_buffer[6])
    {
        /* Reject the measurement when the CRC does not match. */
        dht20_fail();
        return;
    }

    raw_humidity = ((uint32_t)receive_buffer[1] << 12)
        | ((uint32_t)receive_buffer[2] << 4)
        | ((uint32_t)receive_buffer[3] >> 4);

    raw_temperature = (((uint32_t)receive_buffer[3] & 0x0Fu) << 16)
        | ((uint32_t)receive_buffer[4] << 8)
        | (uint32_t)receive_buffer[5];

    humidity = ((float)raw_humidity * 100.0f) / 1048576.0f; /* 20-bit full-scale value. */
    temperature = ((float)raw_temperature * 200.0f) / 1048576.0f - 50.0f;

    dht20_ok_count++;
    dht20_fail_streak = 0u;
    data_valid_flag = 1u;
    data_ready_flag = 1u;
    dht20_state = DHT20_OK;
}

uint8_t dht20_data_ready(void)
{
    return data_ready_flag;
}

void dht20_clear_data_ready(void)
{
    data_ready_flag = 0u;
}

uint8_t dht20_is_valid(void)
{
    return data_valid_flag;
}

float dht20_get_temperature(void)
{
    return temperature;
}

float dht20_get_humidity(void)
{
    return humidity;
}
