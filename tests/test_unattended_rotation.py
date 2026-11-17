#!/usr/bin/env python3
"""Static safety and rotation checks for the opt-in Codex GitHub Action."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
workflow = (root / ".github/workflows/psp3ds-unattended.yml").read_text()
prompt = (root / ".github/codex/psp3ds-unattended.md").read_text()

assert "workflow_dispatch:" in workflow
assert "schedule:" in workflow
assert "vars.PSP3DS_UNATTENDED_ENABLED == 'true'" in workflow
assert "matrix:" in workflow and "lane: [memory, interpreter, platform]" in workflow
assert "openai/codex-action@v1" in workflow
assert "openai-api-key: ${{ secrets.OPENAI_API_KEY }}" in workflow
assert "persist-credentials: false" in workflow
assert "permission-profile: ':workspace'" in workflow
assert "pull-requests: read" in workflow
assert "pull-requests: write" in workflow
assert "needs: codex" in workflow
assert "needs: [codex, verify]" in workflow
assert "ctest --test-dir build/unattended --output-on-failure" in workflow
assert "name: psp3ds-verified-${{ matrix.lane }}" in workflow
assert "if: always() && vars.PSP3DS_UNATTENDED_ENABLED == 'true'" in workflow
assert "peter-evans/create-pull-request@v7" in workflow
assert "psp3ds-bot/${{ matrix.lane }}" in workflow
assert "gh pr list --state open" in workflow
assert "git apply --check candidate/proposed.patch" in workflow
assert "git diff --check" in workflow
assert "git add -A -- source include tests docs TODO.md" in workflow
assert "Do not create proprietary" in prompt
assert "Do not edit .github/" in prompt
print("PASS: unattended rotation is opt-in, bounded, parallel and PR-gated")
