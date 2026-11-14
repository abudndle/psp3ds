/* Redistributable synthetic PSP ELF fixture: source-generated, no game assets.
 * This tests loader-to-CPU handoff using the project's EXISTING test CPU,
 * NOT PPSSPP, a PSP kernel, or a commercial PSP game. */
#include "psp_loader.h"
#include "psp_core.h"
#include "psp_hle.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));
}
static void build_fixture(uint8_t *elf,size_t size){
 static const uint32_t code[]={
  0x24080007u, /* addiu t0, zero, 7 */
  0x24090005u, /* addiu t1, zero, 5 */
  0x01095021u, /* addu t2, t0, t1 */
  0x254affffu, /* addiu t2, t2, -1 */
  0x0000004cu, /* syscall 1: bound to synthetic system-time service */
  0x0000000cu  /* syscall 0: deliberately unbound */
 };
 memset(elf,0,size);
 memcpy(elf,"\177ELF",4);elf[4]=1;elf[5]=1;elf[6]=1;
 put16(elf+16,2);put16(elf+18,8);put32(elf+20,1);
 put32(elf+24,BASE);put32(elf+28,52);
 put16(elf+40,52);put16(elf+42,32);put16(elf+44,1);
 put32(elf+52,1);put32(elf+56,128);put32(elf+60,BASE);
 put32(elf+68,sizeof code);put32(elf+72,sizeof code);
 put32(elf+76,5);put32(elf+80,4);
 for(unsigned i=0;i<sizeof code/sizeof code[0];i++)
  put32(elf+128+i*4,code[i]);
}
int main(void){
 uint8_t elf[256],ram[4096];PspLoadResult loaded;
 build_fixture(elf,sizeof elf);
 memset(ram,0xcc,sizeof ram);
 assert(psp_elf_load(elf,sizeof elf,ram,sizeof ram,BASE,&loaded)==PSP_LOAD_OK);
 assert(loaded.entry==BASE&&loaded.segments==1);
 PspCpu cpu;psp_cpu_init(&cpu,ram,sizeof ram,BASE,loaded.entry);
 PspHle hle;psp_hle_init(&hle);
 assert(psp_hle_bind(&hle,1,PSP_HLE_SYSTEM_TIME_LOW)==0);
 psp_hle_advance_time(&hle,123456u);
 for(unsigned i=0;i<4;i++)assert(psp_hle_step(&hle,&cpu)==0);
 assert(psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==123456u && cpu.pc==BASE+20);
 assert(cpu.fault==0);
 assert(psp_hle_step(&hle,&cpu)==-3);
 assert(cpu.gpr[8]==7&&cpu.gpr[9]==5&&cpu.gpr[10]==11);
 assert(cpu.pc==BASE+20);
 assert(cpu.cp0_epc==BASE+20);
 assert(((cpu.cp0_cause>>2)&31u)==8u);
 assert(cpu.fault==-3);
 puts("PASS: synthetic PSP ELF entry, bound HLE syscall and unbound trap (NOT PPSSPP/game boot)");
 return 0;
}
