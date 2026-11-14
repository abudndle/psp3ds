#!/usr/bin/env python3
"""Private R_MIPS_26 J/JAL relocation audit. Does not execute game code."""
import argparse
import json
import struct
from pathlib import Path

def audit(path, base):
    data = Path(path).read_bytes()
    if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("expected little-endian ELF32")
    h = lambda p: struct.unpack_from("<H", data, p)[0]
    w = lambda p: struct.unpack_from("<I", data, p)[0]
    if h(18) != 8: raise ValueError("expected MIPS")
    po, so = w(28), w(32)
    pn, ps, sn, ss = h(44), h(42), h(48), h(46)
    if ps < 32 or ss < 40 or po + pn*ps > len(data) or so + sn*ss > len(data):
        raise ValueError("invalid ELF tables")
    segs = []
    for i in range(pn):
        p = struct.unpack_from("<8I", data, po + i*ps)
        if p[0] == 1:
            if p[1]+p[4] > len(data) or p[4] > p[5]:
                raise ValueError("invalid PT_LOAD")
            segs.append(p)
    if not segs: raise ValueError("missing PT_LOAD")
    extent = max(p[2]+p[5] for p in segs)
    if extent > 64*1024*1024 or base + extent > 0x100000000 or base & 3:
        raise ValueError("invalid guest mapping")
    image = bytearray(extent)
    for p in segs: image[p[2]:p[2]+p[4]] = data[p[1]:p[1]+p[4]]
    total = applied = 0
    seen = set()
    for i in range(sn):
        s = struct.unpack_from("<10I", data, so + i*ss)
        if s[1] != 0x700000a0: continue
        off, size, stride = s[4], s[5], s[9]
        if stride != 8 or size % 8 or off + size > len(data):
            raise ValueError("invalid relocation section")
        for pos in range(off, off+size, 8):
            rel, info = struct.unpack_from("<II", data, pos)
            kind, ts, bs = info & 255, (info >> 8) & 255, (info >> 16) & 255
            if ts >= len(segs) or bs >= len(segs):
                raise ValueError("invalid segment selector")
            p = segs[ts]
            if rel & 3 or rel + 4 > p[5]:
                raise ValueError("invalid relocation target")
            total += 1
            if kind != 4: continue
            address = p[2]+rel
            if address in seen: raise ValueError("duplicate J26 target")
            seen.add(address)
            old = struct.unpack_from("<I", image, address)[0]
            if old >> 26 not in (2,3): raise ValueError("not J/JAL")
            target = ((old & 0x03ffffff)<<2) + base
            if target > 0xffffffff or ((target ^ (base+address+4)) & 0xf0000000):
                raise ValueError("J26 region overflow")
            new = (old & 0xfc000000) | ((target >> 2) & 0x03ffffff)
            struct.pack_into("<I", image, address, new)
            applied += 1
    return dict(load_base=hex(base), records_checked=total, r_mips_26_applied=applied,
                other_relocations_not_applied=total-applied,
                full_prx_relocation=False, game_booted=False)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("elf")
    parser.add_argument("--load-base", type=lambda x:int(x,0), default=0x08800000)
    args = parser.parse_args()
    print(json.dumps(audit(args.elf,args.load_base),indent=2))
