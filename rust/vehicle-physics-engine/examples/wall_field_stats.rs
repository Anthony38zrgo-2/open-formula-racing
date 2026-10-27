//! Read-only statistics of the wall field built from a real AC3D circuit.
//!
//! cargo run --release --example wall_field_stats -- <track.acc>

use vehicle_physics_engine::ac3d_track::load_or_parse_visual_track;
use vehicle_physics_engine::collision_world::CollisionWorld;

fn main() {
    let Some(path) = std::env::args().nth(1) else {
        eprintln!("usage: wall_field_stats <track.acc>");
        return;
    };
    let started = std::time::Instant::now();
    let track = match load_or_parse_visual_track(&path, None, None) {
        Ok(track) => track,
        Err(error) => {
            eprintln!("load failed: {error}");
            return;
        }
    };
    let load_ms = started.elapsed().as_secs_f64() * 1000.0;
    let built = std::time::Instant::now();
    let world = match CollisionWorld::load_visual_track(&path, None, None) {
        Ok(world) => world,
        Err(error) => {
            eprintln!("world build failed: {error}");
            return;
        }
    };
    let walls = world.walls();
    let build_ms = built.elapsed().as_secs_f64() * 1000.0;

    let mut bins = [0usize; 5];
    let mut short = 0usize;
    let mut short_two_sided = 0usize;
    let mut y_min = f64::INFINITY;
    let mut y_max = f64::NEG_INFINITY;
    for segment in &walls.segments {
        let len = segment.len;
        if len < 0.5 {
            bins[0] += 1;
        } else if len < 1.0 {
            bins[1] += 1;
        } else if len < 3.0 {
            bins[2] += 1;
        } else if len < 10.0 {
            bins[3] += 1;
        } else {
            bins[4] += 1;
        }
        if len < 1.0 {
            short += 1;
            if segment.two_sided {
                short_two_sided += 1;
            }
        }
        y_min = y_min.min(segment.y_min);
        y_max = y_max.max(segment.y_max);
    }
    println!(
        "{} driveable={} obstacles={} load={:.0}ms build={:.0}ms",
        path,
        track.driveable.len(),
        track.obstacles.len(),
        load_ms,
        build_ms
    );
    println!(
        "segments={} one_sided={} two_sided={} (short<1m={} short_two_sided={}) y=[{:.2},{:.2}]",
        walls.len(),
        walls.one_sided_count,
        walls.two_sided_count,
        short,
        short_two_sided,
        y_min,
        y_max
    );
    println!(
        "length bins: <0.5m={} 0.5-1={} 1-3={} 3-10={} >10={}",
        bins[0], bins[1], bins[2], bins[3], bins[4]
    );
}
