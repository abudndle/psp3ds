# Primary commercial compatibility target: Jetpack Joyride (PSP / PlayStation minis)

**Project priority:** Jetpack Joyride is the **first commercial PSP title** PSP3DS
will investigate, boot-test, and optimize for on **New Nintendo 3DS** hardware.
**Original Nintendo 3DS and 3DS XL** are also explicitly targeted, with
separate device-memory and CPU-performance acceptance measurements.
**Nintendo DS, DS Lite with a Slot-2 Memory Expansion Pak, and DSi** are
also requested targets, tracked in a separate,
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

## Game-driven emulation scope

The project targets a repeatable Jetpack Joyride run on all five hardware
profiles below. Use the same known-good PSP game revision and shared runtime
where possible. New 3DS provides the first integration checkpoint. Each
profile needs its own gameplay and performance verification.

| Hardware profile | Device-specific verification |
| --- | --- |
| New 3DS family | Establish the first repeatable gameplay run and measure peak RAM, frame time and audio underruns. |
| Original 3DS / 3DS XL | Repeat the run at the original CPU and memory limits. |
| DSi / DSi XL | Verify DSi-mode memory use, ARM9 throughput, DS GPU output and audio timing. |
| DS / DS Lite with Slot-2 Pak | Verify gameplay with guest data split across main RAM and the separately addressed expansion Pak. |
| DS / DS Lite without a Pak | Verify gameplay within main RAM, measuring the cost of streaming assets and keeping resident data small. |

For each profile, verify cold boot to the game's title, input-driven ascent
and descent, graphics and audio during a run, scoring, collision and restart.
Record the game revision, build, hardware mode, peak memory and frame timing.
When a profile misses its budget, record the measured blocker and use it to
guide the next optimization.

For each boot or gameplay checkpoint, capture the first unsupported guest
operation locally. Record the checkpoint, guest PC, operation category
and identifier (instruction word, import library/NID, or GE opcode),
plus the expected observable behavior. Reproduce that behavior in a small
synthetic fixture, implement the required PSP semantics in the shared
runtime, then rerun the game to confirm progress. Keep game bytes and raw
dumps private. Treat an unobserved feature as deferred and an observed
feature with uncertain semantics as a blocker.

| Area | Implementation scope |
| --- | --- |
| CPU and kernel | Execute the Allegrex instructions the game reaches and implement the PSP imports it calls. Preserve return values, memory effects and wake-up ordering. |
| Graphics | Translate the display lists, vertex layouts, texture formats and state the game uses. Submit through the existing native backend. |
| Storage and input | Serve the asset, directory and controller operations exercised by the selected game revision. |
| Audio | Implement the sample formats, codecs and buffer timing the game exercises, using native output. |

Defer PSP firmware boot, the PSP system menu, peripheral emulation,
additional media formats and compatibility work for other titles until a
target needs them. Preserve working shared functionality. Unsupported
operations must report a diagnostic and stop safely. Optimize measured
frame-time and memory bottlenecks independently on each profile.

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

## Original Nintendo 3DS / 3DS XL compatibility track

**Goal:** Run the same PSP minis Jetpack Joyride edition through PSP3DS on
original Nintendo 3DS and original 3DS XL, not just the New 3DS family.
These systems have less processing headroom and memory than New 3DS hardware;
a New 3DS success must not be reported as original-3DS compatibility.

- Approximate original 3DS platform specifications are 128 MiB FCRAM and
  dual-core ARM11 at about 268 MHz, compared with New 3DS's increased CPU
  capacity. Available memory to a homebrew process is lower than total FCRAM
  and must be measured, not assumed.
- Validate the same `.3dsx`/homebrew code path on original 3DS and 3DS XL,
  including ordinary (non-New3DS-only) CPU configurations.
- Run genuine PSP interpreter, translation, memory, audio, graphics and
  input benchmarks on hardware; publish measured frame time and RAM usage.
- If performance or allocated memory is insufficient, mark this target
  `blocked_by_hardware` rather than claiming playable because a New 3DS
  build exists.

| Checkpoint | Original 3DS objective | Evidence required |
| --- | --- | --- |
| O0: Hardware boot | Compile and launch native homebrew on original 3DS/3DS XL | Exact hardware model, actual executable image, logs |
| O1: Resource budget | Profile CPU interpreter speed, usable RAM and guest-memory allocations | Device-specific measured values, no inferred FPS |
| O2: Video/audio and controls | Validate translated PSP display, input latency and audio | Hardware recordings, synchronization statistics |
| O3: Jetpack Joyride boot | Execute an authorized PSP minis copy to its own UI | Game-specific trace and authenticated gameplay image |
| O4: Interactive playability | Control a run, observe score, collision and restart at usable speed | Repeatable hardware runs and performance measurements |

**Original 3DS status: not_tested; not playable.**

## Nintendo DS / DSi secondary compatibility track

**Goal:** Investigate running the PSP minis Jetpack Joyride edition on both
Nintendo DS and DSi. The desired outcome is gameplay, but **PSP binary
emulation on these devices is not currently demonstrated and may be
technically infeasible at acceptable performance**. DS and DSi hardware
must be assessed independently.

- **Original DS / DS Lite:** approximately 4 MiB main RAM and a 67 MHz
  ARM9; a full PSP software emulator would face severe RAM, instruction
  translation, rendering, and sound constraints.
- **Original DS / DS Lite with Slot-2 Memory Expansion Pak:** the official
  Nintendo DS Browser Memory Expansion Pak contains approximately **8 MiB
  additional RAM** accessed through the Game Boy Advance cartridge slot,
  making about 12 MiB total across **separate memory regions** rather than
  a single contiguous heap. The accessory adds **no CPU acceleration**.
  Homebrew must explicitly identify, probe and map the memory; successful
  detection and read/write tests are required before treating it as usable.
  An original-DS-shaped Pak works in the original DS and DS Lite; the
  shorter DS-Lite-shaped Pak does **not** fit the original DS.
- **DSi / DSi XL:** approximately 16 MiB RAM and a faster 133 MHz ARM9
  when using DSi features; this helps but does not establish feasibility
  of a retail PSP minis executable.
- **DSi cannot use the Slot-2 memory pack** because it lacks the GBA
  cartridge slot. The 3DS family likewise has no compatible Slot-2 port.
- Both DS and DSi have 256×192 screens versus the PSP's 480×272 output; any faithful
  adaptation needs explicit display scaling/cropping/layout decisions.
- The existing `.nds` GitHub artifact is a synthetic demonstration,
  **not** Jetpack Joyride running on either DS model.

| Checkpoint | DS / DSi objective | Evidence required |
| --- | --- | --- |
| D0: Hardware feasibility | Profile memory, CPU budget, display and input constraints separately on DS, DS+Expansion Pak, and DSi | Real-device or accurate emulator measurements, no invented FPS |
| D0a: Expansion Pak detection | Identify official Slot-2 RAM, probe the correct 8 MiB address region and validate safe reads/writes | Actual DS/DS Lite hardware with named compatible Pak, measured effective memory; no automatic heap assumptions |
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

Evidence for the targets is recorded separately:
`new3ds: not_tested`, `old3ds: not_tested`, `ds: not_tested`,
`ds_slot2_ram: not_tested`, `dsi: not_tested`.
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

**Jetpack Joyride: New 3DS, original 3DS/3DS XL, original DS/DS Lite, DS with Slot-2 RAM expansion, and DSi are all not tested / not playable.**

The PSP3DS repository contains experimental MIPS execution, limited ELF/PRX
relocation, a synthetic thread-service prototype, and verified homebrew
Azahar-display captures. It does not have proven retail PSP minis loading,
PSP GE rendering, full firmware imports, retail boot, or commercial gameplay.

**Next engineering checkpoint:** J0/J1 — introduce a safe, redacted PSP minis
package-type probe and public synthetic fixtures to distinguish supported
ELF/PRX structures from containers or encryption the loader cannot handle.
No proprietary game file is required for that public CI work.

### Hardware reference notes

- Nintendo DS Memory Expansion Pak variants and compatible DS models:
  https://en-americas-support.nintendo.com/app/answers/detail/a_id/7619/
- Nintendo DSi operations manual (Slot-2 expansion accessories unsupported):
  https://assets.nintendo.eu/image/upload/v1635390257/NAL/Support/DSiOperationsManual.pdf
- Original 3DS hardware/CPU specifications:
  https://www.3dbrew.org/wiki/Hardware
