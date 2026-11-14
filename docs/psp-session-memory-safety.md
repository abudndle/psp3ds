# PSP game-session memory safety and leak tracking

**Goal:** Prevent the PSP emulation workload from exhausting console RAM and ensure buffers allocated through the session API are released at session end, even if a module load fails. This is a *bounded prototype memory safety mechanism*, not an assertion that retail PSP titles play, or that no memory leak is possible anywhere in the program.

## Model-aware admission limits

| Console/runtime | Physical main RAM | Guest-data allocation class ceiling | Total guarded allocation ceiling | Resource reserve / cold RAM |
|---|---:|---:|---|---|
| DS / DS Lite | 4 MiB | 512 KiB | 75% of prior touched/released scratch heap observation, max 4 MiB physical | 25% of that observed scratch allocation not assigned; no Slot-2 |
| DS / DS Lite + verified Slot-2 RAM | 4 MiB internal + 8 MiB official Pak (separate) | 512 KiB internal | Same DS quota; never count Slot-2 as normal `malloc()` heap | 25% of observed internal scratch; Pak cold data only if opened and independently checked |
| DSi / DSi XL in DSi-enhanced mode | 16 MiB | 2 MiB internal | 75% of previously touched/released scratch heap observation | 25% observed scratch reserve; no Slot-2 |
| Original 3DS / 2DS | 128 MiB physical FCRAM | 32 MiB | Live kernel-reported free APPLICATION memory less 16 MiB reserve | 16 MiB reserve for other application work |
| New 3DS / New 2DS XL | 256 MiB physical FCRAM | 64 MiB | Live kernel-reported free APPLICATION memory less 24 MiB reserve | 24 MiB reserve for other application work |

All rows also enforce separate class ceilings for textures, caches and file I/O based on `psp_platform_budget`; **all classes share the aggregate ceiling**. The class ceiling is NOT guaranteed heap availability. DS and DSi base targets are selected by actual runtime mode; New 3DS is positively detected via `APT_CheckNew3DS`. On original DS, a Slot-2 expansion never increases ARM9 clock speed or hot guest memory. Its cold cache is a separate peripheral, not a heap.

The DS 25% safety margin is relative to one *previously successful temporary allocation*, not a measurement of total free DS RAM. The 3DS 16/24 MiB margin is relative to **observed free application-region memory**, not the physically installed 128/256 MiB. The guarded allocator rejects policies claiming more than that observed memory permits and validates per-model hardware ceilings. A failed, absent or stale probe prevents allocating new session buffers.

## Session ownership API

`include/psp_memory_guard.h` and `source/psp_memory_guard.c` provide `psp_memory_guard_policy_ds/3ds()`, `psp_memory_guard_init()`, `psp_memory_guard_acquire()`, `psp_memory_guard_release()` and `psp_memory_guard_close()`. For each prospective game session:

1. Determine the actual hardware mode and perform the existing OS/heap/peripheral checks. Build the model-aware memory policy and initialize a zeroed `PspMemoryGuard`.
2. Charge **every PSP guest, texture, cache and I/O heap buffer** to exactly one allocation category before attempting memory allocation. The guard rejects invalid sizes, out-of-budget requests, exhausted allocation slots and allocation failures without changing ownership.
3. Explicitly release each owned pointer when no longer needed; freeing an unknown pointer or double-free is rejected. At session end, `close()` returns peak usage, failures and the count/bytes of allocations left outstanding, then **reclaims all remaining tracked allocations**, including buffers not freed after a simulated error.
4. Treat any `reclaimed_allocations > 0` as a **leak-path regression**, not as a passing normal shutdown. Zero leaked records is required for a clean unload. Do not reuse a guard without closing it.

Every allocated buffer has a single tracked owner. The ledger has a fixed 32-buffer limit and requires no dynamic allocation for the records. The code does not forcibly terminate the process, silently overcommit memory, or silently discard guest resource buffers to make space. Out-of-memory is a load/runtime error that future PSP game integration must surface safely.

## PSP executable loads under quota

`psp_guarded_prx_load()` accepts an original user-supplied decrypted `ET_SCE_PRX`-style ELF, **preflights its segments**, calculates the maximum mapped image end, rounds to 4 KiB and allocates the guest memory using the session's GUEST quota. The required image must also remain below the existing 32 MiB mapped PRX loader ceiling. Failed allocation or relocation releases the owned guest buffer; `psp_guarded_prx_unload()` frees it on normal termination.

`psp_prx_image_map_fresh()` works only on a **newly allocated, exclusively owned scratch arena** and skips the old image-sized rollback copy. On error the scratch arena must be discarded as `psp_guarded_prx_load()` does. The existing `psp_prx_image_map_relocated()` maintains whole-memory rollback for shared/preexisting RAM. Other internal relocation staging and third-party allocations still require accounting before a complete runtime is called memory-safe.

Synthetic tests use 4 KiB PRX loads and a **6,320,916-byte mapped image** that matches the approximate Jetpack Joyride executable memory requirement, **without including copyrighted game bytes**. The large image is denied on DS/DS+Pak/DSi under their current conservative hot limits. It is admitted in host simulations of original/New 3DS policies. Admission is NOT game execution or a claim that the real title will run.

## Existing diagnostics and regression tests

- The DS/DSi `.nds` memory diagnostic now performs eight synthetic allocate/free cycles through the guard, validates zero outstanding allocations and reports the result. The Slot-2 peripheral is closed after earlier independent tests; no cold RAM is counted as active once closed.
- The original/New 3DS startup temporary PSP RAM allocation probe uses the guard and explicitly releases the buffer and its session ledger. This is a **temporary allocation test**, not a retained game memory arena.
- The legacy 3DS `EBOOT.PBP` file inspection was converted from allocating an entire file up to 8 MiB to reading a fixed 52-byte header for PBP/ELF identification. General PSP disk/asset loaders remain separate.
- `test_memory_guard`: 128 start/allocate/release cycles on **each of the five models**, forced OOM and quota failures, per-category caps, invalid release/double-free protection, the fixed 32-record limit, and 96 forced-abandon session teardowns with no allocator objects left alive.
- `test_guarded_prx`: PSP ELF mapping and unload on every model, large-image quota rejection on DS-family, safe failure cleanup and simulated abort handling.
- CI: native `.nds`, `.3dsx`, `.cia`, full Linux CTest and AddressSanitizer/UndefinedBehaviorSanitizer. Compile-time checks and synthetic tests do not replace actual-console memory-pressure measurements.

## What this does *not* guarantee

**No program can promise that every possible future gameplay path is free of leaks just from these tests.** The guard tracks only allocations routed through it. Current upstream PPSSPP components are not fully integrated; they might allocate through other allocators. Third-party libraries, GPU VRAM, Citro3D linear allocations, DSi memory-bank configuration, OS/service handles, file descriptors, stacks, firmware resources and the Slot-2 bus require separate ownership auditing. PSP memory paging across cold/SD storage, full kernel/HLE support, emulator gameplay and long real-device soak testing remain incomplete.

Before claiming zero runtime leaks: route all PSP game resource allocators through the guard (and equivalent GPU/handle trackers); capture baseline and post-game RAM/linear-VRAM numbers on every console family; perform repeated load/play/unload sequences and error-path fault injection; require no rising retained memory under AddressSanitizer and real hardware observations.
