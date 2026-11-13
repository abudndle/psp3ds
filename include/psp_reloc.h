#ifndef PSP_RELOC_H
#define PSP_RELOC_H
#include "psp_core.h"
#include <stddef.h>
#include <stdint.h>
/* One mapped PT_LOAD segment. original_vaddr is the link-time address;
 * mapped_vaddr is its location in the emulator guest address space. */
typedef struct {
 uint32_t original_vaddr;
 uint32_t mapped_vaddr;
 uint32_t mem_size;
} PspRelocSegment;
/* Apply uncompressed PSP SHT_PRXRELOC / PT_PRXRELOC entries (8 bytes each).
 * Supports R_MIPS_NONE(0), R_MIPS_32(2), R_MIPS_26(4).
 * Unsupported types / malformed input leave guest memory unchanged.
 * Returns 0 on success, negative on failure. Does not load PRX files. */
int psp_reloc_apply(PspCpu *cpu, const uint8_t *records, size_t record_len,
                    const PspRelocSegment *segments, size_t segment_count);
#endif
