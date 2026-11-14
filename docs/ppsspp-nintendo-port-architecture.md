# Porting upstream PPSSPP to Nintendo DS, DSi, 2DS and 3DS

## Goal and constraints

The scope is the *whole* PPSSPP emulator: CPU, VFPU, memory, HLE/kernel,
graphics, sound/codecs, media/filesystems, controls, save data, UI and
optional network features. It is **not** achieved by compiling the upstream
OpenGL/Vulkan frontend for Nintendo hardware.

The pinned upstream reference in
`.github/workflows/ppsspp-3ds-probe.yml` is
`hrydgard/ppsspp@1aebad2c651ff9d0684ee335e43aceff8fc7e3ec`.
Do not publish third-party games, firmware or keys. Retain upstream PPSSPP
GPL-2.0-or-later obligations and notices whenever reusing code.

### Architecture

```text
PSP game/homebrew
  -> upstream PPSSPP Core (Allegrex/VFPU, memory, HLE, timing)
      -> emulated PSP GPU command semantics / common texture state
          -> portable bounded draw batches / guest memory views
              -> 3DS: Citro3D/Citro2D PICA200 (NO OpenGL)
              -> DS/DS Lite: VRAM tile maps + OAM; bounded software fallback
              -> DSi/DSi XL: DS renderer + measured extra resources in DSi mode
      -> audio + codec services -> 3DS NDSP / DS-DSi audio adapter
      -> controller API -> libctru HID / libnds keys + touch
      -> VFS, game media, saves -> SD/FAT, streaming, caching
      -> system timing and workers -> platform scheduler/threads
```

The same PSP semantics and compatibility-test fixtures must be shared by
the handheld targets. Platform-specific implementations are intentionally
not identical: a PICA200 GPU command stream is not OpenGL; DS tiles and
sprites cannot directly represent general PSP GE rendering.

### Native screen and video contract

`include/psp_video_layout.h` implements aspect-preserving viewport
calculation and rectangle projection with integer operations, no heap and
no dependence on OpenGL/Vulkan or a desktop host window. The PSP native
source is 480 x 272, but synthetic 256 x 192 scenes also use this mapping.
The geometry does **not** yet constitute PSP framebuffer playback.

| Hardware family | Display | PSP 480 x 272 viewport |
| --- | --- | --- |
| DS / DS Lite / verified Slot-2 RAM | Main 256 x 192 | 256 x 145, centered vertically |
| DSi / DSi XL | Main 256 x 192 | 256 x 145, centered vertically |
| 3DS / 3DS XL / 2DS | Top 400 x 240 | 400 x 226, centered vertically |
| New 3DS / New 3DS XL / New 2DS XL | Top 400 x 240 | 400 x 226, centered vertically |
| Any 3DS family | Bottom 320 x 240 | 320 x 181, centered vertically |

The native 3DS synthetic GE-rectangle renderer now maps rectangles using
the same shared integer mapping rather than a hard-coded 1.25x scale.
Native DS/DSi and 3DS entrypoints report their PSP viewport policies.

### Subsystem migration and acceptance gates

1. **CPU/VFPU**: link upstream interpreter and execution state to a
   bounded PSP guest memory implementation. Verify synthetic MIPS programs,
   branches, load/store, exception and VFPU fixtures. Avoid enabling the
   upstream desktop JIT on ARM946E-S or ARM11 without a real supported port.
   The current ARMv6K probe executes only two PPSSPP ADDIU instructions.
2. **Memory / HLE / threads**: bring in the upstream memory services,
   kernel objects, callbacks, modules, PSP imports, timing and scheduler
   progressively. Compare deterministic PSP homebrew traces to upstream.
3. **Graphics**: preserve PSP GE command semantics but replace
   OpenGL/Vulkan/D3D with Nintendo-native backends. Support real guest
   vertex/texture formats, palette/swizzle, blending, render-to-texture,
   depth and frame presentation. Unknown operations must fail explicitly.
   Do not confuse synthetic sprite fixtures with a working GPU emulator.
4. **Media / saves / controls**: bounded ELF/PBP/ISO/CSO handling, FAT/SD
   access, save-data persistence and native button/touch input. Keep game
   resources and save data outside the public repository.
5. **Audio**: port PSP audio APIs, mixing, resampling, ATRAC and any other
   codecs required by test software. Native output targets are NDSP on
   3DS and appropriate DS/DSi audio services, with underrun measurements.
6. **UI / compatibility**: launcher, diagnostics, settings, pause/resume,
   optional networking and remaining PPSSPP functionality once the
   emulation core is validated.
7. **Optimization**: profile per device before replacing correct code.
   Bound guest-memory mappings, cache sizes and upload queues, batch draw
   operations, stream cold assets, and enable higher CPU resources only on
   identified hardware. Record frame times, RAM high-water marks, and
   audio deadlines alongside test results.

### Per-device feasibility and truthfulness

**New 3DS-family hardware is the primary upstream PPSSPP integration
target.** Original 3DS/2DS has lower resource limits. DS/DS Lite and DSi
have vastly less memory and compute than the original PSP; the general
upstream PPSSPP runtime cannot simply be compiled and expected to run PSP
games there. The DS/DSi path is a compact interpreter and backend
adaptation/research target. Slot-2 RAM adds cold data capacity only when
detected and verified; it does not make DS CPU or GPU PSP-compatible.

A native ROM/homebrew build, synthetic MIPS instructions, or a PSP menu
does **not** establish commercial game compatibility. Successful PSP
boot, rendering, audio and sustained play must each be verified separately,
with emulator and physical-device evidence labeled clearly.

### Test strategy

- Linux: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
- Native 3DS: `make` in the devkitARM 3DS environment, generating .3dsx
  and packaged .cia where supported.
- Native DS/DSi: `bash scripts/build_nds.sh` inside BlocksDS, generating .nds.
- Upstream PPSSPP: run the pinned ARMv6K interpreter probe workflow and
  fail the build when the audited compatibility patch no longer applies.
- Emulator: Azahar for actual 3DS images, melonDS for actual DS images;
  do not infer hardware performance from emulator screenshots.

Related tracking issue: #33; initial upstream probe PRs: #35 and #36.
