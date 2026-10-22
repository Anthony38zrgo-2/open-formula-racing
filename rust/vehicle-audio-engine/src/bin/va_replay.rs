//! va_replay — replay a real telemetry CSV through the audio mix core.
//!
//! Usage:
//!   cargo run -p vehicle_audio_engine --bin va_replay -- \
//!     game/telemetry/telemetry_*.csv --bank game/sounds/banks/commons
//!     [--out reports/audio/telemetry_<session>_mix.csv]
//!     [--summary reports/audio/telemetry_<session>_summary.json]
//!     [--synth <physics.json>]   # enable P7.6 procedural-synth replay (additive)

use std::fs;
use std::path::PathBuf;

use vehicle_audio_engine::powertrain;
use vehicle_audio_engine::telemetry::{parse_rows, replay, summarize, synth_summary};
use vehicle_audio_engine::VehicleAudioEngine;

/// Render block size (samples) used when replaying via the procedural synth.
const BLOCK_SIZE: usize = 512;
const SAMPLE_RATE: f64 = 44_100.0;
const BLOCK_AUDIO_NS: f64 = (BLOCK_SIZE as f64 / SAMPLE_RATE) * 1e9;

type CliArgs = (
    PathBuf,
    Option<PathBuf>,
    Option<PathBuf>,
    Option<PathBuf>,
    Option<PathBuf>,
    Option<PathBuf>,
);

fn parse_args() -> Result<CliArgs, String> {
    let mut args: Vec<String> = std::env::args().collect();
    args.remove(0);
    if args.is_empty() {
        return Err(
            "usage: va_replay <telemetry.csv> [--bank <dir>] [--out <csv>] [--summary <json>] [--synth <profile.json>] [--synth-out <csv>]"
                .to_string(),
        );
    }
    let mut csv = None;
    let mut bank: Option<PathBuf> = None;
    let mut out: Option<PathBuf> = None;
    let mut summary: Option<PathBuf> = None;
    let mut synth_profile: Option<PathBuf> = None;
    let mut synth_out: Option<PathBuf> = None;
    let mut i = 0;
    while i < args.len() {
        if csv.is_none() && !args[i].starts_with("--") {
            csv = Some(PathBuf::from(args[i].clone()));
        } else if args[i] == "--bank" {
            i += 1;
            bank = Some(PathBuf::from(
                args.get(i).ok_or("--bank needs a path")?.clone(),
            ));
        } else if args[i].starts_with("--bank=") {
            bank = Some(PathBuf::from(args[i].trim_start_matches("--bank=")));
        } else if args[i] == "--out" {
            i += 1;
            out = Some(PathBuf::from(
                args.get(i).ok_or("--out needs a path")?.clone(),
            ));
        } else if args[i].starts_with("--out=") {
            out = Some(PathBuf::from(args[i].trim_start_matches("--out=")));
        } else if args[i] == "--summary" {
            i += 1;
            summary = Some(PathBuf::from(
                args.get(i).ok_or("--summary needs a path")?.clone(),
            ));
        } else if args[i].starts_with("--summary=") {
            summary = Some(PathBuf::from(args[i].trim_start_matches("--summary=")));
        } else if args[i] == "--synth" {
            i += 1;
            synth_profile = Some(PathBuf::from(
                args.get(i).ok_or("--synth needs a profile path")?.clone(),
            ));
        } else if args[i].starts_with("--synth=") {
            synth_profile = Some(PathBuf::from(args[i].trim_start_matches("--synth=")));
        } else if args[i] == "--synth-out" {
            i += 1;
            synth_out = Some(PathBuf::from(
                args.get(i).ok_or("--synth-out needs a path")?.clone(),
            ));
        } else if args[i].starts_with("--synth-out=") {
            synth_out = Some(PathBuf::from(args[i].trim_start_matches("--synth-out=")));
        } else {
            return Err(format!("unknown arg: {}", args[i]));
        }
        i += 1;
    }
    let csv = csv.ok_or("missing <telemetry.csv>")?;
    Ok((csv, bank, out, summary, synth_profile, synth_out))
}

fn main() -> std::process::ExitCode {
    let (csv_path, bank, out_path, summary_path, synth_profile, synth_out) = match parse_args() {
        Ok(v) => v,
        Err(e) => {
            eprintln!("{e}");
            return std::process::ExitCode::from(2);
        }
    };
    let csv_raw = match fs::read_to_string(&csv_path) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("read {}: {e}", csv_path.display());
            return std::process::ExitCode::from(2);
        }
    };
    // idle/max from setup JSON header not yet parsed per-row; use F1-94 session defaults.
    let idle_rpm = 4500.0;
    let max_rpm = 17000.0;
    let rows = parse_rows(&csv_raw);
    if rows.is_empty() {
        eprintln!("no telemetry rows parsed from {}", csv_path.display());
        return std::process::ExitCode::from(2);
    }
    let bank_dir = match bank.as_ref() {
        Some(path) => path,
        None => {
            eprintln!("--bank is required so replay uses manifest playback metadata");
            return std::process::ExitCode::from(2);
        }
    };
    let playback_bank = match vehicle_audio_engine::bank::VehicleSoundBank::load(bank_dir) {
        Ok(bank) => bank,
        Err(error) => {
            eprintln!("load {}: {error}", bank_dir.display());
            return std::process::ExitCode::from(2);
        }
    };
    let frames = replay(&rows, idle_rpm, max_rpm, &playback_bank.engine_bands);
    let summary = summarize(&frames);

    // Derive out paths from input if not specified.
    let stem = csv_path
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("session");
    let out = out_path.unwrap_or_else(|| PathBuf::from(format!("reports/audio/{stem}_mix.csv")));
    let sum_path =
        summary_path.unwrap_or_else(|| PathBuf::from(format!("reports/audio/{stem}_summary.json")));

    if let Some(dir) = out.parent() {
        let _ = fs::create_dir_all(dir);
    }
    if let Some(dir) = sum_path.parent() {
        let _ = fs::create_dir_all(dir);
    }

    // Write mix CSV.
    let mut csv_out = String::new();
    csv_out.push_str("Time_ms,RPM,NormalizedRPM,Gear,Throttle,Slip,Surface,W_Idle,W_Low,W_Mid,W_High,W_Redline,P_Idle,P_Low,P_Mid,P_High,P_Redline,EngineGain,BedKey,BedGain,Trigger,DominantBand,Crossfade\n");
    for f in &frames {
        csv_out.push_str(&format!(
            "{},{:.1},{:.4},{},{:.3},{:.3},{},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{},{:.4},{},{},{}\n",
            f.time_ms,
            f.rpm,
            f.normalized_rpm,
            f.gear,
            f.throttle,
            f.slip,
            f.surface,
            f.weights[0],
            f.weights[1],
            f.weights[2],
            f.weights[3],
            f.weights[4],
            f.pitch_scales[0],
            f.pitch_scales[1],
            f.pitch_scales[2],
            f.pitch_scales[3],
            f.pitch_scales[4],
            f.engine_gain,
            f.surface_key.as_deref().unwrap_or(""),
            f.surface_gain,
            f.trigger.as_deref().unwrap_or(""),
            f.dominant_band,
            if f.crossfade_active { 1 } else { 0 },
        ));
    }
    if let Err(e) = fs::write(&out, csv_out) {
        eprintln!("write {}: {e}", out.display());
        return std::process::ExitCode::from(1);
    }

    // Bank integrity check (only when --bank given): verify each entry loads,
    // is mono 44.1k 16-bit and its frame count matches the manifest duration.
    let mut bank_integrity: serde_json::Value = serde_json::Value::Null;
    if let Some(bank_dir) = &bank {
        match vehicle_audio_engine::bank::VehicleSoundBank::load(bank_dir) {
            Ok(b) => {
                let mut issues = Vec::new();
                for (key, sample) in &b.samples {
                    let expected_frames = (sample.duration_s() * sample.sample_rate as f64).round();
                    if (sample.pcm.len() as f64 - expected_frames).abs() > 1.0 {
                        issues.push(format!(
                            "{key}: frames {} != manifest duration {} s ({expected_frames} frames)",
                            sample.pcm.len(),
                            sample.duration_s()
                        ));
                    }
                }
                bank_integrity = serde_json::json!({
                    "bank": b.bank_name,
                    "entries": b.samples.len(),
                    "issues": issues,
                });
            }
            Err(e) => {
                bank_integrity = serde_json::json!({ "error": e.to_string() });
            }
        }
    }

    // Write summary JSON.
    let session_name = csv_path
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("session")
        .to_string();
    let mut summary_obj = serde_json::json!({
        "session": session_name,
        "source_csv": csv_path.display().to_string(),
        "idle_rpm": idle_rpm,
        "max_rpm": max_rpm,
        "summary": summary,
        "note": "surface is fixed to asphalt (CSV does not carry surface); engine bands reflect the native RPMs measured on commons.",
        "engine_band_native_rpm": playback_bank
            .engine_bands
            .iter()
            .map(|band| band.native_rpm)
            .collect::<Vec<_>>(),
    });
    if !bank_integrity.is_null() {
        summary_obj["bank_integrity"] = bank_integrity;
    }
    if let Err(e) = fs::write(
        &sum_path,
        serde_json::to_string_pretty(&summary_obj).unwrap() + "\n",
    ) {
        eprintln!("write {}: {e}", sum_path.display());
        return std::process::ExitCode::from(1);
    }

    // Optional P7.6 procedural-synth replay path (additive — legacy band render
    // above still runs). Drives the telemetry rows through VehicleAudioEngine with
    // the synth enabled and emits the per-block synth contract + a synth summary.
    if let Some(profile) = &synth_profile {
        let bank_dir = match bank.as_ref() {
            Some(b) => b.clone(),
            None => {
                eprintln!("--synth also requires --bank");
                return std::process::ExitCode::from(2);
            }
        };
        let profile_json = match fs::read_to_string(profile) {
            Ok(s) => s,
            Err(e) => {
                eprintln!("read profile {}: {e}", profile.display());
                return std::process::ExitCode::from(2);
            }
        };
        let powertrain = match powertrain::from_profile_json(&profile_json) {
            Ok(Some(pt)) => pt,
            Ok(None) => {
                eprintln!(
                    "profile {} has no audio.powertrain_synthesis",
                    profile.display()
                );
                return std::process::ExitCode::from(2);
            }
            Err(e) => {
                eprintln!("invalid powertrain contract: {e}");
                return std::process::ExitCode::from(2);
            }
        };
        let mut engine = match VehicleAudioEngine::new(&bank_dir) {
            Ok(e) => e,
            Err(e) => {
                eprintln!("load bank {}: {e}", bank_dir.display());
                return std::process::ExitCode::from(2);
            }
        };
        engine.enable_synth(&powertrain);
        if !engine.synth_enabled() {
            eprintln!("enable_synth did not enable; not emitting synth rows");
            return std::process::ExitCode::from(2);
        }

        // Render frames at the telemetry time_ms cadence, grouping into blocks.
        let mut synth_frames = Vec::new();
        let (mut cum_underruns, mut cum_virtual) = (0u32, 0u32);
        let mut tmp_l = vec![0.0f32; BLOCK_SIZE];
        let mut tmp_r = vec![0.0f32; BLOCK_SIZE];
        // Render all rows into the block pipeline.
        for row in &rows {
            engine.set_state(
                row.rpm,
                idle_rpm,
                max_rpm,
                row.throttle,
                row.speed_kph,
                row.gear,
                row.slip,
                "asphalt",
            );
            // Render one block's worth of audio per telemetry frame.
            let events_before = engine.synth_events_fired();
            let start = std::time::Instant::now();
            engine.render(&mut tmp_l, &mut tmp_r, BLOCK_SIZE);
            let block_ns = start.elapsed().as_nanos() as f64;
            let over = block_ns > BLOCK_AUDIO_NS;
            if over {
                cum_underruns += 1;
            }
            if engine.synth_lod() as i32 == 3 {
                cum_virtual += 1;
            }
            let events_delta = engine.synth_events_fired().saturating_sub(events_before);
            let block_audio_s = BLOCK_SIZE as f64 / SAMPLE_RATE;
            let limiter = engine.synth_limiter_active();
            let tc = engine.synth_tc_suppression();
            synth_frames.push(vehicle_audio_engine::telemetry::SynthFrame {
                time_ms: row.time_ms,
                lod: engine.synth_lod() as i32,
                distancia: engine.listener_distance(),
                coste: (block_ns / BLOCK_AUDIO_NS) * 100.0,
                frecuencia_eventos: events_delta as f64 / block_audio_s,
                energia: engine.synth_energy(),
                carga: engine.synth_load(),
                resonator_scale: engine.synth_resonator_scale(),
                limiter_active: limiter as i32,
                limiter_cut: engine.synth_limiter_cut(),
                tc_active: (tc > 1e-4) as i32,
                tc_cut: tc,
                underruns: cum_underruns,
                frames_virtualizados: cum_virtual,
            });
            tmp_l.fill(0.0);
            tmp_r.fill(0.0);
        }
        let ssum = synth_summary(&synth_frames);

        let synth_csv_path =
            synth_out.unwrap_or_else(|| PathBuf::from(format!("reports/audio/{stem}_synth.csv")));
        let synth_sum_path = PathBuf::from(format!("reports/audio/{stem}_synth_summary.json"));
        if let Some(dir) = synth_csv_path.parent() {
            let _ = fs::create_dir_all(dir);
        }
        let mut scsv = String::new();
        scsv.push_str(
            "time_ms,lod,distancia,coste,frecuencia_eventos,energia,carga,resonator_scale,limiter_active,limiter_cut,tc_active,tc_cut,underruns,frames_virtualizados\n",
        );
        for f in &synth_frames {
            scsv.push_str(&format!(
                "{},{},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{},{:.4},{},{:.4},{},{}\n",
                f.time_ms,
                f.lod,
                f.distancia,
                f.coste,
                f.frecuencia_eventos,
                f.energia,
                f.carga,
                f.resonator_scale,
                f.limiter_active,
                f.limiter_cut,
                f.tc_active,
                f.tc_cut,
                f.underruns,
                f.frames_virtualizados
            ));
        }
        if let Err(e) = fs::write(&synth_csv_path, scsv) {
            eprintln!("write {}: {e}", synth_csv_path.display());
            return std::process::ExitCode::from(1);
        }
        if let Err(e) = fs::write(
            &synth_sum_path,
            serde_json::to_string_pretty(&ssum).unwrap() + "\n",
        ) {
            eprintln!("write {}: {e}", synth_sum_path.display());
            return std::process::ExitCode::from(1);
        }
        println!(
            "  synth replay: {} blocks  lod_share={:?}  coste median={:.3}% worst={:.3}%  underruns={} virtual={}  -> {} , {}",
            ssum.rows,
            ssum.lod_share,
            ssum.coste_median,
            ssum.coste_worst,
            ssum.underruns,
            ssum.frames_virtualizados,
            synth_csv_path.display(),
            synth_sum_path.display()
        );
    } else {
        // No --synth requested: nothing extra to do.
    }

    println!(
        "replay {}: {} rows  rpm [{:.0}..{:.0}]  shifts up={} down={}  crossfade={:.0}%  -> {} , {}",
        session_name,
        summary.rows,
        summary.rpm_min,
        summary.rpm_max,
        summary.shift_up,
        summary.shift_down,
        summary.crossfade_share * 100.0,
        out.display(),
        sum_path.display()
    );

    // Clamp hits — surface if the pitch was hitting its limits (ranges of the samples).
    let high_hits: usize = summary.clamp_high_hits.iter().sum();
    let low_hits: usize = summary.clamp_low_hits.iter().sum();
    if high_hits + low_hits > 0 {
        println!(
            "  clamp hits: low={:?} high={:?}  (pitch at PITCH_MIN/MAX — timbre stretched)",
            summary.clamp_low_hits, summary.clamp_high_hits
        );
    }

    std::process::ExitCode::SUCCESS
}
