# PPSSPP port for Nintendo handhelds

The first checkpoint is a New 3DS interpreter build of PPSSPP. The ABI
probe checks the pinned upstream integer types and libctru integration
on ARMv6K and displays the console family. It executes no PSP code.

The next build checkpoint is linking upstream CPU, memory, and HLE code
under devkitARM. Start with the interpreter and a headless synthetic PSP
program. Then add native display, audio output, controls and storage.
Keep upstream GPL notices and license terms with any integrated PPSSPP
source. Use original synthetic assets in public CI.

Verify Jetpack Joyride privately through its title screen and a complete
run. Measure frame timing and memory separately on New and original 3DS.
DS, DS with Slot-2 and DSi retain the lightweight runtime path while
their memory and performance limits are evaluated.
