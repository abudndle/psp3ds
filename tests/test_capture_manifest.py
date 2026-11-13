"""Regression checks for emulator screenshot provenance manifests."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/verify_emulator_capture.py"

def invoke(emulator, executable, png, manifest):
    return subprocess.run([
        sys.executable, str(SCRIPT), "--emulator", emulator,
        "--executable", str(executable), "--screenshot", str(png),
        "--manifest", str(manifest),
    ], capture_output=True, text=True)

def main():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        exe = root / "psp3ds.3dsx"
        exe.write_bytes(b"test fixture only")
        png = root / "capture.png"
        manifest = root / "evidence.json"
        assert invoke("azahar", exe, png, manifest).returncode != 0
        r = json.loads(manifest.read_text())
        assert r["capture"] == "not_captured" and not r["game_boot_verified"]
        assert r["executable"]["sha256"]
        Image.new("RGB", (500, 400), (0, 0, 0)).save(png)
        assert invoke("melonds", exe, png, manifest).returncode != 0
        assert json.loads(manifest.read_text())["capture"] == "not_captured"
        im = Image.new("RGB", (500, 400))
        for y in range(400):
            for x in range(500):
                im.putpixel((x, y), (x % 256, y % 256, (x+y) % 256))
        im.save(png)
        ok = invoke("azahar", exe, png, manifest)
        assert ok.returncode == 0, ok.stderr + ok.stdout
        r = json.loads(manifest.read_text())
        assert r["capture"] == "captured"
        assert r["screenshot"]["width"] == 500
        assert len(r["screenshot"]["sha256"]) == 64
        assert r["kind"] == "emulator_window_capture_only"
        assert not r["game_boot_verified"]
        png.write_bytes(b"not png")
        assert invoke("azahar", exe, png, manifest).returncode != 0
        assert json.loads(manifest.read_text())["capture"] == "not_captured"
    print("PASS: screenshot capture evidence rejects missing, blank and corrupt PNGs")

if __name__ == "__main__":
    main()
