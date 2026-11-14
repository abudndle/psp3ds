#include "psp_reloc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void wr(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static uint32_t rd(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void rec(uint8_t *p,uint32_t off,unsigned target,unsigned source,unsigned type){wr(p,off);wr(p+4,(source<<16)|(target<<8)|type);}
static void reset(uint8_t *ram,PspCpu *cpu,PspRelocSegment *seg){
 memset(ram,0,256);psp_cpu_init(cpu,ram,256,BASE,BASE);
 seg[0]=(PspRelocSegment){BASE,BASE,128};
 seg[1]=(PspRelocSegment){BASE+128-0x1000,BASE+128,128};
}
int main(void){
 uint8_t mem[256],recs[48],before[256];PspCpu cpu;PspRelocSegment s[2];
 reset(mem,&cpu,s);memset(recs,0,sizeof recs);
 /* A 0x7ff0 low addend plus 0x1000 needs a carry to upper 16 bits. */
 wr(mem+8,0x3c081234);wr(mem+12,0x25087ff0);
 rec(recs,8,0,1,5);rec(recs+8,12,0,1,6);
 assert(psp_reloc_apply(&cpu,recs,16,s,2)==0);
 assert(rd(mem+8)==0x3c081235 && rd(mem+12)==0x25088ff0);
 /* A sign-extended negative low addend must not trigger a spurious carry. */
 reset(mem,&cpu,s);wr(mem+8,0x3c081234);wr(mem+12,0x25088000);
 assert(psp_reloc_apply(&cpu,recs,16,s,2)==0);
 assert(rd(mem+8)==0x3c081234 && rd(mem+12)==0x25089000);
 /* Multiple HI16 uses can share one LO16, keeping each original upper addend. */
 reset(mem,&cpu,s);wr(mem+8,0x3c081234);wr(mem+12,0x3c091111);wr(mem+16,0x25087ff0);
 rec(recs,8,0,1,5);rec(recs+8,12,0,1,5);rec(recs+16,16,0,1,6);
 assert(psp_reloc_apply(&cpu,recs,24,s,2)==0);
 assert(rd(mem+8)==0x3c081235 && rd(mem+12)==0x3c091112 && rd(mem+16)==0x25088ff0);
 /* A LO16 without a preceding HI16 is still a valid immediate relocation. */
 reset(mem,&cpu,s);wr(mem+16,0x2508000f);rec(recs,16,0,1,6);
 assert(psp_reloc_apply(&cpu,recs,8,s,2)==0 && rd(mem+16)==0x2508100f);
 /* An unmatched HI16 fails without patching any previously staged R_MIPS_32. */
 reset(mem,&cpu,s);wr(mem+4,7);wr(mem+8,0x3c081234);
 rec(recs,4,0,1,2);rec(recs+8,8,0,1,5);memcpy(before,mem,sizeof mem);
 assert(psp_reloc_apply(&cpu,recs,16,s,2)<0 && memcmp(mem,before,sizeof mem)==0);
 /* Mismatched source segment selectors are not allowed for HI/LO pairs. */
 reset(mem,&cpu,s);wr(mem+8,0x3c081234);wr(mem+12,0x25087ff0);
 rec(recs,8,0,1,5);rec(recs+8,12,0,0,6);memcpy(before,mem,sizeof mem);
 assert(psp_reloc_apply(&cpu,recs,16,s,2)<0 && memcmp(mem,before,sizeof mem)==0);
 /* A second relocation to a pending HI16 is rejected atomically. */
 reset(mem,&cpu,s);wr(mem+8,0x3c081234);wr(mem+12,0x25087ff0);
 rec(recs,8,0,1,5);rec(recs+8,8,0,1,2);memcpy(before,mem,sizeof mem);
 assert(psp_reloc_apply(&cpu,recs,16,s,2)<0 && memcmp(mem,before,sizeof mem)==0);
 /* Invalid opcode at the HI/LO target must fail with no writes. */
 reset(mem,&cpu,s);wr(mem+8,0x24081234);wr(mem+12,0x25087ff0);
 rec(recs,8,0,1,5);rec(recs+8,12,0,1,6);memcpy(before,mem,sizeof mem);
 assert(psp_reloc_apply(&cpu,recs,16,s,2)<0 && memcmp(mem,before,sizeof mem)==0);
 /* Jetpack Joyride uses LO16 on loads and stores, not only ADDIU. */
 const unsigned memory_ops[]={32,35,36,40,43,49,57};
 for(size_t i=0;i<sizeof(memory_ops)/sizeof(memory_ops[0]);i++){
  reset(mem,&cpu,s);wr(mem+8,0x3c081234);
  uint32_t low=(memory_ops[i]<<26)|(8u<<21)|(8u<<16)|0x7ff0u;
  wr(mem+12,low);
  rec(recs,8,0,1,5);rec(recs+8,12,0,1,6);
  assert(psp_reloc_apply(&cpu,recs,16,s,2)==0);
  assert(rd(mem+8)==0x3c081235);
  assert(rd(mem+12)==((low&0xffff0000u)|0x8ff0u));
 }
 /* Standalone memory LO16 can be encoded when its signed offset fits. */
 reset(mem,&cpu,s);wr(mem+12,0x8d080010);
 rec(recs,12,0,1,6);
 assert(psp_reloc_apply(&cpu,recs,8,s,2)==0);
 assert(rd(mem+12)==0x8d081010);
 /* Negative signed offsets remain valid after a small positive delta. */
 reset(mem,&cpu,s);wr(mem+12,0xad08fff0);
 rec(recs,12,0,1,6);
 assert(psp_reloc_apply(&cpu,recs,8,s,2)==0);
 assert(rd(mem+12)==0xad080ff0);
 /* Overflowing standalone memory LO16 must be rejected atomically. */
 reset(mem,&cpu,s);wr(mem+12,0x8d087ff0);
 rec(recs,12,0,1,6);memcpy(before,mem,sizeof mem);
 assert(psp_reloc_apply(&cpu,recs,8,s,2)<0 && memcmp(mem,before,sizeof mem)==0);
 /* Invalid segment bounds reject the whole batch before applying changes. */
 reset(mem,&cpu,s);wr(mem+8,0x3c081234);wr(mem+12,0x25087ff0);
 rec(recs,8,0,1,5);rec(recs+8,12,0,1,6);rec(recs+16,127,0,1,2);
 memcpy(before,mem,sizeof mem);
 assert(psp_reloc_apply(&cpu,recs,24,s,2)<0 && memcmp(mem,before,sizeof mem)==0);
 puts("PASS: HI16/LO16 carry, signed addends, multi-HI pairing, atomicity");return 0;
}
