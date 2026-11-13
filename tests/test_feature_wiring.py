#!/usr/bin/env python3
"""Structural CI safeguard: native target entrypoints must use shared features.

This supplements native compilation and host CTests; it does not itself
execute hardware, verify a PSP GPU, or establish retail-game compatibility.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
header=(root/"include/psp_features.h").read_text()
shared=(root/"source/psp_features.c").read_text()
ds=(root/"platforms/nds/source/main.c").read_text()
old_new_3ds=(root/"source/main.c").read_text()
wrapper=(root/"platforms/nds/source/shared_features.c").read_text()
cmake=(root/"CMakeLists.txt").read_text()
for token in (
    "PSP_FEATURE_CPU_SMOKE",
    "PSP_FEATURE_MIPS_BENCHMARK",
    "PSP_FEATURE_MEMORY_POLICY",
    "PSP_FEATURE_GUEST_DATA_BUS",
    "PSP_FEATURE_GRAPHICS_TRANSLATION",
    "PSP_FEATURE_PSP_GAME_BOOT",
    "PSP_FEATURE_COUNT",
):
    assert token in header and token in shared, token
for code,platform in ((ds,"DS/DSi"),(old_new_3ds,"old/New 3DS")):
    for token in (
        '#include "psp_features.h"',
        "psp_features_manifest(",
        "psp_feature_run(PSP_FEATURE_CPU_SMOKE",
        "psp_feature_run(PSP_FEATURE_MIPS_BENCHMARK",
        "PSP_FEATURE_PSP_GAME_BOOT",
    ):
        assert token in code, (platform,token)
assert "../../../source/psp_features.c" in wrapper
assert "source/psp_features.c" in cmake
assert "NAME feature_parity" in cmake
print("PASS: common feature entrypoints and parity gates wired on DS, DSi, original/New 3DS")
