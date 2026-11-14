#ifndef PSP_ROM_FILE_H
#define PSP_ROM_FILE_H
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Use a caller-owned FILE* from NitroFS or any opened read-only file.
 * Suitable for PSP data/assets stored in Nintendo cartridge ROM files;
 * the PSP ROM pager controls 4-KiB page caching and bounds.
 * Does not mount NitroFS or make a PSP executable runnable on DS. */
int psp_rom_file_read(void *opaque,uint32_t file_offset,
                      uint8_t *destination,size_t count);
#ifdef __cplusplus
}
#endif
#endif
