"""Aggregate acceptance evidence before installation; fail closed."""
from pathlib import Path
import csv
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
text = (OUT / "tests_final.log").read_text(encoding="utf-8")
results = re.findall(r"test result: ok\. (\d+) passed; (\d+) failed", text)
assert len(results) == 5 and sum(int(p) for p, _ in results) == 51, results
assert all(int(f) == 0 for _, f in results)
assert "test result: FAILED" not in text
dll = json.loads((OUT / "dll_validation.json").read_text(encoding="utf-8"))
assert dll["passed"]
assert hashlib.sha256((ROOT / "rust/target/release/vehicle_physics_engine.dll").read_bytes()).hexdigest() == dll["sha256"]
speeds = list(csv.DictReader((OUT / "speed_sweep.csv").open()))
assert len(speeds) == 5
assert all(float(r["stop_fraction"]) == 0 and float(r["min_floor_m"]) > .015 and float(r["min_normal_n"]) > 1000 for r in speeds)
assert abs(float(speeds[0]["ride_front_m"]) - .035) < .0003
assert abs(float(speeds[0]["ride_rear_m"]) - .040) < .0003
maneuvers = list(csv.DictReader((OUT / "maneuver_summary.csv").open()))
candidate = [r for r in maneuvers if r["profile"] == "candidate"]
assert len(candidate) == 4
assert all(r["failed"] == "false" and float(r["stop_wheel_fraction"]) == 0 and float(r["min_floor_m"]) > .015 for r in candidate)
for r in candidate:
    if r["case"] != "one_wheel_bump":
        assert float(r["front_unsupported_wheel_seconds"]) == 0
        assert float(r["rear_unsupported_wheel_seconds"]) == 0
trail = list(csv.DictReader((OUT / "candidate_trail_brake_reversal.csv").open()))
assert max(abs(float(r["sideslip_rad"])) for r in trail) < .20
result = {"passed": True, "tests_passed": 51, "test_suites": 5, "dll": dll,
          "speed_sweep": speeds, "maneuvers": maneuvers,
          "trail_max_sideslip_rad": max(abs(float(r["sideslip_rad"])) for r in trail),
          "manual_driving_acceptance": "pending", "candidate_v1": "superseded after trail-brake reversal instability; preserved in candidate_v1/"}
(OUT / "validation.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
print("PASS: 51 tests, real DLL 35/40-mm support, speed sweep and synthetic maneuvers")
