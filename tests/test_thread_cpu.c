#include "psp_thread_cpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE 0x08800000u
static void write32(uint8_t *p,uint32_t w) {
 for (unsigned i=0;i<4;i++) p[i]=(uint8_t)(w>>(i*8));
}
int main(void) {
 uint8_t ram[128]={0};
 PspCpu live;
 PspHle hle;
 PspThreads tasks;
 PspThreadCpu runtime;
 psp_cpu_init(&live,ram,sizeof ram,BASE,BASE);
 psp_hle_init(&hle);
 psp_threads_init(&tasks);
 /* A increments t0 and branches across a delay-slot instruction.
  * B increments t1 independently. No guest kernel services required. */
 write32(ram+0,0x25080001u);
 write32(ram+4,0x10000001u);
 write32(ram+8,0x25080002u);
 write32(ram+12,0x25080004u);
 for(unsigned i=0;i<4;i++) write32(ram+32+4*i,0x25290003u);
 write32(ram+64,0x25080007u);
 assert(psp_thread_cpu_init(&runtime,&tasks,&live,&hle)==0);
 assert(psp_thread_cpu_spawn(&runtime,5,BASE+1,BASE+128)==-1);
 assert(psp_thread_cpu_spawn(&runtime,5,BASE,BASE+129)==-1);
 int32_t a=psp_thread_cpu_spawn(&runtime,5,BASE,BASE+128);
 int32_t b=psp_thread_cpu_spawn(&runtime,5,BASE+32,BASE+124);
 assert(a>0 && b>0);
 assert(psp_thread_cpu_step(&runtime)==0); /* A: addiu */
 assert(live.gpr[8]==1 && live.pc==BASE+4);
 assert(psp_thread_cpu_step(&runtime)==0); /* B: addiu */
 assert(live.gpr[9]==3 && live.gpr[8]==0);
 assert(psp_thread_cpu_step(&runtime)==0); /* A: branch */
 assert(live.pc==BASE+8 && live.next_pc==BASE+12 && live.delay_slot);
 assert(psp_thread_cpu_step(&runtime)==0); /* B: addiu */
 assert(psp_thread_cpu_step(&runtime)==0); /* A: branch delay-slot */
 assert(live.gpr[8]==3 && live.pc==BASE+12 && !live.delay_slot);
 assert(psp_thread_cpu_step(&runtime)==0); /* B */
 assert(live.gpr[9]==9 && live.gpr[8]==0);
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)a)==0);
 assert(live.pc==BASE+12 && live.gpr[8]==3 && live.gpr[9]==0);
 assert(psp_thread_cpu_step(&runtime)==0); /* B turn */
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)a)==0);
 assert(psp_threads_wait(&tasks,(uint32_t)a,17,100)==0);
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)a)==-1);
 assert(psp_thread_cpu_step(&runtime)==0); /* B runs instead */
 assert(live.gpr[9]==12);
 assert(psp_threads_signal(&tasks,17)==a);
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)a)==0);
 assert(live.pc==BASE+16 && live.gpr[8]==7);
 assert(psp_thread_cpu_step(&runtime)==0); /* B active */
 assert(psp_threads_stop(&tasks,(uint32_t)b)==0);
 int32_t c=psp_thread_cpu_spawn(&runtime,5,BASE+64,BASE+120);
 assert(c>b);
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)c)==0);
 assert(live.gpr[8]==0 && live.gpr[9]==0 && live.pc==BASE+64);
 assert(psp_thread_cpu_step(&runtime)==0); /* A or C, valid */
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)c)==0);
 assert(live.pc==BASE+64 || live.pc==BASE+68);
 assert(psp_thread_cpu_flush(&runtime)==0);
 assert(psp_threads_stop(&tasks,(uint32_t)a)==0);
 assert(psp_threads_stop(&tasks,(uint32_t)c)==0);
 assert(psp_thread_cpu_step(&runtime)==2);
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)b)==-1);
 assert(psp_thread_cpu_switch(NULL,(uint32_t)a)==-1);
 uint8_t guest[1024]={0};
 psp_cpu_init(&live,guest,sizeof guest,BASE,BASE);
 psp_threads_init(&tasks);
 assert(psp_thread_cpu_init(&runtime,&tasks,&live,&hle)==0);
 assert(psp_thread_cpu_set_stack_arena(&runtime,BASE+512,BASE+1024)==0);
 int32_t dormant=psp_thread_cpu_create(&runtime,32,BASE,512);
 assert(dormant>0 && psp_thread_cpu_step(&runtime)==2);
 assert(psp_thread_cpu_create(&runtime,32,BASE,256)<0);
 guest[32]=0x7b;
 assert(psp_thread_cpu_start(&runtime,(uint32_t)dormant,1,BASE+32)==0);
 assert(psp_thread_cpu_start(&runtime,(uint32_t)dormant,0,0)<0);
 assert(psp_thread_cpu_switch(&runtime,(uint32_t)dormant)==0);
 assert(live.gpr[4]==1 && guest[live.gpr[5]-BASE]==0x7b);
 assert(live.gpr[26]==BASE+768 && live.gpr[29]<live.gpr[26]);
 assert(guest[768+0xc0]==(uint8_t)dormant && guest[768+0xfc]==0xff);
 write32(guest,0x03e00008u); /* jr ra */
 write32(guest+4,0x2402002au); /* return value in the delay slot */
 assert(psp_thread_cpu_step(&runtime)==0);
 assert(psp_thread_cpu_step(&runtime)==0 && live.gpr[2]==42);
 assert(psp_threads_get(&tasks,(uint32_t)dormant)->state==PSP_TASK_STOPPED);
 assert(psp_thread_cpu_step(&runtime)==2);
 puts("PASS: cooperative CPU registers, branch-delay state, runnable gating and retired-slot reuse");
 return 0;
}
