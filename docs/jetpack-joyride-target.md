# Primary commercial compatibility target: Jetpack Joyride (PSP / PlayStation minis)

**Project priority:** Jetpack Joyride is the **first commercial PSP title** PSP3DS
will investigate, boot-test, and optimize for on **New Nintendo 3DS** hardware.
**Nintendo DS and DSi are also requested targets**, tracked in a separate,
high-risk feasibility path below. A native DS/DSi demake would be a different
implementation from running the PSP executable. These are *targets*, not claims
that the game currently boots or is playable.

## Target identity and legal boundaries

- Game: **Jetpack Joyride** (2012), PSP **PlayStation minis** edition, rather than
  the mobile, native PS Vita, or PS3 editions.
- Developer: Halfbrick; PSP minis publisher: Beatshapers.
- North American PSP minis release: **November 20, 2012**.
- Publicly catalogued US product ID: **NPUZ00292**. Validate the actual title
  metadata and build revision when authorized test media becomes available;
  never infer file layout, encryption, module names or NIDs from the ID alone.
- Authoritative release references:
  - https://blog.playstation.com/2012/11/20/jetpack-joyride-flies-to-psp-ps-vita-ps3-today/
  - https://www.halfbrick.com/blog/jetpack-joyride-hits-ps3-ps-vita-psp
  - https://gamefaqs.gamespot.com/psp/690983-jetpack-joyride/data

Use only a user-authorized copy and legally obtained test inputs. Keep retail
game data, executable binaries, keys and decrypted proprietary assets outside
the public repository, issues, pull requests, logs and CI artifacts.
Never bypass copy protection to generate a public test fixture. Public CI
should use **original synthetic/homebrew test fixtures**, not commercial files.

## Compatibility checkpoints (in dependency order)

| Checkpoint | Objective | Evidence required |
| --- | --- | --- |
| J0: Target identification | Confirm actual PSP minis package, title ID, version and format from authorized metadata | Redacted local diagnostic manifest, no copyrighted payload |
| J1: Executable discovery | Identify the PSP executable/container type and report unimplemented format/encryption without crashing | Fail-closed test and structured loader diagnostic |
| J2: ELF/PRX memory loading | Load and relocate supported original-format modules with correct memory ownership | Real-format *synthetic* regression corpus, ASan/UBSan |
| J3: Kernel services | Capture and implement the precise PSP kernel NIDs needed during authorized boot traces | Deterministic import/syscall tests, no silent unknown-NID stubs |
| J4: Graphics | Implement PSP GE/display/texture/presentation sufficient for an actual game frame | Genuine Azahar and real New 3DS framebuffer captures |
| J5: Input and timing | Map one-button jetpack control and PSP audio/video/event synchronization | Repeatable in-game input trace and timing measurements |
| J6: Save/audio | Implement storage, save lifecycle and audio behavior used by the title | Repeatable save-reload and audio tests |
| J7: First game launch | Reach Jetpack Joyride's own title/game UI from an authorized commercial copy | Unmodified title and verified capture, confirmed game identity |
| J8: First interactive run | Start a run, control ascent/descent, score distance, restart after collision | Input-driven gameplay capture and crash-free repeatability |
| J9: Device viability | Evaluate actual New 3DS memory, frame rate, input latency and stability | Measured hardware logs and reproducible performance report |

## Nintendo DS / DSi secondary compatibility track

**Goal:** Investigate running the PSP minis Jetpack Joyride edition on both
Nintendo DS and DSi. The desired outcome is gameplay, but **PSP binary
emulation on these devices is not currently demonstrated and may be
technically infeasible at acceptable performance**. DS and DSi hardware
must be assessed independently.

- **Original DS / DS Lite:** approximately 4 MiB main RAM and a 67 MHz
  ARM9; a full PSP software emulator would face severe RAM, instruction
  translation, rendering, and sound constraints.
- **DSi / DSi XL:** approximately 16 MiB RAM and a faster 133 MHz ARM9
  when using DSi features; this helps but does not establish feasibility
  of a retail PSP minis executable.
- Both have 256×192 screens versus the PSP's 480×272 output; any faithful
  adaptation needs explicit display scaling/cropping/layout decisions.
- The existing `.nds` GitHub artifact is a synthetic demonstration,
  **not** Jetpack Joyride running on either DS model.

| Checkpoint | DS / DSi objective | Evidence required |
| --- | --- | --- |
| D0: Hardware feasibility | Profile memory, CPU budget, display and input constraints separately on DS and DSi | Real-device or accurate emulator measurements, no invented FPS |
| D1: Minimal PSP runtime | Benchmark synthetic PSP MIPS execution and memory translation on both device classes | Deterministic .nds instrumentation and correctness tests |
| D2: Graphics and audio feasibility | Benchmark PSP-style 2D/texture compositing, scaling and audio within DS limits | Measured rendering/audio performance |
| D3: Authorized game boot attempt | Determine whether the real PSP minis package can progress beyond loading | Verified game-specific execution trace; avoid false boot claims |
| D4: Interactive gameplay gate | Attempt a genuine run with ascent/descent, obstacles, scoring and restart | Real device or melonDS evidence specific to the game |
| D5: Alternative decision | If direct PSP execution is not viable, document a separately authorized **native DS/DSi demake** scope | Explicit asset/code rights, distinct implementation and acceptance tests |

**Decision rule:** If full PSP execution exceeds available DS/DSi memory or
frame-time budgets, mark this path `blocked_by_hardware` with evidence.
Do not quietly substitute a similar-looking native minigame for the retail
PSP executable. A DS-native demake/port requires appropriate rights to the
game assets/code, must be clearly labeled as a separate deliverable, and
cannot be claimed to be commercial PSP binary compatibility.

Evidence for the two device classes is recorded separately:
`new3ds: not_tested`, `ds: not_tested`, `dsi: not_tested`.
The next DS engineering checkpoint is **D0**, not a gameplay claim.

### Public milestone completion rules

1. A merged PR must have reproducible synthetic host regression evidence, with
   sanitizer checks; native homebrew builds remain green where applicable.
2. **Boot evidence** must identify the game's own rendered screen or verified
   guest execution, not merely the emulator's outer window or a synthetic marker.
3. A compiled `.3dsx` or `.cia` alone does **not** count as Jetpack Joyride
   compatibility. Generic PSP homebrew progress is not evidence that this
   specific title works.
4. Do not claim *playable* until J8 is demonstrated on the target platform
   and a reproducible run can be completed, with J9 hardware constraints
   openly reported.
5. Keep a compatibility entry with statuses `not_tested`, `blocked`,
   `boots`, `in_game`, `playable`; begin at **not_tested**. Use only
   evidence-backed transitions, recording game revision and test environment.

## Current status (2026-11-13)

**Jetpack Joyride: New 3DS, original DS and DSi all not tested / not playable.**

The PSP3DS repository contains experimental MIPS execution, limited ELF/PRX
relocation, a synthetic thread-service prototype, and verified homebrew
Azahar-display captures. It does not have proven retail PSP minis loading,
PSP GE rendering, full firmware imports, retail boot, or commercial gameplay.

**Next engineering checkpoint:** J0/J1 — introduce a safe, redacted PSP minis
package-type probe and public synthetic fixtures to distinguish supported
ELF/PRX structures from containers or encryption the loader cannot handle.
No proprietary game file is required for that public CI work.
