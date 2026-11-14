# Nintendo DS, DS + Memory Expansion Pak, and DSi RAM milestone

## Hardware capability versus this emulator's allocation budgets

| Console runtime mode | Internal main RAM | Additional hardware RAM | Normal full-system total | PSP3DS hot-data policy | PSP3DS cold-data policy |
| --- | ---: | ---: | ---: | ---: | ---: |
| Original DS / DS Lite | 4 MiB | none | 4 MiB | at most 512 KiB | none |
| DS / DS Lite + official Nintendo Memory Expansion Pak | 4 MiB | 8 MiB in Slot-2 | 12 MiB separated | at most 512 KiB | at most 2 MiB when open and sampled/verified |
| DSi / DSi XL running DSi enhanced mode | 16 MiB | no Slot-2 | 16 MiB | at most 2 MiB | none |
| DSi / DSi XL running legacy DS compatibility mode | 4 MiB exposed | no Slot-2 | 4 MiB usable to legacy app | at most 512 KiB | none |

Retail DS main RAM and DSi mode sizes are confirmed by the BlocksDS [memory map](https://blocksds.skylyrac.net/docs/internal/memory_map/) and [memory management guide](https://blocksds.skylyrac.net/tutorial/intermediate/memory_management/). The official [Nintendo Memory Expansion Pak](https://en-americas-support.nintendo.com/app/answers/detail/a_id/7619/~/nintendo-ds%252Fds-lite-memory-expansion-pak-overview) is designed for the Opera browser; it is an 8 MiB Slot-2 peripheral. The [DSi operations manual](https://assets.nintendo.eu/image/upload/v1635390257/NAL/Support/DSiOperationsManual.pdf) confirms that DSi cannot use the Memory Expansion Pak because it lacks the GBA Slot-2 connector.

DSi has an additional 768 KiB of configurable NWRAM, but this is **not** folded into the normal 16 MiB main RAM nor the portable PSP guest buffer. VRAM, TCM, and ARM7-specific RAM likewise are not ordinary arbitrary heap space. DSi debugger / 3DS DSi-mode configurations may expose further memory, but this milestone deliberately selects conservative retail DS/DSi profiles.

Some third-party Slot-2 peripherals report 16 or 32 MiB, and their access patterns/banks vary. The code uses the **SDK-reported capacity** only when a single flat bank passes the existing read/write probes, bounded to 32 MiB; it never assumes all 32 MiB are a contiguous normal executable heap. BlocksDS [peripheral documentation](https://blocksds.skylyrac.net/tutorial/intermediate/peripherals/) describes the detection, bank count, memory start and size APIs.

## New enforceable policy

`include/psp_ds_memory.h` and `source/psp_ds_memory.c` select the actual main RAM size from the **active runtime mode**, not the console casing. The caller supplies a successful, temporary, per-page-touched heap allocation size (zero means unverified). `psp_ds_memory_plan()` clamps confirmed hot data to the existing platform budget and refuses inconsistent or unsupported observations. It does **not** claim a 4/16 MiB heap, nor a 512 MiB ROM buffer.

Slot-2 is only selected when the opened device is confirmed as RAM by the SDK, has one bank, and passes the two-location, 32-location sweep, dual-bus and reversible 8-KiB cold cache diagnostics. These are **sampled checks**, not a full 8 MiB memory test or an aliasing/execution guarantee. The native diagnostic closes the peripheral after testing. `active_cold_bytes` is then **zero** even if the device remains physically inserted; it must be reopened and rechecked before any future use.

The original DS and DS+Pak get exactly the same internal 512 KiB hot cap, instruction budget and DS GPU throughput. The extra RAM is for external data/cache only; the mapped ARM9 program does not execute PSP code from the peripheral. DSi receives a 2 MiB internal hot cap only when `isDSiMode()` confirms the enhanced runtime. In legacy DS mode it receives the normal DS cap and no extra memory.

`platforms/nds/source/nds_memory_probe.c` attempts a bounded temporary `malloc()` (at most 512 KiB DS, 2 MiB DSi), touches each 4 KiB page, frees the region, and reports the size that was successfully allocated. A successful probe is a diagnostic observation, **not a permanent guest buffer**, not the full available free heap, and not proof that any 3D PSP game can run.

## Verification and remaining work

`tests/test_ds_memory.c` validates 4/12/16 MiB hardware totals, DSi legacy 4 MiB fallback, policy caps, invalid/unverified/banked Pak rejection, closed Pak active cold memory zero, optional 32 MiB third-party reported size without increasing the hot cap, and zero/insufficient heap handling. Build checks cover host sanitizers and native `.nds` + 3DS compilation.

The PSP executable/runtime still needs writable memory paging, 32-bit guest-address translation across hot and external cold regions, HLE, and graphics. Jetpack Joyride's already-decrypted PRX requires ~6.32 MB of mapped guest image **before emulator overhead**. That image does not fit the 512 KiB DS hot budget, even when an official 8 MiB Pak is present; no PSP game execution is established by this milestone.
