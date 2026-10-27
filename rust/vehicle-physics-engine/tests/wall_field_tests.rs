use vehicle_physics_engine::ac3d_track::{DriveableTriangle, ObstacleTriangle};
use vehicle_physics_engine::types::Vec3;
use vehicle_physics_engine::wall_field::{ConvexPoly2, WallField, WallSideMemory};

#[test]
fn test_collinear_wall_merging() {
    let ground = vec![
        DriveableTriangle {
            vertices: [
                Vec3::new(-10.0, 0.0, -10.0),
                Vec3::new(5.0, 0.0, -10.0),
                Vec3::new(5.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
        DriveableTriangle {
            vertices: [
                Vec3::new(-10.0, 0.0, -10.0),
                Vec3::new(5.0, 0.0, 10.0),
                Vec3::new(-10.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
    ];

    // Two adjacent collinear wall segments along Z from Z=0 to Z=5 and Z=5 to Z=10 at X=5.0
    let walls = vec![
        ObstacleTriangle {
            vertices: [
                Vec3::new(5.0, 0.0, 0.0),
                Vec3::new(5.0, 2.0, 0.0),
                Vec3::new(5.0, 2.0, 5.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
        ObstacleTriangle {
            vertices: [
                Vec3::new(5.0, 0.0, 0.0),
                Vec3::new(5.0, 2.0, 5.0),
                Vec3::new(5.0, 0.0, 5.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
        ObstacleTriangle {
            vertices: [
                Vec3::new(5.0, 0.0, 5.0),
                Vec3::new(5.0, 2.0, 5.0),
                Vec3::new(5.0, 2.0, 10.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
        ObstacleTriangle {
            vertices: [
                Vec3::new(5.0, 0.0, 5.0),
                Vec3::new(5.0, 2.0, 10.0),
                Vec3::new(5.0, 0.0, 10.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
    ];

    let field = WallField::build(&ground, &walls);
    assert_eq!(field.len(), 1, "4 collinear triangles must merge into 1 continuous wall segment");
    assert!(field.segments[0].len >= 9.9, "merged length must span full 10m");
    // Normal should point toward track (track is on -X side, so normal should be [-1, 0])
    assert!(field.segments[0].normal[0] < -0.9, "normal must point towards ground on -X");
}

#[test]
fn test_wall_poly_query_speculative_gap() {
    let ground = vec![
        DriveableTriangle {
            vertices: [
                Vec3::new(-10.0, 0.0, -10.0),
                Vec3::new(5.0, 0.0, -10.0),
                Vec3::new(5.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
        DriveableTriangle {
            vertices: [
                Vec3::new(-10.0, 0.0, -10.0),
                Vec3::new(5.0, 0.0, 10.0),
                Vec3::new(-10.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
    ];

    // Wall at X = 5.0 from Z = -10 to +10
    let walls = vec![
        ObstacleTriangle {
            vertices: [
                Vec3::new(5.0, 0.0, -10.0),
                Vec3::new(5.0, 2.0, -10.0),
                Vec3::new(5.0, 2.0, 10.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
        ObstacleTriangle {
            vertices: [
                Vec3::new(5.0, 0.0, -10.0),
                Vec3::new(5.0, 2.0, 10.0),
                Vec3::new(5.0, 0.0, 10.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
    ];

    let field = WallField::build(&ground, &walls);
    let mut side_mem = WallSideMemory::default();
    let mut scratch = Vec::new();
    let mut contacts = Vec::new();

    // 1. Box approaching wall from track side (centered at X=4.5, half-width=0.4, right edge at X=4.9)
    // Distance to wall at X=5.0 is +0.10 m (positive gap = approaching, no penetration yet)
    let poly_approaching = ConvexPoly2::from_box(
        Vec3::new(4.5, 1.0, 0.0),
        Vec3::new(1.0, 0.0, 0.0),
        Vec3::new(0.0, 1.0, 0.0),
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.4, 0.4, 0.5),
    );

    field.query_poly(
        &poly_approaching,
        0.20, // prediction margin of 20 cm
        &mut side_mem,
        Vec3::new(4.5, 1.0, 0.0),
        0.0,
        &mut scratch,
        &mut contacts,
    );

    assert!(!contacts.is_empty(), "must detect speculative approaching contact within prediction margin");
    let c = &contacts[0];
    assert!(c.gap_m > 0.05 && c.gap_m < 0.15, "approaching gap should be ~0.10 m, got {}", c.gap_m);
    assert!(c.normal.x < -0.9, "normal must point away from wall toward track (-X)");

    // 2. Box penetrating wall (centered at X=4.8, right edge at X=5.2) -> penetration = 0.20 m (gap = -0.20 m)
    contacts.clear();
    let poly_penetrating = ConvexPoly2::from_box(
        Vec3::new(4.8, 1.0, 0.0),
        Vec3::new(1.0, 0.0, 0.0),
        Vec3::new(0.0, 1.0, 0.0),
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.4, 0.4, 0.5),
    );

    field.query_poly(
        &poly_penetrating,
        0.20,
        &mut side_mem,
        Vec3::new(4.8, 1.0, 0.0),
        0.0,
        &mut scratch,
        &mut contacts,
    );

    assert!(!contacts.is_empty());
    let c_pen = &contacts[0];
    assert!(c_pen.gap_m < -0.15 && c_pen.gap_m > -0.25, "penetrating gap should be ~ -0.20 m, got {}", c_pen.gap_m);
    assert!(c_pen.normal.x < -0.9, "normal must remain invariant pointing toward track (-X)");
}

#[test]
fn test_oriented_wheel_poly() {
    let hub = Vec3::new(2.0, 0.33, 0.0);
    let forward = Vec3::new(0.0, 0.0, -1.0);
    let lateral = Vec3::new(1.0, 0.0, 0.0);
    let radius = 0.33;
    let half_width = 0.18;

    let wheel_poly = ConvexPoly2::from_wheel(hub, forward, lateral, radius, half_width);
    assert_eq!(wheel_poly.n, 4);

    let mut min_x = f64::INFINITY;
    let mut max_x = f64::NEG_INFINITY;
    let mut min_z = f64::INFINITY;
    let mut max_z = f64::NEG_INFINITY;

    for i in 0..wheel_poly.n {
        min_x = min_x.min(wheel_poly.v[i][0]);
        max_x = max_x.max(wheel_poly.v[i][0]);
        min_z = min_z.min(wheel_poly.v[i][1]);
        max_z = max_z.max(wheel_poly.v[i][1]);
    }

    assert!((min_x - (2.0 - 0.18)).abs() < 1e-4);
    assert!((max_x - (2.0 + 0.18)).abs() < 1e-4);
    assert!((min_z - (-0.33)).abs() < 1e-4);
    assert!((max_z - (0.33)).abs() < 1e-4);
}

#[test]
fn test_two_sided_wall_side_memory() {
    let empty_ground: Vec<DriveableTriangle> = Vec::new();
    let walls = vec![
        ObstacleTriangle {
            vertices: [
                Vec3::new(0.0, 0.0, -10.0),
                Vec3::new(0.0, 2.0, -10.0),
                Vec3::new(0.0, 2.0, 10.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
        ObstacleTriangle {
            vertices: [
                Vec3::new(0.0, 0.0, -10.0),
                Vec3::new(0.0, 2.0, 10.0),
                Vec3::new(0.0, 0.0, 10.0),
            ],
            friction: 0.50,
            restitution: 0.15,
            damage: 1.0,
        },
    ];

    let field = WallField::build(&empty_ground, &walls);
    assert_eq!(field.len(), 1);
    assert!(field.segments[0].two_sided, "wall with no ground must be marked two_sided");

    let mut side_mem = WallSideMemory::default();
    let mut scratch = Vec::new();
    let mut contacts = Vec::new();

    // 1. Vehicle approaching from +X side (centered at X = 0.5)
    let poly1 = ConvexPoly2::from_box(
        Vec3::new(0.5, 1.0, 0.0),
        Vec3::new(1.0, 0.0, 0.0),
        Vec3::new(0.0, 1.0, 0.0),
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.4, 0.4, 0.5),
    );

    field.query_poly(&poly1, 0.20, &mut side_mem, Vec3::new(0.5, 1.0, 0.0), 0.0, &mut scratch, &mut contacts);
    assert!(!contacts.is_empty());
    assert!(contacts[0].normal.x > 0.9, "must face +X because vehicle is on +X side");

    // 2. Vehicle penetrates through wall center to X = 0.1
    contacts.clear();
    let poly2 = ConvexPoly2::from_box(
        Vec3::new(0.1, 1.0, 0.0),
        Vec3::new(1.0, 0.0, 0.0),
        Vec3::new(0.0, 1.0, 0.0),
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.4, 0.4, 0.5),
    );

    // Side memory should preserve the +X normal instead of flipping
    field.query_poly(&poly2, 0.20, &mut side_mem, Vec3::new(0.1, 1.0, 0.0), 0.01, &mut scratch, &mut contacts);
    assert!(!contacts.is_empty());
    assert!(contacts[0].normal.x > 0.9, "memory must prevent normal flip despite penetration");
}

// ---------------------------------------------------------------------------
// Finite-segment contact query
//
// The regression these tests pin: a wall face used to be treated as an infinite
// plane. A long body polygon (floor, front wing, rear wing) sweeping past a short
// post could cross the plane of one of the post's faces far away from the post and
// be reported as up to ~1.6 m deep, producing a violent push and torque with no
// contact. The query must be a finite segment-to-polygon query instead.
// ---------------------------------------------------------------------------

const POLE_HALF: f64 = 0.15;
const POLE_HEIGHT: f64 = 3.0;

fn flat_plane(half_extent: f64) -> Vec<DriveableTriangle> {
    vec![
        DriveableTriangle {
            vertices: [
                Vec3::new(-half_extent, 0.0, -half_extent),
                Vec3::new(half_extent, 0.0, -half_extent),
                Vec3::new(half_extent, 0.0, half_extent),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
        DriveableTriangle {
            vertices: [
                Vec3::new(-half_extent, 0.0, -half_extent),
                Vec3::new(half_extent, 0.0, half_extent),
                Vec3::new(-half_extent, 0.0, half_extent),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
    ]
}

fn pole_at(center_x: f64, center_z: f64) -> Vec<ObstacleTriangle> {
    let x0 = center_x - POLE_HALF;
    let x1 = center_x + POLE_HALF;
    let z0 = center_z - POLE_HALF;
    let z1 = center_z + POLE_HALF;
    let y0 = 0.0;
    let y1 = POLE_HEIGHT;
    let p = |x: f64, y: f64, z: f64| Vec3::new(x, y, z);
    let mut tris = Vec::new();
    let quad = |a: Vec3, b: Vec3, c: Vec3, d: Vec3, out: &mut Vec<ObstacleTriangle>| {
        out.push(ObstacleTriangle { vertices: [a, b, c], friction: 0.5, restitution: 0.15, damage: 1.0 });
        out.push(ObstacleTriangle { vertices: [a, c, d], friction: 0.5, restitution: 0.15, damage: 1.0 });
    };
    quad(p(x0, y0, z0), p(x1, y0, z0), p(x1, y1, z0), p(x0, y1, z0), &mut tris);
    quad(p(x1, y0, z1), p(x0, y0, z1), p(x0, y1, z1), p(x1, y1, z1), &mut tris);
    quad(p(x1, y0, z0), p(x1, y0, z1), p(x1, y1, z1), p(x1, y1, z0), &mut tris);
    quad(p(x0, y0, z1), p(x0, y0, z0), p(x0, y1, z0), p(x0, y1, z1), &mut tris);
    tris
}

/// The five rigid body boxes `resolve_obstacle_wall_contacts` queries, at a pose.
fn body_polys(car_x: f64, car_z: f64, yaw: f64, ground_y: f64) -> Vec<(&'static str, ConvexPoly2)> {
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
            let centre = origin + basis_x * local.x + basis_y * local.y + basis_z * local.z;
            (*name, ConvexPoly2::from_box(centre, basis_x, basis_y, basis_z, *half))
        })
        .collect()
}

fn query_all(
    field: &WallField,
    polys: &[(&str, ConvexPoly2)],
    car_x: f64,
    car_z: f64,
    prediction: f64,
) -> Vec<(String, vehicle_physics_engine::wall_field::WallContact)> {
    let mut side_mem = WallSideMemory::default();
    let mut scratch = Vec::new();
    let mut contacts = Vec::new();
    let mut out = Vec::new();
    for (name, poly) in polys {
        contacts.clear();
        field.query_poly(
            poly,
            prediction,
            &mut side_mem,
            Vec3::new(car_x, 0.30, car_z),
            0.0,
            &mut scratch,
            &mut contacts,
        );
        for c in &contacts {
            out.push(((*name).to_string(), *c));
        }
    }
    out
}

#[test]
fn short_post_does_not_report_penetration_for_a_clear_tangential_pass() {
    let field = WallField::build(&flat_plane(60.0), &pole_at(0.0, 0.0));
    assert!(!field.is_empty());

    // Two poses in which the five body boxes are clear of the post by 0.2 m and
    // 0.13 m respectively, with a realistic yaw. The old infinite-plane query
    // reported gaps of -1.03 m (floor) and -1.57 m (rear wing); a negative gap is
    // what the impulse solver and the split-impulse push consume.
    for &(offset, yaw_deg) in &[(1.06f64, -8.0f64), (1.25, -8.0), (1.25, 8.0)] {
        let yaw = yaw_deg.to_radians();
        let polys = body_polys(-offset, 0.0, yaw, 0.30);
        let contacts = query_all(&field, &polys, -offset, 0.0, 0.40);
        for (name, c) in &contacts {
            assert!(
                c.gap_m > -0.02,
                "{name} reports phantom penetration {:.3} m at {:?} (normal {:?}) for a clear pass (offset {offset}, yaw {yaw_deg})",
                c.gap_m, c.point, c.normal
            );
        }
    }
}

#[test]
fn a_real_overlap_uses_the_wall_surface_point_and_the_wall_normal() {
    let field = WallField::build(&flat_plane(60.0), &pole_at(0.0, 0.0));
    // A 0.8 x 0.8 box whose +X edge is at -0.10: it overlaps the post's -X face
    // (x = -0.15) by 5 cm.
    let poly = ConvexPoly2::from_box(
        Vec3::new(-0.50, 0.30, 0.0),
        Vec3::RIGHT,
        Vec3::UP,
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.40, 0.10, 0.10),
    );
    let contacts = query_all(&field, &[("box", poly)], -0.50, 0.0, 0.20);
    let deepest = contacts
        .iter()
        .min_by(|a, b| a.1.gap_m.partial_cmp(&b.1.gap_m).unwrap())
        .expect("overlap must report a contact");
    assert!(
        (deepest.1.gap_m + 0.05).abs() < 0.01,
        "penetration must be the 5 cm overlap, got {}",
        deepest.1.gap_m
    );
    assert!(deepest.1.normal.x < -0.9, "normal must push the box away from the post");
    assert!(
        (deepest.1.point.x + 0.15).abs() < 0.01,
        "contact point must lie on the post face, got {:?}",
        deepest.1.point
    );
}

#[test]
fn contacts_beyond_a_segment_end_use_a_radial_normal() {
    let field = WallField::build(&flat_plane(60.0), &pole_at(0.0, 0.0));
    // A 0.2 m box 0.20 m off the post corner (post corner at 0.15, 0.15; box
    // near corner at 0.25, 0.25). The nearest feature is the segment endpoint.
    let poly = ConvexPoly2::from_box(
        Vec3::new(0.35, 0.30, 0.35),
        Vec3::RIGHT,
        Vec3::UP,
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.10, 0.10, 0.10),
    );
    let contacts = query_all(&field, &[("box", poly)], 0.35, 0.35, 0.20);
    let closest = contacts
        .iter()
        .min_by(|a, b| a.1.gap_m.partial_cmp(&b.1.gap_m).unwrap())
        .expect("a corner within the prediction margin must report a contact");
    assert!(
        (closest.1.gap_m - 0.1414).abs() < 0.01,
        "gap must be the Euclidean corner distance, got {}",
        closest.1.gap_m
    );
    assert!(
        closest.1.normal.x > 0.5 && closest.1.normal.z > 0.5,
        "a corner contact must be radial, got {:?}",
        closest.1.normal
    );
}

#[test]
fn one_sided_wall_does_not_drag_a_body_from_behind_it() {
    // Track is on -X; the wall at X = 0 faces the track.
    let ground = vec![
        DriveableTriangle {
            vertices: [
                Vec3::new(-20.0, 0.0, -10.0),
                Vec3::new(0.0, 0.0, -10.0),
                Vec3::new(0.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
        DriveableTriangle {
            vertices: [
                Vec3::new(-20.0, 0.0, -10.0),
                Vec3::new(0.0, 0.0, 10.0),
                Vec3::new(-20.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
    ];
    let walls = vec![
        ObstacleTriangle {
            vertices: [
                Vec3::new(0.0, 0.0, -10.0),
                Vec3::new(0.0, 2.0, -10.0),
                Vec3::new(0.0, 2.0, 10.0),
            ],
            friction: 0.5,
            restitution: 0.15,
            damage: 1.0,
        },
        ObstacleTriangle {
            vertices: [
                Vec3::new(0.0, 0.0, -10.0),
                Vec3::new(0.0, 2.0, 10.0),
                Vec3::new(0.0, 0.0, 10.0),
            ],
            friction: 0.5,
            restitution: 0.15,
            damage: 1.0,
        },
    ];
    let field = WallField::build(&ground, &walls);
    assert_eq!(field.len(), 1);
    assert!(!field.segments[0].two_sided);
    assert!(field.segments[0].normal[0] < -0.9, "the wall must face the track at -X");

    let behind = |x: f64, half: f64| {
        ConvexPoly2::from_box(
            Vec3::new(x, 0.30, 0.0),
            Vec3::RIGHT,
            Vec3::UP,
            Vec3::new(0.0, 0.0, 1.0),
            Vec3::new(half, 0.15, 0.40),
        )
    };

    // Two metres behind the wall: no contact at all. The old query reported a
    // -2 m gap and dragged the body through the wall.
    let far = query_all(&field, &[("behind", behind(2.0, 0.4))], 2.0, 0.0, 0.50);
    assert!(far.is_empty(), "a body behind a one-sided wall must not be pulled: {far:?}");

    // Half a metre behind (just past the recovery band): no contact either.
    let deep = query_all(&field, &[("deep", behind(0.65, 0.05))], 0.65, 0.0, 0.50);
    assert!(deep.is_empty(), "a body fully behind the recovery band must be ignored: {deep:?}");

    // Straddling the wall: the overlap is real and must be recovered toward the
    // track, never pushed further outside.
    let straddle = query_all(&field, &[("straddle", behind(0.30, 0.40))], 0.30, 0.0, 0.50);
    let contact = straddle
        .iter()
        .min_by(|a, b| a.1.gap_m.partial_cmp(&b.1.gap_m).unwrap())
        .expect("a straddling body must report its overlap");
    assert!(contact.1.gap_m < -0.5, "straddle must report the real overlap, got {}", contact.1.gap_m);
    assert!(contact.1.normal.x < -0.9, "the recovery must push toward the track");
}

#[test]
fn low_wall_does_not_collide_with_a_high_body_box() {
    let ground = vec![
        DriveableTriangle {
            vertices: [
                Vec3::new(-20.0, 0.0, -10.0),
                Vec3::new(0.0, 0.0, -10.0),
                Vec3::new(0.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
        DriveableTriangle {
            vertices: [
                Vec3::new(-20.0, 0.0, -10.0),
                Vec3::new(0.0, 0.0, 10.0),
                Vec3::new(-20.0, 0.0, 10.0),
            ],
            friction: 1.0,
            rolling_resistance: 0.002,
        },
    ];
    // A 10 cm wall, far below the 40 cm body box.
    let walls = vec![
        ObstacleTriangle {
            vertices: [
                Vec3::new(0.0, 0.0, -5.0),
                Vec3::new(0.0, 0.10, -5.0),
                Vec3::new(0.0, 0.10, 5.0),
            ],
            friction: 0.5,
            restitution: 0.15,
            damage: 1.0,
        },
        ObstacleTriangle {
            vertices: [
                Vec3::new(0.0, 0.0, -5.0),
                Vec3::new(0.0, 0.10, 5.0),
                Vec3::new(0.0, 0.0, 5.0),
            ],
            friction: 0.5,
            restitution: 0.15,
            damage: 1.0,
        },
    ];
    let field = WallField::build(&ground, &walls);
    let poly = ConvexPoly2::from_box(
        Vec3::new(-0.5, 0.45, 0.0),
        Vec3::RIGHT,
        Vec3::UP,
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.20, 0.05, 0.50),
    );
    let contacts = query_all(&field, &[("high", poly)], -0.5, 0.0, 0.40);
    assert!(
        contacts.is_empty(),
        "the 0.40 m prediction margin must not lift a 10 cm kerb into a 0.40-0.50 m box: {contacts:?}"
    );
}

#[test]
fn two_sided_normal_follows_the_body_across_a_clear_crossing() {
    let field = WallField::build(&Vec::new(), &pole_at(0.0, 0.0));
    let mut side_mem = WallSideMemory::default();
    let mut scratch = Vec::new();
    let mut contacts = Vec::new();

    let box_at = |x: f64| {
        ConvexPoly2::from_box(
            Vec3::new(x, 0.30, 0.0),
            Vec3::RIGHT,
            Vec3::UP,
            Vec3::new(0.0, 0.0, 1.0),
            Vec3::new(0.10, 0.20, 0.10),
        )
    };

    // 20 cm on +X: the post's +X face (at 0.15) faces the body.
    field.query_poly(&box_at(0.35), 0.20, &mut side_mem, Vec3::new(0.35, 0.30, 0.0), 0.0, &mut scratch, &mut contacts);
    let first = contacts
        .iter()
        .min_by(|a, b| a.gap_m.partial_cmp(&b.gap_m).unwrap())
        .expect("near +X face");
    assert!(first.normal.x > 0.9, "must face the body on +X, got {:?}", first.normal);
    assert!((first.gap_m - 0.10).abs() < 0.01, "gap {}", first.gap_m);

    // 20 cm on -X after crossing: the normal must follow the body, not stay
    // latched on the old side (the old two-second latch made the post attract).
    contacts.clear();
    field.query_poly(&box_at(-0.35), 0.20, &mut side_mem, Vec3::new(-0.35, 0.30, 0.0), 0.01, &mut scratch, &mut contacts);
    let second = contacts
        .iter()
        .min_by(|a, b| a.gap_m.partial_cmp(&b.gap_m).unwrap())
        .expect("near -X face");
    assert!(second.normal.x < -0.9, "must face the body on -X after crossing, got {:?}", second.normal);
    assert!((second.gap_m - 0.10).abs() < 0.01, "gap {}", second.gap_m);
}




#[test]
fn a_wall_end_under_the_body_is_pushed_out_through_the_nearest_body_edge() {
    let field = WallField::build(&flat_plane(60.0), &pole_at(0.0, 0.0));
    // The body covers the post's +X half; the post's +X corners (0.15, 0.15)
    // and (0.15, -0.15) are inside the body, so no body vertex reports them and
    // only the endpoint branch can free them. The nearest body edge is the left
    // one at x = 0.05, so the body must move +X.
    let poly = ConvexPoly2::from_box(
        Vec3::new(0.55, 0.30, 0.15),
        Vec3::RIGHT,
        Vec3::UP,
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.50, 0.10, 0.50),
    );
    let contacts = query_all(&field, &[("body", poly)], 0.55, 0.15, 0.20);
    let (_, corner) = contacts
        .iter()
        .find(|(_, c)| {
            (c.point.x - 0.15).abs() < 0.02
                && (c.point.z - 0.15).abs() < 0.02
                && c.gap_m < -0.05
        })
        .expect("a post corner covered by the body must report a contact");
    assert!(
        corner.normal.x > 0.5,
        "the body must be pushed +X so the covered corner leaves through the nearest edge, got {:?}",
        corner.normal
    );
}

#[test]
fn visual_world_adds_an_invisible_boundary_at_the_ground_edge() {
    let ground = flat_plane(5.0); // 10 x 10 m square, two triangles
    let no_obstacles: Vec<ObstacleTriangle> = Vec::new();
    assert_eq!(WallField::build(&ground, &no_obstacles).len(), 0);
    let field = WallField::build_visual(&ground, &no_obstacles);
    assert_eq!(field.len(), 4, "the square ground must get four edge walls");

    let mut side_mem = WallSideMemory::default();
    let mut scratch = Vec::new();
    let mut contacts = Vec::new();
    let poly = ConvexPoly2::from_box(
        Vec3::new(4.0, 0.5, 0.0),
        Vec3::RIGHT,
        Vec3::UP,
        Vec3::new(0.0, 0.0, 1.0),
        Vec3::new(0.5, 0.5, 0.5),
    );
    field.query_poly(
        &poly,
        0.6,
        &mut side_mem,
        Vec3::new(4.0, 0.5, 0.0),
        0.0,
        &mut scratch,
        &mut contacts,
    );
    let edge = contacts
        .iter()
        .find(|c| c.normal.x < -0.9)
        .expect("the +X ground edge must stop the box from leaving");
    assert!((edge.gap_m - 0.5).abs() < 0.02, "gap {}", edge.gap_m);
}
