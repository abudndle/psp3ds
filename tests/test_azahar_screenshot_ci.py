#!/usr/bin/env python3
"""Validate that the Azahar screenshot workflow requires a real native ROM and window."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
workflow=(root/'.github/workflows/azahar-screenshots.yml').read_text()
script=root/'scripts/capture_azahar_ci.sh'
subprocess.run(['bash','-n',str(script)],check=True)
checks=(
    'devkitpro/devkitarm:',
    'make -j2',
    'test -s psp3ds.3dsx',
    'actions/download-artifact@v4',
    'azahar-emu/azahar/releases/latest',
    'xvfb-run',
    'scripts/capture_azahar_ci.sh',
    'psp3ds-azahar.png',
    'if: always()',
    'pull_request:',
    'cp azahar-test/psp3ds.3dsx screenshots/psp3ds.3dsx',
    'scripts/verify_capture_artifact.py',
    'screenshots/evidence.json',
    'screenshots/psp3ds.3dsx',
    'psp3ds-azahar-real-screenshot-or-diagnostics',
)
for check in checks:
    assert check in workflow, 'Missing Azahar workflow guard: '+check
assert '              screenshots/psp3ds-azahar.png\n            screenshots/evidence.json' not in workflow, 'Unintended evidence.json shell invocation after capture'
assert 'python3 scripts/verify_capture_artifact.py' in workflow, 'Downloaded artifact integrity verification missing'
source=script.read_text()
for check in ('kill -0', 'xdotool search --onlyvisible', 'xwd -silent', 'Screenshot appears blank'):
    assert check in source, 'Missing screenshot authenticity check: '+check
print('PASS: Azahar CI includes native build, real emulator window and diagnostic artifact')
