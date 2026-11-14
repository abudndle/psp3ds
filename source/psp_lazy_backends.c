#include "psp_lazy_backends.h"
#include <string.h>
#include <stdint.h>
#define LAZY_ACTIVE 0xa5u
static int valid(const PspLazyBackends *s){
 return s && s->initialized==LAZY_ACTIVE &&
        (unsigned)s->target<PSP_TARGET_COUNT &&
        s->units_per_pump>=1u && s->units_per_pump<=8u;
}
static unsigned total_pending(const PspLazyBackends *s){
 unsigned count=0;
 for(unsigned i=0;i<PSP_LAZY_SUBSYSTEMS;i++)count+=s->pending[i];
 return count;
}
int psp_lazy_backends_init(PspLazyBackends *s,PspTarget target){
 if(!s || (unsigned)target>=PSP_TARGET_COUNT ||
    s->initialized==LAZY_ACTIVE)return -1;
 memset(s,0,sizeof(*s));
 PspPlatformBudget policy;
 if(psp_platform_budget_choose(target,&policy)!=0 ||
    psp_platform_budget_validate(&policy)!=0)return -1;
 static const uint8_t quotas[PSP_TARGET_COUNT]={1,1,2,3,6};
 s->target=target;
 s->units_per_pump=quotas[target];
 s->initialized=LAZY_ACTIVE;
 return 0;
}
int psp_lazy_backends_request(PspLazyBackends *s,
                              PspLazySubsystem sub,unsigned units){
 if(!valid(s)||(unsigned)sub>=PSP_LAZY_SUBSYSTEMS ||
    !units||units>PSP_LAZY_MAX_QUEUED ||
    units>PSP_LAZY_MAX_QUEUED-s->pending[sub])return -1;
 s->pending[sub]+=(uint16_t)units;
 return 0;
}
int psp_lazy_backends_cancel(PspLazyBackends *s,
                             PspLazySubsystem sub,unsigned units){
 if(!valid(s)||(unsigned)sub>=PSP_LAZY_SUBSYSTEMS ||
    !units||units>s->pending[sub])return -1;
 s->pending[sub]-=(uint16_t)units;
 return 0;
}
int psp_lazy_backends_pump(PspLazyBackends *s,
                           const PspLazyBackendOps *ops,unsigned *done){
 if(done)*done=0;
 if(!valid(s)||!done||!ops||!ops->start||!ops->work||
    !ops->stop)return -1;
 if(!total_pending(s)){s->idle_pumps++;return 0;}
 for(unsigned i=0;i<s->units_per_pump;i++){
  unsigned selected=PSP_LAZY_SUBSYSTEMS;
  for(unsigned j=0;j<PSP_LAZY_SUBSYSTEMS;j++){
   unsigned check=(s->next_service+j)%PSP_LAZY_SUBSYSTEMS;
   if(s->pending[check]){selected=check;break;}
  }
  if(selected==PSP_LAZY_SUBSYSTEMS)break;
  PspLazySubsystem sub=(PspLazySubsystem)selected;
  if(!s->started[selected]){
   if(ops->start(ops->context,sub)!=0){
    s->failed_calls++;
    return -2;
   }
   s->started[selected]=1;
   s->initialized_count++;
  }
  if(ops->work(ops->context,sub)!=0){
   s->failed_calls++;
   return -2;
  }
  s->pending[selected]--;
  s->completed[selected]++;
  (*done)++;
  s->next_service=(uint8_t)((selected+1u)%PSP_LAZY_SUBSYSTEMS);
 }
 return *done?1:0;
}
int psp_lazy_backends_close(PspLazyBackends *s,
                            const PspLazyBackendOps *ops,
                            PspLazyShutdown *report){
 if(report)memset(report,0,sizeof(*report));
 if(!valid(s)||!ops||!ops->stop)return -1;
 PspLazyShutdown result={0};
 for(unsigned i=0;i<PSP_LAZY_SUBSYSTEMS;i++){
  result.completed_units+=s->completed[i];
  result.pending_units+=s->pending[i];
  if(s->started[i]){
   ops->stop(ops->context,(PspLazySubsystem)i);
   result.initialized_services++;
  }
 }
 result.leftover_cancelled_on_shutdown=result.pending_units;
 memset(s,0,sizeof(*s));
 if(report)*report=result;
 return 0;
}
