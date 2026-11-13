# DS/DSi constrained-memory and graphics strategy for PSP3DS

This is a **feasibility and engineering plan**, not proof that Jetpack Joyride
PSP minis can be emulated on DS or DSi. The PSP's GPU instruction stream
cannot be sent directly to DS 2D hardware. A graphics translation layer is
required, and original DS CPU resources may still make playable binary
emulation infeasible.

## Measured hardware constraints to design around

- Original DS/DS Lite: **4 MiB shared main RAM**, plus ITCM/DTCM, small
  shared WRAM and **656 KiB total VRAM** in banks with restrictive mappings.
  Not all of these regions can be used as one contiguous RAM buffer.
- Two hardware 2D engines (one per screen), up to four background layers
  and 128 sprites per engine; main engine also offers fixed-function 3D.
- Official external Slot-2 Memory Expansion Pak: **8 MiB separate,
  comparatively slow external storage**, accessed as 16/32-bit units.
  It does not improve ARM9 CPU speed and is absent on DSi.
- The DS screen is **256 × 192**. Original PSP renders **480 × 272**;
  translation must choose crop, downscale, letterbox or redesigned HUD,
  not silently drop important gameplay information.
- The DSi has additional internal RAM and a faster CPU in DSi mode, but
  DSi mode and real-device benchmarks are separate acceptance gates.

## Prioritized design experiments

| Priority | Engineering approach | Why it could help | Verification |
| --- | --- | --- | --- |
| P0 | Profile high-water memory usage and ARM9 frame-time buckets | Avoid pretending a ROM fitting on disk means it fits RAM | Counters on physical DS, DS+Pak, DSi and melonDS |
| P1 | Keep hot MIPS CPU state, interpreter loop, active guest stack and code cache in internal RAM/TCM where valid | Slot-2 reads add latency; relocating hot execution there would be counterproductive | Hardware measurements for guest memory misses and interpreter instructions/s |
| P1 | Represent PSP guest addresses with checked memory regions instead of one giant array | Allows bounded on-demand mapping of cold asset/data pages | Unit-tested read/write/fault and relocation cases |
| P1 | Page compressed cold game data into small internal-RAM working sets | Avoid preloading full package into the 4 MiB heap | Trace page faults, peak RAM, per-frame I/O stalls |
| P2 | Favor DS tiled/paletted backgrounds for stationary and scrolling backgrounds; use OAM sprites for player/obstacles | Hardware handles scrolling and sprite composition without a full software framebuffer | Synthetic GPU demo on melonDS and hardware |
| P2 | Use 4-bit/8-bit indexed palettes and reusable tiles/texture atlases, not 16-bit full-color buffers everywhere | Example: 256×192 bitmap is 96 KiB at 16bpp versus 48 KiB at 8bpp before palettes | Inspect actual VRAM bank assignments and screenshots |
| P2 | DMA/burst-copy staged tiles/sprite data at safe synchronization points | Reduce per-pixel ARM9 CPU work | Hardware profiling with cache coherency verified |
| P3 | If PSP GE draws only a restricted sprite-like subset, translate that subset into DS 2D commands; reject unsupported semantics | Test partial rendering without claiming complete PSP GE emulation | Frame-by-frame golden reference checks |
| P3 | Keep audio buffers and stream read-ahead small; avoid decoding entire soundtracks into main RAM | Reduce peak heap pressure | Measured underruns and latency, not anecdotal FPS |
| P4 | Optional external-RAM cold page store **after** full-bank alias/retention testing | Move infrequently accessed guest data out of internal RAM | Correctness and speed versus internal RAM on DS with physical Pak |

### Illustrative starting RAM caps — not measured usage

For an original DS-only **synthetic diagnostic**, target under 4 MiB:
- Static code/interpreter + runtime: 640 KiB
- Hot guest RAM window: 1024 KiB
- Mapped/decoded small working cache: 384 KiB
- General heap and metadata: 384 KiB
- Streaming read-ahead: 256 KiB
- Audio buffers: 160 KiB
- Stack, SDK, fragmentation and safety reserve: 512 KiB

Total provisional allocation: **3,360 KiB**. The remaining **736 KiB**
is a planning margin only. Verify allocations against the actual linker
map and runtime `malloc`/stack high-water probes before adopting these
caps. These figures are not adequate for an arbitrary 24–64 MiB PSP RAM
model; they are merely a starting synthetic runtime budget.

## Next implementation steps

1. **Current milestone:** `psp_guest_window` checked volatile data
   adapter; validate original 8/16/32-bit values and reject out-of-range
   PSP addresses. Small temporary read/write tests must restore old
   Slot-2 contents. **This is not a CPU bus integration or allocator.**
2. Add a no-copy hot/cold guest-memory address mapper whose memory
   operations route to either internal RAM or a verified external
   region, with bus-fault semantics and exact host tests.
3. Measure Slot-2 read/write ticks versus equivalent internal RAM under
   the same timer and access-count conditions. Record emulation/hardware
   model and clocks; do not extrapolate a synthetic throughput to FPS.
4. Build a **rights-independent synthetic 2D scrolling/sprite workload**
   using BlocksDS backgrounds and OAM and measure hardware CPU time
   versus a software bitmap baseline. Do not ship proprietary game art.
5. Add optional page cache (LRU or clock-style bounded working set)
   only when page fault and integrity tests exist. Guest code execution
   from Slot-2 is not assumed.
6. Keep Jetpack Joyride's target acceptance gates separate by platform.
   Even an excellent DS-native sprite renderer would not constitute
   PSP retail binary compatibility.

References:
- https://blocksds.skylyrac.net/docs/internal/memory_map/
- https://blocksds.skylyrac.net/tutorial/basic/introduction_2d/
- https://blocksds.skylyrac.net/tutorial/basic/backgrounds_scroll/
- https://blocksds.skylyrac.net/docs/guides/optimization_guide/
- https://blocksds.skylyrac.net/tutorial/intermediate/nitrofs/
