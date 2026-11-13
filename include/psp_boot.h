#ifndef PSP_BOOT_H
#define PSP_BOOT_H
#include "psp_loader.h"
#include "psp_module.h"
#include "psp_reloc_image.h"

/* Load a plain, unencrypted ET_EXEC MIPS ELF into an existing guest memory
 * mapping, discover its PSP module info and link only supported HLE imports.
 * No guest instruction may run until PSP_BOOT_OK is returned.
 * On failure CPU registers and 'out' remain unchanged/zero respectively,
 * and HLE bindings stay unchanged if import validation failed. Guest RAM
 * may have been modified by the ELF segment loader: caller must discard it.
 * No PRX relocations, encrypted EBOOT, or firmware loader are implemented.
 */
typedef enum {
 PSP_BOOT_OK = 0,
 PSP_BOOT_BAD_ARGUMENT = -1,
 PSP_BOOT_ELF_FAILED = -2,
 PSP_BOOT_IMPORT_FAILED = -3,
 PSP_BOOT_RELOCATION_FAILED = -4,
 PSP_BOOT_NO_MEMORY = -5,
 PSP_BOOT_MAPPING_FAILED = -6
} PspBootStatus;
int psp_boot_plain_elf(const uint8_t *elf, size_t elf_size,
                       PspCpu *cpu, PspHle *hle,
                       PspLoadResult *loaded, PspModuleInfo *module);
/* Complete a restricted ELF32/MIPS boot from an ALREADY-MAPPED module:
 * discover/apply supported PSPREL streams, resolve module metadata and
 * verified HLE imports, then initialize the guest CPU at the mapped entry.
 * On any error, guest RAM, CPU and HLE are identical to their original state,
 * and outputs are zero. Input ELF and RAM must be separate buffers.
 *
 * This does NOT allocate/map PT_LOAD sections or decrypt PSP executables.
 * Section addresses in the ELF must already match the mapped metadata
 * addresses. Supports <= 32 MiB mapped RAM and <= 128 program headers. */
int psp_boot_mapped_relocated_elf(const uint8_t *elf, size_t elf_size,
                                  PspCpu *cpu, PspHle *hle,
                                  const PspRelocSegment *segments,
                                  size_t segment_count,
                                  PspModuleInfo *module,
                                  size_t *relocation_streams);
/* Derive PT_LOAD placements directly from a plain ELF32/MIPS ET_EXEC.
 * The entry's file-backed executable segment must remain linked at its
 * original guest virtual address. Other PT_LOADs use a nonzero p_paddr
 * placement hint, their original vaddr if free, or first available aligned
 * space after prior load segments. All placements must fit existing RAM.
 * This maps, relocates, links imports and initializes CPU transactionally:
 * on failure guest RAM, CPU and HLE stay unchanged; outputs are zero.
 *
 * Not a general PSP ET_REL or ET_SCE_PRX loader: relocatable entry
 * rebasing, section-table rebasing, encrypted EBOOT and firmware setup
 * are explicitly unsupported. Requires at least one PSP relocation stream.
 */
int psp_boot_auto_mapped_elf(const uint8_t *elf, size_t elf_size,
                             PspCpu *cpu, PspHle *hle,
                             PspModuleInfo *module,
                             size_t *relocation_streams);
#endif
