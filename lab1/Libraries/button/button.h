#ifndef BUTTON_H
#define BUTTON_H
#include <stdint.h>
void Button_Init(void);
/* Returns 1 once per debounced press of active-high B1 on PA0. */
uint8_t Button_Pressed(uint32_t now);
#endif
