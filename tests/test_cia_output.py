#!/usr/bin/env python3
"""Verify that a built CIA has a plausible Nintendo 3DS CIA container header.

Not a signature, security, device-installability or runtime validation.
"""
from pathlib import Path
import struct
import sys

def inspect(path: Path) -> None:
    data = path.read_bytes()
    if len(data) < 0x2020:
        raise ValueError("CIA is shorter than its standard header")
    header_size, type_id, version, cert, ticket, tmd, meta, content = struct.unpack_from(
        "<IHHIIIIQ", data
    )
    if header_size != 0x2020:
        raise ValueError(f"unexpected CIA header size: {header_size:#x}")
    if type_id != 0:
        raise ValueError(f"unexpected CIA type: {type_id}")
    if not ticket or not tmd or not content:
        raise ValueError("CIA is missing ticket, TMD, or content")
    # CIA subregions begin on 64-byte boundaries.
    align = lambda n: (n + 63) & ~63
    minimum = align(header_size) + align(cert) + align(ticket) + align(tmd) + align(meta) + content
    if minimum > len(data):
        raise ValueError("CIA header declares sections beyond end of file")
    print(f"PASS: plausible CIA container ({len(data)} bytes, {content} content bytes)")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_cia_output.py <built.cia>")
    inspect(Path(sys.argv[1]))
