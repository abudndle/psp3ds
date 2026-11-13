# PSP3DS Nintendo DS / DSi-enhanced experiment

This target is inside the **same GitHub repository** as PSP3DS.

The BlocksDS ARM9-only Makefile compiles the existing portable
`source/psp_core.c` through a small wrapper. Its native entry point
detects DSi mode using `isDSiMode()` and executes two synthetic MIPS
instructions to demonstrate code reuse. It does **not** execute PSP games.

With BlocksDS installed:
```sh
make -C platforms/nds
```
Expected output: `platforms/nds/psp3ds_nds.nds`.

The actual BlocksDS compiler is absent from the current Linux environment,
so the native `.nds` binary and melonDS testing are **not verified**.
DSi enhancements are identified but PSP-specific enhanced memory, 3D/audio,
Slot-2 RAM expansion and game-loading services are unimplemented.

See [platform libraries](../../docs/PLATFORM_LIBRARIES.md).
