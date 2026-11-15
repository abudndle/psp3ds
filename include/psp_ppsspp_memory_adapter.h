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
int psp_ppsspp_translate_address(uint32_t guest_address, uint32_t *physical_address);
int psp_ppsspp_adapter_fetch32(const PspGuestBus *bus, uint32_t pc, uint32_t *instruction);
int psp_ppsspp_adapter_read32(const PspGuestBus *bus, uint32_t address, uint32_t *value);
int psp_ppsspp_adapter_write32(const PspGuestBus *bus, uint32_t address, uint32_t value);

#ifdef __cplusplus
}
#endif
#endif
