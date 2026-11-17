#ifndef PSP_PPSSPP_MEMORY_ADAPTER_H
#define PSP_PPSSPP_MEMORY_ADAPTER_H

#include "psp_ppsspp_bus_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PSP CPU physical aliases: cached 0x8xxxxxxx and uncached 0xAxxxxxxx.
 * This is a translation primitive, not a replacement for PPSSPP MemMap.
 * Unmapped addresses must be rejected by the checked bus.
 */
/* Deterministic access validation for a future PPSSPP exception bridge.
 * Validation does not perform the access or raise a CPU exception. */
typedef enum {
    PSP_PPSSPP_ACCESS_OK = 0,
    PSP_PPSSPP_ACCESS_UNMAPPED = 1,
    PSP_PPSSPP_ACCESS_UNALIGNED = 2,
    PSP_PPSSPP_ACCESS_NONEXECUTABLE = 3
} PspPpssppAccessResult;
PspPpssppAccessResult psp_ppsspp_check_access(
    const PspGuestBus *bus, uint32_t address, unsigned bytes, int instruction_fetch);

/* Status-returning entry points for future interpreter exception mapping.
 * They do not raise MIPS exceptions; output is unchanged on failure. */
PspPpssppAccessResult psp_ppsspp_checked_fetch32(
    const PspGuestBus *bus, uint32_t pc, uint32_t *instruction);
PspPpssppAccessResult psp_ppsspp_checked_read32(
    const PspGuestBus *bus, uint32_t address, uint32_t *value);
PspPpssppAccessResult psp_ppsspp_checked_write32(
    const PspGuestBus *bus, uint32_t address, uint32_t value);
PspPpssppAccessResult psp_ppsspp_checked_read8(const PspGuestBus *, uint32_t, uint8_t *);
PspPpssppAccessResult psp_ppsspp_checked_read16(const PspGuestBus *, uint32_t, uint16_t *);
PspPpssppAccessResult psp_ppsspp_checked_read64(const PspGuestBus *, uint32_t, uint64_t *);
PspPpssppAccessResult psp_ppsspp_checked_write8(const PspGuestBus *, uint32_t, uint8_t);
PspPpssppAccessResult psp_ppsspp_checked_write16(const PspGuestBus *, uint32_t, uint16_t);
PspPpssppAccessResult psp_ppsspp_checked_write64(const PspGuestBus *, uint32_t, uint64_t);
int psp_ppsspp_translate_address(uint32_t guest_address, uint32_t *physical_address);
int psp_ppsspp_adapter_fetch32(const PspGuestBus *bus, uint32_t pc, uint32_t *instruction);
int psp_ppsspp_adapter_read8(const PspGuestBus *bus, uint32_t address, uint8_t *value);
int psp_ppsspp_adapter_read16(const PspGuestBus *bus, uint32_t address, uint16_t *value);
int psp_ppsspp_adapter_read64(const PspGuestBus *bus, uint32_t address, uint64_t *value);
int psp_ppsspp_adapter_write8(const PspGuestBus *bus, uint32_t address, uint8_t value);
int psp_ppsspp_adapter_write16(const PspGuestBus *bus, uint32_t address, uint16_t value);
int psp_ppsspp_adapter_write64(const PspGuestBus *bus, uint32_t address, uint64_t value);
int psp_ppsspp_adapter_read32(const PspGuestBus *bus, uint32_t address, uint32_t *value);
int psp_ppsspp_adapter_write32(const PspGuestBus *bus, uint32_t address, uint32_t value);

#ifdef __cplusplus
}
#endif
#endif
