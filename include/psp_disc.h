#ifndef PSP_DISC_H
#define PSP_DISC_H
#include "psp_perf.h"
#include <stddef.h>
#include <stdint.h>
/* Read ISO9660 level-1 file metadata from an ISO or CSO sector provider.
   Strict bounds, no Joliet/Rock Ridge/ISO multi-extent support. */
typedef struct { uint32_t lba; uint32_t size; uint8_t is_dir; } PspDiscEntry;
int psp_disc_find(PspSectorRead reader, void *context,
                  const char *absolute_path, PspDiscEntry *entry);
int psp_disc_read(PspSectorRead reader, void *context,
                  PspDiscEntry entry, uint32_t offset, void *dst, size_t length);
#endif
