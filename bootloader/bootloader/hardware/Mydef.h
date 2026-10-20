#ifndef __MYDEF_H
#define __MYDEF_H



#include "stm32f1xx_hal.h"
#include "stdio.h"
#include "main.h"

#include "gpio.h"

#include "Delay.h"
#include "Mydef.h"
#include "TIM_LED.h"
#include "MyFLASH.h"

#define SPI_CSN(x)		HAL_GPIO_WritePin(SPI_CSN_GPIO_Port,SPI_CSN_Pin,x)




#endif
