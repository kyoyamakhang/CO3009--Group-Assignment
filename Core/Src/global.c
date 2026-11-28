/**
 * @file global.c
 * @brief Shared application state and default traffic timings.
 * @details Traffic durations and countdowns are expressed in seconds.
 */

#include "global.h"

int status = INIT;

/* Default cycle: red 7 s = green 5 s + amber 2 s. */
int time_red = 7;
int time_amber = 2;
int time_green = 5;

int road1_countdown = 7;
int road2_countdown = 5;

float temperature = 0.0f;
float humidity = 0.0f;
