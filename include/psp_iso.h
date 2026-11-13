#ifndef PSP_ISO_H
#define PSP_ISO_H
#include "psp_perf.h"
#include <stdio.h>
#include <stdint.h>
/* Read 2048-byte sectors on demand; never load a whole ISO into RAM. */
typedef struct { FILE *file; uint32_t sectors; } PspIso;
int psp_iso_open(PspIso *iso,const char *path);
void psp_iso_close(PspIso *iso);
int psp_iso_read(void *context,uint32_t lba,uint8_t *out_2048);
int psp_iso_cached(PspIso *iso,PspPerf *perf,uint32_t lba,uint8_t *out_2048);
#endif
