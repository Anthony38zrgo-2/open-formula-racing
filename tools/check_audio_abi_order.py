#!/usr/bin/env python3
"""Pin the C++ <-> Rust ABI field order for open_formula_audio (v1).

Compares, in order, the fields of the four ABI structs between
`src/interfaces/ofp_audio_abi.h` and `rust/open-formula-audio/src/lib.rs`.
Sizes are locked separately (Rust layout tests + C++ static_assert); this
locks the ORDER, which equal sizes alone cannot prove. Exit nonzero with a
diff on any mismatch.

Usage: python tools/check_audio_abi_order.py
"""

import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent

STRUCTS = [
    "OfpAudioCreateParamsV1",
    "OfpAudioPacketV1",
    "OfpAudioDiagnosticsV1",
    "OfpAudioStepV1",
]

PHYSICS_STRUCTS = [
    "OfpVec3",
    "OfpVehicleSpecs",
    "OfpControlInput",
    "OfpBodyContact",
    "OfpWheelState",
    "OfpSnapshot",
]


def cpp_fields(text, name):
    start = text.index("struct " + name)
    body = text[start:text.index("};", start)]
    names = []
    for line in body.splitlines():
        code = line.split("//")[0].strip().rstrip(";")
        if not code or code == "struct " + name or "(" in code:
            continue
        match = re.search(r"(\w+)(\[\w+\])?$", code)
        if match:
            names.append(match.group(1))
    return names


def rust_fields(text, name):
    start = text.index("pub struct " + name)
    body = text[start:text.index("\n}", start)]
    names = []
    for line in body.splitlines():
        code = line.split("//")[0].strip().rstrip(",")
        if not code or code == "pub struct " + name + " {" or code.startswith("#"):
            continue
        match = re.match(r"pub (\w+)\s*:", code)
        if match:
            names.append(match.group(1))
    return names


def main():
    failures = 0
    failures += check_pair(
        REPO_ROOT / "src" / "interfaces" / "ofp_audio_abi.h",
        REPO_ROOT / "rust" / "open-formula-audio" / "src" / "lib.rs",
        STRUCTS,
    )
    failures += check_pair(
        REPO_ROOT / "src" / "modules" / "simu" / "rustphysics" / "rust_abi.h",
        REPO_ROOT / "rust" / "vehicle-physics-engine" / "src" / "open_formula_ffi.rs",
        PHYSICS_STRUCTS,
    )
    if failures:
        return 1
    print("ABI_ORDER_OK")
    return 0


def check_pair(cpp_path, rust_path, names):
    cpp_text = cpp_path.read_text(encoding="utf-8")
    rust_text = rust_path.read_text(encoding="utf-8")
    failures = 0
    for name in names:
        cpp = cpp_fields(cpp_text, name)
        rust = rust_fields(rust_text, name)
        mismatch = [(i, a, b) for i, (a, b) in enumerate(zip(cpp, rust)) if a != b]
        if len(cpp) != len(rust) or mismatch:
            failures += 1
            print(f"{name}: MISMATCH (c++ {len(cpp)} vs rust {len(rust)})")
            for i, a, b in mismatch[:10]:
                print(f"  [{i}] c++={a} rust={b}")
        else:
            print(f"{name}: OK ({len(cpp)} fields, order identical)")
    return failures


if __name__ == "__main__":
    sys.exit(main())
