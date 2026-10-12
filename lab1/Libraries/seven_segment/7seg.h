#ifndef SEVEN_SEGMENT_H
#define SEVEN_SEGMENT_H
#include <stdint.h>
void SevenSeg_Init(void);
void Set7SegDisplayValue(int value);
uint8_t Get7SegDisplayValue(void);
void Run7SegDisplay(void);
#endif
