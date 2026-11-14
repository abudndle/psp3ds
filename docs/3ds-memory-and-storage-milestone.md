# 3DS and New 3DS memory + SD storage milestone

## What the hardware actually provides

| Hardware | Total installed FCRAM | Default APPLICATION memory region | Special APPLICATION modes | Officially supported card |
|---|---:|---:|---:|---|
| Original 3DS / original 2DS | 128 MiB | 64 MiB | 72/80/96 MiB depending on special app config | SD/SDHC <=32 GB |
| New 3DS / New 2DS XL | 256 MiB | 124 MiB | 178 MiB under special configuration (NOT ordinary retail mode) | microSD/microSDHC <=32 GB |

**FCRAM is partitioned into APPLICATION, SYSTEM and BASE.** No PSP3DS
process is entitled to allocate all 128 or 256 MiB. On New 3DS launched
in legacy application mode, the visible application memory can be much
smaller than 124 MiB. Official 32 GB storage is a supported capacity, NOT
a system-RAM allocation. Some SDXC cards formatted FAT32 reportedly work
above that official limit but are not guaranteed by Nintendo.

Hardware/OS references:
- https://www.3dbrew.org/wiki/Memory_layout
- https://www.3dbrew.org/wiki/NCCH/Extended_Header
- https://www.3dbrew.org/wiki/Memory_Management
- https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/os.h
- https://en-americas-support.nintendo.com/app/answers/detail/a_id/274/~/what-are-sd-cards-and-microsd-cards-used-for%3F

## Portable runtime policy

`psp_3ds_memory_plan()` consumes the **observed** application's region
size and free bytes. It keeps 16 MiB (Old 3DS) or 24 MiB (New 3DS) of
reported free APPLICATION memory unassigned for native graphics, audio,
OS allocations and other emulator workloads. These are conservative
project policy reserves, not Nintendo-certified free-memory values.

Beyond the reserve, the initial *guest-memory target* is bounded to
32 MiB for Old 3DS or 64 MiB for New 3DS, and reduced if the observed
free region cannot afford it. It is rounded down to a 1 MiB boundary.
The plan also reports any unassigned room as a *potential* cache budget;
no such texture/audio allocations are automatically made.

On actual 3DS hardware, `psp_3ds_native_memory_probe()` calls libctru's
`osGetMemRegionSize/Used(MEMREGION_APPLICATION)`, prints the measured
region/free sizes, and tries allocating the recommended guest amount.
It writes across each 4 KiB page, **frees the buffer immediately**, and
retries smaller allocations if a large contiguous block fails. Therefore
the device reports empirical heap feasibility, not a retained PSP memory
arena or proof that a PSP executable can boot. No allocation uses the
SYSTEM, BASE, GPU linear-heap or DSP memory regions.

| Policy | Original 3DS | New 3DS |
|---|---:|---:|
| Physical RAM (information only) | 128 MiB | 256 MiB |
| Normal app region (may differ at runtime) | 64 MiB | 124 MiB |
| Minimum free-memory reserve | 16 MiB | 24 MiB |
| Maximum target PSP guest RAM | 32 MiB | 64 MiB |
| Minimum attempted probe | 8 MiB | 8 MiB |

**Do not claim the 64 MiB PSP guest target is wired to the whole PSP
loader yet**: its current PRX loader still uses bounded 32 MiB guest
images; future work must reconcile those limits and add more HLE services.
The user's Jetpack Joyride PRX needs ~6.32 MB of mapped guest image
before further PSP emulation overhead. A successful 8 MiB probe proves
only a temporary allocation, not playable commercial PSP games.

## SDHC and filesystems

The official supported SD/microSDHC card capacity is **32 GB** for both
3DS families. The usual FAT32 per-file bound is **4 GiB minus 1 byte**.
Card storage cannot be used directly as writable guest RAM.
To address parts of individual files beyond 2 GiB, prefer the native
`FSFILE_Read(Handle,u32*,u64,void*,u32)` service: `source/psp_3ds_sd_native.c`
uses this call with a **64-bit file offset**. `psp_3ds_storage_read()`
validates the observed file length, restricts reads to 4 KiB and leaves
the caller's buffer intact if a backend reports a failed/short read.
Reading a 3 GiB file never allocates 3 GiB of RAM.

The native 3DS diagnostic optionally checks `sdmc:/psp3ds/stream-test.bin`
or `sdmc:/psp3ds/EBOOT.PBP`, when present. It uses read-only FS service
handles and tries to fetch the beginning and end of the file without
printing contents or executing a game. When neither file exists it
reports `optional file absent` and continues boot. SD filesystem behavior
still needs validation on real hardware/real media; CI builds alone do
not establish card compatibility or verified 4 GiB file access.

## Regression coverage and next steps

`test_3ds_memory` validates both default app-region sizes, New 3DS legacy
mode, special-mode region sizes, headroom, insufficient-memory fallbacks,
32/64 MiB conservative guest caps and invalid kernel observations.
`test_3ds_storage` validates 64-bit reads beyond 2 GiB, the FAT32
4 GiB minus one boundary, partial callback failure atomicity and
unchanged caller memory.

Next functional gate: retain an allocated guest arena, measure peak
old/New 3DS app memory on real hardware, raise mapped PRX RAM limits
safely and connect real SD file streaming to a guest memory / PSP GE
pipeline. No commercial PSP game boot, frame rates or real-card reads
are proven by these host tests or native compilation.
