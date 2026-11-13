#ifndef PSP_THREADS_H
#define PSP_THREADS_H
#include <stddef.h>
#include <stdint.h>

/* Portable, bounded scheduler metadata. No PSP kernel syscall or CPU context
 * switching is performed by this module. Priority 0 is highest. */
#define PSP_THREADS_MAX 8
#define PSP_THREADS_NO_TIMEOUT UINT64_MAX

typedef enum {
 PSP_TASK_EMPTY = 0,
 PSP_TASK_RUNNABLE,
 PSP_TASK_WAITING,
 PSP_TASK_DORMANT,
 PSP_TASK_STOPPED
} PspTaskState;
typedef enum {
 PSP_WAKE_NONE = 0,
 PSP_WAKE_SIGNAL,
 PSP_WAKE_TIMEOUT
} PspWakeReason;

typedef struct {
 uint32_t id;
 uint32_t wait_token; /* zero = sleeping until deadline */
 uint64_t deadline_us;
 uint8_t priority;
 PspTaskState state;
 PspWakeReason wake_reason;
} PspTask;
typedef struct {
 PspTask tasks[PSP_THREADS_MAX];
 uint64_t now_us;
 uint32_t next_id;
 uint32_t last_selected;
} PspThreads;

void psp_threads_init(PspThreads *threads);
/* Positive task ID on success; negative on invalid args/capacity. */
int32_t psp_threads_create(PspThreads *threads, uint8_t priority);
int32_t psp_threads_create_dormant(PspThreads *threads, uint8_t priority);
int psp_threads_start(PspThreads *threads, uint32_t id);
/* Select highest priority runnable task, round-robin among ties. */
int32_t psp_threads_pick(PspThreads *threads);
/* Put a task to sleep until an absolute deadline or matching token. */
int psp_threads_wait(PspThreads *threads, uint32_t id,
                     uint32_t token, uint64_t deadline_us);
/* Signal one matching waiting task: lowest priority value, then oldest ID. */
int32_t psp_threads_signal(PspThreads *threads, uint32_t token);
/* Advance scheduler clock monotonically and release timed-out waiters. */
int psp_threads_advance(PspThreads *threads, uint64_t now_us);
int psp_threads_stop(PspThreads *threads, uint32_t id);
const PspTask *psp_threads_get(const PspThreads *threads, uint32_t id);
#endif
