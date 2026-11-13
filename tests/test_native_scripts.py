#!/usr/bin/env python3
"""Validate build-script syntax and fail-closed CIA input handling."""
from pathlib import Path
from subprocess import run
root = Path(__file__).resolve().parents[1]
for name in ("build_nds.sh", "build_cia.sh"):
    script = root / "scripts" / name
    assert run(["bash", "-n", str(script)], check=False).returncode == 0
r = run(["bash", str(root / "scripts/build_cia.sh"),
         str(root / "no-such-3dsx.3dsx")], capture_output=True, text=True)
assert r.returncode == 2 and "No compiled 3DSX" in r.stderr
print("PASS: native build script syntax and missing-CIA-input guard")
