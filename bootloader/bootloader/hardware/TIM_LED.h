#ifndef __TIM_LED_H
#define __TIM_LED_H

#include "stm32f1xx_hal.h"

void TIM_LED_Start(void);
uint32_t TIM_LED_GetTimeMs(void);
void TIM_LED_SetSolidOn(uint8_t solid_on);




#endif /* __TIM_LED_H */
