# New 3DS-first PSP game startup checkpoint and low-RAM graphics

## Priority and safety

**Priority hardware:** New 3DS / New 2DS XL in native 3DS mode; Old 3DS, retail DS/DS Lite, verified Slot-2 Memory Expansion Pak, and DSi remain at their previous **synthetic validation** levels. The known local decrypted game ELF and CSO stay **outside public GitHub and CI**. This work does not claim PSP title-screen display, interactive gameplay, PSP graphics/audio emulation, or real-device benchmark data.

## Private Jetpack Joyride startup checkpoint (Nov 2026)

The user's own decrypted Jetpack ELF was inspected locally in the model's working environment. The local Python cross-check maps the two PSP PRX load segments at 0x08800000, zero-fills BSS, and applies **83,187** relocation writes before looking at the first guest instructions. Mapped image extent **6,320,916 bytes**, module name `Jetpack`, **19 imported libraries**, **145 imported functions**. No game bytes, opcodes, display-list data or asset files are published.

| Private check | Result / observed blocker |
| --- | --- |
| Guest module entry | 0x08955018 |
| First executable startup run without linked firmware | 13 guest instructions, then `SysMemUserForUser` NID `0x358CA1BB`, guest stub 0x089F73B4 |
| One exact, verified SDK-version service handled | 18 guest instructions, then next `SysMemUserForUser` NID `0xF77D77CB`, guest stub 0x089F739C |
| First SDK version argument recorded in private check | `0x06060010` |
| No graphics or audio produced | Confirmed: trace stopped at an unresolved service |

The second NID is listed in PSP export references as `sceKernelSetCompilerVersion`, but its precise PSP behavior needs separate verification before implementing an HLE. Reference: https://pspdev.github.io/psplibdoc/modules/sysmem.prx_SysMemUserForUser.html. The first NID corresponds to the PSP compiled SDK version setter, as documented by PSPSDK: https://pspdev.github.io/pspsdk/group__SysMem.html.

**Evidence level:** these 13- and 18-instruction figures come from a repeatable, independent **offline Python trace of the private ELF**, not a production C or New 3DS emulator run. The compiled C diagnostic contains matching bounded first-import, optional known-HLE, VFPU and exception stop paths, verified with entirely original synthetic fixtures in CTest. The default host CLI `psp_startup_probe` stops at the first import. Pass `--link-sdk-version` to link **only** the verified NID in a disposable guest-memory arena and trace to the next unresolved call. Unknown imports must never silently return success. Guard quotas apply to all owned guest RAM and the initial stack reserve.

## Lower-RAM and lower-GPU improvements implemented

- Existing `psp_native_texture_decode_region()` already decodes bounded crop rectangles but assumes the entire source PSP texture is resident. New `psp_native_texture_decode_stream()` takes a file/SD/ROM-backed random-access callback and reads **only** the rows or PSP 128-byte swizzle blocks needed for the crop, with a **fixed sub-3 KiB stack scratch** (2 KiB linear row plus four swizzle blocks and metadata), no internal heap allocation. This is suitable for native DS tile-sized output and incremental New 3DS GPU uploads; it does **not** execute PSP GPU display lists itself.
- Direct-color formats currently supported: `5650`, `5551`, `4444`, `8888`, both linear and PSP 16-byte × 8-row swizzled layouts. Indexed/CLUT and compressed texture formats remain explicitly unsupported until the game's GE traces require them. A failed I/O callback returns a clear error; callers must discard partially decoded output.
- Synthetic host tests compare every supported streamed texture output against the existing full-input decoder. One 8×8 32-bit tile from a 512×512 texture requires **256 bytes of source reads**, not a full **1,048,576-byte** source image in the renderer's RAM. Reading from a file is slower than RAM, so use actual frame-time and cache-miss telemetry to decide what to keep resident. A PSP game's input assets must remain on a legal private user-owned storage path.
- For DS/DS+Pak/DSi this decoder compiles into BlocksDS, but the 8-MiB official Pak remains cold-data storage only; original DS + Pak has no CPU/GPU speed improvement. Old/New 3DS can use libctru/Citro3D to upload only a needed texture region once a correct GE display-list parser is connected.
- Existing optimizations remain: bounded guest RAM guard, optional lazy GPU/audio/UMD initialization, 4 KiB ROM pages, per-model frame/quality cutbacks and the scratch-only PSP relocation path to avoid a redundant full-image copy.

## The measurements must remain separate

| Subsystem | New 3DS measurement to collect | Present verified evidence |
| --- | --- | --- |
| CPU/Allegrex | Instructions/slice; Allegrex fault trace; native ms per CPU slice | Synthetic CPU/HLE only, not real title throughput |
| GPU/GE | Real display lists, state/vertex/texture formats, GPU submission vs GPU idle time, dropped frames | One synthetic PSP sprite format, no real game GE trace |
| Resident RAM | Live region size/free, peak guest heap, texture-cache peak, native linear heap and restart delta | Model-specific bounded allocation diagnostics, not continuous retail gameplay |
| Storage | UMD sector/cache misses, read bytes and seek latency for a single private game path | File/ROM reader and synthetic cache regression tests, no title-run storage throughput |
| Audio | PSP audio output sample format, buffer cadence, underruns, DSP/CPU time | Synthetic backend lifecycle only; no real PSP samples played |

Do not substitute CI compilation for melonDS/Azahar screenshots or hardware measurements. Native emulator checks should produce verified boot logs and screenshots only when an actual emulator run is possible. Do not publish retail game screenshots/ROMs, private assets, or copyrighted bytes as CI artifacts.

## Acceptance tracking

1. **Loader relocation:** portable C PRX mapping and large relocation tests pass; private offline mapping passed; production C-on-private image still needs local execution.
2. **Repeatable startup checkpoint:** offline trace after 13 and 18 instructions is reproducible; the exact first PSP SDK-version firmware service has been implemented and synthetic C tests exercise it; next observed blocker is the compiler-version service.
3. **Render game graphics:** not demonstrated. Follow PR #32's synthetic sprite subset by collecting only the GE commands and vertex/texture formats actually submitted by the running game.
4. **Interactive gameplay:** not demonstrated. Wire only the input, thread/callback, audio and display services observed in private traces; defer broad PSP compatibility and codec work until needed.
5. **Stabilize New 3DS:** not started on real title. Require measured CPU, GPU, RAM, storage and audio budgets, repeatable restart, and bounded memory growth.
6. **Other hardware:** native `.nds`/`.3dsx`/`.cia` and Linux checks preserve regressions; actual DS gameplay feasibility remains unknown. Do not claim all five family profiles can play the game.

**Next engineering gate:** verify `sceKernelSetCompilerVersion` NID `0xF77D77CB` semantics; add only that tested service, then advance and repeat the private startup trace. In parallel, stage New 3DS CPU/GPU/RAM/storage/audio profiling hooks separately and stream texture crops through the new bounded callback API without double-loading the source image.
