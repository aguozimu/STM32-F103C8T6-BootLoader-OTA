#include "TIM.h"

/* TIM2: 72 MHz / 7200 = 10 kHz; 1000 counts = 100 ms. */
void TIM_LED_Init(void)
{
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); /* Active low: start with LED on. */

  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  TIM2->PSC = 7200U - 1U;
  TIM2->ARR = 1000U - 1U;
  TIM2->EGR = TIM_EGR_UG;
  TIM2->SR &= ~TIM_SR_UIF;
  TIM2->DIER |= TIM_DIER_UIE;
  TIM2->CR1 |= TIM_CR1_CEN;

  NVIC_SetPriority(TIM2_IRQn, 1U);
  NVIC_EnableIRQ(TIM2_IRQn);
}

/* Timer interrupt callback. */
static uint8_t tim2_led_count = 0U;

static void TIM_LED_ToggleCallback(void)
{
  if (++tim2_led_count >= 5U)
  {
    tim2_led_count = 0U;
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  }
}

void TIM2_IRQHandler(void)
{
  if ((TIM2->SR & TIM_SR_UIF) != 0U)
  {
    TIM2->SR &= ~TIM_SR_UIF;
    TIM_LED_ToggleCallback();
  }
}
