#include "psp_ppsspp_memory_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    uint8_t hot[32] = {0};
    PspGuestBus bus;
    uint32_t value = 0;
    assert(psp_guest_bus_init(&bus, hot, UINT32_C(0x08800000), sizeof hot, NULL) == 0);
    assert(psp_ppsspp_translate_address(UINT32_C(0x88800000), &value) == 0);
    assert(value == UINT32_C(0x08800000));
    assert(psp_ppsspp_translate_address(UINT32_C(0xa8800000), &value) == 0);
    assert(value == UINT32_C(0x08800000));
    assert(psp_ppsspp_translate_address(UINT32_C(0x08800000), &value) == 0);
    assert(value == UINT32_C(0x08800000));
    assert(psp_ppsspp_translate_address(0, NULL) == -1);
    assert(psp_ppsspp_adapter_write32(&bus, UINT32_C(0x88800000), UINT32_C(0x24080007)) == 0);
    assert(psp_ppsspp_adapter_fetch32(&bus, UINT32_C(0xa8800000), &value) == 0);
    assert(value == UINT32_C(0x24080007));
    assert(psp_ppsspp_adapter_read32(&bus, UINT32_C(0x08800000), &value) == 0);
    assert(value == UINT32_C(0x24080007));
    assert(psp_ppsspp_adapter_fetch32(&bus, UINT32_C(0xa8800001), &value) == -1);
    assert(psp_ppsspp_adapter_read32(&bus, UINT32_C(0x88800020), &value) == -1);
    assert(psp_ppsspp_adapter_write32(&bus, UINT32_C(0xc8800000), 0) == -1);
    puts("PASS: KSEG0/KSEG1 aliases and checked guest memory (not PPSSPP execution)");
    return 0;
}
