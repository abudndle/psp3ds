#include "psp_ppsspp_bus_bridge.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BASE UINT32_C(0x08800000)
#define COLD UINT32_C(0x09000000)

int main(void) {
    uint8_t hot[32] = {0};
    uint16_t cold_words[16] = {0};
    PspGuestBus bus;
    PspGuestWindow cold;
    uint32_t instruction = 0, word = 0;
    uint16_t half = 0;
    uint8_t byte = 0;
    assert(psp_guest_window_init(&cold, cold_words, COLD, sizeof cold_words, 1, 1) == 0);
    assert(psp_guest_bus_init(&bus, hot, BASE, sizeof hot, &cold) == 0);
    assert(psp_ppsspp_bus_store32(&bus, BASE, UINT32_C(0x24080007)) == 0);
    assert(psp_ppsspp_bus_fetch32(&bus, BASE, &instruction) == 0);
    assert(instruction == UINT32_C(0x24080007));
    assert(psp_ppsspp_bus_store16(&bus, BASE + 4, UINT16_C(0x1234)) == 0);
    assert(psp_ppsspp_bus_load16(&bus, BASE + 4, &half) == 0 && half == 0x1234);
    assert(psp_ppsspp_bus_store8(&bus, BASE + 5, 0xab) == 0);
    assert(psp_ppsspp_bus_load8(&bus, BASE + 5, &byte) == 0 && byte == 0xab);
    assert(psp_ppsspp_bus_store32(&bus, COLD, UINT32_C(0x8d090000)) == 0);
    assert(psp_ppsspp_bus_fetch32(&bus, COLD, &word) == 0 && word == UINT32_C(0x8d090000));
    assert(psp_ppsspp_bus_load32(&bus, COLD, &word) == 0 && word == UINT32_C(0x8d090000));
    assert(psp_ppsspp_bus_fetch32(&bus, BASE + 1, &instruction) == -1);
    assert(psp_ppsspp_bus_fetch32(&bus, BASE + sizeof hot, &instruction) == -1);
    assert(psp_ppsspp_bus_load16(&bus, COLD + 1, &half) == -1);
    assert(psp_ppsspp_bus_store32(&bus, COLD + sizeof cold_words, 0) == -1);
    assert(psp_guest_bus_init(&bus, hot, BASE, sizeof hot, NULL) == 0);
    assert(psp_ppsspp_bus_fetch32(&bus, COLD, &word) == -1);
    puts("PASS: checked PSP bus bridge fetch/load/store and bounds (not PPSSPP execution)");
    return 0;
}
