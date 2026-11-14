#!/usr/bin/env python3
"""Read-only per-segment PSP PRX load-address audit (no game bytes exported)."""
import argparse
import collections
import json
import struct
from pathlib import Path

def inspect(path, load_base):
    b = Path(path).read_bytes()
    if len(b) < 52 or b[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("expected little-endian ELF32")
    h = lambda p: struct.unpack_from("<H", b, p)[0]
    w = lambda p: struct.unpack_from("<I", b, p)[0]
    if h(18) != 8: raise ValueError("expected MIPS ELF")
    phoff, shoff = w(28), w(32)
    phsize, phnum, shsize, shnum = h(42), h(44), h(46), h(48)
    if phsize < 32 or shsize < 40 or phoff + phsize * phnum > len(b) or shoff + shsize * shnum > len(b):
        raise ValueError("invalid ELF header tables")
    segments = []
    for i in range(phnum):
        p = struct.unpack_from("<8I", b, phoff + i * phsize)
        if p[0] != 1: continue
        if p[1] + p[4] > len(b) or p[4] > p[5]:
            raise ValueError("invalid PT_LOAD")
        segments.append(p)
    if not segments: raise ValueError("no PT_LOAD segments")
    # Preserve each segment's relative ELF virtual address; never map
    # all segments to the same address.
    mapping = [load_base + p[2] for p in segments]
    if any(v > 0xffffffff for v in mapping):
        raise ValueError("mapping exceeds 32-bit guest address space")
    pending = []
    counts = collections.Counter()
    overflow = collections.Counter()
    first = None
    for i in range(shnum):
        sh = struct.unpack_from("<10I", b, shoff + i * shsize)
        if sh[1] != 0x700000a0: continue
        off, size, entsize = sh[4], sh[5], sh[9]
        if entsize != 8 or size % 8 or off + size > len(b):
            raise ValueError("malformed relocation section")
        for pos in range(off, off + size, 8):
            target, info = struct.unpack_from("<II", b, pos)
            kind, target_seg, base_seg = info & 255, (info >> 8) & 255, (info >> 16) & 255
            if target_seg >= len(segments) or base_seg >= len(segments):
                raise ValueError("bad relocation segment selector")
            seg = segments[target_seg]
            if target % 4 or target + 4 > seg[5]:
                raise ValueError("bad relocation target")
            counts[kind] += 1
            if kind == 5:
                pending.append(base_seg)
            elif kind == 6:
                if not pending and target + 4 <= seg[4]:
                    insn = w(seg[1] + target)
                    opcode = insn >> 26
                    if opcode in (32, 35, 36, 40, 43, 49, 57):
                        immediate = struct.unpack("<h", struct.pack("<H", insn & 65535))[0]
                        delta = mapping[base_seg] - seg_base(segments[base_seg])
                        if not -32768 <= immediate + delta <= 32767:
                            overflow[base_seg] += 1
                            if first is None:
                                first = dict(section=i, opcode=opcode, target_segment=target_seg, address_base_segment=base_seg)
                pending.clear()
    return dict(load_base=hex(load_base),
                segments=[dict(index=i, original_vaddr=hex(p[2]), mapped_vaddr=hex(mapping[i]),
                               delta=hex(mapping[i] - p[2]), file_size=p[4], mem_size=p[5])
                          for i, p in enumerate(segments)],
                relocation_types=dict(sorted(counts.items())),
                standalone_memory_lo16_overflows_by_address_base=dict(sorted(overflow.items())),
                first_overflow=first, relocation_applied=False, game_booted=False)

def seg_base(segment):
    return segment[2]

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("elf")
    parser.add_argument("--load-base", type=lambda s: int(s, 0), default=0x08800000)
    args = parser.parse_args()
    print(json.dumps(inspect(args.elf, args.load_base), indent=2))
