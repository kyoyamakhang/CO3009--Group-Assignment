#ifndef INC_FSM_TUNING_H_
#define INC_FSM_TUNING_H_

#include "global.h"
#include <stdint.h>

/*
 * Mode switching with BTN1:
 *   Automatic -> Manual -> Tune RED -> Tune AMBER -> Tune GREEN -> Automatic
 *
 * Traffic safety in tuning mode:
 *   Both roads are always kept in the RED state (all-red) during the whole adjustment,
 *   so the two roads can NEVER be green at the same time.
 *
 * Buttons in tuning mode:
 *   BTN1 : next item (RED -> AMBER -> GREEN -> back to Automatic).
 *          The entered values are kept across the 3 RED/AMBER/GREEN screens;
 *          leaving without pressing BTN4 discards the unsaved changes.
 *   BTN2 : increase the value (+1, holding > 1 s repeats every 200 ms)
 *   BTN3 : decrease the value (-1, holding > 1 s repeats every 200 ms)
 *   BTN4 : check the constraint RED == GREEN + AMBER (every value >= 1, RED <= 99):
 *          - Valid   : save to time_red / time_amber / time_green, show "SAVED!"
 *          - Invalid : keep the old configuration, show "ERR: R != G + A"
 */

#define TUNE_VAL_MIN        1
#define TUNE_VAL_MAX        99
#define TUNE_GREEN_MIN      1
#define TUNE_GREEN_MAX      98
#define TUNE_AMBER_MIN      1
#define TUNE_AMBER_MAX      98
#define TUNE_RED_MIN        2
#define TUNE_RED_MAX        99

#define TUNE_BTN_MODE       BUTTON_1
#define TUNE_BTN_INC        BUTTON_2
#define TUNE_BTN_DEC        BUTTON_3
#define TUNE_BTN_SET        BUTTON_4

void    fsm_tuning_run(void);        /* scheduler task, every 10 ms */
void    fsm_tuning_enter(void);      /* start a tuning session */
void    fsm_tuning_exit(void);       /* back to Automatic: both roads red, status = INIT */
uint8_t fsm_tuning_is_active(void);
void    fsm_tuning_display(void);    /* redraw the LCD */

extern int tune_draft_green;
extern int tune_draft_amber;
extern int tune_draft_red;

#endif /* INC_FSM_TUNING_H_ */
