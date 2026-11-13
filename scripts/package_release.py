#!/usr/bin/env python3
"""Package existing native outputs and a safe SD-card directory layout.

The ZIP deliberately excludes all PSP ROMs and save contents.
"""
from pathlib import Path
import sys
from zipfile import ZipFile, ZIP_DEFLATED
root = Path(__file__).resolve().parents[1]
out = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "psp3ds-release-layout.zip"
with ZipFile(out, "w", compression=ZIP_DEFLATED) as z:
    for directory in ("psp/", "psp/roms/", "psp/roms/save/"):
        z.writestr(directory, "")
    z.writestr("psp/roms/README.txt",
        "Place legally obtained ISO or CSO images here.\n"
        "Save data is planned for psp/roms/save/<game-disc-id>/.\n"
        "PSP gameplay and save-data HLE are NOT implemented yet.\n")
    candidates = [root / "psp3ds.3dsx", root / "psp3ds.cia"]
    if (root / "build-nds").is_dir():
        candidates.extend(sorted((root / "build-nds").glob("*.nds")))
    for candidate in candidates:
        if candidate.is_file() and candidate.stat().st_size > 0:
            z.write(candidate, candidate.name)
print(out)
