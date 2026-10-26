//! Physical acceptance gates for the Williams 35/40-mm suspension candidate.
use std::{path::PathBuf, sync::OnceLock};
use vehicle_physics_engine::{
    open_formula::{install_track_ground_mesh, OpenFormulaSimulation},
    open_formula_ffi::OfpControlInput,
    simulation::default_spawn_height,
    suspension::{rest_compression_m, SuspensionSystem},
    suspension_setup::adopt_antiroll_design_bar,
    types::{TriRaycastSample, Vec3, WheelIndex},
    vehicle_config::VehicleConfig,
    wheel_mechanics::WheelMechanicalTuning,
};

fn config() -> VehicleConfig {
    let mut c = VehicleConfig::williams_fw19_hockenheim_canonical();
    adopt_antiroll_design_bar(&mut c);
    c
}

fn plane() {
    static INIT: OnceLock<()> = OnceLock::new();
    INIT.get_or_init(|| {
        let p: PathBuf = std::env::temp_dir().join(format!("ofp-35-40-plane-{}.txt", std::process::id()));
        std::fs::write(&p, "FORMULA_GROUND_1 2\n1.0 0.001 -1000 -1000 0 1000 -1000 0 1000 1000 0\n1.0 0.001 -1000 -1000 0 1000 1000 0 -1000 1000 0\n").unwrap();
        let (identity, _generation) =
            install_track_ground_mesh(p.to_str().unwrap(), Some("acceptance-plane"), Some(1)).unwrap();
        assert_eq!(identity.triangle_count, 2);
    });
}

#[test]
fn physical_static_ride_is_35_40_without_stop_support() {
    plane();
    let c = config();
    let mut sim = OpenFormulaSimulation::create(None, Vec3::new(0., default_spawn_height(&c), 0.), 0., Some(7.6)).unwrap();
    let input = OfpControlInput { clutch: 1., gear_request: 0, ..Default::default() };
    for _ in 0..2000 { sim.step(&input, &[], 0.002).unwrap(); }
    let s = sim.snapshot();
    let (origin, _, _, z) = sim.pose();
    let (_, _, y, _) = sim.pose();
    let g = c.suspension.geometric_suspension.as_ref().unwrap();
    let floor = &c.aerodynamics.aero_model.underfloor;
    let measured = [
        origin.y + y.y * (-default_spawn_height(&c) + floor.floor_clearance_front_m.unwrap()) + z.y * g.corners.fl.hub_center.z,
        origin.y + y.y * (-default_spawn_height(&c) + floor.floor_clearance_rear_m.unwrap()) + z.y * g.corners.rl.hub_center.z,
    ];
    assert!((measured[0] - 0.035).abs() < 0.0003, "front ride: {:?}", measured);
    assert!((measured[1] - 0.040).abs() < 0.0003, "rear ride: {:?}", measured);
    assert!(s.wheels.iter().all(|w| w.normal_force_n > 1000.));
    assert!(s.wheel_diagnostics.iter().all(|w| w.stop_force_n.abs() < 0.1));
    assert!(s.floor_impulse_ns.iter().all(|j| j.abs() < 1e-9));
}

fn evaluation(c: &VehicleConfig, q: [f64;4]) -> SuspensionSystem {
    let mut s = SuspensionSystem::new(c);
    // Initialize history without a ground plane, then place the independent DOFs.
    s.step(c, &[TriRaycastSample::default();4], 1e-5);
    for w in WheelIndex::ALL {
        s.wheels[w as usize].suspension_compression_m = rest_compression_m(c,w) + q[w as usize];
        s.wheels[w as usize].unsprung_velocity_m_s = 0.;
    }
    s.step(c, &[TriRaycastSample::default();4], 1e-5);
    assert!(s.mechanical_failure.is_none());
    s
}

#[test]
fn progressive_stops_only_start_in_the_last_five_millimetres() {
    let c = config();
    let s = SuspensionSystem::new(&c);
    for w in [WheelIndex::FrontLeft, WheelIndex::RearLeft] {
        let i=w as usize;
        let max=s.prepared_geometric().unwrap().for_wheel(w).q_max;
        let floor=&c.aerodynamics.aero_model.underfloor;
        let datum=if w.is_front(){floor.floor_clearance_front_m.unwrap()}else{floor.floor_clearance_rear_m.unwrap()};
        assert!((datum-max-0.005).abs()<1e-9,"full bump must retain 5 mm floor clearance");
        let mut before=[0.;4];before[i]=max-0.005-0.00025;
        let mut after=before;after[i]=max-0.005+0.00025;
        assert!(evaluation(&c,before).wheels[i].bottom_out_force.abs()<1e-6);
        // C1 bump stop: the entry slope is zero by construction, so 0.25 mm into
        // the 5 mm working length produces ~0.4 N instead of the ~152 N of the
        // linear entry. It must still be strictly positive (the stop starts in
        // the last 5 mm, not later) and far below the full-stroke force.
        let entered = evaluation(&c,after).wheels[i].bottom_out_force;
        assert!(entered > 0., "{w:?}: stop must start in the last 5 mm, got {entered}");
        assert!(entered < 5., "{w:?}: C1 entry force must stay small, got {entered}");
        assert_eq!(WheelMechanicalTuning::for_wheel(&c,w).hard_stop_overtravel_m,0.0001);
    }
}

#[test]
fn bars_are_secondary_to_the_main_springs_at_the_static_working_pose() {
    let c=config();
    for (w,q) in [(WheelIndex::FrontLeft,-0.005),(WheelIndex::RearLeft,0.005)] {
        let i=w as usize;let e=1e-6;let mut low=[-0.005,-0.005,0.005,0.005];let mut high=low;
        low[i]=q-e;high[i]=q+e;
        let a=evaluation(&c,low);let b=evaluation(&c,high);
        let k=(b.wheels[i].spring_force-a.wheels[i].spring_force)/(b.wheels[i].suspension_compression_m-a.wheels[i].suspension_compression_m);
        let kb=b.axles[i/2].antiroll_stiffness_n_per_m;
        let fraction=2.*kb/(k+2.*kb);
        assert!(fraction<0.4,"{w:?}: bar share {fraction}");
    }
}

#[test]
fn stop_lengths_survive_profile_roundtrip_and_reject_invalid_values() {
    let c=config();
    let text=c.to_resolved_configuration_value().to_string();
    let restored=VehicleConfig::from_json_str(&text).unwrap();
    let g=restored.suspension.geometric_suspension.as_ref().unwrap();
    for a in [&g.front,&g.rear] {
        assert_eq!(a.bump_stop_travel_m,Some(0.005));
        assert_eq!(a.hard_stop_overtravel_m,Some(0.0001));
    }
    let mut bad=g.clone();bad.front.bump_stop_travel_m=Some(0.);
    assert!(bad.validate().is_err());
    bad.front.bump_stop_travel_m=Some(f64::NAN);
    assert!(bad.validate().is_err());
    bad=g.clone();bad.rear.hard_stop_overtravel_m=Some(-0.001);
    assert!(bad.validate().is_err());
}

#[test]
fn old_profiles_keep_their_unspecified_stop_tuning() {
    let c=VehicleConfig::from_json_str(include_str!("fixtures/williams_suspension_pre_35_40.json")).unwrap();
    for w in WheelIndex::ALL {
        let a=c.suspension.geometric_suspension.as_ref().unwrap().axle(w);
        assert!(a.bump_stop_travel_m.is_none());
        assert!(a.hard_stop_overtravel_m.is_none());
        assert_eq!(WheelMechanicalTuning::for_wheel(&c,w).hard_stop_overtravel_m,0.030);
    }
    let value=c.to_resolved_configuration_value();
    assert!(value["suspension"]["geometry_physical"]["front"].get("bump_stop_travel_m").is_none());
}
