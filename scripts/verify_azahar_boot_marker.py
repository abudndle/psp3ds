#!/usr/bin/env python3
"""Detect PSP3DS's diagnostic lower-screen color signature in an Azahar capture.

A positive result verifies that application-produced pixels appear in the
captured emulator window; it does NOT verify PSP executable boot/gameplay.
No OCR, fabricated imagery, or emulator process-status inference is used.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, UnidentifiedImageError


def classify(rgb):
    red, green, blue = rgb
    if red >= 175 and green <= 105 and blue >= 175:
        return 0  # magenta
    if red <= 105 and green >= 175 and blue >= 175:
        return 1  # cyan
    if red >= 175 and green >= 175 and blue <= 105:
        return 2  # yellow
    return -1


def detect(path):
    try:
        with Image.open(path) as opened:
            opened.verify()
        with Image.open(path) as opened:
            if opened.format != "PNG":
                return False, "Not a PNG", {}
            image = opened.convert("RGB")
            width, height = image.size
            if width < 350 or height < 200:
                return False, "Window screenshot is too small", {}
            pixels = image.load()
            n = [0, 0, 0]
            sx = [0, 0, 0]
            sy = [0, 0, 0]
            # Sample only real pixels from the capture, no synthetic replacement.
            for y in range(0, height, 2):
                for x in range(0, width, 2):
                    band = classify(pixels[x, y])
                    if band >= 0:
                        n[band] += 1
                        sx[band] += x
                        sy[band] += y
            if min(n) < 300:
                return False, "Distinct boot-marker color bands missing", {"samples": n}
            centers = [(sx[i] / n[i], sy[i] / n[i]) for i in range(3)]
            # A 3DS lower-screen marker consists of three adjacent colored
            # stripes. Accommodate the emulator's screen rotation/layout.
            for axis in (0, 1):
                other = 1 - axis
                primary = [xy[axis] for xy in centers]
                transverse = [xy[other] for xy in centers]
                direction = (primary[1] - primary[0]) * (primary[2] - primary[1])
                dimension = width if axis == 0 else height
                other_dimension = height if axis == 0 else width
                if (direction > 0 and
                    min(abs(primary[1] - primary[0]),
                        abs(primary[2] - primary[1])) >= 12 and
                    max(primary) - min(primary) <= dimension * 0.65 and
                    max(transverse) - min(transverse) <= other_dimension * 0.14):
                    return True, "PSP3DS diagnostic bottom-screen colors visible", {
                        "samples": n,
                        "centers": [[round(x, 1), round(y, 1)] for x, y in centers],
                        "orientation": "horizontal" if axis == 0 else "vertical",
                        "width": width,
                        "height": height,
                    }
            return False, "Color bands are not adjacent and ordered", {
                "samples": n,
                "centers": [[round(x, 1), round(y, 1)] for x, y in centers],
            }
    except (OSError, ValueError, UnidentifiedImageError) as exc:
        return False, f"Missing/invalid screenshot: {exc}", {}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    passed, reason, measurements = detect(args.image)
    sha = None
    if args.image.is_file():
        sha = hashlib.sha256(args.image.read_bytes()).hexdigest()
    result = {
        "kind": "psp3ds_homebrew_boot_marker",
        "homebrew_boot_marker_visible": passed,
        "psp_game_boot_verified": False,
        "emulator": "azahar",
        "screenshot_sha256": sha,
        "reason": reason,
        "measurements": measurements,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(f"Azahar PSP3DS homebrew marker: {'PASS' if passed else 'FAIL'}: {reason}")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
