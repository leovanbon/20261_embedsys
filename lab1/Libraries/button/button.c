#include "button.h"
#include "main.h"

static GPIO_PinState stable;
static GPIO_PinState candidate;
static uint32_t changed_at;

void Button_Init(void)
{
  stable = candidate = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
  changed_at = HAL_GetTick();
}

uint8_t Button_Pressed(uint32_t now)
{
  GPIO_PinState raw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
  if (raw != candidate)
  {
    candidate = raw;
    changed_at = now;
  }
  if (now - changed_at >= 30U && stable != candidate)
  {
    stable = candidate;
    return stable == GPIO_PIN_SET;
  }
  return 0;
}
