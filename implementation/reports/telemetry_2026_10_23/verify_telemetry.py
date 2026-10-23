"""ABI v7 verification for the Open Formula vertical-suspension telemetry.

Checks, against the real DLL and the real Melbourne ground mesh:
  1. the C layout the host compiles matches the Rust declarations (version + signature);
  2. every new per-corner field is published and physically coherent on a settled car;
  3. the per-axle solve block and the trace ring fill;
  4. an impossible ground demand is rejected with a concrete wheel/axle/demand/limit,
     the rejection counter advances and the published state stays frozen;
  5. the effective-setup report carries values, units, limits and derived parameters.

It never launches the game and never touches its runtime.log.
"""
from __future__ import annotations

import ctypes as C
import hashlib
import json
import math
import re
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CANDIDATE = ROOT / "rust/target/release/vehicle_physics_engine.dll"
SOURCE = ROOT / "rust/vehicle-physics-engine/src/open_formula_ffi.rs"
PROFILE = ROOT / "distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/vehicle.json"
MESH = ROOT / "distribution/share/games/speed-dreams-2/tracks/circuit/melbourne-nineteen-ninety-seven/melbourne_ground_collisions.txt"
OUT = Path(__file__).resolve().parent / "telemetry_validation"
TRACE_DIR = OUT / "trace"

CHECKS: list[dict] = []


def check(name: str, ok: bool, detail: str = "") -> bool:
    CHECKS.append({"name": name, "ok": bool(ok), "detail": detail})
    print(("PASS  " if ok else "FAIL  ") + name + (f"  [{detail}]" if detail else ""))
    return ok


# ---------------------------------------------------------------------------
# ctypes layout parsed from the authoritative Rust declarations
# ---------------------------------------------------------------------------

SCALARS = {"f64": C.c_double, "i32": C.c_int32, "u32": C.c_uint32, "u64": C.c_uint64}
COUNTS = {"OFP_WHEELS": 4, "OFP_FLOOR_PROBES": 2, "OFP_MAX_CONTACTS": 32, "OFP_MAX_BODY_CONTACTS": 32}
STRUCTS = [
    "OfpVec3",
    "OfpVehicleSpecs",
    "OfpControlInput",
    "OfpBodyContact",
    "OfpWheelState",
    "OfpWheelDiagnostics",
    "OfpAxleDiagnostics",
    "OfpFailureDiagnostics",
    "OfpSnapshot",
]


def build_types():
    text = SOURCE.read_text(encoding="utf-8")
    types: dict[str, type] = {}

    def resolve(kind: str):
        kind = kind.strip()
        if kind.startswith("["):
            inner, count = kind[1:-1].rsplit("; ", 1)
            base = resolve(inner)
            size = COUNTS[count] if count in COUNTS else int(count)
            return base * size
        if kind in SCALARS:
            return SCALARS[kind]
        if kind in types:
            return types[kind]
        raise KeyError(f"unresolved field type {kind}")

    for name in STRUCTS:
        body = re.search(r"pub struct " + name + r" \{(.*?)\n\}", text, re.S).group(1)
        fields = []
        for field, kind in re.findall(r"^    pub (\w+): ([^,\n]+),", body, re.M):
            fields.append((field, resolve(kind)))
        types[name] = type(name, (C.Structure,), {"_fields_": fields})
    return types


def expected_signature(types) -> int:
    """Mirror of `ofp_abi_signature`: FNV-1a over the sizes and offsets of the ABI."""
    text = SOURCE.read_text(encoding="utf-8")
    body = re.search(r'pub extern "C" fn ofp_abi_signature\(\) -> u64 \{(.*?)\n\}', text, re.S).group(1)
    hash_value = 0xCBF29CE484222325

    def mix(value: int) -> None:
        nonlocal hash_value
        hash_value ^= value & ((1 << 64) - 1)
        hash_value = (hash_value * 0x100000001B3) & ((1 << 64) - 1)

    pattern = re.compile(
        r"std::mem::size_of::<(\w+)>\(\)|std::mem::offset_of!\((\w+), (\w+)\)"
    )
    for struct_size, struct_offset, field in pattern.findall(body):
        if struct_size:
            mix(C.sizeof(types[struct_size]))
        else:
            mix(getattr(types[struct_offset], field).offset)
    return hash_value


def finite(value: float) -> bool:
    return isinstance(value, float) and math.isfinite(value)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    TRACE_DIR.mkdir(parents=True, exist_ok=True)
    types = build_types()
    V, I, S = types["OfpVec3"], types["OfpControlInput"], types["OfpSnapshot"]
    W = types["OfpWheelDiagnostics"]

    dll = C.CDLL(str(CANDIDATE.resolve()))
    dll.ofp_abi_version.restype = C.c_uint32
    dll.ofp_abi_signature.restype = C.c_uint64

    check("library ABI version is 7", dll.ofp_abi_version() == 7, f"reports {dll.ofp_abi_version()}")
    signature = expected_signature(types)
    check("host-computed layout signature matches the library",
          dll.ofp_abi_signature() == signature,
          f"host=0x{signature:016x} library=0x{dll.ofp_abi_signature():016x}")
    check("OfpSnapshot is 3480 bytes",
          C.sizeof(S) == 3480, f"{C.sizeof(S)} bytes")
    check("OfpWheelDiagnostics is 264 bytes and anchor_position at 184",
          C.sizeof(W) == 264 and W.anchor_position.offset == 184,
          f"{C.sizeof(W)} bytes, offset {W.anchor_position.offset}")

    dll.ofp_load_track_ground_mesh.argtypes = [C.c_char_p]
    dll.ofp_load_track_ground_mesh.restype = C.c_int
    triangles = dll.ofp_load_track_ground_mesh(str(MESH).encode())
    check("Melbourne ground mesh loads", triangles > 0, f"{triangles} triangles")

    dll.ofp_trace_configure.argtypes = [C.c_char_p]
    dll.ofp_trace_configure.restype = C.c_int
    check("trace ring configures into a directory",
          dll.ofp_trace_configure(str(TRACE_DIR).encode()) == 0)

    dll.ofp_create.argtypes = [C.c_char_p, V, C.c_double, C.c_double]
    dll.ofp_create.restype = C.c_void_p
    dll.ofp_destroy.argtypes = [C.c_void_p]
    dll.ofp_step.argtypes = [C.c_void_p, C.POINTER(I), C.c_void_p, C.c_uint32, C.c_double]
    dll.ofp_step.restype = C.c_int
    dll.ofp_snapshot.argtypes = [C.c_void_p, C.POINTER(S)]
    dll.ofp_snapshot.restype = C.c_int
    dll.ofp_last_error.argtypes = [C.c_void_p, C.c_char_p, C.c_uint32]
    dll.ofp_trace_dump.argtypes = [C.c_char_p]
    dll.ofp_trace_dump.restype = C.c_int
    dll.ofp_trace_buffered.restype = C.c_uint32
    dll.ofp_effective_setup.argtypes = [C.c_void_p, C.c_char_p, C.c_uint32]
    dll.ofp_effective_setup.restype = C.c_int

    origin = V(-1007.681, 1.871, -829.024)
    handle = dll.ofp_create(str(PROFILE).encode(), origin, -0.005, 0.0)
    if not check("vehicle handle created", bool(handle)):
        return report()
    try:
        control = I(clutch=1.0, gear_request=0)
        accepted = 0
        for _ in range(480):
            if dll.ofp_step(handle, C.byref(control), None, 0, 1.0 / 240.0) != 0:
                break
            accepted += 1
        check("240 accepted steps (two seconds of settling)", accepted >= 480,
              f"{accepted} accepted")

        snapshot = S()
        check("snapshot reads back", dll.ofp_snapshot(handle, C.byref(snapshot)) == 0)
        wheel = snapshot.wheel_diagnostics
        axle = snapshot.axle_diagnostics

        grounded = [w for w in wheel if w.ground_detected]
        check("every corner reports a ground plane on flat ground",
              len(grounded) == 4, f"{len(grounded)}/4 corners grounded")

        check("every corner reports finite compression and a finite gap",
              all(finite(w.compression_m) and finite(w.ground_separation_m) for w in wheel),
              ", ".join(f"{w.ground_separation_m:+.4f}" for w in wheel))
        check("grounded corners sit within a millimetre of the plane",
              all(abs(w.ground_separation_m) < 2.0e-3 for w in wheel),
              ", ".join(f"{w.ground_separation_m * 1e3:+.2f}mm" for w in wheel))
        check("compression limits bracket the current compression",
              all(w.min_compression_m <= w.compression_m <= w.max_compression_m for w in wheel),
              ", ".join(f"[{w.min_compression_m:.3f},{w.max_compression_m:.3f}]" for w in wheel))
        check("each corner reports spring, damper, anti-roll and stop forces",
              all(finite(w.spring_force_n) and finite(w.damper_force_n)
                  and finite(w.antiroll_force_n) and finite(w.stop_force_n) for w in wheel))
        check("the element sum is the signed sum of the four elements",
              all(abs(w.element_force_sum_n - (w.spring_force_n + w.damper_force_n
                                               + w.antiroll_force_n + w.stop_force_n)) < 1e-6
                  for w in wheel))
        check("each corner reports a positive motion ratio",
              all(0.2 < w.motion_ratio < 2.0 for w in wheel),
              ", ".join(f"{w.motion_ratio:.3f}" for w in wheel))
        check("each corner reports anchor position, velocity and acceleration",
              all(all(finite(v) for v in (w.anchor_position.x, w.anchor_position.y, w.anchor_position.z,
                                          w.anchor_velocity.x, w.anchor_velocity.y, w.anchor_velocity.z,
                                          w.anchor_acceleration.x, w.anchor_acceleration.y,
                                          w.anchor_acceleration.z))
                  for w in wheel))
        check("the static normal load is shared by the four corners",
              sum(w.contact_impulse_ns for w in wheel) > 0.0,
              f"total impulse {sum(w.contact_impulse_ns for w in wheel):.4f} N s")
        check("both axles report a converged solve",
              all(a.converged == 1 for a in axle),
              ", ".join(f"iters={a.iterations} resid={a.residual_m_s:.2e}" for a in axle))
        check("both axles report anti-roll stiffness and load",
              all(a.antiroll_stiffness_n_per_m > 0.0 and a.normal_load_n > 0.0 for a in axle),
              ", ".join(f"k_arb={a.antiroll_stiffness_n_per_m:.0f}N/m" for a in axle))
        check("chassis angular acceleration is published",
              all(finite(v) for v in (snapshot.angular_acceleration.x,
                                      snapshot.angular_acceleration.y,
                                      snapshot.angular_acceleration.z)))
        check("no failure is published while the car is settled",
              snapshot.failure.active == 0 and snapshot.failure.code == 0)
        check("the trace ring is filling",
              snapshot.trace_records > 480,
              f"{snapshot.trace_records} records buffered")

        dump_path = OUT / "settled_trace.csv"
        rows = dll.ofp_trace_dump(str(dump_path).encode())
        lines = dump_path.read_text(encoding="utf-8").splitlines() if dump_path.exists() else []
        header = lines[0].split(",") if lines else []
        check("the trace dump contains every record", rows == len(lines) - 1,
              f"{rows} rows, {len(lines) - 1} written")
        check("the trace has per-corner columns for all four wheels",
              all(f"{name}_{field}" in header
                  for name in ("FL", "FR", "RL", "RR")
                  for field in ("compression_m", "separation_m", "ground_demand_m",
                                "spring_force_n", "damper_force_n", "antiroll_force_n",
                                "contact_impulse_ns")),
              f"{len(header)} columns")
        kinds = {line.split(",")[0] for line in lines[1:]}
        check("the trace keeps accepted ticks and body/corner iterations",
              {"1", "2"}.issubset(kinds), f"kinds present: {sorted(kinds)}")

        buffer = C.create_string_buffer(16384)
        length = dll.ofp_effective_setup(handle, buffer, 16384)
        text = buffer.value.decode("utf-8", "replace")
        field_lines = [line for line in text.splitlines() if line.startswith("FIELD ")]
        derived_lines = [line for line in text.splitlines() if line.startswith("DERIVED ")]
        check("the effective setup report lists the editable fields",
              len(field_lines) > 20, f"{len(field_lines)} FIELD lines")
        check("each field line carries value, unit and admitted limits",
              all(all(token in line for token in ("value=", "unit=", "min=", "max="))
                  for line in field_lines))
        check("the derived block states the solver coefficients",
              any("rest_compression_m=" in line and "motion" not in line for line in derived_lines)
              and any("spring_rate_N_per_m=" in line for line in derived_lines)
              and any("arb_ratio=" in line for line in derived_lines)
              and any("effective_elements=" in line for line in derived_lines),
              f"{len(derived_lines)} DERIVED lines")
        (OUT / "effective_setup.txt").write_text(text, encoding="utf-8")
    finally:
        dll.ofp_destroy(handle)

    # ------------------------------------------------------------------
    # Rejection: a start pose whose ground demand exceeds the bump limit.
    # The wheel must still see the plane (a pose deep enough to put the ray
    # origin under the surface just falls), so the offsets are tried in order
    # of increasing intrusion and the first one that violates the limit is kept.
    # ------------------------------------------------------------------
    dll.ofp_trace_configure(str(TRACE_DIR).encode())
    control = I(clutch=1.0, gear_request=0)
    rejection = {}
    chosen_offset = None
    frozen = S()
    for offset in (-0.12, -0.16, -0.20, -0.08, -0.25):
        sunken = V(origin.x, origin.y + offset, origin.z)
        handle = dll.ofp_create(str(PROFILE).encode(), sunken, -0.005, 0.0)
        if not handle:
            continue
        try:
            code = dll.ofp_step(handle, C.byref(control), None, 0, 1.0 / 240.0)
            if code != 0:
                chosen_offset = offset
                dll.ofp_snapshot(handle, C.byref(frozen))
                break
        finally:
            dll.ofp_destroy(handle)

    check("an impossible ground demand is rejected",
          chosen_offset is not None,
          f"start offset {chosen_offset:+.2f} m" if chosen_offset is not None
          else "no tested offset produced a rejection")
    if chosen_offset is not None:
        failure = frozen.failure
        rejection = {
            "start_offset_m": chosen_offset,
            "active": failure.active,
            "code": failure.code,
            "wheel": failure.wheel,
            "axle": failure.axle,
            "iteration": failure.iteration,
            "subdivisions": failure.subdivisions,
            "demand_m": failure.demand_m,
            "limit_m": failure.limit_m,
            "excess_m": failure.excess_m,
            "limit_velocity_m_s": failure.limit_velocity_m_s,
            "residual_m_s": failure.residual_m_s,
            "rejected_steps": frozen.rejected_steps,
            "state_time_s": frozen.sim_time_s,
        }
        check("the rejection is flagged on the published state", failure.active == 1)
        check("the rejection names a concrete corner and axle",
              failure.wheel >= 0 and failure.axle in (0, 1),
              f"wheel={failure.wheel} axle={failure.axle}")
        check("the incompatible ground/bump limit is reported numerically",
              failure.code == 1 and failure.excess_m > 0.0 and failure.limit_m > 0.0,
              f"code={failure.code} demand={failure.demand_m:.4f} limit={failure.limit_m:.4f} "
              f"excess={failure.excess_m:.4f}")
        check("the rejection counter advances instead of staying at zero",
              frozen.rejected_steps == 1, f"rejected={frozen.rejected_steps}")
        check("the published state stays frozen at the last accepted step",
              frozen.sim_time_s == 0.0 and failure.active == 1,
              f"state_time={frozen.sim_time_s:.6f}")
        dll.ofp_trace_dump(str((OUT / "rejection_trace.csv")).encode())
        lines = (OUT / "rejection_trace.csv").read_text(encoding="utf-8").splitlines()
        kinds = {line.split(",")[0] for line in lines[1:]}
        check("the rejection dump keeps the rejected candidate row",
              "4" in kinds, f"kinds present: {sorted(kinds)}")
        rejected_rows = [line.split(",") for line in lines[1:] if line.startswith("4,")]
        if rejected_rows:
            row = rejected_rows[0]
            check("the rejected candidate row names the axle and the wheel",
                  row[1] in {"0", "1"} and row[2] not in {"-1"}, f"axle={row[1]} wheel={row[2]}")

    # ------------------------------------------------------------------
    # Contact loss: the car starts in the air, so every corner loses ground for
    # longer than the 0.25 s threshold and the ring must write its own window
    # (3 s before, 1 s after) without any host involvement.
    # ------------------------------------------------------------------
    for stale in TRACE_DIR.glob("rustphysics_contact_loss_*"):
        stale.unlink()
    dll.ofp_trace_configure(str(TRACE_DIR).encode())
    airborne = V(origin.x, origin.y + 0.5, origin.z)
    handle = dll.ofp_create(str(PROFILE).encode(), airborne, -0.005, 0.0)
    if check("airborne handle created", bool(handle)):
        try:
            for _ in range(600):
                dll.ofp_step(handle, C.byref(control), None, 0, 1.0 / 240.0)
        finally:
            dll.ofp_destroy(handle)
    dll.ofp_trace_flush()
    windows = sorted(TRACE_DIR.glob("rustphysics_contact_loss_*.json"))
    check("a sustained loss of contact writes its own window",
          bool(windows), f"{len(windows)} window(s) in {TRACE_DIR.name}/")
    if windows:
        metadata = json.loads(windows[0].read_text(encoding="utf-8"))
        check("automatic CSV is written by dedicated background thread",
              metadata.get("writer_thread") == "ofp-telemetry-writer" and
              isinstance(metadata.get("capture_ms"), (int, float)) and
              isinstance(metadata.get("format_write_ms"), (int, float)))
        rows = metadata.get("rows", 0)
        window_from = metadata.get("window_from_s")
        trigger = metadata.get("trigger_time_s")
        check("the contact-loss window carries records from before the event",
              rows > 0 and window_from is not None and trigger is not None
              and abs((trigger - window_from) - 3.0) < 1e-6,
              f"rows={rows} from {window_from:.3f} s to trigger {trigger:.3f} s")
        csv_lines = Path(metadata["csv"]).read_text(encoding="utf-8").splitlines()
        header_columns = csv_lines[0].split(",") if csv_lines else []
        flags_column = header_columns.index("FL_flags") if "FL_flags" in header_columns else -1
        airborne_rows = [
            line for line in csv_lines[1:]
            if flags_column >= 0 and (int(line.split(",")[flags_column]) & 1) == 0
        ]
        check("the contact-loss window records corners with no ground contact",
              bool(airborne_rows), f"{len(airborne_rows)} airborne corner rows")

    # Independent semantic regressions: loads, gap sign, actual substep timing and restored state.
    import csv
    settled = list(csv.DictReader((OUT / "settled_trace.csv").open(encoding="utf-8")))
    ticks = [r for r in settled if r["kind"] == "1"]
    last = ticks[-1]
    check("CSV normal force is contact impulse over interval, not element sum",
          all(abs(float(last[f"{w}_normal_force_n"]) - float(last[f"{w}_contact_impulse_ns"])/float(last[f"{w}_contact_dt_s"])) < .02
              for w in ("FL", "FR", "RL", "RR")))
    check("CSV separately persists element sum and ray geometry",
          all(f"{w}_{field}" in last for w in ("FL", "FR", "RL", "RR")
              for field in ("element_force_sum_n", "contact_x", "normal_y", "ray_hit_mask", "ground_demand_predicted_m", "anchor_ax")))
    check("trace contains actual axle intervals", any(r["kind"] == "3" for r in settled))
    rejection_rows = list(csv.DictReader((OUT / "rejection_trace.csv").open(encoding="utf-8")))
    check("rejection includes restored accepted state", any(r["kind"] == "5" for r in rejection_rows))
    h = dll.ofp_create(str(PROFILE).encode(), V(origin.x, origin.y + .04, origin.z), -.005, 0.0)
    try:
        ok = dll.ofp_step(h, C.byref(I(clutch=1.0, gear_request=0)), None, 0, .01) == 0
        snap = S(); dll.ofp_snapshot(h, C.byref(snap))
        airborne = [w for w in snap.wheel_diagnostics if w.ground_detected and not w.ground_binding]
        for _ in range(40):
            if airborne or not ok: break
            ok = dll.ofp_step(h, C.byref(I(clutch=1.0, gear_request=0)), None, 0, .01) == 0
            dll.ofp_snapshot(h, C.byref(snap))
            airborne = [w for w in snap.wheel_diagnostics if w.ground_detected and not w.ground_binding]
        check("airborne wheel with visible ground reports positive separation", ok and bool(airborne) and
              all(w.ground_separation_m > 0 and abs(w.ground_separation_m -
                  (w.compression_m - w.ground_demand_predicted_m)) < 1e-8 for w in airborne))
        path = OUT / "interval_trace.csv"; dll.ofp_trace_dump(str(path).encode())
        interval_rows = list(csv.DictReader(path.open(encoding="utf-8")))
        substeps = [r for r in interval_rows if r["kind"] == "3" and int(r["sequence"]) == 0]
        # The latest host step is 10 ms, split into three intervals; each reports its own endpoint.
        recent = substeps[-6:]
        check("host subdivision uses real dt rather than host dt", bool(recent) and
              all(abs(float(r["step_dt_s"]) - .01/3) < 1e-6 and
                  abs(float(r["FL_contact_dt_s"]) - .01/3) < 1e-8 for r in recent))
    finally:
        dll.ofp_destroy(h)

    dll.ofp_trace_flush()
    dll.ofp_trace_shutdown()
    summary = {
        "candidate_dll": str(CANDIDATE),
        "candidate_sha256": hashlib.sha256(CANDIDATE.read_bytes()).hexdigest(),
        "abi_version": dll.ofp_abi_version(),
        "abi_signature": f"0x{signature:016x}",
        "snapshot_size": C.sizeof(S),
        "wheel_diagnostics_size": C.sizeof(W),
        "checks": CHECKS,
        "passed": sum(1 for c in CHECKS if c["ok"]),
        "failed": sum(1 for c in CHECKS if not c["ok"]),
        "rejection": rejection,
        "trace_directory": str(TRACE_DIR),
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
    }
    (OUT / "telemetry_validation.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(f"\n{summary['passed']} passed, {summary['failed']} failed")
    return 0 if summary["failed"] == 0 else 1


def report() -> int:
    print("could not create a vehicle handle")
    return 1


if __name__ == "__main__":
    sys.exit(main())
