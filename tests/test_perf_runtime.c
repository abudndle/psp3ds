#include "psp_perf.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void wr(uint8_t *p,uint32_t w){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(w>>(i*8));}
int main(void){
 PspPerf perf;PspHle hle;PspCpu cpu;uint8_t ram[256]={0};unsigned executed=99;
 psp_perf_init(&perf);psp_hle_init(&hle);psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 for(unsigned i=0;i<10;i++)wr(ram+i*4,0x25080001u);
 assert(psp_perf_run_slice(&perf,&hle,&cpu,4,&executed)==0);
 assert(executed==4 && cpu.gpr[8]==4 && cpu.pc==BASE+16);
 assert(psp_perf_run_slice(&perf,&hle,&cpu,0,&executed)==-2);
 assert(executed==0 && cpu.gpr[8]==4);
 psp_perf_wait_until(&perf,200);
 assert(psp_perf_run_slice(&perf,&hle,&cpu,10,&executed)==2 && executed==0);
 assert(psp_perf_schedule(&perf,150,10)==0);
 assert(psp_perf_idle_hle(&perf,&hle,1000)==10);
 assert(perf.now_us==150 && hle.uptime_us==150 && perf.thread_state==PSP_THREAD_WAITING);
 assert(psp_perf_idle_hle(&perf,&hle,1000)==0);
 assert(perf.now_us==200 && hle.uptime_us==200 && perf.thread_state==PSP_THREAD_RUNNABLE);
 assert(psp_perf_run_slice(&perf,&hle,&cpu,3,&executed)==0);
 assert(executed==3 && cpu.gpr[8]==7);
 assert(psp_perf_record_block(&perf,BASE,1)==1);
 assert(psp_perf_texture_seen(&perf,0x111,4)==0);
 assert(psp_perf_texture_seen(&perf,0x111,4)==1);
 psp_perf_invalidate_textures(&perf);
 assert(psp_perf_texture_seen(&perf,0x111,4)==0);
 puts("PASS: bounded CPU slices, deterministic idle clock, lazy cache invalidation");return 0;
}
