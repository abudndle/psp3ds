#include "psp_ppsspp_bus_bridge.h"

int psp_ppsspp_bus_fetch32(const PspGuestBus *bus, uint32_t pc, uint32_t *instruction) {
    return psp_guest_bus_read32(bus, pc, instruction);
}
int psp_ppsspp_bus_load8(const PspGuestBus *bus, uint32_t address, uint8_t *value) {
    return psp_guest_bus_read8(bus, address, value);
}
int psp_ppsspp_bus_load16(const PspGuestBus *bus, uint32_t address, uint16_t *value) {
    return psp_guest_bus_read16(bus, address, value);
}
int psp_ppsspp_bus_load32(const PspGuestBus *bus, uint32_t address, uint32_t *value) {
    return psp_guest_bus_read32(bus, address, value);
}
int psp_ppsspp_bus_store8(const PspGuestBus *bus, uint32_t address, uint8_t value) {
    return psp_guest_bus_write8(bus, address, value);
}
int psp_ppsspp_bus_store16(const PspGuestBus *bus, uint32_t address, uint16_t value) {
    return psp_guest_bus_write16(bus, address, value);
}
int psp_ppsspp_bus_store32(const PspGuestBus *bus, uint32_t address, uint32_t value) {
    return psp_guest_bus_write32(bus, address, value);
}
