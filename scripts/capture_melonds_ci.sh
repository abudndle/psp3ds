#!/usr/bin/env bash
# Capture a real visible melonDS window using a Linux virtual framebuffer.
# Never synthesize a screenshot when emulator execution fails.
set -euo pipefail
ROM="${1:?Usage: capture_melonds_ci.sh <built.nds> <output.png>}"
OUTPUT="${2:?Provide screenshot destination}"
if [[ ! -s "$ROM" ]]; then echo "Missing NDS ROM: $ROM" >&2; exit 2; fi
for cmd in xdotool xwd convert python3 timeout; do
  if ! command -v "$cmd" >/dev/null; then echo "Missing $cmd" >&2; exit 2; fi
done
mkdir -p "$(dirname "$OUTPUT")"
: "${MELONDS_EXE:?Set MELONDS_EXE to the downloaded official melonDS AppImage}"
if [[ ! -s "$MELONDS_EXE" ]]; then echo "Missing melonDS binary" >&2; exit 2; fi
# Prevent crash prompts or Qt trying to access a real graphics session.
QT_X11_NO_MITSHM=1 LIBGL_ALWAYS_SOFTWARE=1   "$MELONDS_EXE" --appimage-extract-and-run "$ROM"   >"$(dirname "$OUTPUT")/melonds-stdout.log" 2>"$(dirname "$OUTPUT")/melonds-stderr.log" &
emulator_pid=$!
trap 'kill "$emulator_pid" 2>/dev/null || true; wait "$emulator_pid" 2>/dev/null || true' EXIT
window=""
for attempt in $(seq 1 40); do
  if ! kill -0 "$emulator_pid" 2>/dev/null; then
    echo 'melonDS exited before a window was displayed' >&2
    cat "$(dirname "$OUTPUT")/melonds-stderr.log" >&2 || true
    exit 3
  fi
  window="$(xdotool search --onlyvisible --name 'melonDS' 2>/dev/null | head -n 1 || true)"
  if [[ -n "$window" ]]; then break; fi
  sleep 1
done
if [[ -z "$window" ]]; then echo 'No visible melonDS window appeared' >&2; exit 3; fi
xdotool windowactivate --sync "$window" || true
sleep 4
if ! kill -0 "$emulator_pid" 2>/dev/null; then
  echo 'melonDS exited before screenshot capture' >&2; exit 3
fi
xwd -silent -id "$window" | convert xwd:- PNG:"$OUTPUT"
python3 - "$OUTPUT" <<'PY'
from PIL import Image
import sys
p=sys.argv[1]
image=Image.open(p).convert("RGB")
if image.width < 200 or image.height < 150:
    raise SystemExit("Screenshot dimensions too small")
colors=image.resize((80,60)).getcolors(80*60)
if colors is None:
    unique=80*60
else:
    unique=len(colors)
if unique < 5:
    raise SystemExit("Screenshot appears blank; not claiming successful capture")
print(f"Captured actual melonDS window: {image.width}x{image.height}, {unique} sampled colors")
PY
echo "Saved emulator screenshot: $OUTPUT"
