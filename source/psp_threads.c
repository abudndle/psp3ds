#include "psp_threads.h"
#include <limits.h>
#include <string.h>

static PspTask *lookup(PspThreads *threads, uint32_t id) {
 if (!threads || !id) return NULL;
 for (size_t i=0;i<PSP_THREADS_MAX;i++) {
  if (threads->tasks[i].id == id && threads->tasks[i].state != PSP_TASK_EMPTY)
   return &threads->tasks[i];
 }
 return NULL;
}
const PspTask *psp_threads_get(const PspThreads *threads, uint32_t id) {
 if (!threads || !id) return NULL;
 for (size_t i=0;i<PSP_THREADS_MAX;i++) {
  if (threads->tasks[i].id == id && threads->tasks[i].state != PSP_TASK_EMPTY)
   return &threads->tasks[i];
 }
 return NULL;
}
void psp_threads_init(PspThreads *threads) {
 if (threads) { memset(threads,0,sizeof(*threads)); threads->next_id=1; }
}
static int32_t create(PspThreads *threads, uint8_t priority, PspTaskState state) {
 if (!threads || !threads->next_id || threads->next_id > INT32_MAX) return -1;
 for (size_t i=0;i<PSP_THREADS_MAX;i++) {
  PspTask *t=&threads->tasks[i];
  if (t->state != PSP_TASK_EMPTY && t->state != PSP_TASK_STOPPED) continue;
  memset(t,0,sizeof(*t));
  t->id=threads->next_id++;
  t->state=state;
  t->priority=priority;
  return (int32_t)t->id;
 }
 return -1;
}
int32_t psp_threads_create(PspThreads *threads, uint8_t priority) {
 return create(threads, priority, PSP_TASK_RUNNABLE);
}
int32_t psp_threads_create_dormant(PspThreads *threads, uint8_t priority) {
 return create(threads, priority, PSP_TASK_DORMANT);
}
int psp_threads_start(PspThreads *threads, uint32_t id) {
 PspTask *task = lookup(threads, id);
 if (!task || task->state != PSP_TASK_DORMANT) return -1;
 task->state = PSP_TASK_RUNNABLE;
 return 0;
}
int32_t psp_threads_pick(PspThreads *threads) {
 if (!threads) return -1;
 uint16_t best=UINT16_MAX;
 for (size_t i=0;i<PSP_THREADS_MAX;i++) {
  const PspTask *t=&threads->tasks[i];
  if (t->state==PSP_TASK_RUNNABLE && t->priority<best) best=t->priority;
 }
 if (best==UINT16_MAX) return 0;
 /* Round robin by monotonically increasing ID; wrap to oldest runnable. */
 uint32_t after=UINT32_MAX, oldest=UINT32_MAX;
 for (size_t i=0;i<PSP_THREADS_MAX;i++) {
  const PspTask *t=&threads->tasks[i];
  if (t->state!=PSP_TASK_RUNNABLE || t->priority!=best) continue;
  if (t->id<oldest) oldest=t->id;
  if (t->id>threads->last_selected && t->id<after) after=t->id;
 }
 threads->last_selected=after!=UINT32_MAX?after:oldest;
 return (int32_t)threads->last_selected;
}
int psp_threads_wait(PspThreads *threads, uint32_t id,
                     uint32_t token, uint64_t deadline_us) {
 PspTask *t=lookup(threads,id);
 if (!t || t->state!=PSP_TASK_RUNNABLE || deadline_us<=threads->now_us ||
     (token==0 && deadline_us==PSP_THREADS_NO_TIMEOUT)) return -1;
 t->state=PSP_TASK_WAITING;
 t->wait_token=token;
 t->deadline_us=deadline_us;
 t->wake_reason=PSP_WAKE_NONE;
 return 0;
}
int32_t psp_threads_signal(PspThreads *threads, uint32_t token) {
 if (!threads || !token) return -1;
 PspTask *chosen=NULL;
 for (size_t i=0;i<PSP_THREADS_MAX;i++) {
  PspTask *t=&threads->tasks[i];
  if (t->state!=PSP_TASK_WAITING || t->wait_token!=token) continue;
  if (!chosen || t->priority<chosen->priority ||
      (t->priority==chosen->priority && t->id<chosen->id)) chosen=t;
 }
 if (!chosen) return 0;
 chosen->state=PSP_TASK_RUNNABLE;
 chosen->wait_token=0;
 chosen->deadline_us=0;
 chosen->wake_reason=PSP_WAKE_SIGNAL;
 return (int32_t)chosen->id;
}
int psp_threads_advance(PspThreads *threads, uint64_t now_us) {
 if (!threads || now_us<threads->now_us) return -1;
 threads->now_us=now_us;
 int woke=0;
 for (size_t i=0;i<PSP_THREADS_MAX;i++) {
  PspTask *t=&threads->tasks[i];
  if (t->state!=PSP_TASK_WAITING || t->deadline_us>now_us) continue;
  t->state=PSP_TASK_RUNNABLE;
  t->wait_token=0;
  t->deadline_us=0;
  t->wake_reason=PSP_WAKE_TIMEOUT;
  woke++;
 }
 return woke;
}
int psp_threads_stop(PspThreads *threads, uint32_t id) {
 PspTask *t=lookup(threads,id);
 if (!t || t->state==PSP_TASK_STOPPED) return -1;
 t->state=PSP_TASK_STOPPED;
 t->wait_token=0;
 t->deadline_us=0;
 return 0;
}
