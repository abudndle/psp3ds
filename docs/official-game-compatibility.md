# Official PSP game compatibility gate

**Current state: NOT SUPPORTED / NOT VERIFIED.** A successful Nintendo homebrew
build, synthetic GE scene, ELF loader unit test, or PPSSPP source checkout
does **not** establish official PSP game compatibility.

## Evidence required before changing status

| Stage | Required evidence | Current |
| --- | --- | --- |
| 0: Source available | Pinned upstream PPSSPP source fetched and audited in CI | Verified |
| 1: Executable loaded | Real game executable identified, decrypted/loaded where applicable, valid PSP module imports and memory mappings | Not verified |
| 2: Game boots | Actual licensed game tested on target hardware or named emulator, reaches game-created menu/title screen, with captured logs | Not verified |
| 3: Playable | Repeatable in-game input, rendering, audio, filesystem/save, timing, and no critical crashes | Not verified |
| 4: Compatible | Repeatable substantial gameplay with known issues recorded per game and platform | Not verified |

A game is **not** considered booting when only its icon, PARAM.SFO metadata,
or a project-created placeholder screen appears. Record the title, game ID,
region, image format, SHA-256 of locally tested game data (do not publish the
image), platform (3DS/DS/DSi), build commit, emulator/hardware version,
reproduction steps, boot log and screenshots. Do not publish copyrighted game
images, decryption keys or proprietary firmware.

## Blockers

- Compile and integrate a real PPSSPP MIPS interpreter component, including
  necessary transitive dependencies and console platform adapters.
- Verify executable/module loading and required PSP kernel/HLE imports with
  actual software; synthetic loader tests alone are insufficient.
- Implement usable PSP GE rendering, audio, controller, timing and storage
  paths on Nintendo hardware.
- Establish reproducible on-device tests. DS/DSi capabilities must be
  reported independently of Nintendo 3DS capabilities.

**Status update rule:** Only advance a game from unverified after recorded,
reproducible evidence. Never infer compatibility from CI build success.
