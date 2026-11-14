#ifndef PSP_PRX_INVENTORY_H
#define PSP_PRX_INVENTORY_H

#include "psp_prx.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Read-only inventory for PSP modules larger than the prototype's 64-HLE
 * binding capacity. Import inventory is NOT firmware service support. */
#define PSP_PRX_INVENTORY_MAX_LIBRARIES 64u
#define PSP_PRX_INVENTORY_MAX_FUNCTIONS 1024u

typedef struct {
 char name[PSP_PRX_MAX_NAME];
 uint32_t nid_table;
 uint32_t stub_table;
 uint32_t first_nid;
 uint16_t function_count;
} PspPrxLibraryInventory;

typedef struct {
 PspPrxLibraryInventory libraries[PSP_PRX_INVENTORY_MAX_LIBRARIES];
 uint16_t library_count;
 uint16_t function_count;
} PspPrxImportInventory;

/* Validate a PSP guest import table and count every function/NID without
 * patching RAM or registering HLE bindings. Handles 20- and 24-byte stubs;
 * malformed entries, variables and overlong names fail atomically, leaving
 * output zero. Does not decrypt, relocate, or execute guest binaries.
 * NIDs remain in guest memory and can be inspected via validated addresses.
 */
int psp_prx_import_inventory(const PspCpu *cpu,uint32_t top,uint32_t end,
                             PspPrxImportInventory *out);

/* Read-only capability audit against the HLE functions actually implemented
 * by psp_nids.c. Unknown imports remain unsupported; never patch guest RAM.
 * Revalidates the NID memory spans and library names before reading.
 * On error output is zero. */
typedef struct {
 uint16_t implemented;
 uint16_t unsupported;
} PspPrxHleCoverage;
int psp_prx_import_coverage(const PspCpu *cpu,
                            const PspPrxImportInventory *inventory,
                            PspPrxHleCoverage *out);

/* Link ONLY already-implemented HLE functions found in a large PRX import
 * table (e.g. 145 functions, where <64 have verified handlers). Missing
 * imports remain unmodified and must stop the startup trace when reached.
 * Failed validation/linking changes neither bindings nor guest memory.
 * A successful result reports verified imports patched, not total support.
 * Call only for an owned, mutable and not-yet-running guest image. */
int psp_prx_link_verified_imports(PspCpu *cpu,PspHle *hle,
                                  const PspPrxImportInventory *inventory,
                                  PspPrxHleCoverage *coverage);

#ifdef __cplusplus
}
#endif
#endif
