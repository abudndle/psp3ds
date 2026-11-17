#include "psp_ppsspp_memory_adapter.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <type_traits>

static_assert(std::is_same<decltype(psp_ppsspp_checked_read32(
    static_cast<const PspGuestBus *>(nullptr), std::uint32_t{}, static_cast<std::uint32_t *>(nullptr))),
    PspPpssppAccessResult>::value, "C++ must see explicit access status");

int main() {
    constexpr std::uint32_t base = UINT32_C(0x08800000);
    constexpr std::uint32_t cold_base = UINT32_C(0x09000000);
    std::array<std::uint8_t, 32> hot{};
    std::array<std::uint16_t, 16> cold_words{};
    PspGuestWindow cold{};
    PspGuestBus bus{};
    assert(psp_guest_window_init(&cold, cold_words.data(), cold_base,
                                 sizeof cold_words, 1, 1) == 0);
    assert(psp_guest_bus_init(&bus, hot.data(), base, hot.size(), &cold) == 0);

    assert(psp_ppsspp_checked_write32(&bus, UINT32_C(0x88800000),
                                      UINT32_C(0x24080007)) == PSP_PPSSPP_ACCESS_OK);
    std::uint32_t instruction = 0;
    assert(psp_ppsspp_checked_fetch32(&bus, UINT32_C(0xa8800000), &instruction) ==
           PSP_PPSSPP_ACCESS_OK);
    assert(instruction == UINT32_C(0x24080007));

    assert(psp_ppsspp_checked_write64(&bus, UINT32_C(0xa9000000),
                                      UINT64_C(0x8877665544332211)) == PSP_PPSSPP_ACCESS_OK);
    std::uint64_t value = 0;
    assert(psp_ppsspp_checked_read64(&bus, UINT32_C(0x89000000), &value) ==
           PSP_PPSSPP_ACCESS_OK);
    assert(value == UINT64_C(0x8877665544332211));

    instruction = UINT32_C(0xdeadbeef);
    assert(psp_ppsspp_checked_fetch32(&bus, UINT32_C(0x89000000), &instruction) ==
           PSP_PPSSPP_ACCESS_NONEXECUTABLE);
    assert(instruction == UINT32_C(0xdeadbeef));
    assert(psp_ppsspp_checked_fetch32(&bus, UINT32_C(0x88800001), &instruction) ==
           PSP_PPSSPP_ACCESS_UNALIGNED);
    assert(instruction == UINT32_C(0xdeadbeef));
    assert(psp_ppsspp_checked_read32(&bus, UINT32_C(0xc8800000), &instruction) ==
           PSP_PPSSPP_ACCESS_UNMAPPED);
    assert(instruction == UINT32_C(0xdeadbeef));

    std::puts("PASS: C++ caller links checked C memory API; PPSSPP interpreter is not running");
    return 0;
}
