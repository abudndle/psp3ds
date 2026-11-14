#include "psp_module.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void w16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void w32(uint8_t *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;}
static void fixture(uint8_t *elf,uint8_t *ram) {
 memset(elf,0,512);memset(ram,0xa5,512);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 /* These existing fixtures represent fixed-address ELF32 ET_EXEC. */
 w16(elf+16,2);w16(elf+18,8);w32(elf+32,256);w16(elf+46,40);w16(elf+48,3);w16(elf+50,1);
 memcpy(elf+160,"\0.shstrtab\0.rodata.sceModuleInfo\0",34);
 w32(elf+296,1);w32(elf+312,160);w32(elf+316,34);
 w32(elf+336,11);w32(elf+348,BASE+64);w32(elf+356,52);
 memset(ram+64,0,52);ram[66]=1;ram[67]=2;memcpy(ram+68,"TestModule",11);
 w32(ram+96,BASE+300);w32(ram+100,BASE+240);w32(ram+104,BASE+240);
 w32(ram+108,BASE+128);w32(ram+112,BASE+148);
 w32(ram+128,BASE+280);ram[136]=5;ram[137]=0;w16(ram+138,2);
 w32(ram+140,BASE+196);w32(ram+144,BASE+208);
 memcpy(ram+280,"IoFileMgrForUser",17);
 w32(ram+196,0x109f50bcu);w32(ram+200,0x6a638d83u);
}
int main(void) {
 uint8_t elf[512],ram[512];PspCpu cpu;PspHle hle;PspModuleInfo info;
 fixture(elf,ram);psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);psp_hle_init(&hle);
 assert(psp_module_link_known(elf,sizeof elf,&cpu,&hle,&info)==0);
 assert(!strcmp(info.name,"TestModule") && hle.count==2);
 assert(rd32(ram+208)==0x03e00008 && rd32(ram+216)==0x03e00008);
 assert(hle.entries[0].service==PSP_HLE_IO_OPEN);
 assert(hle.entries[1].service==PSP_HLE_IO_READ);
 assert(hle.entries[0].syscall_id!=hle.entries[1].syscall_id);
 /* Whole batch must fail without changing stubs or HLE table. */
 fixture(elf,ram);w32(ram+200,0xdeadbeef);psp_hle_init(&hle);
 memset(&info,0xa5,sizeof info);
 assert(psp_module_link_known(elf,sizeof elf,&cpu,&hle,&info)==-4);
 assert(hle.count==0 && rd32(ram+208)==0xa5a5a5a5u && rd32(ram+216)==0xa5a5a5a5u);
 assert(info.module_address==0);
 fixture(elf,ram);ram[136]=3;
 assert(psp_module_link_known(elf,sizeof elf,&cpu,&hle,&info)!=0);
 assert(hle.count==0 && rd32(ram+208)==0xa5a5a5a5u);
 assert(psp_module_link_known(elf,30,&cpu,&hle,&info)!=0);
 assert(hle.count==0);
 puts("PASS: module metadata to atomic known-NID import linking");return 0;
}
