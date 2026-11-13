#include "psp_perf.h"
#include <assert.h>
#include <stdio.h>
#define BASE 0x08800000u
int main(void){
 PspPerf perf;PspHle hle;PspCpu cpu;unsigned executed;uint32_t event;
 uint8_t ram[32]={0};
 psp_perf_init(&perf);psp_hle_init(&hle);
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 psp_perf_wait_until(&perf,500);
 assert(psp_perf_schedule(&perf,100,17)==0);
 assert(psp_perf_schedule(&perf,250,23)==0);
 uint32_t pc=cpu.pc;
 assert(psp_perf_run_demand(&perf,&hle,&cpu,16,1000,&executed,&event)==1);
 assert(!executed && event==17 && perf.now_us==100 && hle.uptime_us==100 && cpu.pc==pc);
 assert(psp_perf_run_demand(&perf,&hle,&cpu,16,1000,&executed,&event)==1);
 assert(!executed && event==23 && perf.now_us==250 && hle.uptime_us==250 && cpu.pc==pc);
 assert(psp_perf_run_demand(&perf,&hle,&cpu,16,300,&executed,&event)==2);
 assert(!executed && !event && perf.now_us==300 && hle.uptime_us==300);
 assert(psp_perf_run_demand(&perf,&hle,&cpu,16,1000,&executed,&event)==3);
 assert(!executed && !event && perf.now_us==500 && hle.uptime_us==500);
 assert(psp_perf_run_demand(&perf,&hle,&cpu,2,1000,&executed,&event)==0);
 assert(executed==2 && cpu.pc==BASE+8);
 assert(psp_perf_subsystem_active(&perf,PSP_SUBSYSTEM_GPU)==0);
 assert(psp_perf_set_subsystem_active(&perf,PSP_SUBSYSTEM_GPU,1)==0);
 assert(psp_perf_subsystem_active(&perf,PSP_SUBSYSTEM_GPU)==1);
 assert(perf.initialized_mask&1u);
 assert(psp_perf_set_subsystem_active(&perf,PSP_SUBSYSTEM_GPU,0)==0);
 assert(psp_perf_subsystem_active(&perf,PSP_SUBSYSTEM_GPU)==0);
 assert(psp_perf_subsystem_active(&perf,(PspSubsystem)90)==-1);
 assert(psp_perf_set_subsystem_active(&perf,(PspSubsystem)90,1)==-1);
 assert(psp_perf_run_demand(&perf,&hle,&cpu,0,1000,&executed,&event)==-2);
 assert(executed==0 && event==0);
 puts("PASS: demand-driven idle jumps, events, subsystem work gating and CPU semantics");return 0;
}
