#include "psp_threads.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
 PspThreads s;
 psp_threads_init(&s);
 assert(psp_threads_pick(&s)==0);
 int32_t a=psp_threads_create(&s,10),b=psp_threads_create(&s,5);
 int32_t c=psp_threads_create(&s,5),d=psp_threads_create(&s,30);
 assert(a>0 && b>0 && c>0 && d>0);
 assert(psp_threads_pick(&s)==b);
 assert(psp_threads_pick(&s)==c);
 assert(psp_threads_pick(&s)==b);
 assert(psp_threads_wait(&s,(uint32_t)b,42,100)==0);
 assert(psp_threads_pick(&s)==c);
 assert(psp_threads_wait(&s,(uint32_t)c,42,200)==0);
 assert(psp_threads_pick(&s)==a);
 assert(psp_threads_signal(&s,999)==0);
 assert(psp_threads_signal(&s,42)==b);
 assert(psp_threads_get(&s,(uint32_t)b)->wake_reason==PSP_WAKE_SIGNAL);
 assert(psp_threads_signal(&s,42)==c);
 assert(psp_threads_signal(&s,42)==0);
 assert(psp_threads_wait(&s,(uint32_t)b,3,100)==0);
 assert(psp_threads_wait(&s,(uint32_t)c,3,100)==0);
 assert(psp_threads_advance(&s,99)==0);
 assert(psp_threads_get(&s,(uint32_t)b)->state==PSP_TASK_WAITING);
 assert(psp_threads_advance(&s,100)==2);
 assert(psp_threads_get(&s,(uint32_t)b)->wake_reason==PSP_WAKE_TIMEOUT);
 assert(psp_threads_get(&s,(uint32_t)c)->wait_token==0);
 assert(psp_threads_advance(&s,99)==-1);
 assert(s.now_us==100);
 assert(psp_threads_wait(&s,(uint32_t)a,0,101)==0);
 assert(psp_threads_advance(&s,101)==1);
 assert(psp_threads_wait(&s,(uint32_t)a,0,PSP_THREADS_NO_TIMEOUT)==-1);
 assert(psp_threads_wait(&s,(uint32_t)a,0,101)==-1);
 assert(psp_threads_wait(&s,(uint32_t)b,42,PSP_THREADS_NO_TIMEOUT)==0);
 assert(psp_threads_advance(&s,UINT64_MAX-1)==0);
 assert(psp_threads_signal(&s,42)==b);
 assert(psp_threads_stop(&s,(uint32_t)c)==0);
 assert(psp_threads_stop(&s,(uint32_t)c)==-1);
 int32_t replacement=psp_threads_create(&s,5);
 assert(replacement>c);
 assert(psp_threads_get(&s,(uint32_t)c)==NULL);
 assert(psp_threads_signal(&s,0)==-1);
 assert(psp_threads_pick(NULL)==-1);
 assert(psp_threads_create(NULL,1)==-1);
 assert(psp_threads_advance(NULL,5)==-1);
 assert(psp_threads_wait(&s,999,1,UINT64_MAX)==-1);
 PspThreads full;
 psp_threads_init(&full);
 for(int i=0;i<PSP_THREADS_MAX;i++) assert(psp_threads_create(&full,10)>0);
 assert(psp_threads_create(&full,10)==-1);
 puts("PASS: bounded task pool, deterministic priorities, RR ties, signals, timeouts, stable IDs");
 return 0;
}
