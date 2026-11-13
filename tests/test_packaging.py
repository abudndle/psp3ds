#!/usr/bin/env python3
"""Verify releases contain safe folders, but no ROM files or fictional binaries."""
from pathlib import Path
from tempfile import TemporaryDirectory
from subprocess import run
from zipfile import ZipFile
import sys

root = Path(__file__).resolve().parents[1]
with TemporaryDirectory() as tmp:
    out = Path(tmp) / "release.zip"
    run([sys.executable, str(root / "scripts/package_release.py"), str(out)],
        check=True, capture_output=True, text=True)
    with ZipFile(out) as archive:
        names = archive.namelist()
        for name in ("psp/", "psp/roms/", "psp/roms/save/"):
            assert name in names
            assert archive.getinfo(name).is_dir()
        guide = archive.read("psp/roms/README.txt").decode("utf-8")
        assert "PSP gameplay and save-data HLE are NOT implemented" in guide
        assert not any(name.lower().endswith((".iso", ".cso", ".pbp"))
                       for name in names)
        for item in names:
            if item.endswith((".nds", ".3dsx", ".cia")):
                assert archive.getinfo(item).file_size > 0
print("PASS: packaged SD layout, content exclusion, valid binaries only")
