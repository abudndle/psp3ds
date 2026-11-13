#!/usr/bin/env bash
# Package an existing 3DSX as a homebrew CIA with external cxitool/makerom.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT="${1:-$ROOT/psp3ds.3dsx}"
OUTPUT="${2:-$ROOT/psp3ds.cia}"
if [[ ! -s "$INPUT" ]]; then
  echo "No compiled 3DSX at $INPUT; build 3DS first" >&2
  exit 2
fi
for tool in cxitool makerom; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Missing $tool; CIA packaging unavailable" >&2
    exit 2
  fi
done
TEMP_CXI="$(mktemp "${TMPDIR:-/tmp}/psp3ds-XXXXXXXX.cxi")"
trap 'rm -f "$TEMP_CXI"' EXIT
cxitool -n PSP3DS "$INPUT" "$TEMP_CXI"
# Unsigned homebrew CXIs do not have an official access-descriptor signature.
# Only skip descriptor-signature verification during homebrew packaging;
# the generated CIA is still subject to independent bounds validation.
makerom -f cia -o "$OUTPUT" -target t -ignoresign -i "${TEMP_CXI}:0:0"
if [[ ! -s "$OUTPUT" ]]; then
  echo "No CIA output produced" >&2
  exit 3
fi
echo "Built: $OUTPUT"
