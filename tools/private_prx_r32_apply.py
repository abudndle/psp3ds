#!/usr/bin/env python3
"""Privately apply only R_MIPS_32 records to a PSP PRX guest-memory image.

Other relocation types remain untouched. Emits counts/checksums only.
Never uploads or prints game data. This is NOT a full PRX loader.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

def run(path, delta):
    b = Path(path).read_bytes()
    if len(b) < 52 or b[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("expected ELF32 little-endian")
    h = lambda o: struct.unpack_from("<H", b, o)[0]
    w = lambda o: struct.unpack_from("<I", b, o)[0]
    if h(18) != 8: raise ValueError("expected MIPS")
    phoff, shoff = w(28), w(32)
    phsize, phnum, shsize, shnum = h(42), h(44), h(46), h(48)
    if phsize < 32 or shsize < 40 or phoff + phsize * phnum > len(b) or shoff + shsize * shnum > len(b):
        raise ValueError("invalid ELF tables")
    segments = []
    for i in range(phnum):
        p = struct.unpack_from("<8I", b, phoff + i * phsize)
        if p[0] == 1:
            if p[1] + p[4] > len(b) or p[4] > p[5]:
                raise ValueError("invalid PT_LOAD")
            segments.append(p)
    if not segments: raise ValueError("no load segments")
    image_size = max(p[2] + p[5] for p in segments)
    if image_size > 64 * 1024 * 1024: raise ValueError("guest image too large")
    image = bytearray(image_size)
    for p in segments:
        image[p[2]:p[2] + p[4]] = b[p[1]:p[1] + p[4]]
    before = hashlib.sha256(image).hexdigest()
    seen = set()
    total = applied = 0
    for i in range(shnum):
        sh = struct.unpack_from("<10I", b, shoff + i * shsize)
        if sh[1] != 0x700000a0: continue
        off, size, stride = sh[4], sh[5], sh[9]
        if stride != 8 or size % 8 or off + size > len(b):
            raise ValueError("invalid PRX relocation section")
        for pos in range(off, off + size, 8):
            rel, info = struct.unpack_from("<II", b, pos)
            kind, target_seg, base_seg = info & 255, (info >> 8) & 255, (info >> 16) & 255
            total += 1
            if target_seg >= len(segments) or base_seg >= len(segments):
                raise ValueError("invalid segment selector")
            target = segments[target_seg]
            if rel & 3 or rel + 4 > target[5]:
                raise ValueError("invalid target")
            if kind != 2: continue
            address = target[2] + rel
            if address in seen: raise ValueError("duplicate R_MIPS_32 target")
            seen.add(address)
            old = struct.unpack_from("<I", image, address)[0]
            struct.pack_into("<I", image, address, (old + delta) & 0xffffffff)
            applied += 1
    return dict(test="R_MIPS_32-only partial relocation", delta=hex(delta),
                relocation_records=total, r_mips_32_applied=applied,
                other_relocations_not_applied=total-applied,
                image_changed=before != hashlib.sha256(image).hexdigest(),
                full_prx_relocation=False, game_booted=False)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("elf")
    parser.add_argument("--delta", type=lambda s: int(s, 0), default=0x08800000)
    args = parser.parse_args()
    print(json.dumps(run(args.elf, args.delta), indent=2))
