#include "psp_ppsspp_bus_bridge.h"

int psp_ppsspp_bus_fetch32(const PspGuestBus *bus, uint32_t pc, uint32_t *instruction) {
    /* Slot-2 cold memory is data-only. Never execute instructions from it. */
    if (!bus || !bus->hot || !instruction || (pc & 3u) ||
        pc < bus->hot_base)
        return -1;
    uint64_t offset = (uint64_t)pc - bus->hot_base;
    if (offset > bus->hot_size || bus->hot_size - offset < 4u)
        return -1;
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

int psp_ppsspp_bus_load64(const PspGuestBus *bus, uint32_t address, uint64_t *value) {
    uint32_t lo, hi;
    if (!value || (address & 7u) || address > UINT32_MAX - 7u ||
        psp_guest_bus_read32(bus, address, &lo) ||
        psp_guest_bus_read32(bus, address + 4u, &hi))
        return -1;
    *value = (uint64_t)lo | ((uint64_t)hi << 32);
    return 0;
}
int psp_ppsspp_bus_store64(const PspGuestBus *bus, uint32_t address, uint64_t value) {
    uint32_t lo, hi;
    if ((address & 7u) || address > UINT32_MAX - 7u ||
        psp_guest_bus_read32(bus, address, &lo) ||
        psp_guest_bus_read32(bus, address + 4u, &hi))
        return -1;
    if (psp_guest_bus_write32(bus, address, (uint32_t)value))
        return -1;
    return psp_guest_bus_write32(bus, address + 4u, (uint32_t)(value >> 32));
}
