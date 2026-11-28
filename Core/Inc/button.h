#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include <stdint.h>

#define NO_OF_BUTTONS      4

#define BUTTON_1           0   /* BTN_1 - D7  - PA8 : mode switch */
#define BUTTON_2           1   /* BTN_2 - D8  - PA9 : increase / next */
#define BUTTON_3           2   /* BTN_3 - D9  - PC7 : decrease */
#define BUTTON_4           3   /* BTN_4 - D10 - PB6 : save (set) */

void    button_init(void);
void    button_read(void);                    /* 10 ms task: read, debounce, detect press and hold */
void    button_reading(void);                 /* compatibility alias */

uint8_t isButtonPressed(uint8_t index);       /* 1 once per press (clears the flag) */
uint8_t isButtonLongPressed(uint8_t index);   /* 1 after a 1 s hold, repeats every 200 ms (clears the flag) */
uint8_t isButtonHeld(uint8_t index);          /* 1 while the button is held down (stable) */
void    button_clear_flags(void);             /* clear the button flags when the mode changes */

#endif /* INC_BUTTON_H_ */
