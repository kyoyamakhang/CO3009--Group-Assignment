#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>

#define SCH_MAX_TASKS   20
#define NO_TASK_ID      0

void SCH_Init(void);
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY_MS, uint32_t PERIOD_MS);
uint8_t SCH_Delete_Task(uint32_t taskID);
void SCH_Update(void);         // O(1) - called in the 10 ms timer interrupt
void SCH_Dispatch_Tasks(void); // called in while(1)

#endif /* INC_SCHEDULER_H_ */
