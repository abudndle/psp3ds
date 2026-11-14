#include "psp_lazy_backends.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
 uint32_t started[PSP_LAZY_SUBSYSTEMS];
 uint32_t worked[PSP_LAZY_SUBSYSTEMS];
 uint32_t stopped[PSP_LAZY_SUBSYSTEMS];
 int fail_start_once;
 int fail_work_once;
} Mock;
static int begin(void *opaque,PspLazySubsystem sub){
 Mock *m=(Mock*)opaque;
 if(m->fail_start_once){m->fail_start_once=0;return -1;}
 m->started[sub]++;
 return 0;
}
static int do_work(void *opaque,PspLazySubsystem sub){
 Mock *m=(Mock*)opaque;
 if(m->fail_work_once){m->fail_work_once=0;return -1;}
 m->worked[sub]++;
 return 0;
}
static void end(void *opaque,PspLazySubsystem sub){
 ((Mock*)opaque)->stopped[sub]++;
}
static PspLazyBackendOps operations(Mock *m){
 PspLazyBackendOps o={begin,do_work,end,m};
 return o;
}
static uint32_t queued(const PspLazyBackends *s){
 uint32_t sum=0;
 for(unsigned j=0;j<PSP_LAZY_SUBSYSTEMS;j++)sum+=s->pending[j];
 return sum;
}
static void model_profiles(void){
 const unsigned work_quota[PSP_TARGET_COUNT]={1,1,2,3,6};
 for(int target=0;target<PSP_TARGET_COUNT;target++){
  PspLazyBackends s={0};
  Mock mock={0};
  PspLazyBackendOps ops=operations(&mock);
  assert(psp_lazy_backends_init(&s,(PspTarget)target)==0);
  assert(s.units_per_pump==work_quota[target]);
  unsigned done=99;
  for(unsigned i=0;i<100;i++){
   assert(psp_lazy_backends_pump(&s,&ops,&done)==0);
   assert(done==0u);
  }
  assert(s.idle_pumps==100u&&s.initialized_count==0u);
  for(unsigned i=0;i<PSP_LAZY_SUBSYSTEMS;i++)
   assert(mock.started[i]==0u && mock.worked[i]==0u);
  assert(psp_lazy_backends_request(&s,PSP_LAZY_GPU,5)==0);
  assert(psp_lazy_backends_request(&s,PSP_LAZY_AUDIO,5)==0);
  assert(psp_lazy_backends_request(&s,PSP_LAZY_UMD,5)==0);
  unsigned total_done=0;
  while(queued(&s)){
   assert(psp_lazy_backends_pump(&s,&ops,&done)==1);
   assert(done>0u && done<=work_quota[target]);
   total_done+=done;
  }
  assert(total_done==15u);
  assert(mock.worked[0]==5u && mock.worked[1]==5u &&
         mock.worked[2]==5u);
  assert(s.initialized_count==3u);
  for(unsigned i=0;i<3;i++)assert(mock.started[i]==1u);
  PspLazyShutdown report;
  assert(psp_lazy_backends_close(&s,&ops,&report)==0);
  assert(report.completed_units==15u);
  assert(report.pending_units==0u && report.initialized_services==3u);
  assert(report.leftover_cancelled_on_shutdown==0u);
  for(unsigned i=0;i<3;i++)assert(mock.stopped[i]==1u);
  assert(!s.initialized && !s.units_per_pump);
  assert(psp_lazy_backends_close(&s,&ops,NULL)==-1);
 }
}
static void failures_retries_and_explicit_cancel(void){
 PspLazyBackends s={0};
 Mock mock={0};
 PspLazyBackendOps ops=operations(&mock);
 unsigned done=0;
 assert(psp_lazy_backends_init(&s,PSP_TARGET_DS_SLOT2)==0);
 assert(s.units_per_pump==1u);
 assert(psp_lazy_backends_request(&s,PSP_LAZY_GPU,0)==-1);
 assert(psp_lazy_backends_request(&s,PSP_LAZY_GPU,65)==-1);
 assert(psp_lazy_backends_request(&s,PSP_LAZY_GPU,64)==0);
 assert(psp_lazy_backends_request(&s,PSP_LAZY_GPU,1)==-1);
 assert(queued(&s)==64u);
 mock.fail_start_once=1;
 assert(psp_lazy_backends_pump(&s,&ops,&done)==-2);
 assert(done==0u && queued(&s)==64u && s.initialized_count==0u);
 mock.fail_work_once=1;
 assert(psp_lazy_backends_pump(&s,&ops,&done)==-2);
 assert(done==0u && queued(&s)==64u && s.initialized_count==1u);
 assert(mock.started[PSP_LAZY_GPU]==1u);
 assert(psp_lazy_backends_pump(&s,&ops,&done)==1);
 assert(done==1u && queued(&s)==63u);
 assert(s.failed_calls==2u);
 assert(psp_lazy_backends_cancel(&s,PSP_LAZY_GPU,63u)==0);
 assert(queued(&s)==0u);
 assert(psp_lazy_backends_cancel(&s,PSP_LAZY_GPU,1u)==-1);
 assert(psp_lazy_backends_request(&s,PSP_LAZY_UMD,2u)==0);
 PspLazyShutdown report;
 assert(psp_lazy_backends_close(&s,&ops,&report)==0);
 assert(report.completed_units==1u);
 assert(report.pending_units==2u &&
        report.leftover_cancelled_on_shutdown==2u);
 assert(report.initialized_services==1u);
 assert(mock.stopped[PSP_LAZY_GPU]==1u);
 assert(mock.started[PSP_LAZY_UMD]==0u);
 assert(mock.stopped[PSP_LAZY_UMD]==0u);
}
static void invalid(void){
 PspLazyBackends s={0};
 Mock mock={0};
 PspLazyBackendOps ops=operations(&mock);
 unsigned done=3u;
 assert(psp_lazy_backends_init(&s,PSP_TARGET_COUNT)==-1);
 assert(s.initialized==0u);
 assert(psp_lazy_backends_init(NULL,PSP_TARGET_DS)==-1);
 assert(psp_lazy_backends_init(&s,PSP_TARGET_DS)==0);
 assert(psp_lazy_backends_init(&s,PSP_TARGET_DS)==-1);
 assert(psp_lazy_backends_request(&s,(PspLazySubsystem)3u,1u)==-1);
 assert(psp_lazy_backends_request(NULL,PSP_LAZY_GPU,1u)==-1);
 assert(psp_lazy_backends_pump(&s,NULL,&done)==-1);
 assert(done==0u);
 assert(psp_lazy_backends_pump(&s,&ops,NULL)==-1);
 assert(psp_lazy_backends_close(&s,&ops,NULL)==0);
 assert(psp_lazy_backends_request(&s,PSP_LAZY_GPU,1u)==-1);
}
int main(void){
 model_profiles();failures_retries_and_explicit_cancel();invalid();
 puts("PASS: demand-only GPU/audio/UMD work across five console profiles, bounded fairness and safe cleanup");
 return 0;
}
