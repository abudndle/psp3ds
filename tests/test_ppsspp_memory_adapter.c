#include "psp_ppsspp_memory_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    uint8_t hot[32] = {0};
    PspGuestBus bus;
    uint16_t cold_words[16] = {0};
    PspGuestWindow cold;
    uint32_t value = 0;
    uint8_t byte = 0;
    uint16_t half = 0;
    uint64_t wide = 0;
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
    assert(psp_ppsspp_adapter_write8(&bus, UINT32_C(0xa8800004), 0x5a) == 0);
    assert(psp_ppsspp_adapter_read8(&bus, UINT32_C(0x88800004), &byte) == 0 && byte == 0x5a);
    assert(psp_ppsspp_adapter_write16(&bus, UINT32_C(0x88800006), UINT16_C(0xbeef)) == 0);
    assert(psp_ppsspp_adapter_read16(&bus, UINT32_C(0xa8800006), &half) == 0 && half == UINT16_C(0xbeef));
    assert(psp_ppsspp_adapter_write64(&bus, UINT32_C(0xa8800008), UINT64_C(0x1122334455667788)) == 0);
    assert(psp_ppsspp_adapter_read64(&bus, UINT32_C(0x88800008), &wide) == 0 && wide == UINT64_C(0x1122334455667788));
    assert(psp_ppsspp_adapter_read64(&bus, UINT32_C(0xa880001c), &wide) == -1);
    assert(psp_ppsspp_adapter_write16(&bus, UINT32_C(0x88800001), 0) == -1);
    /* Cold Slot-2 is readable/writable through aliases, never executable. */
    assert(psp_guest_window_init(&cold, cold_words, UINT32_C(0x09000000),
                                 sizeof cold_words, 1, 1) == 0);
    assert(psp_guest_bus_init(&bus, hot, UINT32_C(0x08800000), sizeof hot, &cold) == 0);
    assert(psp_ppsspp_adapter_write32(&bus, UINT32_C(0xa9000000), UINT32_C(0x8d090000)) == 0);
    assert(psp_ppsspp_adapter_read32(&bus, UINT32_C(0x89000000), &value) == 0);
    assert(value == UINT32_C(0x8d090000));
    value = UINT32_C(0xdeadbeef);
    assert(psp_ppsspp_adapter_fetch32(&bus, UINT32_C(0x89000000), &value) == -1);
    assert(value == UINT32_C(0xdeadbeef));
    assert(psp_ppsspp_adapter_fetch32(&bus, UINT32_C(0xa9000000), &value) == -1);
    assert(value == UINT32_C(0xdeadbeef));
    assert(psp_ppsspp_adapter_fetch32(&bus, UINT32_C(0x88800000), &value) == 0);
    assert(value == UINT32_C(0x24080007));
    assert(psp_ppsspp_check_access(&bus, UINT32_C(0x88800000), 4, 1) ==
           PSP_PPSSPP_ACCESS_OK);
    assert(psp_ppsspp_check_access(&bus, UINT32_C(0xa9000000), 4, 0) ==
           PSP_PPSSPP_ACCESS_OK);
    assert(psp_ppsspp_check_access(&bus, UINT32_C(0x89000000), 4, 1) ==
           PSP_PPSSPP_ACCESS_NONEXECUTABLE);
    assert(psp_ppsspp_check_access(&bus, UINT32_C(0x88800001), 4, 1) ==
           PSP_PPSSPP_ACCESS_UNALIGNED);
    assert(psp_ppsspp_check_access(&bus, UINT32_C(0x88800020), 4, 1) ==
           PSP_PPSSPP_ACCESS_UNMAPPED);
    assert(psp_ppsspp_check_access(&bus, UINT32_C(0x88800000), 3, 0) ==
           PSP_PPSSPP_ACCESS_UNMAPPED);
    assert(psp_ppsspp_check_access(NULL, UINT32_C(0x88800000), 4, 0) ==
           PSP_PPSSPP_ACCESS_UNMAPPED);
    puts("PASS: KSEG0/KSEG1 aliases and checked guest memory (not PPSSPP execution)");
    return 0;
}
