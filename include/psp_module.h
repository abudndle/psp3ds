#ifndef PSP_MODULE_H
#define PSP_MODULE_H
#include "psp_prx.h"
#include <stddef.h>
#include <stdint.h>
#define PSP_MODULE_NAME_SIZE 28
/* Loaded PSP module info, located using an ELF32 section table. */
typedef struct {
 char name[PSP_MODULE_NAME_SIZE];
 uint16_t attributes;
 uint8_t version_minor, version_major;
 uint32_t gp, export_top, export_end, import_top, import_end;
 uint32_t module_address;
} PspModuleInfo;
/* Finds .rodata.sceModuleInfo in a plain ELF32/MIPS image, then validates
 * the structure in guest memory. No relocations, encrypted EBOOTs, sectionless
 * ELF images, PRX load mapping or import patching are performed. */
int psp_module_discover(const uint8_t *elf, size_t elf_size,
                        const PspCpu *cpu, PspModuleInfo *out);
/* Extract function imports from the module's in-memory .lib.stub range. */
int psp_module_imports(const PspCpu *cpu, const PspModuleInfo *module,
                       PspPrxImports *out);
/* Discover a module in an already mapped plain ELF, parse its import tables,
 * and atomically link only the HLE services with verified PSP NIDs.
 * Requires relocation and guest mapping to have completed beforehand.
 * Unknown NIDs cause failure without modifying guest memory or HLE bindings.
 * On failure, out (if supplied) is zeroed.
 */
int psp_module_link_known(const uint8_t *elf, size_t elf_size,
                          PspCpu *cpu, PspHle *hle, PspModuleInfo *out);
/* Resolve the module section at its relocated address. */
int psp_module_link_rebased(const uint8_t *elf, size_t elf_size,
                            PspCpu *cpu, PspHle *hle, uint32_t bias,
                            PspModuleInfo *out);
#endif
