#include "psp_hle.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
#define SYSCALL(code) (((uint32_t)(code)<<6)|UINT32_C(12))
static void wr32(uint8_t *mem,size_t pos,uint32_t word){
 for(unsigned i=0;i<4;i++)mem[pos+i]=(uint8_t)(word>>(8*i));
}
static uint32_t rd32(const uint8_t *mem,size_t pos){
 return (uint32_t)mem[pos]|(uint32_t)mem[pos+1]<<8|
        (uint32_t)mem[pos+2]<<16|(uint32_t)mem[pos+3]<<24;
}
int main(void){
 uint8_t mem[256]={0}; PspCpu cpu; PspHle hle;
 psp_hle_init(&hle);
 assert(psp_hle_bind(NULL,1,PSP_HLE_SYSTEM_TIME_LOW)==-1);
 assert(psp_hle_bind(&hle,0x100000,PSP_HLE_SYSTEM_TIME_LOW)==-1);
 assert(psp_hle_bind(&hle,10,(PspHleService)99)==-1);
 assert(psp_hle_bind(&hle,10,PSP_HLE_SYSTEM_TIME_LOW)==0);
 assert(psp_hle_bind(&hle,10,PSP_HLE_SYSTEM_TIME_LOW)==-1);
 assert(psp_hle_bind(&hle,11,PSP_HLE_SYSTEM_TIME_WIDE)==0);
 assert(psp_hle_bind(&hle,12,PSP_HLE_SYSTEM_TIME)==0);
 for(uint32_t id=13;id<10+PSP_HLE_MAX_BINDINGS;id++)
   assert(psp_hle_bind(&hle,id,PSP_HLE_SYSTEM_TIME_LOW)==0);
 assert(psp_hle_bind(&hle,100,PSP_HLE_SYSTEM_TIME_LOW)==-1);
 psp_hle_init(&hle);
 assert(psp_hle_bind(&hle,10,PSP_HLE_SYSTEM_TIME_LOW)==0);
 assert(psp_hle_bind(&hle,11,PSP_HLE_SYSTEM_TIME_WIDE)==0);
 assert(psp_hle_bind(&hle,12,PSP_HLE_SYSTEM_TIME)==0);
 psp_cpu_init(&cpu,mem,sizeof mem,BASE,BASE);
 psp_hle_advance_time(&hle,UINT64_C(0x100000004));
 wr32(mem,0,SYSCALL(10));
 assert(psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==4 && cpu.pc==BASE+4 && cpu.cp0_status==0 && !cpu.fault);
 wr32(mem,4,SYSCALL(11));
 assert(psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==4 && cpu.gpr[3]==1 && cpu.pc==BASE+8);
 wr32(mem,8,SYSCALL(12));cpu.gpr[4]=BASE+128;
 assert(psp_hle_step(&hle,&cpu)==1);
 assert(cpu.gpr[2]==0 && rd32(mem,128)==4 && rd32(mem,132)==1);
 wr32(mem,12,SYSCALL(12));cpu.gpr[4]=BASE+252;
 assert(psp_hle_step(&hle,&cpu)==1 && cpu.gpr[2]==PSP_HLE_ERR_INVALID_ARGUMENT);
 cpu.gpr[4]=BASE+129;wr32(mem,16,SYSCALL(12));
 assert(psp_hle_step(&hle,&cpu)==1 && cpu.gpr[2]==PSP_HLE_ERR_INVALID_ARGUMENT);
 wr32(mem,20,SYSCALL(9));
 assert(psp_hle_step(&hle,&cpu)==-3 && cpu.pc==BASE+20);
 assert(((cpu.cp0_cause>>2)&31u)==8);
 psp_cpu_init(&cpu,mem,sizeof mem,BASE,BASE+24);wr32(mem,24,13);
 assert(psp_hle_step(&hle,&cpu)==-3 && ((cpu.cp0_cause>>2)&31u)==9);
 psp_cpu_init(&cpu,mem,sizeof mem,BASE,BASE+32);
 wr32(mem,32,0x10000002);wr32(mem,36,SYSCALL(10));
 assert(psp_hle_step(&hle,&cpu)==0 && cpu.pc==BASE+36 && cpu.next_pc==BASE+44);
 assert(psp_hle_step(&hle,&cpu)==1 && cpu.pc==BASE+44 && cpu.next_pc==BASE+48);
 assert(cpu.cp0_status==0 && !(cpu.cp0_cause&0x80000000u));
 puts("PASS: PSP syscall bindings, guest pointers, traps, branch slots");
 return 0;
}
