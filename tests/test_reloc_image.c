#include "psp_reloc_image.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
#define PSPREL UINT32_C(0x700000a0)
#define PSPREL2 UINT32_C(0x700000a1)
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static uint32_t r32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void ph(uint8_t *elf,int index,uint32_t type,uint32_t offset,
               uint32_t vaddr,uint32_t filesz,uint32_t memsz){
 uint8_t *p=elf+52+(size_t)index*32u;
 w32(p,type);w32(p+4,offset);w32(p+8,vaddr);
 w32(p+16,filesz);w32(p+20,memsz);
}
static void image(uint8_t *elf){
 memset(elf,0,512);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,0xffa0);w16(elf+18,8);w32(elf+20,1);
 w32(elf+28,52);w16(elf+40,52);w16(elf+42,32);w16(elf+44,4);
 ph(elf,0,1,320,BASE,32,64);
 ph(elf,1,1,352,BASE+128-0x1000,32,64);
 ph(elf,2,PSPREL,400,0,8,8);
 ph(elf,3,PSPREL2,416,0,22,22);
 w32(elf+400,8);w32(elf+404,(1u<<16)|2u);
 /* PSPREL2 flag table: base-set-absolute, reloc-absolute;
  * type table: NONE, R_MIPS_32; target seg 0, source seg 1. */
 const uint8_t rel2[]={0,0,1,1,2,4,5,2,0,2,
                        0,0,12,0,0,0,
                        11,0,12,0,0,0};
 memcpy(elf+416,rel2,sizeof rel2);
}
static void init(PspCpu *cpu,uint8_t ram[256]){
 memset(ram,0xa5,256);
 psp_cpu_init(cpu,ram,256,BASE,BASE);
 w32(ram,0x24080007u); /* addiu t0, zero, 7 */
 w32(ram+8,0x33);w32(ram+12,0x55);
}
int main(void){
 uint8_t elf[512],ram[256],before[256];PspCpu cpu;
 PspRelocSegment seg[4]={
  {BASE,BASE,64},{BASE+128-0x1000,BASE+128,64},{0,0,0},{0,0,0}
 };
 size_t applied=999;
 image(elf);init(&cpu,ram);
 assert(psp_reloc_image_apply(elf,sizeof elf,&cpu,seg,4,&applied)==0);
 assert(applied==2 && r32(ram+8)==BASE+128+0x33 && r32(ram+12)==BASE+128+0x55);
 assert(cpu.pc==BASE);
 assert(psp_cpu_step(&cpu)==0 && cpu.gpr[8]==7);
 /* Failure in second relocation stream rolls back first stream's write. */
 image(elf);init(&cpu,ram);memcpy(before,ram,sizeof ram);
 elf[416+9]=5;applied=99;
 assert(psp_reloc_image_apply(elf,sizeof elf,&cpu,seg,4,&applied)<0);
 assert(applied==0 && memcmp(before,ram,sizeof ram)==0);
 /* Truncated/malformed program header ranges fail before any writes. */
 image(elf);init(&cpu,ram);memcpy(before,ram,sizeof ram);
 ph(elf,3,PSPREL2,507,0,22,22);
 assert(psp_reloc_image_apply(elf,sizeof elf,&cpu,seg,4,&applied)<0);
 assert(memcmp(before,ram,sizeof ram)==0);
 image(elf);init(&cpu,ram);memcpy(before,ram,sizeof ram);
 w16(elf+44,5);
 assert(psp_reloc_image_apply(elf,sizeof elf,&cpu,seg,4,&applied)<0);
 assert(memcmp(before,ram,sizeof ram)==0);
 image(elf);init(&cpu,ram);memcpy(before,ram,sizeof ram);
 PspRelocSegment wrong[4];memcpy(wrong,seg,sizeof seg);
 wrong[0].original_vaddr++;
 assert(psp_reloc_image_apply(elf,sizeof elf,&cpu,wrong,4,&applied)<0);
 assert(memcmp(before,ram,sizeof ram)==0);
 image(elf);init(&cpu,ram);memcpy(before,ram,sizeof ram);
 ph(elf,2,0,400,0,8,8);ph(elf,3,0,416,0,22,22);
 assert(psp_reloc_image_apply(elf,sizeof elf,&cpu,seg,4,&applied)<0);
 assert(memcmp(before,ram,sizeof ram)==0);
 /* A single valid uncompressed relocation stream remains supported. */
 image(elf);init(&cpu,ram);ph(elf,3,0,416,0,22,22);
 assert(psp_reloc_image_apply(elf,sizeof elf,&cpu,seg,4,&applied)==0);
 assert(applied==1 && r32(ram+8)==BASE+128+0x33 && r32(ram+12)==0x55);
 puts("PASS: PRX ELF stream discovery, PSPREL/PSPREL2, rollback and guest execution");
 return 0;
}
