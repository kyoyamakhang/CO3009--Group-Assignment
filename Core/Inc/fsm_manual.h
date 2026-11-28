#ifndef INC_FSM_MANUAL_H_
#define INC_FSM_MANUAL_H_

/*
 * Mode 2 - Manual (the MAN_* states are in global.h):
 *   BTN2 : go to the next safe light phase (R1-G2 -> R1-A2 -> G1-R2 -> A1-R2)
 *   BTN1 : go to tuning mode (status = TUNE_RED)
 */
void fsm_manual_run(void);   /* scheduler task, every 10 ms */

#endif /* INC_FSM_MANUAL_H_ */
