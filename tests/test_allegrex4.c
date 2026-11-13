#include "psp_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
#define I(op,rs,rt,imm) (((uint32_t)(op)<<26)|((uint32_t)(rs)<<21)|((uint32_t)(rt)<<16)|(uint16_t)(imm))
static void wr(uint8_t *p,uint32_t w){for(unsigned j=0;j<4;j++)p[j]=(uint8_t)(w>>(8*j));}
static void run(PspCpu *c, uint8_t *mem, uint32_t inst){
  assert(c->pc>=BASE && c->pc <BASE+128);wr(mem+c->pc-BASE,inst);assert(psp_cpu_step(c)==0);
}
static void unaligned(void){
  uint8_t mem[256]={0}; PspCpu c;
  for(unsigned k=0;k<4;k++) {
    memset(mem,0,sizeof mem);psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
    c.gpr[2]=BASE+128+k;c.gpr[3]=0xaabbccddu;
    mem[128]=0x11; mem[129]=0x22;mem[130]=0x33;mem[131]=0x44;
    run(&c,mem,I(34,2,3,0));
    const uint32_t lwl[4]={0x11bbccddu,0x2211ccddu,0x332211ddu,0x44332211u};
    assert(c.gpr[3]==lwl[k]);
    c.gpr[3]=0xaabbccddu;run(&c,mem,I(38,2,3,0));
    const uint32_t lwr[4]={0x44332211u,0xaa443322u,0xaabb4433u,0xaabbcc44u};
    assert(c.gpr[3]==lwr[k]);
    c.gpr[3]=0xaabbccddu;run(&c,mem,I(42,2,3,0));
    const uint8_t swl[4][4]={{0xaa,0x22,0x33,0x44},{0xbb,0xaa,0x33,0x44},{0xcc,0xbb,0xaa,0x44},{0xdd,0xcc,0xbb,0xaa}};
    assert(memcmp(mem+128,swl[k],4)==0);
    mem[128]=0x11;mem[129]=0x22;mem[130]=0x33;mem[131]=0x44;
    run(&c,mem,I(46,2,3,0));
    const uint8_t swr[4][4]={{0xdd,0xcc,0xbb,0xaa},{0x11,0xdd,0xcc,0xbb},{0x11,0x22,0xdd,0xcc},{0x11,0x22,0x33,0xdd}};
    assert(memcmp(mem+128,swr[k],4)==0);
  }
  memset(mem,0,sizeof mem);psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
  c.gpr[2]=BASE+129;mem[129]=0x12;mem[130]=0x34;mem[131]=0x56;mem[132]=0x78;
  run(&c,mem,I(34,2,3,3));run(&c,mem,I(38,2,3,0));assert(c.gpr[3]==0x78563412u);
  c.gpr[3]=0xa1b2c3d4u;run(&c,mem,I(42,2,3,3));run(&c,mem,I(46,2,3,0));
  assert(mem[129]==0xd4 && mem[130]==0xc3 && mem[131]==0xb2 && mem[132]==0xa1);
  memset(mem,0,sizeof mem);psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
  c.gpr[2]=BASE+255;c.gpr[3]=0x11223344u;
  run(&c,mem,I(46,2,3,0));assert(mem[255]==0x44);
  psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);c.gpr[2]=BASE+255;c.gpr[3]=0xaabbccddu;
  wr(mem,I(42,2,3,0));assert(psp_cpu_step(&c)==0);
  psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);c.gpr[2]=BASE-1;c.gpr[3]=0xaabbccddu;
  wr(mem,I(38,2,3,0));assert(psp_cpu_step(&c)==-2);
}
static void branch_likely(void){
  uint8_t mem[128]={0};PspCpu c;
  for(unsigned op=20;op<=23;op++){
    memset(mem,0,sizeof mem);psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
    c.gpr[2]=(op==20)?1:(op==21)?0:(op==22)?1:0;c.gpr[3]=0;
    wr(mem,I(op,2,3,3));wr(mem+4,I(9,4,4,100));wr(mem+8,I(9,4,4,1));
    assert(psp_cpu_step(&c)==0&&c.pc==BASE+8);
    assert(psp_cpu_step(&c)==0&&c.gpr[4]==1);
  }
  memset(mem,0,sizeof mem);psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
  c.gpr[2]=1;c.gpr[3]=1;
  wr(mem,I(20,2,3,2));wr(mem+4,I(9,4,4,5));wr(mem+8,I(9,4,4,100));wr(mem+12,I(9,4,4,7));
  assert(psp_cpu_step(&c)==0 && c.pc==BASE+4);
  assert(psp_cpu_step(&c)==0 && c.pc==BASE+12 && c.gpr[4]==5);
  assert(psp_cpu_step(&c)==0 && c.gpr[4]==12);
  for(unsigned variant=0;variant<4;variant++) {
    unsigned rt=(unsigned[]){2,3,18,19}[variant];
    memset(mem,0,sizeof mem);psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);
    c.gpr[2]=(rt==2||rt==18)?1:0xffffffffu; c.gpr[31]=0x55555555u;
    wr(mem,I(1,2,rt,2));wr(mem+4,I(9,5,5,1));
    assert(psp_cpu_step(&c)==0 && c.pc==BASE+8 && c.gpr[31]==0x55555555u);
    psp_cpu_init(&c,mem,sizeof mem,BASE,BASE);c.gpr[2]=(rt==2||rt==18)?0xffffffffu:0;
    assert(psp_cpu_step(&c)==0 && c.pc==BASE+4);
    if(rt==18 || rt==19) assert(c.gpr[31]==BASE+8);
    assert(psp_cpu_step(&c)==0 && c.pc==BASE+12 && c.gpr[5]==1);
  }
}
int main(void){unaligned();branch_likely();puts("PASS: LWL/LWR/SWL/SWR, branch-likely, delay slot annulment, bounds");return 0;}
