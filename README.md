# PSP3DS — experimental PSP emulator research for Nintendo 3DS

**Research prototype only. It does not run commercial PSP games.** Target device is New Nintendo 3DS. PPSSPP is an architectural reference; no PPSSPP source code has been incorporated.

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
