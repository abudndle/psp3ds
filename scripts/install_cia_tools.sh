#!/usr/bin/env bash
# Build the host-side open-source 3DS homebrew CIA packaging tools.
# Intended for a network-enabled Ubuntu runner (incl. a devkitPro container).
# Never downloads Nintendo keys, copyrighted games, or commercial firmware.
set -euo pipefail
PREFIX="${CIA_TOOLS_PREFIX:-${HOME}/.local}"
mkdir -p "$PREFIX/bin"
if [[ "$(uname -s)" != "Linux" ]]; then
  echo 'Linux is required for this setup script' >&2; exit 2
fi
for tool in git make gcc autoreconf; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Missing prerequisite $tool (install git, build-essential, autoconf, automake, libtool)" >&2
    exit 2
  fi
done
if ! getent ahostsv4 github.com >/dev/null 2>&1; then
  echo 'Cannot resolve github.com: tool downloads require a network-enabled runner' >&2
  exit 3
fi
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
git clone --depth 1 --branch cxi-stuff https://github.com/devkitPro/3dstools.git "$work/3dstools"
(
  cd "$work/3dstools"
  autoreconf -fi
  ./configure
  make -j2
)
if [[ ! -s "$work/3dstools/cxitool" ]]; then
  echo 'cxitool was not produced by the upstream source build' >&2; exit 4
fi
install -m 0755 "$work/3dstools/cxitool" "$PREFIX/bin/cxitool"
git clone --depth 1 https://github.com/3DSGuy/Project_CTR.git "$work/Project_CTR"
(
  cd "$work/Project_CTR/makerom"
  make deps
  make -j2
)
if [[ ! -s "$work/Project_CTR/makerom/bin/makerom" ]]; then
  echo 'makerom was not produced by the upstream source build' >&2; exit 4
fi
install -m 0755 "$work/Project_CTR/makerom/bin/makerom" "$PREFIX/bin/makerom"
"$PREFIX/bin/cxitool" --help >/dev/null 2>&1 || test "$?" -eq 1
"$PREFIX/bin/makerom" -help >/dev/null 2>&1 || test "$?" -eq 1
printf 'CIA packaging tools installed in %s/bin\n' "$PREFIX"
