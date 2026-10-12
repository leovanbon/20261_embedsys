#include "led_effects.h"
#include "main.h"

unsigned char LED_Value = 1;
unsigned char LED_Mode = 1;
unsigned char LED_Cycles, LED_CycleDone;
static unsigned char step;
static int previous_mode;
static uint32_t last_tick;

void LED_Init(void)
{
  LED_Value = LED_Mode = 1;
  LED_Cycles = LED_CycleDone = step = 0;
  previous_mode = 0;
  last_tick = 0;
  __HAL_RCC_GPIOD_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOD, 0xFF00U, GPIO_PIN_RESET);
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = 0xFF00U;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &gpio);
}

/* Both exercise modes share one timing path; only auto switching differs. */
static void Run(uint32_t now, uint32_t interval_ms, uint8_t automatic)
{
  if (now - last_tick < interval_ms) return;
  last_tick = now;
  DisplayLEDs(LED_Mode);
  if (automatic && LED_CycleDone && ++LED_Cycles == 2)
  {
    LED_Cycles = 0;
    LED_Mode = LED_Mode == 4 ? 1 : LED_Mode + 1;
  }
}

void LED_RunEffects(uint32_t now, uint32_t interval_ms)
{
  Run(now, interval_ms, 1);
}

void LED_SelectMode(uint8_t mode)
{
  if (mode > 4) return;
  LED_Mode = mode;
  LED_Cycles = 0;
  step = 0;
  previous_mode = mode;
  LED_Value = 0;
  DisplayLEDs(mode);
}

void LED_RunSelected(uint32_t now, uint32_t interval_ms)
{
  Run(now, interval_ms, 0);
}

void DisplayLEDs(int mode)
{
  /* Bit 0 maps to PD8; bit 7 maps to PD15. Include off frames at
     both ends of each cycle as shown in the lab handout. */
  static const unsigned char effects[4][10] = {
    {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00},
    {0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00},
    {0x00, 0xFF, 0x00},
    {0x00, 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81, 0x00}
  };
  static const unsigned char lengths[4] = {10, 10, 3, 10};
  LED_CycleDone = 0;
  if (mode >= 1 && mode <= 4)
  {
    if (mode != previous_mode)
      step = 0;
    previous_mode = mode;
    LED_Value = effects[mode - 1][step];
    if (++step >= lengths[mode - 1])
    {
      step = 0;
      LED_CycleDone = 1;
    }
  }
  for (unsigned int bit = 0; bit < 8; bit++)
  {
    HAL_GPIO_WritePin(GPIOD, (uint16_t)(GPIO_PIN_8 << bit),
                      (LED_Value & (1U << bit)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
}

