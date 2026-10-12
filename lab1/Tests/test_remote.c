#include "main.h"
#include "lab_app.h"
#include "led_effects.h"
#include "7seg.h"
#include "irda.h"
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
static void edge(unsigned ticks) { ir_ticks = ticks; HAL_GPIO_EXTI_Callback(GPIO_PIN_5); }
static void press(unsigned key)
{
  edge(1000); edge(135);
  unsigned bytes[4] = {0, 255, key, key ^ 255};
  for (unsigned k = 0; k < 4; k++)
    for (unsigned b = 0; b < 8; b++)
      edge((bytes[k] & (1U << b)) ? 23 : 11);
  assert(command_ok);
  LabApp_Process();
  assert(!command_ok);
}
int main(void)
{
  LabApp_Init();
  assert(Get7SegDisplayValue() == 0 && LED_Mode == 0);
  const unsigned keys[] = {0x0C, 0x18, 0x5E, 0x08};
  for (unsigned i = 0; i < 4; i++)
  {
    press(keys[i]);
    assert(LED_Mode == i + 1 && Get7SegDisplayValue() == keys[i]);
    for (unsigned step = 0; step < 40; step++)
    {
      tick += 200;
      LabApp_Process();
      assert(LED_Mode == i + 1); /* Hold selected mode beyond two cycles. */
    }
  }
  press(Remote_ToggleKey);
  assert(Remote_LED4State == 1 && (test_g.ODR & GPIO_PIN_14));
  edge(1000); edge(113); LabApp_Process(); /* Repeat must not toggle. */
  assert(Remote_LED4State == 1);
  press(Remote_ToggleKey);
  assert(Remote_LED4State == 0 && !(test_g.ODR & GPIO_PIN_14));
  HAL_GPIO_EXTI_Callback(GPIO_PIN_0);
  assert(Remote_LED4State == 0 && !(test_g.ODR & GPIO_PIN_14));
  press(0x16);
  assert(Get7SegDisplayValue() == 22 && LED_Mode == 4);
  TIM_HandleTypeDef timer = {TIM6};
  for (unsigned i = 0; i < 50; i++) HAL_TIM_PeriodElapsedCallback(&timer);
  assert((test_g.ODR & 12U) == GPIO_PIN_2);
  assert((test_e.BSRR & 0xFF00U) == 0x5B00U);
  puts("PASS: remote 1-4 mapping, persistent effects, display, LED4 toggle, repeat ignored, timer scanning");
}
