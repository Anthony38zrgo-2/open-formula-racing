use vehicle_physics_engine::{
    simplified_body_coupling::{body_accelerations, hub_arm},
    simulation::{BodyKinematics, ForceTorqueOutput, VehicleSimulator},
    suspension::{rest_compression_m, SuspensionBackend},
    types::{RaycastHit, SurfaceType, TriRaycastSample, Vec3, VehicleInput, WheelIndex},
    vehicle_config::VehicleConfig,
    wheel_mechanics::WheelMechanicalTuning,
};

fn config() -> VehicleConfig {
    VehicleConfig::williams_fw19_hockenheim_canonical()
}

#[test]
fn horizontal_force_accelerates_total_mass_and_not_only_sprung_mass() {
    let c = config();
    let sim = VehicleSimulator::new(c.clone(), Vec3::ZERO, 0.0);
    let (base, _) = body_accelerations(&c, &sim.state, ForceTorqueOutput::default()).unwrap();
    let (loaded, _) = body_accelerations(
        &c,
        &sim.state,
        ForceTorqueOutput {
            force_world: Vec3::new(0., 0., 1000.),
            torque_world: Vec3::ZERO,
        },
    )
    .unwrap();
    let total = c.chassis.total_vehicle_mass(&c.tires, &c.fuel);
    assert!(((loaded - base).z - 1000. / total).abs() < 1e-10);
}

#[test]
fn yaw_inertia_includes_transport_of_the_corner_masses() {
    let c = config();
    let sim = VehicleSimulator::new(c.clone(), Vec3::ZERO, 0.0);
    let props=vehicle_physics_engine::vehicle_mass_properties::resolve_vehicle_mass_properties_for_suspension_backend(&c,SuspensionBackend::Simplified);
    let (a0, b0) = body_accelerations(&c, &sim.state, ForceTorqueOutput::default()).unwrap();
    let (a1, b1) = body_accelerations(
        &c,
        &sim.state,
        ForceTorqueOutput {
            force_world: Vec3::ZERO,
            torque_world: props.inertia_about_center_of_mass.multiply(Vec3::UP),
        },
    )
    .unwrap();
    assert!((b1 - b0 - Vec3::UP).length() < 1e-10);
    assert!((a1 - a0).length() < 1e-10);
}

#[test]
fn a_free_corner_sees_the_rotating_anchor_acceleration() {
    let mut c = config();
    c.suspension.front_arb_ratio = 0.;
    c.suspension.rear_arb_ratio = 0.;
    let geo = c.suspension.geometric_suspension.as_mut().unwrap();
    for axle in [&mut geo.front, &mut geo.rear] {
        axle.spring_rate_N_per_m = 1e-12;
        axle.spring_free_length_m = axle.spring_installed_length_m;
        axle.damper_bump_Ns_per_m = 0.;
        axle.damper_rebound_Ns_per_m = 0.;
    }
    let mut sim = VehicleSimulator::new(c.clone(), Vec3::ZERO, 0.0);
    sim.state.linear_acceleration = Vec3::new(0., -9.80665, 0.);
    sim.state.angular_velocity = Vec3::new(0., 0., 2.);
    let expected = WheelIndex::ALL.map(|w| {
        -sim.state
            .angular_velocity
            .cross(sim.state.angular_velocity.cross(hub_arm(&c, &sim.state, w)))
            .y
    });
    let b = sim.state.body_kinematics();
    sim.solve_external(
        b,
        &VehicleInput::default(),
        &[TriRaycastSample::default(); 4],
        1. / 480.,
    );
    assert!(sim.state.suspension.mechanical_failure.is_none());
    for w in WheelIndex::ALL {
        let observed = sim.state.suspension.wheels[w as usize].unsprung_acceleration_m_s2;
        assert!(
            (observed - expected[w as usize]).abs() < 1e-5,
            "{w:?}: {observed} vs {}",
            expected[w as usize]
        );
    }
}

fn momentum(sim: &VehicleSimulator) -> Vec3 {
    let c = &sim.config;
    let s = &sim.state;
    let p=vehicle_physics_engine::vehicle_mass_properties::resolve_vehicle_mass_properties_for_suspension_backend(c,SuspensionBackend::Simplified);
    let mut first = Vec3::ZERO;
    let mut relative = Vec3::ZERO;
    for w in WheelIndex::ALL {
        let m = vehicle_physics_engine::simulation::corner_unsprung_mass_kg(c, w);
        let design = c
            .suspension
            .geometric_suspension
            .as_ref()
            .unwrap()
            .corners
            .get(w)
            .hub_center
            - p.center_of_mass_local_metres;
        first += (hub_arm(c, s, w) - s.transform.basis.transform_vector(design)) * m;
        relative +=
            s.transform.basis.y * (m * s.suspension.wheels[w as usize].unsprung_velocity_m_s);
    }
    s.linear_velocity * p.total_mass_kilograms + s.angular_velocity.cross(first) + relative
}

#[test]
fn actual_world_momentum_in_free_fall_closes_against_gravity_impulse() {
    let c = config();
    let mut sim = VehicleSimulator::new(c, Vec3::ZERO, 0.0);
    let dt = 1. / 480.;
    let before = momentum(&sim);
    sim.step(
        &VehicleInput::default(),
        &[TriRaycastSample::default(); 4],
        dt,
    );
    assert!(
        sim.state.suspension.mechanical_failure.is_none(),
        "{:?}",
        sim.state.suspension.mechanical_failure
    );
    let total = sim
        .config
        .chassis
        .total_vehicle_mass(&sim.config.tires, &sim.config.fuel);
    let error = (momentum(&sim) - before - Vec3::new(0., -9.80665 * total * dt, 0.)).length();
    assert!(error < 0.02, "unexplained impulse {error} N s");
}

#[test]
fn an_infeasible_road_rolls_back_pose_clock_fuel_and_corner_state() {
    let c = config();
    let mut sim = VehicleSimulator::new(c, Vec3::ZERO, 0.0);
    let before = serde_json::to_value(&sim).unwrap();
    let hit = RaycastHit {
        is_colliding: true,
        distance: 0.,
        point: Vec3::new(0., 10., 0.),
        normal: Vec3::UP,
        surface: SurfaceType::Road,
    };
    sim.step(
        &VehicleInput::default(),
        &[TriRaycastSample {
            inner: hit,
            center: hit,
            outer: hit,
        }; 4],
        1. / 120.,
    );
    assert!(sim.state.suspension.mechanical_failure.is_some());
    sim.state.suspension.mechanical_failure = None;
    assert_eq!(serde_json::to_value(&sim).unwrap(), before);
}

#[test]
fn hard_travel_limit_impulses_reach_the_body() {
    let c = config();
    let mut sim = VehicleSimulator::new(c.clone(), Vec3::ZERO, 0.0);
    for w in WheelIndex::ALL {
        let lower = sim
            .state
            .suspension
            .prepared_geometric()
            .unwrap()
            .for_wheel(w)
            .q_min;
        let tuning = WheelMechanicalTuning::for_wheel(&c, w);
        let s = &mut sim.state.suspension.wheels[w as usize];
        s.suspension_compression_m =
            rest_compression_m(&c, w) + lower - tuning.hard_stop_overtravel_m;
        s.unsprung_velocity_m_s = -20.;
    }
    let body = BodyKinematics {
        transform: sim.state.transform,
        orientation: sim.state.orientation,
        linear_velocity: Vec3::ZERO,
        angular_velocity: Vec3::ZERO,
    };
    let dt = 1. / 480.;
    sim.solve_external(
        body,
        &VehicleInput::default(),
        &[TriRaycastSample::default(); 4],
        dt,
    );
    assert!(sim.state.suspension.mechanical_failure.is_none());
    let mut stop_impulse = 0.;
    for w in WheelIndex::ALL {
        let s = &sim.state.suspension.wheels[w as usize];
        stop_impulse += s.travel_limit_impulse_newton_seconds.abs();
        assert!(
            (s.chassis_suspension_force * dt - s.suspension_impulse_newton_seconds
                + s.travel_limit_impulse_newton_seconds)
                .abs()
                < 1e-8
        );
    }
    assert!(stop_impulse > 1.0, "fixture did not reach travel limit");
}
