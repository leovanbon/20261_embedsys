#include "main.h"
#include "lab_app.h"
#include "led_effects.h"
#include "7seg.h"
#include <assert.h>
#include <stdio.h>

GPIO_TypeDef test_a, test_d, test_e, test_g;
static uint32_t tick;
void HAL_GPIO_TogglePin(GPIO_TypeDef *p, uint16_t pins) { p->ODR ^= pins; }
void HAL_GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *g) { (void)p; (void)g; }
uint32_t HAL_GetTick(void) { return tick; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *p, uint16_t pins)
{ return (p->ODR & pins) ? GPIO_PIN_SET : GPIO_PIN_RESET; }
void HAL_GPIO_WritePin(GPIO_TypeDef *p, uint16_t pins, GPIO_PinState state)
{ if (state) p->ODR |= pins; else p->ODR &= ~pins; }
static void process(uint32_t time, unsigned pressed)
{
  tick = time;
  test_a.ODR = pressed;
  LabApp_Process();
}

int main(void)
{
  LabApp_Init();
  assert(Get7SegDisplayValue() == 25);
  /* Bounce before a real press, hold for a long time, then release. */
  process(1, 1); process(5, 0); process(9, 1);
  process(38, 1); assert(Get7SegDisplayValue() == 25);
  process(39, 1); assert(Get7SegDisplayValue() == 26);
  process(1000, 1); assert(Get7SegDisplayValue() == 26);
  process(1001, 0); process(1031, 0);
  Set7SegDisplayValue(99);
  process(1040, 1); process(1070, 1);
  assert(Get7SegDisplayValue() == 0);
  /* A non-TIM6 callback must not advance the multiplexer. */
  Set7SegDisplayValue(25);
  TIM_HandleTypeDef unrelated = {(void *)7}, timer = {TIM6};
  HAL_TIM_PeriodElapsedCallback(&unrelated);
  assert(test_g.ODR == 0);
  for (int i = 0; i < 49; i++) HAL_TIM_PeriodElapsedCallback(&timer);
  assert(test_g.ODR == 0);
  HAL_TIM_PeriodElapsedCallback(&timer);
  assert((test_g.ODR & 12U) == GPIO_PIN_2);
  assert((test_e.BSRR & 0xFF00U) == 0x6D00U); /* units = 5 */
  for (int i = 0; i < 50; i++) HAL_TIM_PeriodElapsedCallback(&timer);
  assert((test_g.ODR & 12U) == GPIO_PIN_3);
  assert((test_e.BSRR & 0xFF00U) == 0x5B00U); /* tens = 2 */
  /* Scanning continues while audio or LED effects run. */
  AudioLED_Enabled = 1; AudioLED_Mask = 0x81; AudioLED_Sequence = 1;
  process(1080, 1); assert((test_d.ODR >> 8) == 0x81);
  process(1580, 1); assert((test_d.ODR >> 8) == 0);
  assert(Get7SegDisplayValue() == 25);
  /* Legacy B1 double click stays separate from display-button debounce. */
  uint32_t old_leds = test_g.ODR & (GPIO_PIN_13 | GPIO_PIN_14);
  HAL_GPIO_EXTI_Callback(GPIO_PIN_0);
  assert((test_g.ODR & (GPIO_PIN_13 | GPIO_PIN_14)) != old_leds);
  for (unsigned i = 0; i < 3000; ++i) HAL_TIM_PeriodElapsedCallback(&timer);
  old_leds = test_g.ODR & (GPIO_PIN_13 | GPIO_PIN_14);
  HAL_GPIO_EXTI_Callback(GPIO_PIN_0);
  assert((test_g.ODR & (GPIO_PIN_13 | GPIO_PIN_14)) == old_leds);
  puts("PASS: debounce, held button, 99->00, timer scanning, digit masks, audio timeout");
  return 0;
}
