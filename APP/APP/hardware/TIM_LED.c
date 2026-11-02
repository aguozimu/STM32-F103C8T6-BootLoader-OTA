#include "Mydef.h"


#define TIM_LED_TOGGLE_CYCLES 5U       /* 100 ms × 5 = 500 ms 翻转一次 LED */
#define TIM_LED_UPDATE_PERIOD_MS 100U  /* TIM2 更新中断周期 */

static uint8_t tim2_led_count = 0U;
static volatile uint8_t tim2_led_solid_on = 0U;
static volatile uint32_t tim2_time_ms = 0U;

/* TIM2 基础初始化由 CubeMX 生成在 Core/Src/tim.c 中 */
void TIM_LED_Start(void)
{
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); /* LED 低电平点亮 */
  HAL_TIM_Base_Start_IT(&htim2);
}

/* 由现有 TIM2 中断维护的非阻塞 100 ms 时基 */
uint32_t TIM_LED_GetTimeMs(void)
{
  return tim2_time_ms;
}

/* solid_on = 1：LED 常亮；solid_on = 0：恢复闪烁 */
void TIM_LED_SetSolidOn(uint8_t solid_on)
{
  tim2_led_solid_on = (solid_on != 0U) ? 1U : 0U;
  tim2_led_count = 0U;
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}

/* TIM2 更新中断回调：更新时基并处理 LED 闪烁 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    tim2_time_ms += TIM_LED_UPDATE_PERIOD_MS;

    if (tim2_led_solid_on != 0U)
    {
      return;
    }

    if (++tim2_led_count >= TIM_LED_TOGGLE_CYCLES)
    {
      tim2_led_count = 0U;
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
  }
}
