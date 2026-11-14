#ifndef PSP_RUNTIME_GUEST_H
#define PSP_RUNTIME_GUEST_H
#include "psp_runtime_tuning.h"
#include "psp_perf.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Monotonic host wall clock in microseconds. Never used as PSP virtual time.
 * A NULL clock runs the same guest work without modifying the slice budget. */
typedef uint64_t (*PspRuntimeHostClock)(void *context);

typedef struct {
 uint32_t requested;
 uint32_t executed;
 uint32_t next_budget;
 uint32_t event_token;
 uint32_t wall_elapsed_us;
 uint8_t feedback_applied;
} PspRuntimeGuestStep;

/* Invoke the *real shared* MIPS interpreter + HLE scheduler using a per-model
 * work quota. The perf scheduler owns guest waiting, idle HLE time, and events.
 * Returns psp_perf_run_demand's status: 0 runnable work, 1 event,
 * 2 waiting, 3 wake, negative error. Result is cleared on invalid input.
 * Only a full successful CPU slice may tune future work, using an optional
 * monotonic wall-clock sample. Never skips MIPS instructions or guest events.
 */
int psp_runtime_guest_step(PspRuntimeTuning *tuning, PspPerf *perf,
                           PspHle *hle, PspCpu *cpu,
                           uint64_t idle_deadline_us,
                           PspRuntimeHostClock clock_us,void *clock_context,
                           PspRuntimeGuestStep *out);
#ifdef __cplusplus
}
#endif
#endif
