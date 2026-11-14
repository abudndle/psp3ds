#ifndef PSP_LAZY_BACKENDS_H
#define PSP_LAZY_BACKENDS_H
#include "psp_platform_budget.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Only requested backend work is submitted. No guest CPU/HLE/audio ticks are
 * discarded. These are bounded *units*, whose real meaning must be defined
 * by the future GPU/audio/UMD frontend. Optional work is not fabricated. */
#define PSP_LAZY_MAX_QUEUED 64u
#define PSP_LAZY_SUBSYSTEMS 3u

typedef enum {
 PSP_LAZY_GPU=0, PSP_LAZY_AUDIO=1, PSP_LAZY_UMD=2
} PspLazySubsystem;

typedef struct {
 /* start() lazily opens a resource. On failure it must clean partial state.
  * work() returns 0 ONLY when exactly one requested unit was performed;
  * any failure keeps the unit queued for retry. stop() releases resources. */
 int (*start)(void *context,PspLazySubsystem subsystem);
 int (*work)(void *context,PspLazySubsystem subsystem);
 void (*stop)(void *context,PspLazySubsystem subsystem);
 void *context;
} PspLazyBackendOps;

typedef struct {
 PspTarget target;
 uint8_t initialized;
 uint8_t units_per_pump;
 uint8_t next_service;
 uint8_t started[PSP_LAZY_SUBSYSTEMS];
 uint16_t pending[PSP_LAZY_SUBSYSTEMS];
 uint32_t completed[PSP_LAZY_SUBSYSTEMS];
 uint32_t idle_pumps;
 uint32_t failed_calls;
 uint32_t initialized_count;
} PspLazyBackends;

typedef struct {
 uint32_t completed_units;
 uint32_t pending_units;
 uint32_t initialized_services;
 uint32_t leftover_cancelled_on_shutdown;
} PspLazyShutdown;

int psp_lazy_backends_init(PspLazyBackends *backends,PspTarget target);
int psp_lazy_backends_request(PspLazyBackends *backends,
                              PspLazySubsystem subsystem,unsigned units);
int psp_lazy_backends_cancel(PspLazyBackends *backends,
                             PspLazySubsystem subsystem,unsigned units);
/* Return 0 when idle; 1 when >=1 unit completed; -2 on backend failure,
 * leaving failed work queued (and previously completed work committed).
 * No callback is made when all queues are empty. Fair round-robin order
 * across GPU/AUDIO/UMD; bounded work per pump for every device profile. */
int psp_lazy_backends_pump(PspLazyBackends *backends,
                           const PspLazyBackendOps *ops,unsigned *completed);
/* Close the session explicitly: stop each started subsystem once, clear
 * queued requests (only at explicit teardown), and zero the entire ledger.
 * Resource owners must implement a correct stop callback; there is no
 * tracking of allocations they perform outside the session memory guard. */
int psp_lazy_backends_close(PspLazyBackends *backends,
                            const PspLazyBackendOps *ops,
                            PspLazyShutdown *report);
#ifdef __cplusplus
}
#endif
#endif
