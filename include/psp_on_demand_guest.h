#ifndef PSP_ON_DEMAND_GUEST_H
#define PSP_ON_DEMAND_GUEST_H
#include "psp_runtime_guest.h"
#include "psp_lazy_backends.h"
#include "psp_play_cutbacks.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 PspRuntimeGuestStep guest;
 uint32_t backend_completed;
 uint32_t backend_remaining;
 uint8_t backend_called;
} PspOnDemandGuestStep;

/* A single bounded scheduler tick:
 * 1. Run the shared MIPS interpreter/HLE only if its guest is runnable;
 *    while waiting, let the existing scheduler deliver pending events
 *    and advance PSP virtual time to the next event/deadline.
 * 2. Dispatch only explicitly requested optional GPU/audio/UMD units.
 *    All unfinished units remain queued; no PSP CPU/audio/timing work is
 *    dropped to increase displayed frame rate. Callback errors return -4.
 * 3. Optional cutbacks affect FUTURE CPU slice size, not guest timing.
 * This is not PPSSPP's complete game runtime; there are still missing HLE
 * functions, GE commands, VFPU and OS threading emulation. */
int psp_on_demand_guest_step(PspRuntimeTuning *tuning,
                             PspPerf *perf,PspHle *hle,PspCpu *cpu,
                             PspLazyBackends *backends,
                             const PspLazyBackendOps *ops,
                             const PspPlayCutbacks *cutbacks,
                             uint64_t idle_deadline_us,
                             PspRuntimeHostClock clock,void *context,
                             PspOnDemandGuestStep *out);
#ifdef __cplusplus
}
#endif
#endif
