//! A/B metric runner (baseline API): one deterministic scenario, one CSV row per sample.
//! Identical inputs and sampling to `examples/ab_metrics.rs` in the refactored tree; only
//! the session-world entry points differ, because that is what changed.

use vehicle_physics_engine::{
    open_formula::{set_track_ground_mesh, OpenFormulaSimulation},
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
    std::fs::write(
        &path,
        "FORMULA_GROUND_1 2\n\
         1.0 0.002 -100 -100 0 100 -100 0 100 100 0\n\
         1.0 0.002 -100 -100 0 100 100 0 -100 100 0\n",
    )
    .unwrap();
    let triangles = set_track_ground_mesh(path.to_str().unwrap()).expect("ground world");
    let mut simulation = OpenFormulaSimulation::create(
        None,
        Vec3::new(0.0, default_spawn_height(&config), 0.0),
        0.0,
        Some(7.6),
    )
    .expect("vehicle");

    println!("# circuit=ab-plane triangles={triangles} hash=baseline");
    println!("t,com_z_or_height,total_load_n,fl_n,fr_n,rl_n,rr_n,speed_m_s,yaw_rate_rad_s,\
fl_travel_m,fr_travel_m,rl_travel_m,rr_travel_m");

    let dt = 1.0 / 240.0;
    let total_steps = 1200;
    let mut next_sample = 0.0;
    for step in 0..total_steps {
        let t = step as f64 * dt;
        let input = if t < 1.0 {
            OfpControlInput { brake: 1.0, clutch: 1.0, gear_request: 0, ..Default::default() }
        } else if t < 3.0 {
            OfpControlInput {
                throttle: 0.55,
                clutch: 0.0,
                gear_request: if t < 2.0 { 2 } else { 3 },
                ..Default::default()
            }
        } else {
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
