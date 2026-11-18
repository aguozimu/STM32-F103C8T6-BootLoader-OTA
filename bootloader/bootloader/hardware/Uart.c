#include "Mydef.h"

#define UART_TX_TIMEOUT_MS 100U

static uint8_t Uart2_ITData[UART2_RF_PAYLOAD_MAX];
static uint8_t Uart2_RingBuffer[UART2_RING_SIZE];
static volatile uint16_t Uart2_RingHead = 0U;
static volatile uint16_t Uart2_RingTail = 0U;
static volatile uint32_t Uart2_LastTick = 0U;

static void Uart2_StartReceive(void)
{
  (void)HAL_UARTEx_ReceiveToIdle_IT(&huart2, Uart2_ITData, UART2_RF_PAYLOAD_MAX);
}

void Uart2_Init(void)
{
  Uart2_RingHead = 0U;
  Uart2_RingTail = 0U;
  Uart2_LastTick = 0U;
  Uart2_StartReceive();
}

void Uart2_Poll(void)
{
  if (huart2.RxState == HAL_UART_STATE_READY)
  {
    Uart2_StartReceive();
  }
}

uint8_t Uart2_HasData(void)
{
  return (Uart2_RingHead != Uart2_RingTail) ? 1U : 0U;
}

uint16_t Uart2_Read(uint8_t *data, uint16_t max_len)
{
  uint16_t length = 0U;

  if (data == NULL)
  {
    return 0U;
  }

  while ((Uart2_RingTail != Uart2_RingHead) && (length < max_len))
  {
    data[length] = Uart2_RingBuffer[Uart2_RingTail];
    Uart2_RingTail = (uint16_t)((Uart2_RingTail + 1U) % UART2_RING_SIZE);
    length++;
  }

  return length;
}

uint32_t Uart2_GetLastRxTick(void)
{
  return Uart2_LastTick;
}

void Uart_Send(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len)
{
  if ((huart != NULL) && (data != NULL) && (len > 0U))
  {
    (void)HAL_UART_Transmit(huart, data, len, UART_TX_TIMEOUT_MS);
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART2)
  {
    if (Size > 0U)
    {
      uint16_t i;
      uint16_t next_head;

      if (Size > UART2_RF_PAYLOAD_MAX)
      {
        Size = UART2_RF_PAYLOAD_MAX;
      }

      Uart2_LastTick = TIM_LED_GetTimeMs();

      /* 将本次接收到的字节写入环形缓冲区，满时丢弃当前帧剩余数据 */
      for (i = 0U; i < Size; i++)
      {
        next_head = (uint16_t)((Uart2_RingHead + 1U) % UART2_RING_SIZE);
        if (next_head == Uart2_RingTail)
        {
          break;
        }

        Uart2_RingBuffer[Uart2_RingHead] = Uart2_ITData[i];
        Uart2_RingHead = next_head;
      }
    }

    Uart2_StartReceive();
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    /* 清除串口错误并重新启动接收，避免 UART2 停止工作 */
    __HAL_UART_CLEAR_OREFLAG(huart);
    Uart2_StartReceive();
  }
}
