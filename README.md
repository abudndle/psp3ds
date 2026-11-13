# PSP3DS — experimental PSP emulator research for Nintendo 3DS

**Research prototype only. It does not run commercial PSP games.** Target device is New Nintendo 3DS. PPSSPP is an architectural reference; no PPSSPP source code has been incorporated.


## Shared GitHub repository: 3DS + DS/DSi-enhanced roadmap

This repository now documents both the existing **Nintendo 3DS PSP3DS** target
and a **future BlocksDS Nintendo DS / DSi-enhanced** port. It is one repository,
not two different source histories. Platform plans live in
[3DS](platforms/3ds/README.md) and [DS/DSi](platforms/nds/README.md).
See the [3DS/DS library inventory](docs/PLATFORM_LIBRARIES.md) for dependencies,
package discovery, reusable PSP core modules and a migration plan.
The DS/DSi port is **not built or tested** yet; no .nds artifact exists.

## Current components
- PSP PBP, plain ELF32/MIPS, ISO9660 and CSO header identification (not decoding ISO/CSO games).
- Bounds-checked loader for plain, unencrypted, fixed-address ELF32/MIPS ET_EXEC segments, including BSS and executable entry-point validation; no encrypted retail executable support, PRX relocation or PSP firmware APIs.
- MIPS/Allegrex integer interpreter subset including arithmetic, branches, delay slots, branch-likely annulment, unaligned memory access, HI/LO, basic CP0 diagnostic state and syscall/break/overflow traps.
- Minimal native 3DS Homebrew Launcher entry point (reads and inspects sdmc:/psp3ds/EBOOT.PBP up to 8MiB).
- Twelve portable host regression test suites, exercised via GitHub Actions.
- Initial HLE syscall dispatcher with explicitly bound 20-bit syscall IDs and deterministic system-time services (`sceKernelGetSystemTimeLow`, `sceKernelGetSystemTimeWide`, `sceKernelGetSystemTime` equivalents). Tests cover unknown syscalls, invalid guest pointers and delay-slot resumption. This is NOT PSP firmware compatibility.

- Prototype import linker for loader-supplied PSP module-library names and function NIDs. It validates and patches guest JR RA/SYSCALL stubs, rejects unsupported imports, and avoids partial writes for invalid batches. Current tests use illustrative NIDs, **not verified PSP firmware NIDs**. Parsing of already-loaded PSP function import tables is now available; automatic PRX loading, relocation and thread scheduling remain unimplemented.

- PSP PRX import table parser for already-loaded 20-byte and 24-byte `.lib.stub` records, extracting library names, function NIDs and addresses of import stubs with checked guest-memory bounds. Variable imports, executable relocation and decryption are deliberately unsupported. Synthetic tests integrate parser output with the existing HLE import linker; this is not yet a complete real-world PRX loader.

- **Milestone 7C (partial):** discover and validate loaded PSP SceModuleInfo via the `.rodata.sceModuleInfo` ELF32/MIPS section. Extract module name, GP, import/export ranges and hand its imported function range to the parser from 7B. Requires an ELF section table and already-mapped guest memory; **PRX relocations, encrypted retail binaries and automatic loading are not implemented**. Synthetic regression tests include malformed boundaries and metadata.

- **Milestone 7D (first relocation subset):** stand-alone uncompressed PSP PRX relocation record handling for `R_MIPS_NONE`, `R_MIPS_32`, and `R_MIPS_26`. Supports explicit load-segment bases, address validation, duplicate-target rejection, and atomic batch application. Does **not** parse relocation sections from a PRX file or implement `HI16`/`LO16`, compressed `PRXRELOC2`, or full relocatable module loading. Ten host tests pass locally with sanitizers.\n\n- **Milestone 7E:** conservative uncompressed MIPS `R_MIPS_HI16` / `R_MIPS_LO16` relocation support, including sign-extended low addends, carry adjustment, several `LUI` HI16 records paired with one `ADDIU` LO16, rejection of mismatched or missing pairs, and atomic batch failure. Standalone `ADDIU`/`ORI` LO16 supported. 12 host suites passed locally with AddressSanitizer and UndefinedBehaviorSanitizer. Still lacks compressed `PRXRELOC2`, relocation stream discovery, other relocation types and commercial-game execution.

- **Milestone 7F (compressed PSPREL2 subset):** a fail-closed decoder for PSP `PT_SCE_PSPREL2` (0x700000A1) streams, supporting bounded flag/type tables, segment selectors, base-setting commands, relative/extended/absolute offsets and `R_MIPS_NONE`, `R_MIPS_32`, `R_MIPS_26` without LO16 sidecar information. It expands validated entries into the existing atomic relocation engine. Encodings needing `HI16/LO16` sidecars, alternate flag semantics, firmware-specific type mappings and broader relocation streams are *not supported*. Tested using synthetic fixtures; not verified on real PSP game modules.\n\n## Host validation
```sh
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```

## 3DS homebrew build
Install devkitPro 3DS toolchain, devkitARM and libctru, then run `make`. This build path has **not been verified** with actual toolchain or hardware.

## Important limitations
Most PSP kernel HLE, remaining compressed PSPREL2 modes, PRX relocation types and variable-import handling, threads, VFPU/FPU, GPU, audio and commercial-game support are NOT implemented. The syscall binding table can be generated from parsed loaded-image function import entries, but the program loader does not yet discover or relocate real PSP PRX modules. Source-level host tests do not demonstrate a playable emulator.

## Milestones
1. [x] Header identification
2. [x] Plain ELF32 loader subset
3. [x] Expanded integer instruction subset
4. [x] Unaligned memory and branch-likely subset
5. [x] CPU exception and CP0 diagnostic groundwork
6. [x] Initial syscall dispatcher and basic guest pointer translation (limited time services only)
7. [~] Import linker, function import-table parsing, ELF module-info discovery, uncompressed relocation subset and limited PSPREL2 decoder implemented; broader compressed relocation, variable imports, NID catalog, threads and filesystem remain pending
8. [ ] GPU, audio and controls
9. [ ] Commercial game execution and hardware optimization

The 5-minute GitHub Actions schedule runs host verification only; it does not autonomously write new emulator code, and GitHub may delay or skip scheduled runs. No game or firmware assets are included.

Reference: https://github.com/hrydgard/ppsspp (GPL-2.0-or-later).

## Performance milestone: selective work and bounded caching (2026-11-12)

This milestone adds a portable **performance infrastructure layer**; it does not
turn this research prototype into a playable PSP emulator. Its host tests are
synthetic, and speed improvements on New 3DS have **not been measured**.

- `psp_perf_schedule` and `psp_perf_idle_hle`: ordered timer events, waiting-
  thread time jumps and matching deterministic HLE clock advancement. No
  complete PSP thread scheduler or interrupt controller exists yet.
- `psp_perf_request_subsystem`: first-use initialization bookkeeping for
  GPU, audio and UMD subsystems. Actual GPU/audio implementations are pending.
- `psp_perf_run_slice`: bounded CPU instruction execution with early trap/idle
  return; hot-PC profiling for identifying interpreter optimization candidates.
  This is **not** a JIT, IR translator or ARM machine-code generator.
- HLE dispatcher: cache the most recently resolved syscall binding, validating
  its ID on each use. Unknown syscalls still raise traps.
- `psp_perf_texture_seen` and `psp_perf_invalidate_textures`: generation-
  checked texture-state cache metadata. No PICA200 renderer is implemented.
- `psp_iso_open`, `psp_iso_read`, `psp_iso_cached`: basic ISO9660 signature
  check, bounded 2048-byte sector reads from a file and a four-sector LRU cache.
  There is no ISO9660 directory walker, CSO decoder or UMD HLE yet.
- `psp_perf_profile`: conservative defaults; no unverified per-game hacks.
  Optional frameskip and quality policy remain future renderer work.

To reproduce host tests:
```sh
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```
To reproduce ASan + UBSan tests:
```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-sanitize --parallel
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-sanitize --output-on-failure
```
Both commands passed locally for **15/15 suites**. The GitHub Actions
workflow is configured to rerun normal and sanitizer tests. No native
`.3dsx` or commercial PSP game execution has been verified.

- **New 3DS CPU speed:** 3DS startup calls libctru `osSetSpeedupEnable(true)` and restores it on exit. This is an unverified native build path; no hardware speedup measurement has been recorded. Reference: https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/os.h

## Next milestone: CSO v1 streaming (2026-11-13)

- `psp_cso_open` validates the CSO v1 header, a 2,048-byte sector size, the
  block-offset index, and file bounds. CSO v2 and ZSO are rejected explicitly.
- `psp_cso_read` reads only the requested sector, uses raw DEFLATE for
  compressed blocks, and copies stored blocks directly. The block index stays
  on disk rather than being allocated in RAM.
- `psp_cso_cached` reuses the existing four-sector cache. **Reset `PspPerf`
  before opening a different image** so cached sectors from a previous image
  cannot be reused incorrectly.
- `test_cso` covers compressed and plain blocks, cache hits, out-of-range
  sectors, unsupported versions, and malformed offsets.
- The host build requires zlib. The 3DS Makefile links `-lz` but the native
  devkitPro toolchain and 3DS runtime have not been verified.
- This is a sector reader, not UMD kernel integration, CSO v2 support, a
  commercial-game loader, or a speed benchmark.

Verification: `ctest` passed 16/16 host suites and 16/16 under
AddressSanitizer/UndefinedBehaviorSanitizer locally.

## Jetpack Joyride (USA) CSO compatibility diagnostic — 2026-11-13

The user-provided CSO was inspected locally without uploading game content:
- CSO v1; 70,592 sectors of 2,048 bytes (144,572,416 uncompressed bytes).
- An actual commercial CSO was found with a zero `header_size` field, unlike
  the usual 24. The v1 reader now accepts 0 or 24 when the canonical
  index begins at offset 24; `test_cso` covers both cases.
- ISO9660 level-1 directory traversal and bounded file reads are implemented
  in `psp_disc_find` and `psp_disc_read` for ISO/CSO sector callbacks.
- `psp_disc_probe <path-to-cso-or-iso>` reads basic `PARAM.SFO` metadata and
  inspects the first four bytes of `PSP_GAME/SYSDIR/EBOOT.BIN`.
- Observed game identifiers: `TITLE=Jetpack Joyride`, `DISC_ID=NPUZ00292`,
  `PSP_SYSTEM_VER=6.60`, EBOOT magic `~PSP` (encrypted/signed PSP executable).
  This executable is **not supported** by PSP3DS's plain ELF loader.
- Tests passed locally: 17/17 host and 17/17 ASan/UBSan suites. The probe
  also completed successfully against the supplied CSO.
- **No native 3DS build or Azahar session occurred**, because the relevant
  emulator and devkitPro toolchain are not available in this environment.
  No title screen, gameplay or playable performance is claimed.
- Original game data is not included in this public repository or test fixtures.

After building on a host, run:
```sh
./build-host/psp_disc_probe "/path/to/Jetpack Joyride (USA).cso"
```
The next blocker is legal homebrew/commercial executable loading and PSP
kernel emulation, followed by GPU/audio and a native 3DS build/test pipeline.

## Shared Nintendo DS/DSi target and ROM/save package folders (2026-11-13)

- `platforms/nds/Makefile` adds an experimental BlocksDS ARM9-only native
  target in **this same GitHub repository**. Its `source/shared_core.c`
  compiles the existing portable PSP MIPS interpreter. The DS entry point
  detects `isDSiMode()` and runs a synthetic two-instruction CPU test.
  This does not run commercial PSP games.
- Install BlocksDS and run `make -C platforms/nds` to attempt the
  `psp3ds_nds.nds` build; no native DS/DSi build was verified here.
- Release archives include `psp/roms/` and `psp/roms/save/`. No game
  files are distributed. A new portable `psp_game_save_dir` helper validates
  game IDs (for instance `NPUZ00292`) and returns
  `psp/roms/save/NPUZ00292`; it does not create directories or implement
  emulated PSP save-data APIs.
- The core library inventory and platform plans are documented at
  [docs/PLATFORM_LIBRARIES.md](docs/PLATFORM_LIBRARIES.md).
- Portable Linux verification passed 18/18 host tests and 18/18 ASan/UBSan
  tests. Native .3dsx/.cia/.nds compilation, melonDS/Azahar runtime tests,
  and playable Jetpack Joyride are unverified.

## Shared-platform and demand-driven runtime milestone (2026-11-13)

This GitHub repository contains both the 3DS emulator research target and a
BlocksDS-based DS/DSi shared-core build **scaffold**, not a playable PSP port.
The source remains centralized in `source/` and `include/`.

- `scripts/build_nds.sh`: stages the BlocksDS `rom_arm9_only` template,
  copies the portable MIPS interpreter and uses
  `platforms/nds/main.c` to exercise a synthetic `ADDIU` instruction.
  Libnds `isDSiMode()` reports whether the launcher is in DSi mode.
  No native `.nds` build or hardware test has been verified.
- The 3DS Makefile now follows devkitARM two-pass compilation rules.
  GitHub Actions `native-3ds.yml` is configured to attempt a real
  `.3dsx` build and upload its artifact. There is **no confirmed successful
  CI build** as of this change. A `.cia` requires further metadata and
  packaging validation and is **not yet produced**.
- `psp_perf_read_sector` now invalidates the 4-sector cache automatically
  when its reader or context changes. Call `psp_perf_media_changed()` if
  reopening a different game in-place with the same context pointer.
  This is an integrity and demand-driven caching fix, not a measured
  commercial-game speed increase.
- `scripts/package_release.py` creates `psp/roms/save/` inside the ZIP,
  with no game or copyrighted firmware files. The `psp_game_save_dir()`
  helper only generates validated per-title paths; PSP savedata HLE is
  not yet implemented.
- The 3DS dependency inventory and optional packages are documented
  in `docs/PLATFORM_LIBRARIES.md`; do not install every portlib, because
  unused libraries increase complexity without making emulation faster.
- **Local Linux verification:** 18/18 host tests and 18/18 ASan/UBSan tests
  passed. The BlocksDS script reports a missing SDK because the cross
  compiler is not installed locally. A DS entry-point syntax-only check
  using a mock header passed, but it is *not* a native libnds compilation.
  Neither melonDS nor Azahar was available for an actual emulator run.

Next substantive work: validate native 3DS/DS builds and start integrating
virtual PSP UMD access and full kernel scheduling. Hardware-speed claims and
commercial game compatibility remain unverified.

## Demand-driven performance cutbacks (2026-11-13)

The portable core now exposes `psp_perf_run_demand`: when the guest is
**waiting**, advance virtual time only as far as the next pending event,
thread wake or caller deadline, while keeping the HLE virtual clock in sync.
Return event tokens explicitly to the caller (never discard interrupts).
When the guest is **runnable**, execute a bounded interpreter slice without
dropping instructions, faults, delay slots or PSP behavior.

`psp_perf_set_subsystem_active` and `psp_perf_subsystem_active` allow
future GPU, audio and UMD backends to avoid optional work until activated,
separate from one-time lazy initialization. No real GPU/audio backend is
implemented yet, and no hardware performance increase is claimed.

New regression test `demand` checks event order, idle time jumps, wake
semantics, zero guest instructions while waiting, and subsystem gating.
All **19/19 Linux host tests and 19/19 ASan/UBSan tests passed locally**.

Next required integration: connect guest kernel wait/signal operations and
hardware interrupts to the event scheduler, and profile real New 3DS
workloads. This scheduling API alone cannot run encrypted retail PSP games.

## Demand-driven execution milestone (2026-11-13)

The experimental portable PSP runtime now includes `psp_demand_run()`
(`source/psp_demand.c`), a **bounded** driver for the existing event queue,
CPU interpreter and HLE virtual clock:

- While a guest thread is waiting, skip directly to the next scheduled
  event, wake-up timestamp or deadline; preserve guest elapsed time in HLE.
- Invoke host-provided callbacks on due events. Callbacks may resume guest
  execution; their implementation is still the caller's responsibility.
- Execute CPU instructions only for runnable guest state and never skip
  CPU instructions, memory accesses, traps, or branch delay slots.
- Bound instruction/event work per call and report instruction counts,
  event counts and virtual microseconds spent idle.
- Turn CPU hot-block profiling off by default using
  `psp_perf_enable_profiling()` to avoid unnecessary per-instruction work.
  Profiling can be reenabled when collecting performance traces.
- Existing backend activity flags and disc/texture caches remain optional,
  and do not fake guest-visible hardware completion.

Verification: **19/19 Linux host tests and 19/19 ASan/UBSan tests passed**
in the working Linux environment, including `test_demand_runtime`.
These tests do not benchmark PSP games. The library is not wired to a
full PSP kernel thread scheduler or real GPU/audio pipeline yet.
Commercial PSP games (including Jetpack Joyride) remain unplayable.

The next true performance milestones require runnable guest workloads,
PSP thread/interrupt HLE, rendering/audio backends, native .3dsx/.nds
build verification, and measurements on actual hardware/emulators.

## Shared native builds, ROM catalog and release verification (2026-11-13)

This milestone **does not make commercial PSP games playable**.

- `scripts/build_nds.sh` now stages BlocksDS's official ARM9-only template
  without retaining SDK example sources. It builds a shared PSP MIPS CPU
  demonstration only. The new `.github/workflows/native-nds.yml` runs in
  the official `skylyrac/blocksds:slim-latest` container and uploads a
  nonempty `.nds` if compilation succeeds. A real cross-build and melonDS
  boot have **not yet been confirmed**.
- The existing devkitARM workflow targets a native `.3dsx`; its success
  and an Azahar run remain unverified.
- `scripts/build_cia.sh` requires a real compiled `.3dsx` and externally
  installed `cxitool` plus `makerom`. It fails on missing prerequisites
  rather than producing a fake `.cia`.
- `psp_catalog_scan()` enumerates ISO, CSO, and PBP names in `psp/roms`
  without entering the `save` subdirectory. The 3DS entry point lists
  discovered names from `sdmc:/psp/roms`. Detection is **not gameplay**.
- `scripts/package_release.py` packages the requested empty
  `psp/roms/save/` structure and any nonempty native binaries that
  actually exist. It never includes the user-supplied Jetpack Joyride CSO
  or any other proprietary game content. `psp_game_save_dir()` can generate
  per-game paths, but PSP savedata kernel services are still missing.
- **Linux verification:** 22/22 ordinary tests and 22/22 sanitizer tests
  passed. The BlocksDS staging path was also tested with a mock build
  command, **not** a real Nintendo cross-compiler. Neither melonDS nor
  Azahar is installed in this environment.

Actual gameplay for Jetpack Joyride is still blocked by its encrypted
`~PSP` executable and unimplemented PSP kernel/GPU/audio services.

## Automatic GitHub uploads (2026-11-13)

`.github/workflows/auto-publish.yml` runs on **every push to main**,
manual workflow dispatch, and a daily 04:17 UTC schedule. It compiles and runs
all portable host tests before producing two artifacts:
- `psp3ds-source.zip`: a Git-tracked source snapshot containing approved
  source, tests, scripts, documentation and workflows.
- `psp3ds-sd-layout.zip`: the SD-card folder layout with `psp/roms/save/`;
  native binaries appear only if they have actually been built in that job.

`scripts/verify_release.py` validates both archives and rejects PSP ROM
images, save files, signing secrets and invalid/empty included binaries. Game
files and original PSP assets must **never** be uploaded to this public repo.
The new `.gitignore` also ignores common private files and generated builds.

GitHub Actions artifacts are retained for 30 days. Native `.3dsx` and `.nds`
build workflows upload their own binaries **only after a successful native
build**; a `.cia` is not available until the packaging toolchain is verified.

IMPORTANT: GitHub Actions automatically uploads **already-committed** sources
and verified artifacts. It does NOT author emulator code or upload uncommitted
files from ChatGPT, this Linux workspace, or a user's computer. Those sources
must first be committed. Workflow execution success is not confirmed merely
because the configuration has been committed.

## Virtual PSP UMD device milestone — 2026-11-13

The portable `PspUmd` device in `include/psp_umd.h` and
`source/psp_umd.c` integrates previously separate ISO and CSO readers
under a single on-demand device API:

- `psp_umd_mount` selects ISO/CSO using file content and validates the
  ISO9660 primary volume descriptor before accepting a mount.
- `psp_umd_sector` decodes just the requested 2 KiB sector and reuses
  the bounded four-sector cache. Disc changes invalidate cached sectors.
- `psp_umd_find` and `psp_umd_read` expose bounded filesystem lookups
  and byte-range reads from the currently mounted image.
- `psp_umd_unmount` closes the underlying file and rejects later reads.
- `test_umd` tests ISO → another ISO → CSO replacement, file reads,
  boundary failures, cache reuse and unmount behavior.

**Verified in Linux:** 24/24 ordinary host suites and 24/24 ASan/UBSan
suites pass. The previously supplied Jetpack Joyride CSO remains readable
by `psp_disc_probe` (disc ID `NPUZ00292`; encrypted `~PSP` EBOOT),
but **cannot execute** in the present emulator. No game assets are committed.

This is **not yet PSP UMD kernel HLE, full ISO9660 support, encrypted
executable loading, a playable PSP game, a native 3DS/DS build, or proof
of measured FPS gains**. The next milestone is connecting device operations
to real guest kernel file handles and thread/blocking semantics.

## PSP file-handle HLE milestone (2026-11-13)

The unified read-only UMD device now has a bounded guest-file layer:
`include/psp_io.h` and `source/psp_io.c`. The `PspIo` handle table
supports 8 simultaneous files; guest filenames must be `disc0:/...`
paths, never host paths, and opening requires PSP's `PSP_O_RDONLY=0x0001`.
It explicitly rejects writes, parent-directory traversal, missing media,
invalid handles and unsupported devices.

The `PspHle` dispatcher can attach a `PspIo` context and explicitly
bind synthetic/import-resolved syscall identifiers for PSP-like
`sceIoOpen`, `sceIoRead`, `sceIoLseek32` and `sceIoClose`.
Guest string and buffer pointers are validated against mapped guest RAM;
a failed buffer check does not advance the file position. Disc swaps
increment the UMD media generation, making previously open handles stale.
The implementation is intentionally read-only: **savedata, memory-stick I/O,
real PSP NID auto-binding, async reads, and firmware-specific error codes
are not supported**. It does not automatically execute a retail EBOOT.

Verification: 25/25 Linux CTest host tests and 25/25 ASan/UBSan suites
passed locally. The new `test_io` covers imported MIPS `SYSCALL`
execution, independent descriptors, file seek/EOF, guest-pointer bounds,
permission rejection and disc-swap invalidation. The uploaded
Jetpack Joyride CSO is still discoverable by the disc probe, but its
encrypted `~PSP` executable remains unplayable.

Reference for guest I/O conventions:
https://pspdev.github.io/pspsdk/group__FileIO.html

## PSP real NID import linking milestone (2026-11-13)

The PSP prototype can now associate four confirmed `IoFileMgrForUser`
function imports with the existing read-only `disc0:` file HLE:
`sceIoOpen` (0x109F50BC), `sceIoRead` (0x6A638D83),
`sceIoLseek32` (0x68963324), and `sceIoClose` (0x810C4BC3).
`psp_imports_link_known` uses the existing all-or-nothing guest
stub linker to allocate internal syscall IDs and bind those HLE services.

The mapping is deliberately small and fail-closed: unrecognized library names,
unknown NIDs, unsupported PSP services and invalid/overlapping stubs reject
the batch before modifying guest memory or bindings. PSP syscall IDs are
still dynamically assigned per import; the NIDs are not syscall IDs.

`test_real_nids` checks all four mappings, validates emitted
`jr $ra` + `syscall` instructions, executes a real MIPS stub through HLE,
and confirms unknown NIDs leave guest stubs and binding counts unchanged.
**Linux host suites: 26/26 passed; ASan/UBSan suites: 26/26 passed.**

This is **not** automatic PRX import discovery, encrypted executable
decryption, complete PSP I/O semantics, savedata support, or commercial
game playback. The next integration target is retrieving parsed PSP import
metadata from an already loaded guest image and linking only verified
supported functions.

References:
- https://pspdev.github.io/psplibdoc/modules/iofilemgr.prx_IoFileMgrForUser.html
- https://pspdev.github.io/pspsdk/group__FileIO.html

## Loaded-module import auto-linking milestone (2026-11-13)

The host-verified `psp_module_link_known()` API combines the existing
plain-ELF module-info discovery, already-mapped PRX import-table parsing,
and verified `IoFileMgrForUser` NID-to-HLE mappings. The caller supplies
an **already mapped** and (where necessary) relocated guest image. Unknown
or malformed imports reject the entire patch batch: no partial guest
stub edits or HLE binding changes. The optional module-info output is
cleared on failure. This does not decrypt retail `~PSP` executables or
automatically load PRX modules.

Synthetic test `module_link` verifies discovery from ELF section headers,
real PSP I/O NIDs extracted from guest .lib.stub records, atomic
linking, and unchanged guest memory for unsupported NIDs and malformed
records. **27/27 host tests and 27/27 ASan/UBSan suites passed on Linux**.
The integration is not yet called from a fully featured PSP loader, and
it does not make Jetpack Joyride playable.

Next milestone: safely integrate this operation into the supported
plain-ELF loading path and extend verified kernel/thread HLE operations.
Native .3dsx/.nds/.cia and emulator launches remain unverified.

## Integrated plain-ELF boot and import-link milestone (2026-11-13)

`psp_boot_plain_elf()` in `source/psp_boot.c` now connects the existing
plain, unencrypted ELF32/MIPS ET_EXEC segment loader to loaded-module
metadata discovery and supported PSP import NID linking.

- A syntactically valid executable must map into bounded guest RAM.
- Its `.rodata.sceModuleInfo` and PRX `.lib.stub` function imports must be
  present, valid, and supported by the known NID table.
- Import failures leave the CPU's register state untouched, clear the
  reported outputs, and do not partially patch PSP HLE bindings or stubs.
  **ELF segments may already be loaded into RAM when import linking fails**:
  callers must discard/reset guest RAM before another boot attempt.
- Only after successful validation does the CPU start at the ELF entry point.
  This prototype does **not** implement PRX relocations, encrypted EBOOT
  decryption, kernel initialization, or an executable launcher UI.

The synthetic `test_boot` fixture verifies ELF program-header loading,
PSP module-info discovery, two real IoFileMgr NIDs, MIPS execution through
the imported syscall stub, unsupported-import refusal, and invalid-memory
load rejection.

**Local Linux verification:** 28/28 host tests and 28/28 ASan/UBSan
regression tests passed. No successful native 3DS/DS build, melonDS/Azahar
launch, or Jetpack Joyride gameplay is claimed. This API is an
integration milestone, not a finished PSP emulator.

## Linux toolchain provisioning milestone (2026-11-13)

The shared repository now includes `.github/workflows/linux-tools.yml` to
provision toolchains **on GitHub-hosted Ubuntu runners** using the installation
method already present in `abudndle/mii-maker-ds`:

- `scripts/install_blocksds_ubuntu.sh`: bootstrap Wonderful Toolchain,
  enable the BlocksDS package repository, install `blocksds-toolchain`,
  verify SDK files and attempt a real `.nds` shared-core demo build.
- A separate job uses the official `devkitpro/devkitarm:20260610`
  container and attempts to compile `psp3ds.3dsx`.
- On **manual workflow dispatch** with `install_emulators=true`,
  `scripts/install_linux_emulators.sh` retrieves the official Linux x86-64
  melonDS AppImage and installs Azahar from Flathub. It verifies the
  installations, **not** GUI emulation or gameplay.
- The workflow is also triggered automatically by changes to these
  installation scripts or the workflow itself, but the optional, large
  emulator job is skipped for regular pushes.
- `test_installers` is a portable CI syntax/safety check for the scripts.

These dependencies are installed only on ephemeral GitHub runners. They
cannot be copied into ChatGPT's Linux container automatically and do not
persist on your computer. Local installation was attempted but the Linux
container cannot resolve external package hosts. No root permissions, system
DNS settings or security restrictions were bypassed.

**Local verification:** 29/29 Linux host suites passed after including the
installer syntax test. A successful native `.nds` or `.3dsx` compilation,
Azahar/melonDS GUI launch and executable gameplay **are not yet confirmed**.
Windows `.exe` bootstrap files, the macOS devkitPro `.pkg`, and the
Azahar `.flatpakref` from the user are not suitable standalone Linux
installers. No encrypted/decrypted commercial game files are uploaded to
GitHub.

Official SDK containers:
- https://blocksds.skylyrac.net/docs/setup/docker/
- https://hub.docker.com/r/devkitpro/devkitarm/tags

## Automated homebrew CIA packaging (2026-11-13)

The native 3DS GitHub Actions workflow now attempts a **real homebrew CIA**
artifact after successful 3DSX compilation. On the network-enabled Linux
runner, `scripts/install_cia_tools.sh` builds `cxitool` from
`devkitPro/3dstools` (`cxi-stuff` branch) and `makerom` from
`3DSGuy/Project_CTR`. It installs the tools to `~/.local/bin`, then invokes
`scripts/build_cia.sh psp3ds.3dsx psp3ds.cia`. The
`tests/test_cia_output.py` check examines the CIA container header and
section bounds before publication.

Successful runs upload two separate artifacts: `psp3ds-native-3dsx` and
`psp3ds-native-cia` at
https://github.com/abudndle/psp3ds/actions/workflows/native-3ds.yml .
The setup uses no game ROMs, firmware dumps or private Nintendo keys.
A plausible CIA header does not guarantee installation or hardware execution.

**Verification status:** A successful GitHub Actions native CIA build is
not yet confirmed. Local Linux tests cannot verify an actual 3DS CIA without
the native toolchain and a runnable cxitool/makerom installation.

## Emulator screenshot evidence milestone (2026-11-13)

Both GitHub screenshot workflows now invoke `scripts/verify_emulator_capture.py`
in an always-run step after attempting the actual emulator capture. This
generates `screenshots/evidence.json` even if no screenshot was produced.
The manifest contains the GitHub commit SHA, emulator name, verified
source-executable SHA-256 digest (when available), and PNG SHA-256 plus
dimensions when a nonblank, readable screenshot exists.

Capture status is either `captured` (a valid real-window image was saved)
or `not_captured` with a reason. The manifest explicitly says
`game_boot_verified: false`: showing a working emulator window is not
evidence of playable PSP gameplay. Real images and diagnostics remain
available from the existing Azahar and melonDS GitHub Actions artifact
upload jobs.

The portable `tests/test_capture_manifest.py` checks missing, uniform,
valid and corrupt PNG inputs and protects the fail-closed evidence format.
The updated local PSP3DS source tree passed **30/30 CTest tests**, including
this new test. No successful native screenshot artifact is asserted until
the GitHub runner output is obtained and inspected.

## Single-thread token wait/signal groundwork (2026-11-13)

The portable scheduler now exposes `psp_perf_wait_signal(perf, token, deadline_us)`
and `psp_perf_signal_wait(perf, token)`. This adds a deterministic,
**single-guest-thread** wait point: a nonzero matching token resumes the
waiting CPU without jumping the guest clock, while an absolute timeout
resumes it when idle advancement reaches its deadline. Unrelated scheduled
events are still returned to the caller without being discarded. Invalid
tokens, an already-waiting guest, and expired deadlines are rejected.

The `wait_signal` CTest suite exercises matching and mismatched signals,
idle event delivery, timeout, and invalid arguments. An isolated
representative scheduler implementation passed with `-Wall -Wextra -Werror`
and AddressSanitizer/UndefinedBehaviorSanitizer on Linux. **The complete
repository CTest suite and native Nintendo builds have not been verified
for this change.**

This is **not** firmware-compatible PSP semaphores, thread switching,
interrupt dispatch, priority scheduling, or playable PSP game support.
No new PSP syscall NIDs are bound. Next work requires multi-thread state
and explicit wait reason/results before integration with real PSP kernel HLE.

## Bounded multi-task scheduler groundwork (2026-11-13)

`psp_threads.h` and `psp_threads.c` introduce an allocation-free pool of up to
eight portable PSP *task metadata records*. Runnable selection chooses the
lowest numeric priority, rotating fairly by stable task ID within equal
priorities. Each task can wait on a nonzero signal token with an absolute
deadline, or sleep until a finite deadline. Signals wake one matching
waiter in priority/creation order; advancing the scheduler clock clears
expired waits and records why they woke. Stopped task slots can be reused
without reusing a live task ID.

`test_threads` checks priority ordering, round-robin ties, unmatched
signals, wake reasons, independent task timeouts, monotonic clock
validation, capacity limits and slot reuse. The standalone module passed
Linux strict `-Wall -Wextra -Werror -pedantic` testing in an optimized build
and with UndefinedBehaviorSanitizer.

**Important:** this is the next data-model milestone, not a working
PSP kernel thread scheduler. The existing interpreter and demand runtime
still use the prior single-thread path. There is no per-thread saved
`PspCpu` context, CPU preemption, PSP thread HLE, context-switch cost,
kernel interrupt routing, or game compatibility from this change.
A future integration must select a task, preserve/restore its CPU state,
and coordinate its clock with `PspPerf` before claiming live multi-thread
execution. A full repository CTest, native 3DS build or emulator launch
has not yet been verified for this change.

## Cooperative per-thread MIPS CPU contexts (2026-11-13)

`psp_thread_cpu.h` and `source/psp_thread_cpu.c` connect the bounded task
scheduler to a **single live** `PspCpu` instance. Each created task receives
an independent initialized snapshot with its own entry PC, stack register,
general registers, HI/LO, branch delay state, trap/CP0 state and halt/fault
state. Switching tasks saves the prior live CPU and restores the selected
task; all tasks deliberately share one mapped guest RAM and HLE dispatcher.

`psp_thread_cpu_step()` selects the next runnable task using the existing
priority/round-robin model and runs exactly one MIPS/HLE interpreter step.
The adapter refuses nonrunnable selections, validates the mapped RAM
identity and disallows misaligned or out-of-bounds entry/stack positions.
A stopped task's CPU slot can be reused without letting its old live
context overwrite the replacement. Tests cover cross-thread register
isolation, branch delay preservation, wait/signal selection and slot reuse.

**Verification:** a strict Linux C11 test build with UndefinedBehaviorSanitizer
passed against the new adapter and a representative CPU/HLE test harness.
The full PSP3DS library CTest suite, native `.3dsx` compilation and
Azahar execution have **not** been verified for this change.

**Limitations:** per-instruction cooperative switching is a host-side
prototype, **not** PSP kernel task switching. There is no guest-thread
creation or scheduling HLE, firmware synchronization semantics, PSP
context-switch timing, interrupt-driven preemption, floating/VFPU register
state, full system RAM layout or PSP commercial game compatibility.

## Experimental guest-facing thread services (2026-11-13)

The HLE dispatcher can now optionally attach a `PspThreadCpu` instance using
`psp_hle_attach_threads()`. It accepts four **manually bound, synthetic**
services, using the existing bounded task pool and cooperative CPU adapter:

- `PSP_HLE_THREAD_CREATE`: a0=entry PC, a1=stack pointer, a2=priority
  (0..255), returns positive task ID or -1.
- `PSP_HLE_THREAD_WAIT`: a0=token (0 allows timeout-only sleep),
  a1=absolute low-32-bit scheduler deadline, returns 0 if queued or -1.
- `PSP_HLE_THREAD_SIGNAL`: a0=nonzero token, returns woken task ID,
  0 if no match, or -1.
- `PSP_HLE_THREAD_EXIT`: marks the current task stopped and returns
  0 or -1. The next cooperative step chooses another runnable task.

These are *not actual PSP kernel NIDs or PSP-compatible syscall signatures*.
Import linking does not recognize them automatically. Guests must be
explicitly assigned interpreter syscall IDs by a trusted test harness.
The attachment is optional; missing/mismatched CPU contexts fail closed.
`test_thread_hle` covers a MIPS syscall-trap sequence that creates,
waits, signals and stops synthetic guest tasks.

**Limitations:** calls return immediately, including WAIT (there is no
firmware-compatible delayed syscall return), a1 deadlines are only
32 bits, tasks share one guest RAM, and there is no PSP semaphore,
priority inversion handling, interrupt dispatch, FPU/VFPU state,
thread-stack allocation, preemption, or commercial PSP executable loading.
An actual PSP game such as Jetpack Joyride is still not playable.
The new test is registered in CTest; a complete host build, sanitizer
run and native 3DS compilation are **not yet confirmed** for this change.

## Plain PSP ELF loading validation hardening (2026-11-13)

The unencrypted, fixed-address MIPS ELF32 `ET_EXEC` loader now rejects
overlapping `PT_LOAD` guest-memory ranges, zero-length mapped segments,
misaligned entry points, and entry points not fully backed by four
bytes of file-supplied executable instructions. Previously an entry
address in the zero-filled BSS tail could be treated as executable,
or a later segment could overwrite code loaded by an earlier segment.
All segments are checked **before guest RAM is modified**, preserving
fail-closed behavior for these malformed images.

The `loader_safety` CTest covers overlapping ranges in both program-header
orders, entry-address alignment and file-backed coverage, intact RAM and
zeroed result fields after rejection, and adjacent valid data/BSS
segments. The existing `loader` and `boot` suites also run in CI.

This is strict validation of the supported **plain ELF32/MIPS ET_EXEC**
subset, not PSP encrypted `EBOOT.BIN` decryption, PRX relocation,
firmware-compatible executable loading, or commercial PSP gameplay.

## PRX ELF relocation stream discovery (2026-11-13)

`psp_reloc_image_apply()` now finds PSP program-header relocation
sections in an **already mapped**, unencrypted ELF32/MIPS module. It
recognizes `PT_SCE_PSPREL` (0x700000A0; uncompressed eight-byte
records) and `PT_SCE_PSPREL2` (0x700000A1; the existing restricted
compressed decoder). The caller supplies one mapping record per ELF
program header so segment selectors remain stable. The helper checks
program-header/file bounds and matching PT_LOAD memory mappings
before applying relocations.

Applying multiple relocation sections is transactional at the guest-RAM
level: a failure in a later section rolls back earlier changes. Working
memory is bounded to one guest-RAM snapshot (maximum 32 MiB) plus
the existing decoder buffers. The `reloc_image` CTest exercises both
relocation formats, combined success, late failure/rollback, malformed
headers, missing streams, invalid mapping, and running an unaffected
MIPS instruction after applying relocations.

This is an **opt-in relocation application stage**, not an automatic
PRX boot path. The module has to be mapped into RAM before calling it;
it does not allocate segments, relocate module metadata, handle PSP
encrypted executables, support full PSPREL2 modes, or implement actual
PSP game boot. Repeatedly relocating the same image is not supported.

## Transactional mapped-ELF boot with PSP relocations (2026-11-13)

`psp_boot_mapped_relocated_elf()` composes the previously separate
relocation stream discovery and known-NID module import linking steps,
then initializes the MIPS CPU at the validated mapped entry. It works
only on **already-mapped, plain ELF32/MIPS ET_EXEC** images with an
explicit program-header-to-guest mapping. It does not allocate or move
PT_LOAD segments, and assumes section metadata addresses already match
the guest mapping. All supported relocation streams are discovered and
applied automatically when this new opt-in boot API is called.

The sequence is transactional. A relocation or module-discovery/import
failure restores the entire guest RAM snapshot and leaves the original
CPU, HLE binding table, output module information and stream count
unchanged/zero as documented. RAM is bounded to 32 MiB; the snapshot
is freed before the function returns. New `boot_relocated` CTest covers
successful relocation-to-MIPS execution, imported HLE stub patching,
failure during relocation, failure after relocation during import
linking, invalid mapped entry points and invalid segment descriptions.

**Not implemented:** automatic mapping of actual PSP ET_REL/PRX files,
relocation of all module metadata, encrypted PSP EBOOT loading, PSP
firmware thread compatibility, and running commercial PSP games.
