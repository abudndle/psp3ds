#include "psp_ppsspp_memory_adapter.h"

PspPpssppAccessResult psp_ppsspp_check_access(
    const PspGuestBus *bus, uint32_t address, unsigned bytes, int instruction_fetch) {
    uint32_t physical;
    if (!bus || !bus->hot || (bytes != 1u && bytes != 2u &&
                             bytes != 4u && bytes != 8u))
        return PSP_PPSSPP_ACCESS_UNMAPPED;
    if ((address & (bytes - 1u)) != 0u)
        return PSP_PPSSPP_ACCESS_UNALIGNED;
    if (instruction_fetch && bytes != 4u)
        return PSP_PPSSPP_ACCESS_UNMAPPED;
    if (psp_ppsspp_translate_address(address, &physical))
        return PSP_PPSSPP_ACCESS_UNMAPPED;
    const uint64_t start = physical;
    const uint64_t end = start + bytes;
    const uint64_t hot_start = bus->hot_base;
    const uint64_t hot_end = hot_start + bus->hot_size;
    if (start >= hot_start && end <= hot_end)
        return PSP_PPSSPP_ACCESS_OK;
    if (bus->has_cold && bus->cold.enabled && bus->cold.words) {
        const uint64_t cold_start = bus->cold.guest_base;
        const uint64_t cold_end = cold_start + bus->cold.capacity;
        if (start >= cold_start && end <= cold_end)
            return instruction_fetch ? PSP_PPSSPP_ACCESS_NONEXECUTABLE :
                                       PSP_PPSSPP_ACCESS_OK;
    }
    return PSP_PPSSPP_ACCESS_UNMAPPED;
}

PspPpssppAccessResult psp_ppsspp_checked_fetch32(
    const PspGuestBus *bus, uint32_t pc, uint32_t *instruction) {
    if (!instruction)
        return PSP_PPSSPP_ACCESS_UNMAPPED;
    PspPpssppAccessResult status = psp_ppsspp_check_access(bus, pc, 4u, 1);
    if (status != PSP_PPSSPP_ACCESS_OK)
        return status;
    return psp_ppsspp_adapter_fetch32(bus, pc, instruction) == 0 ?
           PSP_PPSSPP_ACCESS_OK : PSP_PPSSPP_ACCESS_UNMAPPED;
}
PspPpssppAccessResult psp_ppsspp_checked_read32(
    const PspGuestBus *bus, uint32_t address, uint32_t *value) {
    if (!value)
        return PSP_PPSSPP_ACCESS_UNMAPPED;
    PspPpssppAccessResult status = psp_ppsspp_check_access(bus, address, 4u, 0);
    if (status != PSP_PPSSPP_ACCESS_OK)
        return status;
    return psp_ppsspp_adapter_read32(bus, address, value) == 0 ?
           PSP_PPSSPP_ACCESS_OK : PSP_PPSSPP_ACCESS_UNMAPPED;
}
PspPpssppAccessResult psp_ppsspp_checked_write32(
    const PspGuestBus *bus, uint32_t address, uint32_t value) {
    PspPpssppAccessResult status = psp_ppsspp_check_access(bus, address, 4u, 0);
    if (status != PSP_PPSSPP_ACCESS_OK)
        return status;
    return psp_ppsspp_adapter_write32(bus, address, value) == 0 ?
           PSP_PPSSPP_ACCESS_OK : PSP_PPSSPP_ACCESS_UNMAPPED;
}

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

#define DEFINE_ADAPTER_READ(bits, type) \
int psp_ppsspp_adapter_read##bits(const PspGuestBus *bus, uint32_t address, type *value) { \
    uint32_t physical; \
    if (psp_ppsspp_translate_address(address, &physical)) return -1; \
    return psp_ppsspp_bus_load##bits(bus, physical, value); \
}
#define DEFINE_ADAPTER_WRITE(bits, type) \
int psp_ppsspp_adapter_write##bits(const PspGuestBus *bus, uint32_t address, type value) { \
    uint32_t physical; \
    if (psp_ppsspp_translate_address(address, &physical)) return -1; \
    return psp_ppsspp_bus_store##bits(bus, physical, value); \
}
DEFINE_ADAPTER_READ(8, uint8_t)
DEFINE_ADAPTER_READ(16, uint16_t)
DEFINE_ADAPTER_READ(64, uint64_t)
DEFINE_ADAPTER_WRITE(8, uint8_t)
DEFINE_ADAPTER_WRITE(16, uint16_t)
DEFINE_ADAPTER_WRITE(64, uint64_t)
#undef DEFINE_ADAPTER_READ
#undef DEFINE_ADAPTER_WRITE
