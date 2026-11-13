# Cross-platform feature parity contract

PSP3DS targets **original DS/DS Lite**, **DS/DS Lite with a verified
Slot-2 RAM expansion**, **DSi/DSi XL**, **original 3DS/3DS XL**, and
**New 3DS/New 3DS XL**.

## Rule

Every new PSP3DS feature must have one shared semantic ID and a
well-defined status on **every** target. The common feature manifest is
`include/psp_features.h` / `source/psp_features.c`; targets may choose
different backends and memory budgets but cannot silently omit the
feature ID. `tests/test_features.c` checks that IDs, display names and
feature **status** are consistent on all five targets and that backend
choices match hardware. `tests/test_feature_wiring.py` requires DS and
3DS native entrypoints to use the same manifest and shared CPU tests.

The **existing GitHub Actions host workflow** automatically runs these
tests on pushes, pull requests, scheduled five-minute checks and manual
dispatches, and the existing native workflows build DS and 3DS outputs.
A failed parity test blocks a clean CI result; it does not automatically
create feature code, merge pull requests, or bypass review.

## Current feature availability

| Shared semantic feature | All targets | Hardware-specific implementation |
| --- | --- | --- |
| Synthetic MIPS CPU self-test | **Diagnostic available** | Same portable `psp_cpu_step` program and expected result |
| Synthetic 2,048-instruction benchmark | **Diagnostic available** | Same shared benchmark; DS reports its local ARM9 timer |
| Conservative RAM/graphics budget policy | **Diagnostic available** | DS/DSi/Slot-2/old3DS/New3DS policies have separate caps |
| Checked guest-data bus | **Experimental library** | Internal region on all; optional external bus only on verified DS+Pak |
| Synthetic scrolling 2D scene | **Diagnostic available** | DS/DSi hardware tiled BG + OAM sprite; 3DS opt-in Citro2D PICA200 batched rectangles |
| PSP GE graphics translation | **Not implemented** | Planned DS tile/OAM versus 3DS GPU rendering |
| Commercial PSP game boot | **Not implemented** | No hardware backend; Jetpack Joyride remains untested/unplayable |

A backend choice is a **plan** until a renderer is implemented and
real screenshot evidence exists. Listing a planned feature on every
device does not make it work. An experimental library does not imply
the MIPS CPU already routes all loads/stores through it.

## New feature implementation rule

1. Add a shared ID and stable description in `psp_features.h`. All
   five targets must expose that ID, even if not yet implemented.
2. Implement the portable semantics and host regression fixtures
   before using hardware backends; do not duplicate the PSP behavior
   in per-platform code.
3. Select the target-specific backend and memory policy; verify that
   missing optional hardware (including the Slot-2 pack) fails safely
   without removing the feature entirely.
4. Register in `test_features.c` and update native entrypoints.
   Host CTest, ASan/UBSan, NDS, 3DSX/CIA and emulator smoke CI must
   pass before claiming a milestone.
5. Increase availability only when the actual native implementation
   is exercised; preserve separate hardware evidence for each model.
   Tests must never treat an emulator window alone as PSP gameplay.

This approach supports automatically **testing parity** of future
features; code-generation and autonomous feature development still
depend on a separately configured development agent with repository
write permissions. This contract does not enable such an agent itself.

## First actual shared display-scene increment

`psp_scene_build(frame)` creates an original deterministic 32×32 tile map, three
4bpp tiles, a four-color BGR555 palette, horizontal scrolling and an 8×8
moving actor. Host `shared_scene` validates scroll, actor, boundaries,
and pixel-raster equivalence. DS/DSi uses main-engine background VRAM
and a real OAM sprite. 3DS uses GPU-submitted Citro2D rectangles on the lower screen, toggled
with **A**, preserving the existing synthetic boot marker at startup.
The portable rectangle batch is compared pixel-for-pixel against the
original scene for ten frame states. This proves synthetic scene
consistency, not PSP graphics compatibility.
The scene is synthetic and not PSP GE output or Jetpack Joyride content.

## First PICA200 batch milestone

`psp_scene_batch_build` emits at most 64 clipped, ordered opaque
rectangles from the same tile scene. The 3DS path initializes
Citro3D/Citro2D only when the user presses **A**, submits rectangles to
the bottom-screen PICA200 render target with `C2D_DrawRectSolid`, then
ends the frame with `C3D_FrameEnd`. Original 3DS and New 3DS use the
same code; there are no New3DS-only GPU instructions. The former
CPU-drawn BGR8-per-pixel fallback is no longer used for the scene.

The `scene_batch` host test compares all 49,152 pixel colors over ten
frames between rectangle composition and the shared scene semantics.
Native 3DS compilation is not hardware display verification. An actual
3DS or compatible emulator must still press A and capture the GPU
scene before claiming visible GPU runtime correctness or a speedup.
