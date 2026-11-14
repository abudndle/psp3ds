# Adaptive gameplay workload cutbacks (all Nintendo targets)

## Status and scope

This milestone provides an actual compiled, device-aware **native scene workload throttle** and a guarded **future allocation admission throttle**. The running DS and 3DS UIs currently display only original **synthetic** scenes/GE triangles; there is **no retail PSP game-playing loop yet**. The policies are intended for the later PSP interpreter/HLE/game renderer, but do not prove the later game path is protected until it is routed through them.

## Policy by model

| Hardware mode | Presentation frame budget (policy) | Normal synthetic triangle cap | Reduced | Emergency | Slot-2 effect |
| --- | ---: | ---: | ---: | ---: | --- |
| Original DS / DS Lite | 33.33 ms | 4 | 2 | 1 | None |
| DS + verified Memory Expansion Pak | 33.33 ms | 4 | 2 | 1 | Cold cache only; **no CPU/GPU boost** |
| DSi / DSi XL in DSi mode | 33.33 ms | 8 | 4 | 2 | None |
| Original 3DS / 2DS | 33.33 ms | 8 | 4 | 2 | None |
| New 3DS / New 2DS XL | 16.67 ms | 24 | 12 | 6 | Hardware-gated high clock remains separately managed |

These figures describe workload **limits** and desired presentation intervals for the synthetic renderer; they are NOT PSP game FPS, PSP polygon render rates or measured physical-device performance. Legacy DSi in DS mode takes the DS row. New 3DS high CPU clock is requested only when hardware identification succeeds.

## Pressure levels and recovery

`PspPlayCutbacks` (`include/psp_play_cutbacks.h`, `source/psp_play_cutbacks.c`) uses three states: NORMAL, REDUCED and EMERGENCY. It monitors elapsed presentation-frame time and, on original/New 3DS, the OS-reported APPLICATION free bytes. For future guarded PSP game sessions it may also read the live tracked allocation ratio and observed allocation denials.

- **REDUCED:** Enter after two over-budget frames (~120% of the per-model target), or when free app RAM approaches the protected reserve (<=150% of reserve) for two observations. Present one of every two *synthetic* scenes; cap geometry at half; permit future texture/cache/I/O allocations up to 75% of their hard classes and total future heap admissions to 90%.
- **EMERGENCY:** Enter immediately when a frame exceeds 2x target, free app RAM reaches the reserve, the tracked PSP game session uses >=95% of its hard total, or an allocation is denied/fails. Present one of three *synthetic* frames; reduce GE triangle count to one quarter; cap future texture/cache/I/O class admissions to 50% and future total managed heap admissions to 75%.
- **RECOVERY:** After eight consecutive healthy observations, increase quality by one state. After another eight healthy observations, return to NORMAL. Rapid oscillation is therefore avoided.

Existing guest RAM and GPU allocations are **not forcibly freed** by soft limits. `psp_memory_guard_set_soft_limits()` validates all class/total limits atomically and denies new allocations while usage is above the new soft limits. Actual cache eviction/freeing must be explicitly implemented by future cache owners; a soft limit alone cannot return already allocated bytes to the OS.

**Never** apply presentation skipping to PSP CPU interpreter advancement, guest HLE virtual timing, audio mixing or controller polling. The native DS and 3DS demonstration loops continue polling input and waiting for VBlank even when skipping an optional scene refresh. The PSP virtual time model is independent of wall time. The earlier per-model CPU scheduler slice policy may be capped down during pressure but this changes the **maximum size of a batch of instructions**, not how many PSP instructions are eventually executed.

## Native backends

- DS/DS+Pak/DSi: `platforms/nds/source/main.c` uses BlocksDS `cpuStartTiming(0)`, `cpuEndTiming()` and `timerTicks2usec()` around the synthetic scene loop to obtain measured wall time. No continuous *free heap bytes* reading is invented on DS. Native 2D BG/OAM and the 3D diagnostic are presented at a reduced cadence if frame workload is repeatedly too expensive. The Memory Pak does not change the clock/polygon budget.
- Original/New 3DS: `source/main.c` uses libctru `TickCounter` to measure frame intervals and `osGetMemRegionSize/Used(MEMREGION_APPLICATION)` to detect approaching the 16/24 MiB reserve. `psp_3ds_triangle_draw()` accepts a hard maximum triangle count from the active quality tier; when frames are skipped it waits for VBlank without clearing or swapping out the previously presented image.

## Regression coverage and remaining obligations

`tests/test_play_cutbacks.c` checks all five target defaults, matching DS/DS+Pak CPU and triangles, New 3DS larger workload, two-frame reductions, immediate emergency on severe frame/memory pressure and allocation failure, eight healthy frame recovery, invalid observations, guest-code limits and immutable allocation ownership. Existing `test_memory_guard` and sanitizers test heap cap enforcement and leaked allocation cleanup separately.

**Outstanding before playable game compatibility:** route real PPSSPP guest execution, actual GE 3D texture caches and audio buffers through these policies, implement cache evictions, test real hardware memory/temperature/sustained frame load, ensure no skipped simulation/audio/HLE work and run game load/unload soak tests. Do not claim power or battery savings or smoother game performance until benchmarked on a console.
