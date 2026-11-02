#ifndef __MYDEF_H
#define __MYDEF_H



#include "stm32f1xx_hal.h"
#include "stdio.h"
#include "string.h"
#include "main.h"


#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "crc.h"
#include "tim.h"

#include "Delay.h"
#include "FREQ2_4G.h"
#include "Mydef.h"
#include "TIM_LED.h"
#include "Uart.h"
#include "MyFLASH.h"
#include "FlashManage.h"




#define SPI_CSN(x)		HAL_GPIO_WritePin(SPI_CSN_GPIO_Port,SPI_CSN_Pin,x)




#endif
