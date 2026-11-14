
# Jetpack Joyride: first checkpoint and five-device advancement

**All five device profiles advance concurrently.** New 3DS remains the lead for the first playable build, but original 3DS, DSi, verified Slot-2 DS and DS/DS Lite receive real native tests and resource profiling. Never mark retail PSP gameplay as working because a binary compiles.

## Private sample and first startup dependency

Private, user-supplied Jetpack Joyride ET_SCE_PRX: 2,891,733 file bytes, 6,320,916 mapped guest bytes, 83,187 valid PSPREL records, entry 0x08955018, 145 imports in 19 libraries. A private offline mapped-image control-flow analysis identifies the first unconditional JAL at entry instruction index 11, targeting SysMemUserForUser NID 0x358CA1BB (the compiled-SDK-version setter). This is a **static candidate**, not evidence that the commercial game has executed. Public PSP NID references include https://github.com/pspdev/psp-ghidra-scripts/blob/master/ppsspp_niddb.xml and https://pspdev.github.io/pspsdk/group__SysMem.html.

The verified import linker patches only NIDs with an actually implemented service; every missing firmware import stays unlinked and stops the bounded startup trace. Existing first-handler semantics record the caller SDK version and return the documented value. The private host probe accepts a local decrypted ELF and reports PC, first missing NID, and a byte-free control-flow fingerprint. It never uploads the game.

## Five active target profiles

| Target | Hardware constraint | Next measurable implementation goal |
| --- | --- | --- |
| New 3DS / New 2DS XL | Measured free APPLICATION memory, protected 24 MiB reserve; PICA200 | Actual private import-linked C startup trace; real GE list, sound, input and peak RAM |
| Original 3DS / 2DS | Smaller measured application region; protected 16 MiB reserve; PICA200 | Same bounded trace and native GPU/SD observations, lower cache/CPU slice |
| DSi / DSi XL enhanced | 16 MiB main RAM, no Slot-2 | Native Allegrex checkpoint; use DSi-specific TWL code/data; test bounded working-set paging |
| DS + verified Memory Pak | 4 MiB main + separate 8 MiB slow Slot-2 data, no CPU/GPU boost | Native startup checkpoint plus fault-safe hot/cold page ownership; probe throughput and aliasing |
| DS / DS Lite | 4 MiB main RAM and limited native GPU | Native startup checkpoint; paged ROM assets, paletted sprites and texture conversions |

DSi legacy-mode uses the DS row. No DS-family profile can hold the full ~6.32 MB PRX in the current 512 KiB/2 MiB hot-buffer policy. A Memory Pak is not a 12 MiB contiguous executable heap. Commercial gameplay feasibility is unproven.

## Separate bottleneck measurements

The portable stage metrics collect separately: actual CPU benchmark time; CPU wall time submitting native GPU commands (**not GPU hardware completion**); real storage-read timing if a real file exists; actual audio-processing timing only when implemented; observed peak application RAM (kernel memory counters on 3DS). Missing values are explicitly **unmeasured**. Linux fixtures provide deterministic test measurements, not physical console performance.

Build verification requires Linux CTest and ASan/UBSan, fresh BlocksDS NDS and devkitARM 3DS builds. melonDS, Azahar and real-hardware evidence must be separately recorded from actual launches, not inferred from successful CI. Neither emulator is installed in the current coding container, and no retail game bytes are checked in.

## Low-RAM and native-GPU opportunities

1. Preserve PSP CLUT4/CLUT8 pixel indices and palette information when native DS texture formats are compatible. A 128x128 indexed 4-bit buffer needs 8 KiB of indices, versus 64 KiB at RGBA8888, before palettes and upload staging. Confirm actual PSP texture layout and nibble/swizzle order before use. DS VRAM is banked, not general heap: https://blocksds.skylyrac.net/docs/internal/memory_map/ ; https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/.
2. Stream the one known-good disc/data path via 4 KiB page reads. Reuse fixed small staging buffers and evict only unreferenced data. Keep Slot-2 cold data under verified open/close ownership. On 3DS, Tex3DS supports callback/file-descriptor texture imports to avoid preloading entire files: https://github.com/devkitPro/citro3d/blob/master/include/tex3ds.h.
3. Batch compatible sprites, avoid redundant texture uploads and geometry; use native DS tile/OAM hardware and PICA200 Citro3D when real PSP GPU state can be reproduced. Unknown GE state must fail closed.
4. Place DSi-only data/code in dedicated TWL sections so DS static memory does not grow: https://blocksds.skylyrac.net/docs/internal/build_process/ . Avoid full-image copies during PRX relocation; load exclusive scratch images under quotas.
5. Only initialize a GPU backend, audio codec, threads, or UMD cache when the game actually requests one. Audio and PSP guest virtual time must continue correctly even when only presentation frequency is reduced.

## Per-game and per-system milestone gates

**2 - Startup:** repeatable private C trace with one genuine known SDK setter dispatched and stable next blocker; currently not yet confirmed from actual C execution of the private ELF.

**3 - Graphics:** actual game display lists execute via native graphics with verified vertex formats, textures, blend/present state; not demonstrated.

**4 - Interactive:** PSP controller polling and correctly timed real PSP sound output reach controllable gameplay; not demonstrated.

**5 - Stable:** repeat plays and restarts with measured CPU, GPU, RAM, storage, audio and no growing resource leaks on New 3DS first; not demonstrated.

**6 - All devices:** independent melonDS, Azahar and physical console verification for DS, DS+Pak, DSi, Old 3DS, New 3DS. Each test has distinct fields for module boot, first frame, gameplay, restart, peak RAM and frame times. Keep retail samples private and report negative feasibility results without disguising them as passes.
