#include <3ds.h>
#include <cstdio>
#include "Common/CommonTypes.h"

static_assert(sizeof(u32) == 4 && sizeof(u64) == 8, "Unexpected PSP integer ABI");
static_assert(sizeof(void *) == 4, "Expected a 32-bit 3DS target");

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, nullptr);
    bool new_model = false;
    const auto result = APT_CheckNew3DS(&new_model);
    std::printf("PPSSPP 3DS ABI probe\n");
    std::printf("Model: %s\n", R_FAILED(result) ? "unknown" :
                new_model ? "New 3DS family" : "original 3DS family");
    std::printf("START: exit\n");
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
}
