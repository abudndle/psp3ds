#ifndef PSP_PPSSPP_BUS_BRIDGE_H
#define PSP_PPSSPP_BUS_BRIDGE_H

#include "psp_guest_bus.h"
#include <stdint.h>

/* Checked adapter primitives for a future PPSSPP Memory:: backend.
 * This deliberately does not claim to implement PPSSPP's Memory namespace.
 * Never return a fabricated zero instruction on an unmapped fetch.
 */
#ifdef __cplusplus
extern "C" {
#endif

int psp_ppsspp_bus_fetch32(const PspGuestBus *bus, uint32_t pc, uint32_t *instruction);
int psp_ppsspp_bus_load8(const PspGuestBus *bus, uint32_t address, uint8_t *value);
int psp_ppsspp_bus_load16(const PspGuestBus *bus, uint32_t address, uint16_t *value);
int psp_ppsspp_bus_load32(const PspGuestBus *bus, uint32_t address, uint32_t *value);
int psp_ppsspp_bus_store8(const PspGuestBus *bus, uint32_t address, uint8_t value);
int psp_ppsspp_bus_store16(const PspGuestBus *bus, uint32_t address, uint16_t value);
int psp_ppsspp_bus_store32(const PspGuestBus *bus, uint32_t address, uint32_t value);

#ifdef __cplusplus
}
#endif
#endif
