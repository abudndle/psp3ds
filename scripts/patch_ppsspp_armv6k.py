#!/usr/bin/env python3
"""Apply one audited PPSSPP source-compatibility fix to a pinned checkout.

The devkitARM C++ ABI gives uint32_t and PPSSPP's u32 different overload
ranks. Casting the 32-bit IR register word to upstream u32 disambiguates
the overload without changing the scanned bits. Fail closed if upstream
changes the line so the patch can be reviewed instead of silently skipped.
"""
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_ppsspp_armv6k.py UPSTREAM_DIRECTORY")
path = Path(sys.argv[1]) / "Core/MIPS/IR/IRRegCache.h"
original = path.read_text(encoding="utf-8")
before = "const int b = LeastSignificantSetBit(word);"
after = "const int b = LeastSignificantSetBit((u32)word);"
if original.count(before) != 1 or after in original:
    raise SystemExit("upstream PPSSPP IRRegCache.h changed: review ARMv6K patch")
path.write_text(original.replace(before, after), encoding="utf-8")
print("Applied pinned ARMv6K PPSSPP IRRegCache 32-bit overload cast")
