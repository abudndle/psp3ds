#!/usr/bin/env python3
"""Read-only PSP PRX relocation inventory. Never exports game bytes."""
import hashlib, json, struct, sys
from collections import Counter
from pathlib import Path

p = Path(sys.argv[1])
b = p.read_bytes()
def u16(o): return struct.unpack_from("<H", b, o)[0]
def u32(o): return struct.unpack_from("<I", b, o)[0]
if b[:4] != b"\x7fELF" or b[4:7] != bytes([1, 1, 1]) or u16(18) != 8:
    sys.exit("not a supported MIPS ELF header")
phoff, shoff = u32(28), u32(32)
phentsz, phnum = u16(42), u16(44)
shentsz, shnum = u16(46), u16(48)
if phentsz < 32 or shentsz < 40 or phoff + phentsz * phnum > len(b) or shoff + shentsz * shnum > len(b):
    sys.exit("invalid header bounds")
segments, relocs, invalid = [], [], []
relocation_types = Counter()
relocation_source_segments = Counter()
for i in range(phnum):
    t, off, va, pa, fs, ms, flags, align = struct.unpack_from("<8I", b, phoff + i * phentsz)
    if t == 1:
        if off + fs > len(b) or fs > ms: invalid.append("bad load segment " + str(i))
        segments.append(dict(index=i, vaddr=hex(va), file_size=fs, memory_size=ms, flags=hex(flags)))
for i in range(shnum):
    _, t, _, _, off, size, _, _, _, entsz = struct.unpack_from("<10I", b, shoff + i * shentsz)
    if t == 0x700000a0:
        if off + size > len(b) or entsz != 8 or size % 8: invalid.append("bad PSP relocation section " + str(i))
        relocs.append(dict(section_index=i, entries=size // 8, bytes=size))
        if off + size > len(b) or entsz != 8 or size % 8:
            continue
        for pos in range(off, off + size, 8):
            target_offset, info = struct.unpack_from("<II", b, pos)
            kind = info & 0xff
            target_segment = (info >> 8) & 0xff
            source_segment = (info >> 16) & 0xff
            relocation_types[kind] += 1
            relocation_source_segments[source_segment] += 1
            if target_segment >= len(segments) or source_segment >= len(segments):
                invalid.append(f"relocation section {i}: invalid segment selector {target_segment}/{source_segment}")
                break
            if target_offset + 4 > segments[target_segment]["memory_size"]:
                invalid.append(f"relocation section {i}: out-of-bounds relocation target")
                break
entry = u32(24)
mapped = any(int(s["vaddr"], 16) <= entry < int(s["vaddr"], 16) + s["file_size"] and int(s["flags"], 16) & 1 for s in segments)
report = dict(test="Private PSP PRX relocation inventory; no emulation", sha256=hashlib.sha256(b).hexdigest(), file_bytes=len(b), elf_type=hex(u16(16)), entry=hex(entry), entry_file_backed_executable=mapped, load_segments=segments, relocation_sections=relocs, relocation_entry_total=sum(s["entries"] for s in relocs), relocation_types=dict(sorted(relocation_types.items())), relocation_source_segments=dict(sorted(relocation_source_segments.items())), validation_errors=invalid, boot_status="NOT BOOTED", note="Relocation entries and target bounds validated, not applied; no copyrighted bytes exported")
print(json.dumps(report, indent=2))
if invalid or not mapped: sys.exit(1)
