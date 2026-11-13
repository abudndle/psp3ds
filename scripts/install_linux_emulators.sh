#!/usr/bin/env bash
# Ubuntu desktop/CI emulator provisioner. Requires internet access and root
# permission for apt; no PSP game files, BIOS or firmware are downloaded.
set -euo pipefail
if [[ "$(uname -s)" != Linux ]]; then echo 'Linux required' >&2; exit 2; fi
if ! getent ahostsv4 github.com >/dev/null; then
  echo 'Cannot resolve github.com: emulator installation requires network access' >&2
  exit 3
fi
if ! command -v sudo >/dev/null; then echo 'sudo required on Ubuntu runner' >&2; exit 4; fi
sudo apt-get update
sudo apt-get install -y --no-install-recommends flatpak curl python3 ca-certificates file
mkdir -p "$HOME/.local/bin"
tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT
curl -fLsS --retry 3 https://api.github.com/repos/melonDS-emu/melonDS/releases/latest -o "$tmp"
url="$(python3 - "$tmp" <<'PY'
import json,sys
with open(sys.argv[1],encoding='utf-8') as f: release=json.load(f)
assets=release.get('assets',[])
for a in assets:
    name=a.get('name','').lower()
    if name.endswith('.appimage') and any(t in name for t in ('x86_64','amd64','x64')):
        print(a['browser_download_url']);break
else:
    raise SystemExit('No Linux x86_64 AppImage in latest official melonDS release')
PY
)"
curl -fLsS --retry 3 "$url" -o "$HOME/.local/bin/melonDS.AppImage"
chmod +x "$HOME/.local/bin/melonDS.AppImage"
file "$HOME/.local/bin/melonDS.AppImage"
flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak --user install --noninteractive -y flathub org.azahar_emu.Azahar
flatpak --user info org.azahar_emu.Azahar
printf 'Installed melonDS AppImage and Azahar Flatpak for current Linux user.\n'
printf 'Graphics/display and emulator ROM startup have NOT been validated.\n'
