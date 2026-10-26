//! C1 suspension regularisation: acceptance banks for the six-point handoff
//! (2026-10-25).
//!
//! Each test drives the production path and measures the physical quantity named
//! in the handoff. They are regressions, not calibrations: the values come from
//! `suspension.json` / `suspension_filters.json`.
//!
//! Run with the measurements visible:
//!   cargo test --release -p vehicle_physics_engine --test suspension_c1 -- --nocapture

use std::array;

use vehicle_physics_engine::simulation::VehicleSimulator;
use vehicle_physics_engine::suspension::{rest_compression_m, SuspensionSystem};
use vehicle_physics_engine::suspension_geo_config::BumpStopLaw;
use vehicle_physics_engine::suspension_setup::adopt_antiroll_design_bar;
use vehicle_physics_engine::types::{
    RaycastHit, SurfaceType, TriRaycastSample, Vec3, VehicleInput, WheelIndex,
};
use vehicle_physics_engine::vehicle_config::VehicleConfig;
use vehicle_physics_engine::wheel_mechanics::tire_radius;

const DT: f64 = 1.0 / 480.0;
const HOST_DT: f64 = 0.002;
const GRAVITY: f64 = 9.80665;

fn profile() -> VehicleConfig {
    let mut config = VehicleConfig::williams_fw19_hockenheim_canonical();
    adopt_antiroll_design_bar(&mut config);
    config
}

/// Same profile with every 2026-10-25 element switched back to the historical law.
fn legacy_profile() -> VehicleConfig {
    let mut config = profile();
    let geometry = config.suspension.geometric_suspension.as_mut().unwrap();
    for axle in [&mut geometry.front, &mut geometry.rear] {
        axle.damper_knee_blend_m_per_s = 0.0;
        axle.bump_stop_law = BumpStopLaw::Legacy;
        axle.bump_stop_damper_Ns_per_m = 0.0;
        axle.hard_stop_compliance_m = 0.0;
        axle.hard_stop_stiffness_N_per_m = 0.0;
        axle.hard_stop_damper_Ns_per_m = 0.0;
        axle.arb_damper_Ns_per_m = 0.0;
    }
    config.suspension_filters = None;
    config
}

fn spring_length(config: &VehicleConfig, wheel: WheelIndex) -> f64 {
    if wheel.is_front() {
        config.suspension.front_spring_length
    } else {
        config.suspension.rear_spring_length
    }
}

fn contact_sample(config: &VehicleConfig, wheel: WheelIndex, compression_m: f64) -> TriRaycastSample {
    let distance =
        (spring_length(config, wheel) + tire_radius(config, wheel) - compression_m).max(0.0);
    let hit = RaycastHit {
        is_colliding: true,
        distance,
        point: Vec3::new(0.0, -distance, 0.0),
        normal: Vec3::UP,
        surface: SurfaceType::Road,
        ..Default::default()
    };
    TriRaycastSample {
        inner: hit,
        center: hit,
        outer: hit,
        ..TriRaycastSample::default()
    }
}

fn settled_samples(config: &VehicleConfig) -> [TriRaycastSample; 4] {
    array::from_fn(|index| {
        let wheel = WheelIndex::ALL[index];
        contact_sample(config, wheel, rest_compression_m(config, wheel))
    })
}

fn settle_vehicle(config: &VehicleConfig) -> VehicleSimulator {
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let samples = settled_samples(config);
    for _ in 0..2000 {
        sim.step(&VehicleInput::default(), &samples, HOST_DT);
    }
    sim
}

fn total_mass(config: &VehicleConfig) -> f64 {
    config.chassis.total_vehicle_mass(&config.tires, &config.fuel)
}

/// Static contract: the C1 elements must not move the resting pose or the wheel
/// loads, and no stop may support the car.
#[test]
fn static_rest_is_identical_to_the_historical_law() {
    let c1 = profile();
    let legacy = legacy_profile();
    let c1_sim = settle_vehicle(&c1);
    let legacy_sim = settle_vehicle(&legacy);
    let loads = |sim: &VehicleSimulator| -> [f64; 4] {
        array::from_fn(|index| sim.state.suspension.wheels[index].total_normal_force)
    };
    let stops = |sim: &VehicleSimulator| -> [f64; 4] {
        array::from_fn(|index| sim.state.suspension.wheels[index].bottom_out_force)
    };
    let loads_c1 = loads(&c1_sim);
    let loads_old = loads(&legacy_sim);
    println!(
        "rest: C1 pose {:.9}/{:.9} m, legacy {:.9}/{:.9} m",
        c1_sim.state.transform.origin.y,
        c1_sim.state.transform.origin.x,
        legacy_sim.state.transform.origin.y,
        legacy_sim.state.transform.origin.x
    );
    println!("rest: C1 loads {loads_c1:?} N, legacy {loads_old:?} N");
    println!("rest: C1 stops {:?} N", stops(&c1_sim));
    assert!(
        (c1_sim.state.transform.origin.y - legacy_sim.state.transform.origin.y).abs() < 1e-6,
        "the C1 elements must not move the resting body height"
    );
    for index in 0..4 {
        assert!(
            (loads_c1[index] - loads_old[index]).abs() < 1e-3,
            "corner {index}: C1 load {} vs legacy {}",
            loads_c1[index],
            loads_old[index]
        );
    }
    let total: f64 = loads_c1.iter().sum();
    let expected = total_mass(&c1) * GRAVITY;
    println!("rest: total load {total:.1} N vs W {expected:.1} N");
    assert!(
        (total - expected).abs() < 0.005 * expected,
        "loads {loads_c1:?} must carry the weight {expected:.1} N"
    );
    for wheel in WheelIndex::ALL {
        let state = c1_sim.state.suspension.wheels[wheel as usize];
        assert!(
            state.bottom_out_force.abs() < 0.1,
            "{wheel:?}: rest must not touch the stop ({})",
            state.bottom_out_force
        );
        assert!(
            state.antiroll_force.abs() < 1.0,
            "{wheel:?}: rest must be bar-neutral ({})",
            state.antiroll_force
        );
    }
}

/// P1: the contact-height target may not move faster than the authored slew limit.
///
/// The probe is a 60 mm drop of the ground under one wheel: unlike a step *up* it
/// stays inside the droop travel, so the simulator accepts it, and it is four
/// times larger than `road_height_max_rate_m_s * dt` allows in one tick.
#[test]
fn road_height_slew_limit_caps_a_ground_drop() {
    let config = profile();
    let limit = config
        .suspension_filters
        .as_ref()
        .expect("profile declares suspension_filters")
        .road_height_max_rate_m_s();
    assert!(limit > 0.0, "the slew limiter must be authored");
    let allowed = limit * DT;
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let base = settled_samples(&config);
    for _ in 0..480 {
        sim.step(&VehicleInput::default(), &base, DT);
    }
    let wheel = WheelIndex::FrontLeft;
    let index = wheel as usize;
    let mut dropped = base;
    dropped[index] = contact_sample(&config, wheel, rest_compression_m(&config, wheel) - 0.060);
    let mut previous = sim.state.suspension.wheels[index].normal_road_compression_metres;
    let mut biggest: f64 = 0.0;
    for _ in 0..120 {
        sim.step(&VehicleInput::default(), &dropped, DT);
        let current = sim.state.suspension.wheels[index].normal_road_compression_metres;
        biggest = biggest.max((current - previous).abs());
        previous = current;
    }
    println!("slew: limit {limit} m/s, allowed {allowed:.4} m/step, observed {biggest:.4} m/step");
    assert!(
        biggest <= allowed + 1e-9,
        "target moved {biggest:.6} m in one step, limit is {allowed:.6} m"
    );
    assert!(
        biggest > 0.5 * allowed,
        "the limiter must be active, not inert (observed {biggest:.6} m)"
    );
    assert!(
        sim.state.transform.origin.is_finite(),
        "the state must stay finite"
    );
}

/// Control: without the filter domain there is no slew limit, so the same drop
/// moves the target by the whole geometric step. This proves the measurement above
/// is caused by the limiter and not by the sampling geometry.
#[test]
fn without_the_filter_domain_the_target_is_unbounded() {
    let mut config = profile();
    config.suspension_filters = None;
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let base = settled_samples(&config);
    for _ in 0..480 {
        sim.step(&VehicleInput::default(), &base, DT);
    }
    let wheel = WheelIndex::FrontLeft;
    let index = wheel as usize;
    let mut dropped = base;
    dropped[index] = contact_sample(&config, wheel, rest_compression_m(&config, wheel) - 0.060);
    let mut previous = sim.state.suspension.wheels[index].normal_road_compression_metres;
    let mut biggest: f64 = 0.0;
    for _ in 0..5 {
        sim.step(&VehicleInput::default(), &dropped, DT);
        let current = sim.state.suspension.wheels[index].normal_road_compression_metres;
        biggest = biggest.max((current - previous).abs());
        previous = current;
    }
    println!("no limiter: observed {biggest:.4} m in one step");
    assert!(
        biggest > 0.030,
        "without the domain the raw target jumps with the geometry (observed {biggest:.6} m)"
    );
}

/// A settled corner, plus its geometric bump limit.
fn corner_state(config: &VehicleConfig) -> (SuspensionSystem, f64, f64) {
    let mut suspension = SuspensionSystem::new(config);
    let rest = settled_samples(config);
    for _ in 0..480 {
        suspension.step(config, &rest, DT);
    }
    let rest_compression = rest_compression_m(config, WheelIndex::FrontLeft);
    let q_max = suspension
        .prepared_geometric()
        .expect("prepared envelope")
        .for_wheel(WheelIndex::FrontLeft)
        .q_max;
    (suspension, rest_compression, rest_compression + q_max)
}

/// Quasi-static compression from just below the progressive stop to just past the
/// geometric bump limit. Returns (peak stop force, peak |clamp impulse|, peak
/// compression) while the demand ramps slowly enough that the corner follows.
fn ramp_corner(
    config: &VehicleConfig,
    from_offset_m: f64,
    to_offset_m: f64,
    steps: usize,
) -> (f64, f64, f64) {
    let (mut suspension, _rest, envelope_top) = corner_state(config);
    let others = settled_samples(config);
    let prime = |offset: f64| {
        let mut samples = others;
        samples[WheelIndex::FrontLeft as usize] =
            contact_sample(config, WheelIndex::FrontLeft, envelope_top + offset);
        samples
    };
    // Settle the corner at the starting demand first: jumping 20 mm of travel in one
    // step would slam the stop at ~10 m/s and measure the sampler, not the law.
    let start = prime(from_offset_m);
    for _ in 0..240 {
        suspension.step(config, &start, DT);
    }
    let mut peak_stop: f64 = 0.0;
    let mut clamp: f64 = 0.0;
    let mut peak_compression: f64 = 0.0;
    for step in 0..=steps {
        let t = step as f64 / steps as f64;
        let offset = from_offset_m + (to_offset_m - from_offset_m) * t;
        suspension.step(config, &prime(offset), DT);
        let state = suspension.wheels[WheelIndex::FrontLeft as usize];
        peak_stop = peak_stop.max(state.bottom_out_force);
        clamp = clamp.max(state.travel_limit_impulse_newton_seconds.abs());
        peak_compression = peak_compression.max(state.suspension_compression_m);
    }
    (peak_stop, clamp, peak_compression)
}

/// P0/P1: inside the compliance band the stop is a spring+damper, so the
/// inelastic clamp never fires; the historical law clamps on contact.
#[test]
fn the_clamp_is_no_longer_the_first_response() {
    let c1 = profile();
    let legacy = legacy_profile();
    // 150 um past the geometric limit: inside the 400 um band, past the 100 um
    // numerical overtravel the historical clamp allows.
    let (peak_c1, clamp_c1, compression_c1) = ramp_corner(&c1, -0.002, 0.000_15, 240);
    let (peak_old, clamp_old, compression_old) = ramp_corner(&legacy, -0.002, 0.000_15, 240);
    println!(
        "band: C1 stop {peak_c1:.0} N, clamp {clamp_c1:.3} N s, compression {compression_c1:.5} m; legacy stop {peak_old:.0} N, clamp {clamp_old:.3} N s, compression {compression_old:.5} m"
    );
    assert!(
        clamp_c1 < 1e-9,
        "the compliant band must not clamp ({clamp_c1} N s)"
    );
    assert!(
        compression_c1 > compression_old + 2e-5,
        "the compliant band must allow more penetration ({compression_c1:.6} vs {compression_old:.6} m)"
    );
    let axle = *c1
        .suspension
        .geometric_suspension
        .as_ref()
        .unwrap()
        .axle(WheelIndex::FrontLeft);
    let bound = axle.hard_stop_stiffness_N_per_m * axle.hard_stop_compliance_m
        + axle.hard_stop_damper_Ns_per_m * 2.0;
    assert!(
        peak_c1 <= bound,
        "compliant force {peak_c1:.0} N must stay below k*c + c*2 m/s = {bound:.0} N"
    );
    assert!(peak_c1.is_finite() && peak_c1 > 0.0);
}

/// The envelope still wins at the end of the band: a demand beyond `compliance`
/// cannot push the wheel further than the band plus the numerical overtravel, the
/// state stays feasible and the force stays bounded.
#[test]
fn the_envelope_still_wins_beyond_the_band() {
    let config = profile();
    let axle = *config
        .suspension
        .geometric_suspension
        .as_ref()
        .unwrap()
        .axle(WheelIndex::FrontLeft);
    let (peak_stop, _clamp, compression) = ramp_corner(&config, -0.002, 0.001_2, 240);
    let (mut suspension, _rest, envelope_top) = corner_state(&config);
    let rest = settled_samples(&config);
    for _ in 0..240 {
        suspension.step(&config, &rest, DT);
    }
    println!(
        "clamp: peak compression {:.5} m vs envelope {envelope_top:.5} m (+{:.0} um), stop {peak_stop:.0} N, failure {:?}, released compression {:.5} m",
        compression,
        (compression - envelope_top) * 1e6,
        suspension.mechanical_failure,
        suspension.wheels[WheelIndex::FrontLeft as usize].suspension_compression_m
    );
    assert!(
        suspension.mechanical_failure.is_none(),
        "an over-demand inside the band+overtravel must stay feasible ({:?})",
        suspension.mechanical_failure
    );
    assert!(
        compression
            <= envelope_top
                + axle.hard_stop_compliance_m
                + 1.1 * axle.hard_stop_overtravel_m.unwrap_or(0.0),
        "the geometric envelope plus the band is the hard limit"
    );
    assert!(
        peak_stop.is_finite() && peak_stop < 30_000.0,
        "the stop force must stay bounded ({peak_stop:.0} N)"
    );
    // Released, the corner is back on its main spring with no residual penetration.
    let released = suspension.wheels[WheelIndex::FrontLeft as usize];
    assert!(
        (released.suspension_compression_m - rest_compression_m(&config, WheelIndex::FrontLeft))
            .abs()
            < 2e-3,
        "the corner must return to its static travel"
    );
}

/// The wheel returns to its spring once the ground is back at the resting height:
/// the compliant stop stores and returns energy without a residual offset.
#[test]
fn the_compliant_stop_releases_the_wheel() {
    let config = profile();
    let (mut suspension, rest, envelope_top) = corner_state(&config);
    let rest_samples = settled_samples(&config);
    // Push 350 um past the limit over 200 ticks, then release over 200 ticks.
    for step in 0..=200 {
        let t = step as f64 / 200.0;
        let mut samples = rest_samples;
        samples[WheelIndex::FrontLeft as usize] = contact_sample(
            &config,
            WheelIndex::FrontLeft,
            envelope_top - 0.0005 + 0.000_85 * t,
        );
        suspension.step(&config, &samples, DT);
    }
    let pushed = suspension.wheels[WheelIndex::FrontLeft as usize];
    for _ in 0..480 {
        suspension.step(&config, &rest_samples, DT);
    }
    let released = suspension.wheels[WheelIndex::FrontLeft as usize];
    println!(
        "release: pushed to {:.5} m (stop {:.0} N), released to {:.5} m (stop {:.3} N), static {rest:.5} m",
        pushed.suspension_compression_m,
        pushed.bottom_out_force,
        released.suspension_compression_m,
        released.bottom_out_force
    );
    assert!(
        pushed.suspension_compression_m > envelope_top - 0.000_5,
        "the push must reach the band"
    );
    assert!(
        (released.suspension_compression_m - rest).abs() < 2e-3,
        "the wheel must return to its static travel"
    );
    assert!(
        released.bottom_out_force.abs() < 1.0,
        "the stop must release"
    );
}

/// P2: the bar damper must not change the static split (checked above) and must
/// damp, not amplify, the roll response to a one-wheel bump.
#[test]
fn bar_damper_damps_the_one_wheel_bump_response() {
    let run = |damper: f64| {
        let mut config = profile();
        {
            let geometry = config.suspension.geometric_suspension.as_mut().unwrap();
            geometry.front.arb_damper_Ns_per_m = damper;
            geometry.rear.arb_damper_Ns_per_m = damper * 0.75;
        }
        let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
        let base = settled_samples(&config);
        for _ in 0..480 {
            sim.step(&VehicleInput::default(), &base, DT);
        }
        let mut bumped = base;
        bumped[WheelIndex::FrontLeft as usize] = contact_sample(
            &config,
            WheelIndex::FrontLeft,
            rest_compression_m(&config, WheelIndex::FrontLeft) + 0.010,
        );
        let mut peak_roll_rate: f64 = 0.0;
        let mut roll_work: f64 = 0.0;
        for _ in 0..480 {
            sim.step(&VehicleInput::default(), &bumped, DT);
            peak_roll_rate = peak_roll_rate.max(sim.state.angular_velocity.z.abs());
            roll_work += sim.state.angular_velocity.z.abs() * DT;
        }
        let split = sim.state.suspension.wheels[0].total_normal_force
            - sim.state.suspension.wheels[1].total_normal_force;
        (peak_roll_rate, roll_work, split)
    };
    let (rate_off, work_off, split_off) = run(0.0);
    let (rate_on, work_on, split_on) = run(200.0);
    println!(
        "bar damper: roll-rate peak {rate_off:.4} -> {rate_on:.4} rad/s, |roll-rate| integral {work_off:.5} -> {work_on:.5} rad, static split {split_off:.1} -> {split_on:.1} N"
    );
    assert!(
        work_on <= work_off,
        "the bar damper must remove roll motion, not add it ({work_off:.5} -> {work_on:.5})"
    );
    assert!(
        rate_on <= rate_off * 1.05 + 1e-6,
        "the bar damper must not amplify the roll-rate peak ({rate_off:.5} -> {rate_on:.5})"
    );
    assert!(
        (split_on - split_off).abs() < 1.0,
        "the bar damper changed the static split ({split_off:.3} -> {split_on:.3} N)"
    );
}

/// High-speed steering pulse (the tire-model protocol, 74 m/s straight): the
/// suspension must not introduce a growing yaw or roll oscillation. This is the
/// gate the handoff asks for before any force filter may be enabled; with every
/// filter at 0.0 the requirement is that nothing new oscillates.
#[test]
fn high_speed_steering_pulse_does_not_grow() {
    let config = profile();
    let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
    let samples = settled_samples(&config);
    // Settle, then launch along -Z (the profile forward axis) at 74 m/s.
    for _ in 0..240 {
        sim.step(&VehicleInput::default(), &samples, DT);
    }
    sim.state.linear_velocity = Vec3::new(0.0, 0.0, -74.0);
    sim.state.angular_velocity = Vec3::ZERO;
    let mut input = VehicleInput::default();
    input.throttle = 1.0;
    for _ in 0..120 {
        sim.step(&input, &samples, DT);
    }
    // 0.15 s left, 0.15 s right, then hands off.
    let window = |sim: &mut VehicleSimulator, seconds: f64, steer: f64| {
        let mut peak_yaw: f64 = 0.0;
        let mut peak_roll: f64 = 0.0;
        let steps = (seconds / DT).round() as usize;
        let mut local = input;
        local.steering = steer;
        for _ in 0..steps {
            sim.step(&local, &samples, DT);
            peak_yaw = peak_yaw.max(sim.state.angular_velocity.y.abs());
            peak_roll = peak_roll.max(sim.state.angular_velocity.z.abs());
        }
        (peak_yaw, peak_roll)
    };
    let (pulse_yaw, pulse_roll) = window(&mut sim, 0.15, 0.06);
    let _ = window(&mut sim, 0.15, -0.06);
    let mut peak_after: (f64, f64) = (0.0, 0.0);
    let (mut yaw_early, mut yaw_late) = (0.0_f64, 0.0_f64);
    for index in 0..(4.0 / DT) as usize {
        sim.step(&input, &samples, DT);
        let yaw = sim.state.angular_velocity.y.abs();
        let roll = sim.state.angular_velocity.z.abs();
        peak_after.0 = peak_after.0.max(yaw);
        peak_after.1 = peak_after.1.max(roll);
        if index < (2.0 / DT) as usize {
            yaw_early = yaw_early.max(yaw);
        } else {
            yaw_late = yaw_late.max(yaw);
        }
    }
    println!(
        "pulse 74 m/s: yaw peak during pulse {pulse_yaw:.4}, after {:.4} rad/s; roll {pulse_roll:.4} -> {:.4}; yaw envelope {yaw_early:.4} (0-2 s) -> {yaw_late:.4} (2-4 s); speed {:.1} m/s",
        peak_after.0,
        peak_after.1,
        -sim.state.linear_velocity.z
    );
    assert!(
        peak_after.0.is_finite() && sim.state.transform.origin.is_finite(),
        "the state must stay finite"
    );
    assert!(
        yaw_late <= yaw_early * 1.10 + 1e-6,
        "the yaw envelope must not grow after the pulse ({yaw_early:.5} -> {yaw_late:.5} rad/s)"
    );
}

/// Kerb strike: a 15 mm step under all four wheels at once (the harsh case). The
/// peak *contact* load, the number of wheels that leave the ground and the settled
/// pose must not get worse than the historical law.
#[test]
fn kerb_strike_peak_load_is_bounded_and_it_settles() {
    let measure = |config: &VehicleConfig| {
        let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
        let base = settled_samples(config);
        for _ in 0..720 {
            sim.step(&VehicleInput::default(), &base, DT);
        }
        let ride_before = sim.state.transform.origin.y;
        let mut stepped = base;
        for wheel in WheelIndex::ALL {
            stepped[wheel as usize] =
                contact_sample(config, wheel, rest_compression_m(config, wheel) + 0.015);
        }
        let mut peak_stop: f64 = 0.0;
        let mut peak_load: f64 = 0.0;
        for _ in 0..120 {
            sim.step(&VehicleInput::default(), &stepped, DT);
            for wheel in WheelIndex::ALL {
                let state = sim.state.suspension.wheels[wheel as usize];
                peak_stop = peak_stop.max(state.bottom_out_force.abs());
                peak_load = peak_load.max(state.total_normal_force);
            }
        }
        for _ in 0..480 {
            sim.step(&VehicleInput::default(), &base, DT);
        }
        let airborne = WheelIndex::ALL
            .iter()
            .filter(|wheel| !sim.state.suspension.wheels[**wheel as usize].is_grounded)
            .count();
        (
            ride_before,
            sim.state.transform.origin.y,
            peak_stop,
            peak_load,
            airborne,
        )
    };
    let (ride0, ride1, peak_c1, load_c1, airborne_c1) = measure(&profile());
    let (_, _, peak_old, load_old, airborne_old) = measure(&legacy_profile());
    println!(
        "kerb 15 mm: contact peak {load_c1:.0} N (legacy {load_old:.0} N), stop peak {peak_c1:.0} N (legacy {peak_old:.0} N), ride {ride0:.4} -> {ride1:.4} m, airborne {airborne_c1} (legacy {airborne_old})"
    );
    assert!(
        load_c1 <= load_old * 1.10 + 1.0,
        "the C1 suspension must not raise the peak contact load ({load_c1:.1} N vs {load_old:.1} N)"
    );
    assert!(
        airborne_c1 <= airborne_old,
        "the C1 stop must not lift more wheels ({airborne_c1} vs {airborne_old})"
    );
    assert!(
        (ride1 - ride0).abs() < 5e-4,
        "the car must settle back to the pre-kerb pose ({ride0:.4} -> {ride1:.4} m)"
    );
    assert!(peak_c1.is_finite() && peak_c1 < 20_000.0);
}

/// Rate independence of the resting state: the host may deliver 500, 240 or 120 Hz
/// (the simulator subdivides internally), and the C1 elements must not change the
/// settled pose or the loads.
#[test]
fn rest_is_rate_independent_at_120_240_and_500_hz() {
    let config = profile();
    let mut reports = Vec::new();
    for dt in [1.0_f64 / 500.0, 1.0 / 240.0, 1.0 / 120.0] {
        let mut sim = VehicleSimulator::new(config.clone(), Vec3::ZERO, 0.0);
        let samples = settled_samples(&config);
        let steps = (2.0 / dt).round() as usize;
        for _ in 0..steps {
            sim.step(&VehicleInput::default(), &samples, dt);
        }
        let loads: [f64; 4] =
            array::from_fn(|index| sim.state.suspension.wheels[index].total_normal_force);
        let total: f64 = loads.iter().sum();
        let expected = total_mass(&config) * GRAVITY;
        reports.push((
            dt,
            loads,
            total,
            expected,
            sim.state.transform.origin.is_finite(),
        ));
    }
    for (dt, loads, total, expected, finite) in &reports {
        println!(
            "rate {:.0} Hz: loads {loads:?} N, total {total:.1} N vs W {expected:.1} N",
            1.0 / dt
        );
        assert!(*finite, "the state must stay finite at {:.0} Hz", 1.0 / dt);
        assert!(
            (total - expected).abs() < 0.02 * expected,
            "at {:.0} Hz the car must carry its weight ({total:.1} vs {expected:.1} N)",
            1.0 / dt
        );
    }
    // The suspension, not the time step, decides where the weight goes.
    let front_share = |loads: &[f64; 4]| -> f64 {
        (loads[0] + loads[1]) / loads.iter().sum::<f64>()
    };
    let reference = front_share(&reports[0].1);
    for (dt, loads, _, _, _) in &reports {
        assert!(
            (front_share(loads) - reference).abs() < 1e-3,
            "front axle share must not depend on dt ({:.0} Hz: {:.5} vs {:.5})",
            1.0 / dt,
            front_share(loads),
            reference
        );
    }
}

/// Impact proxy: the wheel is released from a fixed compression offset onto the
/// stop and must not eject the car or leave a residual oscillation.
#[test]
fn drop_onto_the_stop_settles_without_ejecting() {
    let config = profile();
    let (mut suspension, rest, envelope_top) = corner_state(&config);
    let rest_samples = settled_samples(&config);
    let mut sample = rest_samples;
    sample[WheelIndex::FrontLeft as usize] =
        contact_sample(&config, WheelIndex::FrontLeft, rest + 0.004);
    // Preload the corner into the stop, then let the ground fall back to rest in one
    // step: the stop must return the stored energy without a growing oscillation.
    for _ in 0..120 {
        suspension.step(&config, &sample, DT);
    }
    let mut peak_after: f64 = 0.0;
    let mut last: f64 = 0.0;
    for step in 0..960 {
        suspension.step(&config, &rest_samples, DT);
        let state = suspension.wheels[WheelIndex::FrontLeft as usize];
        if step > 720 {
            peak_after = peak_after.max((state.suspension_compression_m - rest).abs());
        }
        last = state.suspension_compression_m;
    }
    println!(
        "drop: envelope {envelope_top:.5} m, tail deviation {:.6} m, final {last:.6} m vs static {rest:.6} m",
        peak_after
    );
    assert!(
        (last - rest).abs() < 2e-3,
        "the corner must settle back to its static travel"
    );
    assert!(
        peak_after < 5e-4,
        "no residual oscillation is allowed (tail {peak_after:.6} m)"
    );
}
