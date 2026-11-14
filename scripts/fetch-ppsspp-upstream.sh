#!/usr/bin/env bash
# Fetch a reproducible, opt-in PPSSPP upstream source snapshot for porting.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHA="1aebad2c651ff9d0684ee335e43aceff8fc7e3ec"
DEST="${1:-$ROOT/vendor/ppsspp}"
if [[ -e "$DEST" ]]; then
  echo "Refusing to replace existing path: $DEST" >&2
  exit 1
fi
mkdir -p "$(dirname "$DEST")"
git clone --filter=blob:none --no-checkout https://github.com/hrydgard/ppsspp.git "$DEST"
git -C "$DEST" sparse-checkout init --cone
git -C "$DEST" sparse-checkout set Common Core/MIPS Core/HLE Core/HW GPU Core/FileSystems Core/FileLoaders
git -C "$DEST" checkout --detach "$SHA"
ACTUAL="$(git -C "$DEST" rev-parse HEAD)"
if [[ "$ACTUAL" != "$SHA" ]]; then
  echo "PPSSPP revision mismatch" >&2
  exit 1
fi
printf 'PPSSPP source ready at %s (revision %s)\n' "$DEST" "$ACTUAL"
printf 'Review upstream licensing and dependencies before integrating any code.\n'
