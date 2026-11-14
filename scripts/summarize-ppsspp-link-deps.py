#!/usr/bin/env python3
"""Summarize unresolved PPSSPP MIPS interpreter symbols from nm -u -C output.

Reads host object diagnostics; does not claim a working interpreter link.
"""
import argparse
import collections
import json
import re
from pathlib import Path


def summarize(raw):
    objects = collections.defaultdict(set)
    current = "(unknown object)"
    for line in raw.splitlines():
        stripped = line.strip()
        if stripped.endswith(":") and not stripped.startswith("U "):
            current = stripped[:-1]
            continue
        match = re.match(r"^(?:[0-9a-fA-F]+\s+)?U\s+(.+)$", stripped)
        if match:
            objects[current].add(match.group(1))
    symbols = sorted(set().union(*objects.values())) if objects else []
    categories = collections.Counter()
    for symbol in symbols:
        if "MIPS" in symbol or "mips" in symbol.lower():
            categories["MIPS execution/state"] += 1
        elif "Memory::" in symbol or "MemoryMap" in symbol:
            categories["guest memory"] += 1
        elif "sce" in symbol or "HLE" in symbol:
            categories["PSP HLE/kernel"] += 1
        elif "Core" in symbol or "CPU" in symbol:
            categories["core runtime"] += 1
        elif symbol.startswith(("std::", "__", "_Unwind", "operator ", "mem", "str")):
            categories["C++ runtime / libc"] += 1
        else:
            categories["other dependencies"] += 1
    return {
        "objects": {name: len(items) for name, items in sorted(objects.items())},
        "unique_unresolved_symbols": len(symbols),
        "categories_heuristic": dict(sorted(categories.items())),
        "sample_symbols": symbols[:40],
        "linked_to_psp3ds": False,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("nm_output", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = json.dumps(summarize(args.nm_output.read_text()), indent=2) + "\n"
    if args.output:
        args.output.write_text(result)
    else:
        print(result, end="")


if __name__ == "__main__":
    main()
