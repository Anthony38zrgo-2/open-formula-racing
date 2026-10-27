//! Route-based ground coverage audit for the rally stage.
//!
//! Samples the actual game route (`staging_rally/integration/route_segments.json`)
//! and raycasts down at lateral offsets. This answers "is there terrain if I leave
//! the road?" independently of how the visual triangles are triangulated.

use std::path::Path;

use serde_json::Value;

use vehicle_physics_engine::ac3d_track::load_or_parse_visual_track;
use vehicle_physics_engine::collision_world::CollisionWorld;
use vehicle_physics_engine::types::Vec3;

fn num(v: &Value, key: &str) -> f64 {
    v[key].as_f64().unwrap_or(0.0)
}

fn main() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");
    let acc = std::env::args()
        .nth(1)
        .map(std::path::PathBuf::from)
        .unwrap_or_else(|| {
            root.join(
                "distribution/share/games/speed-dreams-2/tracks/circuit/russia-stage1/russia-stage1.acc",
            )
        });
    let route_path = root.join("staging_rally/integration/route_segments.json");
    let transform_path = root.join("staging_rally/integration/transform.json");
    let route: Value = serde_json::from_str(&std::fs::read_to_string(&route_path).unwrap()).unwrap();
    let transform: Value = serde_json::from_str(&std::fs::read_to_string(&transform_path).unwrap()).unwrap();
    let minimum = transform["minimum"].as_array().unwrap();
    let min = [
        minimum[0].as_f64().unwrap(),
        minimum[1].as_f64().unwrap(),
        minimum[2].as_f64().unwrap(),
    ];
    let segments = route.as_array().unwrap();
    let track = load_or_parse_visual_track(acc.to_str().unwrap(), None, None).unwrap();
    let world = CollisionWorld::from_host_export(
        "route-coverage",
        1,
        &track.driveable_triangles(),
        &track.driveable_materials(),
    )
    .unwrap();
    println!("route segments={} reference={}", segments.len(), acc.display());

    // (x_sd, y_sd, z_sd) = route - minimum; Rust = (-y_sd, z_sd, -x_sd).
    let to_rust = |x: f64, y: f64, z: f64| {
        Vec3::new(
            -(y - min[1]),
            z - min[2],
            -(x - min[0]),
        )
    };
    let offsets = [0.0f64, 3.0, 5.0, 8.0, 12.0, 20.0, 30.0, 50.0];
    let mut total = [0usize; 8];
    let mut missing = [0usize; 8];
    let mut missing_samples = Vec::new();
    let mut step = 0usize;
    for seg in segments {
        step += 1;
        if step % 2 != 0 {
            continue;
        }
        let start = seg["start"].as_array().unwrap();
        let end = seg["end"].as_array().unwrap();
        let x0 = start[0].as_f64().unwrap();
        let y0 = start[1].as_f64().unwrap();
        let z0 = start[2].as_f64().unwrap();
        let x1 = end[0].as_f64().unwrap();
        let y1 = end[1].as_f64().unwrap();
        let z1 = end[2].as_f64().unwrap();
        let heading = num(seg, "heading");
        let lateral = [-heading.sin(), heading.cos()];
        for frac in [0.25f64, 0.75] {
            let cx = x0 + (x1 - x0) * frac;
            let cy = y0 + (y1 - y0) * frac;
            let cz = z0 + (z1 - z0) * frac;
            for (slot, offset) in offsets.iter().enumerate() {
                for sign in [-1.0f64, 1.0] {
                    let px = cx + lateral[0] * offset * sign;
                    let py = cy + lateral[1] * offset * sign;
                    let point = to_rust(px, py, cz);
                    total[slot] += 1;
                    if world
                        .raycast(point + Vec3::new(0.0, 120.0, 0.0), -Vec3::UP, 400.0)
                        .is_none()
                    {
                        missing[slot] += 1;
                        if (*offset - 30.0).abs() < 0.1 && sign > 0.0 {
                            missing_samples.push((cx, cy, cz));
                        }
                    }
                }
            }
        }
    }
    for (slot, offset) in offsets.iter().enumerate() {
        let pct = if total[slot] == 0 {
            0.0
        } else {
            100.0 * missing[slot] as f64 / total[slot] as f64
        };
        println!(
            "lateral {:>4.0} m: missing ground {:>5}/{:<5} ({pct:.1}%)",
            offset, missing[slot], total[slot]
        );
    }
    println!("road points with no ground at +30 m (first 15):");
    for (x, y, z) in missing_samples.iter().take(15) {
        println!("  route=({x:.1},{y:.1},{z:.1})");
    }
}


