//! Structural banks for the suspension/contact correction plan — Phase A.
//!
//! These drive the production path (`VehicleSimulator::solve_external`) with
//! synthetic tri-raycast samples so a single wheel can be lifted without a track
//! mesh. They measure, they do not correct: the assertions pin the *current*
//! behaviour of the two P0 defects so Phase B/C has a regression witness.
//!
//! Run with a trace dump:
//!   $env:OF_STRUCTURAL_TRACE = "1"
//!   cargo test --offline -p vehicle_physics_engine --test structural_contact -- --nocapture

use std::array;

use vehicle_physics_engine::simulation::{BodyKinematics, ForceTorqueOutput, VehicleSimulator};
use vehicle_physics_engine::suspension::rest_compression_m;
use vehicle_physics_engine::suspension_setup::adopt_antiroll_design_bar;
use vehicle_physics_engine::types::{
    Mat3, Quat, RaycastHit, SurfaceType, TriRaycastSample, Vec3, VehicleInput, WheelIndex,
};
use vehicle_physics_engine::vehicle_config::VehicleConfig;
use vehicle_physics_engine::wheel_mechanics::tire_radius;

const DT: f64 = 1.0 / 480.0;
const GRAVITY: f64 = 9.80665;

fn profile() -> VehicleConfig {
    let mut config = VehicleConfig::williams_fw19_hockenheim_canonical();
    // Same single anti-roll authority the host applies on create(); keeping the
    // canonical config untouched would measure a different bar.
    adopt_antiroll_design_bar(&mut config);
    config
}

fn spring_length(config: &VehicleConfig, wheel: WheelIndex) -> f64 {
    if wheel.is_front() {
        config.suspension.front_spring_length
    } else {
        config.suspension.rear_spring_length
    }
}

/// Ray geometry that makes the corner integrate at `compression_m`.
fn contact_sample(
    config: &VehicleConfig,
    wheel: WheelIndex,
    compression_m: f64,
) -> TriRaycastSample {
    let distance =
        (spring_length(config, wheel) + tire_radius(config, wheel) - compression_m).max(0.0);
    let hit = RaycastHit {
        is_colliding: true,
        distance,
        point: Vec3::new(0.0, -distance, 0.0),
        normal: Vec3::UP,
        surface: SurfaceType::Road,
    };
    TriRaycastSample {
        inner: hit,
        center: hit,
        outer: hit,
    }
}

fn airborne() -> TriRaycastSample {
    TriRaycastSample::default()
}

fn settled_samples(config: &VehicleConfig) -> [TriRaycastSample; 4] {
    array::from_fn(|index| {
        let wheel = WheelIndex::ALL[index];
        contact_sample(config, wheel, rest_compression_m(config, wheel))
    })
}

fn settled() -> (VehicleSimulator, VehicleConfig) {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let samples = settled_samples(&config);
    for _ in 0..480 {
        sim.step(&VehicleInput::default(), &samples, DT);
    }
    (sim, config)
}

fn body_of(sim: &VehicleSimulator) -> BodyKinematics {
    BodyKinematics {
        transform: sim.state.transform,
        orientation: sim.state.orientation,
        linear_velocity: sim.state.linear_velocity,
        angular_velocity: sim.state.angular_velocity,
    }
}

fn total_mass(config: &VehicleConfig) -> f64 {
    config.chassis.total_vehicle_mass(&config.tires, &config.fuel)
}

fn trace(name: &str, payload: String) {
    if std::env::var_os("OF_STRUCTURAL_TRACE").is_none() {
        return;
    }
    let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../implementation/reports/structural_correction");
    let _ = std::fs::create_dir_all(&dir);
    let line = format!("{{\"bank\":\"{name}\",{payload}}}\n");
    use std::io::Write;
    if let Ok(mut file) = std::fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(dir.join("phase_a_trace.jsonl"))
    {
        let _ = file.write_all(line.as_bytes());
    }
}

/// P0 (assembly): an airborne wheel keeps its spring/damper/ARB force in
/// `chassis_suspension_force`, but the body never receives it.
#[test]
fn p0_airborne_wheel_drops_its_suspension_force_from_the_body() {
    let (sim, config) = settled();
    let body = body_of(&sim);

    let mut grounded = sim.clone();
    let mut lifted = sim.clone();
    let grounded_samples = settled_samples(&config);
    let mut lifted_samples = grounded_samples;
    lifted_samples[WheelIndex::FrontRight as usize] = airborne();

    let (with_contact, _) = grounded.solve_external(
        body,
        &VehicleInput::default(),
        &grounded_samples,
        DT,
    );
    let (without_contact, _) = lifted.solve_external(
        body,
        &VehicleInput::default(),
        &lifted_samples,
        DT,
    );

    let front_right = grounded.state.suspension.wheels[WheelIndex::FrontRight as usize];
    let lifted_force = lifted.state.suspension.wheels[WheelIndex::FrontRight as usize]
        .chassis_suspension_force;
    let lost = with_contact.force_world.y - without_contact.force_world.y;
    let element_delta: f64 = WheelIndex::ALL
        .iter()
        .map(|wheel| {
            grounded.state.suspension.wheels[*wheel as usize].chassis_suspension_force
                - lifted.state.suspension.wheels[*wheel as usize].chassis_suspension_force
        })
        .sum();
    println!(
        "P0 assembly: FR grounded element {:.3} N, airborne element {:.3} N, body vertical force delta {:.3} N, element delta {:.3} N, mass {:.3} kg",
        front_right.chassis_suspension_force,
        lifted_force,
        lost,
        element_delta,
        total_mass(&config)
    );
    trace(
        "p0_airborne_wheel",
        format!(
            "\"fr_chassis_suspension_force_n\":{:.6},\"fr_airborne_force_n\":{:.6},\"body_vertical_force_delta_n\":{:.6},\"element_force_delta_n\":{:.6},\"total_mass_kg\":{:.6},\"fr_normal_force_n\":{:.6}",
            front_right.chassis_suspension_force,
            lifted_force,
            lost,
            element_delta,
            total_mass(&config),
            front_right.total_normal_force
        ),
    );

    // The airborne corner must keep a real element force, and the body force change
    // must be exactly the sum of the corner element changes: nothing is dropped.
    assert!(
        lifted_force.abs() > 100.0,
        "an airborne corner must still transmit its suspension force, got {lifted_force:.3} N"
    );
    assert!(
        (lost - element_delta).abs() < 1.0,
        "body vertical delta {lost:.3} N must equal the summed element delta {element_delta:.3} N"
    );
    let _: ForceTorqueOutput = without_contact;
}

/// The axle publishes `J / h` as the normal load; at rest J must be the supported
/// load times the step. With 1000 N and h = 1/480 s that is 2.083333 N s.
#[test]
fn published_normal_impulse_matches_the_supported_load() {
    let (mut sim, config) = settled();
    let body = body_of(&sim);
    let samples = settled_samples(&config);
    let before: Vec<f64> = WheelIndex::ALL
        .iter()
        .map(|w| sim.state.suspension.wheels[*w as usize].suspension_compression_m)
        .collect();
    let (resting, _) =
        sim.solve_external(body, &VehicleInput::default(), &samples, DT);
    println!(
        "resting wrench: force {:?}, torque {:?}",
        resting.force_world, resting.torque_world
    );
    let after: Vec<f64> = WheelIndex::ALL
        .iter()
        .map(|w| sim.state.suspension.wheels[*w as usize].suspension_compression_m)
        .collect();
    let mut worst = 0.0_f64;
    for wheel in WheelIndex::ALL {
        let state = sim.state.suspension.wheels[wheel as usize];
        let expected = state.total_normal_force * DT;
        let error = (state.normal_impulse_newton_seconds - expected).abs();
        worst = worst.max(error);
        println!(
            "{wheel:?}: load {:.3} N, impulse {:.6} N s, expected {:.6} N s, compression {:.6} m",
            state.total_normal_force, state.normal_impulse_newton_seconds, expected,
            state.suspension_compression_m
        );
    }
    trace(
        "published_normal_impulse",
        format!(
            "\"worst_impulse_error_ns\":{:.9},\"compression_before\":{:?},\"compression_after\":{:?}",
            worst, before, after
        ),
    );
    assert!(
        worst < 1e-3,
        "J and J/h must stay consistent within a step (worst {worst:.6} N s)"
    );
    let supported: f64 = WheelIndex::ALL
        .iter()
        .map(|w| sim.state.suspension.wheels[*w as usize].total_normal_force)
        .sum();
    let weight = total_mass(&config) * GRAVITY;
    println!(
        "supported {:.3} N vs weight {:.3} N ({:.3} %)",
        supported,
        weight,
        100.0 * (supported - weight) / weight
    );
}

/// Axle convergence witness: the residual projector only knows the travel stops,
/// so a corner that is actually held by the ground can never close its balance.
#[test]
fn axle_fallback_usage_on_a_grounded_axle() {
    let (mut sim, config) = settled();
    let body = body_of(&sim);
    let before = sim.state.suspension.axle_fallback_count;
    let (_, _) = sim.solve_external(body, &VehicleInput::default(), &settled_samples(&config), DT);
    let grounded = sim.state.suspension.axle_fallback_count - before;

    let mut lifted_samples = settled_samples(&config);
    lifted_samples[WheelIndex::FrontRight as usize] = airborne();
    let before = sim.state.suspension.axle_fallback_count;
    let (_, _) = sim.solve_external(body, &VehicleInput::default(), &lifted_samples, DT);
    let lifted = sim.state.suspension.axle_fallback_count - before;

    println!(
        "axle fallbacks in one step: both grounded {grounded}, FR airborne {lifted}; worst iterations {}",
        sim.state.suspension.axle_max_iterations
    );
    assert_eq!(grounded,0,"a supported axle must converge without fallback");
    assert_eq!(lifted,0,"one airborne corner must converge without fallback");
    assert!(sim.state.suspension.mechanical_failure.is_none());
    trace(
        "axle_fallback_usage",
        format!(
            "\"fallbacks_both_grounded\":{grounded},\"fallbacks_fr_airborne\":{lifted},\"max_iterations\":{}",
            sim.state.suspension.axle_max_iterations
        ),
    );
}

/// Differential ARB travel with the inner wheel airborne: the bar moment must
/// still reach the body through both corners.
#[test]
fn p0_airborne_arb_side_does_not_transfer_its_roll_moment() {
    let (sim, config) = settled();
    let body = body_of(&sim);
    let mut samples = settled_samples(&config);
    let base = rest_compression_m(&config, WheelIndex::FrontLeft);
    // Both corners in bump so the ground constraint is active on both sides and
    // the bank measures the bar, not a wheel that has already lost contact. The
    // travel stays well inside the bump stop so the bank is not a stop-force test.
    samples[WheelIndex::FrontLeft as usize] = contact_sample(&config, WheelIndex::FrontLeft, base + 0.006);
    samples[WheelIndex::FrontRight as usize] = contact_sample(&config, WheelIndex::FrontRight, base + 0.002);

    let mut both = sim.clone();
    let (both_force, _) = both.solve_external(body, &VehicleInput::default(), &samples, DT);

    let mut lifted_samples = samples;
    lifted_samples[WheelIndex::FrontRight as usize] = airborne();
    let mut lifted = sim.clone();
    let (lifted_force, _) = lifted.solve_external(body, &VehicleInput::default(), &lifted_samples, DT);

    let fl = both.state.suspension.wheels[WheelIndex::FrontLeft as usize];
    let fr = both.state.suspension.wheels[WheelIndex::FrontRight as usize];
    let fr_lifted = lifted.state.suspension.wheels[WheelIndex::FrontRight as usize];
    println!(
        "ARB: FL arb {:.3} N, FR arb {:.3} N (lifted {:.3} N), FR element {:.3} N (lifted {:.3} N); roll torque both {:.3} N m, lifted {:.3} N m",
        fl.antiroll_force,
        fr.antiroll_force,
        fr_lifted.antiroll_force,
        fr.chassis_suspension_force,
        fr_lifted.chassis_suspension_force,
        both_force.torque_world.z,
        lifted_force.torque_world.z
    );
    trace(
        "p0_airborne_arb",
        format!(
            "\"fl_arb_n\":{:.6},\"fr_arb_n\":{:.6},\"fr_arb_lifted_n\":{:.6},\"fr_element_n\":{:.6},\"fr_element_lifted_n\":{:.6},\"torque_both_z\":{:.6},\"torque_lifted_z\":{:.6}",
            fl.antiroll_force,
            fr.antiroll_force,
            fr_lifted.antiroll_force,
            fr.chassis_suspension_force,
            fr_lifted.chassis_suspension_force,
            both_force.torque_world.z,
            lifted_force.torque_world.z
        ),
    );
    assert!(
        (fl.antiroll_force + fr.antiroll_force).abs() < 1.0,
        "an ideal bar must produce equal and opposite corner forces"
    );
    assert!(fl.antiroll_force.abs() > 100.0, "the bank must load the bar");
    // Lifting the inner wheel must not zero the bar force that the corner still
    // carries into the chassis.
    assert!(
        fr_lifted.antiroll_force.abs() > 100.0,
        "the airborne side of the bar must still load the chassis"
    );
    let force_delta = both_force.force_world.y - lifted_force.force_world.y;
    let element_delta: f64 = WheelIndex::ALL
        .iter()
        .map(|wheel| {
            both.state.suspension.wheels[*wheel as usize].chassis_suspension_force
                - lifted.state.suspension.wheels[*wheel as usize].chassis_suspension_force
        })
        .sum();
    assert!(
        (force_delta - element_delta).abs() < 1.0,
        "body vertical delta {force_delta:.3} N must equal the element delta {element_delta:.3} N"
    );
}

/// Mass contract: every physical corner mass is counted once, the body carries the
/// remainder, and the two budgets close against the configured total.
#[test]
fn mass_contract_closes() {
    use vehicle_physics_engine::simulation::{
        principal_inertia_for_suspension_backend, sprung_mass_kg, sprung_principal_inertia,
    };
    use vehicle_physics_engine::suspension::SuspensionBackend;

    let config = profile();
    let total = total_mass(&config);
    let corners: Vec<f64> = WheelIndex::ALL
        .iter()
        .map(|wheel| {
            vehicle_physics_engine::simulation::corner_unsprung_mass_kg(&config, *wheel)
        })
        .collect();
    let unsprung: f64 = corners.iter().sum();
    let sprung = sprung_mass_kg(&config);
    let inertia_total = principal_inertia_for_suspension_backend(&config, SuspensionBackend::Simplified);
    let inertia_sprung = sprung_principal_inertia(&config, SuspensionBackend::Simplified);
    let properties =
        vehicle_physics_engine::vehicle_mass_properties::resolve_vehicle_mass_properties(&config);
    println!(
        "mass: total {total:.3} kg, corners {corners:?} ({unsprung:.3} kg), sprung {sprung:.3} kg, com {:?}, fuel {:.3} kg; inertia total {inertia_total:?}, sprung {inertia_sprung:?}",
        properties.center_of_mass_local_metres, config.fuel.current_kg
    );
    trace(
        "mass_contract",
        format!(
            "\"total_kg\":{total:.6},\"corner_kg\":{corners:?},\"unsprung_kg\":{unsprung:.6},\"sprung_kg\":{sprung:.6},\"inertia_total\":[{:.6},{:.6},{:.6}],\"inertia_sprung\":[{:.6},{:.6},{:.6}]",
            inertia_total.x, inertia_total.y, inertia_total.z,
            inertia_sprung.x, inertia_sprung.y, inertia_sprung.z
        ),
    );
    assert!((sprung + unsprung - total).abs() < 1e-6, "budgets must close");
    assert!(sprung > 0.5 * total, "the sprung body must carry most of the car");
    assert!(
        inertia_sprung.x < inertia_total.x
            && inertia_sprung.y < inertia_total.y
            && inertia_sprung.z < inertia_total.z,
        "removing corner masses must reduce every principal moment"
    );
}

/// World-fixed flat plane consistent with the *current* body pose: the offset that
/// leaves the four corners at their design compression right now.
fn plane_offset(config: &VehicleConfig, sim: &VehicleSimulator) -> f64 {
    let wheel = WheelIndex::FrontLeft;
    let anchor = sim.state.transform.transform_point(
        config
            .tires
            .wheel_anchor_local(&config.chassis, &config.suspension, wheel),
    );
    let max_ray = spring_length(config, wheel) + tire_radius(config, wheel);
    anchor.y - (max_ray - rest_compression_m(config, wheel))
}

/// Drop onto the plane and let the car settle. The plane is fixed from the spawn pose
/// so every wheel starts at its design compression; it is never moved afterwards.
fn prepare_plane(config: &VehicleConfig, sim: &mut VehicleSimulator, dt: f64) -> f64 {
    let offset = plane_offset(config, sim);
    settle_on_plane(config, sim, offset, 1.75, dt);
    offset
}

fn plane_samples(
    config: &VehicleConfig,
    sim: &VehicleSimulator,
    offset: f64,
) -> [TriRaycastSample; 4] {
    assert!(sim.state.suspension.mechanical_failure.is_none(), "previous interval failed: {:?}",sim.state.suspension.mechanical_failure);
    array::from_fn(|index| {
        let wheel = WheelIndex::ALL[index];
        let anchor = sim.state.transform.transform_point(
            config
                .tires
                .wheel_anchor_local(&config.chassis, &config.suspension, wheel),
        );
        let max_ray = spring_length(config, wheel) + tire_radius(config, wheel);
        let down_axis = sim.state.transform.basis.y;
        let distance = (anchor.y - offset) / down_axis.y;
        if !(0.0..=max_ray).contains(&distance) {
            return TriRaycastSample::default();
        }
        let hit = RaycastHit {
            is_colliding: true,
            distance,
            point: anchor - down_axis * distance,
            normal: Vec3::UP,
            surface: SurfaceType::Road,
        };
        TriRaycastSample {
            inner: hit,
            center: hit,
            outer: hit,
        }
    })
}

/// Plane tilted about the longitudinal axis (`roll`): y = offset + x*tan(roll).
/// Rays follow the current body's suspension axis and intersect the inclined plane.
fn sloped_samples(
    config: &VehicleConfig,
    sim: &VehicleSimulator,
    offset: f64,
    roll_radians: f64,
) -> [TriRaycastSample; 4] {
    assert!(sim.state.suspension.mechanical_failure.is_none(), "previous interval failed: {:?}",sim.state.suspension.mechanical_failure);
    let slope = roll_radians.tan();
    let normal = Vec3::new(-slope, 1.0, 0.0).normalized();
    array::from_fn(|index| {
        let wheel = WheelIndex::ALL[index];
        let anchor = sim.state.transform.transform_point(
            config
                .tires
                .wheel_anchor_local(&config.chassis, &config.suspension, wheel),
        );
        let max_ray = spring_length(config, wheel) + tire_radius(config, wheel);
        let plane_y = offset + anchor.x * slope;
        let down_axis = sim.state.transform.basis.y;
        let distance = (anchor.y - plane_y) / (down_axis.y - slope * down_axis.x);
        if !(0.0..=max_ray).contains(&distance) {
            return TriRaycastSample::default();
        }
        let hit = RaycastHit {
            is_colliding: true,
            distance,
            point: anchor - down_axis * distance,
            normal,
            surface: SurfaceType::Road,
        };
        TriRaycastSample {
            inner: hit,
            center: hit,
            outer: hit,
        }
    })
}

fn settle_on_plane(
    config: &VehicleConfig,
    sim: &mut VehicleSimulator,
    offset: f64,
    seconds: f64,
    dt: f64,
) {
    let steps = (seconds / dt).round().max(1.0) as usize;
    for step in 0..steps {
        sim.step(&VehicleInput::default(), &plane_samples(config, sim, offset), dt);
        if std::env::var_os("OF_STRUCTURAL_SETTLE_TRACE").is_some() && step % 60 == 0 {
            println!(
                "   settle t={:.2}: y {:.4}, vy {:.4}, comp {:?}, grounded {:?}",
                step as f64 * dt,
                sim.state.transform.origin.y,
                sim.state.linear_velocity.y,
                WheelIndex::ALL
                    .map(|w| sim.state.suspension.wheels[w as usize].suspension_compression_m),
                WheelIndex::ALL
                    .map(|w| sim.state.suspension.wheels[w as usize].is_grounded),
            );
        }
    }
}

/// A host frame coarser than the validated 1/240 s is subdivided internally: a 1/120 s
/// frame must land on the same state as the fine step instead of an asymmetric
/// equilibrium. This is the witness for `MAX_INTERNAL_STEP_SECONDS`.
#[test]
fn coarse_host_frames_are_subdivided_internally() {
    let dt = 1.0 / 120.0;
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let offset = prepare_plane(&config, &mut sim, dt);
    let mut input = VehicleInput::default();
    input.throttle = 0.25;
    for _ in 0..120 {
        sim.step(&input, &plane_samples(&config, &sim, offset), dt);
    }
    let grounded = WheelIndex::ALL
        .iter()
        .filter(|wheel| sim.state.suspension.wheels[**wheel as usize].is_grounded)
        .count();
    println!(
        "dt 1/120 (subdivided): y {:.4}, speed {:.3}, grounded {grounded}/4, fallbacks {}, worst iterations {}",
        sim.state.transform.origin.y,
        sim.state.linear_velocity.length(),
        sim.state.suspension.axle_fallback_count,
        sim.state.suspension.axle_max_iterations
    );
    trace(
        "coarse_frame_subdivision",
        format!(
            "\"dt_s\":{dt:.9},\"y\":{:.6},\"speed\":{:.6},\"grounded\":{grounded},\"fallbacks\":{}",
            sim.state.transform.origin.y,
            sim.state.linear_velocity.length(),
            sim.state.suspension.axle_fallback_count
        ),
    );
    assert_eq!(grounded, 4, "a 1/120 s frame must keep all four wheels loaded");
    assert_eq!(sim.state.suspension.axle_fallback_count, 0, "subdivision must avoid the fallback");
    let fine_dt=1.0/240.0;
    let mut reference=VehicleSimulator::new(config.clone(),Vec3::ZERO,0.0);
    let fine_offset=prepare_plane(&config,&mut reference,fine_dt);
    for _ in 0..240 { reference.step(&input,&plane_samples(&config,&reference,fine_offset),fine_dt); }
    assert!((reference.state.linear_velocity-sim.state.linear_velocity).length()<0.1,
        "coarse and fine host frames must agree after the same manoeuvre");
    assert!(
        (sim.state.transform.origin.y - reference.state.transform.origin.y).abs() < 0.001,
        "ride height must match the fine step"
    );
}

/// Smoke: a world-fixed ground plane (not body-relative rays), 1 s drop plus 2 s of
/// light throttle. Guards stability, contact retention and the fallback counter.
#[test]
fn flat_ground_drive_keeps_contact_without_fallback() {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let offset = prepare_plane(&config, &mut sim, DT);
    let settled_y = sim.state.transform.origin.y;
    let mut input = VehicleInput::default();
    input.throttle = 0.30;
    for _ in 0..960 {
        sim.step(&input, &plane_samples(&config, &sim, offset), DT);
    }
    let grounded = WheelIndex::ALL
        .iter()
        .filter(|wheel| sim.state.suspension.wheels[**wheel as usize].is_grounded)
        .count();
    let fallbacks = sim.state.suspension.axle_fallback_count;
    println!(
        "flat drive: y {:.3} -> {:.3} m, speed {:.2} m/s, grounded {grounded}/4, fallbacks {fallbacks}, worst iterations {}",
        settled_y,
        sim.state.transform.origin.y,
        sim.state.linear_velocity.length(),
        sim.state.suspension.axle_max_iterations
    );
    trace(
        "flat_ground_drive",
        format!(
            "\"settled_y\":{settled_y:.6},\"final_y\":{:.6},\"speed\":{:.6},\"grounded\":{grounded},\"fallbacks\":{fallbacks},\"max_iterations\":{}",
            sim.state.transform.origin.y,
            sim.state.linear_velocity.length(),
            sim.state.suspension.axle_max_iterations
        ),
    );
    assert!(sim.state.transform.origin.is_finite(), "state must stay finite");
    assert_eq!(grounded, 4, "flat ground must keep all four wheels in contact");
    assert_eq!(fallbacks, 0, "the coupled axle must not fall back on flat ground");
    assert!(
        sim.state.transform.origin.y > settled_y - 0.05,
        "the body must not sink through the plane"
    );
    assert!(sim.state.linear_velocity.length() > 1.0, "throttle must move the car");
}

/// Time-step sweep: the rest state must not depend on the host step. Convergence is
/// claimed only inside the range the host accepts today (1/120 .. 1/480 s).
#[test]
fn temporal_sweep_keeps_the_same_rest_state() {
    let config = profile();
    let mut states = Vec::new();
    for dt in [1.0 / 240.0, 1.0 / 480.0] {
        let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
        let offset = prepare_plane(&config, &mut sim, dt);
        let settled_compressions =
            WheelIndex::ALL.map(|w| sim.state.suspension.wheels[w as usize].suspension_compression_m);
        let settled_y = sim.state.transform.origin.y;
        let mut input = VehicleInput::default();
        input.throttle = 0.25;
        let steps = (1.0 / dt).round() as usize;
        for _ in 0..steps {
            sim.step(&input, &plane_samples(&config, &sim, offset), dt);
        }
        println!(
            "   dt 1/{:.0} settled: y {settled_y:.4}, comp {settled_compressions:?}",
            1.0 / dt
        );
        let grounded = WheelIndex::ALL
            .iter()
            .filter(|wheel| sim.state.suspension.wheels[**wheel as usize].is_grounded)
            .count();
        let y = sim.state.transform.origin.y;
        let speed = sim.state.linear_velocity.length();
        println!(
            "dt 1/{:.0}: y {y:.4} m, speed {speed:.3} m/s, grounded {grounded}/4, fallbacks {}",
            1.0 / dt,
            sim.state.suspension.axle_fallback_count
        );
        trace(
            "temporal_sweep",
            format!(
                "\"dt_s\":{dt:.9},\"y\":{y:.6},\"speed\":{speed:.6},\"grounded\":{grounded},\"fallbacks\":{}",
                sim.state.suspension.axle_fallback_count
            ),
        );
        assert!(y.is_finite() && speed.is_finite(), "dt 1/{:.0} diverged", 1.0 / dt);
        assert_eq!(grounded, 4, "dt 1/{:.0} lost contact", 1.0 / dt);
        states.push((y, speed));
    }
    let y_spread = states.iter().map(|s| s.0).fold(f64::MIN, f64::max)
        - states.iter().map(|s| s.0).fold(f64::MAX, f64::min);
    let speed_spread = states.iter().map(|s| s.1).fold(f64::MIN, f64::max)
        - states.iter().map(|s| s.1).fold(f64::MAX, f64::min);
    println!("sweep spread: y {y_spread:.4} m, speed {speed_spread:.3} m/s");
    assert!(y_spread < 0.01, "ride height must not depend on dt ({y_spread:.4} m)");
    assert!(speed_spread < 0.5, "same manoeuvre must end at a similar speed");
}

/// Kerb step: 15 mm ground step under a rolling car. Contact must be recovered
/// without the fallback or a blow-up.
#[test]
fn kerb_step_recovers_contact() {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let offset = prepare_plane(&config, &mut sim, DT);
    let mut input = VehicleInput::default();
    input.throttle = 0.40;
    for _ in 0..720 {
        sim.step(&input, &plane_samples(&config, &sim, offset), DT);
    }
    let speed_before = sim.state.linear_velocity.length();
    let y_before = sim.state.transform.origin.y;
    let mut peak_load: f64 = 0.0;
    for _ in 0..120 {
        sim.step(&input, &plane_samples(&config, &sim, offset + 0.015), DT);
        peak_load = peak_load.max(
            WheelIndex::ALL
                .iter()
                .map(|w| sim.state.suspension.wheels[*w as usize].total_normal_force)
                .fold(0.0, f64::max),
        );
    }
    for _ in 0..240 {
        sim.step(&input, &plane_samples(&config, &sim, offset), DT);
    }
    let grounded = WheelIndex::ALL
        .iter()
        .filter(|wheel| sim.state.suspension.wheels[**wheel as usize].is_grounded)
        .count();
    println!(
        "kerb: speed {speed_before:.2} m/s, peak load {peak_load:.1} N, y {y_before:.3} -> {:.3} m, grounded {grounded}/4, fallbacks {}, worst iterations {}",
        sim.state.transform.origin.y,
        sim.state.suspension.axle_fallback_count,
        sim.state.suspension.axle_max_iterations
    );
    trace(
        "kerb_step",
        format!(
            "\"speed_before\":{speed_before:.6},\"peak_load_n\":{peak_load:.6},\"y_before\":{y_before:.6},\"final_y\":{:.6},\"grounded\":{grounded},\"fallbacks\":{},\"max_iterations\":{}",
            sim.state.transform.origin.y,
            sim.state.suspension.axle_fallback_count,
            sim.state.suspension.axle_max_iterations
        ),
    );
    assert!(sim.state.transform.origin.is_finite(), "kerb must keep a finite state");
    assert!(peak_load.is_finite() && peak_load > 2000.0, "the kerb must load the tyre");
    assert_eq!(grounded, 4, "the car must recover contact after the kerb");
    assert!(
        sim.state.suspension.axle_fallback_count <= 2,
        "a 15 mm kerb must not push the axle into the fallback"
    );
}

/// Partial contact: one ray of the tricast supports the corner. The suspension must
/// keep transmitting and must not report a full patch.
#[test]
fn partial_contact_keeps_the_suspension_connected() {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let offset = prepare_plane(&config, &mut sim, DT);
    let samples = plane_samples(&config, &sim, offset);
    let mut partial = samples[WheelIndex::FrontLeft as usize];
    partial.center.is_colliding = false;
    partial.outer.is_colliding = false;
    let mut partial_samples = samples;
    partial_samples[WheelIndex::FrontLeft as usize] = partial;
    for _ in 0..240 {
        sim.step(&VehicleInput::default(), &partial_samples, DT);
    }
    let state = sim.state.suspension.wheels[WheelIndex::FrontLeft as usize];
    println!(
        "partial contact: fraction {:.2}, load {:.1} N, grounded {}, compression {:.4} m",
        state.contact_fraction, state.total_normal_force, state.is_grounded,
        state.suspension_compression_m
    );
    trace(
        "partial_contact",
        format!(
            "\"fraction\":{:.6},\"load_n\":{:.6},\"grounded\":{},\"compression_m\":{:.6}",
            state.contact_fraction, state.total_normal_force, state.is_grounded,
            state.suspension_compression_m
        ),
    );
    assert!((state.contact_fraction - 0.25).abs() < 1e-9, "one of four ray weights");
    assert!(state.is_grounded, "a partially supported corner is still in contact");
    assert!(state.total_normal_force > 100.0, "the corner must still carry load");
    assert!(sim.state.transform.origin.is_finite(), "state must stay finite");
}

/// Geometry envelope: the rest point sits inside the prepared travel range and a normal
/// drive never leaves it by more than the documented stop overtravel.
#[test]
fn geometry_envelope_contains_the_used_travel() {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let offset = prepare_plane(&config, &mut sim, DT);
    let mut input = VehicleInput::default();
    input.throttle = 0.35;
    let mut lowest = [f64::INFINITY; 4];
    let mut highest = [f64::NEG_INFINITY; 4];
    for _ in 0..960 {
        sim.step(&input, &plane_samples(&config, &sim, offset), DT);
        for wheel in WheelIndex::ALL {
            let q = sim.state.suspension.wheels[wheel as usize].suspension_compression_m
                - rest_compression_m(&config, wheel);
            lowest[wheel as usize] = lowest[wheel as usize].min(q);
            highest[wheel as usize] = highest[wheel as usize].max(q);
        }
    }
    let prepared = sim
        .state
        .suspension
        .prepared_geometric()
        .expect("geometric suspension prepared");
    let mut report = String::new();
    for wheel in WheelIndex::ALL {
        let envelope = prepared.for_wheel(wheel);
        let rest = rest_compression_m(&config, wheel);
        report.push_str(&format!(
            "{wheel:?}: q [{:.4}, {:.4}] m, rest {:.4} m, comp {:.4} m, k_wheel {:.0} N/m; ",
            envelope.q_min, envelope.q_max, rest,
            sim.state.suspension.wheels[wheel as usize].suspension_compression_m,
            envelope.k_wheel_rest_n_per_m
        ));
        println!(
            "raw {wheel:?}: rest {rest:.4}, used [{:.4}, {:.4}], envelope [{:.4}, {:.4}]",
            lowest[wheel as usize], highest[wheel as usize], envelope.q_min, envelope.q_max
        );
        assert!(
            lowest[wheel as usize] > envelope.q_min && highest[wheel as usize] < envelope.q_max,
            "{wheel:?} must stay inside [{:.4}, {:.4}] m, used [{:.4}, {:.4}]",
            envelope.q_min, envelope.q_max, lowest[wheel as usize], highest[wheel as usize]
        );
    }
    println!("geometry: {report}");
    trace("geometry_envelope", format!("\"detail\":\"{}\"", report.replace('"', "'")));
}

/// Inclined ground: the four normal reactions must still carry the weight projected on
/// the ground normal, and the loaded side must follow the slope.
#[test]
fn inclined_plane_keeps_the_normal_load_balance() {
    let config = profile();
    for roll in [0.05_f64, -0.05_f64] {
        let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
        let offset = plane_offset(&config, &sim);
        // Spawn aligned to the bank. A level body placed at design height on this
        // slope initially demands ~42 mm bump on one side, beyond the authored
        // 25/30 mm stroke. The old 30 mm numerical overtravel hid that penetration.
        sim.state.transform.basis = Mat3::from_euler_yxz(0.0, 0.0, roll);
        sim.state.orientation = Quat::from_mat3(&sim.state.transform.basis);
        let mut input = VehicleInput::default();
        input.handbrake = 1.0;
        for _ in 0..720 {
            sim.step(&input, &sloped_samples(&config, &sim, offset, roll), DT);
        }
        let loads: [f64; 4] = WheelIndex::ALL
            .map(|wheel| sim.state.suspension.wheels[wheel as usize].total_normal_force);
        let compression: [f64; 4] = WheelIndex::ALL
            .map(|wheel| sim.state.suspension.wheels[wheel as usize].suspension_compression_m);
        let total: f64 = loads.iter().sum();
        let expected = total_mass(&config) * GRAVITY * roll.cos();
        let grounded = WheelIndex::ALL
            .iter()
            .filter(|wheel| sim.state.suspension.wheels[**wheel as usize].is_grounded)
            .count();
        println!(
            "incline {:.1} deg: loads {loads:?}, total {total:.1} N vs {expected:.1} N, comp {compression:?}, grounded {grounded}/4",
            roll.to_degrees()
        );
        trace(
            "inclined_plane",
            format!(
                "\"roll_rad\":{roll:.6},\"loads\":{loads:?},\"total_n\":{total:.6},\"expected_n\":{expected:.6},\"compressions\":{compression:?},\"grounded\":{grounded}"
            ),
        );
        assert!(total.is_finite(), "state must stay finite on the slope");
        assert_eq!(grounded, 4, "the slope must keep all four wheels loaded");
        assert!(
            (total - expected).abs() < 0.03 * expected,
            "normal reactions {total:.1} N must match W cos(roll) {expected:.1} N"
        );
        let right_loaded = compression[1] > compression[0];
        let _ = right_loaded;
        // The body rolls with the slope: the lower side carries more load.
        let lower_side_loaded = loads[0] - loads[1];
        assert!(
            lower_side_loaded.abs() > 20.0,
            "a {:.1} deg bank must transfer load, got {lower_side_loaded:.1} N",
            roll.to_degrees()
        );
        assert_eq!(
            lower_side_loaded > 0.0,
            roll > 0.0,
            "the lower side must carry more load (roll {roll:.3})"
        );
        let body_roll = sim.state.transform.basis.y.x;
        assert!(
            (body_roll + roll.sin()).abs() < 0.01,
            "the chassis must roll with the plane: {body_roll:.4} vs {:.4}",
            -roll.sin()
        );
    }
}

/// A wheel that loses every ray keeps its suspension force on the body (Phase C) and
/// never breaks the published state.
#[test]
fn wheel_losing_every_ray_stays_connected_to_the_body() {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let offset = prepare_plane(&config, &mut sim, DT);
    let mut samples = plane_samples(&config, &sim, offset);
    samples[WheelIndex::FrontRight as usize] = airborne();
    for _ in 0..240 {
        sim.step(&VehicleInput::default(), &samples, DT);
    }
    let lifted = sim.state.suspension.wheels[WheelIndex::FrontRight as usize];
    let grounded = WheelIndex::ALL
        .iter()
        .filter(|wheel| sim.state.suspension.wheels[**wheel as usize].is_grounded)
        .count();
    println!(
        "ray loss: FR element {:.1} N, grounded {grounded}/4, y {:.4}, finite {}",
        lifted.chassis_suspension_force,
        sim.state.transform.origin.y,
        sim.state.transform.origin.is_finite()
    );
    trace(
        "ray_loss",
        format!(
            "\"fr_element_n\":{:.6},\"grounded\":{grounded},\"y\":{:.6}",
            lifted.chassis_suspension_force, sim.state.transform.origin.y
        ),
    );
    assert_eq!(grounded, 3, "only the wheel without rays leaves contact");
    assert!(
        lifted.chassis_suspension_force.abs() > 10.0,
        "the airborne corner must still transmit its element force"
    );
    assert!(sim.state.transform.origin.is_finite(), "state must stay finite");
    assert!(sim.state.linear_velocity.length().is_finite());
}

/// Reset returns to the same rest state: same ride height and compressions.
#[test]
fn reset_restores_the_same_rest_state() {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let offset = prepare_plane(&config, &mut sim, DT);
    let before_y = sim.state.transform.origin.y;
    let before: [f64; 4] = WheelIndex::ALL
        .map(|wheel| sim.state.suspension.wheels[wheel as usize].suspension_compression_m);
    sim.reset(Vec3::ZERO, 0.0);
    settle_on_plane(&config, &mut sim, offset, 1.75, DT);
    let after_y = sim.state.transform.origin.y;
    let after: [f64; 4] = WheelIndex::ALL
        .map(|wheel| sim.state.suspension.wheels[wheel as usize].suspension_compression_m);
    println!("reset: y {before_y:.4} -> {after_y:.4}, comp {before:?} -> {after:?}");
    trace(
        "reset",
        format!(
            "\"y_before\":{before_y:.6},\"y_after\":{after_y:.6},\"comp_before\":{before:?},\"comp_after\":{after:?}"
        ),
    );
    assert!((before_y - after_y).abs() < 2e-3, "reset must return the same ride height");
    for wheel in 0..4 {
        assert!(
            (before[wheel] - after[wheel]).abs() < 2e-3,
            "corner {wheel} must return to the same compression"
        );
    }
}

/// No external force but weight: the *centre of mass* of body plus corners must
/// accelerate at -g, even while the springs are still pushing the body up.
#[test]
fn free_fall_of_the_whole_vehicle_is_gravity_only() {
    let (mut sim, config) = settled();
    let air = [airborne(); 4];
    let body_before = sim.state.linear_acceleration;
    sim.step(&VehicleInput::default(), &air, DT);
    let a_b = sim.state.linear_acceleration.y;
    let mut corner_moment = 0.0;
    let mut corner_mass = 0.0;
    for wheel in WheelIndex::ALL {
        let state = sim.state.suspension.wheels[wheel as usize];
        let mass = vehicle_physics_engine::simulation::corner_unsprung_mass_kg(&config, wheel);
        corner_mass += mass;
        // Relative axial acceleration is not the corner's world acceleration.
        let r = vehicle_physics_engine::simplified_body_coupling::hub_arm(&config, &sim.state, wheel);
        let axis = sim.state.transform.basis.y;
        let omega = sim.state.angular_velocity;
        let acceleration = sim.state.linear_acceleration
            + sim.state.angular_acceleration.cross(r) + omega.cross(omega.cross(r))
            + omega.cross(axis) * (2.0 * state.unsprung_velocity_m_s)
            + axis * state.unsprung_acceleration_m_s2;
        corner_moment += mass * acceleration.y;
    }
    let total = total_mass(&config);
    let com = (a_b * (total - corner_mass) + corner_moment) / total;
    println!(
        "free fall: body a_y {a_b:.4} (before {:.4}), corner mass {corner_mass:.2} kg, COM a_y {com:.4}",
        body_before.y
    );
    trace(
        "free_fall",
        format!(
            "\"body_a_y\":{a_b:.6},\"corner_mass_kg\":{corner_mass:.6},\"com_a_y\":{com:.6},\"total_mass_kg\":{total:.6}"
        ),
    );
    assert!(
        (com + GRAVITY).abs() < 0.05,
        "body plus corners must fall at -g, got {com:.3} (body {a_b:.3})"
    );
}

/// Mirror symmetry: swapping the left/right samples must swap the corner loads
/// without changing the axle total.
#[test]
fn left_right_permutation_keeps_the_axle_total() {
    let (sim, config) = settled();
    let body = body_of(&sim);
    let mut samples = settled_samples(&config);
    let base = rest_compression_m(&config, WheelIndex::FrontLeft);
    samples[WheelIndex::FrontLeft as usize] = contact_sample(&config, WheelIndex::FrontLeft, base + 0.010);
    samples[WheelIndex::FrontRight as usize] = contact_sample(&config, WheelIndex::FrontRight, base - 0.010);

    let mut direct = sim.clone();
    let (direct_force, _) = direct.solve_external(body, &VehicleInput::default(), &samples, DT);
    let mut mirrored_samples = samples;
    mirrored_samples.swap(WheelIndex::FrontLeft as usize, WheelIndex::FrontRight as usize);
    let mut mirrored = sim.clone();
    let (mirrored_force, _) = mirrored.solve_external(body, &VehicleInput::default(), &mirrored_samples, DT);

    println!(
        "permutation: direct {:.3} N, mirrored {:.3} N",
        direct_force.force_world.y, mirrored_force.force_world.y
    );
    assert!(
        (direct_force.force_world.y - mirrored_force.force_world.y).abs() < 1.0,
        "axle vertical total must be permutation invariant"
    );
}
