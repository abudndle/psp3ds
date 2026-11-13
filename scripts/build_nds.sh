#!/usr/bin/env bash
# Build the portable PSP CPU demonstration with the official BlocksDS template.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BLOCKSDS="${BLOCKSDS:-/opt/blocksds/core}"
TEMPLATE="${BLOCKSDS}/templates/rom_arm9_only"
if [[ ! -f "${TEMPLATE}/Makefile" ]]; then
  echo "BlocksDS template missing: ${TEMPLATE}/Makefile" >&2
  exit 2
fi
BUILD="${ROOT}/build-nds"
mkdir -p "${BUILD}"
cp -R "${TEMPLATE}/." "${BUILD}/"
# Remove SDK example files and stale source from previous stages to avoid
# duplicate entry points. The compiled demo only needs these 3 source files.
rm -rf "${BUILD}/source" "${BUILD}/include"
mkdir -p "${BUILD}/source"
cp "${ROOT}/platforms/nds/main.c" "${BUILD}/source/main.c"
cp "${ROOT}/source/psp_core.c" "${BUILD}/source/psp_core.c"
cp "${ROOT}/include/psp_core.h" "${BUILD}/source/psp_core.h"
make --no-print-directory -C "${BUILD}" "$@"
mapfile -t ROMS < <(find "${BUILD}" -maxdepth 1 -type f -name '*.nds' -print)
if [[ "${#ROMS[@]}" -eq 0 ]]; then
  echo "Build produced no NDS binary" >&2
  exit 3
fi
for rom in "${ROMS[@]}"; do printf 'Built: %s\n' "${rom}"; done
