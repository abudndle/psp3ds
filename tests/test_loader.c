#include "psp_loader.h"
#include "psp_core.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static void w16(uint8_t *p, unsigned x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void w32(uint8_t *p, uint32_t x){for(int i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8));}
static void make_elf(uint8_t *buf){
 memset(buf,0,256); memcpy(buf,"\177ELF",4);buf[4]=1;buf[5]=1;buf[6]=1;
 w16(buf+16,2);w16(buf+18,8);w32(buf+20,1);w32(buf+24,0x08800000);
 w32(buf+28,52);w16(buf+40,52);w16(buf+42,32);w16(buf+44,1);
 w32(buf+52,1);w32(buf+56,128);w32(buf+60,0x08800000);
 w32(buf+68,4);w32(buf+72,8);w32(buf+76,5);w32(buf+80,4);
 w32(buf+128,0x24080001);
}
int main(void){
 uint8_t elf[256],ram[256];PspLoadResult r; make_elf(elf);
 memset(ram,0xaa,sizeof(ram));
 assert(psp_elf_load(elf,sizeof(elf),ram,sizeof(ram),0x08800000,&r)==0);
 assert(r.entry==0x08800000 && r.segments==1);
 assert(ram[0]==1 && ram[1]==0 && ram[2]==8 && ram[3]==0x24);
 assert(ram[4]==0 && ram[7]==0 && ram[8]==0xaa);
 PspCpu cpu; psp_cpu_init(&cpu,ram,sizeof(ram),0x08800000,r.entry);
 assert(psp_cpu_step(&cpu)==0 && cpu.gpr[8]==1);
 make_elf(elf);w32(elf+72,300);memset(ram,0xaa,sizeof(ram));
 assert(psp_elf_load(elf,sizeof(elf),ram,sizeof(ram),0x08800000,&r)==PSP_LOAD_BAD_SEGMENT);
 assert(ram[0]==0xaa);
 make_elf(elf);w32(elf+60,0x08800100);
 assert(psp_elf_load(elf,sizeof(elf),ram,sizeof(ram),0x08800000,&r)==PSP_LOAD_BAD_SEGMENT);
 make_elf(elf);w32(elf+76,4);
 assert(psp_elf_load(elf,sizeof(elf),ram,sizeof(ram),0x08800000,&r)==PSP_LOAD_ENTRY_INVALID);
 make_elf(elf);w16(elf+16,1);
 assert(psp_elf_load(elf,sizeof(elf),ram,sizeof(ram),0x08800000,&r)==PSP_LOAD_UNSUPPORTED);
 make_elf(elf);w32(elf+28,255);
 assert(psp_elf_load(elf,sizeof(elf),ram,sizeof(ram),0x08800000,&r)==PSP_LOAD_BAD_SEGMENT);
 puts("PASS: ELF32 PSP executable loader checks"); return 0;
}
