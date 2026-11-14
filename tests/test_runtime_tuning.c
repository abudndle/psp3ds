#include "psp_runtime_tuning.h"
#include "psp_benchmark.h"
#include "psp_ge_triangles.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void){
 PspRuntimeTuning profiles[PSP_TARGET_COUNT];
 for(int i=0;i<PSP_TARGET_COUNT;i++){
  assert(psp_runtime_tuning_choose((PspTarget)i,&profiles[i])==0);
  assert(profiles[i].target==i);
  assert(profiles[i].cpu_slice_current==profiles[i].cpu_slice_initial);
  assert(profiles[i].cpu_slice_max<=65536u);
  assert(profiles[i].cpu_slice_initial<=profiles[i].cpu_slice_max);
  assert(profiles[i].triangle_demo_budget<=PSP_GE_TRIANGLE_MAX_TRIANGLES);
  assert(profiles[i].benchmark_rounds<=PSP_BENCH_MAX_ROUNDS);
  PspPlatformBudget policy;
  assert(psp_platform_budget_choose((PspTarget)i,&policy)==0);
  assert(profiles[i].io_staging_bytes<=policy.io_budget);
  assert(profiles[i].texture_decode_staging_bytes<=policy.texture_budget);
  PspBenchResult benchmark;
  assert(psp_benchmark_run(profiles[i].benchmark_rounds,&benchmark)==0);
  assert(benchmark.executed_instructions==
         PSP_BENCH_INSNS_PER_ROUND*profiles[i].benchmark_rounds);
 }
 const PspRuntimeTuning *ds=&profiles[PSP_TARGET_DS];
 const PspRuntimeTuning *pak=&profiles[PSP_TARGET_DS_SLOT2];
 const PspRuntimeTuning *dsi=&profiles[PSP_TARGET_DSI];
 const PspRuntimeTuning *old=&profiles[PSP_TARGET_OLD3DS];
 const PspRuntimeTuning *new3ds=&profiles[PSP_TARGET_NEW3DS];
 assert(ds->cpu_slice_initial==pak->cpu_slice_initial);
 assert(ds->cpu_slice_max==pak->cpu_slice_max);
 assert(ds->triangle_demo_budget==pak->triangle_demo_budget);
 assert(ds->request_new3ds_speedup==0 && pak->request_new3ds_speedup==0);
 assert(dsi->cpu_slice_initial>ds->cpu_slice_initial);
 assert(old->cpu_slice_initial>dsi->cpu_slice_initial);
 assert(new3ds->cpu_slice_initial>old->cpu_slice_initial);
 assert(new3ds->triangle_demo_budget>old->triangle_demo_budget);
 assert(new3ds->texture_decode_staging_bytes>old->texture_decode_staging_bytes);
 assert(new3ds->request_new3ds_speedup==1);
 assert(old->request_new3ds_speedup==0);
 PspRuntimeTuning t=*new3ds;
 unsigned start=t.cpu_slice_current;
 assert(psp_runtime_tuning_feedback(&t,start,1)==1);
 assert(t.cpu_slice_current==start*2);
 assert(psp_runtime_tuning_feedback(&t,start*2,1)==0);
 assert(t.cpu_slice_current==t.cpu_slice_max);
 assert(psp_runtime_tuning_feedback(&t,t.cpu_slice_current,t.cpu_window_us)==0);
 assert(psp_runtime_tuning_feedback(&t,t.cpu_slice_current,t.cpu_window_us*2)==1);
 assert(t.cpu_slice_current==start);
 assert(psp_runtime_tuning_feedback(&t,t.cpu_slice_current,t.cpu_window_us*2)==1);
 assert(t.cpu_slice_current==start/2);
 assert(psp_runtime_tuning_feedback(&t,t.cpu_slice_current,t.cpu_window_us*2)==0);
 assert(t.cpu_slice_current==start/2);
 PspRuntimeTuning snapshot=t;
 assert(psp_runtime_tuning_feedback(&t,0,100)==-1);
 assert(psp_runtime_tuning_feedback(&t,1,0)==-1);
 assert(psp_runtime_tuning_feedback(&t,1,100)==-1);
 assert(memcmp(&t,&snapshot,sizeof t)==0);
 assert(psp_runtime_tuning_feedback(NULL,1,1)==-1);
 assert(psp_runtime_tuning_choose(PSP_TARGET_COUNT,&t)==-1);
 assert(t.cpu_slice_current==0 && t.request_new3ds_speedup==0);
 assert(psp_runtime_tuning_choose(PSP_TARGET_NEW3DS,NULL)==-1);
 puts("PASS: New 3DS-specific CPU/triangle budgets, adaptive bounds, DS+Pak unchanged");
 return 0;
}
