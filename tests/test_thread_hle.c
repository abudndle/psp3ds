#include "psp_thread_cpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE 0x08800000u
static void word(uint8_t *p,uint32_t v) {
 for (int i=0;i<4;i++) p[i]=(uint8_t)(v>>(i*8));
}
static uint32_t syscall_insn(uint32_t id) {return (id<<6)|12u;}
int main(void) {
 uint8_t ram[128]={0};
 PspCpu cpu;
 PspHle h;
 PspThreads tasks;
 PspThreadCpu rt;
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 psp_hle_init(&h);
 psp_threads_init(&tasks);
 assert(psp_thread_cpu_init(&rt,&tasks,&cpu,&h)==0);
 psp_hle_attach_threads(&h,&rt);
 assert(psp_hle_bind(&h,41,PSP_HLE_THREAD_CREATE)==0);
 assert(psp_hle_bind(&h,42,PSP_HLE_THREAD_WAIT)==0);
 assert(psp_hle_bind(&h,43,PSP_HLE_THREAD_SIGNAL)==0);
 assert(psp_hle_bind(&h,44,PSP_HLE_THREAD_EXIT)==0);
 assert(psp_hle_bind(&h,45,(PspHleService)1000)==-1);
 word(ram,syscall_insn(41));    /* A creates B */
 word(ram+4,syscall_insn(42));  /* A waits */
 word(ram+8,syscall_insn(44));  /* A exits */
 word(ram+32,syscall_insn(43)); /* B signals */
 word(ram+36,syscall_insn(44)); /* B exits */
 int32_t a=psp_thread_cpu_spawn(&rt,5,BASE,BASE+128);
 assert(a>0);
 assert(psp_thread_cpu_switch(&rt,(uint32_t)a)==0);
 cpu.gpr[4]=BASE+32;
 cpu.gpr[5]=BASE+124;
 cpu.gpr[6]=9;
 assert(psp_thread_cpu_step(&rt)==1);
 int32_t b=(int32_t)cpu.gpr[2];
 assert(b>a && psp_threads_get(&tasks,(uint32_t)b)!=NULL);
 assert(cpu.pc==BASE+4);
 cpu.gpr[4]=77;
 cpu.gpr[5]=100;
 assert(psp_thread_cpu_step(&rt)==1);
 assert((int32_t)cpu.gpr[2]==0);
 assert(psp_threads_get(&tasks,(uint32_t)a)->state==PSP_TASK_WAITING);
 assert(psp_thread_cpu_switch(&rt,(uint32_t)a)==-1);
 assert(psp_thread_cpu_switch(&rt,(uint32_t)b)==0);
 cpu.gpr[4]=77;
 assert(psp_thread_cpu_step(&rt)==1);
 assert(cpu.gpr[2]==(uint32_t)a);
 assert(psp_threads_get(&tasks,(uint32_t)a)->wake_reason==PSP_WAKE_SIGNAL);
 assert(psp_thread_cpu_step(&rt)==1);
 assert(rt.active_id==(uint32_t)a && cpu.pc==BASE+12);
 assert(psp_threads_get(&tasks,(uint32_t)a)->state==PSP_TASK_STOPPED);
 assert(psp_thread_cpu_step(&rt)==1);
 assert(psp_threads_get(&tasks,(uint32_t)b)->state==PSP_TASK_STOPPED);
 assert(psp_thread_cpu_step(&rt)==2);
 assert(psp_threads_advance(&tasks,100)==0);
 assert(psp_threads_signal(&tasks,77)==0);
 puts("PASS: synthetic guest create/wait/signal/exit through real MIPS syscall traps");
 return 0;
}
