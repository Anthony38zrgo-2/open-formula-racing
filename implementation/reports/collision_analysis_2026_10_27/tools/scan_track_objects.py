"""Scan the AC3D visual circuits the way `ac3d_track.rs` does and report which
objects become obstacle wall segments.

Read-only diagnostic: replicates the parser's classification (name keywords and
normal/height thresholds) so we can see which vertical props contribute wall
segments to the 2.5D wall field. Output is a CSV plus a short console summary.
"""

import csv
import math
import os
import sys
from collections import defaultdict

TRACKS = [
    r"D:\open-formula\distribution\share\games\speed-dreams-2\tracks\circuit\melbourne-nineteen-ninety-seven\melbourne-nineteen-ninety-seven.acc",
    r"D:\open-formula\distribution\share\games\speed-dreams-2\tracks\circuit\fuji-seventy-six-seventy-seven\fuji-seventy-six-seventy-seven.acc",
    r"D:\open-formula\distribution\share\games\speed-dreams-2\tracks\circuit\jarama\jarama.acc",
    r"D:\open-formula\distribution\share\games\speed-dreams-2\tracks\circuit\russia-stage1\russia-stage1.acc",
]

IGNORED = ["sky", "horizon", "shadow", "shad", "reflect_", "bestline", "crowd", "cameraman",
           "marshall", "pitman", "chopper", "billboard", "tree", "people", "ppl", "videotv",
           "fbuild", "largesign", "smallsign", "pitslot", "stslot", "aisle", "bridge", "stand",
           "tribune", "light"]
OBSTACLES = ["wall", "fence", "tirewall", "tyrewall", "armco", "barrier", "garage", "pitbld",
             "building", "mbox", "cone", "flag_pitblock", "ctrlcntr", "ctrltwr", "rearwall",
             "guard", "podium", "rvboxes", "trailer", "amblnc", "towtrk", "pickup"]
DRIVEABLE = ["track", "trak", "pitlane", "pitfloor", "pit_box", "paddock", "kerb", "curb",
             "verge", "sidl", "sidr", "nland", "grass", "gravel", "sand", "road", "asphalt"]


def find_key(name, keys):
    return next((k for k in keys if k in name), None)


def parse(path):
    """Yields (object_name, [triangles]) with triangles as 9-tuples in Rust frame."""
    name = ""
    verts = []
    tris = []
    obj_name = None
    with open(path, "r", encoding="latin-1", errors="replace") as f:
        lines = f.readlines()
    i = 0
    n = len(lines)
    while i < n:
        raw = lines[i]
        line = raw.strip()
        i += 1
        if line.startswith("OBJECT"):
            if obj_name is not None:
                yield obj_name, tris
            obj_name = "?"
            name = ""
            verts = []
            tris = []
        elif line.startswith('name "'):
            end = line.find('"', 6)
            if end > 0:
                name = line[6:end]
                obj_name = name
        elif line.startswith("numvert "):
            count = int(line[8:].strip() or 0)
            verts = []
            for _ in range(count):
                if i >= n:
                    break
                parts = lines[i].split()
                i += 1
                if len(parts) < 3:
                    continue
                x, y, z = (float(parts[0]), float(parts[1]), float(parts[2]))
                # AC3D (x, y, z) -> Rust (z, y, -x)
                verts.append((z, y, -x))
        elif line.startswith("numsurf "):
            surf = int(line[8:].strip() or 0)
            for _ in range(surf):
                refs_count = 0
                while i < n:
                    t = lines[i].strip()
                    i += 1
                    if t.startswith("refs "):
                        refs_count = int(t[5:].strip() or 0)
                        break
                refs = []
                for _ in range(refs_count):
                    if i >= n:
                        break
                    tok = lines[i].split()
                    i += 1
                    if tok:
                        try:
                            refs.append(int(tok[0]))
                        except ValueError:
                            pass
                if len(refs) < 3 or not verts:
                    continue
                for k in range(2, len(refs)):
                    a, b, c = refs[0], refs[k - 1], refs[k]
                    if max(a, b, c) >= len(verts):
                        continue
                    tris.append((verts[a], verts[b], verts[c], name))
    if obj_name is not None:
        yield obj_name, tris


def tri_stats(v0, v1, v2):
    e1 = (v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2])
    e2 = (v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2])
    nx = e1[1] * e2[2] - e1[2] * e2[1]
    ny = e1[2] * e2[0] - e1[0] * e2[2]
    nz = e1[0] * e2[1] - e1[1] * e2[0]
    area = math.sqrt(nx * nx + ny * ny + nz * nz)
    return ny / area if area > 1e-9 else 1.0


def main():
    out_rows = []
    for path in TRACKS:
        if not os.path.exists(path):
            continue
        track = os.path.basename(path)
        per_obj = defaultdict(list)
        for obj, tris in parse(path):
            per_obj[obj].extend(tris)

        for obj, tris in per_obj.items():
            if not tris:
                continue
            name = tris[0][3].lower()
            if find_key(name, IGNORED):
                continue
            xs = [p[0] for t in tris for p in t[:3]]
            ys = [p[1] for t in tris for p in t[:3]]
            zs = [p[2] for t in tris for p in t[:3]]
            footprint_x = max(xs) - min(xs)
            footprint_z = max(zs) - min(zs)
            height = max(ys) - min(ys)
            obstacle_tris = 0
            wall_2d = 0
            for t in tris:
                v0, v1, v2 = t[0], t[1], t[2]
                ny_abs = abs(tri_stats(v0, v1, v2))
                hspan = max(v0[1], v1[1], v2[1]) - min(v0[1], v1[1], v2[1])
                is_obs = bool(find_key(name, OBSTACLES))
                is_drv = bool(find_key(name, DRIVEABLE))
                obstacle = False
                if is_obs:
                    obstacle = ny_abs < 0.55
                elif is_drv:
                    obstacle = (ny_abs < 0.55) and hspan >= 0.35
                else:
                    obstacle = ny_abs < 0.40 and hspan >= 0.35
                if obstacle:
                    obstacle_tris += 1
                # wall_field: horizontal normal needs |n_h| > 1e-5
                e1 = (v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2])
                e2 = (v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2])
                nx = e1[1] * e2[2] - e1[2] * e2[1]
                nz = e1[0] * e2[1] - e1[1] * e2[0]
                if obstacle and math.hypot(nx, nz) > 1e-5:
                    wall_2d += 1
            if obstacle_tris:
                out_rows.append({
                    "track": track,
                    "object": obj,
                    "name": tris[0][3],
                    "triangles": len(tris),
                    "obstacle_triangles": obstacle_tris,
                    "wall_segments_before_merge": wall_2d,
                    "height_m": round(height, 2),
                    "footprint_x_m": round(footprint_x, 2),
                    "footprint_z_m": round(footprint_z, 2),
                    "min_footprint_m": round(min(footprint_x, footprint_z), 3),
                    "vertical_slender": height > 1.5 and min(footprint_x, footprint_z) < 1.5,
                })

    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(here, "vertical_objects.csv")
    out_rows.sort(key=lambda r: (-r["obstacle_triangles"], r["track"]))
    with open(out, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()))
        writer.writeheader()
        writer.writerows(out_rows)

    print(f"objects with obstacle triangles: {len(out_rows)} -> {out}")
    by_track = defaultdict(lambda: [0, 0])
    for r in out_rows:
        by_track[r["track"]][0] += 1
        by_track[r["track"]][1] += r["obstacle_triangles"]
    for k, v in by_track.items():
        print(f"  {k}: {v[0]} objects, {v[1]} obstacle triangles")
    slender = [r for r in out_rows if r["vertical_slender"]]
    print(f"vertical slender objects (tall, footprint < 1.5 m): {len(slender)}")
    for r in slender[:40]:
        print("   ", r["object"], r["name"], f"h={r['height_m']}m", f"fp={r['footprint_x_m']}x{r['footprint_z_m']}m", f"walltris={r['wall_segments_before_merge']}")


if __name__ == "__main__":
    main()
