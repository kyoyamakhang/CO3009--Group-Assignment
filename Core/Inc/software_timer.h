#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_

#define MAX_TIMERS  10
#define TIMER_TICK  10 // TIM2 interrupt period is 10 ms

/*
 * Timer channel assignment:
 * Timer 0: times the traffic-light phase change (Red / Green / Amber)
 * Timer 1: 1 second tick that decrements road1_countdown and road2_countdown
 * Timer 2: spare
 */
extern int timer_flag[MAX_TIMERS];
extern int timer_counter[MAX_TIMERS];

void timerTick(void);   /* called in the TIM2 interrupt (every 10 ms) */
void setTimer(int index, int duration_ms);
void clearTimer(int index);
void timerRun(void);    /* scheduler task: subtracts the real number of elapsed ticks */

#endif /* INC_SOFTWARE_TIMER_H_ */
