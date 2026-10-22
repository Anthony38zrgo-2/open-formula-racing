"""Etapa 3/5 (offline): la DLL real vehicle_physics_engine.dll por ctypes.

Verifica el artefacto que instala CMake: 22 simbolos ofp_* exportados,
version 6, firma de layout estable entre llamadas y procedencia con ambas
revisiones (donante + destino). Sale distinto de cero ante cualquier desvio.
"""

import ctypes
import pathlib
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
DLL = REPO / "rust" / "target" / "release" / "vehicle_physics_engine.dll"

EXPECTED_SYMBOLS = [
    "ofp_abi_signature", "ofp_abi_version", "ofp_apply_setup", "ofp_backend_mask",
    "ofp_build_provenance", "ofp_create", "ofp_destroy", "ofp_fuel_density",
    "ofp_last_error", "ofp_load_track_ground_mesh", "ofp_pit_fuel", "ofp_pit_tires",
    "ofp_reset", "ofp_reset_setup", "ofp_set_pose", "ofp_setup_document",
    "ofp_setup_fields", "ofp_snapshot", "ofp_step", "ofp_track_ground_triangles",
    "ofp_vehicle_geometry", "ofp_vehicle_specs",
]

failures = []


def check(name, condition, detail=""):
    print(("OK   " if condition else "FAIL ") + name + (" " + str(detail) if detail and not condition else ""))
    if not condition:
        failures.append(name)


def main():
    dll = ctypes.CDLL(str(DLL))
    for symbol in EXPECTED_SYMBOLS:
        try:
            getattr(dll, symbol)
            present = True
        except AttributeError:
            present = False
        check(f"export {symbol}", present)
    dll.ofp_abi_version.restype = ctypes.c_uint32
    dll.ofp_abi_signature.restype = ctypes.c_uint64
    dll.ofp_build_provenance.restype = ctypes.c_char_p
    check("abi_version == 6", dll.ofp_abi_version() == 6, dll.ofp_abi_version())
    first = dll.ofp_abi_signature()
    check("signature stable", first == dll.ofp_abi_signature(), hex(first))
    check("signature nonzero", first != 0)
    provenance = dll.ofp_build_provenance().decode(errors="replace")
    check("provenance abi=6", "abi=6" in provenance, provenance[:120])
    check("provenance cites donor + dest", "f3efcf2" in provenance and "9c91648" in provenance)
    print("PHYSICS_DLL_OK" if not failures else "FAILURES: %d" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
