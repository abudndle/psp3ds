#include <cstdio>
#include <cstdarg>
#include "Common/Log.h"
#include "Core/MIPS/Interpreter.h"
#include "Core/MIPS/MIPS.h"
#include "psp_mips_probe_fixture.h"

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
