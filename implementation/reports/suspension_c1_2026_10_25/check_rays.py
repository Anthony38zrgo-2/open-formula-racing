"""Read the deployed DLL's contact-ray counters for the shipped profile.

Creates a car on a flat plane, steps twice and prints the wheel ray queries/hits the
snapshot publishes. Four queries per step means the single-ray mode is really active
in the installed binary; twelve means the tricast is still in use.
"""
from pathlib import Path
import ctypes as C
import importlib.util
import sys

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location(
    "telemetry_layout", ROOT / "implementation/reports/telemetry_2026_10_23/verify_telemetry.py"
)
layout = importlib.util.module_from_spec(spec)
spec.loader.exec_module(layout)
types = layout.build_types()
V, I, S = (types[n] for n in ("OfpVec3", "OfpControlInput", "OfpSnapshot"))

dll_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(
    "distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll"
)
profile = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(
    "distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/vehicle.json"
)
dll = C.CDLL(str(dll_path))
plane = HERE / "flat_ground.txt"
plane.write_text("FORMULA_GROUND_1 2\n1.0 0.001 -1000 -1000 0 1000 -1000 0 1000 1000 0\n1.0 0.001 -1000 -1000 0 1000 1000 0 -1000 1000 0\n")
dll.ofp_load_track_ground_mesh.argtypes = [C.c_char_p]
assert dll.ofp_load_track_ground_mesh(str(plane).encode()) == 2
dll.ofp_create.argtypes = [C.c_char_p, V, C.c_double, C.c_double]
dll.ofp_create.restype = C.c_void_p
dll.ofp_step.argtypes = [C.c_void_p, C.POINTER(I), C.c_void_p, C.c_uint32, C.c_double]
dll.ofp_snapshot.argtypes = [C.c_void_p, C.POINTER(S)]
dll.ofp_destroy.argtypes = [C.c_void_p]
handle = dll.ofp_create(str(profile).encode(), V(0.0, 0.538650, 0.0), 0.0, 7.6)
assert handle
try:
    control = I(clutch=1.0, gear_request=0)
    for _ in range(400):
        assert dll.ofp_step(handle, C.byref(control), None, 0, 0.002) == 0
    snapshot = S()
    assert dll.ofp_snapshot(handle, C.byref(snapshot)) == 0
    print(f"dll={dll_path}")
    print(f"profile={profile}")
    print(f"ride_m=[{snapshot.position.y:.9f}] loads_n={[round(w.normal_force_n, 3) for w in snapshot.wheels]}")
    print(f"wheel_ray_queries={snapshot.wheel_ray_queries} wheel_ray_hits={snapshot.wheel_ray_hits}")
    expected = 4
    assert snapshot.wheel_ray_queries == expected, (
        f"expected {expected} queries/step (single ray), got {snapshot.wheel_ray_queries}"
    )
    print("single-ray mode confirmed in the deployed binary")
finally:
    dll.ofp_destroy(handle)
