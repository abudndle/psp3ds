# Nintendo 3DS target (PSP3DS)

This is the **current** platform. For now, the 3DS entry point is
`source/main.c`, with the root `Makefile` handling native builds. The
portable components remain under `source/` and `include/` to avoid breaking
existing tests. See [dependency inventory](../../docs/PLATFORM_LIBRARIES.md).

Planned hardware adapters:
- libctru for input, filesystem, system clock, NDSP audio, thread services;
- citro3d for a future PSP GE-to-PICA200 command translator;
- citro2d for UI overlays as needed;
- zlib for existing CSO sector decompression.

**No PICA200 PSP renderer, dynarec, or playable game compatibility exists.**
A native build and Azahar run must be verified before claiming execution.
