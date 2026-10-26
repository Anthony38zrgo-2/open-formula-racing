//! Contact-query resolution: `suspension.contact_ray_count` selects 1 or 3
//! transverse rays per wheel.
//!
//! The production sampler is the only place that casts rays, so the parameter is
//! observable three ways: how many queries reach the collision world per step, what
//! the snapshot publishes, and whether the resting state is the same. These banks pin
//! all three, plus the validation of the authored value.
//!
//! Run with the measurements visible:
//!   cargo test --release -p vehicle_physics_engine --test contact_ray_count -- --nocapture

use std::array;
use std::path::PathBuf;
use std::sync::OnceLock;

use vehicle_physics_engine::open_formula::{install_track_ground_mesh, OpenFormulaSimulation};
use vehicle_physics_engine::open_formula_ffi::OfpControlInput;
use vehicle_physics_engine::suspension::{rest_compression_m, SuspensionSystem};
use vehicle_physics_engine::suspension_setup::adopt_antiroll_design_bar;
use vehicle_physics_engine::types::{
    RaycastHit, SurfaceType, TriRaycastSample, Vec3, WheelIndex,
};
use vehicle_physics_engine::vehicle_config::VehicleConfig;

const HOST_DT: f64 = 0.002;
const GRAVITY: f64 = 9.80665;

fn profile() -> VehicleConfig {
    let mut config = VehicleConfig::williams_fw19_hockenheim_canonical();
    adopt_antiroll_design_bar(&mut config);
    config
}

fn with_rays(config: &VehicleConfig, rays: u8) -> VehicleConfig {
    let mut config = config.clone();
    config.suspension.contact_ray_count = rays;
    config
}

fn plane() {
    static INIT: OnceLock<()> = OnceLock::new();
    INIT.get_or_init(|| {
        let path: PathBuf = std::env::temp_dir().join(format!(
            "ofp-contact-rays-plane-{}.txt",
            std::process::id()
        ));
        std::fs::write(&path, "FORMULA_GROUND_1 2\n1.0 0.001 -1000 -1000 0 1000 -1000 0 1000 1000 0\n1.0 0.001 -1000 -1000 0 1000 1000 0 -1000 1000 0\n").unwrap();
        let (identity, _generation) =
            install_track_ground_mesh(path.to_str().unwrap(), Some("contact-ray-plane"), Some(1))
                .unwrap();
        assert_eq!(identity.triangle_count, 2);
    });
}

fn settle(config: &VehicleConfig, steps: usize) -> OpenFormulaSimulation {
    // `create(None, ..)` always builds the canonical (single-ray) profile, so a
    // modified configuration has to reach the simulator through a document.
    let path: PathBuf = std::env::temp_dir().join(format!(
        "ofp-contact-rays-{}-{}.json",
        config.suspension.contact_ray_count,
        std::process::id()
    ));
    std::fs::write(&path, config.to_resolved_configuration_value().to_string()).unwrap();
    let mut sim = OpenFormulaSimulation::create(
        Some(&path),
        Vec3::new(0.0, vehicle_physics_engine::simulation::default_spawn_height(config), 0.0),
        0.0,
        Some(7.6),
    )
    .expect("simulation");
    let input = OfpControlInput {
        clutch: 1.0,
        gear_request: 0,
        ..Default::default()
    };
    for _ in 0..steps {
        sim.step(&input, &[], HOST_DT).expect("step");
    }
    sim
}

fn total_mass(config: &VehicleConfig) -> f64 {
    config.chassis.total_vehicle_mass(&config.tires, &config.fuel)
}

/// The profile ships with the single-ray mode enabled for this iteration.
#[test]
fn the_profile_selects_one_ray_per_wheel() {
    let config = profile();
    println!(
        "profile contact_ray_count = {}, effective rays = {}",
        config.suspension.contact_ray_count,
        config.suspension.contact_rays()
    );
    assert_eq!(config.suspension.contact_ray_count, 1);
    assert_eq!(config.suspension.contact_rays(), 1);
    assert_eq!(
        with_rays(&config, 3).suspension.contact_rays(),
        3,
        "the field must select the tricast"
    );
}

/// Query counters: four wheel rays per step in single mode, twelve in tricast mode.
#[test]
fn the_snapshot_reports_the_rays_that_were_actually_cast() {
    plane();
    for rays in [1u8, 3u8] {
        let config = with_rays(&profile(), rays);
        let mut sim = settle(&config, 480);
        let input = OfpControlInput {
            clutch: 1.0,
            gear_request: 0,
            ..Default::default()
        };
        sim.step(&input, &[], HOST_DT).expect("step");
        let snapshot = sim.snapshot();
        let expected = if rays == 1 { 4 } else { 12 };
        println!(
            "rays {rays}: queries {} (expected {expected}), hits {}",
            snapshot.wheel_ray_queries, snapshot.wheel_ray_hits
        );
        assert_eq!(
            snapshot.wheel_ray_queries, expected,
            "each wheel must cast exactly {rays} rays"
        );
        assert!(
            snapshot.wheel_ray_hits <= expected,
            "hits cannot exceed queries"
        );
    }
}

/// On a flat plane both resolutions describe the same contact, so the resting pose
/// and the loads must agree: switching the parameter is not a physics change.
#[test]
fn single_ray_and_tricast_settle_to_the_same_rest_state() {
    plane();
    let one = settle(&with_rays(&profile(), 1), 2000);
    let three = settle(&with_rays(&profile(), 3), 2000);
    let loads = |sim: &OpenFormulaSimulation| -> [f64; 4] {
        let snapshot = sim.snapshot();
        array::from_fn(|index| snapshot.wheels[index].normal_force_n)
    };
    let (a, b) = (loads(&one), loads(&three));
    println!(
        "rest loads: single-ray {a:?}, tricast {b:?}; ride y {:.9} vs {:.9}",
        one.pose().0.y,
        three.pose().0.y
    );
    assert!(
        (one.pose().0.y - three.pose().0.y).abs() < 1e-6,
        "the resting height must not depend on the number of rays"
    );
    for index in 0..4 {
        assert!(
            (a[index] - b[index]).abs() < 0.01 * b[index].max(1.0),
            "corner {index}: {:.3} N vs {:.3} N",
            a[index],
            b[index]
        );
    }
    let total: f64 = a.iter().sum();
    let expected = total_mass(&profile()) * GRAVITY;
    println!("single-ray total load {total:.1} N vs W {expected:.1} N");
    assert!((total - expected).abs() < 0.01 * expected);
}

/// The single ray is a full-patch query: the inner/outer slots repeat the centre
/// hit, the contact fraction is one, and telemetry does not claim three hits.
#[test]
fn a_single_ray_is_reported_as_one_hit() {
    let config = profile();
    let mut suspension = SuspensionSystem::new(&config);
    let rest = rest_compression_m(&config, WheelIndex::FrontLeft);
    let spring_length = config.suspension.front_spring_length;
    let radius = config.tires.front_tire_radius;
    let distance = spring_length + radius - rest;
    let hit = RaycastHit {
        is_colliding: true,
        distance,
        point: Vec3::new(0.0, -distance, 0.0),
        normal: Vec3::UP,
        surface: SurfaceType::Road,
        ..Default::default()
    };
    let sample = TriRaycastSample {
        inner: hit,
        center: hit,
        outer: hit,
        context: vehicle_physics_engine::types::WheelRayContext::from_wheel_centre(
            Vec3::ZERO,
            Vec3::RIGHT,
            Vec3::ZERO,
            Vec3::UP,
            rest,
            radius,
            0.09,
        )
        .with_ray_count(1),
    };
    let samples = [sample; 4];
    suspension.step(&config, &samples, HOST_DT);
    let state = suspension.wheels[WheelIndex::FrontLeft as usize];
    println!(
        "single ray: grounded mask {:?}, contact fraction {:.2}, compression {:.5} m",
        state.ray_grounded, state.contact_fraction, state.suspension_compression_m
    );
    assert_eq!(
        state.ray_grounded,
        [false, true, false],
        "only the centre ray was cast, telemetry must say so"
    );
    assert!(
        (state.contact_fraction - 1.0).abs() < 1e-9,
        "one ray covers the whole patch"
    );
    // The same sample declared as a tricast reports three hits.
    let mut tricast = SuspensionSystem::new(&config);
    let mut samples3 = [sample; 4];
    for sample in samples3.iter_mut() {
        sample.context.ray_count = 3;
    }
    tricast.step(&config, &samples3, HOST_DT);
    assert_eq!(
        tricast.wheels[WheelIndex::FrontLeft as usize].ray_grounded,
        [true, true, true]
    );
}

/// Profile contract: the field roundtrips, an absent field keeps the tricast, and any
/// value other than 1 or 3 is rejected at load time.
#[test]
fn the_ray_count_is_part_of_the_profile_contract() {
    let config = profile();
    let json = config.to_resolved_configuration_value().to_string();
    let restored = VehicleConfig::from_json_str(&json).expect("roundtrip");
    assert_eq!(restored.suspension.contact_ray_count, 1);

    let mut three = config.clone();
    three.suspension.contact_ray_count = 3;
    let json = three.to_resolved_configuration_value().to_string();
    let restored = VehicleConfig::from_json_str(&json).expect("roundtrip");
    assert_eq!(restored.suspension.contact_ray_count, 3);

    // Absent field in the legacy document: default tricast.
    let mut value = config.to_resolved_configuration_value();
    value["suspension"]
        .as_object_mut()
        .expect("suspension object")
        .remove("contact_ray_count");
    let restored = VehicleConfig::from_json_str(&value.to_string()).expect("roundtrip");
    assert_eq!(
        restored.suspension.contact_ray_count, 3,
        "absent must keep the historical tricast"
    );
    assert_eq!(restored.suspension.contact_rays(), 3);

    for bad in [0u8, 2, 4, 255] {
        let mut value = config.to_resolved_configuration_value();
        value["suspension"]["contact_ray_count"] = serde_json::json!(bad);
        let error = VehicleConfig::from_json_str(&value.to_string())
            .expect_err("invalid ray count must be rejected");
        assert!(
            error.contains("contact_ray_count"),
            "error must name the field: {error}"
        );
    }
}
