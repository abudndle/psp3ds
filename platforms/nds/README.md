# Nintendo DS with DSi enhancements (future PSP experiment)

This is the **planned** second console target in the same PSP3DS GitHub repo,
not a second GitHub repository. There is no `.nds` binary or BlocksDS build
yet. The future build should run in basic DS mode on DS/DS Lite, optionally
use detected Slot-2 RAM expansion, and enable DSi enhancements when available.

BlocksDS dependency candidates:
- libnds for input, ARM9/ARM7 and storage access;
- Maxmod for sound if an audio backend becomes practical;
- Nitro Engine for experimental 3D rendering;
- optional NightFox's Lib / libdsf for UI;
- optional libteak only in DSi mode if benchmarks justify its use.

Do not attempt to allocate PSP-sized guest RAM on DS without validating
hardware capacity. The DS/DSi is vastly less powerful than a PSP, so
commercial-game compatibility is **not an achievable commitment** at this
stage. The first milestones should be compilable `.nds` startup, hardware
identification, bounded guest-memory allocation, then CPU unit demos.

See [library inventory](../../docs/PLATFORM_LIBRARIES.md) and
[BlocksDS documentation](https://blocksds.skylyrac.net/docs/libs/).
