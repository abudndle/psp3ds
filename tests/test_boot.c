#include "psp_boot.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void w16(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t r32(const uint8_t *p){return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;}
static void fixture(uint8_t *elf){
 memset(elf,0,1024);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,2);w16(elf+18,8);w32(elf+20,1);w32(elf+24,BASE);
 w32(elf+28,52);w32(elf+32,384);w16(elf+40,52);
 w16(elf+42,32);w16(elf+44,1);w16(elf+46,40);w16(elf+48,3);w16(elf+50,1);
 w32(elf+52,1);w32(elf+56,512);w32(elf+60,BASE);
 w32(elf+68,384);w32(elf+72,384);w32(elf+76,5);
 memcpy(elf+320,"\0.shstrtab\0.rodata.sceModuleInfo\0",34);
 w32(elf+424,1);w32(elf+440,320);w32(elf+444,34);
 w32(elf+464,11);w32(elf+476,BASE+64);w32(elf+484,52);
 uint8_t *m=elf+512;
 w32(m,0x24080001u); /* addiu t0, zero, 1 */
 m[66]=1;m[67]=2;memcpy(m+68,"BootModule",11);
 w32(m+96,BASE+300);w32(m+100,BASE+240);w32(m+104,BASE+240);
 w32(m+108,BASE+128);w32(m+112,BASE+148);
 w32(m+128,BASE+280);m[136]=5;w16(m+138,2);
 w32(m+140,BASE+196);w32(m+144,BASE+208);
 memcpy(m+280,"IoFileMgrForUser",17);
 w32(m+196,0x109f50bcu);w32(m+200,0x6a638d83u);
}
int main(void){
 uint8_t elf[1024],ram[512];PspCpu cpu;PspHle hle;
 PspLoadResult loaded;PspModuleInfo mod;
 fixture(elf);memset(ram,0xcd,sizeof ram);
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE+4);psp_hle_init(&hle);
 assert(psp_boot_plain_elf(elf,sizeof elf,&cpu,&hle,&loaded,&mod)==PSP_BOOT_OK);
 assert(loaded.entry==BASE&&loaded.segments==1&&cpu.pc==BASE);
 assert(!strcmp(mod.name,"BootModule")&&hle.count==2);
 assert(r32(ram+208)==0x03e00008u && r32(ram+216)==0x03e00008u);
 assert(psp_cpu_step(&cpu)==0 && cpu.gpr[8]==1);
 cpu.pc=BASE+208;cpu.next_pc=BASE+212;cpu.gpr[31]=BASE+16;
 assert(psp_hle_step(&hle,&cpu)==0);
 assert(psp_hle_step(&hle,&cpu)==1 && cpu.pc==BASE+16 && cpu.gpr[2]==UINT32_MAX);
 /* An unimplemented import must not result in a runnable CPU or binding. */
 fixture(elf);w32(elf+512+200,0xdeadbeefu);
 memset(ram,0xcd,sizeof ram);psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE+4);
 psp_hle_init(&hle);loaded.entry=123;memset(&mod,0x55,sizeof mod);
 assert(psp_boot_plain_elf(elf,sizeof elf,&cpu,&hle,&loaded,&mod)==PSP_BOOT_IMPORT_FAILED);
 assert(cpu.pc==BASE+4 && hle.count==0 && loaded.entry==0 && mod.module_address==0);
 assert(r32(ram+208)==0 && r32(ram+216)==0); /* loaded, not patched */
 fixture(elf);w32(elf+60,BASE+600);
 assert(psp_boot_plain_elf(elf,sizeof elf,&cpu,&hle,&loaded,&mod)==PSP_BOOT_ELF_FAILED);
 assert(cpu.pc==BASE+4 && hle.count==0 && loaded.entry==0);
 assert(psp_boot_plain_elf(NULL,sizeof elf,&cpu,&hle,&loaded,&mod)==PSP_BOOT_BAD_ARGUMENT);
 puts("PASS: plain ELF load to verified module NID linking and fail-closed boot");return 0;
}
