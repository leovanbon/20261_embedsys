#include "irda.h"
#include <assert.h>
#include <stdio.h>
static void edge(unsigned ticks) { ir_ticks = ticks; IrGetBitTime(); }
static void frame(unsigned key, unsigned inv) {
 edge(1000); edge(135);
 unsigned bytes[4] = {0,255,key,inv};
 for(unsigned k=0;k<4;k++) for(unsigned b=0;b<8;b++)
  edge((bytes[k] & (1U<<b)) ? 23 : 11);
 assert(command_ok); IrDecode(); command_ok=0;
}
int main(void) {
 IR_Init();
 IR_Tick(); assert(ir_ticks == 1001);
 frame(0xAD,0x52);
 assert(IR_FrameCount==1 && IR_LastKey==0xAD);
 assert(irda_cmd[0]==0 && irda_cmd[1]==255 && irda_cmd[3]==0x52);
 edge(1000);edge(113); assert(!command_ok);
 frame(0x12,0x12); assert(IR_FrameCount==1);
 edge(135);edge(11);edge(40);assert(!command_ok);
 frame(0x45,0xBA);assert(IR_FrameCount==2 && IR_LastKey==0x45);
 puts("PASS: NEC LSB-first decoding, repeat ignored, invalid complement, incomplete frame recovery");
}
