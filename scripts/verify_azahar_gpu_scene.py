#!/usr/bin/env python3
"""Fail-closed visible-pixel proof of PSP3DS's original synthetic GPU scene.

This verifies only expected screen colors/spatial arrangement in an actual
Azahar capture; it cannot prove 3DS hardware speed, PSP GE, or game boot.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, UnidentifiedImageError

def tag(pixel):
    r, g, b = pixel[:3]
    if r < 65 and 100 < g < 195 and b > 200:
        return "sky"
    if 65 < r < 165 and 65 < g < 165 and b < 70:
        return "ground"
    if r > 175 and g < 90 and b > 175:
        return "actor"
    return None

def detect(path):
    try:
        with Image.open(path) as check:
            check.verify()
        with Image.open(path) as opened:
            if opened.format != "PNG":
                return False, "Not a PNG", {}
            im = opened.convert("RGB")
            w, h = im.size
            if w < 350 or h < 240:
                return False, "Emulator window too small", {}

            # Find a long *contiguous* horizontal sky run. Looking at total
            # pixel colors across the window would also include Azahar's
            # menus, console text and status bar, creating false negatives.
            best = (0, 0, 0, 0)  # run length in sampled pixels, x0, x1, y
            for y in range(0, h, 2):
                start = None
                for x in range(0, w + 2, 2):
                    sky = x < w and tag(im.getpixel((x, y))) == "sky"
                    if sky and start is None:
                        start = x
                    elif not sky and start is not None:
                        length = (x - start) // 2
                        if length > best[0]:
                            best = (length, start, min(x - 2, w - 1), y)
                        start = None
            if best[0] < 60:
                return False, "No broad contiguous synthetic sky layer", {
                    "longest_sky_run_samples": best[0]}
            _, x0, x1, _ = best
            # Identify sky rows from the same X interval, allowing one
            # small moving marker to interrupt the blue pixel streak.
            sky_rows = []
            for y in range(0, h, 2):
                sky_samples = sum(
                    tag(im.getpixel((x, y))) == "sky"
                    for x in range(x0, x1 + 1, 2)
                )
                if sky_samples >= (x1 - x0) // 4:
                    sky_rows.append(y)
            if len(sky_rows) < 28:
                return False, "Sky layer too short for a 3DS screen", {
                    "sky_rows": len(sky_rows)}
            top, sky_bottom = min(sky_rows), max(sky_rows)
            if sky_bottom - top < 90:
                return False, "Sky layer lacks expected height", {
                    "sky_top": top, "sky_bottom": sky_bottom}
            ground = 0
            actor = []
            for y in range(top, min(h, sky_bottom + 110), 2):
                for x in range(x0, x1 + 1, 2):
                    kind = tag(im.getpixel((x,y)))
                    if kind == "ground" and y > sky_bottom:
                        ground += 1
                    if kind == "actor" and top < y < sky_bottom - 2:
                        actor.append((x,y))
            if ground < 150 or not 3 <= len(actor) <= 500:
                return False, "Ground or upper-screen actor not present", {
                    "ground_samples": ground, "actor_samples": len(actor),
                    "scene_x": [x0,x1], "sky_y": [top,sky_bottom]}
            ax = [v[0] for v in actor]
            ay = [v[1] for v in actor]
            if max(ax) - min(ax) > 50 or max(ay) - min(ay) > 50:
                return False, "Moving marker is not compact", {
                    "actor_bounds": [min(ax),min(ay),max(ax),max(ay)]}
            return True, "Original PSP3DS synthetic scene pixels visible", {
                "sky_x": [x0,x1], "sky_y": [top,sky_bottom],
                "actor_bounds": [min(ax),min(ay),max(ax),max(ay)],
                "ground_samples": ground, "width": w, "height": h}
    except (OSError,ValueError,UnidentifiedImageError) as exc:
        return False,f"Missing/invalid capture: {exc}",{}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image",type=Path,required=True)
    parser.add_argument("--report",type=Path,required=True)
    args=parser.parse_args()
    passed,reason,observations=detect(args.image)
    result={
        "kind":"psp3ds_synthetic_3ds_gpu_scene",
        "azahar_scene_visible":passed,
        "pica_gpu_calls_compiled":None,
        "physical_3ds_verified":False,
        "psp_ge_translation_verified":False,
        "jetpack_joyride_gameplay_verified":False,
        "screenshot_sha256":hashlib.sha256(args.image.read_bytes()).hexdigest() if args.image.is_file() else None,
        "reason":reason,"observations":observations
    }
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(result,indent=2,sort_keys=True)+"\n")
    print(f"Azahar original GPU scene pixels: {'PASS' if passed else 'FAIL'}: {reason}")
    return 0 if passed else 1
if __name__=="__main__":
    raise SystemExit(main())
