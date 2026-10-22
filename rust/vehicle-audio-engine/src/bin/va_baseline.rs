//! Baseline tool: render deterministic scenarios and capture golden metrics.
//!
//! Generates synthetic telemetry sequences that exercise key mixer behaviors,
//! renders audio through VehicleAudioEngine, and outputs metrics (peak, RMS,
//! hash, L/R continuity) to establish a frozen baseline before implementing
//! schema v2 features.
//!
//! Usage: cargo run -p vehicle_audio_engine --bin va_baseline -- <bank_dir>
//! Example: cargo run -p vehicle_audio_engine --bin va_baseline -- game/sounds/banks/commons

use sha2::{Digest, Sha256};
use std::path::Path;
use vehicle_audio_engine::powertrain;
use vehicle_audio_engine::telemetry::SynthFrame;
use vehicle_audio_engine::VehicleAudioEngine;

/// Render block size (samples). 512 samples @ 44.1 kHz ≈ 11.6 ms.
const BLOCK_SIZE: usize = 512;

/// Total frames to render per scenario (10 seconds @ 44.1 kHz).
const SCENARIO_FRAMES: usize = 441_000;

/// Audio time budget for one render block, in nanoseconds (used for `coste`).
const BLOCK_AUDIO_NS: f64 = (BLOCK_SIZE as f64 / 44_100.0) * 1e9;

/// A synthetic telemetry scenario.
struct Scenario {
    name: &'static str,
    description: &'static str,
    frames: Vec<Frame>,
}

#[derive(Clone, Copy)]
struct Frame {
    rpm: f64,
    idle_rpm: f64,
    max_rpm: f64,
    throttle: f32,
    speed_kph: f64,
    gear: i32,
    slip: f32,
    surface: &'static str,
}

fn build_scenarios() -> Vec<Scenario> {
    let idle_rpm = 4500.0;
    let max_rpm = 17000.0;
    let frames_per_sec = 60; // telemetry rate
    let total_frames = SCENARIO_FRAMES / (44100 / frames_per_sec);

    // Scenario 1: Idle (steady low RPM, no throttle)
    let idle_frames: Vec<Frame> = (0..total_frames)
        .map(|_| Frame {
            rpm: idle_rpm,
            idle_rpm,
            max_rpm,
            throttle: 0.0,
            speed_kph: 0.0,
            gear: 0,
            slip: 0.0,
            surface: "asphalt",
        })
        .collect();

    // Scenario 2: Redline (steady max RPM, full throttle)
    let redline_frames: Vec<Frame> = (0..total_frames)
        .map(|_| Frame {
            rpm: max_rpm - 500.0,
            idle_rpm,
            max_rpm,
            throttle: 1.0,
            speed_kph: 280.0,
            gear: 6,
            slip: 0.0,
            surface: "asphalt",
        })
        .collect();

    // Scenario 3: Sweep (idle -> redline -> idle over 10s)
    let sweep_frames: Vec<Frame> = (0..total_frames)
        .map(|i| {
            let t = i as f64 / total_frames as f64;
            let ramp = if t < 0.5 { t * 2.0 } else { 2.0 - t * 2.0 };
            let rpm = idle_rpm + (max_rpm - idle_rpm) * ramp;
            let throttle = ramp as f32;
            let speed = 280.0 * ramp;
            let gear = (1.0 + ramp * 5.0) as i32;
            Frame {
                rpm,
                idle_rpm,
                max_rpm,
                throttle,
                speed_kph: speed,
                gear,
                slip: 0.0,
                surface: "asphalt",
            }
        })
        .collect();

    // Scenario 4: Surface grass (low speed, slip on grass)
    let grass_frames: Vec<Frame> = (0..total_frames)
        .map(|_| Frame {
            rpm: 6000.0,
            idle_rpm,
            max_rpm,
            throttle: 0.3,
            speed_kph: 40.0,
            gear: 2,
            slip: 0.4,
            surface: "grass",
        })
        .collect();

    // Scenario 5: Shift up (gear change at high RPM)
    let mut shift_up_frames: Vec<Frame> = Vec::new();
    for i in 0..total_frames {
        let t = i as f64 / total_frames as f64;
        let (rpm, gear) = if t < 0.45 {
            (idle_rpm + (max_rpm - idle_rpm) * 0.8, 3)
        } else if t < 0.55 {
            (idle_rpm + (max_rpm - idle_rpm) * 0.6, 4)
        } else {
            (idle_rpm + (max_rpm - idle_rpm) * 0.7, 4)
        };
        shift_up_frames.push(Frame {
            rpm,
            idle_rpm,
            max_rpm,
            throttle: 0.9,
            speed_kph: 180.0,
            gear,
            slip: 0.0,
            surface: "asphalt",
        });
    }

    // Scenario 6: Backfire (lift-off from high RPM)
    let mut backfire_frames: Vec<Frame> = Vec::new();
    for i in 0..total_frames {
        let t = i as f64 / total_frames as f64;
        let (throttle, rpm) = if t < 0.4 {
            (0.9, max_rpm - 1000.0)
        } else if t < 0.42 {
            (0.0, max_rpm - 1500.0)
        } else {
            (0.0, max_rpm - 3000.0 * (t - 0.42) / 0.58)
        };
        backfire_frames.push(Frame {
            rpm: rpm.max(idle_rpm),
            idle_rpm,
            max_rpm,
            throttle,
            speed_kph: 200.0,
            gear: 5,
            slip: 0.0,
            surface: "asphalt",
        });
    }

    vec![
        Scenario {
            name: "idle",
            description: "Steady idle RPM, no throttle",
            frames: idle_frames,
        },
        Scenario {
            name: "redline",
            description: "Steady near-redline, full throttle",
            frames: redline_frames,
        },
        Scenario {
            name: "sweep",
            description: "RPM sweep idle->redline->idle",
            frames: sweep_frames,
        },
        Scenario {
            name: "grass",
            description: "Low speed on grass with slip",
            frames: grass_frames,
        },
        Scenario {
            name: "shift_up",
            description: "Gear shift up at high RPM",
            frames: shift_up_frames,
        },
        Scenario {
            name: "backfire",
            description: "Lift-off from high RPM (backfire trigger)",
            frames: backfire_frames,
        },
    ]
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
struct ScenarioMetrics {
    name: String,
    description: String,
    frames_rendered: usize,
    sample_count: usize,
    peak_l: f32,
    peak_r: f32,
    rms_l: f32,
    rms_r: f32,
    hash_l: String,
    hash_r: String,
    l_equals_r: bool,
    max_discontinuity: f32,
    nan_count: usize,
    inf_count: usize,
    /// P7.6 per-block synth telemetry (empty when the synth is disabled).
    synth: Vec<SynthFrame>,
}

fn compute_metrics(
    name: &str,
    description: &str,
    buf_l: &[f32],
    buf_r: &[f32],
    synth: Vec<SynthFrame>,
) -> ScenarioMetrics {
    let sample_count = buf_l.len().min(buf_r.len());
    let peak_l = buf_l
        .iter()
        .take(sample_count)
        .map(|x| x.abs())
        .fold(0.0f32, f32::max);
    let peak_r = buf_r
        .iter()
        .take(sample_count)
        .map(|x| x.abs())
        .fold(0.0f32, f32::max);
    let rms_l =
        (buf_l.iter().take(sample_count).map(|x| x * x).sum::<f32>() / sample_count as f32).sqrt();
    let rms_r =
        (buf_r.iter().take(sample_count).map(|x| x * x).sum::<f32>() / sample_count as f32).sqrt();

    // Hash first 44100 samples (1 second) for quick comparison
    let hash_window = sample_count.min(44100);
    let mut hasher_l = Sha256::new();
    let mut hasher_r = Sha256::new();
    for i in 0..hash_window {
        hasher_l.update(buf_l[i].to_le_bytes());
        hasher_r.update(buf_r[i].to_le_bytes());
    }
    let hash_l = format!("{:x}", hasher_l.finalize());
    let hash_r = format!("{:x}", hasher_r.finalize());

    let l_equals_r = buf_l
        .iter()
        .take(sample_count)
        .zip(buf_r.iter().take(sample_count))
        .all(|(a, b)| (a - b).abs() < 1e-6);

    // Max discontinuity (sample-to-sample delta)
    let mut max_disc = 0.0f32;
    for i in 1..sample_count {
        let disc_l = (buf_l[i] - buf_l[i - 1]).abs();
        let disc_r = (buf_r[i] - buf_r[i - 1]).abs();
        max_disc = max_disc.max(disc_l).max(disc_r);
    }

    let nan_count = buf_l
        .iter()
        .chain(buf_r.iter())
        .take(sample_count * 2)
        .filter(|x| x.is_nan())
        .count();
    let inf_count = buf_l
        .iter()
        .chain(buf_r.iter())
        .take(sample_count * 2)
        .filter(|x| x.is_infinite())
        .count();

    ScenarioMetrics {
        name: name.to_string(),
        description: description.to_string(),
        frames_rendered: 0,
        sample_count,
        peak_l,
        peak_r,
        rms_l,
        rms_r,
        hash_l,
        hash_r,
        l_equals_r,
        max_discontinuity: max_disc,
        nan_count,
        inf_count,
        synth,
    }
}

fn render_scenario(
    engine: &mut VehicleAudioEngine,
    scenario: &Scenario,
) -> (Vec<f32>, Vec<f32>, usize, Vec<SynthFrame>) {
    let mut buf_l = Vec::new();
    let mut buf_r = Vec::new();
    let frames_per_sec = 60;
    let samples_per_frame = 44100 / frames_per_sec;
    let mut frame_idx = 0;
    let mut synth_frames = Vec::new();
    let mut cum_underruns: u32 = 0;
    let mut cum_virtual: u32 = 0;

    let mut tmp_l = vec![0.0f32; BLOCK_SIZE];
    let mut tmp_r = vec![0.0f32; BLOCK_SIZE];

    let total_blocks = SCENARIO_FRAMES / BLOCK_SIZE;
    for block_idx in 0..total_blocks {
        // Update telemetry once per frame (735 samples)
        if block_idx * BLOCK_SIZE >= frame_idx * samples_per_frame
            && frame_idx < scenario.frames.len()
        {
            let f = &scenario.frames[frame_idx];
            engine.set_state(
                f.rpm,
                f.idle_rpm,
                f.max_rpm,
                f.throttle,
                f.speed_kph,
                f.gear,
                f.slip,
                f.surface,
            );
            frame_idx += 1;
        }
        let events_before = engine.synth_events_fired();
        let start = std::time::Instant::now();
        engine.render(&mut tmp_l, &mut tmp_r, BLOCK_SIZE);
        let block_ns = start.elapsed().as_nanos() as f64;
        buf_l.extend_from_slice(&tmp_l);
        buf_r.extend_from_slice(&tmp_r);

        // P7.6 per-block synth telemetry (only when the synth drives the path).
        if engine.synth_enabled() {
            let is_virtual = engine.synth_lod() as i32 == 3;
            if is_virtual {
                cum_virtual += 1;
            }
            let over = block_ns > BLOCK_AUDIO_NS;
            if over {
                cum_underruns += 1;
            }
            let events_delta = engine.synth_events_fired().saturating_sub(events_before);
            let block_audio_s = BLOCK_SIZE as f64 / 44_100.0;
            let limiter = engine.synth_limiter_active();
            let tc = engine.synth_tc_suppression();
            synth_frames.push(SynthFrame {
                time_ms: (block_idx as u64 * (BLOCK_AUDIO_NS as u64)) / 1_000_000,
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
        }
    }

    (buf_l, buf_r, frame_idx, synth_frames)
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let bank_dir = args
        .iter()
        .skip(1)
        .find(|arg| !arg.starts_with("--") && !arg.ends_with(".json"))
        .cloned()
        .unwrap_or_else(|| "game/sounds/banks/commons".to_string());
    let golden_path = args
        .windows(2)
        .find(|pair| pair[0] == "--check-golden")
        .map(|pair| pair[1].as_str());
    let profile = args
        .windows(2)
        .find(|pair| pair[0] == "--profile")
        .map(|pair| pair[1].as_str())
        .unwrap_or("game/data/vehicles/f1_2026_2008/f1_2026_2008_physics.json");

    eprintln!("=== Formula-90 Audio Baseline Tool ===");
    eprintln!("Bank directory: {}", bank_dir);
    eprintln!("Profile: {}", profile);
    eprintln!();

    // Load the powertrain contract so the synth can be enabled (seed 4500).
    let profile_json = std::fs::read_to_string(profile).unwrap_or_else(|e| {
        eprintln!("ERROR: cannot read profile {profile}: {e}");
        std::process::exit(1);
    });
    let powertrain = match powertrain::from_profile_json(&profile_json) {
        Ok(pt) => pt,
        Err(e) => {
            eprintln!("ERROR: invalid powertrain contract: {e}");
            std::process::exit(1);
        }
    };

    let _engine = match VehicleAudioEngine::new(Path::new(&bank_dir)) {
        Ok(e) => e,
        Err(e) => {
            eprintln!("ERROR: Failed to load bank: {:?}", e);
            std::process::exit(1);
        }
    };

    let scenarios = build_scenarios();
    let mut all_metrics = Vec::new();

    for scenario in &scenarios {
        eprintln!(
            "Rendering scenario: {} ({})",
            scenario.name, scenario.description
        );
        let mut eng_clone = VehicleAudioEngine::new(Path::new(&bank_dir)).unwrap();
        if let Some(pt) = &powertrain {
            eng_clone.enable_synth(pt);
        }
        let (buf_l, buf_r, frames, synth) = render_scenario(&mut eng_clone, scenario);
        let mut metrics =
            compute_metrics(scenario.name, scenario.description, &buf_l, &buf_r, synth);
        metrics.frames_rendered = frames;
        eprintln!(
            "  Frames: {}, Samples: {}, Peak L/R: {:.3}/{:.3}, RMS L/R: {:.3}/{:.3}",
            frames,
            metrics.sample_count,
            metrics.peak_l,
            metrics.peak_r,
            metrics.rms_l,
            metrics.rms_r
        );
        eprintln!(
            "  L==R: {}, Max discontinuity: {:.4}, NaN: {}, Inf: {}",
            metrics.l_equals_r, metrics.max_discontinuity, metrics.nan_count, metrics.inf_count
        );
        eprintln!("  Hash L: {}", &metrics.hash_l[..16]);
        eprintln!("  Hash R: {}", &metrics.hash_r[..16]);
        eprintln!(
            "  Synth blocks: {}, last LOD: {:?}, virtualized: {}, underruns: {}",
            metrics.synth.len(),
            metrics.synth.last().map(|s| s.lod),
            metrics
                .synth
                .last()
                .map(|s| s.frames_virtualizados)
                .unwrap_or(0),
            metrics.synth.last().map(|s| s.underruns).unwrap_or(0)
        );
        eprintln!();
        all_metrics.push(metrics);
    }

    if let Some(path) = golden_path {
        let golden: Vec<ScenarioMetrics> =
            serde_json::from_str(&std::fs::read_to_string(path).unwrap_or_else(|error| {
                eprintln!("ERROR: cannot read golden {path}: {error}");
                std::process::exit(1);
            }))
            .unwrap_or_else(|error| {
                eprintln!("ERROR: invalid golden {path}: {error}");
                std::process::exit(1);
            });
        if golden.len() != all_metrics.len() {
            eprintln!("ERROR: golden scenario count differs");
            std::process::exit(1);
        }
        for actual in &all_metrics {
            let Some(expected) = golden.iter().find(|entry| entry.name == actual.name) else {
                eprintln!("ERROR: scenario {} missing from golden", actual.name);
                std::process::exit(1);
            };
            let drift = (actual.peak_l - expected.peak_l)
                .abs()
                .max((actual.peak_r - expected.peak_r).abs())
                .max((actual.rms_l - expected.rms_l).abs())
                .max((actual.rms_r - expected.rms_r).abs());
            if actual.sample_count != expected.sample_count
                || actual.nan_count != 0
                || actual.inf_count != 0
                || drift > 1e-4
            {
                eprintln!(
                    "ERROR: scenario {} drifted (max metric drift {drift})",
                    actual.name
                );
                std::process::exit(1);
            }
        }
        eprintln!("Golden comparison passed");
    }

    // Output JSON
    let json = serde_json::to_string_pretty(&all_metrics).unwrap();
    println!("{}", json);
}
