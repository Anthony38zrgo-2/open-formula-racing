#!/usr/bin/env python3
"""Apply the measured Williams 35/40-mm candidate to its suspension domain only.

The companion document records calibrated element coordinates and physical ride
references. No aero floor offsets, geometry hardpoints, user DB or controls change.
"""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", type=Path, default=ROOT / "rust/data/vehicles/williams-fw19-hockenheim")
    parser.add_argument("--parameters", type=Path, default=Path(__file__).with_name("williams_suspension_setup_35_40.json"))
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    parameters = json.loads(args.parameters.read_text(encoding="utf-8"))
    path = args.profile / "suspension.json"
    before = json.loads(path.read_text(encoding="utf-8"))
    candidate = json.loads(json.dumps(before))
    suspension = candidate["suspension"]
    for name in ("front", "rear"):
        axle = parameters[name]
        suspension[name]["arb_ratio"] = axle["arb_ratio"]
        suspension[name]["bump_stop_multiplier"] = axle["bump_stop_multiplier"]
        suspension["geometry_physical"][name].update(axle["elements"])
        bar = suspension["geometry_physical"][name + "_arb"]
        if bar["mode"] != "legacy_ratio":
            raise ValueError("this candidate requires the declared LegacyRatio bar")
        bar["ratio"] = axle["arb_ratio"]
    if args.check:
        if before != candidate:
            raise SystemExit("suspension differs from the calibrated 35/40-mm candidate")
        print("WILLIAMS_SUSPENSION_35_40_OK")
    else:
        path.write_text(json.dumps(candidate, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(path)


if __name__ == "__main__":
    main()
