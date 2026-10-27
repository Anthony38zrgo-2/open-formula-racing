//! Diagnostic probe for the 2.5D wall field: reported contact gap versus the true
//! 2D penetration of the query polygon into the obstacle footprint.
//!
//! Run with:
//!   cargo run --release --example wall_contact_probe
//!
//! The scene is a flat driveable plane with one free-standing vertical box (a pole),
//! which is exactly the "vertical object" case the wall field is meant to resolve.
//! The car is approximated with the same five body boxes and four wheel rectangles
//! that `resolve_obstacle_wall_contacts` uses, swept past the pole at several
//! lateral offsets and yaws.

use vehicle_physics_engine::ac3d_track::{DriveableTriangle, ObstacleTriangle};
use vehicle_physics_engine::types::Vec3;
use vehicle_physics_engine::wall_field::{ConvexPoly2, WallField, WallSideMemory};

const HALF_POLE: f64 = 0.15;
const POLE_HEIGHT: f64 = 3.0;

fn plane(half: f64, y: f64) -> Vec<DriveableTriangle> {
    let a = Vec3::new(-half, y, -half);
    let b = Vec3::new(half, y, -half);
    let c = Vec3::new(half, y, half);
    let d = Vec3::new(-half, y, half);
    vec![
        DriveableTriangle { vertices: [a, b, c], friction: 1.0, rolling_resistance: 0.002 },
        DriveableTriangle { vertices: [a, c, d], friction: 1.0, rolling_resistance: 0.002 },
    ]
}

fn pole(center_x: f64, center_z: f64) -> Vec<ObstacleTriangle> {
    let x0 = center_x - HALF_POLE;
    let x1 = center_x + HALF_POLE;
    let z0 = center_z - HALF_POLE;
    let z1 = center_z + HALF_POLE;
    let y0 = 0.0;
    let y1 = POLE_HEIGHT;
    let p = |x: f64, y: f64, z: f64| Vec3::new(x, y, z);
    let quad = |a: Vec3, b: Vec3, c: Vec3, d: Vec3, out: &mut Vec<ObstacleTriangle>| {
        out.push(ObstacleTriangle { vertices: [a, b, c], friction: 0.5, restitution: 0.15, damage: 1.0 });
        out.push(ObstacleTriangle { vertices: [a, c, d], friction: 0.5, restitution: 0.15, damage: 1.0 });
    };
    let mut tris = Vec::new();
    // Four vertical faces. Winding is chosen so the raw 3D normal points outwards.
    quad(p(x0, y0, z0), p(x1, y0, z0), p(x1, y1, z0), p(x0, y1, z0), &mut tris); // -z face
    quad(p(x1, y0, z1), p(x0, y0, z1), p(x0, y1, z1), p(x1, y1, z1), &mut tris); // +z face
    quad(p(x1, y0, z0), p(x1, y0, z1), p(x1, y1, z1), p(x1, y1, z0), &mut tris); // +x face
    quad(p(x0, y0, z1), p(x0, y0, z0), p(x0, y1, z0), p(x0, y1, z1), &mut tris); // -x face
    tris
}

/// 2D convex hull of the box projection at a car pose, mimicking `from_box`.
fn body_polys(car_x: f64, car_z: f64, yaw: f64, ground_y: f64) -> Vec<(String, ConvexPoly2)> {
    let basis_x = Vec3::new(yaw.cos(), 0.0, -yaw.sin());
    let basis_y = Vec3::UP;
    let basis_z = Vec3::new(yaw.sin(), 0.0, yaw.cos());
    let boxes = [
        ("monocoque", Vec3::new(0.0, -0.02, -0.35), Vec3::new(0.28, 0.22, 0.80)),
        ("engine", Vec3::new(0.0, -0.03, 0.80), Vec3::new(0.30, 0.22, 0.60)),
        ("floor", Vec3::new(0.0, -0.26, 0.0), Vec3::new(0.64, 0.02, 1.28)),
        ("front_wing", Vec3::new(0.0, -0.17, -1.80), Vec3::new(0.80, 0.04, 0.17)),
        ("rear_wing", Vec3::new(0.0, 0.45, 1.70), Vec3::new(0.70, 0.04, 0.13)),
    ];
    let origin = Vec3::new(car_x, ground_y, car_z);
    boxes
        .iter()
        .map(|(name, local, half)| {
            let centre = origin
                + basis_x * local.x
                + basis_y * local.y
                + basis_z * local.z;
            (
                name.to_string(),
                ConvexPoly2::from_box(centre, basis_x, basis_y, basis_z, *half),
            )
        })
        .collect()
}

/// Signed distance between two convex polygons: >0 separated, <0 overlapping.
/// Uses SAT; returns the minimum overlap depth and its axis.
fn sat_overlap(a: &ConvexPoly2, b: &ConvexPoly2) -> (f64, [f64; 2]) {
    let mut best = f64::INFINITY;
    let mut best_axis = [1.0, 0.0];
    for poly in [a, b] {
        for i in 0..poly.n {
            let j = (i + 1) % poly.n;
            let ex = poly.v[j][0] - poly.v[i][0];
            let ez = poly.v[j][1] - poly.v[i][1];
            let len = (ex * ex + ez * ez).sqrt();
            if len < 1e-12 {
                continue;
            }
            let axis = [-ez / len, ex / len];
            let (a_min, a_max) = project(a, axis);
            let (b_min, b_max) = project(b, axis);
            let overlap = (a_max.min(b_max) - a_min.max(b_min)).abs();
            let separated = a_max < b_min || b_max < a_min;
            if separated {
                // Separating axis found: the polygons do not overlap; the distance
                // along this axis is the gap.
                return (-overlap, axis);
            }
            if overlap < best {
                best = overlap;
                best_axis = axis;
            }
        }
    }
    (best, best_axis)
}

fn project(p: &ConvexPoly2, axis: [f64; 2]) -> (f64, f64) {
    let mut lo = f64::INFINITY;
    let mut hi = f64::NEG_INFINITY;
    for i in 0..p.n {
        let v = p.v[i][0] * axis[0] + p.v[i][1] * axis[1];
        lo = lo.min(v);
        hi = hi.max(v);
    }
    (lo, hi)
}

fn main() {
    let ground = plane(60.0, 0.0);
    let obstacles = pole(0.0, 0.0);
    let field = WallField::build(&ground, &obstacles);
    println!("=== pole wall field: {} segments ({} one-sided, {} two-sided) ===",
        field.len(), field.one_sided_count, field.two_sided_count);
    for (i, s) in field.segments.iter().enumerate() {
        println!(
            "  seg {i}: a=({:.3},{:.3}) b=({:.3},{:.3}) len={:.3} n=({:.3},{:.3}) y=[{:.2},{:.2}] two_sided={}",
            s.a[0], s.a[1], s.b[0], s.b[1], s.len, s.normal[0], s.normal[1], s.y_min, s.y_max, s.two_sided
        );
    }

    let ground_y = 0.30;
    println!("\n=== swept tangential passes (yaw sweep) ===");
    println!("offset_m yaw_deg  poly        reported_gap true_pen  phantom  normal       point");
    let mut worst: Option<(f64, String)> = None;
    let mut side_mem = WallSideMemory::default();
    let mut contacts = Vec::new();
    let mut scratch = Vec::new();

    for &offset in &[1.02f64, 1.06, 1.10, 1.14, 1.18, 1.25] {
        for &yaw_deg in &[-8.0f64, -4.0, 0.0, 4.0, 8.0] {
            let yaw = yaw_deg.to_radians();
            let car_x = -offset;
            for &car_z in &[-1.0, -0.5, 0.0, 0.5, 1.0] {
                side_mem = WallSideMemory::default();
                contacts.clear();
                let polys = body_polys(car_x, car_z, yaw, ground_y);
                for (name, poly) in &polys {
                    let before = contacts.len();
                    field.query_poly(poly, 0.40, &mut side_mem,
                        Vec3::new(car_x, ground_y, car_z), 0.0, &mut scratch, &mut contacts);
                    for c in &contacts[before..] {
                        // The pole footprint, as a polygon, for the true distance test.
                        let pole_poly = ConvexPoly2::from_box(
                            Vec3::new(0.0, 0.0, 0.0),
                            Vec3::new(1.0, 0.0, 0.0),
                            Vec3::new(0.0, 1.0, 0.0),
                            Vec3::new(0.0, 0.0, 1.0),
                            Vec3::new(HALF_POLE, POLE_HEIGHT * 0.5, HALF_POLE),
                        );
                        let (true_overlap, _) = sat_overlap(poly, &pole_poly);
                        let phantom = (-c.gap_m - true_overlap.max(0.0)).max(0.0);
                        println!(
                            "{offset:8.3} {yaw_deg:7.1} {name:11} gap={:8.3} true={:8.3} phantom={:8.3} n=({:6.3},{:6.3}) p=({:7.3},{:7.3})",
                            c.gap_m, true_overlap, phantom, c.normal.x, c.normal.z, c.point.x, c.point.z
                        );
                        if phantom > worst.as_ref().map(|w| w.0).unwrap_or(0.0) {
                            worst = Some((phantom, format!(
                                "offset={offset} yaw={yaw_deg} z={car_z} poly={name} gap={} true={} n=({},{})",
                                c.gap_m, true_overlap, c.normal.x, c.normal.z)));
                        }
                    }
                }
            }
        }
    }
    match worst {
        Some((p, text)) => println!("\nWORST PHANTOM PENETRATION: {p:.3} m\n  {text}"),
        None => println!("\nno phantom penetration found"),
    }
}
