#!/usr/bin/env python3
"""Audit the actual upstream PPSSPP MIPS source dependencies before porting."""
from pathlib import Path
import re
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else "vendor/ppsspp")
sources = [
    "Core/MIPS/Interpreter.cpp",
    "Core/MIPS/InterpreterDispatch.cpp",
    "Core/MIPS/MIPSCodeUtils.cpp",
]
if not root.is_dir():
    sys.exit("Missing PPSSPP checkout: run scripts/fetch-ppsspp-upstream.sh")
missing = []
external = set()
for source in sources:
    path = root / source
    if not path.is_file():
        missing.append(source)
        continue
    includes = re.findall(r'^\s*#\s*include\s*"([^"]+)"', path.read_text(), re.M)
    print(f"{source}: {len(includes)} quoted includes")
    for inc in includes:
        if not (root / inc).is_file():
            missing.append(f"{source}: {inc}")
        elif not inc.startswith("Core/MIPS/"):
            external.add(inc)
print(f"Dependencies outside Core/MIPS: {len(external)}")
for inc in sorted(external):
    print(f"  {inc}")
if missing:
    print("Unresolved upstream includes:", file=sys.stderr)
    for inc in missing:
        print(f"  {inc}", file=sys.stderr)
    sys.exit(1)
print("Direct quoted dependencies resolved; transitive/platform dependencies remain unaudited.")
