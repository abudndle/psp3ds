#include <cstdio>
#include <cstdarg>
#include "Common/Log.h"
#include "Core/MIPS/Interpreter.h"
#include "Core/MIPS/MIPS.h"

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
    cpu.pc = 0x08800000;
    cpu.r[0] = 0;
    MIPSInt::Int_IType(&cpu, MIPSOpcode(0x2408002a)); // addiu t0, zero, 42
    MIPSInt::Int_IType(&cpu, MIPSOpcode(0x2509fffe)); // addiu t1, t0, -2
    return cpu.r[8] == 42 && cpu.r[9] == 40 && cpu.pc == 0x08800008;
}
