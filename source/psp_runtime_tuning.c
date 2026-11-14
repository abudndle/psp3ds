#include "psp_runtime_tuning.h"
#include "psp_benchmark.h"
#include "psp_ge_triangles.h"
#include <string.h>

typedef struct {
 uint32_t slice,max_slice,window_us,texture_staging,io_staging;
 uint16_t triangles;
 uint8_t rounds,speedup;
} TuningDefaults;

/* Intentional *policy*, not a measurement. No memory is allocated by
 * selecting a profile. Slot-2 is cold data only: it cannot raise ARM9
 * instruction throughput or 3D polygon throughput. */
static const TuningDefaults defaults[PSP_TARGET_COUNT]={
 /* DS */      {128,256,2000,256,4096,4,4,0},
 /* DS+Pak */  {128,256,2000,256,4096,4,4,0},
 /* DSi */     {256,512,2500,512,8192,8,8,0},
 /* Old 3DS */ {2048,4096,4000,2048,16384,8,16,0},
 /* New 3DS */ {8192,16384,6000,4096,32768,24,64,1}
};
int psp_runtime_tuning_choose(PspTarget target,PspRuntimeTuning *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||(unsigned)target>=PSP_TARGET_COUNT)return -1;
 PspPlatformBudget b;
 if(psp_platform_budget_choose(target,&b)!=0 ||
    psp_platform_budget_validate(&b)!=0)return -1;
 const TuningDefaults *d=&defaults[target];
 if(!d->slice||d->slice>d->max_slice||d->max_slice>65536u||
    !d->window_us||!d->rounds||d->rounds>PSP_BENCH_MAX_ROUNDS||
    d->triangles>PSP_GE_TRIANGLE_MAX_TRIANGLES ||
    d->texture_staging>b.texture_budget ||
    d->io_staging>b.io_budget ||
    d->speedup!=(target==PSP_TARGET_NEW3DS))return -1;
 out->target=target;
 out->cpu_slice_initial=d->slice;
 out->cpu_slice_current=d->slice;
 out->cpu_slice_max=d->max_slice;
 out->cpu_window_us=d->window_us;
 out->texture_decode_staging_bytes=d->texture_staging;
 out->io_staging_bytes=d->io_staging;
 out->triangle_demo_budget=d->triangles;
 out->benchmark_rounds=d->rounds;
 out->request_new3ds_speedup=d->speedup;
 return 0;
}
int psp_runtime_tuning_feedback(PspRuntimeTuning *t,
                                uint32_t completed,uint32_t elapsed_us){
 if(!t || (unsigned)t->target>=PSP_TARGET_COUNT ||
    !t->cpu_slice_initial || !t->cpu_slice_current ||
    t->cpu_slice_current>t->cpu_slice_max ||
    !t->cpu_window_us || !completed || !elapsed_us ||
    completed!=t->cpu_slice_current)return -1;
 uint32_t next=t->cpu_slice_current;
 /* Conservative hysteresis, clamped at one power-of-two step. It is not
  * real PSP timing: the emulator must independently preserve guest clocks. */
 if(elapsed_us>t->cpu_window_us) {
  next/=2u;
  if(next<t->cpu_slice_initial/2u)next=t->cpu_slice_initial/2u;
 }else if((uint64_t)elapsed_us*2u<t->cpu_window_us) {
  if(next<t->cpu_slice_max/2u)next*=2u;
  else next=t->cpu_slice_max;
 }
 if(next==t->cpu_slice_current)return 0;
 t->cpu_slice_current=next;
 return 1;
}
