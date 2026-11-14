#ifndef PSP_ROM_PAGER_H
#define PSP_ROM_PAGER_H
#include "psp_guest_window.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* ROM stores immutable file/archive data, NOT writable PSP guest RAM.
 * 4-KiB hot pages are caller-owned internal RAM; an optional, already
 * verified AND CURRENTLY OPEN Slot-2 window stores additional cold pages.
 * The callback can read NitroFS, a file, or a synthetic fixture.
 * No heap, ROM-resident executable mapping, decompression, or PSP CPU hookup.
 */
#define PSP_ROM_PAGER_PAGE_BYTES 4096u
#define PSP_ROM_PAGER_MAX_HOT 4u
#define PSP_ROM_PAGER_MAX_COLD 8u
#define PSP_ROM_PAGER_MAX_READ 32u
#define PSP_ROM_PAGER_MAX_IMAGE (512u*1024u*1024u)

typedef int (*PspRomPageRead)(void *opaque,uint32_t file_offset,
                               uint8_t *destination,size_t length);
typedef struct {
 uint32_t hot_hits,cold_hits,source_misses,source_reads;
} PspRomPagerStats;
typedef struct {
 uint8_t *hot_bytes;
 uint32_t image_bytes;
 uint32_t clock;
 uint32_t hot_tags[PSP_ROM_PAGER_MAX_HOT];
 uint32_t hot_ages[PSP_ROM_PAGER_MAX_HOT];
 uint8_t hot_valid[PSP_ROM_PAGER_MAX_HOT];
 uint8_t hot_count;
 uint8_t cold_count;
 uint32_t cold_tags[PSP_ROM_PAGER_MAX_COLD];
 uint32_t cold_ages[PSP_ROM_PAGER_MAX_COLD];
 uint8_t cold_valid[PSP_ROM_PAGER_MAX_COLD];
 PspGuestWindow cold_window;
 PspRomPageRead read_source;
 void *opaque;
 PspRomPagerStats stats;
} PspRomPager;

/* cold_window is NULL unless the caller currently holds an open and
 * verified Slot-2 RAM session; use cold_count=0 when not present.
 * hot_storage must accommodate hot_count*4096 writable bytes and must
 * remain valid throughout the pager's lifetime. */
int psp_rom_pager_init(PspRomPager *pager,
                       uint8_t *hot_storage,unsigned hot_count,
                       uint32_t image_bytes,PspRomPageRead read_source,
                       void *opaque,const PspGuestWindow *cold_window,
                       unsigned cold_count);

/* Atomic output: failed source/cold reads leave destination unchanged.
 * The cache itself may evict a page on a failed callback.
 * Bounded reads (1..32 bytes) support aligned/unaligned and cross-page
 * ROM accesses. Guest writes are intentionally not supported. */
int psp_rom_pager_read(PspRomPager *pager,uint32_t offset,
                        uint8_t *destination,size_t count);
int psp_rom_pager_read32(PspRomPager *pager,uint32_t offset,uint32_t *out);
#ifdef __cplusplus
}
#endif
#endif
