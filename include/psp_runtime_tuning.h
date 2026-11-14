#ifndef PSP_RUNTIME_TUNING_H
#define PSP_RUNTIME_TUNING_H
#include <stdint.h>
#include "psp_platform_budget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Software work budgets, not estimated PSP game speed or detected free RAM.
 * The New 3DS policy takes advantage of the higher-clock ARM11 while the
 * old 3DS, DS/DS+Slot-2, and DSi retain conservative bounded workloads. */
typedef struct {
 PspTarget target;
 uint32_t cpu_slice_initial;
 uint32_t cpu_slice_current;
 uint32_t cpu_slice_max;
 uint32_t cpu_window_us;
 uint32_t texture_decode_staging_bytes;
 uint32_t io_staging_bytes;
 uint16_t triangle_demo_budget;
 uint8_t benchmark_rounds;
 uint8_t request_new3ds_speedup;
} PspRuntimeTuning;

int psp_runtime_tuning_choose(PspTarget target,PspRuntimeTuning *out);

/* A successful, fully completed instruction slice may adjust the NEXT
 * slice based on measured time. Never skips guest work or adjusts PSP
 * virtual time, and never assumes faster hardware implies game FPS.
 * Return -1 for invalid samples without mutating the tuning profile.
 * Return 0 if unchanged, 1 if changed. */
int psp_runtime_tuning_feedback(PspRuntimeTuning *t,
                                uint32_t completed_instructions,
                                uint32_t elapsed_us);
#ifdef __cplusplus
}
#endif
#endif
