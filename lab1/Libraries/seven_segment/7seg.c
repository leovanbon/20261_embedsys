#include "7seg.h"
#include "main.h"

/* Common cathode; a..g,dp on PE8..PE15. PG2 = units, PG3 = tens. */
static const uint8_t digit_masks[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
static volatile uint16_t segment_frame;
static uint8_t display_value;
static uint8_t position;

void SevenSeg_Init(void)
{
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, 0xFF00U, GPIO_PIN_RESET);
  GPIO_InitTypeDef gpio = {0};
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Pin = 0xFF00U;
  HAL_GPIO_Init(GPIOE, &gpio);
  gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  HAL_GPIO_Init(GPIOG, &gpio);
  position = 0;
  Set7SegDisplayValue(25);
}

void Set7SegDisplayValue(int value)
{
  value %= 100;
  if (value < 0)
    value += 100;
  display_value = (uint8_t)value;
  /* One aligned write prevents the timer reading a partly updated frame. */
  segment_frame = (uint16_t)(digit_masks[value % 10]
                            | ((uint16_t)digit_masks[value / 10] << 8));
}

uint8_t Get7SegDisplayValue(void)
{
  return display_value;
}

void Run7SegDisplay(void)
{
  uint16_t frame = segment_frame;
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
  uint8_t mask = (position == 0) ? (uint8_t)frame : (uint8_t)(frame >> 8);
  uint32_t pins = (uint32_t)mask << 8;
  GPIOE->BSRR = ((0xFF00U & ~pins) << 16) | pins;
  HAL_GPIO_WritePin(GPIOG, position == 0 ? GPIO_PIN_2 : GPIO_PIN_3, GPIO_PIN_SET);
  position ^= 1U;
}
