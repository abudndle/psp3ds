# PSP guest memory integration gate

## Verified baseline

The strict devkitARM PPSSPP interpreter probe compiles ten pinned upstream translation units and combines them into an ARM **relocatable** object. This is not a PSP emulator executable.

## Blocker found on 2026-11-14

Attempting to compile upstream `Core/MemMap.cpp` directly on devkitARM failed because `Common/MemoryUtil.h` unconditionally includes POSIX `<sys/mman.h>` outside Windows and Switch. The DS/3DS bare-metal cross toolchain does not provide that interface. Workflow run 43560210568 captures the compiler error. Do not add an empty `sys/mman.h` shim or treat a relocatable link as proof of working memory.

## Next implementation requirements

1. Select the PSP guest memory layout and bounds for each Nintendo target. Existing `psp_guest_bus` and `psp_guest_window` are small, host-tested memory access components, **not** a PPSSPP memory backend.
2. Identify the subset of PPSSPP `Memory::` APIs actually referenced by `Interpreter.cpp`, `MIPS.cpp`, and `MIPSCodeUtils.cpp`, and supply a real target implementation with checked read/write/fetch behavior.
3. Keep guest physical address translation distinct from host virtual memory reservation and executable-page protection. Nintendo hardware must not be assumed to have POSIX `mmap` or PSP-sized RAM.
4. Write a redistributable synthetic MIPS program test that executes through the **upstream PPSSPP interpreter**, with observable registers, loads/stores, branches and exceptions. Existing `test_synthetic_elf_execution` exercises PSP3DS's custom CPU, not PPSSPP.
5. Only then attempt a fully linked target executable and hardware/emulator boot. Commercial PSP game boot and compatibility are separate later milestones.

The workflow excludes `Core/MemMap.cpp` until its host-memory assumptions are replaced or adapted with a functional backend.