# PSP3DS cross-platform PPSSPP integration targets

This document records development targets, **not verified PSP game compatibility**.

| Platform | Role | PSP game boot verified |
| --- | --- | --- |
| New Nintendo 3DS / New 2DS XL | Primary PPSSPP-derived runtime target | No |
| Original Nintendo 3DS / 2DS | Constrained runtime target | No |
| Nintendo DSi | Experimental platform adapter and diagnostics | No |
| Nintendo DS / DS Lite | Experimental platform adapter and diagnostics | No |
| Linux host | Unit tests, source compilation and linker diagnostics | No |

## Architecture

1. Keep PSP ELF/PRX parsing, MIPS execution, HLE, and module lifecycle
   independent of Nintendo platform APIs.
2. Integrate **actual PPSSPP source**, preserving license notices and
   documenting changes, rather than claiming a custom interpreter is PPSSPP.
3. Provide platform-specific adapters for memory allocation, graphics,
   audio, input, timing, filesystem access, and error reporting.
4. Use runtime capability checks and conservative memory budgets. Never
   silently claim that an unsupported PSP feature works.
5. Maintain host regression tests and per-platform build checks. A successful
   build or Linux host link does **not** establish a PSP game boot.
6. Keep game executables and proprietary assets outside public CI and source
   control. Use homebrew PSP test programs for redistributable boot tests.

## Proposed milestones

- Link upstream PPSSPP interpreter dependencies in a host test executable.
- Run a small, redistributable PSP MIPS instruction program through that
  interpreter and compare results with known expected register values.
- Introduce a shared platform capability interface and per-console budgets.
- Build Nintendo 3DS and DS/DSi adapters independently, reporting unsupported
  capabilities honestly.
- Verify ELF/PRX loading, kernel startup, and graphics on real hardware or
  compatible emulators before making any game-boot claim.

## Hardware limits

PSP software may require more RAM, CPU throughput, graphics features, or
system services than DS/DSi hardware can supply. Cross-platform source
compatibility does not imply universal PSP game compatibility.
