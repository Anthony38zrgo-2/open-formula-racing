"""Verify the candidate ABI and real static support through the host-facing DLL."""
from pathlib import Path
import ctypes as C
import hashlib
import importlib.util
import json
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("telemetry_layout", ROOT / "implementation/reports/telemetry_2026_10_23/verify_telemetry.py")
layout = importlib.util.module_from_spec(spec)
spec.loader.exec_module(layout)
types = layout.build_types()
V, I, S = (types[n] for n in ("OfpVec3", "OfpControlInput", "OfpSnapshot"))
candidate = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "rust/target/release/vehicle_physics_engine.dll"
profile = Path(sys.argv[2]) if len(sys.argv)>2 else ROOT / "rust/data/vehicles/williams-fw19-hockenheim/vehicle.json"
result_name = sys.argv[3] if len(sys.argv)>3 else "dll_validation.json"
dll = C.CDLL(str(candidate))
dll.ofp_abi_signature.restype = C.c_uint64
assert dll.ofp_abi_version() == 7
assert dll.ofp_abi_signature() == layout.expected_signature(types)
plane = OUT / "flat_ground.txt"
plane.write_text("FORMULA_GROUND_1 2\n1.0 0.001 -1000 -1000 0 1000 -1000 0 1000 1000 0\n1.0 0.001 -1000 -1000 0 1000 1000 0 -1000 1000 0\n")
dll.ofp_load_track_ground_mesh.argtypes = [C.c_char_p]
assert dll.ofp_load_track_ground_mesh(str(plane).encode()) == 2
dll.ofp_create.argtypes = [C.c_char_p, V, C.c_double, C.c_double]
dll.ofp_create.restype = C.c_void_p
dll.ofp_step.argtypes = [C.c_void_p, C.POINTER(I), C.c_void_p, C.c_uint32, C.c_double]
dll.ofp_snapshot.argtypes = [C.c_void_p, C.POINTER(S)]
dll.ofp_destroy.argtypes = [C.c_void_p]
dll.ofp_effective_setup.argtypes = [C.c_void_p, C.c_char_p, C.c_uint32]
dll.ofp_last_error.argtypes = [C.c_void_p, C.c_char_p, C.c_uint32]
handle = dll.ofp_create(str(profile).encode(), V(0., 0.538650, 0.), 0., 7.6)
assert handle
try:
    ctrl = I(clutch=1., gear_request=0)
    for _ in range(2000):
        code = dll.ofp_step(handle, C.byref(ctrl), None, 0, .002)
        if code:
            error = C.create_string_buffer(4096)
            dll.ofp_last_error(handle, error, len(error))
            raise AssertionError(error.value)
    s = S()
    assert dll.ofp_snapshot(handle, C.byref(s)) == 0
    # Existing physical floor datum. These are NOT modified to obtain the target.
    ride = [s.position.y + s.basis_y.y * (-.538650 + h) + s.basis_z.y * z
            for h, z in [(.030, -1.146992), (.045, 1.743008)]]
    assert abs(ride[0] - .035) < .0003, ride
    assert abs(ride[1] - .040) < .0003, ride
    assert all(w.normal_force_n > 1000 for w in s.wheels)
    assert all(abs(w.stop_force_n) < .1 for w in s.wheel_diagnostics)
    assert all(abs(j) < 1e-9 for j in s.floor_impulse_ns)
    assert s.failure.active == 0 and s.rejected_steps == 0
    assert all(abs(w.ground_separation_m) < .0005 for w in s.wheel_diagnostics)
    # A normal physics session must not collect trace records without opt-in.
    dll.ofp_trace_buffered.restype = C.c_uint32
    assert dll.ofp_trace_buffered() == 0
    buffer = C.create_string_buffer(32768)
    count = dll.ofp_effective_setup(handle, buffer, len(buffer))
    assert count > 0
    (OUT / "effective_setup.txt").write_text(buffer.value.decode(), encoding="utf-8")
    result = {"passed": True, "candidate": str(candidate), "sha256": hashlib.sha256(candidate.read_bytes()).hexdigest(),
              "abi_version": 7, "abi_signature": hex(dll.ofp_abi_signature()), "accepted_steps": 2000,
              "trace_records_without_opt_in": 0,
              "ride_height_m": ride, "normal_loads_n": [w.normal_force_n for w in s.wheels],
              "stop_forces_n": [w.stop_force_n for w in s.wheel_diagnostics], "floor_impulse_ns": list(s.floor_impulse_ns),
              "source_profile_sha256": hashlib.sha256(profile.with_name("suspension.json").read_bytes()).hexdigest()}
    (OUT / result_name).write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
finally:
    dll.ofp_destroy(handle)
