#include "psp_perf.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void){
 PspPerf p; psp_perf_init(&p);
 assert(psp_perf_wait_signal(NULL,1,5)==-1);
 assert(psp_perf_wait_signal(&p,0,5)==-1);
 assert(psp_perf_wait_signal(&p,1,0)==-1);
 assert(psp_perf_wait_signal(&p,3,100)==0);
 assert(psp_perf_wait_signal(&p,3,200)==-1);
 assert(psp_perf_signal_wait(&p,4)==0);
 assert(psp_perf_idle_advance(&p,40)==0 && p.now_us==40 &&
        p.thread_state==PSP_THREAD_WAITING);
 assert(psp_perf_schedule(&p,50,7)==0);
 assert(psp_perf_idle_advance(&p,80)==7 && p.now_us==50 &&
        p.thread_state==PSP_THREAD_WAITING);
 assert(psp_perf_signal_wait(&p,3)==1);
 assert(p.thread_state==PSP_THREAD_RUNNABLE &&
        p.wait_signal_token==0 && p.now_us==50);
 assert(psp_perf_signal_wait(&p,3)==0);
 assert(psp_perf_wait_signal(&p,9,70)==0);
 assert(psp_perf_idle_advance(&p,70)==0 && p.now_us==70 &&
        p.thread_state==PSP_THREAD_RUNNABLE);
 assert(p.wait_signal_token==0 && psp_perf_signal_wait(&p,9)==0);
 psp_perf_wait_until(&p,90);
 assert(p.thread_state==PSP_THREAD_WAITING && p.wait_signal_token==0);
 assert(psp_perf_signal_wait(&p,9)==0);
 assert(psp_perf_idle_advance(&p,90)==0 &&
        p.thread_state==PSP_THREAD_RUNNABLE);
 assert(psp_perf_wait_signal(&p,5,90)==-1);
 assert(psp_perf_signal_wait(NULL,5)==-1);
 assert(psp_perf_signal_wait(&p,0)==-1);
 puts("PASS: token wait/signal, event delivery, timeout and invalid operations");
 return 0;
}
