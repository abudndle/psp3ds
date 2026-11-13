#!/usr/bin/env python3
"""Portable preflight for real emulator screenshot generation in GitHub Actions.

This test does not substitute a simulated screenshot for a running emulator.
"""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
script=ROOT/"scripts/capture_melonds_ci.sh"
workflow=ROOT/".github/workflows/emulator-screenshots.yml"
subprocess.run(["bash","-n",str(script)],check=True)
w=workflow.read_text(encoding="utf-8")
s=script.read_text(encoding="utf-8")
required_workflow=(
    "ctest --test-dir build-ci --output-on-failure",
    "bash scripts/build_nds.sh",
    "xvfb-run",
    "capture_melonds_ci.sh",
    "psp3ds-melonds.png",
    "actions/upload-artifact@v4",
    "if: always()",
)
for value in required_workflow:
    assert value in w, f"Missing screenshot workflow operation: {value}"
for value in ("xdotool search --onlyvisible","kill -0","xwd -silent","unique < 5"):
    assert value in s, f"Missing genuine-window verification: {value}"
print("PASS: Linux emulator workflow syntax, real-window capture and diagnostic uploads")
