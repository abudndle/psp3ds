#include "psp_boot.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
static void w16(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t r32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void fixture(uint8_t elf[1024]){
 memset(elf,0,1024);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+16,2);w16(elf+18,8);w32(elf+20,1);w32(elf+24,BASE);
 w32(elf+28,52);w32(elf+32,384);w16(elf+40,52);
 w16(elf+42,32);w16(elf+44,3);w16(elf+46,40);
 w16(elf+48,3);w16(elf+50,1);
 /* Loaded code and module metadata: identity mapping. */
 w32(elf+52,1);w32(elf+56,512);w32(elf+60,BASE);
 w32(elf+68,384);w32(elf+72,384);w32(elf+76,5);
 /* A moved data section whose relocation delta is +0x1000. */
 w32(elf+84,1);w32(elf+88,896);w32(elf+92,BASE+400-0x1000);
 w32(elf+100,32);w32(elf+104,32);w32(elf+108,6);
 /* PSP uncompressed relocation stream targets word 16 in first segment. */
 w32(elf+116,0x700000a0);w32(elf+120,992);
 w32(elf+132,8);w32(elf+136,8);
 w32(elf+992,16);w32(elf+996,(1u<<16)|2u);
 memcpy(elf+320,"\0.shstrtab\0.rodata.sceModuleInfo\0",34);
 w32(elf+424,1);w32(elf+440,320);w32(elf+444,34);
 w32(elf+464,11);w32(elf+476,BASE+64);w32(elf+484,52);
 uint8_t *m=elf+512;
 w32(m,0x24080001u);w32(m+16,0x20); /* one relocation pointer */
 m[66]=1;m[67]=2;memcpy(m+68,"BootModule",11);
 w32(m+96,BASE+300);w32(m+100,BASE+240);w32(m+104,BASE+240);
 w32(m+108,BASE+128);w32(m+112,BASE+148);
 w32(m+128,BASE+280);m[136]=5;w16(m+138,2);
 w32(m+140,BASE+196);w32(m+144,BASE+208);
 memcpy(m+280,"IoFileMgrForUser",17);
 w32(m+196,0x109f50bcu);w32(m+200,0x6a638d83u);
}
static void mapped(uint8_t ram[512],const uint8_t elf[1024],PspCpu *cpu,PspHle *hle){
 memset(ram,0x5a,512);
 memcpy(ram,elf+512,384);
 memcpy(ram+400,elf+896,32);
 psp_cpu_init(cpu,ram,512,BASE,BASE+4);
 psp_hle_init(hle);
}
int main(void){
 uint8_t elf[1024],ram[512],original[512];PspCpu cpu;
 PspHle hle;PspModuleInfo module;size_t streams;
 PspRelocSegment segments[3]={
   {BASE,BASE,384},
   {BASE+400-0x1000,BASE+400,32},
   {0,0,0}
 };
 fixture(elf);mapped(ram,elf,&cpu,&hle);
 assert(psp_boot_mapped_relocated_elf(
    elf,sizeof elf,&cpu,&hle,segments,3,&module,&streams)==PSP_BOOT_OK);
 assert(streams==1 && cpu.pc==BASE && cpu.next_pc==BASE+4);
 assert(r32(ram+16)==0x1020 && hle.count==2);
 assert(!strcmp(module.name,"BootModule") && module.module_address==BASE+64);
 assert(r32(ram+208)==0x03e00008u && r32(ram+216)==0x03e00008u);
 assert(psp_cpu_step(&cpu)==0 && cpu.gpr[8]==1);
 /* Relocation stream failure must not change ANY CPU, HLE or RAM state. */
 fixture(elf);w32(elf+992,383);mapped(ram,elf,&cpu,&hle);
 assert(psp_hle_bind(&hle,999,PSP_HLE_SYSTEM_TIME_LOW)==0);
 memcpy(original,ram,sizeof ram);PspCpu saved=cpu;PspHle hlesaved=hle;
 memset(&module,0x7f,sizeof module);streams=123;
 assert(psp_boot_mapped_relocated_elf(
    elf,sizeof elf,&cpu,&hle,segments,3,&module,&streams)==PSP_BOOT_RELOCATION_FAILED);
 assert(memcmp(ram,original,sizeof ram)==0 && memcmp(&cpu,&saved,sizeof cpu)==0);
 assert(memcmp(&hle,&hlesaved,sizeof hle)==0);
 assert(module.module_address==0 && streams==0);
 /* Failure after successful relocation must roll back relocation and
  * guest stub modifications, and preserve existing HLE bindings. */
 fixture(elf);w32(elf+512+200,0xdeadbeef);
 mapped(ram,elf,&cpu,&hle);memcpy(original,ram,sizeof ram);saved=cpu;hlesaved=hle;
 streams=99;
 assert(psp_boot_mapped_relocated_elf(
    elf,sizeof elf,&cpu,&hle,segments,3,&module,&streams)==PSP_BOOT_IMPORT_FAILED);
 assert(memcmp(ram,original,sizeof ram)==0 && memcmp(&cpu,&saved,sizeof cpu)==0);
 assert(memcmp(&hle,&hlesaved,sizeof hle)==0 && streams==0);
 /* Bad pre-mapped entry and inconsistent segment metadata fail early. */
 fixture(elf);mapped(ram,elf,&cpu,&hle);memcpy(original,ram,sizeof ram);
 w32(elf+24,BASE+382);
 assert(psp_boot_mapped_relocated_elf(
    elf,sizeof elf,&cpu,&hle,segments,3,&module,&streams)==PSP_BOOT_ELF_FAILED);
 assert(memcmp(ram,original,sizeof ram)==0);
 fixture(elf);PspRelocSegment invalid[3];memcpy(invalid,segments,sizeof segments);
 invalid[0].mem_size=383;
 assert(psp_boot_mapped_relocated_elf(
    elf,sizeof elf,&cpu,&hle,invalid,3,&module,&streams)==PSP_BOOT_ELF_FAILED);
 assert(memcmp(ram,original,sizeof ram)==0);
 assert(psp_boot_mapped_relocated_elf(
    NULL,sizeof elf,&cpu,&hle,segments,3,&module,&streams)==PSP_BOOT_BAD_ARGUMENT);
 puts("PASS: relocated ELF boot, MIPS execution, module imports and whole-boot rollback");
 return 0;
}
