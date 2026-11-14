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

## New 3DS higher-power policy (experimental)

`source/psp_runtime_tuning.c` picks a tested, bounded software policy by
hardware family. It **does not** allocate its cache caps or implement
real PSP game execution. The New 3DS runtime exclusively requests
`osSetSpeedupEnable(true)` **after** a successful
`APT_CheckNew3DS` model check; the request is released on shutdown.
This requests the higher New 3DS ARM11 speed but the OS API has no success
return, so the app does not claim a measured actual clock speed.

| Target profile | Initial guest instruction slice | Max slice | Triangle demo budget | 128-insn benchmark rounds |
| --- | ---: | ---: | ---: | ---: |
| Original DS / DS Lite | 128 | 256 | 4 | 4 |
| DS / DS Lite with verified Slot-2 Memory Pak | 128 | 256 | 4 | 4 |
| DSi / DSi XL | 256 | 512 | 8 | 8 |
| Original 3DS / 3DS XL / 2DS | 2,048 | 4,096 | 8 | 16 |
| New 3DS / New 3DS XL / New 2DS XL | 8,192 | 16,384 | 24 | 64 |

A 3DS startup synthetic MIPS interpreter benchmark runs with the selected
round count and uses a system tick timer to report elapsed time. The
portable controller can grow or shrink its *next* CPU slice with hysteresis,
clamped to the model's cap. These are prospective emulator scheduling
limits and a synthetic CPU benchmark. PSP virtual timing remains independent;
never advance PSP virtual time just because native hardware runs faster.

In the optional **Y** key triangle demo, original 3DS submits eight
triangles to PICA200 and New 3DS submits 24. This demonstrates model-aware
bounded hardware GPU work, **not** measured PSP 3D game frame rate or
PPSSPP graphics compatibility. The DS/DSi triangle renderer uses native
libnds hardware and does not request the New 3DS boost. The Memory Pak
provides cold data storage only after verification, not additional
ARM9/geometry-engine performance.

Before adjusting real PSP game work quotas, measure elapsed CPU interpreter
time per guest-instruction slice, frame time, HLE/GE overhead, audio underruns
and memory usage on hardware. Regression tests check policy caps and safe
adaptation; `.nds` / `.3dsx` / `.cia` builds alone do not establish gameplay.

## HLE-aware real interpreter scheduling bridge (experimental)

The `psp_runtime_guest_step()` API (`include/psp_runtime_guest.h`,
`source/psp_runtime_guest.c`) uses the existing
`psp_perf_run_demand()` interpreter/HLE scheduler with the current
per-device CPU instruction budget. It does not replace PPSSPP's upstream
interpreter or implement commercial PSP game boot.

- **Normal runnable slices** execute the selected number of actual MIPS
  instructions, retaining the existing syscall dispatch, branching, and
  interpreter exceptions. Host timing is sampled through an optional
  monotonic callback; only an *entire successful slice* can affect the
  next budget. A nil, backwards or overflowing measurement cannot tune.
- **Guest idle or waiting** is delegated to `psp_perf_run_demand`: virtual
  scheduler/HLE time advances to the pending wake/event, without inventing
  instructions. An event, partial slice, trap or syscall boundary never
  triggers performance-policy feedback.
- **Native old/New 3DS** calls an original synthetic two-stage fixture at
  startup in `psp_3ds_guest_demo.c`. The first instruction dispatches a
  bound synthetic system-time syscall through the HLE code. The next step
  executes the model's selected ADDIU instruction slice, measures real
  device elapsed time with libctru `TickCounter` and reports its next
  budget. The New 3DS speedup request is still hardware-gated.
- **DS, DS+verified Memory Expansion Pak and DSi** continue using the
  same portable policy rules in native builds. The new real-HLE bridge is
  integrated into the 3DS diagnostic and host regression suite only; a
  DS-native linkage and runtime smoke are future work. Slot-2 RAM never
  increases interpreter clock speed.
- `test_runtime_guest` tests executed MIPS instruction totals across
  all five policy profiles, HLE syscalls, scheduler idle events, unchanged
  virtual clock during runnable work, correct adaptation, faults and
  non-monotonic timer rejection.

This connects a **compact shared interpreter**, not PPSSPP's full upstream
guest-memory/HLE implementation. It is an incremental emulator core
milestone; further milestones must replace/bridge upstream Allegrex and
VFPU state, load actual PSP homebrew, schedule guest threads and render
real GE display lists. Compilation and host tests are not evidence of
real game compatibility or native-device frame rates.

## PSP ET_SCE_PRX image mapping (Jetpack Joyride compatibility increment)

The `psp_prx_image_map_relocated()` function maps **already decrypted**
PSP ELF32/MIPS `ET_SCE_PRX` (`0xFFA0`) program images. Unlike ordinary
`ET_EXEC`, the image and entry addresses are relative to the caller's
selected PSP guest address base (usually `0x08800000`). The parser copies
PT_LOAD data into guest memory, zero-fills BSS, validates nonoverlap and the
executable entry, applies existing supported PT_SCE_PSPREL/PSPREL2
relocations, and initializes the guest PC **only on success**.

The code preserves preexisting guest RAM and CPU registers if segment
validation or relocation fails, and requires at most a 32 MiB guest buffer.
This is **not** a decrypted PSP game loader or a general module
relocator: it does not rewrite section tables or firmware import names,
resolve unknown firmware NIDs, initialize VFPU/threads, or execute a
game. Existing ET_EXEC and restricted PRX mapping paths stay separate.

`tools/psp_prx_probe.c` is a host utility built with `cmake` alongside the
regression suite. For a **locally held, lawfully obtained decrypted ELF**:

```sh
cmake -S . -B build
cmake --build build --target psp_prx_probe
./build/psp_prx_probe /path/to/decrypted-EBOOT.elf
```

The utility maps relocations and prints *only* guest entry, image size,
segment and relocation-stream counts. It **does not execute game code**,
print decrypted bytes or upload anything to CI. No original game assets
are committed.

Private validation of the user-supplied decrypted **Jetpack Joyride** ELF
showed 2 PT_LOAD segments, a 6,320,916-byte mapped guest image,
83,187 relocations in one PSPREL stream, a mapped entry at
`0x08955018`, valid unique relocation targets and zero-initialized BSS.
This validation was an independent *offline Python memory-mapping
dry run*, not a claim that the new C loader has already run this
private executable on a Nintendo device. The C implementation is
regression-tested using a separate **original synthetic ELF fixture**.

Resource implications: the mapped Jetpack image alone exceeds an
original DS's 4 MiB main RAM. An Expansion Pak is not faster CPU RAM;
this PRX mapping stage is therefore primarily applicable to 3DS and
New 3DS testing, not a proof of Jetpack gameplay on DS or DSi.
The game also imports approximately 145 PSP functions across 19
libraries, exceeding current prototype NID/HLE coverage. Subsequent
work must support module-info rebasing and import resolution before
guest execution, then VFPU, graphics, audio and PSP kernel services.
