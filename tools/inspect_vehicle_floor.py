#!/usr/bin/env python3
"""Inspect the lower envelope of a Speed Dreams AC3D vehicle mesh.

The chassis ACC is authored in the renderer frame used by this port:
    acc_x = longitudinal (positive forward)
    acc_y = height above the chassis reference plane
    acc_z = lateral (sign resolved from the named left/right objects)

The script only reads vertices; it does not modify the model.
"""

import argparse
import json
import pathlib
import sys

import numpy


def read_objects(path):
    lines = pathlib.Path(path).read_text(encoding="utf-8", errors="replace").splitlines()
    objects = {}
    current = None
    index = 0
    while index < len(lines):
        line = lines[index].strip()
        if line.startswith("name "):
            current = line[5:].strip().strip('"')
        elif line.startswith("numvert "):
            count = int(line.split()[1])
            block = lines[index + 1:index + 1 + count]
            points = numpy.asarray([[float(value) for value in row.split()[:3]]
                                    for row in block], dtype=float)
            if current:
                objects.setdefault(current, []).append(points)
            index += count
        index += 1
    return {name: numpy.concatenate(blocks) for name, blocks in objects.items()}


def bounds(points):
    return points.min(axis=0), points.max(axis=0)


def profile(points, bin_size, threshold, x_range):
    records = []
    x_min, x_max = x_range
    for x_center in numpy.arange(x_min, x_max + 1e-9, bin_size):
        selected = points[numpy.abs(points[:, 0] - x_center) <= bin_size * 0.5]
        if len(selected) == 0:
            continue
        bin_min = selected[:, 1].min()
        near = selected[selected[:, 1] <= bin_min + threshold]
        records.append({
            "x": float(x_center),
            "samples": int(len(selected)),
            "min_y": float(bin_min),
            "z_min": float(near[:, 2].min()),
            "z_max": float(near[:, 2].max()),
            "mean_y": float(near[:, 1].mean()),
            "count": int(len(near)),
        })
    return records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mesh")
    parser.add_argument("--json", dest="json_path")
    parser.add_argument("--bin", type=float, default=0.05)
    parser.add_argument("--band", type=float, default=0.003)
    arguments = parser.parse_args()

    objects = read_objects(arguments.mesh)
    if not objects:
        print("no vertices")
        return 1

    all_points = numpy.concatenate(list(objects.values()))
    print(f"objects={len(objects)} vertices={len(all_points)}")

    report = {"mesh": str(arguments.mesh), "objects": {}, "profiles": {}}
    print("\nobjects (acc bounds):")
    print("name                                                     n      x_min   x_max   y_min   y_max   z_min   z_max")
    for name in sorted(objects):
        points = objects[name]
        low, high = bounds(points)
        report["objects"][name] = {
            "vertices": int(len(points)),
            "min": low.tolist(),
            "max": high.tolist(),
        }
        print("{:<56} {:6d} {:+.3f} {:+.3f} {:+.3f} {:+.3f} {:+.3f} {:+.3f}".format(
            name[:56], len(points), low[0], high[0], low[1], high[1], low[2], high[2]))

    # Resolve the lateral sign from the named sidepod panels.
    left_name = next((name for name in objects if "SIDEPOD" in name and "LEFT" in name), None)
    right_name = next((name for name in objects if "SIDEPOD" in name and "RIGHT" in name), None)
    lateral_sign = None
    if left_name and right_name:
        left_z = objects[left_name][:, 2].mean()
        right_z = objects[right_name][:, 2].mean()
        lateral_sign = 1.0 if left_z > right_z else -1.0
        print(f"\nlateral sign: acc_z positive is "
              f"{'left' if lateral_sign > 0 else 'right'} "
              f"(left panel z={left_z:+.4f}, right panel z={right_z:+.4f})")
    report["lateral_sign_acc_z_positive_is_left"] = lateral_sign

    # Floor candidates: main body + sidepods + engine/gearbox, excluding wings.
    floor_names = [name for name in objects
                   if "MAIN_BODY" in name or "SIDEPOD" in name or "ENGINE" in name]
    floor_points = numpy.concatenate([objects[name] for name in floor_names])
    floor_low, floor_high = bounds(floor_points)
    print("\nfloor candidate bounds: x=[{:+.3f},{:+.3f}] y=[{:+.3f},{:+.3f}] "
          "z=[{:+.3f},{:+.3f}]".format(floor_low[0], floor_high[0],
                                       floor_low[1], floor_high[1],
                                       floor_low[2], floor_high[2]))
    report["floor_candidates"] = {
        "names": floor_names,
        "min": floor_low.tolist(),
        "max": floor_high.tolist(),
    }

    for label, points, x_range in (
            ("floor", floor_points, (-2.30, 2.20)),
            ("front_wing", numpy.concatenate([objects[name] for name in objects
                                              if "FRONT_WING" in name]), (1.20, 2.20)),
            ("rear_structure", numpy.concatenate([objects[name] for name in objects
                                                  if "ENGINE" in name or "REAR_WING" in name]),
             (-2.30, -1.00))):
        records = profile(points, arguments.bin, arguments.band, x_range)
        report["profiles"][label] = records
        if not records:
            continue
        print(f"\n{label} lower envelope (band {arguments.band:.3f} m):")
        print("x       n     z_min   z_max   min_y   mean_y")
        for record in records:
            print("{:+.3f} {:5d}  {:+.3f}  {:+.3f}  {:+.4f}  {:+.4f}".format(
                record["x"], record["count"], record["z_min"], record["z_max"],
                record["min_y"], record["mean_y"]))

    # Plane fit on the lowest 10 mm of the floor candidates, excluding wings.
    band = floor_points[floor_points[:, 1] <= floor_points[:, 1].min() + 0.010]
    if len(band) >= 3:
        design = numpy.column_stack((numpy.ones(len(band)), band[:, 0], band[:, 2]))
        coefficients, _, _, _ = numpy.linalg.lstsq(design, band[:, 1], rcond=None)
        error = band[:, 1] - design @ coefficients
        report["floor_plane_fit"] = {
            "count": int(len(band)),
            "intercept": float(coefficients[0]),
            "slope_x": float(coefficients[1]),
            "slope_z": float(coefficients[2]),
            "rms": float(numpy.sqrt(numpy.mean(error * error))),
            "max_abs": float(numpy.max(numpy.abs(error))),
        }
        print("\nfloor plane fit y = a + b*x + c*z")
        print("n={} a={:.6f} b={:+.6f} c={:+.6f} rms={:.6f} max={:.6f}".format(
            len(band), coefficients[0], coefficients[1], coefficients[2],
            report["floor_plane_fit"]["rms"], report["floor_plane_fit"]["max_abs"]))

    if arguments.json_path:
        pathlib.Path(arguments.json_path).write_text(json.dumps(report, indent=2))
        print(f"\nwrote {arguments.json_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
