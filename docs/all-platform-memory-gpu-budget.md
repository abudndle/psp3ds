# PSP3DS memory and graphics policies — all target systems

**Jetpack Joyride PSP minis compatibility: not tested / not playable on any
target.** These are conservative experimental allocation *caps* to guide
future work, not measured free heap, successfully allocated memory, PSP
graphics emulation, hardware benchmarks, or a substitute for an actual
PSP executable.

The portable `psp_platform_budget` module separates policy by model.
It computes exact framebuffer byte requirements and rejects malformed
or contradictory Slot-2 capability flags. Native DS prints its selected
base policy, then only exposes the expansion profile after the BlocksDS
RAM peripheral has passed detection and sampled validation. The 3DS
homebrew queries libctru's hardware flag to distinguish the original
3DS family from New 3DS; a failed query conservatively selects old-3DS
policy.

| Target | Hot guest data cap | Cold external cap | Decoded cache cap | Texture / tile cache cap | Target graphics path |
| --- | ---: | ---: | ---: | ---: | --- |
| Original DS / Lite | 512 KiB | none | 256 KiB | 64 KiB | 2D tile/palette + OAM sprites |
| Original DS / Lite + Slot-2 Pak | 512 KiB | 2,048 KiB | 256 KiB | 64 KiB | 2D tile/palette + OAM sprites |
| DSi / DSi XL | 2,048 KiB | none | 512 KiB | 128 KiB | 2D tile/palette + OAM sprites |
| Original 3DS / XL | 8,192 KiB | none | 1,024 KiB | 2,048 KiB | GPU-backed sprite batching |
| New 3DS / XL | 16,384 KiB | none | 2,048 KiB | 4,096 KiB | GPU-backed sprite batching |

**Important:** These are independently chosen engineering limits, not
device RAM totals and not reserved/allocated buffers. They are subject
to real application high-water measurements and will need to change.
DSi does not accept Slot-2 carts. A DS memory pack does not improve
CPU throughput. 3DS cold asset storage would be SD streaming/caching,
not Slot-2 memory.

## Efficient graphics for each system

- **DS / DS Lite**: exploit background tilemap scrolling, 4/8-bit
  indexed palettes, hardware OAM sprites, VRAM banks and sub-screen
  reuse. Avoid software redrawing an entire 256×192 framebuffer every
  frame. On original DS with Slot-2 expansion, keep hot draw-command
  queues and sprite state in *internal* RAM; use the verified cartridge
  only for relatively cold data after performance and integrity testing.
- **DSi**: use larger internal RAM for decoded sprites, scrolling maps
  and frame staging while preserving the same DS VRAM/screen limits.
  DSi-enhanced mode must be detected rather than guessed. Do not count
  hypothetical camera/SD capabilities as measured performance gains.
- **Original 3DS**: use GPU render-target and texture atlases with
  batched 2D geometry, avoid per-pixel CPU framebuffers, cap active
  texture cache and keep a measured memory reserve.
- **New 3DS**: enable larger batches/texture cache only on positively
  identified hardware; don't make old-model startup depend on higher
  CPU speed or bigger heaps. Prefer bounded work queues over unbounded
  shader buffers.
- **Across every system**: separate frame updates from the input/audio
  loop; downsample/crop 480×272 PSP output with documented HUD
  constraints; stream only *authorized* game resources; reject unknown
  PSP GE operations instead of silently drawing incorrect frames.

## Graphics memory arithmetic

The portable policy computes framebuffer bytes using
`ceil(width × height × bits_per_pixel / 8)`:

| Screen and format | One frame |
| --- | ---: |
| DS 256×192 at 8 bpp | 49,152 bytes (48 KiB) |
| DS 256×192 at 16 bpp | 98,304 bytes (96 KiB) |
| 3DS 400×240 at 16 bpp | 192,000 bytes |
| 3DS 400×240 at 24 bpp | 288,000 bytes |

These counts do not include textures, metadata, alignment, depth
buffers, double buffering or actual render-target allocations. They
must not be interpreted as available contiguous VRAM.

## Gates before these policies become real optimizations

1. **Implementation verified now:** all five policy records, validation
   and framebuffer arithmetic have host CTests and are compiled on both
   native build targets; console reports select detected hardware.
2. Measure real free heap, high-water stack/heap, VRAM mapping, physical
   CPU timing and asset throughput for each model separately. Capture
   emulator results but do not label them as hardware benchmark results.
3. Implement actual 2D tile/OAM synthetic renderer for DS/DSi and
   texture-atlas GPU batching on 3DS; compare known synthetic frames
   against reference images with timings.
4. Wire the selected **tested** budget into bounded cache allocations
   and a future guest-memory mapper. Avoid allocating the hypothetical
   whole PSP RAM in an original DS 4 MiB process.
5. Test verified guest CPU execution/graphics on supported synthetic PSP
   homebrew before evaluating authorized Jetpack Joyride media. No
   commercial gameplay claims until real game launch evidence exists.

Sources: BlocksDS memory map and 2D background/sprite documentation,
libctru hardware query API, and each platform's actual toolchain.
