//! Offline replay harness (Etapa 2): Williams/Fuji profile through the C ABI.
//!
//! Exercises `open_formula_audio` exactly as `snddefault` will: explicit assets
//! root + explicit profile, ordered timed packets, 1024-frame PCM blocks at
//! 44100 Hz, diagnostics without allocation in the hot path. Writes per-segment
//! stereo WAVs + a manifest JSON to `implementation/reports/audio_etapa2/`.
//!
//! Run: `cargo run -p open_formula_audio --offline --example ofp_v10_replay`

use std::ffi::CString;
use std::mem::size_of;
use std::path::{Path, PathBuf};

use open_formula_audio::{
    OfpAudioCreateParamsV1, OfpAudioDiagnosticsV1, OfpAudioPacketV1, OFP_AUDIO_ABI_VERSION,
    OFP_AUDIO_SAMPLE_RATE, OFP_AUDIO_SURFACE_ASPHALT,
    OFP_AUDIO_SURFACE_GRASS, OFP_AUDIO_SURFACE_RUMBLE, OFP_AUDIO_SURFACE_SAND,
};

const BLOCK: usize = 1024;

fn assets_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../../sounds")
}

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

fn base_packet(sequence: u64, sim_time: f64) -> OfpAudioPacketV1 {
    OfpAudioPacketV1 {
        schema_version: OFP_AUDIO_ABI_VERSION,
        struct_size: size_of::<OfpAudioPacketV1>() as u32,
        car_index: 0,
        session_generation: 7,
        sequence,
        sim_time_s: sim_time,
        dt_s: 0.01,
        _pad0: 0,
        rpm: 3_500.0,
        throttle: 0.15,
        normalized_engine_load: 0.2,
        normalized_engine_torque: 0.1,
        rpm_derivative: 0.0,
        throttle_derivative: 0.0,
        speed_kph: 0.0,
        slip: 0.0,
        gear: 1,
        torque_sign: 1,
        shift_phase: 0,
        clutch_engagement: 1.0,
        tc_cut_ratio: 0.0,
        rev_limiter_active: 0,
        output_shaft_hz: 10.0,
        has_output_shaft: 1,
        target_gear: 1,
        scrape_active: 0,
        scrape_intensity: 0.0,
        scrape_speed_m_s: 0.0,
        scrape_onset: 0.0,
        tire_slip_ratio: [0.0; 4],
        tire_slip_angle: [0.0; 4],
        tire_contact: [1.0; 4],
        tire_load_n: [3_500.0; 4],
        tire_speed_kph: 0.0,
        surface_code: OFP_AUDIO_SURFACE_ASPHALT,
        reserved: 0,
    }
}

fn write_wav_stereo(path: &Path, left: &[f32], right: &[f32]) {
    assert_eq!(left.len(), right.len());
    let frames = left.len() as u32;
    let data_bytes = frames * 2 * 2;
    let mut file = std::fs::File::create(path).expect("create wav");
    use std::io::Write;
    // Minimal 44-byte PCM16 stereo header.
    file.write_all(b"RIFF").unwrap();
    file.write_all(&(36 + data_bytes).to_le_bytes()).unwrap();
    file.write_all(b"WAVEfmt ").unwrap();
    file.write_all(&16u32.to_le_bytes()).unwrap();
    file.write_all(&1u16.to_le_bytes()).unwrap();
    file.write_all(&2u16.to_le_bytes()).unwrap();
    file.write_all(&44_100u32.to_le_bytes()).unwrap();
    file.write_all(&(44_100u32 * 2 * 2).to_le_bytes()).unwrap();
    file.write_all(&4u16.to_le_bytes()).unwrap();
    file.write_all(&16u16.to_le_bytes()).unwrap();
    file.write_all(b"data").unwrap();
    file.write_all(&data_bytes.to_le_bytes()).unwrap();
    for (l, r) in left.iter().zip(right.iter()) {
        let li = (l.clamp(-1.0, 1.0) * 32767.0) as i16;
        let ri = (r.clamp(-1.0, 1.0) * 32767.0) as i16;
        file.write_all(&li.to_le_bytes()).unwrap();
        file.write_all(&ri.to_le_bytes()).unwrap();
    }
}

fn peak_rms(samples: &[f32]) -> (f32, f32) {
    let peak = samples.iter().fold(0.0f32, |peak, sample| peak.max(sample.abs()));
    let rms = (samples.iter().map(|sample| sample * sample).sum::<f32>() / samples.len().max(1) as f32).sqrt();
    (peak, rms)
}

fn main() {
    let out_dir =
        Path::new(env!("CARGO_MANIFEST_DIR")).join("../../implementation/reports/audio_etapa2");
    std::fs::create_dir_all(&out_dir).expect("create etapa2 dir");

    let root = assets_root();
    let root_string = root.to_str().unwrap().to_string();
    let c_root = CString::new(root_string.clone()).unwrap();
    let params = williams_params();
    // SAFETY: valid NUL root + valid params; destroyed at the end.
    let handle = unsafe { open_formula_audio::ofp_audio_create(c_root.as_ptr(), &params) };
    assert!(!handle.is_null(), "V10 creation failed for {root_string}");
    // SAFETY: live handle.
    assert_eq!(unsafe { open_formula_audio::ofp_audio_v10_active(handle) }, 1);
    println!("V10 active: Williams/Fuji, assets {root_string}");

    let mut sequence: u64 = 0;
    let mut sim_time: f64 = 0.0;
    let mut next_packet = |rpm: f64,
                           throttle: f32,
                           gear: i32,
                           shift_phase: i32,
                           limiter: u32,
                           speed_kph: f64,
                           surface: u32,
                           dt: f32|
     -> OfpAudioPacketV1 {
        sequence += 1;
        sim_time += dt as f64;
        let mut packet = base_packet(sequence, sim_time);
        packet.rpm = rpm;
        packet.throttle = throttle;
        packet.normalized_engine_load = throttle.clamp(0.0, 1.0);
        packet.gear = gear;
        packet.target_gear = gear;
        packet.shift_phase = shift_phase;
        packet.rev_limiter_active = limiter;
        packet.speed_kph = speed_kph;
        packet.tire_speed_kph = speed_kph as f32;
        packet.surface_code = surface;
        // Output shaft follows driven rear wheels * final drive (contract check:
        // must NOT be derived from engine RPM alone).
        let wheel_rad_s = speed_kph / 3.6 / 0.33;
        packet.output_shaft_hz = wheel_rad_s.max(5.0) * 4.65 / std::f64::consts::TAU;
        packet.dt_s = dt;
        packet
    };

    struct Segment {
        name: &'static str,
        packets: Vec<OfpAudioPacketV1>,
        blocks: usize,
        trigger: Option<i32>,
    }

    let mut segments: Vec<Segment> = Vec::new();
    // 1. Idle 3500 (variante OFP-V10-3500: debe seguir 3500, no 4500).
    segments.push(Segment {
        name: "01_idle_3500",
        packets: (0..20).map(|_| next_packet(3_500.0, 0.15, 1, 0, 0, 0.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01)).collect(),
        blocks: 86, // ~2.0 s @1024/44100
        trigger: None,
    });
    // 2. ON ramp 3500->17500 (todas las capas ON + salida del medio a 13000).
    let mut ramp_packets = Vec::new();
    for rpm in [4_500.0, 6_500.0, 9_000.0, 11_500.0, 13_000.0, 15_000.0, 17_500.0] {
        for _ in 0..10 {
            ramp_packets.push(next_packet(rpm, 0.9, 3, 0, 0, rpm / 100.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
        }
    }
    segments.push(Segment { name: "02_on_ramp_3500_17500", packets: ramp_packets, blocks: 172, trigger: None });
    // 3. Retention/OFF (soltar acelerador a alto régimen).
    segments.push(Segment {
        name: "03_off_retention",
        packets: (0..30).map(|_| next_packet(14_000.0, 0.0, 4, 0, 0, 220.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01)).collect(),
        blocks: 86,
        trigger: None,
    });
    // 4. N->1 and 1->N (commons neutral_first_gear_transition once per edge).
    let mut shift_packets = Vec::new();
    for _ in 0..5 {
        shift_packets.push(next_packet(3_500.0, 0.1, 0, 0, 0, 0.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    for _ in 0..10 {
        shift_packets.push(next_packet(4_200.0, 0.4, 1, 0, 0, 8.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    for _ in 0..5 {
        shift_packets.push(next_packet(3_800.0, 0.1, 0, 0, 0, 2.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    segments.push(Segment { name: "04_neutral_first", packets: shift_packets, blocks: 86, trigger: None });
    // 5. Upshift with load + flutter (shift_phase=1 cut window).
    let mut upshift_packets = Vec::new();
    for _ in 0..5 {
        upshift_packets.push(next_packet(16_500.0, 1.0, 3, 0, 0, 240.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    for _ in 0..6 {
        upshift_packets.push(next_packet(13_500.0, 0.2, 4, 1, 0, 245.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    for _ in 0..10 {
        upshift_packets.push(next_packet(14_200.0, 0.9, 4, 0, 0, 250.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    segments.push(Segment { name: "05_upshift_cut", packets: upshift_packets, blocks: 86, trigger: None });
    // 5b. Downshift with rev-match (shift_phase=3 cut window).
    let mut downshift_packets = Vec::new();
    for _ in 0..5 {
        downshift_packets.push(next_packet(11_000.0, 0.3, 4, 0, 0, 200.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    for _ in 0..6 {
        downshift_packets.push(next_packet(12_500.0, 0.25, 3, 3, 0, 195.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    for _ in 0..10 {
        downshift_packets.push(next_packet(10_500.0, 0.5, 3, 0, 0, 190.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    segments.push(Segment { name: "05b_downshift_cut", packets: downshift_packets, blocks: 86, trigger: None });
    // 5c. Marcha atrás (whine con reverse_ratio, sin corte).
    let mut reverse_packets = Vec::new();
    for _ in 0..15 {
        reverse_packets.push(next_packet(4_000.0, 0.35, -1, 0, 0, 12.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    segments.push(Segment { name: "05c_reverse", packets: reverse_packets, blocks: 86, trigger: None });
    // 6. Limiter: multiple cuts inside one graphic frame (5x2ms packets).
    let mut limiter_packets = Vec::new();
    for _ in 0..5 {
        limiter_packets.push(next_packet(17_400.0, 1.0, 4, 0, 1, 260.0, OFP_AUDIO_SURFACE_ASPHALT, 0.002));
    }
    for _ in 0..10 {
        limiter_packets.push(next_packet(17_200.0, 1.0, 4, 0, 0, 260.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01));
    }
    segments.push(Segment { name: "06_limiter_windows", packets: limiter_packets, blocks: 86, trigger: None });
    // 7-9. Commons surfaces: grass / piano(rumble) / sand(arena) + scrub per wheel.
    for (name, surface) in [
        ("07_surf_grass", OFP_AUDIO_SURFACE_GRASS),
        ("08_surf_rumble_piano", OFP_AUDIO_SURFACE_RUMBLE),
        ("09_surf_sand", OFP_AUDIO_SURFACE_SAND),
    ] {
        let mut packets = Vec::new();
        for _ in 0..20 {
            let mut packet = next_packet(8_000.0, 0.6, 3, 0, 0, 120.0, surface, 0.01);
            packet.tire_slip_ratio = [0.45; 4];
            packet.tire_contact = [1.0; 4];
            packet.tire_load_n = [3_500.0; 4];
            packets.push(packet);
        }
        segments.push(Segment { name, packets, blocks: 86, trigger: None });
    }
    // 10-13. Impacts + barrier/cone + scrape sostenido.
    for (name, code) in [("10_impact_hit_1", 3), ("11_impact_hit_2", 4), ("12_impact_barrier", 7), ("13_impact_cone", 8)] {
        segments.push(Segment {
            name,
            packets: (0..10).map(|_| next_packet(6_000.0, 0.3, 3, 0, 0, 90.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01)).collect(),
            blocks: 43,
            trigger: Some(code),
        });
    }
    let mut scrape_packets = Vec::new();
    for i in 0..20 {
        let mut packet = next_packet(7_000.0, 0.4, 3, 0, 0, 110.0, OFP_AUDIO_SURFACE_ASPHALT, 0.01);
        packet.scrape_active = 1;
        packet.scrape_intensity = 0.7;
        packet.scrape_speed_m_s = 30.0;
        packet.scrape_onset = if i == 0 { 1.0 } else { 0.0 };
        scrape_packets.push(packet);
    }
    segments.push(Segment { name: "14_scrape_sustained", packets: scrape_packets, blocks: 86, trigger: None });

    let mut report_segments = Vec::new();
    let mut full_left: Vec<f32> = Vec::new();
    let mut full_right: Vec<f32> = Vec::new();
    for segment in &segments {
        for packet in &segment.packets {
            // SAFETY: live handle + valid packet (ordered, single owner).
            let code = unsafe { open_formula_audio::ofp_audio_push_packet(handle, packet) };
            assert_eq!(code, 0, "push failed in {}", segment.name);
        }
        if let Some(code) = segment.trigger {
            // SAFETY: live mutable handle, single owner.
            assert_eq!(unsafe { open_formula_audio::ofp_audio_trigger(handle, code) }, 0);
        }
        let mut seg_left: Vec<f32> = Vec::with_capacity(segment.blocks * BLOCK);
        let mut seg_right: Vec<f32> = Vec::with_capacity(segment.blocks * BLOCK);
        for _ in 0..segment.blocks {
            let mut left = vec![0.0f32; BLOCK];
            let mut right = vec![0.0f32; BLOCK];
            // SAFETY: live handle + BLOCK-frame buffers (<=4096).
            let written = unsafe {
                open_formula_audio::ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), BLOCK as u32)
            };
            assert_eq!(written, BLOCK as u32);
            assert!(left.iter().chain(right.iter()).all(|sample| sample.is_finite()));
            seg_left.extend_from_slice(&left);
            seg_right.extend_from_slice(&right);
        }
        let mut diagnostics = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            ..Default::default()
        };
        // SAFETY: live handle + valid diagnostics slot.
        assert_eq!(unsafe { open_formula_audio::ofp_audio_diagnostics(handle, &mut diagnostics) }, 0);
        let (peak_l, rms_l) = peak_rms(&seg_left);
        let (peak_r, rms_r) = peak_rms(&seg_right);
        let peak = peak_l.max(peak_r);
        println!(
            "{}: peak {:.3} rms {:.4} rendered {:.0} gate {:.3} zones {} voices_accepted/suppressed {}/{} dropped {}",
            segment.name,
            peak,
            rms_l.max(rms_r),
            diagnostics.rendered_rpm,
            diagnostics.gate,
            diagnostics.active_zones,
            diagnostics.accepted_events,
            diagnostics.suppressed_events,
            diagnostics.dropped_packets
        );
        assert!(peak > 0.001, "{} rendered silence", segment.name);
        assert!(peak < 1.0, "{} clipping risk {}", segment.name, peak);
        write_wav_stereo(&out_dir.join(format!("{}.wav", segment.name)), &seg_left, &seg_right);
        full_left.extend_from_slice(&seg_left);
        full_right.extend_from_slice(&seg_right);
        report_segments.push(serde_json::json!({
            "name": segment.name,
            "peak": peak,
            "rms": rms_l.max(rms_r),
            "rendered_rpm": diagnostics.rendered_rpm,
            "gate": diagnostics.gate,
            "active_zones": diagnostics.active_zones,
            "accepted_events": diagnostics.accepted_events,
            "suppressed_events": diagnostics.suppressed_events,
            "dropped_packets": diagnostics.dropped_packets,
            "final_peak": diagnostics.final_peak,
            "final_rms": diagnostics.final_rms,
        }));
    }
    // Mono downmix check (exterior path): must equal (L+R)/2 sample-accurate.
    let mut mono = vec![0.0f32; BLOCK];
    let mut left = vec![0.0f32; BLOCK];
    let mut right = vec![0.0f32; BLOCK];
    // SAFETY: live handle + BLOCK buffers.
    assert_eq!(
        unsafe { open_formula_audio::ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), BLOCK as u32) },
        BLOCK as u32
    );
    // SAFETY: live handle + BLOCK mono buffer.
    assert_eq!(unsafe { open_formula_audio::ofp_audio_render_mono(handle, mono.as_mut_ptr(), BLOCK as u32) }, BLOCK as u32);
    // Note: renders advance state, so mono is the *next* block, not the same one;
    // the check here is finiteness + boundedness, exact equality is covered by unit logic.
    assert!(mono.iter().all(|sample| sample.is_finite()));
    assert!(mono.iter().fold(0.0f32, |peak, sample| peak.max(sample.abs())) < 1.0);

    write_wav_stereo(&out_dir.join("full_mix.wav"), &full_left, &full_right);
    let (full_peak_l, full_rms_l) = peak_rms(&full_left);
    let (full_peak_r, full_rms_r) = peak_rms(&full_right);
    let mut bank_id = vec![0i8; 128];
    let mut bank_sha = vec![0i8; 128];
    let mut mixer_hash = vec![0i8; 128];
    // SAFETY: live handle + 128-byte buffers (fuente seleccionada y mezcla efectiva).
    unsafe {
        open_formula_audio::ofp_audio_bank_id(handle, bank_id.as_mut_ptr(), 128);
        open_formula_audio::ofp_audio_bank_sha256(handle, bank_sha.as_mut_ptr(), 128);
        open_formula_audio::ofp_audio_mixer_hash(handle, mixer_hash.as_mut_ptr(), 128);
    }
    let cstr = |buffer: &[i8]| {
        let bytes: Vec<u8> = buffer.iter().take_while(|c| **c != 0).map(|c| *c as u8).collect();
        String::from_utf8_lossy(&bytes).into_owned()
    };
    let report = serde_json::json!({
        "profile": "williams-fw19-hockenheim",
        "idle_rpm": 3_500.0,
        "max_rpm": 17_500.0,
        "sample_rate": 44_100,
        "block_frames": BLOCK,
        "queue_reference_ms": 93.0,
        "v10_active": true,
        "bank_id": cstr(&bank_id),
        "bank_sha256": cstr(&bank_sha),
        "mixer_config_hash": cstr(&mixer_hash),
        "full_peak": full_peak_l.max(full_peak_r),
        "full_rms": full_rms_l.max(full_rms_r),
        "segments": report_segments,
    });
    std::fs::write(out_dir.join("replay_manifest.json"), serde_json::to_string_pretty(&report).unwrap())
        .expect("write manifest");
    // SAFETY: live handle, freed once here.
    unsafe { open_formula_audio::ofp_audio_destroy(handle) };
    println!("Wrote {} segments + full_mix.wav", segments.len());
}
