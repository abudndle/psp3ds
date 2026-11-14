# Nintendo DS game-card ROM paging

Game-card ROM (up to 512 MiB / 4 Gbit) is **read-only storage**, not PSP guest RAM. An original DS/DS Lite has 4 MiB main RAM; a compatible Slot-2 expansion adds a separate RAM device but does not accelerate its ARM9 or 3D hardware.

## Implemented

- `include/psp_rom_pager.h` and `source/psp_rom_pager.c` cache **4 KiB pages** from a ROM image with logical offsets up to 536,870,912 bytes. There is no 512 MiB RAM allocation.
- Caller owns **1–4 internal hot pages** (4–16 KiB). An optional **1–8 cold pages** (4–32 KiB) use the existing verified, **still open** Slot-2 window. This RAM area must be reserved exclusively for the cache.
- Read-through LRU replacement; 1–32-byte cross-page reads preserve caller output on failed source reads. There is no write-back or writable PSP guest-memory mapping.
- `psp_rom_file_read()` reads a caller-opened `FILE*` stream, such as a properly mounted NitroFS resource file or a host file. The caller manages mounting/open/close and verifies filesystem capacity; this module does not mount NitroFS.

## Verification

- `tests/test_rom_pager.c`: sparse synthetic full 512 MiB address range, actual seek to the end of a **sparse host file**, cross-page correctness, final partial page, failure atomicity, LRU hot/cold hits and rejecting unverified Slot-2 devices.
- Native `.nds` diagnostic tests **synthetic** 512 MiB offsets, without a physical 512 MiB cartridge. When Slot-2 RAM has passed the existing independent probes, it saves a full 8 KiB region, exercises the 4 KiB cold cache and restores the entire area. The device must remain open for these accesses.
- DS+Memory Pak and standard DS have the same CPU/hot-RAM budget. DSi has no Slot-2, so it uses the hot cache only. Native 3DS builds also compile the shared read-only pager.

## Restrictions / next milestone

Neither ROM page caching nor the successful `.nds` build proves that Jetpack Joyride can execute on DS. The PSP game needs approximately 6.32 MB of mapped guest memory **before** graphics, PSP kernel data and emulator overhead. The shared MIPS interpreter still uses contiguous guest RAM and cannot fetch instructions from these ROM pages. Future work: mount a real NitroFS file in the DS frontend, make the PSP guest-memory bus page-aware, supply writable RAM/relocations and test correctness/performance without claiming unverified gameplay.
