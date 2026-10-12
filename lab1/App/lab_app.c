#include "lab_app.h"
#include "led_effects.h"
#include "main.h"
#if LAB_HAS_DISPLAY
#include "7seg.h"
#endif
#if LAB_EXERCISE == 2
#include "button.h"
#endif
#if LAB_HAS_IR
#include "irda.h"
#endif

/* Values visible through ST-LINK Watch and tools/ir_monitor.py. */
volatile uint8_t Remote_ToggleKey = REMOTE_KEY_PLAY_PAUSE;
volatile uint8_t Remote_LastValue, Remote_LED4State;
/* Keep the existing audio script compatible with exercises 1 and 2. */
volatile uint32_t AudioLED_Mask, AudioLED_Enabled, AudioLED_Sequence;
static uint32_t button_ticks;
#if LAB_HAS_DISPLAY
static uint8_t scan_ticks;
#endif

void LabApp_Init(void)
{
  LED_Init();
  button_ticks = 0;
#if LAB_HAS_DISPLAY
  scan_ticks = 0;
  SevenSeg_Init();
#endif
#if LAB_EXERCISE == 2
  Button_Init();
#endif
#if LAB_HAS_IR
  IR_Init();
#endif
#if LAB_EXERCISE == 4
  Remote_LastValue = Remote_LED4State = 0;
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
  Set7SegDisplayValue(0);
  LED_SelectMode(0);
#endif
}

#if LAB_EXERCISE == 4
static void ProcessRemoteKey(uint8_t key)
{
  static const uint8_t effect_keys[] = {
    REMOTE_KEY_1, REMOTE_KEY_2, REMOTE_KEY_3, REMOTE_KEY_4
  };
  Remote_LastValue = key;
  Set7SegDisplayValue(key);
  if (key == Remote_ToggleKey)
  {
    Remote_LED4State ^= 1U;
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14,
                     Remote_LED4State ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
  for (uint8_t i = 0; i < 4; ++i)
    if (key == effect_keys[i])
    {
      LED_SelectMode(i + 1U);
      break;
    }
}
#endif

#if LAB_EXERCISE <= 2
static void ProcessLEDs(uint32_t now)
{
  static uint32_t last_sequence, last_audio_tick;
  if (!AudioLED_Enabled)
  {
    LED_RunEffects(now, LAB_EXERCISE == 2 ? LAB_FAST_INTERVAL_MS
                                        : LAB_LED_INTERVAL_MS);
    return;
  }
  if (last_sequence != AudioLED_Sequence)
  {
    last_sequence = AudioLED_Sequence;
    last_audio_tick = now;
  }
  LED_Value = now - last_audio_tick < 500U ? (uint8_t)AudioLED_Mask : 0;
  DisplayLEDs(0);
}
#endif

void LabApp_Process(void)
{
#if LAB_EXERCISE <= 2
  uint32_t now = HAL_GetTick();
#if LAB_EXERCISE == 2
  if (Button_Pressed(now))
    Set7SegDisplayValue(Get7SegDisplayValue() + 1);
#endif
  ProcessLEDs(now);
#else
  if (IR_ProcessFrame())
  {
#if LAB_EXERCISE == 4
    ProcessRemoteKey(IR_LastKey);
#endif
    IR_FrameReady(); /* PuTTY monitor reads only after a complete frame. */
  }
#if LAB_EXERCISE == 4
  LED_RunSelected(HAL_GetTick(), LAB_LED_INTERVAL_MS);
#endif
#endif
}

/* Core interrupt handlers only dispatch to HAL; application work lives here. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{
  if (timer->Instance != TIM6) return;
  if (button_ticks < LAB_DOUBLE_CLICK_TICKS) ++button_ticks;
#if LAB_HAS_IR
  IR_Tick();
#endif
#if LAB_HAS_DISPLAY
  if (++scan_ticks == LAB_SCAN_TICKS)
  {
    scan_ticks = 0;
    Run7SegDisplay();
  }
#endif
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
#if LAB_HAS_IR
  if (pin == GPIO_PIN_5) IrGetBitTime();
#endif
  if (pin != GPIO_PIN_0) return;
#if LAB_EXERCISE != 4
  if (button_ticks < LAB_DOUBLE_CLICK_TICKS)
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13 | GPIO_PIN_14);
#endif
  button_ticks = 0;
}
