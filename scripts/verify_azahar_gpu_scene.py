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
    if r < 65 and g > 185 and b > 185:
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
            # Downsampled visual evidence is sufficient for this synthetic
            # palette diagnostic, independent of the emulator window size.
            points = {"sky": [], "ground": [], "actor": []}
            for y in range(0,h,2):
                for x in range(0,w,2):
                    kind = tag(im.getpixel((x,y)))
                    if kind:
                        points[kind].append((x,y))
            counts = {k:len(v) for k,v in points.items()}
            if counts["sky"] < 1100 or counts["ground"] < 150 or not 3 <= counts["actor"] <= 7000:
                return False, "Scene sky/ground/actor colors missing", {"sample_counts": counts}
            bounds = {k:(
                min(p[0] for p in values), min(p[1] for p in values),
                max(p[0] for p in values), max(p[1] for p in values)
            ) for k,values in points.items()}
            sx0,sy0,sx1,sy1=bounds["sky"]
            gx0,gy0,gx1,gy1=bounds["ground"]
            ax0,ay0,ax1,ay1=bounds["actor"]
            if sx1-sx0 < 90 or gx1-gx0 < 85 or sx1-sx0 > w*0.9:
                return False, "Sky/ground not broad enough for the synthetic LCD scene", {"bounds": bounds}
            if not (sy0 < ay0 < gy0 and sy1 < gy1 and
                    sx0 <= ax0 <= sx1 and ax1 <= sx1 and
                    gx0 <= sx1 and gx1 >= sx0):
                return False, "Scene layers/marker not in expected positions", {"bounds":bounds}
            return True,"Original PSP3DS synthetic scene spatial colors visible",{
                "sample_counts":counts,"bounds":bounds,"width":w,"height":h
            }
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
