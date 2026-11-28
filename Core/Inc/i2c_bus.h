#ifndef INC_I2C_BUS_H_
#define INC_I2C_BUS_H_

#include "main.h"

#define I2C_BUS_OK    0
#define I2C_BUS_ERR   1

uint8_t i2c_bus_probe(uint8_t addr);       /* 3 attempts, 100 ms timeout (used at start-up) */
uint8_t i2c_bus_probe_fast(uint8_t addr);  /* 1 attempt, 20 ms timeout (used for retries inside the scheduler) */
uint8_t i2c_bus_write(uint8_t addr, const uint8_t *buf, uint16_t len, uint32_t timeout);
uint8_t i2c_bus_read(uint8_t addr, uint8_t *buf, uint16_t len, uint32_t timeout);

#endif /* INC_I2C_BUS_H_ */
