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
