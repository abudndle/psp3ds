#include "psp_runtime_guest.h"
#include <string.h>
#include <stdint.h>

int psp_runtime_guest_step(PspRuntimeTuning *t,PspPerf *perf,
                           PspHle *hle,PspCpu *cpu,
                           uint64_t idle_deadline_us,
                           PspRuntimeHostClock clock,void *context,
                           PspRuntimeGuestStep *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!t||!perf||!hle||!cpu||
    (unsigned)t->target>=PSP_TARGET_COUNT ||
    !t->cpu_slice_initial || !t->cpu_slice_max ||
    !t->cpu_window_us || !t->cpu_slice_current ||
    t->cpu_slice_initial>t->cpu_slice_max ||
    t->cpu_slice_max>65536u ||
    t->cpu_slice_current>t->cpu_slice_max ||
    t->cpu_slice_current<t->cpu_slice_initial/2u)return -2;
 const unsigned requested=t->cpu_slice_current;
 const int runnable=perf->thread_state==PSP_THREAD_RUNNABLE;
 const uint64_t before=(clock&&runnable)?clock(context):0;
 unsigned executed=0;
 uint32_t token=0;
 const int status=psp_perf_run_demand(perf,hle,cpu,requested,
                                       idle_deadline_us,&executed,&token);
 const uint64_t after=(clock&&runnable)?clock(context):0;
 out->requested=requested;
 out->executed=executed;
 out->event_token=token;
 out->next_budget=t->cpu_slice_current;
 /* An invalid, non-monotonic or overflowing sample cannot influence
  * emulation scheduling. Do not tune on a short syscall/branch, fault,
  * an idle wait, or an event result. HLE virtual time stays untouched. */
 if(clock&&runnable&&after>before&&after-before<=UINT32_MAX){
  out->wall_elapsed_us=(uint32_t)(after-before);
  if(status==0&&executed==requested){
   int adjusted=psp_runtime_tuning_feedback(t,executed,out->wall_elapsed_us);
   if(adjusted>0){
    out->feedback_applied=1;
    out->next_budget=t->cpu_slice_current;
   }
  }
 }
 return status;
}
