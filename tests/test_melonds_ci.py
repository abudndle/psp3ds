#!/usr/bin/env python3
"""Check melonDS CI uses an actual built .nds and preserves failure evidence."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
wf=(root/'.github/workflows/melonds-nds-smoke.yml').read_text()
capture=root/'scripts/capture_melonds_ci.sh'
subprocess.run(['bash','-n',str(capture)],check=True)
for required in (
 'skylyrac/blocksds:slim-latest',
 'bash scripts/build_nds.sh',
 'actions/download-artifact@v4',
 'melonDS-emu/melonDS/releases/latest',
 'xvfb-run',
 'bash scripts/capture_melonds_ci.sh melonds-test/psp3ds_nds.nds',
 'melonds-evidence/melonds-window.png',
 'if: always()',
 'melonds-nds-window-or-diagnostics',
):
 assert required in wf,required
source=capture.read_text()
for required in ('kill -0','xdotool search --onlyvisible','xwd -silent','Screenshot appears blank'):
 assert required in source,required
print('PASS: melonDS workflow must use a native build and actual visible window capture')
