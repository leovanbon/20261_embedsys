#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef struct { uint32_t ODR, BSRR; } GPIO_TypeDef;
extern GPIO_TypeDef test_a, test_d, test_e, test_g;
#define GPIOA (&test_a)
#define GPIOD (&test_d)
#define GPIOE (&test_e)
#define GPIOG (&test_g)
#define GPIO_PIN_0 (1U << 0)
#define GPIO_PIN_2 (1U << 2)
#define GPIO_PIN_3 (1U << 3)
#define GPIO_PIN_8 (1U << 8)
#define GPIO_PIN_5 (1U << 5)
#define GPIO_PIN_13 (1U << 13)
#define GPIO_PIN_14 (1U << 14)
#define GPIO_MODE_OUTPUT_PP 1
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_LOW 0
#define __HAL_RCC_GPIOD_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOE_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOG_CLK_ENABLE() ((void)0)
#define TIM6 ((void *)6)
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
typedef struct { void *Instance; } TIM_HandleTypeDef;
void HAL_GPIO_TogglePin(GPIO_TypeDef *, uint16_t);
void HAL_GPIO_EXTI_Callback(uint16_t);
void HAL_GPIO_Init(GPIO_TypeDef *, GPIO_InitTypeDef *);
void HAL_GPIO_WritePin(GPIO_TypeDef *, uint16_t, GPIO_PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *, uint16_t);
uint32_t HAL_GetTick(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *);
#endif
