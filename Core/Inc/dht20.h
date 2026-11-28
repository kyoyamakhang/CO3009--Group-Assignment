#ifndef INC_DHT20_H_
#define INC_DHT20_H_

#include "global.h"
#include <stdint.h>

/*
 * DHT20 temperature / humidity sensor (AHT20 compatible) on I2C1.
 * Non-blocking read: dht20_start() sends the measurement command; dht20_process()
 * is called by the scheduler every 10 ms and waits for 80 ms before reading the result.
 * The results are stored in temperature / humidity in global.c.
 */

/* ------------------------- Datasheet parameters ------------------------- */
#define DHT20_I2C_ADDRESS       0x38u   /* 7-bit address (i2c_bus shifts it left by 1 bit) */

#define DHT20_CMD_TRIGGER       0xACu   /* start measurement: AC 33 00 */
#define DHT20_ARG_TRIGGER_0     0x33u
#define DHT20_ARG_TRIGGER_1     0x00u
#define DHT20_CMD_INIT          0xBEu   /* load calibration: BE 08 00 */
#define DHT20_ARG_INIT_0        0x08u
#define DHT20_ARG_INIT_1        0x00u

#define DHT20_STATUS_BUSY       0x80u   /* bit7 = 1: measuring */
#define DHT20_STATUS_CALIB      0x08u   /* bit3 = 1: calibrated */

#define DHT20_CONVERSION_MS     80u     /* minimum conversion time */

/* Scheduler period of dht20_process() in ms. Change it if the scheduler tick is not 10 ms. */
#ifndef DHT20_TICK_MS
#define DHT20_TICK_MS           10u
#endif

/* Number of dht20_process() calls to skip before reading.
 * Rounded up, and the first call is skipped, so the real conversion time is always >= 80 ms
 * even if the task runs out of phase with dht20_start(). */
#define DHT20_WAIT_TICKS        (((DHT20_CONVERSION_MS + DHT20_TICK_MS - 1u) / DHT20_TICK_MS) + 1u)

#define DHT20_BUSY_RETRY        3u      /* retries while the sensor is busy */
#define DHT20_MAX_FAIL          3u      /* after this many consecutive failures the data is treated as invalid */
#define DHT20_I2C_TIMEOUT       20u     /* ms */

/* ------------------------------- State ------------------------------ */
typedef enum {
    DHT20_IDLE = 0,         /* no measurement yet */
    DHT20_CONVERTING,       /* command sent, waiting */
    DHT20_OK,               /* last read succeeded */
    DHT20_ERR               /* I2C / CRC error or sensor not ready */
} DHT20_State_t;

extern volatile DHT20_State_t dht20_state;
extern uint8_t  dht20_fail_streak;      /* consecutive failures (reset to 0 on a good read) */
extern uint32_t dht20_ok_count;         /* total number of successful reads */
extern uint8_t  dht20_last_status;      /* raw status byte of the last read (debug) */

/* --------------------------------- API ----------------------------------- */
uint8_t dht20_init(void);               /* call once in main(), BEFORE the scheduler starts */
void    dht20_start(void);              /* 2000 ms task: send a new measurement command */
void    dht20_process(void);            /* 10 ms task: wait 80 ms, read 7 bytes, check the CRC */
uint8_t dht20_data_ready(void);         /* 1 = new data that has not been displayed yet */
void    dht20_clear_data_ready(void);
uint8_t dht20_is_valid(void);           /* 1 = good data available and no continuous sensor failure */
float   dht20_get_temperature(void);    /* °C */
float   dht20_get_humidity(void);       /* %RH */

#endif /* INC_DHT20_H_ */
