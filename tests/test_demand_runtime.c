#include "psp_demand.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE 0x08800000u
static void wr(uint8_t *p,uint32_t w){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(w>>(8*i));}
typedef struct {uint32_t calls,tokens[8];} Callback;
static int on_event(void *ctx,uint32_t token,PspPerf *p){
 Callback *c=(Callback*)ctx;c->tokens[c->calls++]=token;
 if(token==12)p->thread_state=PSP_THREAD_RUNNABLE;
 return 0;
}
int main(void){
 PspPerf p;PspHle h;PspCpu cpu;uint8_t mem[128]={0};PspDemandStats s;Callback cb={0};
 psp_perf_init(&p);psp_hle_init(&h);psp_cpu_init(&cpu,mem,sizeof mem,BASE,BASE);
 for(unsigned i=0;i<20;i++)wr(mem+i*4,0x25080001u);
 assert(p.profile_cpu==0);
 assert(psp_demand_run(&p,&h,&cpu,5,3,100,on_event,&cb,&s)==0);
 assert(s.instructions==5 && cpu.gpr[8]==5 && p.hot_hits[(BASE>>2)%PSP_PERF_HOT_SLOTS]==0);
 psp_perf_enable_profiling(&p,1);
 assert(psp_demand_run(&p,&h,&cpu,3,3,100,on_event,&cb,&s)==0);
 assert(s.instructions==3 && cpu.gpr[8]==8);
 assert(p.hot_hits[((BASE+20)>>2)%PSP_PERF_HOT_SLOTS]==1);
 psp_perf_enable_profiling(&p,0);
 psp_perf_wait_until(&p,80);
 assert(psp_perf_schedule(&p,20,11)==0);
 assert(psp_perf_schedule(&p,40,12)==0);
 assert(psp_demand_run(&p,&h,&cpu,3,3,100,on_event,&cb,&s)==0);
 assert(s.events==2 && s.skipped_idle_us==40 && p.now_us==40 && h.uptime_us==40);
 assert(cb.calls==2 && cb.tokens[0]==11 && cb.tokens[1]==12 && cpu.gpr[8]==11);
 psp_perf_wait_until(&p,150);
 assert(psp_perf_schedule(&p,50,13)==0);
 assert(psp_demand_run(&p,&h,&cpu,2,1,100,on_event,&cb,&s)==1);
 assert(s.events==1 && s.skipped_idle_us==60 && p.now_us==100 && h.uptime_us==100);
 assert(cpu.gpr[8]==11);
 assert(psp_perf_schedule(&p,110,14)==0);
 assert(psp_perf_schedule(&p,120,15)==0);
 assert(psp_demand_run(&p,&h,&cpu,1,1,140,on_event,&cb,&s)==2);
 assert(s.events==1 && p.count==1 && p.pending[0].token==15 && p.now_us==110);
 assert(psp_demand_run(&p,&h,&cpu,0,1,140,on_event,&cb,&s)==-2);
 puts("PASS: demand-driven idle skip, events, CPU equivalence, optional profiling");
 return 0;
}
