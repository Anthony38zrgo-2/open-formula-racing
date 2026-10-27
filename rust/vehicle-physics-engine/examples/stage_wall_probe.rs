//! Reports the wall segments closest to the rally route, to see whether the
//! invisible barrier is at the road edge or farther out.

use std::path::Path;

use serde_json::Value;

use vehicle_physics_engine::collision_world::CollisionWorld;
use vehicle_physics_engine::types::Vec3;

fn point_segment_distance_2d(p: [f64; 2], a: [f64; 2], b: [f64; 2]) -> f64 {
    let ex = b[0] - a[0];
    let ez = b[1] - a[1];
    let len_sq = ex * ex + ez * ez;
    if len_sq < 1e-12 {
        return ((p[0] - a[0]).powi(2) + (p[1] - a[1]).powi(2)).sqrt();
    }
    let t = (((p[0] - a[0]) * ex + (p[1] - a[1]) * ez) / len_sq).clamp(0.0, 1.0);
    let qx = a[0] + ex * t;
    let qz = a[1] + ez * t;
    ((p[0] - qx).powi(2) + (p[1] - qz).powi(2)).sqrt()
}

fn main() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");
    let acc = root.join(
        "distribution/share/games/speed-dreams-2/tracks/circuit/russia-stage1/russia-stage1.acc",
    );
    let route: Value =
        serde_json::from_str(&std::fs::read_to_string(root.join("staging_rally/integration/route_segments.json")).unwrap()).unwrap();
    let transform: Value = serde_json::from_str(
        &std::fs::read_to_string(root.join("staging_rally/integration/transform.json")).unwrap(),
    )
    .unwrap();
    let minimum = transform["minimum"].as_array().unwrap();
    let min = [
        minimum[0].as_f64().unwrap(),
        minimum[1].as_f64().unwrap(),
        minimum[2].as_f64().unwrap(),
    ];
    let world = CollisionWorld::load_visual_track(acc.to_str().unwrap(), None, None).unwrap();
    let walls = world.walls();
    println!("walls={} segments", walls.len());
    let to_rust = |x: f64, y: f64, z: f64| {
        Vec3::new(-(y - min[1]), z - min[2], -(x - min[0]))
    };
    let mut bins = [0usize; 6];
    let mut samples = 0usize;
    let mut edge_examples = Vec::new();
    let mut index = 0usize;
    for seg in route.as_array().unwrap() {
        index += 1;
        if index % 20 != 0 {
            continue;
        }
        let start = seg["start"].as_array().unwrap();
        let p = to_rust(
            start[0].as_f64().unwrap(),
            start[1].as_f64().unwrap(),
            start[2].as_f64().unwrap(),
        );
        let mut best = f64::INFINITY;
        let mut best_seg = 0usize;
        for (i, wall) in walls.segments.iter().enumerate() {
            let d = point_segment_distance_2d([p.x, p.z], wall.a, wall.b);
            if d < best {
                best = d;
                best_seg = i;
            }
        }
        samples += 1;
        let bin = if best < 3.0 {
            0
        } else if best < 5.0 {
            1
        } else if best < 10.0 {
            2
        } else if best < 20.0 {
            3
        } else if best < 50.0 {
            4
        } else {
            5
        };
        bins[bin] += 1;
        if bin <= 2 && edge_examples.len() < 12 {
            let w = &walls.segments[best_seg];
            edge_examples.push((p, best, w));
        }
    }
    println!(
        "nearest wall: <3m={} 3-5m={} 5-10m={} 10-20m={} 20-50m={} >50m={} (samples={})",
        bins[0], bins[1], bins[2], bins[3], bins[4], bins[5], samples
    );
    for (p, d, w) in edge_examples {
        println!(
            "  route Rust=({:.1},{:.1},{:.1}) nearest={:.2}m seg a=({:.2},{:.2}) b=({:.2},{:.2}) n=({:.2},{:.2}) y=[{:.2},{:.2}]",
            p.x, p.y, p.z, d, w.a[0], w.a[1], w.b[0], w.b[1], w.normal[0], w.normal[1], w.y_min, w.y_max
        );
    }
}

