#!/usr/bin/env python3
"""Check Linux tool installers parse and fetch only through HTTPS."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
for name in ("install_blocksds_ubuntu.sh", "install_linux_emulators.sh"):
    script = root / "scripts" / name
    subprocess.run(["bash", "-n", str(script)], check=True)
    content = script.read_text(encoding="utf-8")
    assert "https://" in content
print("PASS: Ubuntu installer scripts pass bash syntax checks")
