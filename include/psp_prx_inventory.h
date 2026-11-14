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

#ifdef __cplusplus
}
#endif
#endif
