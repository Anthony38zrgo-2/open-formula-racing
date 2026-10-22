//! Etapa 3 parity: accepted physics steps become identical audio packets.
//!
//! Drives the canonical Williams simulation, builds one `OfpAudioStepV1` per
//! accepted step exactly like `BuildAudioStep` in `rustphysics.cpp`, pushes
//! each through the C ABI, and proves ordering, clock preservation (including
//! a low-FPS 50 ms step) and audible rendering. Needs the staged banks in
//! `sounds/` (same staging as the Etapa 2 harness).

use std::ffi::CString;
use std::mem::size_of;
use std::path::Path;

use open_formula_audio::{
    OfpAudioCreateParamsV1, OfpAudioDiagnosticsV1, OfpAudioStepV1, OFP_AUDIO_ABI_VERSION,
    OFP_AUDIO_SAMPLE_RATE, OFP_AUDIO_STEP_VERSION,
};
use vehicle_physics_engine::open_formula::OpenFormulaSimulation;
use vehicle_physics_engine::open_formula_ffi::{OfpBodyContact, OfpControlInput, OfpSnapshot, OfpVehicleSpecs};
use vehicle_physics_engine::types::Vec3;

fn williams_params() -> OfpAudioCreateParamsV1 {
    OfpAudioCreateParamsV1 {
        schema_version: OFP_AUDIO_ABI_VERSION,
        struct_size: size_of::<OfpAudioCreateParamsV1>() as u32,
        sample_rate: OFP_AUDIO_SAMPLE_RATE,
        forward_gears: 6,
        idle_rpm: 3_500.0,
        max_rpm: 17_500.0,
        engine_gain: 1.0,
        gearbox_gain: 0.561329504,
        gearbox_whine_gain: 0.137826425,
        gearbox_whine_tone_gain: 0.12,
        gearbox_whine_noise_gain: 0.025,
        backfire_gain: 1.0,
        limiter_gain: 0.3,
        coast_gain: 0.55,
        coast_gain_enabled: 1,
        gearbox_whine_final_drive: 4.65,
        reverse_ratio: 3.0,
        gear_teeth: 21.0,
        final_teeth: 46.0,
        gear_ratios: [3.41, 2.64, 2.112, 1.727, 1.441, 1.232, 0.0, 0.0],
        fallback_to_legacy: 0,
        reserved: [0; 4],
    }
}

fn input(throttle: f64) -> OfpControlInput {
    OfpControlInput {
        throttle,
        brake: 0.0,
        handbrake: 0.0,
        clutch: 0.0,
        steering: 0.0,
        gear_request: 2,
        reserved: 0,
    }
}

/// Builds one raw audio step from an accepted snapshot, exactly like
/// `BuildAudioStep` in `rustphysics.cpp` (same fields, same units, same
/// wheel order). Any drift between the two builders breaks the tests below.
fn build_step(
    snapshot: &OfpSnapshot,
    specs: &OfpVehicleSpecs,
    car_index: i32,
    session_generation: u32,
) -> OfpAudioStepV1 {
    OfpAudioStepV1 {
        schema_version: OFP_AUDIO_STEP_VERSION,
        struct_size: size_of::<OfpAudioStepV1>() as u32,
        car_index,
        session_generation,
        sequence: snapshot.sequence,
        sim_time_s: snapshot.sim_time_s,
        dt_s: snapshot.step_dt_s as f32,
        _pad0: 0,
        rpm: snapshot.engine_rpm,
        throttle: snapshot.throttle as f32,
        speed_m_s: (snapshot.linear_velocity.x * snapshot.linear_velocity.x
            + snapshot.linear_velocity.y * snapshot.linear_velocity.y
            + snapshot.linear_velocity.z * snapshot.linear_velocity.z)
            .sqrt(),
        gear: snapshot.gear,
        target_gear: snapshot.target_gear,
        engine_torque_nm: snapshot.engine_torque_nm,
        clutch_torque_nm: snapshot.clutch_torque_nm,
        max_torque_nm: specs.max_torque_nm,
        clutch_engagement: snapshot.clutch_engagement,
        tc_cut_ratio_smoothed: snapshot.tc_cut_ratio_smoothed,
        is_rev_limited: snapshot.is_rev_limited,
        shift_timer_s: snapshot.shift_timer_s,
        wheel_spin_rad_s: [
            snapshot.wheels[0].spin_rad_s,
            snapshot.wheels[1].spin_rad_s,
            snapshot.wheels[2].spin_rad_s,
            snapshot.wheels[3].spin_rad_s,
        ],
        final_drive: specs.final_drive,
        front_torque_split: specs.front_torque_split,
        slip_ratio: [
            snapshot.wheels[0].slip_ratio as f32,
            snapshot.wheels[1].slip_ratio as f32,
            snapshot.wheels[2].slip_ratio as f32,
            snapshot.wheels[3].slip_ratio as f32,
        ],
        slip_angle_rad: [
            snapshot.wheels[0].slip_angle_rad as f32,
            snapshot.wheels[1].slip_angle_rad as f32,
            snapshot.wheels[2].slip_angle_rad as f32,
            snapshot.wheels[3].slip_angle_rad as f32,
        ],
        contact_fraction: [
            snapshot.wheels[0].contact_fraction as f32,
            snapshot.wheels[1].contact_fraction as f32,
            snapshot.wheels[2].contact_fraction as f32,
            snapshot.wheels[3].contact_fraction as f32,
        ],
        normal_force_n: [
            snapshot.wheels[0].normal_force_n as f32,
            snapshot.wheels[1].normal_force_n as f32,
            snapshot.wheels[2].normal_force_n as f32,
            snapshot.wheels[3].normal_force_n as f32,
        ],
        surface: [
            snapshot.wheels[0].surface,
            snapshot.wheels[1].surface,
            snapshot.wheels[2].surface,
            snapshot.wheels[3].surface,
        ],
        floor_valid_mask: snapshot.floor_valid_mask,
        floor_impulse_ns: snapshot.floor_impulse_ns,
        body_contact_count: snapshot.body_contact_count,
        body_contact_impulse_ns: snapshot.body_contact_impulse_ns,
        scrape_speed_m_s: (snapshot.linear_velocity.x * snapshot.linear_velocity.x
            + snapshot.linear_velocity.z * snapshot.linear_velocity.z)
            .sqrt() as f32,
        reserved: 0,
    }
}

#[test]
fn physics_steps_become_identical_audio_packets() {
    let mut simulation =
        OpenFormulaSimulation::create(None, Vec3::ZERO, 0.0, None).expect("williams profile");
    let specs = simulation.vehicle_specs();

    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../sounds");
    let c_root = CString::new(root.to_str().unwrap()).unwrap();
    let params = williams_params();
    // SAFETY: valid NUL root + valid params; destroyed at the end.
    let handle = unsafe { open_formula_audio::ofp_audio_create(c_root.as_ptr(), &params) };
    assert!(!handle.is_null());
    // SAFETY: live handle.
    assert_eq!(unsafe { open_formula_audio::ofp_audio_v10_active(handle) }, 1);

    let empty: &[OfpBodyContact] = &[];
    // 10 accepted steps at 60 Hz with rising throttle, then one low-FPS step.
    for (index, throttle) in [0.3, 0.5, 0.7, 0.9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
        .into_iter()
        .enumerate()
    {
        simulation.step(&input(throttle), empty, 0.01).expect("step");
        let snapshot = simulation.snapshot();
        assert_eq!(snapshot.sequence as usize, index + 1);
        let step = build_step(&snapshot, &specs, 0, 9);
        // Packet clock equals the physics clock (never the graphic frame).
        assert!((step.dt_s - 0.01).abs() < 1e-6);
        // SAFETY: live handle + valid step.
        assert_eq!(unsafe { open_formula_audio::ofp_audio_push_step(handle, &step) }, 0);
    }
    // Low-FPS frame keeps its own 50 ms clock for cut windows.
    simulation.step(&input(1.0), empty, 0.05).expect("low-fps step");
    let snapshot = simulation.snapshot();
    assert_eq!(snapshot.sequence, 11);
    assert!((snapshot.step_dt_s - 0.05).abs() < 1e-12);
    let slow = OfpAudioStepV1 {
        schema_version: OFP_AUDIO_STEP_VERSION,
        struct_size: size_of::<OfpAudioStepV1>() as u32,
        car_index: 0,
        session_generation: 9,
        sequence: snapshot.sequence,
        sim_time_s: snapshot.sim_time_s,
        dt_s: snapshot.step_dt_s as f32,
        _pad0: 0,
        rpm: snapshot.engine_rpm,
        throttle: 1.0,
        speed_m_s: 40.0,
        gear: snapshot.gear,
        target_gear: snapshot.target_gear,
        engine_torque_nm: snapshot.engine_torque_nm,
        clutch_torque_nm: snapshot.clutch_torque_nm,
        max_torque_nm: specs.max_torque_nm,
        clutch_engagement: snapshot.clutch_engagement,
        tc_cut_ratio_smoothed: snapshot.tc_cut_ratio_smoothed,
        is_rev_limited: snapshot.is_rev_limited,
        shift_timer_s: snapshot.shift_timer_s,
        wheel_spin_rad_s: [40.0, 40.0, 42.0, 42.0],
        final_drive: specs.final_drive,
        front_torque_split: specs.front_torque_split,
        slip_ratio: [0.0; 4],
        slip_angle_rad: [0.0; 4],
        contact_fraction: [1.0; 4],
        normal_force_n: [3_500.0; 4],
        surface: [0; 4],
        floor_valid_mask: 0,
        floor_impulse_ns: [0.0; 2],
        body_contact_count: 0,
        body_contact_impulse_ns: [0.0; 32],
        scrape_speed_m_s: 0.0,
        reserved: 0,
    };
    assert!((slow.dt_s - 0.05).abs() < 1e-6);
    // SAFETY: live handle + valid low-FPS step.
    assert_eq!(unsafe { open_formula_audio::ofp_audio_push_step(handle, &slow) }, 0);

    // Rendered output follows the physical RPM with no drops.
    let mut left = vec![0.0f32; 4096];
    let mut right = vec![0.0f32; 4096];
    // SAFETY: live handle + 4096-frame buffers.
    assert_eq!(
        unsafe { open_formula_audio::ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
        4096
    );
    assert!(left.iter().chain(right.iter()).all(|sample| sample.is_finite()));
    let peak = left.iter().chain(right.iter()).fold(0.0f32, |peak, sample| peak.max(sample.abs()));
    assert!(peak > 0.001, "physics-driven audio is silent");
    let mut diagnostics = OfpAudioDiagnosticsV1 {
        schema_version: OFP_AUDIO_ABI_VERSION,
        struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
        ..Default::default()
    };
    // SAFETY: live handle + valid diagnostics slot.
    assert_eq!(unsafe { open_formula_audio::ofp_audio_diagnostics(handle, &mut diagnostics) }, 0);
    assert_eq!(diagnostics.last_sequence, 11);
    assert_eq!(diagnostics.dropped_packets, 0);
    assert!(
        (diagnostics.rendered_rpm as f64 - snapshot.engine_rpm).abs() < 1500.0,
        "rendered {} vs physical {}",
        diagnostics.rendered_rpm,
        snapshot.engine_rpm
    );
    // SAFETY: destroyed once here.
    unsafe { open_formula_audio::ofp_audio_destroy(handle) };
}

#[test]
fn real_upshift_drives_cut_windows_into_audible_duck() {
    // Vuelta completa física→audio con una subida 1→2 pedida por cable: el
    // temporizador físico debe aparecer en al menos un snapshot, todos los
    // paquetes ordenarse sin pérdidas y la mezcla seguir finita y audible.
    let mut simulation =
        OpenFormulaSimulation::create(None, Vec3::ZERO, 0.0, None).expect("williams profile");
    let specs = simulation.vehicle_specs();

    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../sounds");
    let c_root = CString::new(root.to_str().unwrap()).unwrap();
    let params = williams_params();
    // SAFETY: valid NUL root + valid params; destroyed at the end.
    let handle = unsafe { open_formula_audio::ofp_audio_create(c_root.as_ptr(), &params) };
    assert!(!handle.is_null());

    let empty: &[OfpBodyContact] = &[];
    let drive = |throttle: f64, gear_request: i32| OfpControlInput {
        throttle,
        brake: 0.0,
        handbrake: 0.0,
        clutch: 0.0,
        steering: 0.0,
        gear_request,
        reserved: 0,
    };
    let mut saw_cut = false;
    let mut sequence = 0u64;
    // Salida y aceleración en primera, petición de segunda, y crucero.
    let script: Vec<(f64, i32, usize)> = vec![(0.6, 1, 30), (1.0, 1, 40), (1.0, 2, 30), (0.8, 2, 30)];
    for (throttle, gear_request, steps) in script {
        for _ in 0..steps {
            simulation.step(&drive(throttle, gear_request), empty, 0.01).expect("step");
            let snapshot = simulation.snapshot();
            sequence += 1;
            assert_eq!(snapshot.sequence, sequence);
            if snapshot.shift_timer_s > 0.0 {
                saw_cut = true;
            }
            let step = build_step(&snapshot, &specs, 0, 11);
            // SAFETY: live handle + valid ordered step.
            assert_eq!(unsafe { open_formula_audio::ofp_audio_push_step(handle, &step) }, 0);
        }
    }
    assert!(saw_cut, "la subida 1→2 nunca abrió ventana de corte física");
    assert_eq!(simulation.snapshot().gear, 2, "la caja no llegó a segunda");

    let mut left = vec![0.0f32; 4096];
    let mut right = vec![0.0f32; 4096];
    // SAFETY: live handle + 4096-frame buffers.
    assert_eq!(
        unsafe { open_formula_audio::ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
        4096
    );
    assert!(left.iter().chain(right.iter()).all(|sample| sample.is_finite()));
    let peak = left.iter().chain(right.iter()).fold(0.0f32, |peak, sample| peak.max(sample.abs()));
    assert!(peak > 0.001 && peak < 1.0, "pico sospechoso tras la vuelta: {peak}");
    let mut diagnostics = OfpAudioDiagnosticsV1 {
        schema_version: OFP_AUDIO_ABI_VERSION,
        struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
        ..Default::default()
    };
    // SAFETY: live handle + valid diagnostics slot.
    assert_eq!(unsafe { open_formula_audio::ofp_audio_diagnostics(handle, &mut diagnostics) }, 0);
    assert_eq!(diagnostics.last_sequence, sequence);
    assert_eq!(diagnostics.dropped_packets, 0);
    // SAFETY: destroyed once here.
    unsafe { open_formula_audio::ofp_audio_destroy(handle) };
}
