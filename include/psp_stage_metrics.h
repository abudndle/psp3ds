#ifndef PSP_STAGE_METRICS_H
#define PSP_STAGE_METRICS_H
#include "psp_platform_budget.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Separate bottleneck categories: CPU instruction execution, CPU time
 * submitting native GPU commands (NOT GPU hardware completion), real
 * storage reads, and actual audio buffer processing. Never report
 * an unmeasured subsystem as zero-cost. */
typedef enum {
 PSP_STAGE_CPU=0,
 PSP_STAGE_GPU_SUBMIT=1,
 PSP_STAGE_STORAGE=2,
 PSP_STAGE_AUDIO=3,
 PSP_STAGE_COUNT=4
} PspStageKind;
typedef struct {
 uint64_t cumulative_us;
 uint64_t transferred_bytes;
 uint32_t measured_operations;
 uint32_t errors;
 uint32_t max_us;
} PspStageCounter;
typedef struct {
 PspTarget target;
 PspStageCounter stage[PSP_STAGE_COUNT];
 uint32_t memory_samples;
 uint32_t peak_application_used_bytes;
 uint32_t smallest_application_free_bytes;
 uint32_t minimum_reserved_free_bytes;
 uint8_t memory_sampled;
} PspStageMetrics;

int psp_stage_metrics_init(PspStageMetrics *metrics,PspTarget target);
/* Times are observed by the caller's real system clock. A synthetic
 * benchmark may call this, but must label its results as synthetic.
 * Missing stage samples are returned as unmeasured, NOT zero. */
int psp_stage_metrics_record(PspStageMetrics *metrics,PspStageKind kind,
                             uint32_t elapsed_us,uint32_t bytes,int error);
int psp_stage_metrics_memory(PspStageMetrics *metrics,
                             uint32_t actual_app_used_bytes,
                             uint32_t actual_app_free_bytes,
                             uint32_t protected_reserve_bytes);
int psp_stage_metrics_average(const PspStageMetrics *metrics,
                              PspStageKind kind,uint32_t *average_us);
/* Returns 1 only if at least one genuine stage timing was recorded;
 * names are static and contain no game data. */
const char *psp_stage_metrics_name(PspStageKind kind);
#ifdef __cplusplus
}
#endif
#endif
