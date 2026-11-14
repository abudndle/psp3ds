# PSP3DS on-demand emulator work (all Nintendo families)

This milestone reduces **unnecessary optional backend work** while preserving required guest CPU/HLE instructions and event timing. It makes the backend-demand mechanism available to the five target profiles and demonstrates the combined path using original **synthetic** MIPS/GE/audio/UMD work. It does **not** claim retail PSP compatibility or that any backend can be omitted when a game genuinely requires it.

## Bounded work units by model

| Runtime profile | GPU/audio/UMD work units per scheduler tick | Existing initial guest CPU slice | Physical memory note |
| --- | ---: | ---: | --- |
| Nintendo DS / DS Lite | 1 | 128 instructions | 4 MiB main RAM |
| DS / DS Lite with independently verified Slot-2 Memory Pak | 1 | 128 instructions | The Pak is a cold data store; no CPU/GPU speed increase |
| Nintendo DSi / DSi XL enhanced mode | 2 | 256 instructions | 16 MiB; legacy DS mode uses the DS row |
| Original 3DS / 2DS | 3 | 2,048 instructions | 128 MiB installed, normally 64 MiB app region |
| New 3DS / New 2DS XL | 6 | 8,192 instructions | 256 MiB installed, normally 124 MiB app region |

These numbers are conservative **software policy quotas**, not instructions-per-second, native GPU capabilities or game FPS. The New 3DS CPU-speedup request is still gated on a successful New hardware detection; the 6-unit backend budget is not a measured throughput figure.

## Demand-activated backends

`psp_lazy_backends.h/.c` manages a fixed-size 3-subsystem work ledger for GPU, audio and UMD (disc I/O):

- Nothing is initialized or called when **all queues are empty**. Idle pumps count as idle activity but do not access backends.
- The frontend explicitly queues one or more **work units** in the subsystem that requires them. Maximum pending units per subsystem: 64. Overflow is rejected without discarding anything.
- Each tick services at most the model's quota and visits nonempty queues **round-robin** to prevent GPU work from starving disc or audio work. Never silently discard requested units to make frame rates look better.
- The optional backend `start()` method is called only once, just before the first requested unit. On a later failure, the failed unit remains queued and retries are possible. `start()` callback implementations must release partial resources before returning an error.
- `stop()` releases every started subsystem **exactly once** on explicit session closure; any pending units cancelled only by the caller's deliberate teardown are reported, not mistaken for completed game work.
- `psp_perf`'s existing optional `initialized_mask` and `active_mask` are synchronized with successfully started backends and remaining requested work. With no requests, neither mask is activated.

Future real PSP GPU and audio devices need to define correct, deterministic work-unit boundaries (submitted PSP GE lists, audio samples to mix and decode, UMD sectors read) and submit them when appropriate. This milestone does not create those submissions from proprietary game instructions.

## Actual guest instruction/event integration

`psp_on_demand_guest.h/.c` combines the existing real compact shared MIPS interpreter/HLE guest scheduler (`psp_runtime_guest_step`) with optional backend service requests:

- **Runnable PSP guest**: execute the bounded real MIPS instruction slice, including bound HLE traps, then service only requested backend units. CPU instructions and PSP virtual time are never skipped because a frame is slow.
- **Waiting guest**: the existing performance scheduler advances PSP virtual HLE time to the next guest event or wake deadline without busy-spinning on unproductive instructions. It can still service explicit audio, graphics or disc requests while the guest is waiting.
- **Failure**: if backend work fails, return a distinct error. Already executed guest instructions remain committed; failed optional units remain queued. No fictitious PSP CPU rollback.
- **Performance cutbacks**: the existing three-level policy can lower a subsequent CPU slice ceiling; it cannot change or omit already required guest work. Presentation cutbacks remain separate from simulation, input or audio execution.

### Native status

- On **original/New 3DS**, the startup diagnostic uses a synthetic syscall followed by real MIPS ADDIU execution through this combined API. It verifies zero optional backend calls on the first CPU step, explicitly requests one GPU, audio and UMD unit, then checks all three services start, run and close. Measured runtime is shown on device, not inferred from CI.
- On **DS/DS+verified Pak/DSi**, the native diagnostic compiles the standalone portable lazy backend and checks that eight idle pumps initialize nothing, followed by exactly two requested units each for GPU, audio and disc work. Actual shared PSP MIPS/HLE runtime execution is **not** newly wired into the BlocksDS main loop by this milestone; the preexisting synthetic MIPS CPU diagnostic remains.
- Host `test_lazy_backends` checks the 1/1/2/3/6 quotas, fair scheduling, lazy initialization, failure retries, overflow, explicit cancellation and cleanup. Host `test_on_demand_guest` checks actual MIPS instruction/PC/register results, HLE wait/wake event timing, no extra backend calls when idle, backend failures preserving queued work and compatibility with adaptive cutbacks on all five target policies.

## Further optimization milestones

Connect the real PPSSPP guest PSP HLE services, GE list handler, audio engine and disc/ISO loader to these explicit work requests. Perform correct demand-only texture/sector decoding, track actual GPU/audio memory, reclaim closed sessions, and profile CPU time and audio continuity on real hardware. The original DS has far less native CPU/GPU throughput than PSP; demand-driven work cannot turn it into a fully compatible real-time PSP emulator. PSP retail game boot, graphics and audio output are not yet established.
