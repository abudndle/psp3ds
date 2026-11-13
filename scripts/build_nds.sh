#!/usr/bin/env bash
# Build the portable PSP CPU demonstration using this repository's BlocksDS
# project. Never requires a template omitted from slim SDK containers.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BLOCKSDS="${BLOCKSDS:-/opt/blocksds/core}"
if [[ ! -f "${BLOCKSDS}/sys/crts/ds_arm9.specs" ]]; then
  echo "BlocksDS ARM9 toolchain missing: ${BLOCKSDS}/sys/crts/ds_arm9.specs" >&2
  exit 2
fi
make --no-print-directory -C "${ROOT}/platforms/nds" "$@"
BUILD="${ROOT}/build-nds"
mkdir -p "${BUILD}"
ROM="${ROOT}/platforms/nds/psp3ds_nds.nds"
if [[ ! -s "${ROM}" ]]; then
  echo "Build produced no NDS binary" >&2
  exit 3
fi
cp "${ROM}" "${BUILD}/psp3ds_nds.nds"
printf 'Built: %s\n' "${BUILD}/psp3ds_nds.nds"
