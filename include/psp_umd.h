#ifndef PSP_UMD_H
#define PSP_UMD_H
#include "psp_iso.h"
#include "psp_cso.h"
#include "psp_disc.h"
typedef enum { PSP_UMD_NONE=0, PSP_UMD_ISO=1, PSP_UMD_CSO=2 } PspUmdKind;
typedef struct {
 PspUmdKind kind;
 uint32_t generation; /* increments on every media unmount/remount */
 PspIso iso;
 PspCso cso;
 PspPerf cache;
} PspUmd;
void psp_umd_init(PspUmd *device);
void psp_umd_unmount(PspUmd *device);
int psp_umd_mount(PspUmd *device, const char *path);
int psp_umd_sector(PspUmd *device, uint32_t lba, uint8_t *out_2048);
int psp_umd_find(PspUmd *device,const char *path,PspDiscEntry *entry);
int psp_umd_read(PspUmd *device,PspDiscEntry entry,uint32_t offset,void *dst,size_t length);
#endif
