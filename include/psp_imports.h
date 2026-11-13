#ifndef PSP_IMPORTS_H
#define PSP_IMPORTS_H
#include "psp_hle.h"
#include <stddef.h>
#include <stdint.h>
/* Prototype import metadata supplied by a loader. Not a PSP PRX parser. */
typedef struct { const char *library; uint32_t nid; uint32_t stub_address; } PspImport;
typedef struct { const char *library; uint32_t nid; PspHleService service; } PspExport;
/* Patch each 8-byte guest stub (jr $ra; syscall ID) and bind its HLE service.
   Reject invalid/unsupported imports atomically, before changing guest memory. */
int psp_imports_link(PspCpu *cpu, PspHle *hle, const PspImport *imports,
                     size_t count, const PspExport *exports, size_t export_count);
#endif
