#!/usr/bin/env python3
"""Fail closed if release packages contain private assets or fake binaries."""
from pathlib import Path
from zipfile import ZipFile
import sys

FORBIDDEN_SUFFIXES = (
    '.iso', '.cso', '.pbp', '.cci', '.cia', '.3ds', '.3dsx', '.nds',
    '.keys', '.key', '.pem', '.p12', '.env', '.sav', '.sfo',
)

def verify(source: Path, sd_layout: Path) -> None:
    with ZipFile(source) as archive:
        assert archive.testzip() is None, 'corrupt source archive'
        names = archive.namelist()
        assert 'CMakeLists.txt' in names and 'source/psp_core.c' in names
        assert not any(n.lower().endswith(FORBIDDEN_SUFFIXES) for n in names), (
            'unexpected ROM, native binary, save or secret file in source archive'
        )
        assert not any('__pycache__/' in n or '/.git/' in n for n in names)
    with ZipFile(sd_layout) as archive:
        assert archive.testzip() is None, 'corrupt SD archive'
        names = archive.namelist()
        for path in ('psp/', 'psp/roms/', 'psp/roms/save/'):
            assert path in names and archive.getinfo(path).is_dir()
        assert 'psp/roms/README.txt' in names
        assert not any(n.lower().endswith((
            '.iso', '.cso', '.pbp', '.keys', '.key', '.pem', '.sav'))
            for n in names)
        for n in names:
            if n.lower().endswith(('.nds', '.3dsx', '.cia')):
                assert archive.getinfo(n).file_size > 0

if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit('usage: verify_release.py source.zip sd-layout.zip')
    verify(Path(sys.argv[1]), Path(sys.argv[2]))
    print('Verified source and SD-layout packages contain no ROMs or private assets')
