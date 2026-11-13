#!/usr/bin/env python3
"""Portable CIA packaging CI checks that need no Nintendo SDK."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
for script in ("install_cia_tools.sh","build_cia.sh"):
    subprocess.run(["bash","-n",str(root/"scripts"/script)],check=True)
workflow=(root/".github/workflows/native-3ds.yml").read_text()
for expected in (
    "scripts/install_cia_tools.sh",
    "scripts/build_cia.sh",
    "test_cia_output.py",
    "psp3ds-native-cia",
    "psp3ds-native-3dsx",
    "actions/upload-artifact@v4"
):
    assert expected in workflow, f"Missing CI build step: {expected}"
checker=root/"tests/test_cia_output.py"
with tempfile.TemporaryDirectory() as directory:
    sample=Path(directory)/"fixture.cia"
    header=bytearray(0x2020)
    struct.pack_into("<IHHIIIIQ",header,0,0x2020,0,0,0x40,0x40,0x40,0,0x80)
    sample.write_bytes(header+b"C"*0x40+b"T"*0x40+b"M"*0x40+b"X"*0x80)
    subprocess.run([sys.executable,str(checker),str(sample)],check=True)
    sample.write_bytes(bytes(0x2020))
    bad=subprocess.run([sys.executable,str(checker),str(sample)],capture_output=True)
    assert bad.returncode!=0, "Invalid CIA must not be accepted"
print("PASS: packaging scripts, GitHub artifact workflow and CIA header validation")
