#include "psp_reloc.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Original synthetic PRX relocation fixture with a record count that
 * exceeds the old 65,536-entry cap, like some real PSP titles.
 * No proprietary game code or relocation records are included. */
#define RELOC_COUNT 83187u
#define BASE UINT32_C(0x08800000)
static void w32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t r32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
int main(void){
 const size_t bytes=(size_t)RELOC_COUNT*4u;
 const size_t record_bytes=(size_t)RELOC_COUNT*8u;
 uint8_t *guest=(uint8_t*)malloc(bytes);
 uint8_t *records=(uint8_t*)malloc(record_bytes);
 uint8_t *previous=(uint8_t*)malloc(bytes);
 assert(guest&&records&&previous);
 const unsigned opcode[]={9,35,43,57,49,36,40,32};
 for(unsigned i=0;i<RELOC_COUNT;i++){
  w32(guest+i*4u,(opcode[i%8]<<26)|(8u<<21)|(9u<<16)|0x0020u);
  w32(records+i*8u,i*4u);
  w32(records+i*8u+4u,(1u<<16)|6u);
 }
 PspCpu cpu;
 psp_cpu_init(&cpu,guest,bytes,BASE,BASE);
 PspRelocSegment segments[2]={
  {BASE,BASE,(uint32_t)bytes},
  {0x00010000u,0x00011000u,4096u}
 };
 assert(psp_reloc_apply(&cpu,records,record_bytes,segments,2)==0);
 for(unsigned i=0;i<RELOC_COUNT;i++){
  uint32_t x=r32(guest+i*4u);
  assert((x&0xffffu)==0x1020u);
  assert((x>>26)==opcode[i%8]);
 }
 /* A duplicate target near the end must reject the whole batch with
  * no changes, even when there are over eighty thousand valid records. */
 memcpy(previous,guest,bytes);
 w32(records+(RELOC_COUNT-1u)*8u,0u);
 assert(psp_reloc_apply(&cpu,records,record_bytes,segments,2)!=0);
 assert(memcmp(guest,previous,bytes)==0);
 /* A record stream exceeding the bounded maximum must fail early. */
 assert(psp_reloc_apply(&cpu,records,131073u*8u,segments,2)!=0);
 assert(memcmp(guest,previous,bytes)==0);
 /* HI16 paired with a signed-offset LW must carry into LUI correctly. */
 uint8_t hi_ram[8],pair[16];
 w32(hi_ram,0x3c080001u); /* lui t0,0x0001 */
 w32(hi_ram+4,0x8d090010u); /* lw t1,0x0010(t0) */
 w32(pair,0u);w32(pair+4,(1u<<16)|5u);
 w32(pair+8,4u);w32(pair+12,(1u<<16)|6u);
 psp_cpu_init(&cpu,hi_ram,sizeof hi_ram,BASE,BASE);
 PspRelocSegment tiny[2]={{BASE,BASE,8u},{0x10000u,0x19000u,8u}};
 assert(psp_reloc_apply(&cpu,pair,sizeof pair,tiny,2)==0);
 assert(r32(hi_ram)==0x3c080002u);
 assert(r32(hi_ram+4)==0x8d090010u);
 free(previous);free(records);free(guest);
 puts("PASS: 83187 synthetic PSP PRX relocations, load/store LO16, atomic duplicates");
 return 0;
}
