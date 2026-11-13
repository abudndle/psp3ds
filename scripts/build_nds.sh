#!/usr/bin/env bash
# BlocksDS ARM9-only template staging for the portable PSP CPU subset.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BLOCKSDS="${BLOCKSDS:-/opt/blocksds/core}"
TEMPLATE="$BLOCKSDS/templates/rom_arm9_only"
if [[ ! -f "$TEMPLATE/Makefile" ]]; then
  echo "Missing BlocksDS template: $TEMPLATE" >&2; exit 2
fi
BUILD="$ROOT/build-nds"
mkdir -p "$BUILD"
cp -R "$TEMPLATE"/. "$BUILD"/
mkdir -p "$BUILD/source" "$BUILD/include"
cp "$ROOT/platforms/nds/main.c" "$BUILD/source/main.c"
cp "$ROOT/source/psp_core.c" "$BUILD/source/psp_core.c"
cp "$ROOT/include/psp_core.h" "$BUILD/include/psp_core.h"
if ! grep -q '^INCLUDEDIRS' "$BUILD/Makefile"; then
  printf '\nINCLUDEDIRS += include\n' >> "$BUILD/Makefile"
fi
make -C "$BUILD" --no-print-directory "$@"
find "$BUILD" -maxdepth 1 -name '*.nds' -print
