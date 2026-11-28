#ifndef INC_TRAFFIC_LIGHT_H_
#define INC_TRAFFIC_LIGHT_H_

#include <stdint.h>
#include "global.h"

#define ROAD_1   1
#define ROAD_2   2

#define OFF     LIGHT_OFF
#define RED     LIGHT_RED
#define GREEN   LIGHT_GREEN
#define AMBER   LIGHT_AMBER

void    traffic_set(uint8_t road, uint8_t color);
uint8_t traffic_get(uint8_t road);

#endif /* INC_TRAFFIC_LIGHT_H_ */
