#ifndef PSP_PRX_IMAGE_H
#define PSP_PRX_IMAGE_H
#include "psp_reloc_image.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An unencrypted ET_SCE_PRX (0xFFA0) is link-relative rather than a normal
 * ET_EXEC image. Mapping/relocations are distinct from PSP firmware boot.
 * This loader handles load segments and the already-supported PSPREL streams.
 * It does NOT resolve PSP module imports, initialize thread/GE state, or
 * execute a commercial game. No decrypted game content belongs in the repo. */
typedef struct {
 uint32_t entry;
 uint32_t image_bytes;
 uint16_t load_segments;
 uint16_t relocation_streams;
} PspPrxImageResult;

typedef enum {
 PSP_PRX_IMAGE_OK=0,
 PSP_PRX_IMAGE_ARGUMENT=-1,
 PSP_PRX_IMAGE_FORMAT=-2,
 PSP_PRX_IMAGE_MAPPING=-3,
 PSP_PRX_IMAGE_NO_MEMORY=-4,
 PSP_PRX_IMAGE_RELOCATION=-5
} PspPrxImageStatus;

/* Map PT_LOAD segments at (cpu->ram_base + original relative vaddr),
 * filling BSS and applying all PT_SCE_PSPREL[2] relocations before setting
 * the entry PC. Supports up to 32 MiB mapped RAM and 128 program headers.
 * The source ELF and guest RAM must not overlap.
 * On ANY failure guest RAM, CPU registers and output are unchanged/zero.
 * Caller chooses guest memory and reserves enough address space; in
 * particular 4-MB DS RAM cannot map many larger PSP titles. */
int psp_prx_image_map_relocated(const uint8_t *elf,size_t elf_size,
                                PspCpu *cpu,PspPrxImageResult *out);

#ifdef __cplusplus
}
#endif
#endif
