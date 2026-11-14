#!/usr/bin/env python3
"""Compare PSP PRX relocation offset encodability under two load deltas.

Read-only diagnostic; not an emulator, relocator, or executable loader.
Only aggregate statistics are emitted, never game bytes or instructions.
"""
import argparse
import collections
import json
import struct
from pathlib import Path

def analyze(path, delta):
    b = Path(path).read_bytes()
    if len(b) < 52 or b[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("expected little-endian ELF32")
    h = lambda p: struct.unpack_from("<H", b, p)[0]
    w = lambda p: struct.unpack_from("<I", b, p)[0]
    if h(18) != 8: raise ValueError("expected MIPS ELF")
    phoff, shoff = w(28), w(32)
    phsize, phnum = h(42), h(44)
    shsize, shnum = h(46), h(48)
    if phsize < 32 or shsize < 40 or phoff + phsize * phnum > len(b) or shoff + shsize * shnum > len(b):
        raise ValueError("invalid ELF tables")
    segments = []
    for i in range(phnum):
        ph = struct.unpack_from("<8I", b, phoff + i * phsize)
        if ph[0] == 1:
            if ph[1] + ph[4] > len(b) or ph[4] > ph[5]:
                raise ValueError("invalid load segment")
            segments.append(ph)
    pending = []
    checked = 0
    standalone = 0
    overflowing = 0
    opcodes = collections.Counter()
    for i in range(shnum):
        sh = struct.unpack_from("<10I", b, shoff + i * shsize)
        if sh[1] != 0x700000a0: continue
        off, size, stride = sh[4], sh[5], sh[9]
        if stride != 8 or size % 8 or off + size > len(b):
            raise ValueError("invalid relocation section")
        for pos in range(off, off + size, 8):
            target, info = struct.unpack_from("<II", b, pos)
            kind, ts, bs = info & 255, (info >> 8) & 255, (info >> 16) & 255
            if ts >= len(segments) or bs >= len(segments):
                raise ValueError("invalid segment selector")
            seg = segments[ts]
            if target & 3 or target + 4 > seg[5]:
                raise ValueError("invalid relocation target")
            checked += 1
            if kind == 5: pending.append(bs)
            elif kind == 6:
                if not pending and target + 4 <= seg[4]:
                    instruction = w(seg[1] + target)
                    opcode = instruction >> 26
                    opcodes[opcode] += 1
                    if opcode in (32, 35, 36, 40, 43, 49, 57):
                        standalone += 1
                        immediate = struct.unpack("<h", struct.pack("<H", instruction & 65535))[0]
                        if not -32768 <= immediate + delta <= 32767:
                            overflowing += 1
                pending.clear()
    return dict(delta=hex(delta), records_checked=checked,
                standalone_memory_lo16=standalone,
                standalone_memory_lo16_overflow=overflowing,
                standalone_lo16_opcodes=dict(sorted(opcodes.items())),
                relocation_applied=False, game_booted=False)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("elf")
    parser.add_argument("--delta", type=lambda s: int(s, 0), default=0x08800000)
    args = parser.parse_args()
    print(json.dumps(analyze(args.elf, args.delta), indent=2))
