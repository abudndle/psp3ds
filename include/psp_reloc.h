#ifndef PSP_RELOC_H
#define PSP_RELOC_H
#include "psp_core.h"
#include <stddef.h>
#include <stdint.h>
#define PSP_RELOC_MAX_RECORDS 131072u
/* One mapped PT_LOAD segment. original_vaddr is the link-time address;
 * mapped_vaddr is its location in the emulator guest address space. */
typedef struct {
 uint32_t original_vaddr;
 uint32_t mapped_vaddr;
 uint32_t mem_size;
} PspRelocSegment;
/* Apply uncompressed PSP SHT_PRXRELOC / PT_PRXRELOC entries (8 bytes each).
 * Supports R_MIPS_NONE(0), R_MIPS_32(2), R_MIPS_26(4),
 * R_MIPS_HI16(5), and R_MIPS_LO16(6). HI16 must be paired with a
 * subsequent LO16 using the same address-base selector; multiple HI16
 * records may share a LO16. LO16-only immediate relocation is accepted.
 * Unsupported types / malformed input leave guest memory unchanged.
 * Returns 0 on success, negative on failure. Does not load PRX files. */
int psp_reloc_apply(PspCpu *cpu, const uint8_t *records, size_t record_len,
                    const PspRelocSegment *segments, size_t segment_count);
#endif
