#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H
#include <stdint.h>
extern unsigned char LED_Value;
extern unsigned char LED_Mode;
extern unsigned char LED_Cycles;
extern unsigned char LED_CycleDone;
void LED_Init(void);
void LED_SelectMode(uint8_t mode);
void LED_RunSelected(uint32_t now, uint32_t interval_ms);
void DisplayLEDs(int mode);
void LED_RunEffects(uint32_t now, uint32_t interval_ms);
#endif
