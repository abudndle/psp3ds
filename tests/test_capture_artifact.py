"""Verify downloaded emulator evidence and reject tampered files/metadata."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
from PIL import Image

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "verify_capture_artifact.py"
spec = importlib.util.spec_from_file_location("capture_artifact", SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        exe = root / "psp3ds.3dsx"
        exe.write_bytes(b"fixture executable")
        png = root / "azahar.png"
        image = Image.new("RGB", (320, 240))
        for y in range(240):
            for x in range(320):
                image.putpixel((x, y), (x % 256, y % 256, (x + y) % 256))
        image.save(png)
        data = {
            "kind": "emulator_window_capture_only", "emulator": "azahar",
            "game_boot_verified": False, "capture": "captured", "reason": "",
            "executable": {"name": exe.name, "bytes": exe.stat().st_size, "sha256": checksum(exe)},
            "screenshot": {"name": png.name, "bytes": png.stat().st_size, "sha256": checksum(png), "width": 320, "height": 240},
        }
        manifest = root / "evidence.json"
        def check(expected):
            manifest.write_text(json.dumps(data))
            actual, detail = module.verify(manifest, root)
            assert actual is expected, detail
        check(True)
        data["game_boot_verified"] = True
        check(False)
        data["game_boot_verified"] = False
        data["screenshot"]["name"] = "../azahar.png"
        check(False)
        data["screenshot"]["name"] = png.name
        data["screenshot"]["width"] = 321
        check(False)
        data["screenshot"]["width"] = 320
        original = exe.read_bytes()
        exe.write_bytes(b"fixture executablE")
        check(False)
        exe.write_bytes(original)
        png.write_bytes(b"corrupted PNG")
        check(False)
        data["capture"] = "not_captured"
        data["reason"] = "Screenshot missing"
        data["screenshot"] = None
        check(True)
        data["reason"] = ""
        check(False)
    print("PASS: downloaded screenshot evidence integrity and failure-state checks")


if __name__ == "__main__":
    main()
