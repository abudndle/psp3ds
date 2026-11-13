"""Synthetic image regression for real emulator boot-marker detection.

These fixtures test only the detector. They are not Azahar screenshots and
must never be published as emulator proof.
"""
import importlib.util
from pathlib import Path
import tempfile
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "boot_marker_verifier", ROOT / "scripts/verify_azahar_boot_marker.py")
verify = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify)

COLORS = [(255, 0, 255), (0, 255, 255), (255, 255, 0)]


def screenshot(path, order=(0, 1, 2), vertical=False):
    image = Image.new("RGB", (853, 526), (44, 48, 53))
    draw = ImageDraw.Draw(image)
    for i, color_index in enumerate(order):
        if vertical:
            # Rotated emulator presentation: stacked horizontal bands.
            rect = (260, 290 + i * 58, 550, 347 + i * 58)
        else:
            rect = (225 + i * 90, 300, 314 + i * 90, 480)
        draw.rectangle(rect, fill=COLORS[color_index])
    image.save(path)


def main():
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "fixture.png"
        screenshot(path)
        ok, reason, measures = verify.detect(path)
        assert ok, (reason, measures)
        assert measures["orientation"] == "horizontal"
        screenshot(path, vertical=True)
        ok, reason, measures = verify.detect(path)
        assert ok, (reason, measures)
        assert measures["orientation"] == "vertical"
        screenshot(path, order=(2, 1, 0))
        assert verify.detect(path)[0]  # screen may be rotated 180 degrees
        screenshot(path, order=(0, 2, 1))
        assert not verify.detect(path)[0]  # incorrect color sequence
        Image.new("RGB", (853, 526), (41, 41, 41)).save(path)
        assert not verify.detect(path)[0]
        image = Image.new("RGB", (853, 526), (44, 48, 53))
        ImageDraw.Draw(image).rectangle((230, 300, 510, 470), fill=COLORS[0])
        image.save(path)
        assert not verify.detect(path)[0]  # one colored rectangle
        path.write_bytes(b"not a PNG")
        assert not verify.detect(path)[0]
        path.unlink()
        assert not verify.detect(path)[0]
    print("PASS: true boot signature, rotations, wrong order, blank, partial, corrupt, missing")


if __name__ == "__main__":
    main()
