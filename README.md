# PSP3DS — experimental PSP emulator research for Nintendo 3DS

**Research prototype only. It does not run commercial PSP games.** Target device is New Nintendo 3DS. PPSSPP is an architectural reference; no PPSSPP source code has been incorporated.

## Current components
- PSP PBP, plain ELF32/MIPS, ISO9660 and CSO header identification (not decoding ISO/CSO games).
- Bounds-checked loader for plain, unencrypted, fixed-address ELF32/MIPS ET_EXEC segments, including BSS and executable entry-point validation; no encrypted retail executable support, PRX relocation or PSP firmware APIs.
- MIPS/Allegrex integer interpreter subset including arithmetic, branches, delay slots, branch-likely annulment, unaligned memory access, HI/LO, basic CP0 diagnostic state and syscall/break/overflow traps.
- Minimal native 3DS Homebrew Launcher entry point (reads and inspects sdmc:/psp3ds/EBOOT.PBP up to 8MiB).
- Seven portable host regression test suites, exercised via GitHub Actions.
- Initial HLE syscall dispatcher with explicitly bound 20-bit syscall IDs and deterministic system-time services (`sceKernelGetSystemTimeLow`, `sceKernelGetSystemTimeWide`, `sceKernelGetSystemTime` equivalents). Tests cover unknown syscalls, invalid guest pointers and delay-slot resumption. This is NOT PSP firmware compatibility.

- Prototype import linker for loader-supplied PSP module-library names and function NIDs. It validates and patches guest JR RA/SYSCALL stubs, rejects unsupported imports, and avoids partial writes for invalid batches. Current tests use illustrative NIDs, **not verified PSP firmware NIDs**. Real PRX import-table parsing and PSP thread scheduling remain unimplemented.

## Host validation
```sh
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
```

## 3DS homebrew build
Install devkitPro 3DS toolchain, devkitARM and libctru, then run `make`. This build path has **not been verified** with actual toolchain or hardware.

## Important limitations
Most PSP kernel HLE, real PRX module import parsing, threads, VFPU/FPU, GPU, audio and commercial-game support are NOT implemented. The syscall binding table is generated from loader-provided synthetic import records in tests, not from actual PSP game import tables. Source-level host tests do not demonstrate a playable emulator.

## Milestones
1. [x] Header identification
2. [x] Plain ELF32 loader subset
3. [x] Expanded integer instruction subset
4. [x] Unaligned memory and branch-likely subset
5. [x] CPU exception and CP0 diagnostic groundwork
6. [x] Initial syscall dispatcher and basic guest pointer translation (limited time services only)
7. [~] Import-linker groundwork completed for synthetic import records; real PRX import parsing, NID catalog, threads, filesystem and module HLE remain pending
8. [ ] GPU, audio and controls
9. [ ] Commercial game execution and hardware optimization

The 5-minute GitHub Actions schedule runs host verification only; it does not autonomously write new emulator code, and GitHub may delay or skip scheduled runs. No game or firmware assets are included.

Reference: https://github.com/hrydgard/ppsspp (GPL-2.0-or-later).
