#ifndef IRDA_H
#define IRDA_H
#include <stdint.h>
extern volatile uint32_t ir_ticks, IR_EdgeCount, IR_FrameCount;
extern volatile uint32_t IR_LastInterval, IR_LastFrame;
extern volatile int command_ok;
extern volatile unsigned char irda_cmd[4];
extern volatile unsigned char IR_LastKey;
extern volatile int bit_time[33];
void IrGetBitTime(void);
void IrDecode(void);
void IR_Init(void);
void IR_Tick(void);
uint8_t IR_ProcessFrame(void);
void IR_FrameReady(void);
#endif
