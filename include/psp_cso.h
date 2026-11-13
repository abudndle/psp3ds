#ifndef PSP_CSO_H
#define PSP_CSO_H
#include "psp_perf.h"
#include <stdint.h>
#include <stdio.h>
/* CSO v1, 2048-byte sectors, raw DEFLATE. No full-file allocation. */
typedef struct {
 FILE *file;
 uint32_t sectors;
 uint8_t alignment;
 uint64_t file_size;
 uint64_t index_end;
} PspCso;
int psp_cso_open(PspCso *cso, const char *path);
void psp_cso_close(PspCso *cso);
/* Sector callback contract: 0 success, negative error. */
int psp_cso_read(void *ctx, uint32_t lba, uint8_t *out_2048);
/* 0 cache miss filled, 1 cache hit, negative error. */
int psp_cso_cached(PspCso *cso, PspPerf *perf, uint32_t lba, uint8_t *out_2048);
#endif
