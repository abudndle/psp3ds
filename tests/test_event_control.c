#include "psp_perf.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void assert_order(PspPerf *p,const uint32_t *tokens,size_t n){
 assert(p->count==n);
 for(size_t i=0;i<n;i++)assert(p->pending[i].token==tokens[i]);
}
int main(void){
 PspPerf p;
 psp_perf_init(&p);
 assert(psp_perf_schedule(&p,40,1)==0);
 assert(psp_perf_schedule(&p,20,2)==0);
 assert(psp_perf_schedule(&p,40,3)==0);
 { const uint32_t expected[]={2,1,3};assert_order(&p,expected,3); }
 assert(psp_perf_cancel_event(&p,2)==0);
 { const uint32_t expected[]={1,3};assert_order(&p,expected,2); }
 assert(psp_perf_cancel_event(&p,2)==-1);
 assert(psp_perf_cancel_event(NULL,1)==-1);
 assert(psp_perf_cancel_event(&p,0)==-1);
 assert(psp_perf_reschedule_event(&p,1,10)==0);
 { const uint32_t expected[]={1,3};assert_order(&p,expected,2); }
 assert(p.pending[0].deadline==10 && p.pending[1].deadline==40);
 assert(psp_perf_reschedule_event(&p,1,40)==0);
 { const uint32_t expected[]={3,1};assert_order(&p,expected,2); }
 assert(psp_perf_reschedule_event(&p,99,30)==-1);
 assert(psp_perf_reschedule_event(NULL,1,30)==-1);
 assert(psp_perf_reschedule_event(&p,0,30)==-1);
 p.now_us=25;
 PspEvent old[PSP_PERF_EVENTS];
 memcpy(old,p.pending,sizeof old);
 assert(psp_perf_reschedule_event(&p,1,24)==-1);
 assert(memcmp(old,p.pending,sizeof old)==0);
 assert(p.now_us==25);
 psp_perf_wait_until(&p,100);
 assert(psp_perf_idle_advance(&p,60)==3);
 assert(p.now_us==40 && p.count==1);
 assert(psp_perf_cancel_event(&p,1)==0);
 assert(psp_perf_idle_advance(&p,60)==0);
 assert(p.now_us==60 && p.count==0);
 assert(psp_perf_reschedule_event(&p,1,61)==-1);
 assert(psp_perf_schedule(&p,61,4)==0);
 assert(psp_perf_idle_advance(&p,61)==4);
 assert(p.now_us==61);
 puts("PASS: timer cancellation, stable rescheduling, atomic invalid deadlines, idle delivery");
 return 0;
}
