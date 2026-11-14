#!/usr/bin/env python3
"""Identify unresolved upstream PPSSPP MIPS symbols not defined by the built objects.

Uses nm -C output from host-built PPSSPP objects; this is a dependency
inventory, not proof of linking or Nintendo hardware compatibility.
"""
import argparse
import json
import re
from pathlib import Path


def parse_nm(raw):
    undefined, defined = set(), set()
    for line in raw.splitlines():
        match = re.match(r"^\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z])\s+(.+?)\s*$", line)
        if not match:
            continue
        kind, symbol = match.groups()
        if kind == "U":
            undefined.add(symbol)
        elif kind.upper() in {"T", "D", "B", "R", "W", "V", "C", "G", "S"}:
            defined.add(symbol)
    return undefined, defined


def inventory(undefined_raw, all_symbols_raw):
    unresolved, _ = parse_nm(undefined_raw)
    _, defined = parse_nm(all_symbols_raw)
    external = sorted(unresolved - defined)
    return dict(undefined_references=len(unresolved),
                resolved_within_compiled_objects=len(unresolved & defined),
                remaining_external_symbols=len(external),
                remaining_external_sample=external[:80],
                actual_link_succeeded=False)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("undefined_nm", type=Path)
    parser.add_argument("all_symbols_nm", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = json.dumps(inventory(args.undefined_nm.read_text(), args.all_symbols_nm.read_text()), indent=2) + "\n"
    if args.output:
        args.output.write_text(result)
    else:
        print(result, end="")


if __name__ == "__main__":
    main()
