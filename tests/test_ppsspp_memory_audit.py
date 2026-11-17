#!/usr/bin/env python3
"""Host regression for upstream PPSSPP memory pointer inventory."""
import json
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/audit-ppsspp-memory-access.py"

with tempfile.TemporaryDirectory() as folder:
    directory = pathlib.Path(folder)
    header = directory / "MemMap.h"
    output = directory / "audit.json"
    header.write_text(
        "inline u32 ReadUnchecked_U32(u32 address) {\n"
        " return *(u32 *)(base + (address & 0x3fffffff));\n"
        "}\n"
        "u8 *GetPointerUnchecked(u32 address);\n"
        "void MemcpyUnchecked(void *p, u32 a, u32 n);\n"
    )
    additional = directory / "Interpreter.cpp"
    additional.write_text("auto x = ReadUnchecked_U8(addr);\n"
                          "auto p = Memory::base + addr;\n"
                          "auto q = GetPointer(addr);\n")
    subprocess.run([sys.executable, str(SCRIPT), str(header),
                    "--additional", str(additional),
                    "--output", str(output)], check=True)
    report = json.loads(output.read_text())
    assert report["count"] >= 7, report
    assert report["by_kind"]["direct_memory_base"] >= 1
    assert report["by_kind"]["pointer_cast"] >= 1
    assert any(item["source"] == str(additional) for item in report["matches"])
    assert {item["kind"] for item in report["matches"]} >= {
        "base_pointer_arithmetic", "unchecked_pointer_api",
        "unchecked_scalar_api", "unchecked_copy_api"}
    header.write_text("int unrelated();\n")
    failed = subprocess.run([sys.executable, str(SCRIPT), str(header),
                             "--output", str(output)], capture_output=True)
    assert failed.returncode != 0
print("PASS: PPSSPP memory access audit detects direct accesses and empty inventories")
