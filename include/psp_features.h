#ifndef PSP_FEATURES_H
#define PSP_FEATURES_H
#include "psp_platform_budget.h"
#include <stddef.h>
#include <stdint.h>
/* Every target receives exactly the same semantic feature IDs. Capability
 * states distinguish implemented synthetic diagnostics from planned PSP
 * emulation. Backend choices change implementation, never feature IDs. */
typedef enum {
 PSP_FEATURE_CPU_SMOKE=0,
 PSP_FEATURE_MIPS_BENCHMARK=1,
 PSP_FEATURE_MEMORY_POLICY=2,
 PSP_FEATURE_GUEST_DATA_BUS=3,
 PSP_FEATURE_GRAPHICS_TRANSLATION=4,
 PSP_FEATURE_PSP_GAME_BOOT=5,
 PSP_FEATURE_COUNT=6
} PspFeatureId;
typedef enum {
 PSP_FEATURE_AVAILABLE=0,    /* implemented portable diagnostic */
 PSP_FEATURE_EXPERIMENTAL=1, /* underlying module exists but is not
                               * integrated in all target runtimes */
 PSP_FEATURE_PLANNED=2
} PspFeatureState;
typedef enum {
 PSP_BACKEND_SHARED_CPU=0,
 PSP_BACKEND_INTERNAL_RAM=1,
 PSP_BACKEND_SLOT2_DATA=2,
 PSP_BACKEND_DS_2D=3,
 PSP_BACKEND_3DS_GPU=4,
 PSP_BACKEND_UNIMPLEMENTED=5
} PspFeatureBackend;
typedef struct {
 PspFeatureId id;
 const char *name;
 PspFeatureState state;
 PspFeatureBackend backend;
} PspFeatureDescriptor;
typedef struct {
 PspTarget target;
 PspPlatformBudget policy;
 PspFeatureDescriptor features[PSP_FEATURE_COUNT];
} PspFeatureManifest;
/* Initializes a COMPLETE manifest; never silently drops a feature.
 * Slot-2 backend is selected only for the positively verified Pak target. */
int psp_features_manifest(PspTarget target,PspFeatureManifest *out);
const char *psp_feature_state_name(PspFeatureState state);
/* Run the exact same synthetic MIPS tests on DS, DSi, old and New 3DS.
 * No file, kernel, graphics or commercial PSP execution is implied.
 * detail receives instructions executed for benchmark; CPU register
 * result for smoke. Outputs are reset on failure. */
int psp_feature_run(PspFeatureId id,uint32_t *detail);
#endif
