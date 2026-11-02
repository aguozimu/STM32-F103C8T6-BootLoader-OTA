#ifndef __UART_H
#define __UART_H

#include "stm32f1xx_hal.h"

#define UART2_RF_PAYLOAD_MAX 32U        /* UART2 单次接收/2.4G 单包最大有效数据长度 */
#define UART2_RING_SIZE 1024U           /* UART2 接收环形缓冲区大小 */

void Uart2_Init(void);
void Uart2_Poll(void);
uint8_t Uart2_HasData(void);
uint16_t Uart2_Read(uint8_t *data, uint16_t max_len);
uint32_t Uart2_GetLastRxTick(void);
void Uart_Send(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len);

#endif /* __UART_H */
