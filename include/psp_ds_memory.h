#ifndef PSP_DS_MEMORY_H
#define PSP_DS_MEMORY_H
#include "psp_platform_budget.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define PSP_DS_MIB (1024u*1024u)
#define PSP_DS_KIB 1024u
#define PSP_DS_MAIN_RAM_BYTES (4u*PSP_DS_MIB)
#define PSP_DSI_MAIN_RAM_BYTES (16u*PSP_DS_MIB)
#define PSP_DS_OFFICIAL_PAK_BYTES (8u*PSP_DS_MIB)
#define PSP_DS_MAX_SLOT2_BYTES (32u*PSP_DS_MIB)
#define PSP_DS_MEMORY_GRANULE 4096u

/* A scratch malloc() probe does not establish the actual free heap size.
 * It proves only that an allocation of at least that size succeeded and
 * had each 4-KiB page touched, after which the buffer was freed.
 * Slot-2 "verified" refers to sparse checks, not a full memory test. */
typedef struct {
 uint32_t largest_touched_heap_allocation;
 uint32_t slot2_reported_bytes;
 uint8_t slot2_sampled_verified;
 uint8_t slot2_single_bank;
 uint8_t slot2_currently_open;
} PspDsMemoryObservation;

typedef struct {
 PspTarget target;
 uint32_t installed_main_bytes;
 uint32_t reported_slot2_bytes;
 uint32_t combined_reported_bytes;
 uint32_t platform_hot_budget_bytes;
 uint32_t confirmed_temporary_heap_bytes;
 uint32_t usable_hot_bytes;
 uint32_t potential_cold_bytes;
 uint32_t active_cold_bytes;
 uint32_t native_cache_budget_bytes;
 uint32_t texture_budget_bytes;
 uint32_t io_budget_bytes;
 uint8_t slot2_sampled_verified;
 uint8_t slot2_open_for_use;
} PspDsMemoryPlan;

/* Input must be backed by actual heap and/or SDK hardware observations.
 * DS+Pak requires a verified single-bank peripheral. If it has been closed,
 * the reported cap remains informational but active_cold_bytes stays zero.
 * DS and DSi reject all Slot-2 input. No 3DS target is accepted.
 * On failure the output is zeroed and no allocation is performed. */
int psp_ds_memory_plan(PspTarget model,const PspDsMemoryObservation *observed,
                       PspDsMemoryPlan *out);

#ifdef __cplusplus
}
#endif
#endif
