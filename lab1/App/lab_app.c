#include "lab_app.h"
#include "main.h"
#include "led_effects.h"
#if LAB_HAS_DISPLAY
#include "7seg.h"
#endif
#if LAB_EXERCISE == 2
#include "button.h"
#endif
#if LAB_HAS_IR
#include "irda.h"
#endif

/* State exposed to the ST-LINK monitor. */
volatile uint8_t Remote_ToggleKey = REMOTE_KEY_OK;
volatile uint8_t Remote_LastValue, Remote_LED4State;
volatile uint32_t AudioLED_Mask, AudioLED_Enabled, AudioLED_Sequence;
static uint32_t button_ticks;
#if LAB_HAS_DISPLAY
static uint8_t scan_ticks;
#endif

/* Shared LED and audio handling for exercises 1 and 2. */
#if LAB_EXERCISE <= 2
static void RunLEDs(uint32_t now, uint32_t interval_ms)
{
  static uint32_t last_sequence, last_audio_tick;
  if (!AudioLED_Enabled)
  {
    LED_RunEffects(now, interval_ms);
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

/* Exercise 1: LED effects (LED preset). */
#if LAB_EXERCISE == 1
static void Bai1_Init(void)
{
  LED_Init();
}

static void Bai1_Process(void)
{
  RunLEDs(HAL_GetTick(), LAB_LED_INTERVAL_MS);
}
#endif

/* Exercise 2: seven-segment display and B1 counter (Debug preset). */
#if LAB_EXERCISE == 2
static void Bai2_Init(void)
{
  LED_Init();
  SevenSeg_Init();
  Button_Init();
}

static void Bai2_Process(void)
{
  uint32_t now = HAL_GetTick();
  if (Button_Pressed(now))
    Set7SegDisplayValue(Get7SegDisplayValue() + 1);
  RunLEDs(now, LAB_FAST_INTERVAL_MS);
}
#endif

/* Exercise 3: NEC remote decoding (IR preset). */
#if LAB_EXERCISE == 3
static void Bai3_Init(void)
{
  LED_Init();
  IR_Init();
}

static void Bai3_Process(void)
{
  if (IR_ProcessFrame())
    IR_FrameReady(); /* Read by the monitor after a complete frame. */
}
#endif

/* Exercise 4: remote-controlled LEDs (Remote preset). */
#if LAB_EXERCISE == 4
static void Bai4_Init(void)
{
  LED_Init();
  SevenSeg_Init();
  IR_Init();
  Remote_LastValue = Remote_LED4State = 0;
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
  Set7SegDisplayValue(0);
  LED_SelectMode(0);
}

static void Bai4_HandleKey(uint8_t key)
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

static void Bai4_Process(void)
{
  if (IR_ProcessFrame())
  {
    Bai4_HandleKey(IR_LastKey);
    IR_FrameReady();
  }
  LED_RunSelected(HAL_GetTick(), LAB_LED_INTERVAL_MS);
}
#endif

/* Initialize and run the exercise selected by the build preset. */
void LabApp_Init(void)
{
  button_ticks = 0;
#if LAB_HAS_DISPLAY
  scan_ticks = 0;
#endif
#if LAB_EXERCISE == 1
  Bai1_Init();
#elif LAB_EXERCISE == 2
  Bai2_Init();
#elif LAB_EXERCISE == 3
  Bai3_Init();
#elif LAB_EXERCISE == 4
  Bai4_Init();
#endif
}

void LabApp_Process(void)
{
#if LAB_EXERCISE == 1
  Bai1_Process();
#elif LAB_EXERCISE == 2
  Bai2_Process();
#elif LAB_EXERCISE == 3
  Bai3_Process();
#elif LAB_EXERCISE == 4
  Bai4_Process();
#endif
}

/* Shared timer and GPIO callbacks. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{
  if (timer->Instance != TIM6) return;
  if (button_ticks < LAB_DOUBLE_CLICK_TICKS) ++button_ticks;
#if LAB_HAS_IR
  IR_Tick();
#endif
#if LAB_HAS_DISPLAY
  /* Refresh one digit every 5 ms. */
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
  /* Preserve the sample project's B1 double-click behavior. */
  if (button_ticks < LAB_DOUBLE_CLICK_TICKS)
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13 | GPIO_PIN_14);
#endif
  button_ticks = 0;
}
