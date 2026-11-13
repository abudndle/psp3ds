# PSP3DS multi-platform library inventory

This repository is the single home for PSP3DS (Nintendo 3DS) and the planned
Nintendo DS / DSi-enhanced PSP experimentation port. A library inventory is
not proof of a working PSP emulator. **No commercial PSP game is playable.**

## Nintendo 3DS (current target)

### Essential build toolchain
- **devkitPro pacman + devkitARM**: ARM cross-compiler, linker, binutils.
- **3ds-dev package group**: baseline 3DS build support (including **libctru**).
- **libctru**: 3DS services, input, SD filesystem, audio interfaces, graphics
  service, timers, threading, New 3DS CPU speed setting.
- **3ds-zlib**: CSO v1 DEFLATE decoder, already used by `psp_cso.c`.
- **3ds-examples**: known-good reference projects for the Makefile and services.
- **3ds-tools / 3dsxtool**: building `.3dsx` homebrew outputs, as installed
  by the normal devkitPro toolchain.

### Graphics
- **citro3d**: PICA200 hardware graphics, primary candidate for the eventual
  PSP GE command renderer. PSP GPU semantics still require translation.
- **citro2d**: UI, fonts and 2D overlay, built on citro3d; optional.
- **tex3ds**: texture conversion/atlasing at build time; optional.

### Audio, input, filesystem, compression
- **libctru ndsp/csnd, hid, fs, gfx, apt**: preferred low-level 3DS APIs.
- **3ds-zlib**: required for compressed CSO v1 disc images.
- **3ds-libpng**: optional texture/icon decoding.
- **3ds-libjpeg-turbo / compatible JPEG portlib**: optional image assets;
  check the live package catalog for the exact package name.
- **3ds-freetype**: optional text rendering; system font / citro2d can be
  sufficient for simpler UI.
- **3ds-bzip2, 3ds-liblzma**: optional archive utilities; not essential to PSP
  execution and should not be linked until needed.

### Optional third-party portlibs (not default dependencies)
- **3ds-sdl**: portable input/video/audio abstraction, but generally adds
  an unnecessary abstraction to a native PICA200 PSP renderer.
- **3ds-mbedtls**: networking/cryptographic protocols, not a PSP firmware
  replacement or a drop-in decryptor for encrypted commercial EBOOTs.
- **3ds-curl**: optional network services; no requirement for offline PSP
  playback.
- **3ds-sqlite**: optional metadata database; not needed in initial runtime.
Check exact names and availability using:
```sh
(dkp-pacman -Ss '^3ds-' || pacman -Ss '^3ds-')
```
Do **not** install every available library: extra dependencies enlarge
binaries, increase build complexity and often do nothing for emulation speed.

Install the baseline through devkitPro's package manager:
```sh
(dkp-pacman -S --needed 3ds-dev 3ds-zlib || pacman -S --needed 3ds-dev 3ds-zlib)
```
Additional GPU/UI portlibs can be added only when their backends are built.

## Nintendo DS / DSi enhanced (future platform)

- **BlocksDS** and **libnds**: shared DS/DSi application hardware APIs.
- **BlocksDS filesystem support**: DLDI/FAT for DS carts, SD when DSi mode
  permits it, and NitroFS for bundled non-game data.
- **Maxmod**: DS audio, optional once a PSP audio backend exists.
- **Nitro Engine**: optional DS 3D renderer; it cannot emulate PSP GPU directly.
- **NightFox's Lib** or **µLibrary**: optional DS UI/sprites.
- **libdsf**: optional text/font UI.
- **libteak**: experimental DSi DSP acceleration only when justified.
- **DSWiFi**: optional network features, not needed for initial proof-of-concept.
- **Slot-2 expansion**: available only on DS/DS Lite with compatible hardware;
  DSi has no Slot-2. Detect and validate capacity before use.

The DS/DSi is far less capable than the PSP. Any port may need to limit itself
to homebrew or small instruction/graphics demonstrations; commercial gameplay
is not promised.

## Source organization and migration plan

Currently, the tested and 3DS-targeted sources remain in the historical
`include/`, `source/`, and `tests/` directories. **Do not duplicate or
move them until both platform build systems have green CI**.

```text
psp3ds/
  include/                 # existing portable PSP CPU/HLE/disc APIs
  source/                  # existing shared C implementation and 3DS main.c
  tests/                   # portable host regression tests
  platforms/
    3ds/                   # 3DS-specific platform adapter plan / future code
    nds/                   # BlocksDS DS + DSi enhanced adapter plan / future code
  docs/
    PLATFORM_LIBRARIES.md  # this inventory
  Makefile                 # current 3DS homebrew build
  CMakeLists.txt           # portable host tests
```

### Cross-platform API boundary

The portable CPU interpreter, import/relocation parsers, HLE layer,
scheduler, ISO/CSO decoder and ISO9660 code may be reused when their
memory requirements fit. Platform adapters should provide:
- `platform_clock_us()`: monotonically increasing time;
- `platform_present_frame()`: console-specific rendering;
- `platform_audio_submit()`: output PCM via native APIs;
- `platform_read_input()`: keys/touch controls;
- `platform_alloc_guest_memory()`: bounded, device-specific guest RAM;
- `platform_open_disc()`: stream on-demand sectors from supported storage.

Only add implementations alongside real CI builds and runtime tests.
3DS-only code must never be compiled into an NDS build. Use capability
checks, not imaginary PSP hardware passthrough.

## Sources (upstream)
- https://github.com/devkitPro/pacman-packages
- https://github.com/devkitPro/libctru
- https://github.com/devkitPro/citro3d
- https://github.com/devkitPro/citro2d
- https://github.com/devkitPro/3ds-examples
- https://blocksds.skylyrac.net/docs/libs/core_libs/
- https://blocksds.skylyrac.net/docs/libs/additional_libs/
- https://github.com/blocksds/packages
