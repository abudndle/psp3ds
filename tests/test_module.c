#include "psp_module.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void fixture(uint8_t *elf,uint8_t *ram) {
 memset(elf,0,512);memset(ram,0,512);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 w16(elf+18,8);w32(elf+32,256);w16(elf+46,40);w16(elf+48,3);w16(elf+50,1);
 memcpy(elf+160,"\0.shstrtab\0.rodata.sceModuleInfo\0",34);
 w32(elf+256+40,1);w32(elf+256+40+16,160);w32(elf+256+40+20,34);
 w32(elf+256+80,11);w32(elf+256+80+12,BASE+64);w32(elf+256+80+20,52);
 w16(ram+64,0);ram[66]=1;ram[67]=2;
 memcpy(ram+68,"TestModule",11);
 w32(ram+64+32,BASE+300);
 w32(ram+64+36,BASE+240);w32(ram+64+40,BASE+240);
 w32(ram+64+44,BASE+128);w32(ram+64+48,BASE+148);
 w32(ram+128,BASE+180);ram[136]=5;w16(ram+138,1);
 w32(ram+140,BASE+196);w32(ram+144,BASE+208);
 memcpy(ram+180,"UtilsForUser",13);w32(ram+196,0xabcdef12);
}
int main(void) {
 uint8_t elf[512],ram[512];PspCpu cpu;PspModuleInfo info;PspPrxImports imports;
 fixture(elf,ram);psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==0);
 assert(strcmp(info.name,"TestModule")==0 && info.module_address==BASE+64);
 assert(info.version_major==2 && info.gp==BASE+300);
 assert(info.import_top==BASE+128 && info.import_end==BASE+148);
 assert(psp_module_imports(&cpu,&info,&imports)==0 && imports.count==1);
 assert(imports.imports[0].nid==0xabcdef12 && imports.imports[0].stub_address==BASE+208);
 fixture(elf,ram);w32(elf+256+80,0);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==-3);
 fixture(elf,ram);w32(elf+256+80+20,40);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==-2);
 fixture(elf,ram);w32(ram+64+48,BASE+600);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==-2);
 fixture(elf,ram);ram[95]=1;
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==-2);
 fixture(elf,ram);w16(elf+50,4);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==-2);
 fixture(elf,ram);w32(elf+256+40+20,5);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==-2);
 fixture(elf,ram);w32(elf+256+80+12,BASE+500);
 assert(psp_module_discover(elf,sizeof elf,&cpu,&info)==-2);
 puts("PASS: PSP module-info discovery, import handoff and malformed ELF/guest boundaries");
 return 0;
}
