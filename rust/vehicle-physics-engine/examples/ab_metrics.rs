//! A/B metric runner: one deterministic scenario, one CSV row per sample.
//!
//! Prints to stdout so the same program can be built in two checkouts (before/after) and
//! diffed by the driving script. Units are SI.

use vehicle_physics_engine::{
    open_formula::{install_track_ground_mesh, OpenFormulaSimulation},
    open_formula_ffi::OfpControlInput,
    simulation::default_spawn_height,
    suspension_setup::adopt_antiroll_design_bar,
    types::Vec3,
    vehicle_config::VehicleConfig,
};

fn main() {
    let mut config = VehicleConfig::williams_fw19_hockenheim_canonical();
    adopt_antiroll_design_bar(&mut config);
    let directory = std::env::temp_dir().join("ofp-ab-metrics");
    std::fs::create_dir_all(&directory).unwrap();
    let path = directory.join("ab_ground_collisions.txt");
    // 200 m flat plane in Speed Dreams coordinates (X forward, Y left, Z up).
    std::fs::write(
        &path,
        "FORMULA_GROUND_1 2\n\
         1.0 0.002 -100 -100 0 100 -100 0 100 100 0\n\
         1.0 0.002 -100 -100 0 100 100 0 -100 100 0\n",
    )
    .unwrap();
    let (identity, _) = install_track_ground_mesh(path.to_str().unwrap(), Some("ab-plane"), Some(1))
        .expect("ground world");
    let mut simulation = OpenFormulaSimulation::create_with_world(
        None,
        Vec3::new(0.0, default_spawn_height(&config), 0.0),
        0.0,
        Some(7.6),
        true,
    )
    .expect("vehicle");

    println!("# circuit={} triangles={} hash={}",
        identity.circuit_id, identity.triangle_count, &identity.geometry_sha256[..16]);
    println!("t,com_z_or_height,total_load_n,fl_n,fr_n,rl_n,rr_n,speed_m_s,yaw_rate_rad_s,\
fl_travel_m,fr_travel_m,rl_travel_m,rr_travel_m");

    let dt = 1.0 / 240.0;
    let total_steps = 1200; // 5 s
    let mut next_sample = 0.0;
    for step in 0..total_steps {
        let t = step as f64 * dt;
        // 0-1 s settle, 1-3 s straight braking, 3-5 s braking + left turn.
        let input = if t < 1.0 {
            // Settle on the brakes with the clutch disengaged.
            OfpControlInput { brake: 1.0, clutch: 1.0, gear_request: 0, ..Default::default() }
        } else if t < 3.0 {
            // Straight acceleration through the gears.
            OfpControlInput {
                throttle: 0.55,
                clutch: 0.0,
                gear_request: if t < 2.0 { 2 } else { 3 },
                ..Default::default()
            }
        } else {
            // Braking while turning left: longitudinal + lateral load transfer.
            OfpControlInput {
                brake: 0.55,
                steering: -0.30,
                clutch: 1.0,
                gear_request: 3,
                ..Default::default()
            }
        };
        if simulation.step(&input, &[], dt).is_err() {
            println!("# rejected at t={t:.4}");
            break;
        }
        if t + 1e-9 >= next_sample {
            next_sample += 0.25;
            let s = simulation.snapshot();
            let total: f64 = (0..4).map(|i| s.wheels[i].normal_force_n).sum();
            let speed = (s.linear_velocity.x.powi(2)
                + s.linear_velocity.y.powi(2)
                + s.linear_velocity.z.powi(2))
            .sqrt();
            println!(
                "{:.3},{:.6},{:.3},{:.3},{:.3},{:.3},{:.3},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6}",
                s.sim_time_s,
                s.position.y,
                total,
                s.wheels[0].normal_force_n,
                s.wheels[1].normal_force_n,
                s.wheels[2].normal_force_n,
                s.wheels[3].normal_force_n,
                speed,
                s.angular_velocity.y,
                s.wheels[0].suspension_travel_m,
                s.wheels[1].suspension_travel_m,
                s.wheels[2].suspension_travel_m,
                s.wheels[3].suspension_travel_m
            );
        }
    }
}
