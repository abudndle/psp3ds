#!/usr/bin/env python3
"""Report transitive local includes of upstream PPSSPP MIPS source files.

This is a dependency inventory, not a compilation or compatibility claim.
"""
from collections import deque
from pathlib import Path
import re
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else "vendor/ppsspp")
seeds = [
    "Core/MIPS/Interpreter.cpp",
    "Core/MIPS/InterpreterDispatch.cpp",
    "Core/MIPS/MIPSCodeUtils.cpp",
]
pattern = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)
queue = deque(seeds)
seen = set()
missing = set()
while queue:
    name = queue.popleft()
    if name in seen:
        continue
    seen.add(name)
    path = root / name
    if not path.is_file():
        missing.add(name)
        continue
    for include in pattern.findall(path.read_text(errors="replace")):
        # PPSSPP uses root-relative includes; keep only safe relative paths.
        inc = Path(include)
        if inc.is_absolute() or ".." in inc.parts:
            sys.exit(f"Unsafe include path: {include}")
        queue.append(inc.as_posix())
print(f"Visited {len(seen)} upstream files referenced through quoted includes")
print(f"Missing from sparse checkout: {len(missing)}")
for name in sorted(missing):
    print(f"MISSING {name}")
if missing:
    print("RESULT: interpreter source tree is not yet self-contained.")
else:
    print("RESULT: quoted includes present; system headers, linking, platform APIs remain unverified.")
