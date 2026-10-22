//! Etapa 3 probe: Williams simulation publishes ABI v6 audio-mechanical state.
//!
//! Creates the canonical Williams profile, takes accepted steps at 60 Hz and
//! at low-FPS 20 Hz, and prints the v6 snapshot tail (sequence, dt, clutch
//! torque, TC, limiter, shift timer) plus the audio specs. Fails loudly on
//! any mismatch, never falls back.
//!
//! Run: `cargo run -p vehicle_physics_engine --offline --example ofp_audio_v6_probe`

use vehicle_physics_engine::open_formula::{OpenFormulaSimulation, FLOOR_PROBE_COUNT};
use vehicle_physics_engine::open_formula_ffi::{
    OFP_ABI_VERSION, OfpBodyContact, OfpControlInput, OfpSnapshot, OfpVehicleSpecs,
};
use vehicle_physics_engine::types::Vec3;

fn neutral_input() -> OfpControlInput {
    OfpControlInput {
        throttle: 0.15,
        brake: 0.0,
        handbrake: 0.0,
        clutch: 0.0,
        steering: 0.0,
        gear_request: 1,
        reserved: 0,
    }
}

fn main() {
    assert_eq!(OFP_ABI_VERSION, 6, "physics ABI must be v6 for audio presentation");
    // Layout locks: the v6 tail is appended, so every v5 offset is preserved.
    // (Mirrors `rust_abi.h`; the loader signature hashes these lengths.)
    assert_eq!(std::mem::offset_of!(OfpVehicleSpecs, max_torque_nm), 384);
    assert_eq!(std::mem::offset_of!(OfpVehicleSpecs, front_torque_split), 392);
    assert_eq!(std::mem::size_of::<OfpVehicleSpecs>(), 400);
    assert_eq!(std::mem::offset_of!(OfpSnapshot, clutch_torque_nm), 2168);
    assert_eq!(std::mem::offset_of!(OfpSnapshot, sequence), 2208);
    println!(
        "layout: specs={} snapshot={} wheels_at={} step_tail={}..{}",
        std::mem::size_of::<OfpVehicleSpecs>(),
        std::mem::size_of::<OfpSnapshot>(),
        std::mem::offset_of!(OfpSnapshot, wheels),
        std::mem::offset_of!(OfpSnapshot, clutch_torque_nm),
        std::mem::offset_of!(OfpSnapshot, sequence),
    );
    let mut simulation =
        OpenFormulaSimulation::create(None, Vec3::ZERO, 0.0, None).expect("williams profile");
    let specs = simulation.vehicle_specs();
    assert_eq!(specs.idle_rpm, 3_500.0);
    assert_eq!(specs.max_rpm, 17_500.0);
    assert!(specs.max_torque_nm > 100.0, "max_torque {}", specs.max_torque_nm);
    assert_eq!(specs.front_torque_split, 0.0);
    assert_eq!(specs.final_drive, 4.65);
    assert_eq!(FLOOR_PROBE_COUNT, 2);

    let input = neutral_input();
    let empty: &[OfpBodyContact] = &[];
    simulation.step(&input, empty, 0.01).expect("step 1");
    simulation.step(&input, empty, 0.01).expect("step 2");
    let snapshot = simulation.snapshot();
    assert_eq!(snapshot.abi_version, 6);
    assert_eq!(snapshot.sequence, 2, "sequence must count accepted steps");
    assert!((snapshot.step_dt_s - 0.01).abs() < 1e-12, "dt {}", snapshot.step_dt_s);
    assert!(snapshot.engine_rpm.is_finite());
    assert!(snapshot.clutch_torque_nm.is_finite());
    assert!((0.0..=1.0).contains(&snapshot.tc_cut_ratio_smoothed));
    assert!(snapshot.shift_timer_s >= 0.0);
    assert!(snapshot.is_rev_limited <= 1);
    println!(
        "60Hz: seq={} dt={:.4} rpm={:.1} clutch_tq={:.1} tc={:.3} limiter={} shift_t={:.3}",
        snapshot.sequence,
        snapshot.step_dt_s,
        snapshot.engine_rpm,
        snapshot.clutch_torque_nm,
        snapshot.tc_cut_ratio_smoothed,
        snapshot.is_rev_limited,
        snapshot.shift_timer_s
    );

    // Low-FPS frame: one accepted step carries its own 50 ms clock.
    simulation.step(&input, empty, 0.05).expect("low-fps step");
    let slow = simulation.snapshot();
    assert_eq!(slow.sequence, 3);
    assert!((slow.step_dt_s - 0.05).abs() < 1e-12, "dt {}", slow.step_dt_s);
    assert_eq!(slow.physics_hz, 20);
    println!(
        "20Hz: seq={} dt={:.4} hz={} limiter={} shift_t={:.3}",
        slow.sequence, slow.step_dt_s, slow.physics_hz, slow.is_rev_limited, slow.shift_timer_s
    );
    println!("OFP_AUDIO_V6_PROBE_OK");
}
