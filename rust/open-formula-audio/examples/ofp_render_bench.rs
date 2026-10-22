//! Micro-benchmark: p50/p95/p99 por bloque de 1024 @44100 en release.
//!
//! Crea el handle Williams, alimenta paquetes estables a 9000 RPM y mide 300
//! renders estereo. No incluye OpenAL: es el coste del mixer aislado.
//!
//! Run: `cargo run -p open_formula_audio --release --offline --example ofp_render_bench`

use std::ffi::CString;
use std::mem::size_of;
use std::path::Path;
use std::time::Instant;

use open_formula_audio::{
    OfpAudioCreateParamsV1, OfpAudioPacketV1, OFP_AUDIO_ABI_VERSION, OFP_AUDIO_SAMPLE_RATE,
    OFP_AUDIO_SURFACE_ASPHALT,
};

const BLOCK: usize = 1024;
const BLOCKS: usize = 300;

fn main() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../sounds");
    let c_root = CString::new(root.to_str().unwrap()).unwrap();
    let params = OfpAudioCreateParamsV1 {
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
    };
    // SAFETY: valid NUL root + valid params; destroyed at the end.
    let handle = unsafe { open_formula_audio::ofp_audio_create(c_root.as_ptr(), &params) };
    assert!(!handle.is_null());
    let packet = OfpAudioPacketV1 {
        schema_version: OFP_AUDIO_ABI_VERSION,
        struct_size: size_of::<OfpAudioPacketV1>() as u32,
        car_index: 0,
        session_generation: 1,
        sequence: 1,
        sim_time_s: 0.01,
        dt_s: 0.01,
        _pad0: 0,
        rpm: 9_000.0,
        throttle: 0.8,
        normalized_engine_load: 0.8,
        normalized_engine_torque: 0.6,
        rpm_derivative: 0.0,
        throttle_derivative: 0.0,
        speed_kph: 180.0,
        slip: 0.02,
        gear: 4,
        torque_sign: 1,
        shift_phase: 0,
        clutch_engagement: 1.0,
        tc_cut_ratio: 0.0,
        rev_limiter_active: 0,
        output_shaft_hz: 90.0,
        has_output_shaft: 1,
        target_gear: 4,
        scrape_active: 0,
        scrape_intensity: 0.0,
        scrape_speed_m_s: 0.0,
        scrape_onset: 0.0,
        tire_slip_ratio: [0.0; 4],
        tire_slip_angle: [0.0; 4],
        tire_contact: [1.0; 4],
        tire_load_n: [3_500.0; 4],
        tire_speed_kph: 180.0,
        surface_code: OFP_AUDIO_SURFACE_ASPHALT,
        reserved: 0,
    };
    // SAFETY: live handle + valid packet.
    assert_eq!(unsafe { open_formula_audio::ofp_audio_push_packet(handle, &packet) }, 0);
    let _ = packet.sequence;

    let mut left = vec![0.0f32; BLOCK];
    let mut right = vec![0.0f32; BLOCK];
    // Warm-up (caches, filtros, envelopes).
    for _ in 0..10 {
        // SAFETY: live handle + BLOCK-frame buffers.
        assert_eq!(
            unsafe { open_formula_audio::ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), BLOCK as u32) },
            BLOCK as u32
        );
    }
    let mut times_ns: Vec<u128> = Vec::with_capacity(BLOCKS);
    for _ in 0..BLOCKS {
        let start = Instant::now();
        // SAFETY: live handle + BLOCK-frame buffers.
        assert_eq!(
            unsafe { open_formula_audio::ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), BLOCK as u32) },
            BLOCK as u32
        );
        times_ns.push(start.elapsed().as_nanos());
    }
    times_ns.sort_unstable();
    let percentile = |q: f64| times_ns[((q * BLOCKS as f64) as usize).min(BLOCKS - 1)] as f64 / 1_000.0;
    let deadline_us = BLOCK as f64 / 44_100.0 * 1e6;
    println!("blocks: {} x {} @44100 (release)", BLOCKS, BLOCK);
    println!(
        "p50 {:.1} us | p95 {:.1} us | p99 {:.1} us | max {:.1} us | deadline {:.0} us",
        percentile(0.5),
        percentile(0.95),
        percentile(0.99),
        *times_ns.last().unwrap() as f64 / 1_000.0,
        deadline_us
    );
    // SAFETY: destroyed once here.
    unsafe { open_formula_audio::ofp_audio_destroy(handle) };
}
