#include "psp_on_demand_guest.h"
#include "psp_benchmark.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BASE UINT32_C(0x08800000)
#define MAX_RAM 131072u
typedef struct {
 unsigned begin[PSP_LAZY_SUBSYSTEMS];
 unsigned calls[PSP_LAZY_SUBSYSTEMS];
 unsigned end[PSP_LAZY_SUBSYSTEMS];
 int fail_once;
} Backend;
static int backend_start(void *ctx,PspLazySubsystem sub){
 ((Backend*)ctx)->begin[sub]++;
 return 0;
}
static int backend_work(void *ctx,PspLazySubsystem sub){
 Backend *b=(Backend*)ctx;
 if(b->fail_once){b->fail_once=0;return -1;}
 b->calls[sub]++;
 return 0;
}
static void backend_stop(void *ctx,PspLazySubsystem sub){
 ((Backend*)ctx)->end[sub]++;
}
static PspLazyBackendOps ops(Backend *b){
 PspLazyBackendOps o={backend_start,backend_work,backend_stop,b};
 return o;
}
static void write32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static uint8_t ram[MAX_RAM];
static void prepare(void){
 for(unsigned i=0;i<MAX_RAM;i+=4u)
  write32(ram+i,UINT32_C(0x25080001)); /* addiu t0,t0,1 */
}
static void models(void){
 const unsigned quotas[PSP_TARGET_COUNT]={1,1,2,3,6};
 for(int target=0;target<PSP_TARGET_COUNT;target++){
  PspRuntimeTuning tune;
  PspPerf perf;PspHle hle;PspCpu cpu;
  PspLazyBackends lazy={0};
  PspPlayCutbacks cuts;
  Backend b={0};
  PspLazyBackendOps cb=ops(&b);
  PspOnDemandGuestStep step;
  assert(psp_runtime_tuning_choose((PspTarget)target,&tune)==0);
  assert(psp_lazy_backends_init(&lazy,(PspTarget)target)==0);
  assert(psp_play_cutbacks_init(&cuts,(PspTarget)target)==0);
  psp_perf_init(&perf);psp_hle_init(&hle);
  psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
  const unsigned initial=tune.cpu_slice_current;
  /* No active requests -> no GPU, UMD, audio init or callbacks. */
  assert(psp_on_demand_guest_step(&tune,&perf,&hle,&cpu,&lazy,&cb,
                       &cuts,0,NULL,NULL,&step)==0);
  assert(step.guest.executed==initial);
  assert(cpu.gpr[8]==initial && cpu.pc==BASE+4u*initial);
  assert(step.backend_called==0 && step.backend_completed==0 &&
         lazy.initialized_count==0u);
  assert(perf.initialized_mask==0u && perf.active_mask==0u);
  assert(perf.now_us==0 && hle.uptime_us==0);
  assert(psp_lazy_backends_request(&lazy,PSP_LAZY_AUDIO,3u)==0);
  assert(psp_lazy_backends_request(&lazy,PSP_LAZY_GPU,2u)==0);
  assert(psp_lazy_backends_request(&lazy,PSP_LAZY_UMD,4u)==0);
  assert(psp_on_demand_guest_step(&tune,&perf,&hle,&cpu,&lazy,&cb,
                       &cuts,0,NULL,NULL,&step)==0);
  assert(step.guest.executed==initial);
  assert(step.backend_called==1 && step.backend_completed==quotas[target]);
  assert(step.backend_remaining==9u-quotas[target]);
  assert(perf.initialized_mask!=0u && perf.active_mask!=0u);
  for(unsigned i=0;i<PSP_LAZY_SUBSYSTEMS;i++){
   if(lazy.started[i]){
    assert((perf.initialized_mask&(1u<<i))!=0u);
    assert(((perf.active_mask&(1u<<i))!=0u)==(lazy.pending[i]!=0u));
   }else{
    assert((perf.initialized_mask&(1u<<i))==0u);
   }
  }
  assert(cpu.gpr[8]==initial*2u);
  assert(perf.now_us==0 && hle.uptime_us==0);
  const uint32_t pc=cpu.pc;
  /* Waiting guest: only actual scheduled events advance virtual time.
   * Explicitly queued audio/GPU/UMD work can still be processed. */
  psp_perf_wait_until(&perf,500u);
  assert(psp_perf_schedule(&perf,450u,12u)==0);
  assert(psp_on_demand_guest_step(&tune,&perf,&hle,&cpu,&lazy,&cb,
                    &cuts,1000u,NULL,NULL,&step)==1);
  assert(step.guest.executed==0 && step.guest.event_token==12u);
  assert(cpu.pc==pc && perf.now_us==450u && hle.uptime_us==450u);
  assert(psp_on_demand_guest_step(&tune,&perf,&hle,&cpu,&lazy,&cb,
                    &cuts,1000u,NULL,NULL,&step)==3);
  assert(cpu.pc==pc && perf.now_us==500u && hle.uptime_us==500u);
  assert(perf.thread_state==PSP_THREAD_RUNNABLE);
  assert(psp_lazy_backends_request(&lazy,PSP_LAZY_GPU,1u)==0);
  b.fail_once=1;
  assert(psp_on_demand_guest_step(&tune,&perf,&hle,&cpu,&lazy,&cb,
                    &cuts,1000u,NULL,NULL,&step)==-4);
  assert(step.guest.executed==initial);
  assert(cpu.pc==pc+initial*4u);
  /* The failed optional backend unit is still pending, never lost.
   * The CPU stage completed; there is no fictitious rollback. */
  assert(step.backend_remaining>0u);
  PspLazyShutdown shutdown;
  assert(psp_lazy_backends_close(&lazy,&cb,&shutdown)==0);
  assert(shutdown.initialized_services>0u);
  for(unsigned i=0;i<3;i++)
   assert(b.begin[i]==b.end[i]);
 }
}
static void cutbacks_and_invalid(void){
 PspRuntimeTuning tune;
 PspPerf perf;PspHle hle;PspCpu cpu;
 PspLazyBackends lazy={0};
 PspPlayCutbacks cuts;
 Backend b={0};
 PspLazyBackendOps cb=ops(&b);
 PspOnDemandGuestStep step;
 assert(psp_runtime_tuning_choose(PSP_TARGET_NEW3DS,&tune)==0);
 assert(psp_lazy_backends_init(&lazy,PSP_TARGET_NEW3DS)==0);
 assert(psp_play_cutbacks_init(&cuts,PSP_TARGET_NEW3DS)==0);
 assert(psp_play_cutbacks_observe(&cuts,50000u,0,0,NULL)==1);
 assert(cuts.quality==PSP_PLAY_EMERGENCY);
 psp_perf_init(&perf);psp_hle_init(&hle);
 psp_cpu_init(&cpu,ram,sizeof ram,BASE,BASE);
 tune.cpu_slice_current=tune.cpu_slice_max;
 assert(psp_on_demand_guest_step(&tune,&perf,&hle,&cpu,&lazy,&cb,
                       &cuts,0,NULL,NULL,&step)==0);
 assert(step.guest.executed==cuts.cpu_slice_ceiling);
 assert(tune.cpu_slice_current==cuts.cpu_slice_ceiling);
 assert(cpu.gpr[8]==cuts.cpu_slice_ceiling);
 assert(psp_on_demand_guest_step(NULL,&perf,&hle,&cpu,&lazy,&cb,
                       &cuts,0,NULL,NULL,&step)==-2);
 assert(step.backend_completed==0 && step.guest.executed==0);
 assert(psp_lazy_backends_close(&lazy,&cb,NULL)==0);
}
int main(void){
 prepare();models();cutbacks_and_invalid();
 puts("PASS: demand-only real MIPS/HLE CPU, idle events, lazy GPU/audio/UMD, 5 model budgets");
 return 0;
}
