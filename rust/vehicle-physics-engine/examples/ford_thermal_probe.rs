//! Headless thermal probe for the Ford Escort RS2000 rally profile.
//!
//! Drives the production `OpenFormulaSimulation` on a flat gravel plane and prints
//! tire tread/carcass/gas temperatures, pressure and the profile's optimal window.
//! Used to decide whether the F1-style default thermal window is reachable on a
//! rally stage.

use std::path::PathBuf;

use vehicle_physics_engine::collision_world::{
    install_world, uninstall_world, CollisionWorld, TriangleMaterial,
};
use vehicle_physics_engine::open_formula::OpenFormulaSimulation;
use vehicle_physics_engine::open_formula_ffi::OfpControlInput;
use vehicle_physics_engine::types::{Vec3, WheelIndex};

fn flat_gravel_plane(half_extent: f64) -> CollisionWorld {
    let mut triangles = Vec::new();
    let cells = 4;
    let step = 2.0 * half_extent / cells as f64;
    for ix in 0..cells {
        for iz in 0..cells {
            let x0 = -half_extent + step * ix as f64;
            let x1 = x0 + step;
            let z0 = -half_extent + step * iz as f64;
            let z1 = z0 + step;
            let p00 = Vec3::new(x0, 0.0, z0);
            let p10 = Vec3::new(x1, 0.0, z0);
            let p11 = Vec3::new(x1, 0.0, z1);
            let p01 = Vec3::new(x0, 0.0, z1);
            triangles.push([p00, p01, p11]);
            triangles.push([p00, p11, p10]);
        }
    }
    let materials = vec![TriangleMaterial::new(0.95, 0.12); triangles.len()]; // gravel class
    CollisionWorld::from_host_export("ford-thermal-gravel", 1, &triangles, &materials)
        .expect("gravel plane")
}

fn thermal_grip_scale(temp_c: f64, optimal: f64, window: f64, overheat: f64) -> f64 {
    let window = window.max(2.0);
    let low = optimal - window * 0.5;
    let high = optimal + window * 0.5;
    let overheat = overheat.max(high + 2.0);
    let lerp = |a: f64, b: f64, t: f64| a + (b - a) * t.clamp(0.0, 1.0);
    if temp_c <= 20.0 {
        0.68
    } else if temp_c < low {
        lerp(0.68, 1.0, (temp_c - 20.0) / (low - 20.0).max(1.0))
    } else if temp_c <= high {
        1.0
    } else if temp_c < overheat {
        lerp(1.0, 0.94, (temp_c - high) / (overheat - high))
    } else {
        lerp(0.94, 0.78, (temp_c - overheat) / 35.0)
    }
}

fn main() {
    let manifest = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let profile = manifest.join("../data/vehicles/ford-escort-rs2000/vehicle.json");
    if !profile.exists() {
        eprintln!("missing profile {}", profile.display());
        return;
    }
    uninstall_world();
    install_world(flat_gravel_plane(2000.0));

    let mut sim = OpenFormulaSimulation::create_with_world(
        Some(&profile),
        Vec3::new(0.0, 0.65, 0.0),
        0.0,
        Some(45.0),
        true,
    )
    .expect("Ford profile must load on the gravel world");
    let specs = sim.vehicle_specs();
    println!(
        "profile optimal tread={:.1}C window={:.1} overheat={:.1} carcass={:.1}",
        specs.tread_optimal_temperature_c[0],
        specs.tread_optimal_window_c[0],
        specs.tread_overheat_temperature_c[0],
        specs.carcass_optimal_temperature_c[0]
    );
    println!("t,speed,gear,avg_tread,carcass,gas,pressure,grip_scale");

    let dt = 1.0 / 240.0;
    let total: f64 = std::env::args().nth(1).and_then(|v| v.parse().ok()).unwrap_or(40.0);
    let mut t: f64 = 0.0;
    let mut next_sample = 0.0;
    let mut step = 0u64;
    while t < total {
        let input = if t < 2.0 {
            OfpControlInput {
                brake: 1.0,
                clutch: 1.0,
                gear_request: 0,
                ..Default::default()
            }
        } else if t < 5.0 {
            OfpControlInput {
                throttle: 0.75,
                clutch: 0.0,
                gear_request: 2,
                ..Default::default()
            }
        } else {
            // Slalom at rally speed: alternate steering every 2 s.
            let phase = ((t - 5.0) / 2.0).floor() as i32;
            let steer = if phase % 2 == 0 { 0.30 } else { -0.30 };
            OfpControlInput {
                throttle: 0.55,
                steering: steer,
                clutch: 0.0,
                gear_request: std::env::args().nth(2).and_then(|v| v.parse().ok()).unwrap_or(3),
                ..Default::default()
            }
        };
        if sim.step(&input, &[], dt).is_err() {
            println!("rejected at t={t:.2}: {}", sim.last_error());
            break;
        }
        t += dt;
        step += 1;
        if t + 1e-9 >= next_sample {
            next_sample += 5.0;
            let s = sim.snapshot();
            let avg = (0..4)
                .map(|i| s.wheels[i].tread_temperature_c)
                .sum::<f64>()
                / 4.0;
            let carcass = (0..4)
                .map(|i| s.wheels[i].carcass_temperature_c)
                .sum::<f64>()
                / 4.0;
            let gas = (0..4)
                .map(|i| s.wheels[i].gas_temperature_c)
                .sum::<f64>()
                / 4.0;
            let pressure = (0..4)
                .map(|i| s.wheels[i].pressure_kpa_gauge)
                .sum::<f64>()
                / 4.0;
            let grip = thermal_grip_scale(
                avg,
                specs.tread_optimal_temperature_c[0],
                specs.tread_optimal_window_c[0],
                specs.tread_overheat_temperature_c[0],
            );
            let speed = (s.linear_velocity.x.powi(2)
                + s.linear_velocity.y.powi(2)
                + s.linear_velocity.z.powi(2))
            .sqrt();
            println!(
                "{:.1},{:.2},{},{:.1},{:.1},{:.1},{:.1},{:.3}",
                t, speed, s.gear, avg, carcass, gas, pressure, grip
            );
        }
        if step > 20000 {
            break;
        }
    }
    let _ = WheelIndex::ALL;
    uninstall_world();
}


