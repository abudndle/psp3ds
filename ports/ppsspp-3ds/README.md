# PPSSPP upstream integration for Nintendo handhelds

This directory contains the initial 3DS cross-compiled **upstream PPSSPP**
interpreter probe. The upstream revision is pinned and fetched by
`.github/workflows/ppsspp-3ds-probe.yml`, not vendored into the repository.
A guarded compatibility patch lives in
`scripts/patch_ppsspp_armv6k.py`.

## Verified scope

- `upstream_probe.cpp` links PPSSPP's upstream `MIPSState`,
  `MIPSInt::Int_IType`, and the real upstream interpreter translation unit.
  The diagnostic executes two ADDIU opcodes and checks resulting registers
  and program counter. This is **not** an ELF boot or playable PSP game.
- `abi_probe.cpp` reports that result and whether the console is part of
  the New 3DS family, without requiring New 3DS-only CPU instructions.
- The main PSP3DS project separately provides a compact shared MIPS
  interpreter on DS/DSi/3DS. It is **not** yet replaced by PPSSPP's
  upstream CPU/memory/HLE runtime.
- The portable `psp_video_layout` module now projects PSP 480x272 output
  to Nintendo displays without OpenGL/Vulkan and is compiled by DS and 3DS
  native builds. The existing 3DS synthetic renderer uses this mapping.

## Full migration

See [the complete native-backend architecture and subsystem checklist]
(../../docs/ppsspp-nintendo-port-architecture.md) and issue #33. The plan
covers CPU/VFPU, memory, HLE/kernel, guest GPU, sound/codecs, game media,
controls, storage, settings, multiplayer and device-specific optimizations.

PPSSPP's desktop graphics APIs cannot be used directly. On 3DS, use the
PICA200 GPU through Citro3D/Citro2D; on DS/DSi, use verified tile/OAM and
bounded software techniques where feasible. Preserve PSP behavior in common
code rather than rewriting game-specific behavior. Original DS/DSi resource
limits make universal compatibility an aspirational research goal, not a
promise.

Keep GPL-2.0-or-later notices/obligations for upstream PPSSPP code.
Use original synthetic fixtures in public CI; keep legally obtained PSP
games, firmware and other restricted media out of public repositories.

No PSP commercial gameplay is verified on any Nintendo target.
