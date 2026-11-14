#!/usr/bin/env python3
"""Patch pinned PPSSPP VFPU RNG pointer signatures for ARM type compatibility.

The MIPS VFPU control registers are u32[], but on devkitARM uint32_t can be
unsigned long while u32 is unsigned int. Both are 32-bit but distinct C++
pointer types. Keep the upstream RNG implementation and use the actual
register element type rather than disabling type checking.
"""
import pathlib
import sys

def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch-ppsspp-vfpu-types.py <ppsspp-root>")
    root = pathlib.Path(sys.argv[1])
    files = (
        root / "Core/MIPS/MIPSVFPUUtils.h",
        root / "Core/MIPS/MIPSVFPUUtils.cpp",
    )
    signatures = (
        ("vrnd_init_default(uint32_t *rcx)", "vrnd_init_default(u32 *rcx)"),
        ("vrnd_init(uint32_t seed, uint32_t *rcx)", "vrnd_init(uint32_t seed, u32 *rcx)"),
        ("vrnd_generate(uint32_t *rcx)", "vrnd_generate(u32 *rcx)"),
    )
    for path in files:
        original = path.read_text()
        updated = original
        for old, new in signatures:
            count = updated.count(old)
            if count != 1:
                raise SystemExit(f"{path}: expected one occurrence of {old!r}, got {count}")
            updated = updated.replace(old, new)
        path.write_text(updated)
        print(f"patched {path}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
