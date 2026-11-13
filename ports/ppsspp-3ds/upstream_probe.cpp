#include "Common/CommonTypes.h"
#include "Common/Data/Convert/SmallDataConvert.h"

static_assert(sizeof(u32) == 4 && sizeof(u64) == 8, "Unexpected PSP integer ABI");

extern "C" bool ppsspp_integer_probe() {
    volatile u32 immediate = 0x2408fffe;
    return SignExtend16ToS32(immediate) == -2;
}
