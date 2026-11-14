#!/usr/bin/env python3
"""Regression tests for PPSSPP MIPS object linker closure inventory."""
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "audit_ppsspp_link_closure", root / "scripts" / "audit-ppsspp-link-closure.py"
)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def test_closure():
    undefined = """Interpreter.o:
                 U MIPSState::Reset()
                 U Memory::Read_U32(unsigned int)
                 U std::terminate()
Dispatch.o:
                 U MIPSState::Reset()
                 U ExternalGPU()
"""
    all_symbols = """Interpreter.o:
0000000000000000 T MIPSState::Reset()
                 U Memory::Read_U32(unsigned int)
Dispatch.o:
0000000000000020 T InternalDispatch()
                 U ExternalGPU()
"""
    result = module.inventory(undefined, all_symbols)
    assert result["undefined_references"] == 4, result
    assert result["resolved_within_compiled_objects"] == 1, result
    assert result["remaining_external_symbols"] == 3, result
    assert result["remaining_external_sample"] == [
        "ExternalGPU()", "Memory::Read_U32(unsigned int)", "std::terminate()"
    ], result
    assert result["actual_link_succeeded"] is False


if __name__ == "__main__":
    test_closure()
    print("PPSSPP link closure regression passed")
