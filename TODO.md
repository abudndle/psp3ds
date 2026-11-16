# PSP3DS development TODO

Last reviewed: 2026-11-16. Statuses are evidence-based; a passing build is not proof that a PSP game boots.

## Verified / completed
- [x] Establish portable host CMake core and regression suite.
- [x] Compile ten pinned upstream PPSSPP MIPS interpreter translation units into ARM objects (workflow run 43560915853).
- [x] Relocatable-link PPSSPP interpreter ARM objects with PSP3DS guest bus/window objects (not a runnable executable).
- [x] Implement checked guest-memory bus with optional Slot-2 cold-data window and host tests.
- [x] Implement C ABI PPSSPP-facing memory bridge with 8/16/32/64-bit operations.
- [x] Implement PSP cached/uncached KSEG address aliases for checked bus operations.
- [x] Implement synthetic ELF loader-to-guest-bus instruction fetch regression (not upstream interpreter execution).
- [x] Restrict instruction fetches to hot internal memory, rejecting cold Slot-2 data and invalid fetches.
- [x] Fix null-bus regression test compilation failure; host regression run 43812721259 passed.
- [x] Document why upstream PPSSPP Core/MemMap.cpp cannot be compiled directly with bare-metal devkitARM.

## Priority 0 — actual PPSSPP execution (not yet completed)
- [ ] Audit every direct pointer/inline memory access in pinned upstream Core/MemMap.h and all interpreter call sites; avoid unchecked host pointer dereferences.
- [ ] Implement a genuine bare-metal PPSSPP Memory:: backend with guest address translation, access validation, fault/exception semantics and coherent read/write/fetch.
- [ ] Integrate backend with upstream interpreter; do not claim integration based solely on ARM relocatable object linking.
- [ ] Fully link a runnable minimal upstream PPSSPP interpreter harness, resolving dependencies without POSIX mmap stubs.
- [ ] Execute a redistributable synthetic MIPS ELF through upstream PPSSPP interpreter; assert registers, memory, branch/delay-slot behavior, syscall and invalid-memory exceptions.
- [ ] Run sanitizer and cross-compiled ARM checks on the new backend; document any architecture-specific differences.

## Priority 1 — platform runtime
- [ ] Define realistic memory maps/budgets per old 3DS, new 3DS, DSi, DS, and DS+verified Slot-2 RAM; reject unsupported game requirements.
- [ ] Connect executable loader, HLE syscalls, CPU scheduler, GPU/audio and input to the same PPSSPP-backed runtime.
- [ ] Boot synthetic homebrew on 3DS emulator/hardware; capture logs and evidence.
- [ ] Validate DS/DSi builds separately; Slot-2 probing is not proof of contiguous RAM or PSP compatibility.
- [ ] Add repeatable emulator integration tests and release artifacts for supported targets.

## Priority 2 — compatibility and release
- [ ] Boot and document at least one legally distributable PSP homebrew program end-to-end.
- [ ] Profile CPU, memory and rendering performance; publish target-specific compatibility limits.
- [ ] Test additional PSP titles only with user-provided legally held assets; do not commit proprietary ROMs or game data.
- [ ] Publish honest compatibility matrix, installation instructions, limitations and reproducible release process.
- [ ] Commercial PSP compatibility is **not achieved**; universal compatibility on DS/DSi is not a realistic deliverable.

## CI snapshot
- Host regression run 43812721259: **success**.
- Earlier PPSSPP ARM bridge run 43754415670: **success** (relocatable compilation/link only).
- Other platform workflows may be running; check GitHub Actions for live status rather than treating this snapshot as current.

## Milestone acceptance criteria
A milestone is complete only when its code is committed, relevant tests pass, and evidence matches the stated claim. In particular, an ELF loaded into memory, a relocatable ARM object, or a DS/3DS scaffold boot does **not** count as a PSP game boot.
