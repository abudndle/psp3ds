#ifndef PSP_PLAY_CUTBACKS_H
#define PSP_PLAY_CUTBACKS_H
#include "psp_memory_guard.h"
#include "psp_runtime_tuning.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Adapt render quality and FUTURE allocation admission; never skip PSP
 * interpreter instructions, audio processing, controller input or HLE time.
 * These conservative thresholds are project policy, not proven PSP FPS.
 * The current Nintendo UI scenes are synthetic, not real PSP gameplay. */
typedef enum {
 PSP_PLAY_NORMAL=0,
 PSP_PLAY_REDUCED=1,
 PSP_PLAY_EMERGENCY=2
} PspPlayQuality;
typedef struct {
 PspTarget target;
 PspPlayQuality quality;
 uint32_t target_frame_us;
 uint32_t cpu_slice_ceiling;
 uint32_t texture_soft_bytes;
 uint32_t cache_soft_bytes;
 uint32_t io_soft_bytes;
 uint16_t triangle_limit;
 uint8_t render_every_n_frames;
 uint8_t high_pressure_streak;
 uint8_t recovery_streak;
 uint32_t previous_denials;
 uint32_t previous_alloc_failures;
} PspPlayCutbacks;

int psp_play_cutbacks_init(PspPlayCutbacks *out,PspTarget target);
/* frame_us=0 and free_bytes=0 independently mean "unknown".
 * app_reserve_bytes is required when free_bytes is provided. Denied
 * allocations and allocation failures trigger immediate emergency.
 * Sustained slow frames cause cutbacks; eight healthy observations
 * permit one step of gradual recovery. Invalid samples do not mutate. */
int psp_play_cutbacks_observe(PspPlayCutbacks *c,uint32_t frame_us,
                              uint32_t free_bytes,uint32_t app_reserve_bytes,
                              const PspMemoryGuard *guard);
/* Tighten future cache/texture/IO allocations; cannot evict existing
 * buffers. The caller owns actual resource eviction and memory telemetry.
 * Guest RAM soft cap stays at the hard limit: loaded code remains valid. */
int psp_play_cutbacks_apply_guard(const PspPlayCutbacks *c,
                                   PspMemoryGuard *guard);
/* Lower the maximum subsequent interpreter slice; no guest work skipped. */
int psp_play_cutbacks_apply_tuning(const PspPlayCutbacks *c,
                                    PspRuntimeTuning *tuning);
/* Optional native PRESENTATION work reduction ONLY. Input/audio/HLE
 * must still run every tick. Frame indexes always advance. */
int psp_play_cutbacks_should_render(const PspPlayCutbacks *c,uint32_t frame);
const char *psp_play_quality_name(PspPlayQuality quality);
#ifdef __cplusplus
}
#endif
#endif
