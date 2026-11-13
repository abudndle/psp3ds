#ifndef PSP_RELOC_IMAGE_H
#define PSP_RELOC_IMAGE_H
#include "psp_reloc2.h"

/* Apply supported PSP relocation streams discovered from an ELF32/MIPS
 * program header table to guest RAM which has ALREADY been mapped.
 *
 * The segment array has one entry for each ELF program header, in the same
 * order. PT_LOAD slots must describe the original ELF addresses and mapped
 * RAM locations; unused slots may be zero. Supports PT_SCE_PSPREL
 * (0x700000A0, 8-byte records) and PT_SCE_PSPREL2 (0x700000A1, bounded
 * compressed subset). All streams are atomic as one batch. No guest CPU
 * registers are modified. This is not a PRX loader or firmware emulation.
 *
 * Returns 0 on success; negative on malformed image or unsupported streams.
 * out_streams, if non-NULL, is set to zero on failure.
 */
int psp_reloc_image_apply(const uint8_t *elf, size_t elf_size, PspCpu *cpu,
                          const PspRelocSegment *segments, size_t segment_count,
                          size_t *out_streams);
#endif
