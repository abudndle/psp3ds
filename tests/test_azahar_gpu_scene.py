"""Synthetic test fixture for the visual detector, NEVER an emulator capture."""
import importlib.util
import tempfile
from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("scene_detector",root/"scripts/verify_azahar_gpu_scene.py")
detector=importlib.util.module_from_spec(spec)
spec.loader.exec_module(detector)
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory)/"synthetic-fixture.png"
    scene=Image.new("RGB",(853,526),(34,34,36))
    paint=ImageDraw.Draw(scene)
    paint.rectangle((280,260,599,439),fill=(0,131,255))
    paint.rectangle((280,395,599,439),fill=(123,107,0))
    paint.rectangle((335,330,343,339),fill=(255,0,255))
    scene.save(path)
    assert detector.detect(path)[0],detector.detect(path)
    # A 3-bar boot marker may share cyan and magenta, but not brown ground.
    marker=Image.new("RGB",(853,526),(34,34,36))
    d=ImageDraw.Draw(marker)
    d.rectangle((280,270,599,315),fill=(255,0,255))
    d.rectangle((280,316,599,360),fill=(0,255,255))
    d.rectangle((280,361,599,409),fill=(255,255,0))
    marker.save(path)
    assert not detector.detect(path)[0]
    # A colorful but otherwise blank window must not count as guest scene.
    blank=Image.new("RGB",(853,526),(34,34,36))
    blank.save(path)
    assert not detector.detect(path)[0]
    scene.save(path)
    d=ImageDraw.Draw(scene)
    d.rectangle((280,260,599,439),fill=(0,131,255))
    d.rectangle((280,395,599,439),fill=(123,107,0))
    # Invert scene ordering: should reject.
    bad=Image.new("RGB",(853,526),(34,34,36))
    d=ImageDraw.Draw(bad)
    d.rectangle((280,260,599,304),fill=(123,107,0))
    d.rectangle((280,305,599,439),fill=(0,131,255))
    d.rectangle((330,360,338,370),fill=(255,0,255))
    bad.save(path)
    assert not detector.detect(path)[0]
    path.write_bytes(b"not PNG")
    assert not detector.detect(path)[0]
    path.unlink()
    assert not detector.detect(path)[0]
print("PASS: Azahar GPU scene detector synthetic reference, marker rejection, misplaced layers, invalid file")
