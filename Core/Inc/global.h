#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

#include <stdint.h>

/* 1. SYSTEM STATES (the status variable is shared by the 3 FSMs) */
#define INIT                0

/* Automatic mode */
#define AUTO_RED_GREEN      1
#define AUTO_RED_AMBER      2
#define AUTO_GREEN_RED      3
#define AUTO_AMBER_RED      4

/* Manual mode */
#define MAN_RED_GREEN       11
#define MAN_RED_AMBER       12
#define MAN_GREEN_RED       13
#define MAN_AMBER_RED       14

/* Tuning mode */
#define TUNE_RED            21
#define TUNE_AMBER          22
#define TUNE_GREEN          23

/* 2. TRAFFIC LIGHT COLOR CODES (used by traffic_set) */
#define LIGHT_OFF           0
#define LIGHT_RED           1
#define LIGHT_AMBER         2
#define LIGHT_GREEN         3

/* 3. SHARED GLOBAL VARIABLES */
extern int status;
extern int time_red;        /* Red light time (default 7 s = 5 s + 2 s) */
extern int time_amber;      /* Amber light time (default 2 s) */
extern int time_green;      /* Green light time (default 5 s) */

extern int road1_countdown; /* LCD countdown for road 1 */
extern int road2_countdown; /* LCD countdown for road 2 */

extern float temperature;   /* Temperature from the DHT20 (C) */
extern float humidity;      /* Humidity from the DHT20 (%) */

#endif /* INC_GLOBAL_H_ */
