#ifndef PSP_3DS_MEMORY_H
#define PSP_3DS_MEMORY_H
#include "psp_platform_budget.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_3DS_MIB (1024u*1024u)
#define PSP_3DS_OLD_PHYSICAL (128u*PSP_3DS_MIB)
#define PSP_3DS_NEW_PHYSICAL (256u*PSP_3DS_MIB)
#define PSP_3DS_OLD_STANDARD_APP (64u*PSP_3DS_MIB)
#define PSP_3DS_NEW_STANDARD_APP (124u*PSP_3DS_MIB)
/* Exceptional application modes require launch permissions. Neither
 * special configuration is available to ordinary homebrew by default. */
#define PSP_3DS_OLD_MAX_SPECIAL_APP (96u*PSP_3DS_MIB)
#define PSP_3DS_NEW_MAX_SPECIAL_APP (178u*PSP_3DS_MIB)
#define PSP_3DS_OLD_PSP_GUEST_CAP (32u*PSP_3DS_MIB)
#define PSP_3DS_NEW_PSP_GUEST_CAP (64u*PSP_3DS_MIB)
#define PSP_3DS_MIN_GUEST (8u*PSP_3DS_MIB)
#define PSP_3DS_NEW_HEADROOM (24u*PSP_3DS_MIB)
#define PSP_3DS_OLD_HEADROOM (16u*PSP_3DS_MIB)

/* Physical FCRAM is partitioned among APPLICATION, SYSTEM, and BASE. Only
 * a kernel-granted application region is relevant to app allocations.
 * Region free bytes are not a promise malloc() can obtain one large block.
 * Free and region size MUST come from the live libctru memory query.
 * This policy never assumes New 3DS advanced/exheader memory modes. */
typedef struct {
 PspTarget target;
 uint32_t installed_ram_bytes;
 uint32_t typical_application_bytes;
 uint32_t special_application_upper_bytes;
 uint32_t observed_application_bytes;
 uint32_t observed_free_bytes;
 uint32_t preserve_for_system_bytes;
 uint32_t permitted_guest_bytes;
 uint32_t recommended_guest_bytes;
 uint32_t potential_extra_cache_bytes;
 uint8_t full_psp_guest_fits;
 uint8_t jetpack_prx_fits;
} Psp3dsMemoryPlan;

/* Return -1 and zero output for invalid/impossible observations.
 * Returns 0 even when memory is inadequate: guest/caches then remain zero.
 * PSP guest RAM target is at most 32 MiB for old and 64 MiB for new
 * (separate from additional PSP GPU buffers, audio and native OS memory).
 * The old3DS value does not implicitly increase on a New model running
 * legacy 3DS application memory mode. */
int psp_3ds_memory_plan(PspTarget model,uint32_t app_region,
                        uint32_t app_free,Psp3dsMemoryPlan *out);

#ifdef __cplusplus
}
#endif
#endif
