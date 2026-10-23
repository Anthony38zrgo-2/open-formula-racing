//! Read-only probe of the public production suspension path. No production overrides.
use std::{
    fs::File,
    io::{BufWriter, Write},
    path::Path,
};
use vehicle_physics_engine::{
    suspension::{rest_compression_m, SuspensionSystem, WheelSuspensionState},
    suspension_kinematics::{jacobian, solve_corner},
    suspension_setup::adopt_antiroll_design_bar,
    types::{TriRaycastSample, WheelIndex},
    vehicle_config::{load_vehicle_configuration, VehicleConfig},
};

const H: f64 = 1e-5; // API's documented-in-source minimum interval.

fn query(base: &SuspensionSystem, cfg: &VehicleConfig, qs: [f64; 4]) -> SuspensionSystem {
    let mut s = base.clone();
    for w in WheelIndex::ALL {
        let i = w as usize;
        s.wheels[i].suspension_compression_m = rest_compression_m(cfg, w) + qs[i];
        s.wheels[i].unsprung_velocity_m_s = 0.0;
    }
    s.step(cfg, &[TriRaycastSample::default(); 4], H);
    assert!(s.mechanical_failure.is_none(), "{:?}", s.mechanical_failure);
    s
}

fn exact(cfg: &VehicleConfig, w: WheelIndex, q: f64) -> Option<(f64, f64, f64, f64)> {
    let g = cfg.suspension.geometric_suspension.as_ref()?;
    let a = g.axle(w);
    let c = g.corners.get(w);
    let sol = solve_corner(c, q, 0.0, w.is_front(), 0.0, 0.0, 1.0)?;
    let r = jacobian(c, q, 0.0, w.is_front())?.motion_ratio;
    let d =
        (a.spring_free_length_m - a.spring_installed_length_m + sol.damper_compression).max(0.0);
    Some((
        a.spring_rate_N_per_m * d * r,
        r,
        sol.damper_compression,
        0.5 * a.spring_rate_N_per_m * d * d,
    ))
}

fn spring_energy(cfg: &VehicleConfig, w: WheelIndex, x: &WheelSuspensionState) -> f64 {
    let a = cfg
        .suspension
        .geometric_suspension
        .as_ref()
        .unwrap()
        .axle(w);
    let d =
        (a.spring_free_length_m - a.spring_installed_length_m + x.damper_compression_m).max(0.0);
    0.5 * a.spring_rate_N_per_m * d * d
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let out = Path::new(args.get(1).expect("output directory"));
    let mut cfg=load_vehicle_configuration(Path::new("D:/open-formula/distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/vehicle.json"))?;
    adopt_antiroll_design_bar(&mut cfg);
    let mut base = SuspensionSystem::new(&cfg);
    base.step(&cfg, &[TriRaycastSample::default(); 4], H); // initialize contact history, no plane.
    let mut curves = BufWriter::new(File::create(out.join("spring_curve.csv"))?);
    writeln!(curves,"wheel,q_input_m,q_end_m,compression_m,motion_ratio,spring_n,damper_compression_m,k_spring_fd_n_m,k_reported_n_m,k_reference_n_m,exact_spring_n,exact_motion_ratio,exact_s_m,energy_gradient_n,energy_gradient_error_n,exact_energy_gradient_error_n")?;
    let e = 1e-6;
    for w in WheelIndex::ALL {
        let i = w as usize;
        let env = base.prepared_geometric().unwrap().for_wheel(w);
        eprintln!(
            "ENVELOPE {w:?} q_min={} q_max={} k_reference={}",
            env.q_min, env.q_max, env.k_wheel_rest_n_per_m
        );
        for n in -80..=29 {
            let q = n as f64 * 0.001;
            if q <= env.q_min + e || q >= env.q_max - e {
                continue;
            }
            let mut qs = [-0.055, -0.055, -0.05, -0.05];
            qs[i] = q;
            let mid = query(&base, &cfg, qs);
            qs[i] = q + e;
            let plus = query(&base, &cfg, qs);
            qs[i] = q - e;
            let minus = query(&base, &cfg, qs);
            let x = &mid.wheels[i];
            let xp = &plus.wheels[i];
            let xm = &minus.wheels[i];
            let dq = xp.suspension_compression_m - xm.suspension_compression_m;
            let tangent = (xp.spring_force - xm.spring_force) / dq;
            let grad = (spring_energy(&cfg, w, xp) - spring_energy(&cfg, w, xm)) / dq;
            let qend = x.suspension_compression_m - rest_compression_m(&cfg, w);
            let (ef, er, es, _) = exact(&cfg, w, qend).unwrap();
            let exact_grad = (exact(&cfg, w, qend + e).unwrap().3
                - exact(&cfg, w, qend - e).unwrap().3)
                / (2.0 * e);
            writeln!(curves,"{w:?},{q:0.9},{qend:0.9},{:0.9},{:0.9},{:0.9},{:0.9},{tangent:0.6},{:0.6},{:0.6},{ef:0.9},{er:0.9},{es:0.9},{grad:0.9},{:0.9},{:0.9}",x.suspension_compression_m,x.motion_ratio,x.spring_force,x.damper_compression_m,x.tangent_wheel_rate_n_per_m,env.k_wheel_rest_n_per_m,x.spring_force-grad,ef-exact_grad)?;
        }
    }
    let mut modal = BufWriter::new(File::create(out.join("modal_stiffness.csv"))?);
    writeln!(modal,"scale,axle,q_mean_m,ratio,k_bar_n_m,k_spring_local_n_m,bar_roll_share_pct,bar_sym_sum_n,bar_heave_n,bar_difference_m,bar_diff_force_n,bar_energy_j")?;
    for scale in [1., 0.5, 0.3, 0.2, 0.] {
        let mut c = cfg.clone();
        c.suspension.front_arb_ratio = scale;
        c.suspension.rear_arb_ratio = 0.8 * scale;
        adopt_antiroll_design_bar(&mut c);
        let b = SuspensionSystem::new(&c);
        for (axle, q, ratio) in [
            (0, -0.055, scale),
            (0, -0.045, scale),
            (0, 0., scale),
            (1, -0.05, 0.8 * scale),
            (1, 0., 0.8 * scale),
        ] {
            let i = axle * 2;
            let mut qs = [-0.055, -0.055, -0.05, -0.05];
            qs[i] = q;
            qs[i + 1] = q;
            let symmetric = query(&b, &c, qs);
            let mut qp = qs;
            qp[i] += e;
            let p = query(&b, &c, qp);
            let mut qm = qs;
            qm[i] -= e;
            let m = query(&b, &c, qm);
            let kspring = (p.wheels[i].spring_force - m.wheels[i].spring_force)
                / (p.wheels[i].suspension_compression_m - m.wheels[i].suspension_compression_m);
            qs[i] += 0.0075;
            qs[i + 1] -= 0.0075;
            let s = query(&b, &c, qs);
            let delta =
                s.wheels[i].suspension_compression_m - s.wheels[i + 1].suspension_compression_m;
            let kbar = s.axles[axle].antiroll_stiffness_n_per_m;
            let a = s.wheels[i].antiroll_force;
            let z = s.wheels[i + 1].antiroll_force;
            writeln!(modal,"{scale},{axle},{q},{ratio},{kbar:0.6},{kspring:0.6},{:0.6},{:0.9},{:0.9},{delta:0.9},{a:0.9},{:0.9}",100.*2.*kbar/(kspring+2.*kbar),a+z,symmetric.wheels[i].antiroll_force,if axle==0{s.arb_energy_front_j}else{s.arb_energy_rear_j})?;
        }
    }
    Ok(())
}
