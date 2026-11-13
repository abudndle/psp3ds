#include "psp_demand.h"
#include <string.h>
int psp_demand_run(PspPerf *p,PspHle *hle,PspCpu *cpu,
                   uint32_t instruction_budget,uint32_t event_budget,
                   uint64_t deadline_us,PspDemandEventHandler handler,
                   void *opaque,PspDemandStats *stats){
 if(stats)memset(stats,0,sizeof(*stats));
 if(!p || !hle || !cpu || !stats || !instruction_budget ||
    instruction_budget>65536 || !event_budget || event_budget>PSP_PERF_EVENTS ||
    deadline_us<p->now_us)return -2;
 while(1){
  if(p->thread_state==PSP_THREAD_WAITING){
   if(p->count && p->pending[0].deadline<=deadline_us && stats->events>=event_budget)
    return 2;
   uint64_t old=p->now_us;
   uint32_t token=psp_perf_idle_hle(p,hle,deadline_us);
   stats->skipped_idle_us+=p->now_us-old;
   if(token){
    stats->events++;
    if(handler){
     int rc=handler(opaque,token,p);
     if(rc)return rc<0?rc:-3;
    }
    continue;
   }
   if(p->thread_state==PSP_THREAD_WAITING || p->now_us==deadline_us)return 1;
  }
  if(stats->instructions>=instruction_budget)return 0;
  uint32_t pc=cpu->pc;
  int rc=psp_perf_step(p,hle,cpu);
  if(rc==2)continue;
  if(rc<0)return rc;
  stats->instructions++;
  if(p->profile_cpu)(void)psp_perf_record_block(p,pc,16);
  if(rc==1)return 0;
 }
}
