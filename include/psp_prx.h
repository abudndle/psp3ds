#ifndef PSP_PRX_H
#define PSP_PRX_H
#include "psp_imports.h"
#define PSP_PRX_MAX_NAME 64u
#define PSP_PRX_MAX_IMPORTS 256u
typedef struct {
 PspImport imports[PSP_PRX_MAX_IMPORTS];
 char names[PSP_PRX_MAX_IMPORTS][PSP_PRX_MAX_NAME];
 size_t count;
} PspPrxImports;
/* Parse a range of already-loaded little-endian PSP .lib.stub entries.
   Table addresses are PSP guest virtual addresses, not offsets in a file.
   Supports 20-byte and 24-byte function-import entries. Variable imports
   and PSP executable/PRX relocation and decryption are not supported.
   On error, output is zeroed and no guest memory is changed. */
int psp_prx_parse_imports(const PspCpu *cpu, uint32_t start,
                          uint32_t end, PspPrxImports *out);
#endif
