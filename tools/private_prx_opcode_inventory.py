#!/usr/bin/env python3
"""Inspect PSP PRX relocation instruction classes without disclosing game data.

This is an analysis tool, not a relocation applier or game boot test.
"""
import argparse
import collections
import json
import struct
from pathlib import Path

def inspect(path):
    data = Path(path).read_bytes()
    if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("expected 32-bit little-endian ELF")
    u16 = lambda offset: struct.unpack_from("<H", data, offset)[0]
    u32 = lambda offset: struct.unpack_from("<I", data, offset)[0]
    if u16(18) != 8:
        raise ValueError("expected MIPS ELF")
    phoff, shoff = u32(28), u32(32)
    phentsize, phnum = u16(42), u16(44)
    shentsize, shnum = u16(46), u16(48)
    if phentsize < 32 or shentsize < 40 or phoff + phentsize * phnum > len(data) or shoff + shentsize * shnum > len(data):
        raise ValueError("invalid ELF header table bounds")
    segments = []
    for i in range(phnum):
        p = struct.unpack_from("<8I", data, phoff + i * phentsize)
        if p[0] == 1:
            if p[1] + p[4] > len(data) or p[4] > p[5]:
                raise ValueError("invalid loadable segment")
            segments.append(p)
    types = collections.Counter()
    lo16_opcodes = collections.Counter()
    invalid = 0
    for i in range(shnum):
        sh = struct.unpack_from("<10I", data, shoff + i * shentsize)
        if sh[1] != 0x700000a0:
            continue
        offset, size, entry_size = sh[4], sh[5], sh[9]
        if entry_size != 8 or size % 8 or offset + size > len(data):
            raise ValueError("invalid PSP relocation section")
        for pos in range(offset, offset + size, 8):
            target, info = struct.unpack_from("<II", data, pos)
            kind, target_seg, base_seg = info & 255, (info >> 8) & 255, (info >> 16) & 255
            types[kind] += 1
            if target_seg >= len(segments) or base_seg >= len(segments):
                invalid += 1
                continue
            seg = segments[target_seg]
            if target & 3 or target + 4 > seg[5]:
                invalid += 1
                continue
            if kind == 6:
                if target + 4 > seg[4]:
                    invalid += 1
                    continue
                instruction = u32(seg[1] + target)
                lo16_opcodes[instruction >> 26] += 1
    return {
        "analysis_only": True,
        "relocation_count": sum(types.values()),
        "relocation_types": dict(sorted(types.items())),
        "lo16_mips_opcodes": dict(sorted(lo16_opcodes.items())),
        "invalid_targets": invalid,
        "boot_verified": False,
    }

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("elf")
    args = parser.parse_args()
    result = inspect(args.elf)
    print(json.dumps(result, indent=2))
    if result["invalid_targets"]:
        raise SystemExit(1)
