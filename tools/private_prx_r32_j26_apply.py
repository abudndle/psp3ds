#!/usr/bin/env python3
"""Private PSP PRX R_MIPS_32 + R_MIPS_26 guest-image relocation probe.

This diagnostic does not execute PSP code, apply HI16/LO16, or export game data.
"""
import argparse
import collections
import json
import struct
from pathlib import Path


def apply(path, load_base):
    blob = Path(path).read_bytes()
    if len(blob) < 52 or blob[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('expected little-endian ELF32')
    h = lambda o: struct.unpack_from('<H', blob, o)[0]
    w = lambda o: struct.unpack_from('<I', blob, o)[0]
    phoff, shoff = w(28), w(32)
    phsize, phnum, shsize, shnum = h(42), h(44), h(46), h(48)
    if h(18) != 8 or phsize < 32 or shsize < 40 or phoff + phsize * phnum > len(blob) or shoff + shsize * shnum > len(blob):
        raise ValueError('invalid MIPS ELF tables')
    segments = []
    for i in range(phnum):
        p = struct.unpack_from('<8I', blob, phoff + i * phsize)
        if p[0] == 1:
            if p[1] + p[4] > len(blob) or p[4] > p[5]:
                raise ValueError('invalid load segment')
            segments.append(p)
    if not segments:
        raise ValueError('missing load segments')
    image_size = max(p[2] + p[5] for p in segments)
    if image_size > 64 * 1024 * 1024 or load_base + image_size > 0x100000000 or load_base & 3:
        raise ValueError('guest mapping out of bounds')
    memory = bytearray(image_size)
    for p in segments:
        memory[p[2]:p[2] + p[4]] = blob[p[1]:p[1] + p[4]]
    counts = collections.Counter()
    seen = set()
    for section in range(shnum):
        s = struct.unpack_from('<10I', blob, shoff + section * shsize)
        if s[1] != 0x700000a0:
            continue
        off, size, stride = s[4], s[5], s[9]
        if stride != 8 or size % 8 or off + size > len(blob):
            raise ValueError('malformed relocation section')
        for pos in range(off, off + size, 8):
            rel, info = struct.unpack_from('<II', blob, pos)
            kind, target_seg, base_seg = info & 255, (info >> 8) & 255, (info >> 16) & 255
            if target_seg >= len(segments) or base_seg >= len(segments):
                raise ValueError('invalid relocation segment index')
            p = segments[target_seg]
            if rel & 3 or rel + 4 > p[5]:
                raise ValueError('invalid relocation target')
            counts[kind] += 1
            if kind not in (2, 4):
                continue
            address = p[2] + rel
            if address in seen:
                raise ValueError('duplicate R_MIPS_32/R_MIPS_26 target')
            seen.add(address)
            old = struct.unpack_from('<I', memory, address)[0]
            if kind == 2:
                struct.pack_into('<I', memory, address, (old + load_base) & 0xffffffff)
                continue
            if old >> 26 not in (2, 3):
                raise ValueError('R_MIPS_26 is not J/JAL')
            new_target = ((old & 0x03ffffff) << 2) + load_base
            guest_pc = load_base + address
            if new_target > 0xffffffff or ((new_target ^ (guest_pc + 4)) & 0xf0000000):
                raise ValueError('J/JAL target crosses MIPS 256 MiB region')
            new = (old & 0xfc000000) | ((new_target >> 2) & 0x03ffffff)
            struct.pack_into('<I', memory, address, new)
    return dict(load_base=hex(load_base), relocation_records=sum(counts.values()),
                r_mips_26_applied=counts[4], r_mips_32_applied=counts[2],
                other_relocations_not_applied=sum(counts.values()) - counts[4] - counts[2],
                all_j26_instructions_valid=True, full_prx_relocation=False, game_booted=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('elf')
    parser.add_argument('--load-base', type=lambda v: int(v, 0), default=0x08800000)
    args = parser.parse_args()
    print(json.dumps(apply(args.elf, args.load_base), indent=2))
