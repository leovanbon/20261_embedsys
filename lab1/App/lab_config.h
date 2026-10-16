#ifndef LAB_CONFIG_H
#define LAB_CONFIG_H

/* Selected by the CMake preset: LED=1, Debug=2, IR=3, Remote=4. */
#ifndef LAB_EXERCISE
#define LAB_EXERCISE 2
#endif
#if LAB_EXERCISE < 1 || LAB_EXERCISE > 4
#error "LAB_EXERCISE must be between 1 and 4"
#endif
#define LAB_HAS_IR      (LAB_EXERCISE >= 3)
#define LAB_HAS_DISPLAY (LAB_EXERCISE == 2 || LAB_EXERCISE == 4)

/* TIM6: 90 MHz / (89 + 1) / (99 + 1) = 10 kHz = 100 us/tick. */
#define LAB_SCAN_TICKS       50U
#define LAB_LED_INTERVAL_MS  200U
#define LAB_FAST_INTERVAL_MS 5U
#define LAB_DOUBLE_CLICK_TICKS 3000U

/* Actual command bytes measured on the lab remote. */
#define REMOTE_KEY_OK         0x1CU
#define REMOTE_KEY_1          0x45U
#define REMOTE_KEY_2          0x46U
#define REMOTE_KEY_3          0x47U
#define REMOTE_KEY_4          0x44U

#endif
