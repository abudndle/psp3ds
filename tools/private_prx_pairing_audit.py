#!/usr/bin/env python3
"""Read-only HI16/LO16 pairing audit for private PSP ELF files."""
import json
import struct
import sys
from pathlib import Path

data = Path(sys.argv[1]).read_bytes()
if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01":
    raise SystemExit("expected little-endian ELF32")
def h(o): return struct.unpack_from("<H", data, o)[0]
def w(o): return struct.unpack_from("<I", data, o)[0]
if h(18) != 8: raise SystemExit("expected MIPS")
shoff, count, stride = w(32), h(48), h(46)
if stride < 40 or shoff + count * stride > len(data):
    raise SystemExit("invalid section header table")
pending = []
lo_count = standalone = mismatch = 0
for i in range(count):
    sh = struct.unpack_from("<10I", data, shoff + i * stride)
    if sh[1] != 0x700000a0: continue
    off, size, entsize = sh[4], sh[5], sh[9]
    if entsize != 8 or size % 8 or off + size > len(data):
        raise SystemExit("invalid relocation section")
    for pos in range(off, off + size, 8):
        _, info = struct.unpack_from("<II", data, pos)
        kind, base = info & 255, (info >> 16) & 255
        if kind == 5: pending.append(base)
        elif kind == 6:
            lo_count += 1
            if not pending: standalone += 1
            elif any(x != base for x in pending): mismatch += 1
            pending.clear()
print(json.dumps(dict(lo16_count=lo_count, standalone_lo16=standalone,
                      mismatched_pair_groups=mismatch, unmatched_hi16=len(pending),
                      relocation_applied=False, game_booted=False), indent=2))
