#!/usr/bin/env python3
"""Package built native binaries if present, plus SD layout without game content."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import sys
root=Path(__file__).resolve().parents[1]
out=Path(sys.argv[1]) if len(sys.argv)>1 else root/"psp3ds-release-layout.zip"
with ZipFile(out,"w",compression=ZIP_DEFLATED) as z:
    for dirname in ("psp/","psp/roms/","psp/roms/save/"):
        z.writestr(dirname,"")
    z.writestr("psp/roms/README.txt",
        "Copy legally obtained PSP ISO/CSO files here.\n"
        "Per-game saves are planned under psp/roms/save/<disc-id>/.\n"
        "PSP gameplay and save-data HLE are not implemented yet.\n")
    for file in (root/"psp3ds.3dsx",root/"psp3ds.cia",root/"build-nds"/"psp3ds-nds.nds"):
        if file.is_file():z.write(file,file.name)
print(out)
