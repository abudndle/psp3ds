# PSP3DS — experimental PSP emulator research for Nintendo 3DS

**Research prototype only. It does not run commercial PSP games.** Target device is New Nintendo 3DS. PPSSPP is an architectural reference; no PPSSPP source code has been incorporated.

## Current components
- PSP PBP, plain ELF32/MIPS, ISO9660 and CSO header identification (not decoding ISO/CSO games).
- Bounds-checked loader for plain, unencrypted, fixed-address ELF32/MIPS ET_EXEC segments, including BSS and executable entry-point validation; no encrypted retail executable support, PRX relocation or PSP firmware APIs.
- MIPS/Allegrex integer interpreter subset including arithmetic, branches, delay slots, branch-likely annulment, unaligned memory access, HI/LO, basic CP0 diagnostic state and syscall/break/overflow traps.
- Minimal native 3DS Homebrew Launcher entry point (reads and inspects sdmc:/psp3ds/EBOOT.PBP up to 8MiB).
- Nine portable host regression test suites, exercised via GitHub Actions.
- Initial HLE syscall dispatcher with explicitly bound 20-bit syscall IDs and deterministic system-time services (`sceKernelGetSystemTimeLow`, `sceKernelGetSystemTimeWide`, `sceKernelGetSystemTime` equivalents). Tests cover unknown syscalls, invalid guest pointers and delay-slot resumption. This is NOT PSP firmware compatibility.

- Prototype import linker for loader-supplied PSP module-library names and function NIDs. It validates and patches guest JR RA/SYSCALL stubs, rejects unsupported imports, and avoids partial writes for invalid batches. Current tests use illustrative NIDs, **not verified PSP firmware NIDs**. Parsing of already-loaded PSP function import tables is now available; automatic PRX loading, relocation and thread scheduling remain unimplemented.

- PSP PRX import table parser for already-loaded 20-byte and 24-byte `.lib.stub` records, extracting library names, function NIDs and addresses of import stubs with checked guest-memory bounds. Variable imports, executable relocation and decryption are deliberately unsupported. Synthetic tests integrate parser output with the existing HLE import linker; this is not yet a complete real-world PRX loader.

- **Milestone 7C (partial):** discover and validate loaded PSP SceModuleInfo via the `.rodata.sceModuleInfo` ELF32/MIPS section. Extract module name, GP, import/export ranges and hand its imported function range to the parser from 7B. Requires an ELF section table and already-mapped guest memory; **PRX relocations, encrypted retail binaries and automatic loading are not implemented**. Synthetic regression tests include malformed boundaries and metadata.

## Host validation
```sh
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```

## 3DS homebrew build
Install devkitPro 3DS toolchain, devkitARM and libctru, then run `make`. This build path has **not been verified** with actual toolchain or hardware.

## Important limitations
Most PSP kernel HLE, PRX relocation and variable-import handling, threads, VFPU/FPU, GPU, audio and commercial-game support are NOT implemented. The syscall binding table can be generated from parsed loaded-image function import entries, but the program loader does not yet discover or relocate real PSP PRX modules. Source-level host tests do not demonstrate a playable emulator.

## Milestones
1. [x] Header identification
2. [x] Plain ELF32 loader subset
3. [x] Expanded integer instruction subset
4. [x] Unaligned memory and branch-likely subset
5. [x] CPU exception and CP0 diagnostic groundwork
6. [x] Initial syscall dispatcher and basic guest pointer translation (limited time services only)
7. [~] Import linker, function import-table parsing and ELF module-info discovery implemented; relocation, variable imports, real NID catalog, thread scheduling and filesystem remain pending
8. [ ] GPU, audio and controls
9. [ ] Commercial game execution and hardware optimization

The 5-minute GitHub Actions schedule runs host verification only; it does not autonomously write new emulator code, and GitHub may delay or skip scheduled runs. No game or firmware assets are included.

Reference: https://github.com/hrydgard/ppsspp (GPL-2.0-or-later).
