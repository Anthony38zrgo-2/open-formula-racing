#!/usr/bin/env python3
"""Validate the proposed floor contact points against the installed ACC mesh.

Checks:
- ACC bounds against the vehicle XML body dimensions;
- every proposed point lies on the named source object's lower surface;
- the ACC-to-solver mapping and the nominal gap;
- left/right symmetry of the paired points;
- tier counts and the nominal minimum height.
"""

import argparse
import json
import pathlib
import sys
import xml.etree.ElementTree as element_tree

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
            vertices = numpy.asarray([[float(value) for value in row.split()[:3]]
                                      for row in lines[index + 1:index + 1 + count]], dtype=float)
            cursor = index + 1 + count
            while cursor < len(lines) and not lines[cursor].strip().startswith("numsurf "):
                cursor += 1
            surface_count = int(lines[cursor].strip().split()[1]) if cursor < len(lines) else 0
            triangles = []
            cursor += 1
            while cursor < len(lines) and len(triangles) < surface_count:
                if lines[cursor].strip().startswith("SURF"):
                    cursor += 1
                    while cursor < len(lines) and not lines[cursor].strip().startswith("refs "):
                        cursor += 1
                    cursor += 1
                    triangle = []
                    for _ in range(3):
                        triangle.append(int(lines[cursor].strip().split()[0]))
                        cursor += 1
                    triangles.append(triangle)
                else:
                    cursor += 1
            if current:
                objects.setdefault(current, []).append(
                    (vertices, numpy.asarray(triangles, dtype=int)))
            index = cursor
        index += 1
    return objects


def merge(objects, name):
    blocks = objects[name]
    vertices = []
    triangles = []
    offset = 0
    for block_vertices, block_triangles in blocks:
        vertices.append(block_vertices)
        triangles.append(block_triangles + offset)
        offset += len(block_vertices)
    return numpy.vstack(vertices), numpy.vstack(triangles)


def surface_at(objects, name, x, z):
    vertices, triangles = merge(objects, name)
    a = vertices[triangles[:, 0]]
    b = vertices[triangles[:, 1]]
    c = vertices[triangles[:, 2]]
    mask = ((numpy.minimum(numpy.minimum(a[:, 0], b[:, 0]), c[:, 0]) <= x)
            & (numpy.maximum(numpy.maximum(a[:, 0], b[:, 0]), c[:, 0]) >= x)
            & (numpy.minimum(numpy.minimum(a[:, 2], b[:, 2]), c[:, 2]) <= z)
            & (numpy.maximum(numpy.maximum(a[:, 2], b[:, 2]), c[:, 2]) >= z))
    if not mask.any():
        return None
    a, b, c = a[mask], b[mask], c[mask]
    v0 = b[:, [0, 2]] - a[:, [0, 2]]
    v1 = c[:, [0, 2]] - a[:, [0, 2]]
    v2 = numpy.array([x, z]) - a[:, [0, 2]]
    denominator = v0[:, 0] * v1[:, 1] - v1[:, 0] * v0[:, 1]
    good = numpy.abs(denominator) > 1.0e-12
    if not good.any():
        return None
    a, b, c = a[good], b[good], c[good]
    v0, v1, v2, denominator = v0[good], v1[good], v2[good], denominator[good]
    u = (v2[:, 0] * v1[:, 1] - v1[:, 0] * v2[:, 1]) / denominator
    v = (v0[:, 0] * v2[:, 1] - v2[:, 0] * v0[:, 1]) / denominator
    inside = (u >= -1.0e-6) & (v >= -1.0e-6) & (u + v <= 1.0 + 1.0e-6)
    if not inside.any():
        return None
    a, b, c, u, v = a[inside], b[inside], c[inside], u[inside], v[inside]
    return float((a[:, 1] * (1.0 - u - v) + b[:, 1] * u + c[:, 1] * v).min())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mesh", required=True)
    parser.add_argument("--xml", required=True)
    parser.add_argument("--points", required=True)
    parser.add_argument("--tolerance", type=float, default=0.0005)
    arguments = parser.parse_args()

    checks = 0
    failures = 0

    def check(condition, message):
        nonlocal checks, failures
        checks += 1
        if not condition:
            failures += 1
            print(f"FAIL {message}")

    objects = read_objects(arguments.mesh)
    proposal = json.loads(pathlib.Path(arguments.points).read_text())

    document = element_tree.parse(arguments.xml).getroot()

    def number(section, name):
        element = document.find(f"section[@name='{section}']/attnum[@name='{name}']")
        if element is None:
            raise ValueError(f"missing {section}/{name}")
        return float(element.get("val"))

    body_length = number("Car", "body length")
    body_width = number("Car", "body width")
    body_height = number("Car", "body height")
    gc_height = number("Car", "GC height")

    all_vertices = numpy.vstack([block[0] for blocks in objects.values()
                                  for block in blocks])
    bounds_min = all_vertices.min(axis=0)
    bounds_max = all_vertices.max(axis=0)

    check(abs((bounds_max[0] - bounds_min[0]) - body_length) < 1.0e-5,
          "ACC body length matches XML")
    check(abs((bounds_max[2] - bounds_min[2]) - body_width) < 1.0e-5,
          "ACC body width matches XML")
    check(abs((bounds_max[1] - bounds_min[1]) - body_height) < 1.0e-5,
          "ACC body height matches XML")

    front_axle = number("Front Axle", "xpos")
    rear_axle = number("Rear Axle", "xpos")
    front_repartition = number("Car", "front-rear weight repartition")
    center_of_gravity_x = front_axle * front_repartition + rear_axle * (1.0 - front_repartition)
    check(abs(center_of_gravity_x - (-0.456958055)) < 1.0e-6,
          "center of gravity x matches the proposal")

    lowest_vertex = bounds_min[1]
    check(abs(lowest_vertex + 0.025101) < 1.0e-6,
          "lowest vertex y matches the audit")
    nominal_minimum = proposal["nominal_body_position_meters"] + lowest_vertex
    check(abs(nominal_minimum - 0.024899) < 1.0e-6,
          "nominal minimum height matches 24.899 mm")
    check(abs(gc_height - 0.245) < 1.0e-9, "GC height matches 0.245 m")

    tiers = {1: 0, 2: 0, 3: 0}
    for group_name, points in proposal["groups"].items():
        for point in points:
            acc = point["acc"]
            solver = point["solver"]
            source = point["source"]
            surface = surface_at(objects, source, acc["x"], acc["z"])
            check(surface is not None,
                  f"{point['id']} has a surface at acc x/z")
            if surface is not None:
                check(abs(surface - acc["y"]) <= arguments.tolerance,
                      f"{point['id']} height matches the mesh (mesh={surface:.6f} "
                      f"proposal={acc['y']:.6f})")
            check(abs(solver["x"] - (acc["x"] + 0.456958055)) < 1.0e-6,
                  f"{point['id']} solver x mapping")
            check(abs(solver["y"] - (-acc["z"])) < 1.0e-9,
                  f"{point['id']} solver y mapping")
            check(abs(solver["z"] - acc["y"]) < 1.0e-9,
                  f"{point['id']} solver z mapping")
            check(abs(point["nominal_gap_meters"]
                      - (proposal["nominal_body_position_meters"] + solver["z"])) < 1.0e-9,
                  f"{point['id']} nominal gap")
            if group_name == "floor":
                tiers[point["tier"]] += 1

    check(tiers[1] == 8, "tier 1 has 8 floor points")
    check(tiers[1] + tiers[2] == 10, "tier 2 extends to 10 floor points")
    check(tiers[1] + tiers[2] + tiers[3] == 12,
          "tier 3 extends to 12 floor points")
    check(len(proposal["groups"]["front_wing"]) == 2, "front wing has 2 points")

    # Symmetry of the paired points (same x/z, opposite y).
    by_id = {point["id"]: point for group in proposal["groups"].values() for point in group}
    for left, right in (("F2", "F3"), ("R2", "R3"), ("L1", "L2"),
                        ("T2", "T3"), ("W1", "W2")):
        if left in by_id and right in by_id:
            a = by_id[left]["solver"]
            b = by_id[right]["solver"]
            check(abs(a["x"] - b["x"]) < 1.0e-9
                  and abs(a["y"] + b["y"]) < 1.0e-9
                  and abs(a["z"] - b["z"]) < 1.0e-9,
                  f"{left}/{right} are symmetric")

    print(f"Summary: checks={checks} failures={failures}")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
