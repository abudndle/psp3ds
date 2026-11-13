#!/usr/bin/env python3
"""Independently verify PSP3DS emulator screenshot evidence after download.

This checks artifact integrity, not guest boot, gameplay, or screenshot origin.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, UnidentifiedImageError


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def verify(manifest, directory):
    try:
        data = json.loads(manifest.read_text(encoding="utf8"))
        if not isinstance(data, dict) or data.get("kind") != "emulator_window_capture_only":
            return False, "Unexpected evidence format"
        if data.get("emulator") not in ("azahar", "melonds") or data.get("game_boot_verified") is not False:
            return False, "Unsupported emulator or gameplay assertion"
        state = data.get("capture")
        if state == "not_captured":
            if data.get("screenshot") is not None or not isinstance(data.get("reason"), str) or not data["reason"]:
                return False, "Invalid failed-capture record"
            return True, "Valid failure manifest; no screenshot captured"
        if state != "captured" or data.get("reason") != "":
            return False, "Invalid capture status"
        for field in ("executable", "screenshot"):
            record = data.get(field)
            if not isinstance(record, dict):
                return False, f"Missing {field} record"
            name = record.get("name")
            if not isinstance(name, str) or not name or Path(name).name != name or name in (".", ".."):
                return False, f"Unsafe {field} name"
            path = directory / name
            if not path.is_file() or path.stat().st_size != record.get("bytes") or path.stat().st_size == 0:
                return False, f"{field} file absent or wrong size"
            if not isinstance(record.get("sha256"), str) or digest(path) != record["sha256"]:
                return False, f"{field} SHA-256 mismatch"
        png = directory / data["screenshot"]["name"]
        with Image.open(png) as image:
            image.verify()
        with Image.open(png) as image:
            if image.format != "PNG" or list(image.size) != [data["screenshot"].get("width"), data["screenshot"].get("height")]:
                return False, "PNG dimensions or format mismatch"
            if image.width < 200 or image.height < 150:
                return False, "Screenshot is too small"
            colors = image.convert("RGB").resize((80, 60)).getcolors(4800)
            if colors is not None and len(colors) < 5:
                return False, "Screenshot appears blank"
        return True, "Capture artifact hashes and image verified; game boot NOT verified"
    except (OSError, ValueError, TypeError, KeyError, UnidentifiedImageError, json.JSONDecodeError) as exc:
        return False, f"Invalid evidence: {exc}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, help="Downloaded evidence.json")
    parser.add_argument("--directory", type=Path, help="Directory with executable and screenshot")
    args = parser.parse_args()
    ok, message = verify(args.manifest, args.directory or args.manifest.parent)
    print(message)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
