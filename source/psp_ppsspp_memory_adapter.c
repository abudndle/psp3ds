#include "psp_ppsspp_memory_adapter.h"

int psp_ppsspp_translate_address(uint32_t address, uint32_t *physical) {
    if (!physical)
        return -1;
    /* MIPS KSEG0/KSEG1 mirror low 512 MiB; other segments are not
     * translated here. Actual validity is enforced by PspGuestBus. */
    if ((address & UINT32_C(0xc0000000)) == UINT32_C(0x80000000))
        address &= UINT32_C(0x1fffffff);
    *physical = address;
    return 0;
}
int psp_ppsspp_adapter_fetch32(const PspGuestBus *bus, uint32_t pc, uint32_t *instruction) {
    uint32_t address;
    if (psp_ppsspp_translate_address(pc, &address))
        return -1;
    return psp_ppsspp_bus_fetch32(bus, address, instruction);
}
int psp_ppsspp_adapter_read32(const PspGuestBus *bus, uint32_t address, uint32_t *value) {
    uint32_t physical;
    if (psp_ppsspp_translate_address(address, &physical))
        return -1;
    return psp_ppsspp_bus_load32(bus, physical, value);
}
int psp_ppsspp_adapter_write32(const PspGuestBus *bus, uint32_t address, uint32_t value) {
    uint32_t physical;
    if (psp_ppsspp_translate_address(address, &physical))
        return -1;
    return psp_ppsspp_bus_store32(bus, physical, value);
}
