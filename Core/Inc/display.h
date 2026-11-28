#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

/* Update the LCD for automatic mode, manual mode and the DHT20 data.
 * Called periodically by the scheduler (250 ms) or immediately on a mode change. */
void display_update(void);

#endif /* INC_DISPLAY_H_ */
