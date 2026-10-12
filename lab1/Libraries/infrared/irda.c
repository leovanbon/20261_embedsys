/* Adapted from Lab 1/IrRemoteControl.zip: IrGetBitTime + IrDecode.
   Original source is retained in sample/. Timer units: 100 us. */
#include "irda.h"
volatile uint32_t ir_ticks = 1000, IR_EdgeCount, IR_FrameCount;
volatile uint32_t IR_LastInterval, IR_LastFrame;
volatile int bit_time[33];
static int bit_count;
volatile int command_ok;
volatile unsigned char irda_cmd[4];
volatile unsigned char IR_LastKey;

void IR_Init(void)
{
  ir_ticks = 1000;
  bit_count = command_ok = 0;
  IR_EdgeCount = IR_FrameCount = IR_LastInterval = IR_LastFrame = 0;
  IR_LastKey = 0;
  for (unsigned i = 0; i < 4; ++i) irda_cmd[i] = 0;
  for (unsigned i = 0; i < 33; ++i) bit_time[i] = 0;
}

void IR_Tick(void)
{
  if (ir_ticks < 1000000U) ++ir_ticks;
}

void IrGetBitTime(void)
{
  uint32_t elapsed = ir_ticks;
  ir_ticks = 0;
  IR_LastInterval = elapsed;
  ++IR_EdgeCount;
  if (command_ok) return; /* Keep completed frame stable for main. */
  if (elapsed >= 120 && elapsed <= 150)
  {
    bit_time[0] = (int)elapsed;
    bit_count = 1;
    return;
  }
  if (!bit_count) return;
  if (!((elapsed >= 8 && elapsed <= 15) ||
        (elapsed >= 18 && elapsed <= 27)))
  {
    bit_count = 0; /* Includes repeat frames and incomplete frames. */
    return;
  }
  bit_time[bit_count++] = (int)elapsed;
  if (bit_count == 33)
  {
    bit_count = 0;
    command_ok = 1;
  }
}

void IrDecode(void)
{
  uint32_t frame = 0;
  /* bit_time[0] is the leader; remaining intervals carry LSB-first data. */
  for (unsigned bit = 0; bit < 32; ++bit)
    if (bit_time[bit + 1] > 17) frame |= (uint32_t)1 << bit;
  uint8_t key = (uint8_t)(frame >> 16);
  if ((uint8_t)(key ^ (uint8_t)(frame >> 24)) == 0xFFU)
  {
    for (unsigned i = 0; i < 4; ++i)
      irda_cmd[i] = (uint8_t)(frame >> (8 * i));
    IR_LastFrame = frame;
    IR_LastKey = key;
    ++IR_FrameCount;
  }
}

uint8_t IR_ProcessFrame(void)
{
  if (!command_ok) return 0;
  uint32_t previous = IR_FrameCount;
  IrDecode();
  command_ok = 0; /* Release the capture buffer only after decoding. */
  return IR_FrameCount != previous;
}

/* Terminal reader sets a breakpoint here, after each decoded frame. */
__attribute__((noinline)) void IR_FrameReady(void)
{
  __asm__ volatile ("nop");
}
