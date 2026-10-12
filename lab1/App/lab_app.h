#ifndef LAB_APP_H
#define LAB_APP_H
#include <stdint.h>
#include "lab_config.h"
extern volatile uint32_t AudioLED_Mask;
extern volatile uint32_t AudioLED_Enabled;
extern volatile uint32_t AudioLED_Sequence;
extern volatile uint8_t Remote_ToggleKey, Remote_LastValue, Remote_LED4State;
void LabApp_Init(void);
void LabApp_Process(void);
#endif
