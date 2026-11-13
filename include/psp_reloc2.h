#ifndef PSP_RELOC2_H
#define PSP_RELOC2_H
#include "psp_reloc.h"
#include <stddef.h>
#include <stdint.h>
/* Decode a deliberately limited PSP PT_SCE_PSPREL2 (0x700000A1) stream.
 * Supports base-setting commands and relocation commands with offset modes
 * 0/2/4, no LO16 sidecar, and mapped relocation types NONE, 32, 26.
 * Unsupported encodings fail without modifying guest memory.
 * rel_segment_index is the program header index used to size the segment field.
 */
int psp_reloc2_apply(PspCpu *cpu, const uint8_t *bytes, size_t length,
                     const PspRelocSegment *segments, size_t segment_count,
                     unsigned rel_segment_index);
 int psp_reloc2_apply_with_base_mode(PspCpu *cpu, const uint8_t *bytes, size_t length,
                                     const PspRelocSegment *segments, size_t count,
                                     unsigned rel_segment_index, int segment_relative);
#endif
