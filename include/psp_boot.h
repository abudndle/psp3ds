#ifndef PSP_BOOT_H
#define PSP_BOOT_H
#include "psp_loader.h"
#include "psp_module.h"

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
 PSP_BOOT_IMPORT_FAILED = -3
} PspBootStatus;
int psp_boot_plain_elf(const uint8_t *elf, size_t elf_size,
                       PspCpu *cpu, PspHle *hle,
                       PspLoadResult *loaded, PspModuleInfo *module);
#endif
