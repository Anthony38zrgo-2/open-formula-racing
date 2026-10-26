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
