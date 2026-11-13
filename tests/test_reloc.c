#include "psp_reloc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void wr(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static uint32_t rd(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void rec(uint8_t *p,uint32_t off,unsigned ofs,unsigned addr,unsigned type){wr(p,off);wr(p+4,(addr<<16)|(ofs<<8)|type);}
int main(void){
 uint8_t ram[256]={0},rels[32]={0},snapshot[256];PspCpu cpu;
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 PspRelocSegment seg[2]={{0,BASE,128},{0x08000000u,BASE+128,128}};
 /* R_MIPS_32 resolves a pointer from segment 1, at an offset in segment 0. */
 wr(ram+8,0x10);rec(rels,8,0,1,2);
 assert(psp_reloc_apply(&cpu,rels,8,seg,2)==0);
 assert(rd(ram+8)==BASE+128-0x08000000u+0x10);
 /* R_MIPS_26 patch of a jump with segment relocation delta 0x1000. */
 seg[1].original_vaddr=BASE+128-0x1000;
 wr(ram+16,0x08000040u);rec(rels,16,0,1,4);
 assert(psp_reloc_apply(&cpu,rels,8,seg,2)==0);
 assert(rd(ram+16)==0x08000440u);
 /* R_MIPS_NONE does not touch memory. */
 memcpy(snapshot,ram,sizeof ram);rec(rels,250,0,1,0);
 assert(psp_reloc_apply(&cpu,rels,8,seg,2)==0 && memcmp(snapshot,ram,sizeof ram)==0);
 /* Unsupported types reject entire batch without partial changes. */
 wr(ram+20,33);rec(rels,20,0,1,2);rec(rels+8,24,0,1,5);
 memcpy(snapshot,ram,sizeof ram);
 assert(psp_reloc_apply(&cpu,rels,16,seg,2)<0 && memcmp(snapshot,ram,sizeof ram)==0);
 rec(rels+8,20,0,1,2);
 assert(psp_reloc_apply(&cpu,rels,16,seg,2)<0 && memcmp(snapshot,ram,sizeof ram)==0);
 rec(rels+8,24,2,1,2);
 assert(psp_reloc_apply(&cpu,rels,16,seg,2)<0 && memcmp(snapshot,ram,sizeof ram)==0);
 rec(rels+8,24,0,1,2);rec(rels+8,127,0,1,2);
 assert(psp_reloc_apply(&cpu,rels,16,seg,2)<0 && memcmp(snapshot,ram,sizeof ram)==0);
 rec(rels+8,24,0,1,2);rec(rels+8,24,0,1,4);
 wr(ram+24,0x08000040u);memcpy(snapshot,ram,sizeof ram);
 seg[1].mapped_vaddr=BASE+129;
 assert(psp_reloc_apply(&cpu,rels,16,seg,2)<0 && memcmp(snapshot,ram,sizeof ram)==0);
 seg[1].mapped_vaddr=BASE+128;
 assert(psp_reloc_apply(&cpu,rels,7,seg,2)<0);
 assert(psp_reloc_apply(&cpu,rels,0,seg,2)==0);
 printf("PASS: PSP R_MIPS_32/R_MIPS_26 relocations, validation and atomicity\n");
 return 0;
}
