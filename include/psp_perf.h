#ifndef PSP_PERF_H
#define PSP_PERF_H
#include "psp_hle.h"
#include <stddef.h>
#include <stdint.h>
#define PSP_PERF_EVENTS 16
#define PSP_PERF_HOT_SLOTS 64
#define PSP_PERF_TEXTURE_SLOTS 32
#define PSP_PERF_SECTOR_SIZE 2048
#define PSP_PERF_SECTOR_SLOTS 4
typedef enum { PSP_SUBSYSTEM_GPU=0, PSP_SUBSYSTEM_AUDIO=1, PSP_SUBSYSTEM_UMD=2, PSP_SUBSYSTEM_COUNT=3 } PspSubsystem;
typedef enum { PSP_THREAD_RUNNABLE, PSP_THREAD_WAITING } PspThreadState;
typedef struct { uint64_t deadline; uint32_t token; } PspEvent;
typedef struct {
 PspEvent pending[PSP_PERF_EVENTS]; size_t count;
 uint64_t now_us; PspThreadState thread_state; uint64_t wake_us;
 uint32_t wait_signal_token; /* zero means no signal wait */
 unsigned initialized_mask;
 unsigned active_mask; /* optional work activation, not guest execution */
 uint32_t hot_pc[PSP_PERF_HOT_SLOTS], hot_hits[PSP_PERF_HOT_SLOTS];
 uint32_t texture_key[PSP_PERF_TEXTURE_SLOTS], texture_generation[PSP_PERF_TEXTURE_SLOTS];
 uint8_t texture_valid[PSP_PERF_TEXTURE_SLOTS];
 uint32_t sector_lba[PSP_PERF_SECTOR_SLOTS], sector_age[PSP_PERF_SECTOR_SLOTS], tick;
 /* A cache is bound to the mounted media reader and its context. */
 int (*sector_reader_identity)(void *, uint32_t, uint8_t *);
 void *sector_context_identity;
 uint8_t sector_valid[PSP_PERF_SECTOR_SLOTS];
 uint8_t sector_data[PSP_PERF_SECTOR_SLOTS][PSP_PERF_SECTOR_SIZE];
 uint8_t frameskip, allow_texture_cache, profile_cpu;
} PspPerf;
typedef int (*PspSectorRead)(void *context, uint32_t lba, uint8_t *out_2048);
void psp_perf_init(PspPerf *p);
int psp_perf_schedule(PspPerf *p,uint64_t deadline,uint32_t token);
/* Advance only while the guest CPU is idle; return event token or 0. */
uint32_t psp_perf_idle_advance(PspPerf *p,uint64_t until_us);
uint32_t psp_perf_idle_hle(PspPerf *p,PspHle *hle,uint64_t until_us);
/* Cancel a queued event by token. No clock changes or callbacks. */
int psp_perf_cancel_event(PspPerf *p,uint32_t token);
/* Atomically move an existing token to a new absolute deadline. */
int psp_perf_reschedule_event(PspPerf *p,uint32_t token,uint64_t deadline);
/* One guest thread may wait for a nonzero token until an absolute deadline.
 * This does not implement PSP kernel semaphores or thread switching. */
int psp_perf_wait_signal(PspPerf *p,uint32_t token,uint64_t deadline_us);
/* Return 1 if the matching wait was woken, 0 if unmatched, -1 if invalid. */
int psp_perf_signal_wait(PspPerf *p,uint32_t token);
void psp_perf_wait_until(PspPerf *p,uint64_t wake_us);
/* Execute an instruction if runnable; otherwise return 2 for idle. */
int psp_perf_step(PspPerf *p,PspHle *hle,PspCpu *cpu);
/* Bounded instruction slice with no skipped guest semantics. */
int psp_perf_run_slice(PspPerf *p,PspHle *hle,PspCpu *cpu,unsigned budget,unsigned *executed);
/* 1 if newly initialized, 0 if previously requested, -1 if invalid. */
int psp_perf_request_subsystem(PspPerf *p,PspSubsystem subsystem);
/* Optional backends perform no work until activated; CPU/timers are unaffected. */
int psp_perf_set_subsystem_active(PspPerf *p,PspSubsystem subsystem,int active);
int psp_perf_subsystem_active(const PspPerf *p,PspSubsystem subsystem);
/* 0 = CPU slice complete, 1 = pending event delivered, 2 = waiting,
 * 3 = guest wake, negative = argument/CPU failure. Event handling remains
 * the caller's responsibility; no guest instructions are skipped when runnable. */
int psp_perf_run_demand(PspPerf *p,PspHle *hle,PspCpu *cpu,
                        unsigned budget,uint64_t idle_deadline_us,
                        unsigned *executed,uint32_t *event_token);
/* Profiling defaults off: do not pay per-instruction cost unless requested. */
void psp_perf_enable_profiling(PspPerf *p, int enabled);
/* Profile hot code without modifying guest instruction execution. */
int psp_perf_record_block(PspPerf *p,uint32_t pc,uint32_t threshold);
int psp_perf_texture_seen(PspPerf *p,uint32_t key,uint32_t generation);
void psp_perf_invalidate_textures(PspPerf *p);
/* Explicitly reset when a disc is remounted at the same context address. */
void psp_perf_media_changed(PspPerf *p);
int psp_perf_read_sector(PspPerf *p,uint32_t lba,PspSectorRead reader,void *context,uint8_t *out);
void psp_perf_profile(PspPerf *p,const char *disc_id);
#endif
