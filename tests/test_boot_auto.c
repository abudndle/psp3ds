#include "psp_boot.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
static void w16(uint8_t *p,unsigned x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void w32(uint8_t *p,uint32_t x){for(unsigned j=0;j<4;j++)p[j]=(uint8_t)(x>>(8*j));}
static uint32_t rd32(const uint8_t *p){
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void fixture(uint8_t e[1024]) {
 memset(e,0,1024);
 memcpy(e,"\177ELF",4);e[4]=1;e[5]=1;e[6]=1;
 w16(e+16,2);w16(e+18,8);w32(e+20,1);w32(e+24,BASE);
 w32(e+28,52);w32(e+32,384);w16(e+40,52);
 w16(e+42,32);w16(e+44,3);w16(e+46,40);
 w16(e+48,3);w16(e+50,1);
 /* Executable PT_LOAD stays at original address; module metadata resides here. */
 w32(e+52,1);w32(e+56,512);w32(e+60,BASE);
 w32(e+68,384);w32(e+72,384);w32(e+76,5);
 /* Data PT_LOAD needs placement; p_paddr is a tested location hint. */
 w32(e+84,1);w32(e+88,896);w32(e+92,BASE+400-0x1000);
 w32(e+96,BASE+400);w32(e+100,32);w32(e+104,48);w32(e+108,6);
 /* R_MIPS_32 at offset 16 of segment 0: source segment 1. */
 w32(e+116,0x700000a0);w32(e+120,992);
 w32(e+132,8);w32(e+136,8);
 w32(e+992,16);w32(e+996,(1u<<16)|2u);
 memcpy(e+320,"\0.shstrtab\0.rodata.sceModuleInfo\0",34);
 w32(e+424,1);w32(e+440,320);w32(e+444,34);
 w32(e+464,11);w32(e+476,BASE+64);w32(e+484,52);
 uint8_t *m=e+512;
 w32(m,0x24080001u);w32(m+16,0x20);
 m[66]=1;m[67]=2;memcpy(m+68,"BootModule",11);
 w32(m+96,BASE+300);w32(m+100,BASE+240);w32(m+104,BASE+240);
 w32(m+108,BASE+128);w32(m+112,BASE+148);
 w32(m+128,BASE+280);m[136]=5;w16(m+138,2);
 w32(m+140,BASE+196);w32(m+144,BASE+208);
 memcpy(m+280,"IoFileMgrForUser",17);
 w32(m+196,0x109f50bcu);w32(m+200,0x6a638d83u);
 e[896]=0x31;e[897]=0x42;
}
static void setup(uint8_t ram[512],PspCpu *cpu,PspHle *hle) {
 memset(ram,0xa5,512);
 psp_cpu_init(cpu,ram,512,BASE,BASE+4);
 psp_hle_init(hle);
}
static void unchanged(const uint8_t before[512],const uint8_t ram[512],
                      const PspCpu *cpu,const PspCpu *old,
                      const PspHle *hle,const PspHle *hle_old,
                      const PspModuleInfo *mod,size_t streams){
 assert(memcmp(ram,before,512)==0);
 assert(memcmp(cpu,old,sizeof(*cpu))==0);
 assert(memcmp(hle,hle_old,sizeof(*hle))==0);
 assert(mod->module_address==0 && streams==0);
}
int main(void){
 uint8_t elf[1024],ram[512],before[512];
 PspCpu cpu,saved;PspHle hle,previous;
 PspModuleInfo mod;size_t streams;
 fixture(elf);setup(ram,&cpu,&hle);
 assert(psp_boot_auto_mapped_elf(elf,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_OK);
 assert(cpu.pc==BASE && streams==1 && hle.count==2);
 assert(!strcmp(mod.name,"BootModule"));
 assert(rd32(ram+16)==0x1020); /* moved data's relocation delta +0x1000 */
 assert(ram[400]==0x31 && ram[401]==0x42);
 for(int i=432;i<448;i++) assert(ram[i]==0); /* BSS zero-filled */
 assert(rd32(ram+208)==0x03e00008u && rd32(ram+216)==0x03e00008u);
 assert(psp_cpu_step(&cpu)==0 && cpu.gpr[8]==1);
 /* Without a p_paddr hint, select the first aligned gap after code. */
 fixture(elf);w32(elf+96,0);setup(ram,&cpu,&hle);
 assert(psp_boot_auto_mapped_elf(elf,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_OK);
 assert(rd32(ram+16)==0x1010 && ram[384]==0x31 && ram[385]==0x42);
 for(int i=416;i<432;i++) assert(ram[i]==0);
 /* Explicit placement overlaps are rejected before copying bytes. */
 fixture(elf);w32(elf+96,BASE+128);setup(ram,&cpu,&hle);
 memcpy(before,ram,512);saved=cpu;previous=hle;streams=55;
 assert(psp_boot_auto_mapped_elf(elf,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_MAPPING_FAILED);
 unchanged(before,ram,&cpu,&saved,&hle,&previous,&mod,streams);
 /* First segment must stay linked in this restricted module subset. */
 fixture(elf);w32(elf+64,BASE+32);setup(ram,&cpu,&hle);
 memcpy(before,ram,512);saved=cpu;previous=hle;
 assert(psp_boot_auto_mapped_elf(elf,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_MAPPING_FAILED);
 unchanged(before,ram,&cpu,&saved,&hle,&previous,&mod,streams);
 /* Import linkage failure after segment copying and successful relocation
  * must restore old RAM and HLE tables. */
 fixture(elf);w32(elf+512+200,0xdeadbeefu);
 setup(ram,&cpu,&hle);assert(psp_hle_bind(&hle,700,PSP_HLE_SYSTEM_TIME_LOW)==0);
 memcpy(before,ram,512);saved=cpu;previous=hle;
 assert(psp_boot_auto_mapped_elf(elf,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_IMPORT_FAILED);
 unchanged(before,ram,&cpu,&saved,&hle,&previous,&mod,streams);
 /* Relocation failure after successful segment copying also rolls back. */
 fixture(elf);w32(elf+992,383);setup(ram,&cpu,&hle);
 memcpy(before,ram,512);saved=cpu;previous=hle;
 assert(psp_boot_auto_mapped_elf(elf,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_RELOCATION_FAILED);
 unchanged(before,ram,&cpu,&saved,&hle,&previous,&mod,streams);
 /* Missing/truncated section source is caught before touching RAM. */
 fixture(elf);w32(elf+100,500);setup(ram,&cpu,&hle);
 memcpy(before,ram,512);saved=cpu;previous=hle;
 assert(psp_boot_auto_mapped_elf(elf,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_MAPPING_FAILED);
 unchanged(before,ram,&cpu,&saved,&hle,&previous,&mod,streams);
 assert(psp_boot_auto_mapped_elf(NULL,sizeof elf,&cpu,&hle,&mod,&streams)==PSP_BOOT_BAD_ARGUMENT);
 puts("PASS: automatic ELF PT_LOAD mapping, BSS, relocation, imports, MIPS execution, rollback");
 return 0;
}
