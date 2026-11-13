#include "psp_boot.h"
#include <string.h>

int psp_boot_plain_elf(const uint8_t *elf, size_t elf_size,
                       PspCpu *cpu, PspHle *hle,
                       PspLoadResult *loaded, PspModuleInfo *module) {
 if (loaded) memset(loaded, 0, sizeof(*loaded));
 if (module) memset(module, 0, sizeof(*module));
 if (!elf || !cpu || !hle || !cpu->ram || !loaded || !module ||
     !cpu->ram_size) return PSP_BOOT_BAD_ARGUMENT;
 PspLoadResult staged;
 if (psp_elf_load(elf, elf_size, cpu->ram, cpu->ram_size,
                  cpu->ram_base, &staged) != PSP_LOAD_OK)
  return PSP_BOOT_ELF_FAILED;
 /* Keep CPU registers unchanged until all imports have been validated. */
 PspCpu candidate = *cpu;
 psp_cpu_init(&candidate, cpu->ram, cpu->ram_size, cpu->ram_base,
              staged.entry);
 PspModuleInfo info;
 if (psp_module_link_known(elf, elf_size, &candidate, hle, &info) != 0)
  return PSP_BOOT_IMPORT_FAILED;
 *cpu = candidate;
 *loaded = staged;
 *module = info;
 return PSP_BOOT_OK;
}
