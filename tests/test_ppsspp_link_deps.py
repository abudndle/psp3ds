#!/usr/bin/env python3
"""Regression tests for PPSSPP linker dependency inventory."""
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "summarize_ppsspp_link_deps", root / "scripts" / "summarize-ppsspp-link-deps.py"
)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def test_summary():
    raw = """Interpreter.o:
                 U MIPSState::Reset()
                 U Memory::Read_U32(unsigned int)
                 U std::terminate()
InterpreterDispatch.o:
                 U MIPSState::Reset()
                 U sceKernelStartModule()
                 U Core_EnableStepping()
"""
    result = module.summarize(raw)
    assert result["unique_unresolved_symbols"] == 5, result
    assert result["objects"] == {"Interpreter.o": 3, "InterpreterDispatch.o": 3}
    assert result["categories_heuristic"]["MIPS execution/state"] == 1
    assert result["categories_heuristic"]["guest memory"] == 1
    assert result["categories_heuristic"]["PSP HLE/kernel"] == 1
    assert result["categories_heuristic"]["core runtime"] == 1
    assert result["categories_heuristic"]["C++ runtime / libc"] == 1
    assert result["linked_to_psp3ds"] is False


if __name__ == "__main__":
    test_summary()
    print("PPSSPP linker dependency summary regression passed")
