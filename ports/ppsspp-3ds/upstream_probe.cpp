#include <cstdio>
#include <cstdarg>
#include "Common/Log.h"
#include "Core/MIPS/Interpreter.h"
#include "Core/MIPS/MIPS.h"
#include "psp_mips_probe_fixture.h"
#include "psp_nintendo_input.h"
#include "psp_native_texture.h"

// The arithmetic probe links no guest memory subsystem. MIPSState's upstream
// constructor takes this object's address, but the probe never dereferences it.
Memory::AddressSpace Memory::g_mainSpace{};
 LogChannel g_log[(size_t)Log::NUMBER_OF_LOGS]{};
static bool logging = false;
bool *g_bLogEnabledSetting = &logging;
void GenericLog(Log, LogLevel, const char *, int, const char *format, ...) {
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
}

extern "C" bool ppsspp_integer_probe() {
    MIPSState cpu;
    cpu.pc = PSP_PROBE_GUEST_BASE;
    cpu.r[0] = 0;
    MIPSInt::Int_IType(&cpu, MIPSOpcode(PSP_PROBE_ADDIU_T0));
    MIPSInt::Int_IType(&cpu, MIPSOpcode(PSP_PROBE_ADDIU_T1));
    return cpu.r[8] == PSP_PROBE_EXPECT_T0 &&
           cpu.r[9] == PSP_PROBE_EXPECT_T1 &&
           cpu.r[0] == 0 && cpu.pc == PSP_PROBE_EXPECT_PC;
}

/* Verify that the same PSP sceCtrl-compatible bit layout is callable from
 * PPSSPP's C++ side without libctru/libnds symbols leaking into the core. */
extern "C" bool ppsspp_controls_probe() {
    PspNintendoCtrl pad{};
    if (psp_nintendo_ctrl_sample(PSP_TARGET_NEW3DS,
            PSP_NINTENDO_KEY_A | PSP_NINTENDO_KEY_UP,
            156, -156, &pad) != 0)
        return false;
    if (pad.buttons != (PSP_CTRL_CIRCLE | PSP_CTRL_UP) ||
        pad.lx != 255 || pad.ly != 255)
        return false;
    if (psp_nintendo_ctrl_sample(PSP_TARGET_DS_SLOT2,
            PSP_NINTENDO_KEY_B | PSP_NINTENDO_KEY_LEFT,
            156, 156, &pad) != 0)
        return false;
    return pad.buttons == (PSP_CTRL_CROSS | PSP_CTRL_LEFT) &&
           pad.lx == 0 && pad.ly == 128;
}

/* Exercise the same PSP GE direct-color decoder on the PPSSPP C++ link
 * boundary without any OpenGL, Vulkan or D3D runtime. Synthetic only. */
extern "C" bool ppsspp_texture_probe() {
    const PspNativeTexture t{2, 1, 2, PSP_TEXTURE_5650, 0};
    const PspNativeTextureRegion region{0, 0, 2, 1};
    const uint8_t texels[]{0x1f, 0x00, 0xe0, 0x07};
    uint8_t pixels[8]{};
    if (psp_native_texture_decode_region(&t, texels, sizeof(texels),
                                         &region, pixels, 8, sizeof(pixels)) != PSP_TEXTURE_OK)
        return false;
    const uint8_t expected[]{255, 0, 0, 255, 0, 255, 0, 255};
    for (unsigned i = 0; i < sizeof(pixels); ++i)
        if (pixels[i] != expected[i])
            return false;
    return true;
}
