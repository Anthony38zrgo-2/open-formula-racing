//! Ground coverage audit for a visual stage.
//!
//! Samples gravel-road triangles from the driveable mesh and raycasts down at
//! lateral offsets. Reports how often there is no ground outside the road, which
//! is where a rally car ends up when it leaves the track.

use std::path::Path;

use vehicle_physics_engine::ac3d_track::load_or_parse_visual_track;
use vehicle_physics_engine::collision_world::CollisionWorld;
use vehicle_physics_engine::types::Vec3;

fn main() {
    let Some(path) = std::env::args().nth(1) else {
        eprintln!("usage: stage_coverage <track.acc> [every_n]");
        return;
    };
    let every: usize = std::env::args()
        .nth(2)
        .and_then(|v| v.parse().ok())
        .unwrap_or(100);
    let track = match load_or_parse_visual_track(&path, None, None) {
        Ok(t) => t,
        Err(e) => {
            eprintln!("load failed: {e}");
            return;
        }
    };
    let triangles = track.driveable_triangles();
    let materials = track.driveable_materials();
    let world = CollisionWorld::from_host_export("coverage", 1, &triangles, &materials)
        .expect("driveable world");
    println!(
        "{} driveable={} road-ish={}",
        path,
        track.driveable.len(),
        track
            .driveable
            .iter()
            .filter(|t| t.rolling_resistance > 0.09 && t.rolling_resistance < 0.15)
            .count()
    );

    let offsets = [3.0f64, 5.0, 8.0, 12.0, 20.0, 30.0, 50.0];
    let mut total = [0usize; 7];
    let mut missing = [0usize; 7];
    let mut missing_road_30 = Vec::new();
    let mut sampled = 0usize;
    let mut index = 0usize;
    for tri in &track.driveable {
        index += 1;
        if tri.rolling_resistance <= 0.09 || tri.rolling_resistance >= 0.15 {
            continue;
        }
        if index % every != 0 {
            continue;
        }
        sampled += 1;
        let v = tri.vertices;
        let centroid = (v[0] + v[1] + v[2]) * (1.0 / 3.0);
        // Longest 2D edge is the road direction on a ribbon triangle.
        let edges = [(v[1] - v[0]), (v[2] - v[1]), (v[0] - v[2])];
        let mut best_len = 0.0;
        let mut tangent = Vec3::new(0.0, 0.0, -1.0);
        for edge in edges {
            let flat = Vec3::new(edge.x, 0.0, edge.z);
            let len = flat.length();
            if len > best_len {
                best_len = len;
                if len > 1e-9 {
                    tangent = flat * (1.0 / len);
                }
            }
        }
        let lateral = Vec3::new(-tangent.z, 0.0, tangent.x);
        for (slot, offset) in offsets.iter().enumerate() {
            for sign in [-1.0f64, 1.0] {
                let origin = centroid
                    + lateral * (sign * offset)
                    + Vec3::new(0.0, 40.0, 0.0);
                total[slot] += 1;
                if world
                    .raycast(origin, -Vec3::UP, 80.0)
                    .is_none()
                {
                    missing[slot] += 1;
                    if (*offset - 30.0).abs() < 0.1 {
                        missing_road_30.push((centroid, sign));
                    }
                }
            }
        }
    }

    println!("sampled road triangles: {sampled}");
    for (slot, offset) in offsets.iter().enumerate() {
        let pct = if total[slot] == 0 {
            0.0
        } else {
            100.0 * missing[slot] as f64 / total[slot] as f64
        };
        println!(
            "  lateral {:>4.0} m: missing ground {:>6}/{:<6} ({pct:.1}%)",
            offset, missing[slot], total[slot]
        );
    }
    println!("sample of road points with no ground at 30 m:");
    for (centroid, sign) in missing_road_30.iter().take(20) {
        println!(
            "  road=({:.1},{:.1},{:.1}) side={}",
            centroid.x, centroid.y, centroid.z, sign
        );
    }
    let _ = Path::new(&path);
}
