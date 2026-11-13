#include "psp_reloc2.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
static void wr(uint8_t *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8));}
static uint32_t rd(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int main(void) {
 uint8_t ram[256]={0}, before[256]; PspCpu cpu;
 PspRelocSegment segments[2]={{BASE,BASE,128},{BASE+128-0x1000,BASE+128,128}};
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 /* Header, two command flags (absolute base/absolute relocation),
  * two types (NONE/32), set base to 8 then relocate word 8 vs segment 1. */
 uint8_t stream[]={0,0,1,1,2,4,5,2,0,2,
                   0,0, 8,0,0,0,
                   7,0, 8,0,0,0};
 wr(ram+8,0x11223344);
 assert(psp_reloc2_apply(&cpu,stream,sizeof stream,segments,2,2)==0);
 assert(rd(ram+8)==0x11224344);
 /* Unknown relocation mapping is rejected without writes. */
 wr(ram+8,0x11223344);memcpy(before,ram,sizeof ram);
 stream[9]=5;
 assert(psp_reloc2_apply(&cpu,stream,sizeof stream,segments,2,2)<0);
 assert(memcmp(ram,before,sizeof ram)==0);
 stream[9]=2;
 /* Out-of-bounds relocation offset must not write guest memory. */
 stream[18]=252;
 assert(psp_reloc2_apply(&cpu,stream,sizeof stream,segments,2,2)<0);
 assert(memcmp(ram,before,sizeof ram)==0);
 stream[18]=8;
 /* Validate truncated stream, invalid indices, impossible bit counts. */
 assert(psp_reloc2_apply(&cpu,stream,sizeof stream-1,segments,2,2)<0);
 assert(memcmp(ram,before,sizeof ram)==0);
 stream[10]=1;
 assert(psp_reloc2_apply(&cpu,stream,sizeof stream,segments,2,2)<0);
 stream[10]=0;
 stream[2]=15;
 assert(psp_reloc2_apply(&cpu,stream,sizeof stream,segments,2,2)<0);
 stream[2]=1;
 stream[8]=0xff;
 assert(psp_reloc2_apply(&cpu,stream,sizeof stream,segments,2,2)==0);
 puts("PASS: bounded PSPREL2 absolute command decoding and atomic failure");
 return 0;
}
