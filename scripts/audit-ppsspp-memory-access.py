#!/usr/bin/env python3
"""Inventory PPSSPP MemMap.h direct guest-host pointer access sites.

An inventory is not a safe-memory rewrite. The gate deliberately fails when
the pinned header has no matches, so a source-layout change cannot silently
produce an empty, misleading report.
"""
import argparse
import json
import pathlib
import re

PATTERNS = {
    "base_pointer_arithmetic": re.compile(r"\bbase\s*\+\s*(?:\(|address\b|\w+\b)"),
    "unchecked_pointer_api": re.compile(r"\b(?:GetPointerUnchecked|GetPointerWriteUnchecked|GetBlockShadowEntry)\s*\("),
    "unchecked_scalar_api": re.compile(r"\b(?:ReadUnchecked|WriteUnchecked)_(?:U8|U16|U32|U64|Float)\s*\("),
    "unchecked_copy_api": re.compile(r"\bMemcpyUnchecked\s*\("),
}

def inventory(source):
    matches = []
    for number, line in enumerate(source.splitlines(), 1):
        for kind, pattern in PATTERNS.items():
            if pattern.search(line):
                matches.append({"line": number, "kind": kind, "text": line.strip()[:240]})
    return matches

def main():
    p = argparse.ArgumentParser()
    p.add_argument("header", type=pathlib.Path)
    p.add_argument("--output", type=pathlib.Path, required=True)
    args = p.parse_args()
    matches = inventory(args.header.read_text(encoding="utf-8"))
    result = {"source": str(args.header), "count": len(matches), "matches": matches,
              "warning": "Inventory only: PPSSPP interpreter memory is not connected to the checked PSP3DS bus."}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Found {len(matches)} unchecked pointer/memory API sites in {args.header}")
    if not matches:
        raise SystemExit("FAIL: no matching sites; audit patterns or upstream source must be reviewed")

if __name__ == "__main__":
    main()
