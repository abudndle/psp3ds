#!/usr/bin/env bash
# Reuses the Wonderful/BlocksDS setup from abudndle/mii-maker-ds.
# Intended for a fresh Ubuntu GitHub Actions runner with network access.
set -euo pipefail
if [[ "$(uname -s)" != Linux ]]; then
  echo 'Linux required' >&2; exit 2
fi
if command -v wf-pacman >/dev/null 2>&1 && [[ -f "${BLOCKSDS:-/opt/wonderful/thirdparty/blocksds/core}/templates/rom_arm9_only/Makefile" ]]; then
  echo 'BlocksDS is already installed'; exit 0
fi
if ! getent ahostsv4 wonderful.asie.pl >/dev/null; then
  echo 'Cannot resolve wonderful.asie.pl: package installation needs network access' >&2; exit 3
fi
if ! command -v curl >/dev/null || ! command -v tar >/dev/null; then
  echo 'Install curl and tar before running this setup' >&2; exit 4
fi
PREFIX="${WONDERFUL_TOOLCHAIN:-/opt/wonderful}"
mkdir -p "$PREFIX"
if [[ ! -e "$PREFIX/bin/wf-pacman" ]]; then
  temp="$(mktemp -d)"
  trap 'rm -rf "$temp"' EXIT
  curl --fail --location --silent --show-error --retry 3 \
    https://wonderful.asie.pl/bootstrap/wf-bootstrap-x86_64.tar.gz -o "$temp/bootstrap.tar.gz"
  tar -xzf "$temp/bootstrap.tar.gz" -C "$PREFIX"
fi
export PATH="$PREFIX/bin:$PATH"
wf-pacman -Syu --noconfirm wf-tools
wf-config repo enable blocksds
wf-pacman -Syu --noconfirm
wf-pacman -S --noconfirm blocksds-toolchain
BLOCKSDS="${BLOCKSDS:-$PREFIX/thirdparty/blocksds/core}"
test -s "$BLOCKSDS/templates/rom_arm9_only/Makefile"
command -v wf-pacman
printf 'Installed BlocksDS: %s\n' "$BLOCKSDS"
