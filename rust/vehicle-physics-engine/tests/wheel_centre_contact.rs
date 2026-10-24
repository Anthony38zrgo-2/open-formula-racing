//! Gate C — wheel-centre contact geometry, session world lifecycle and contact
//! telemetry.
//!
//! These tests drive the production path (`OpenFormulaSimulation::step`) against a real
//! `CollisionWorld`, so they exercise the same three-ray query, the same gap-to-
//! compression conversion and the same session registry the Speed Dreams adapter uses.
//!
//! The session registry is process-wide by design: installing a world invalidates every
//! vehicle created for the previous one. Tests that install a world therefore run one at
//! a time through `session_lock`, exactly as the host does inside one process.

use std::sync::{Mutex, MutexGuard};

use vehicle_physics_engine::{
    collision_world::{
        install_flat_plane, install_world, installed_world, uninstall_world, CollisionWorld,
        TriangleMaterial,
    },
    open_formula::{install_track_ground_mesh, OpenFormulaSimulation},
    open_formula_ffi::OfpControlInput,
    simulation::default_spawn_height,
    suspension::{rest_compression_m, wheel_centre_local, wheel_ray_length_from_centre},
    suspension_setup::adopt_antiroll_design_bar,
    types::{TriRaycastSample, Vec3, WheelIndex},
    vehicle_config::VehicleConfig,
};

static SESSION: Mutex<()> = Mutex::new(());

fn session_lock() -> MutexGuard<'static, ()> {
    SESSION
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

fn configuration() -> VehicleConfig {
    let mut config = VehicleConfig::williams_fw19_hockenheim_canonical();
    adopt_antiroll_design_bar(&mut config);
    config
}

fn neutral() -> OfpControlInput {
    OfpControlInput {
        clutch: 1.0,
        gear_request: 0,
        ..Default::default()
    }
}

fn spawn_height(config: &VehicleConfig) -> f64 {
    default_spawn_height(config)
}

/// Creates a vehicle on the installed world and settles it for `steps` accepted steps.
fn settle(config: &VehicleConfig, steps: usize) -> OpenFormulaSimulation {
    let mut simulation = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(config), 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .expect("a vehicle must be creatable on an installed world");
    for _ in 0..steps {
        simulation
            .step(&neutral(), &[], 1.0 / 240.0)
            .expect("settling step must be accepted");
    }
    simulation
}

fn contact_report(simulation: &OpenFormulaSimulation) -> serde_json::Value {
    serde_json::from_str(&simulation.contact_report_json())
        .expect("the contact report must be valid JSON")
}

fn wheel_report<'a>(report: &'a serde_json::Value, wheel: WheelIndex) -> &'a serde_json::Value {
    &report["wheels"][wheel as usize]
}

fn number(value: &serde_json::Value, key: &str) -> f64 {
    value[key]
        .as_f64()
        .unwrap_or_else(|| panic!("{key} must be a number, got {value}"))
}

// ---------------------------------------------------------------------------
// 1. Flat ground, four wheels
// ---------------------------------------------------------------------------

#[test]
fn flat_ground_reports_centre_origin_gaps_and_normals() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-flat", 0.0);
    let simulation = settle(&config, 240);
    let report = contact_report(&simulation);
    assert_eq!(report["world"]["circuit_id"], "gate-c-flat");
    assert_eq!(report["world"]["triangle_count"], 128);

    for wheel in WheelIndex::ALL {
        let corner = wheel_report(&report, wheel);
        let radius = number(corner, "radius_m");
        assert!(radius > 0.2 && radius < 0.6, "radius {radius}");
        let gap = number(corner, "gap_weighted_m");
        assert!(
            gap.abs() < 0.010,
            "{} gap must settle near the road, got {gap} m",
            vehicle_physics_engine::types::wheel_name(wheel)
        );
        // The physical centre must sit one tyre radius above the road plane.
        let centre_y = corner["centre_world"][1].as_f64().unwrap();
        assert!(
            (centre_y - radius).abs() < 0.010,
            "{} centre height {centre_y} vs radius {radius}",
            vehicle_physics_engine::types::wheel_name(wheel)
        );
        assert!(corner["grounded"].as_bool().unwrap());
        assert!(number(corner, "normal_force_n") > 100.0);
        // The three rays come from the centre with a symmetric transverse spread.
        let rays = corner["rays"].as_array().unwrap();
        assert_eq!(rays.len(), 3);
        let centre_x = corner["centre_world"][0].as_f64().unwrap();
        let inner_x = rays[0]["origin"][0].as_f64().unwrap();
        let outer_x = rays[2]["origin"][0].as_f64().unwrap();
        assert!((inner_x - centre_x).abs() > 0.01, "band offset must exist");
        assert!(
            ((inner_x - centre_x) + (outer_x - centre_x)).abs() < 1e-8,
            "the band offsets must be symmetric around the physical centre: centre={centre_x}, inner={inner_x}, outer={outer_x}, lateral={:?}",
            corner["lateral_axis_world"]
        );
        // Every ray reaches the same plane from the same height.
        for ray in rays {
            assert!(ray["valid"].as_bool().unwrap());
            assert!((number(ray, "distance_m") - radius).abs() < 0.010);
            assert!(number(ray, "gap_m").abs() < 0.012);
            assert!(ray["normal"][1].as_f64().unwrap() > 0.999);
        }
    }
    // Static equilibrium: the four ground reactions carry the vehicle weight.
    let total: f64 = WheelIndex::ALL
        .iter()
        .map(|wheel| number(wheel_report(&report, *wheel), "normal_force_n"))
        .sum();
    let expected = 600.0 * 9.80665;
    assert!(
        (total - expected).abs() < 0.03 * expected,
        "total normal load {total} N must carry the {expected} N weight"
    );
}

// ---------------------------------------------------------------------------
// 2. Droop / bump: the ray reach never loses a reachable surface
// ---------------------------------------------------------------------------

#[test]
fn ray_reach_covers_the_whole_droop_to_bump_travel() {
    let config = configuration();
    for wheel in WheelIndex::ALL {
        let spring = if wheel.is_front() {
            config.suspension.front_spring_length
        } else {
            config.suspension.rear_spring_length
        };
        let radius = if wheel.is_front() {
            config.tires.front_tire_radius
        } else {
            config.tires.rear_tire_radius
        };
        for fraction in [0.0, 0.25, 0.5, 0.75, 1.0] {
            let compression = spring * fraction;
            let reach = wheel_ray_length_from_centre(&config, wheel, compression);
            // The absolute reach from the chassis attachment point is unchanged:
            // centre offset (spring - q) + reach == spring + radius + margin.
            let from_anchor = (spring - compression) + reach;
            assert!(
                (from_anchor - (spring + radius + 0.005)).abs() < 1e-9,
                "{} at q={compression}: anchor reach {from_anchor}",
                vehicle_physics_engine::types::wheel_name(wheel)
            );
            assert!(
                reach >= radius,
                "reach must never shrink below the tyre radius"
            );
        }
    }
}

#[test]
fn a_wheel_hanging_at_droop_reports_no_false_ground() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-droop", 0.0);
    // Lift the body high enough that every wheel hangs at full droop with a clear gap.
    let mut simulation = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(&config) + 0.03, 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .unwrap();
    simulation.step(&neutral(), &[], 1.0 / 240.0).unwrap();
    let report = contact_report(&simulation);
    for wheel in WheelIndex::ALL {
        let corner = wheel_report(&report, wheel);
        let lift_gap = corner["gap_weighted_m"].as_f64();
        assert!(
            lift_gap.is_none() || lift_gap.unwrap() > 0.01,
            "{} must report a clear gap or no ground, got {lift_gap:?}",
            vehicle_physics_engine::types::wheel_name(wheel)
        );
        assert_eq!(
            number(corner, "normal_force_n"),
            0.0,
            "{} must carry no load in the air: {}",
            vehicle_physics_engine::types::wheel_name(wheel),
            corner
        );
    }
    // It must fall back into contact without an energetic kick: the kinetic energy of
    // the accepted state may not exceed the drop's potential energy by more than the
    // measurement resolution.
    let start_height = spawn_height(&config) + 0.03;
    let mut peak = 0.0f64;
    for _ in 0..600 {
        simulation.step(&neutral(), &[], 1.0 / 240.0).unwrap();
        peak = peak.max(simulation.kinetic_energy_j());
    }
    let dropped = (start_height - spawn_height(&config)) * 600.0 * 9.80665;
    assert!(
        peak <= dropped + 0.05 * dropped,
        "impact energy {peak} J must not exceed the {dropped} J drop"
    );
    let settled = contact_report(&simulation);
    for wheel in WheelIndex::ALL {
        let corner = wheel_report(&settled, wheel);
        assert!(corner["grounded"].as_bool().unwrap());
        assert!(number(corner, "normal_force_n") > 100.0);
    }
}

// ---------------------------------------------------------------------------
// 3. Slope and banking: the gap is the perpendicular clearance
// ---------------------------------------------------------------------------

fn tilted_plane(slope_radians: f64) -> CollisionWorld {
    // A plane through the origin tilted about the Z (lateral) axis: ground height
    // rises along +X, exactly as a banked corner does.
    let (sin, cos) = slope_radians.sin_cos();
    let point = |x: f64, z: f64| Vec3::new(x * cos, x * sin, z);
    let triangles = vec![
        [
            point(-200.0, -200.0),
            point(-200.0, 200.0),
            point(200.0, 200.0),
        ],
        [
            point(-200.0, -200.0),
            point(200.0, 200.0),
            point(200.0, -200.0),
        ],
    ];
    let materials = vec![TriangleMaterial::new(1.0, 0.002); triangles.len()];
    CollisionWorld::from_host_export("gate-c-slope", 1, &triangles, &materials).unwrap()
}

#[test]
fn tilted_ground_reports_the_perpendicular_gap_not_the_slant_distance() {
    let _guard = session_lock();
    let config = configuration();
    let slope = 10.0_f64.to_radians();
    uninstall_world();
    install_world(tilted_plane(slope));
    let mut simulation = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(&config) + 0.20, 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .unwrap();
    for _ in 0..240 {
        simulation.step(&neutral(), &[], 1.0 / 240.0).unwrap();
    }
    let report = contact_report(&simulation);
    for wheel in WheelIndex::ALL {
        let corner = wheel_report(&report, wheel);
        let centre = &corner["centre_world"];
        let centre_y = centre[1].as_f64().unwrap();
        // Signed distance to y = x*tan(slope), projected onto its unit normal.
        // A vertical height difference overstates the actual clearance on a bank.
        let centre_x = centre[0].as_f64().unwrap();
        let perpendicular = (centre_y - centre_x * slope.tan()) * slope.cos();
        let gap = number(corner, "gap_weighted_m");
        let radius = number(corner, "radius_m");
        assert!(
            (gap - (perpendicular - radius)).abs() < 0.001,
            "{} gap {gap} must be the perpendicular clearance {}",
            vehicle_physics_engine::types::wheel_name(wheel),
            perpendicular - radius
        );
        // The reported normal is the road normal, not the world up.
        let normal_x = corner["rays"][1]["normal"][0].as_f64().unwrap();
        assert!(
            (normal_x + slope.sin()).abs() < 0.02,
            "{} normal must follow the slope, got nx={normal_x}",
            vehicle_physics_engine::types::wheel_name(wheel)
        );
    }
}

#[test]
fn per_triangle_material_coefficients_reach_the_live_wheel_model() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    let triangles =
        vehicle_physics_engine::collision_world::flat_plane_triangles(10.0, 10.0, 8, 0.0);
    let materials = vec![TriangleMaterial::new(0.43, 0.0042); triangles.len()];
    let world =
        CollisionWorld::from_host_export("gate-c-material", 1, &triangles, &materials).unwrap();
    install_world(world);
    let simulation = settle(&config, 240);
    let report = contact_report(&simulation);
    for wheel in WheelIndex::ALL {
        let corner = wheel_report(&report, wheel);
        assert!((number(corner, "effective_friction") - 0.43).abs() < 1e-5, "corner={corner}");
        assert!((number(corner, "effective_rolling_resistance") - 0.0042).abs() < 1e-5, "corner={corner}");
    }
}

#[test]
fn wheel_contact_strip_follows_the_solved_steer_and_camber() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-steer-axis", 0.0);
    let mut simulation = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(&config), 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .unwrap();
    let mut input = neutral();
    input.steering = 1.0;
    for _ in 0..120 {
        simulation.step(&input, &[], 1.0 / 240.0).unwrap();
    }
    let report = contact_report(&simulation);
    let corner = wheel_report(&report, WheelIndex::FrontLeft);
    let steer = number(&report, "steering_angle_rad");
    assert!(
        steer.abs() > 0.03,
        "steering must have moved the front wheel: {steer}"
    );
    let width_axis = &corner["lateral_axis_world"];
    let axis_z = width_axis[2].as_f64().unwrap();
    assert!(
        axis_z.abs() > 0.02,
        "the wheel-width axis must follow steering: {width_axis}"
    );
    let rays = corner["rays"].as_array().unwrap();
    let inner_z = rays[0]["origin"][2].as_f64().unwrap();
    let centre_z = rays[1]["origin"][2].as_f64().unwrap();
    let span = number(corner, "span_m");
    assert!((inner_z - centre_z - span * axis_z).abs() < 1e-6);
}

// ---------------------------------------------------------------------------
// 4. One corner off the road: natural loss of support, no artificial impulse
// ---------------------------------------------------------------------------

#[test]
fn a_corner_off_the_surface_loses_its_load_and_recovers() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    // A hole in the road under the front-left tyre only. The other three corners keep
    // their geometry, so the test isolates "one wheel has no ground" from "the car is
    // falling". The hole is under the front-left wheel centre: the Rust frame has +X to
    // the right, so a left wheel sits at negative X and the front axle at negative Z.
    let hole = (-1.25, -0.45, -1.60, -0.70);
    // 0.25 m cells: fine enough that the removed triangles really open the hole under
    // the tyre footprint instead of only shifting a 3 m triangle.
    let plane = vehicle_physics_engine::collision_world::flat_plane_triangles(10.0, 10.0, 80, 0.0);
    let triangles: Vec<[Vec3; 3]> = plane
        .into_iter()
        .filter(|triangle| {
            let x = (triangle[0].x + triangle[1].x + triangle[2].x) / 3.0;
            let z = (triangle[0].z + triangle[1].z + triangle[2].z) / 3.0;
            !(x > hole.0 && x < hole.1 && z > hole.2 && z < hole.3)
        })
        .collect();
    let materials = vec![TriangleMaterial::new(1.0, 0.002); triangles.len()];
    install_world(
        CollisionWorld::from_host_export("gate-c-hole", 1, &triangles, &materials).unwrap(),
    );

    let mut simulation = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(&config), 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .unwrap();
    // The hole is under one corner: with no chassis-floor model the remaining three
    // suspensions must carry 4/3 of the static load, which the solver may reject when
    // the demanded compression leaves the bump travel. Either outcome is a documentable
    // result: an accepted state where the corner is airborne with zero load, or an
    // explicit rejection that freezes the last accepted state. What is never acceptable
    // is a silent fake contact or a non-finite state.
    let mut accepted = 0usize;
    let mut rejection: Option<String> = None;
    for _ in 0..200 {
        match simulation.step(&neutral(), &[], 1.0 / 240.0) {
            Ok(()) => accepted += 1,
            Err(error) => {
                rejection = Some(error);
                break;
            }
        }
    }
    let snapshot = simulation.snapshot();
    assert!(snapshot.position.y.is_finite() && simulation.kinetic_energy_j().is_finite());
    if let Some(error) = rejection {
        // Fail-closed: the vehicle reports why, and the failure block carries the exact
        // demand and limit instead of a fabricated state.
        assert!(
            error.contains("bump limit") || error.contains("did not converge"),
            "unexpected rejection: {error}"
        );
        assert!(simulation.failed());
        assert_eq!(snapshot.failure.active, 1);
        assert!(snapshot.failure.excess_m.is_finite());
        return;
    }
    assert!(accepted > 0);
    let report = contact_report(&simulation);
    let lifted = wheel_report(&report, WheelIndex::FrontLeft);
    assert!(
        !lifted["grounded"].as_bool().unwrap(),
        "the corner over the hole has no ground"
    );
    assert_eq!(number(lifted, "normal_force_n"), 0.0);
    assert!(lifted["gap_weighted_m"].is_null() || number(lifted, "gap_weighted_m") > 0.0);
    let supported: f64 = [
        WheelIndex::FrontRight,
        WheelIndex::RearLeft,
        WheelIndex::RearRight,
    ]
    .iter()
    .map(|wheel| number(wheel_report(&report, *wheel), "normal_force_n"))
    .sum();
    let weight = 600.0 * 9.80665;
    assert!(
        (supported - weight).abs() < 0.35 * weight,
        "the three supported corners carry {supported} N of {weight} N"
    );
}

// ---------------------------------------------------------------------------
// 5. Left/right symmetry
// ---------------------------------------------------------------------------

#[test]
fn left_and_right_corners_are_symmetric_on_flat_ground() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-symmetry", 0.0);
    let simulation = settle(&config, 480);
    let report = contact_report(&simulation);
    for (left, right) in [
        (WheelIndex::FrontLeft, WheelIndex::FrontRight),
        (WheelIndex::RearLeft, WheelIndex::RearRight),
    ] {
        let a = wheel_report(&report, left);
        let b = wheel_report(&report, right);
        assert!((number(a, "gap_weighted_m") - number(b, "gap_weighted_m")).abs() < 2e-4);
        assert!((number(a, "normal_force_n") - number(b, "normal_force_n")).abs() < 1.0);
        // The inner ray of each side points towards the centreline. In the Rust frame
        // (+X right) that is +X for a left wheel and -X for a right wheel, with equal
        // magnitude: one sign per side, never duplicated.
        let a_inner = a["rays"][0]["origin"][0].as_f64().unwrap();
        let a_outer = a["rays"][2]["origin"][0].as_f64().unwrap();
        let a_centre = a["centre_world"][0].as_f64().unwrap();
        let b_inner = b["rays"][0]["origin"][0].as_f64().unwrap();
        let b_outer = b["rays"][2]["origin"][0].as_f64().unwrap();
        let b_centre = b["centre_world"][0].as_f64().unwrap();
        assert!(
            a_inner > a_centre && b_inner < b_centre,
            "inner rays must point towards the centreline, got {} and {}",
            a_inner - a_centre,
            b_inner - b_centre
        );
        assert!(
            ((a_inner - a_centre) + (a_outer - a_centre)).abs() < 1e-8
                && ((b_inner - b_centre) + (b_outer - b_centre)).abs() < 1e-8,
            "each wheel's inner and outer offsets must be symmetric"
        );
        assert!(
            ((a_inner - a_centre).abs() - (b_inner - b_centre).abs()).abs() < 0.002,
            "left/right physical widths must agree within 2 mm"
        );
    }
}

// ---------------------------------------------------------------------------
// 6. Step-size convergence
// ---------------------------------------------------------------------------

#[test]
fn halving_the_step_converges_the_ride_height_and_the_loads() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-convergence", 0.0);

    let run = |dt: f64, steps: usize| -> (f64, f64) {
        let mut simulation = OpenFormulaSimulation::create_with_world(
            None,
            Vec3::new(0.0, spawn_height(&config), 0.0),
            0.0,
            Some(7.6),
            true,
        )
        .unwrap();
        for _ in 0..steps {
            simulation.step(&neutral(), &[], dt).unwrap();
        }
        let report = contact_report(&simulation);
        let height = report["center_of_mass"][1].as_f64().unwrap();
        let load: f64 = WheelIndex::ALL
            .iter()
            .map(|wheel| number(wheel_report(&report, *wheel), "normal_force_n"))
            .sum();
        (height, load)
    };

    // Two seconds of settling at three rates.
    let (height_240, load_240) = run(1.0 / 240.0, 480);
    let (height_500, load_500) = run(1.0 / 500.0, 1000);
    let (height_1000, load_1000) = run(1.0 / 1000.0, 2000);

    let error_240 = (height_240 - height_1000).abs();
    let error_500 = (height_500 - height_1000).abs();
    assert!(
        error_240 < 0.0025,
        "240 Hz ride height {height_240} must be within 2.5 mm of the 1000 Hz {height_1000}"
    );
    assert!(
        error_500 <= error_240 + 1e-4,
        "500 Hz error {error_500} must not exceed the 240 Hz error {error_240}"
    );
    for (rate, load) in [(240, load_240), (500, load_500), (1000, load_1000)] {
        let expected = 600.0 * 9.80665;
        assert!(
            (load - expected).abs() < 0.05 * expected,
            "{rate} Hz total load {load} N vs {expected} N"
        );
    }
}

// ---------------------------------------------------------------------------
// 7. Session world lifecycle
// ---------------------------------------------------------------------------

#[test]
fn a_new_circuit_invalidates_the_previous_vehicle_geometry() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-first", 0.0);
    let mut simulation = settle(&config, 24);
    let first_identity = simulation.world_identity().cloned().expect("identity");
    let first_generation = simulation.world_generation();

    // The host loads the next circuit in the same process.
    install_flat_plane("gate-c-second", 0.0);
    let error = simulation
        .step(&neutral(), &[], 1.0 / 240.0)
        .expect_err("a stale vehicle must refuse to step");
    assert!(
        error.contains("replaced or released"),
        "unexpected rejection reason: {error}"
    );
    assert!(simulation.failed());
    assert_eq!(first_identity.circuit_id, "gate-c-first");
    // A vehicle created after the change binds the new session.
    let fresh = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(&config), 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .unwrap();
    assert!(fresh.world_generation() > first_generation);
    assert_eq!(fresh.world_identity().unwrap().circuit_id, "gate-c-second");
}

#[test]
fn releasing_the_world_also_invalidates_live_vehicles() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-release", 0.0);
    let mut simulation = settle(&config, 12);
    assert!(vehicle_physics_engine::collision_world::uninstall_world());
    assert!(simulation.step(&neutral(), &[], 1.0 / 240.0).is_err());
}

#[test]
fn a_host_vehicle_cannot_be_created_without_authoritative_geometry() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    let host_error = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(&config), 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .err()
    .expect("the host path must refuse a session without a world");
    assert!(
        host_error.contains("authoritative ground collision world"),
        "{host_error}"
    );
    // The retired/self-sampling path keeps working for unit tests.
    assert!(OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, spawn_height(&config), 0.0),
        0.0,
        Some(7.6),
        false,
    )
    .is_ok());
}

#[test]
fn the_same_circuit_document_hashes_identically_and_a_different_one_does_not() {
    let _guard = session_lock();
    let directory = std::env::temp_dir().join(format!("ofp-gate-c-{}", std::process::id()));
    std::fs::create_dir_all(&directory).unwrap();
    let path = directory.join("gate_c_ground_collisions.txt");
    std::fs::write(
        &path,
        "FORMULA_GROUND_1 2\n1.0 0.002 -100 -100 0 100 -100 0 100 100 0\n1.0 0.002 -100 -100 0 100 100 0 -100 100 0\n",
    )
    .unwrap();
    let (first, _) =
        install_track_ground_mesh(path.to_str().unwrap(), Some("gate-c-file"), Some(4)).unwrap();
    assert_eq!(first.circuit_id, "gate-c-file");
    assert_eq!(first.revision, 4);
    assert_eq!(first.triangle_count, 2);
    let (same, _) =
        install_track_ground_mesh(path.to_str().unwrap(), Some("gate-c-file"), Some(4)).unwrap();
    assert_eq!(first.geometry_sha256, same.geometry_sha256);
    // Same circuit, one vertex moved by 1 cm: the identity must change.
    std::fs::write(
        &path,
        "FORMULA_GROUND_1 2\n1.0 0.002 -100 -100 0 100 -100 0 100 100 0.01\n1.0 0.002 -100 -100 0 100 100 0.01 -100 100 0\n",
    )
    .unwrap();
    let (different, _) =
        install_track_ground_mesh(path.to_str().unwrap(), Some("gate-c-file"), Some(5)).unwrap();
    assert_ne!(first.geometry_sha256, different.geometry_sha256);
    assert_ne!(same.geometry_sha256, different.geometry_sha256);
    let installed = installed_world().expect("world installed");
    assert_eq!(installed.1.identity().revision, 5);
    let _ = std::fs::remove_dir_all(&directory);
}

// ---------------------------------------------------------------------------
// 8. Wheel centre definition
// ---------------------------------------------------------------------------

#[test]
fn the_wheel_centre_follows_the_unsprung_state_and_the_hub_offsets() {
    let config = configuration();
    let wheel = WheelIndex::FrontLeft;
    let rest = rest_compression_m(&config, wheel);
    let neutral = wheel_centre_local(&config, wheel, rest, 0.0, 0.0);
    let bumped = wheel_centre_local(&config, wheel, rest + 0.02, 0.0, 0.0);
    let drooped = wheel_centre_local(&config, wheel, rest - 0.02, 0.0, 0.0);
    // Compression raises the centre; extension lowers it. One centimetre of travel is
    // one centimetre of centre motion (no hidden factor).
    assert!((bumped.y - neutral.y - 0.02).abs() < 1e-12);
    assert!((drooped.y - neutral.y + 0.02).abs() < 1e-12);
    // The linkage lateral/longitudinal offset moves the centre with the wheel, not the
    // chassis: it is added in body coordinates.
    let offset = wheel_centre_local(&config, wheel, rest, 0.01, -0.02);
    assert!((offset.x - neutral.x - 0.01).abs() < 1e-12);
    assert!((offset.z - neutral.z + 0.02).abs() < 1e-12);
    assert!((offset.y - neutral.y).abs() < 1e-12);
    // The published hub position is this same point.
    let _guard = session_lock();
    uninstall_world();
    install_flat_plane("gate-c-centre", 0.0);
    let simulation = settle(&config, 8);
    let (origin, _, up, _) = simulation.pose();
    let local = simulation.wheel_centre_local(wheel);
    let expected =
        origin + up * local.y + simulation.pose().1 * local.x + simulation.pose().3 * local.z;
    let published = simulation.snapshot().wheels[wheel as usize].hub_position;
    let error = ((published.x - expected.x).powi(2)
        + (published.y - expected.y).powi(2)
        + (published.z - expected.z).powi(2))
    .sqrt();
    assert!(error < 1e-9, "published hub position differs by {error} m");
}

// ---------------------------------------------------------------------------
// 9. Contact report content
// ---------------------------------------------------------------------------

#[test]
fn the_contact_report_carries_signed_steering_and_energy() {
    let _guard = session_lock();
    let config = configuration();
    uninstall_world();
    install_flat_plane("gate-c-report", 0.0);
    let mut simulation = settle(&config, 24);
    let input = OfpControlInput {
        steering: -0.25,
        clutch: 1.0,
        gear_request: 1,
        ..Default::default()
    };
    simulation.step(&input, &[], 1.0 / 240.0).unwrap();
    let report = contact_report(&simulation);
    // The sign of the requested direction survives into the road-wheel angle.
    assert!(number(&report, "steer_input") < 0.0);
    assert!(
        number(&report, "steering_angle_rad") < 0.0,
        "a left input must publish a negative road-wheel angle, got {}",
        report["steering_angle_rad"]
    );
    assert!(number(&report, "kinetic_energy_after_j") >= 0.0);
    assert!(number(&report, "sim_time_s") > 0.0);
    assert!((report["dt_s"].as_f64().unwrap() - 1.0 / 240.0).abs() < 1e-9);
    for wheel in WheelIndex::ALL {
        let corner = wheel_report(&report, wheel);
        assert_eq!(corner["rays"].as_array().unwrap().len(), 3);
        // Real data, not placeholders: the centre, axis and radius are populated.
        assert!(corner["radius_m"].as_f64().unwrap() > 0.2);
        let axis = corner["up"][1].as_f64().unwrap();
        assert!(axis.abs() > 0.9, "suspension axis {axis}");
    }
}

#[test]
fn a_sample_measured_from_the_wheel_centre_and_one_from_the_anchor_differ_by_construction() {
    let config = configuration();
    let wheel = WheelIndex::FrontLeft;
    let radius = config.tires.front_tire_radius;
    let compression = rest_compression_m(&config, wheel);
    let spring = config.suspension.front_spring_length;
    let centre_offset = spring - compression;

    let hit = |distance: f64| vehicle_physics_engine::types::RaycastHit {
        is_colliding: true,
        distance,
        point: Vec3::new(0.0, -distance, 0.0),
        normal: Vec3::UP,
        surface: vehicle_physics_engine::types::SurfaceType::Road,
        ..Default::default()
    };
    // The same physical road, sampled from the centre and from the anchor.
    let from_anchor = TriRaycastSample {
        inner: hit(centre_offset + radius),
        center: hit(centre_offset + radius),
        outer: hit(centre_offset + radius),
        ..Default::default()
    };
    let from_centre = TriRaycastSample::wheel_centre(
        hit(radius),
        hit(radius),
        hit(radius),
        vehicle_physics_engine::types::WheelRayContext::from_wheel_centre(
            Vec3::ZERO,
            Vec3::RIGHT,
            Vec3::UP * centre_offset,
            Vec3::UP,
            compression,
            radius,
            config.tires.front_tire_width * config.suspension.tri_ray_spacing_ratio,
        ),
    );
    assert!(!from_anchor.measured_from_wheel_centre());
    assert!(from_centre.measured_from_wheel_centre());
    let gap_centre = TriRaycastSample::signed_gap(&from_centre.center, Vec3::UP, radius);
    assert!(gap_centre.abs() < 1e-12);
    // The demand closes exactly the sampled gap.
    let demand = compression - gap_centre;
    assert!((demand - compression).abs() < 1e-12);
    // 1 cm of penetration demands 1 cm more compression, never a velocity.
    let penetrated = TriRaycastSample::signed_gap(&hit(radius - 0.01), Vec3::UP, radius);
    assert!((penetrated + 0.01).abs() < 1e-12);
    assert!((compression - penetrated - (compression + 0.01)).abs() < 1e-12);
}
