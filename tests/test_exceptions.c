#include "psp_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
#define I(op,rs,rt,imm) (((uint32_t)(op)<<26)|((uint32_t)(rs)<<21)|((uint32_t)(rt)<<16)|(uint16_t)(imm))
#define R(rs,rt,rd,sh,fn) (((uint32_t)(rs)<<21)|((uint32_t)(rt)<<16)|((uint32_t)(rd)<<11)|((uint32_t)(sh)<<6)|(fn))
static void write32(uint8_t *m,unsigned offset,uint32_t v) { for(unsigned i=0;i<4;i++)m[offset+i]=(uint8_t)(v>>(8*i)); }
static void reset(PspCpu *c,uint8_t *m) { memset(m,0,128);psp_cpu_init(c,m,128,BASE,BASE); }
int main(void) {
 uint8_t m[128];PspCpu c;
 reset(&c,m);write32(m,0,(0xabcdeu<<6)|12u);
 assert(psp_cpu_step(&c)==-3 && c.last_trap_code==0xabcdeu);
 assert(((c.cp0_cause>>2)&31u)==8 && c.cp0_epc==BASE && (c.cp0_status&2));
 assert(c.pc==BASE && c.fault==-3);
 reset(&c,m);write32(m,0,(42u<<6)|13u);
 assert(psp_cpu_step(&c)==-3 && ((c.cp0_cause>>2)&31u)==9 && c.last_trap_code==42);
 reset(&c,m);c.gpr[1]=0x7fffffffu;c.gpr[2]=1;c.gpr[3]=0xdeadbeefu;
 write32(m,0,R(1,2,3,0,32));
 assert(psp_cpu_step(&c)==-3 && ((c.cp0_cause>>2)&31u)==12 && c.gpr[3]==0xdeadbeefu);
 reset(&c,m);c.gpr[1]=0x80000000u;c.gpr[2]=1;c.gpr[3]=0xdeadbeefu;
 write32(m,0,R(1,2,3,0,34));
 assert(psp_cpu_step(&c)==-3 && c.gpr[3]==0xdeadbeefu);
 reset(&c,m);c.gpr[1]=0x7fffffffu;c.gpr[3]=99;
 write32(m,0,I(8,1,3,1));
 assert(psp_cpu_step(&c)==-3 && c.gpr[3]==99);
 reset(&c,m);c.gpr[1]=1;c.gpr[2]=1;
 write32(m,0,I(4,1,2,2));write32(m,4,12);
 assert(psp_cpu_step(&c)==0 && c.pc==BASE+4 && c.delay_slot);
 assert(psp_cpu_step(&c)==-3 && c.cp0_epc==BASE && (c.cp0_cause&0x80000000u));
 reset(&c,m);c.gpr[1]=1;c.gpr[2]=2;
 write32(m,0,I(20,1,2,2));write32(m,4,12);write32(m,8,12);
 assert(psp_cpu_step(&c)==0 && c.pc==BASE+8 && !c.delay_slot);
 assert(psp_cpu_step(&c)==-3 && c.cp0_epc==BASE+8 && !(c.cp0_cause&0x80000000u));
 reset(&c,m);c.cp0_status=0x1234;
 write32(m,0,I(16,0,4,12<<11));
 assert(psp_cpu_step(&c)==0 && c.gpr[4]==0x1234);
 write32(m,4,I(16,4,5,12<<11));c.gpr[5]=0x200;
 assert(psp_cpu_step(&c)==0 && c.cp0_status==0x200);
 write32(m,8,I(16,0,6,12<<11));
 assert(psp_cpu_step(&c)==0 && c.gpr[6]==0x200);
 puts("PASS: syscall/break, overflow, COP0 subset, exception PC and delay-slot BD flag");
 return 0;
}
