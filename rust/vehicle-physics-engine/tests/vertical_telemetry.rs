#![allow(dead_code)]
// Compile the diagnostics' own regression tests without unrelated legacy lib fixtures.
mod types { pub use vehicle_physics_engine::types::Vec3; }
#[path = "../src/vertical_diagnostics.rs"]
mod vertical_diagnostics;

#[test]
fn exhausted_body_iterations_keep_candidate_without_test_solver_overrides() {
    use vehicle_physics_engine::{simulation::VehicleSimulator, types::{Vec3, VehicleInput, TriRaycastSample},
        vehicle_config::VehicleConfig, vertical_diagnostics as diag};
    diag::configure(None);
    diag::begin_step(0.0, 0.02, 7);
    let mut sim = VehicleSimulator::new(VehicleConfig::williams_fw19_hockenheim_canonical(), Vec3::ZERO, 0.0);
    // Deliberately poor initial horizontal acceleration guess; axial motion is unaffected.
    // Even 32 relaxed iterations cannot close this finite residual. This drives the real failure path.
    sim.state.linear_acceleration = Vec3::new(1e30, 0.0, 0.0);
    sim.step(&VehicleInput::default(), &[TriRaycastSample::default();4], 0.001);
    assert!(sim.state.suspension.mechanical_failure.is_some());
    assert_eq!(sim.state.suspension.failure.unwrap().code, diag::FailureCode::BodyCornerNotConverged);
    let path = std::env::temp_dir().join(format!("ofp-body-exhaustion-{}.csv",std::process::id()));
    diag::trace_dump(path.to_str().unwrap()).unwrap();
    let csv = std::fs::read_to_string(path).unwrap();
    let rows: Vec<_> = csv.lines().skip(1).map(|line| line.split(',').collect::<Vec<_>>()).collect();
    let rejected = rows.iter().find(|r| r[0] == "4").expect("rejected candidate missing");
    assert_eq!(rejected[6], "0.001000");
    assert_eq!(rejected[7], "0.001000");
    assert_eq!(rejected[8], "7");
    assert!(rows.iter().any(|r| r[0] == "3"));
    diag::disable();
}
