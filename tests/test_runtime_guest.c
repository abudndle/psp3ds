#include "psp_runtime_guest.h"
#include "psp_benchmark.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
static uint8_t guest[PSP_BENCH_INSNS_PER_ROUND * PSP_BENCH_MAX_ROUNDS * 4u];
typedef struct {uint64_t first,second;unsigned calls;} Sample;
static uint64_t sample(void *ctx){
 Sample *s=(Sample*)ctx;
 return s->calls++==0?s->first:s->second;
}
static void write32(uint8_t *p,uint32_t w){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(w>>(i*8u));
}
static void program(void){
 for(size_t i=0;i<sizeof guest;i+=4u)
  write32(guest+i,UINT32_C(0x25080001)); /* addiu t0,t0,1 */
}
static void run_each_model(void){
 for(int target=0;target<PSP_TARGET_COUNT;target++){
  PspRuntimeTuning tuning;
  assert(psp_runtime_tuning_choose((PspTarget)target,&tuning)==0);
  PspPerf perf;PspHle hle;PspCpu cpu;
  psp_perf_init(&perf);psp_hle_init(&hle);
  psp_cpu_init(&cpu,guest,sizeof guest,BASE,BASE);
  PspRuntimeGuestStep result;
  Sample clock={1000,1100,0};
  const uint32_t expected=tuning.cpu_slice_current;
  assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                                sample,&clock,&result)==0);
  assert(clock.calls==2);
  assert(result.requested==expected && result.executed==expected);
  assert(cpu.gpr[8]==expected && cpu.gpr[0]==0 &&
         cpu.pc==BASE+expected*4u);
  assert(result.wall_elapsed_us==100);
  assert(hle.uptime_us==0 && perf.now_us==0);
  assert(result.event_token==0);
  assert(tuning.cpu_slice_current>expected);
  assert(result.next_budget==tuning.cpu_slice_current);
  assert(result.feedback_applied==1);
  /* Cross-device identical MIPS semantics, different native work caps. */
 }
}
static void invalid_and_partial(void){
 PspRuntimeTuning tuning;
 assert(psp_runtime_tuning_choose(PSP_TARGET_NEW3DS,&tuning)==0);
 PspPerf perf;PspHle hle;PspCpu cpu;
 psp_perf_init(&perf);psp_hle_init(&hle);
 psp_cpu_init(&cpu,guest,sizeof guest,BASE,BASE);
 PspRuntimeGuestStep r;
 memset(&r,0xa5,sizeof r);
 assert(psp_runtime_guest_step(NULL,&perf,&hle,&cpu,0,NULL,NULL,&r)==-2);
 assert(r.executed==0&&r.requested==0&&r.next_budget==0);
 const PspRuntimeTuning old=tuning;
 assert(psp_runtime_guest_step(&tuning,NULL,&hle,&cpu,0,NULL,NULL,&r)==-2);
 assert(psp_runtime_guest_step(&tuning,&perf,NULL,&cpu,0,NULL,NULL,&r)==-2);
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,NULL,0,NULL,NULL,&r)==-2);
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,NULL,NULL,NULL)==-2);
 assert(memcmp(&tuning,&old,sizeof tuning)==0);
 /* No clock: still performs the exact work, but cannot change policy. */
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                               NULL,NULL,&r)==0);
 assert(r.executed==old.cpu_slice_current && r.feedback_applied==0 &&
        r.wall_elapsed_us==0 && r.next_budget==old.cpu_slice_current);
 assert(memcmp(&tuning,&old,sizeof tuning)==0);
 /* Timer went backwards: ignore it, never change guest execution. */
 psp_cpu_init(&cpu,guest,sizeof guest,BASE,BASE);
 Sample backward={1234,1233,0};
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                               sample,&backward,&r)==0);
 assert(r.executed==old.cpu_slice_current &&
        r.wall_elapsed_us==0 && r.feedback_applied==0);
 assert(memcmp(&tuning,&old,sizeof tuning)==0);
 /* Very slow wall time causes one bounded reduction, not lost instructions. */
 psp_cpu_init(&cpu,guest,sizeof guest,BASE,BASE);
 Sample slow={100,100000,0};
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                               sample,&slow,&r)==0);
 assert(r.executed==old.cpu_slice_current);
 assert(tuning.cpu_slice_current==old.cpu_slice_current/2u);
 assert(r.feedback_applied==1);
 /* Invalid policy rejected before any guest execution. */
 const uint32_t previous_pc=cpu.pc;
 tuning.cpu_slice_current=tuning.cpu_slice_max+1;
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                               NULL,NULL,&r)==-2);
 assert(cpu.pc==previous_pc && r.requested==0);
}
static void hle_and_idle_events(void){
 PspRuntimeTuning tuning;
 assert(psp_runtime_tuning_choose(PSP_TARGET_DS_SLOT2,&tuning)==0);
 PspPerf perf;PspHle hle;PspCpu cpu;PspRuntimeGuestStep r;
 psp_perf_init(&perf);psp_hle_init(&hle);
 psp_cpu_init(&cpu,guest,sizeof guest,BASE,BASE);
 assert(psp_hle_bind(&hle,7,PSP_HLE_SYSTEM_TIME_LOW)==0);
 /* A genuine MIPS syscall is handled by the shared HLE dispatcher. */
 uint8_t short_guest[16]={0};
 write32(short_guest,((uint32_t)7u<<6)|12u);
 psp_cpu_init(&cpu,short_guest,sizeof short_guest,BASE,BASE);
 hle.uptime_us=9876;
 Sample clock={100,120,0};
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,0,
                               sample,&clock,&r)==0);
 assert(r.requested==128 && r.executed==1);
 assert(cpu.gpr[2]==9876 && cpu.pc==BASE+4u);
 assert(r.feedback_applied==0 && tuning.cpu_slice_current==128);
 /* With no runnable guest, advancing a virtual idle deadline remains
  * the scheduler's job: no host-time feedback or phantom instructions. */
 assert(psp_perf_wait_signal(&perf,11,200)==0);
 assert(psp_perf_schedule(&perf,150,22)==0);
 clock.calls=0;
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,300,
                               sample,&clock,&r)==1);
 assert(r.event_token==22 && r.executed==0);
 assert(perf.now_us==150 && hle.uptime_us==10026);
 assert(clock.calls==0);
 assert(psp_runtime_guest_step(&tuning,&perf,&hle,&cpu,300,
                               sample,&clock,&r)==3);
 assert(perf.now_us==200 && hle.uptime_us==10076);
 assert(perf.thread_state==PSP_THREAD_RUNNABLE);
 assert(tuning.cpu_slice_current==128);
}
int main(void){
 program();
 run_each_model();
 invalid_and_partial();
 hle_and_idle_events();
 puts("PASS: real bounded MIPS/HLE execution, New 3DS adaptive feedback, DS+Pak virtual timing");
 return 0;
}
