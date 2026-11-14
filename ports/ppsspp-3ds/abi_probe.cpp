#include <3ds.h>
#include <cstdio>
extern "C" bool ppsspp_integer_probe();
extern "C" bool ppsspp_controls_probe();
extern "C" bool ppsspp_texture_probe();

static_assert(sizeof(void *) == 4, "Expected a 32-bit 3DS target");

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, nullptr);
    bool new_model = false;
    const auto result = APT_CheckNew3DS(&new_model);
    std::printf("PPSSPP 3DS ABI probe\n");
    std::printf("Upstream ADDIU execution: %s\n", ppsspp_integer_probe() ? "PASS" : "FAIL");
    std::printf("Nintendo PSP controls: %s\n", ppsspp_controls_probe() ? "PASS" : "FAIL");
    std::printf("Native PSP textures: %s\n", ppsspp_texture_probe() ? "PASS" : "FAIL");
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
