#!/usr/bin/env python3
"""Write evidence of a real emulator-window screenshot attempt.

This is not proof of PSP gameplay or even successful guest boot.
A manifest is written even if the screenshot is absent or corrupt.
"""
import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path
from PIL import Image, UnidentifiedImageError

def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()

def report(emulator, screenshot, executable):
    data = {
        "emulator": emulator,
        "kind": "emulator_window_capture_only",
        "game_boot_verified": False,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "git_sha": os.environ.get("GITHUB_SHA", ""),
        "capture": "not_captured",
        "reason": "",
        "executable": None,
        "screenshot": None,
    }
    if executable.is_file() and executable.stat().st_size:
        data["executable"] = {
            "name": executable.name, "bytes": executable.stat().st_size,
            "sha256": sha256(executable),
        }
    else:
        data["reason"] = "Executable missing or empty"
        return data
    if not screenshot.is_file() or not screenshot.stat().st_size:
        data["reason"] = "Screenshot file missing or empty"
        return data
    try:
        with Image.open(screenshot) as image:
            image.verify()
        with Image.open(screenshot) as image:
            if image.format != "PNG":
                raise ValueError("Not a PNG image")
            width, height = image.size
            if width < 200 or height < 150:
                raise ValueError("Image dimensions too small")
            colors = image.convert("RGB").resize((80, 60)).getcolors(80 * 60)
            if colors is not None and len(colors) < 5:
                raise ValueError("Image appears blank or nearly uniform")
    except (OSError, UnidentifiedImageError, ValueError) as exc:
        data["reason"] = f"Invalid screenshot: {exc}"
        return data
    data["capture"] = "captured"
    data["screenshot"] = {
        "name": screenshot.name, "bytes": screenshot.stat().st_size,
        "sha256": sha256(screenshot), "width": width, "height": height,
    }
    return data

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--emulator", choices=["melonds", "azahar"], required=True)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--screenshot", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    result = report(args.emulator, args.screenshot, args.executable)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf8")
    print(f"{result['emulator']}: {result['capture']}: {result['reason'] or 'window image verified'}")
    return 0 if result["capture"] == "captured" else 1

if __name__ == "__main__":
    raise SystemExit(main())
