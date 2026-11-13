#include "psp_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
#define R(rs,rt,rd,sh,fn) (((rs)<<21)|((rt)<<16)|((rd)<<11)|((sh)<<6)|(fn))
#define I(op,rs,rt,imm) (((op)<<26)|((rs)<<21)|((rt)<<16)|((uint16_t)(imm)))
static void wr(uint8_t *p,uint32_t x){for(unsigned j=0;j<4;j++)p[j]=(uint8_t)(x>>(j*8));}
static void execute(PspCpu *c,uint8_t *memory,uint32_t instruction) {
  size_t offset=(size_t)(c->pc-BASE);wr(memory+offset,instruction);assert(psp_cpu_step(c)==0);
}
static void test_memory(void) {
  uint8_t mem[512]={0};PspCpu c;
  psp_cpu_init(&c,mem,sizeof mem,BASE,BASE); c.gpr[2]=BASE+256;c.gpr[8]=0x80ff7f01u;
  execute(&c,mem,I(43,2,8,0));
  execute(&c,mem,I(32,2,9,3));assert(c.gpr[9]==0xffffff80u);
  execute(&c,mem,I(36,2,10,3));assert(c.gpr[10]==0x80u);
  execute(&c,mem,I(33,2,11,2));assert(c.gpr[11]==0xffff80ffu);
  execute(&c,mem,I(37,2,12,2));assert(c.gpr[12]==0x80ffu);
  c.gpr[8]=0x12345678u;
  execute(&c,mem,I(40,2,8,1));assert(mem[257]==0x78);
  execute(&c,mem,I(41,2,8,2));assert(mem[258]==0x78&&mem[259]==0x56);
  execute(&c,mem,I(35,2,9,0));assert(c.gpr[9]==0x56787801u);
  unsigned previous=c.pc;
  wr(mem+(c.pc-BASE),I(37,2,12,1));assert(psp_cpu_step(&c)==-2&&c.pc==previous);
  psp_cpu_init(&c,mem,sizeof mem,BASE,BASE+64);c.gpr[2]=BASE+511;
  wr(mem+64,I(43,2,8,0));assert(psp_cpu_step(&c)==-2);
}
static void test_arithmetic(void) {
  uint8_t mem[512]={0};PspCpu c;psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
  c.gpr[8]=0xffffffffu;c.gpr[9]=2;
  execute(&c,mem,R(8,9,0,0,24));
  execute(&c,mem,R(0,0,10,0,16));assert(c.gpr[10]==0xffffffffu);
  execute(&c,mem,R(0,0,11,0,18));assert(c.gpr[11]==0xfffffffeu);
  execute(&c,mem,R(8,9,0,0,25));
  execute(&c,mem,R(0,0,12,0,16));assert(c.gpr[12]==1);
  execute(&c,mem,R(0,0,13,0,18));assert(c.gpr[13]==0xfffffffeu);
  execute(&c,mem,R(8,9,0,0,26));
  execute(&c,mem,R(0,0,14,0,18));assert(c.gpr[14]==0);
  execute(&c,mem,R(0,0,15,0,16));assert(c.gpr[15]==0xffffffffu);
  execute(&c,mem,R(8,9,0,0,27));
  execute(&c,mem,R(0,0,16,0,18));assert(c.gpr[16]==0x7fffffffu);
  execute(&c,mem,R(0,0,17,0,16));assert(c.gpr[17]==1);
  execute(&c,mem,R(8,9,18,0,39));assert(c.gpr[18]==0);
  execute(&c,mem,R(9,8,19,0,7));assert(c.gpr[19]==0xffffffffu);
  execute(&c,mem,R(9,8,20,0,6));assert(c.gpr[20]==0x3fffffffu);
  execute(&c,mem,R(9,8,21,0,4));assert(c.gpr[21]==0xfffffffcu);
  execute(&c,mem,R(8,0,0,0,17));execute(&c,mem,R(9,0,0,0,19));
  assert(c.hi==0xffffffffu&&c.lo==2);
}
static void test_regimm_branch(void) {
  uint8_t mem[128]={0};PspCpu c;
  psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);c.gpr[8]=0xffffffffu;
  wr(mem,I(1,8,0,2));wr(mem+4,I(9,9,9,1));
  wr(mem+8,I(9,9,9,100));wr(mem+12,I(9,9,9,2));
  for(int j=0;j<3;j++)assert(psp_cpu_step(&c)==0);
  assert(c.gpr[9]==3&&c.pc==BASE+16);
  psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);c.gpr[8]=0;
  wr(mem,I(1,8,17,2));
  assert(psp_cpu_step(&c)==0&&c.gpr[31]==BASE+8);
  assert(psp_cpu_step(&c)==0&&c.pc==BASE+12);
  psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
  wr(mem,I(1,8,4,2));assert(psp_cpu_step(&c)==-1);
}
static void test_cop1_transfer(void) {
 uint8_t mem[32]={0};
 PspCpu c;
 psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
 c.gpr[4]=0x7fc12345u;
 c.gpr[5]=BASE+24;
 wr(mem, (17u<<26)|(4u<<21)|(4u<<16)|(12u<<11));
 wr(mem+4,I(57,5,12,0));
 wr(mem+8,I(49,5,13,0));
 wr(mem+12,(17u<<26)|(6u<<16)|(13u<<11));
 for(int i=0;i<4;i++) assert(psp_cpu_step(&c)==0);
 assert(c.gpr[6]==0x7fc12345u);
 wr(mem+16,(17u<<26)|(16u<<21)|(13u<<11)|(14u<<6)|6u);
 assert(psp_cpu_step(&c)==0 && c.fpr[14]==0x7fc12345u);
 wr(mem+20,I(49,5,13,1));
 assert(psp_cpu_step(&c)==-2 && c.fpr[13]==0x7fc12345u);
}
static void test_ll_sc(void) {
 uint8_t mem[48]={0};
 PspCpu c;
 psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
 c.gpr[4]=BASE+40;
 wr(mem,I(48,4,5,0));
 wr(mem+4,I(9,5,5,1));
 wr(mem+8,I(56,4,5,0));
 wr(mem+12,I(56,4,5,0));
 wr(mem+16,I(48,4,5,0));
 wr(mem+20,I(43,4,0,0));
 wr(mem+24,I(56,4,5,0));
 wr(mem+28,I(48,4,5,1));
 wr(mem+40,7);
 for(int i=0;i<3;i++) assert(psp_cpu_step(&c)==0);
 assert(c.gpr[5]==1 && mem[40]==8);
 assert(psp_cpu_step(&c)==0 && c.gpr[5]==0 && mem[40]==8);
 for(int i=0;i<3;i++) assert(psp_cpu_step(&c)==0);
 assert(c.gpr[5]==0 && mem[40]==0);
 assert(psp_cpu_step(&c)==-2);
}
 static void test_fcr31(void) {
 uint8_t mem[8]={0}; PspCpu c;
 psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
 c.gpr[4]=0xffffffffu;
 wr(mem,(17u<<26)|(6u<<21)|(4u<<16)|(31u<<11));
 wr(mem+4,(17u<<26)|(2u<<21)|(5u<<16)|(31u<<11));
 assert(psp_cpu_step(&c)==0 && psp_cpu_step(&c)==0);
 assert(c.gpr[5]==0x0181ffffu);
}
 static void test_conditional_move(void) {
 uint8_t mem[16]={0}; PspCpu c;
 psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
 c.gpr[4]=42;c.gpr[5]=1;
 execute(&c,mem,R(4,5,6,0,10)); assert(c.gpr[6]==0);
 execute(&c,mem,R(4,5,6,0,11)); assert(c.gpr[6]==42);
 execute(&c,mem,R(0,0,6,0,11)); assert(c.gpr[6]==42);
 execute(&c,mem,R(0,0,6,0,10)); assert(c.gpr[6]==0);
}
int main(void){test_conditional_move();test_fcr31();test_ll_sc();test_cop1_transfer();test_memory();test_arithmetic();test_regimm_branch();puts("PASS: expanded MIPS integer, memory, branch and HI/LO instruction tests");return 0;}
