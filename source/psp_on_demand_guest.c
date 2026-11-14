#include "psp_on_demand_guest.h"
#include <string.h>
static unsigned outstanding(const PspLazyBackends *s){
 unsigned total=0;
 for(unsigned i=0;i<PSP_LAZY_SUBSYSTEMS;i++)total+=s->pending[i];
 return total;
}
int psp_on_demand_guest_step(PspRuntimeTuning *tuning,
                             PspPerf *perf,PspHle *hle,PspCpu *cpu,
                             PspLazyBackends *backends,
                             const PspLazyBackendOps *ops,
                             const PspPlayCutbacks *cutbacks,
                             uint64_t idle_deadline_us,
                             PspRuntimeHostClock clock,void *context,
                             PspOnDemandGuestStep *out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!tuning||!perf||!hle||!cpu||!backends||!ops ||
    (unsigned)tuning->target>=PSP_TARGET_COUNT ||
    backends->target!=tuning->target ||
    (cutbacks && cutbacks->target!=tuning->target))return -2;
 if(cutbacks && psp_play_cutbacks_apply_tuning(cutbacks,tuning)!=0)
  return -2;
 const int guest_result=psp_runtime_guest_step(tuning,perf,hle,cpu,
                                    idle_deadline_us,clock,context,
                                    &out->guest);
 if(guest_result<0)return guest_result;
 /* A successful timed slice may have grown the future instruction budget.
  * Honor cutback ceilings for the NEXT step, never shorten the one run. */
 if(cutbacks){
  (void)psp_play_cutbacks_apply_tuning(cutbacks,tuning);
  out->guest.next_budget=tuning->cpu_slice_current;
 }
 if(outstanding(backends)){
  unsigned completed=0;
  const int result=psp_lazy_backends_pump(backends,ops,&completed);
  out->backend_completed=completed;
  out->backend_called=1u;
  out->backend_remaining=outstanding(backends);
  if(result<0)return -4;
 }else{
  /* Even with no backend work, the guest CPU/event stage above still ran.
   * Do not touch any lazy subsystem or initialize its resources. */
  out->backend_remaining=0;
 }
 return guest_result;
}
