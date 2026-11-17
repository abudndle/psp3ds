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
    "direct_memory_base": re.compile(r"\b(?:Memory::)?base\s*(?:\[|\+)"),
    "pointer_cast": re.compile(r"\b(?:GetPointer|GetPointerWrite|GetPointerUnchecked|GetPointerWriteUnchecked)\s*\("),
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
    p.add_argument("--additional", type=pathlib.Path, nargs="*", default=[],
                   help="Additional interpreter call-site files to inventory")
    args = p.parse_args()
    matches = []
    for source_file in [args.header, *args.additional]:
        for match in inventory(source_file.read_text(encoding="utf-8")):
            matches.append({"source": str(source_file), **match})
    counts = {kind: sum(m["kind"] == kind for m in matches) for kind in PATTERNS}
    result = {"source": str(args.header), "count": len(matches), "by_kind": counts, "matches": matches,
              "warning": "Inventory only: PPSSPP interpreter memory is not connected to the checked PSP3DS bus."}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Found {len(matches)} unchecked pointer/memory API sites across {1 + len(args.additional)} files")
    if not matches:
        raise SystemExit("FAIL: no matching sites; audit patterns or upstream source must be reviewed")

if __name__ == "__main__":
    main()
