#ifndef PSP_GUARDED_PRX_H
#define PSP_GUARDED_PRX_H
#include "psp_memory_guard.h"
#include "psp_prx_image.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define PSP_GUARDED_PRX_IMAGE_LIMIT (32u*1024u*1024u)
typedef struct {
 PspCpu cpu;
 PspPrxImageResult image;
 uint8_t *guest_ram;
 uint32_t allocated_bytes;
} PspGuardedPrx;

/* Exclusively owned PSP guest image memory is charged to the GUEST class.
 * The input ELF is caller-owned. The loader does not run any game code,
 * resolve HLE imports, or account for subsequent GPU/OS system allocations.
 * The output must be zero-initialized (or previously unloaded).
 * Abort on any error without leaking the temporary guest buffer. */
int psp_guarded_prx_load(PspMemoryGuard *guard,const uint8_t *elf,
                         size_t elf_size,uint32_t guest_base,
                         PspGuardedPrx *out);
/* Reserve up to 2 MiB of additional zeroed guest RAM ABOVE the mapped
 * PSP image for diagnostic stack/memory. Charged to the same protected
 * GUEST budget. This is not the complete PSP kernel's heap allocator. */
int psp_guarded_prx_load_with_reserve(PspMemoryGuard *guard,
                                      const uint8_t *elf,size_t elf_size,
                                      uint32_t guest_base,
                                      uint32_t extra_guest_bytes,
                                      PspGuardedPrx *out);
int psp_guarded_prx_unload(PspMemoryGuard *guard,PspGuardedPrx *image);
#ifdef __cplusplus
}
#endif
#endif
