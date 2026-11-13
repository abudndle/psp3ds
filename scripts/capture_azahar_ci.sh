#!/usr/bin/env bash
# Capture a real Azahar emulator window on an Ubuntu Xvfb runner.
# Fails instead of producing a fake screenshot if launch/window validation fails.
set -euo pipefail
ROM="${1:?Usage: capture_azahar_ci.sh <compiled.3dsx> <output.png>}"
OUTPUT="${2:?Provide screenshot output path}"
: "${AZAHAR_EXE:?Set AZAHAR_EXE to official Azahar Linux AppImage}"
if [[ ! -s "$ROM" || ! -s "$AZAHAR_EXE" ]]; then
  echo 'Compiled 3DSX or Azahar AppImage missing' >&2; exit 2
fi
for cmd in xdotool xwd convert python3; do
  command -v "$cmd" >/dev/null || { echo "Missing $cmd" >&2; exit 2; }
done
mkdir -p "$(dirname "$OUTPUT")"
# Xvfb is launched by the GitHub Actions workflow. No protected game assets.
QT_QPA_PLATFORM=xcb QT_X11_NO_MITSHM=1 LIBGL_ALWAYS_SOFTWARE=1 \
  "$AZAHAR_EXE" --appimage-extract-and-run "$ROM" \
  >"$(dirname "$OUTPUT")/azahar-stdout.log" \
  2>"$(dirname "$OUTPUT")/azahar-stderr.log" &
pid=$!
trap 'kill "$pid" 2>/dev/null || true; wait "$pid" 2>/dev/null || true' EXIT
window=""
for attempt in $(seq 1 45); do
  if ! kill -0 "$pid" 2>/dev/null; then
    echo 'Azahar exited before its window appeared' >&2
    cat "$(dirname "$OUTPUT")/azahar-stderr.log" >&2 || true
    exit 3
  fi
  window="$(xdotool search --onlyvisible --name '[Aa]zahar' 2>/dev/null | head -n 1 || true)"
  if [[ -n "$window" ]]; then break; fi
  sleep 1
done
if [[ -z "$window" ]]; then
  echo 'No visible Azahar emulator window was found' >&2; exit 3
fi
sleep 8
if ! kill -0 "$pid" 2>/dev/null; then
  echo 'Azahar exited before screenshot capture' >&2; exit 3
fi
xwd -silent -id "$window" | convert xwd:- PNG:"$OUTPUT"
python3 - "$OUTPUT" <<'PY'
from PIL import Image
import sys
im=Image.open(sys.argv[1]).convert("RGB")
if im.width < 350 or im.height < 200:
    raise SystemExit("Screenshot too small to verify Azahar window")
colors=im.resize((80,60)).getcolors(80*60)
if colors is not None and len(colors)<5:
    raise SystemExit("Screenshot appears blank")
print(f"Captured real Azahar window: {im.width}x{im.height}")
PY
echo "Saved real emulator-window capture: $OUTPUT"
