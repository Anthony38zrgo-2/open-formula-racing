#![deny(unsafe_op_in_unsafe_fn)]
//! TEMPORARY isolation probe (gitignored, never committed). Times
//! synth-enabled scenarios through `VehicleAudioEngine::render` to reveal which
//! layer drives the P9 CPU gate over budget on the active scenarios (grass,
//! backfire, shift_up) that the always-on short-circuit did not fix.

use std::hint::black_box;
use std::path::PathBuf;
use std::time::Instant;

use vehicle_audio_engine::powertrain;
use vehicle_audio_engine::VehicleAudioEngine;

const BLOCK_SIZE: usize = 512;
const SAMPLE_RATE: f64 = 44_100.0;
const BLOCK_AUDIO_NS: f64 = (BLOCK_SIZE as f64 / SAMPLE_RATE) * 1e9;
const SCENARIO_FRAMES: usize = 441_000;

fn stat(v: &[f64]) -> (f64, f64, f64) {
    let mut s = v.to_vec();
    s.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let q = |p: f64| s[((s.len() - 1) as f64 * p).round() as usize];
    (q(0.50), q(0.95), s[s.len() - 1])
}

fn time_scenario(
    engine: &mut VehicleAudioEngine,
    frames: &[(f64, f64, f64, f32, f64, i32, f32, &str)],
) -> (f64, f64, f64) {
    stat(&time_scenario_vec(engine, frames))
}

fn time_scenario_vec(
    engine: &mut VehicleAudioEngine,
    frames: &[(f64, f64, f64, f32, f64, i32, f32, &str)],
) -> Vec<f64> {
    let samples_per_frame = SAMPLE_RATE as usize / 60;
    let mut frame_idx = 0usize;
    let mut tmp_l = vec![0.0f32; BLOCK_SIZE];
    let mut tmp_r = vec![0.0f32; BLOCK_SIZE];
    let total_blocks = SCENARIO_FRAMES / BLOCK_SIZE;
    let mut pcts = Vec::with_capacity(total_blocks);
    for block in 0..total_blocks {
        if block * BLOCK_SIZE >= frame_idx * samples_per_frame && frame_idx < frames.len() {
            let f = &frames[frame_idx];
            engine.set_state(f.0, f.1, f.2, f.3, f.4, f.5, f.6, f.7);
            frame_idx += 1;
        }
        let start = Instant::now();
        engine.render(black_box(&mut tmp_l), black_box(&mut tmp_r), BLOCK_SIZE);
        let ns = start.elapsed().as_nanos() as f64;
        black_box((tmp_l[0], tmp_r[0]));
        pcts.push(ns / BLOCK_AUDIO_NS * 100.0);
    }
    pcts
}

fn main() -> std::process::ExitCode {
    let args: Vec<String> = std::env::args().collect();
    let bank_dir = PathBuf::from(args.get(1).cloned().unwrap_or_else(|| {
        eprintln!("usage: va_probe <bank_dir>");
        std::process::exit(2)
    }));
    let profile_path = PathBuf::from("game/data/vehicles/f1_2026_2008/f1_2026_2008_physics.json");
    let profile_json = std::fs::read_to_string(&profile_path).expect("read profile");
    let pt = powertrain::from_profile_json(&profile_json)
        .expect("profile")
        .expect("powertrain present");

    let mut engine = VehicleAudioEngine::new(&bank_dir).expect("load bank");
    engine.enable_synth(&pt);

    let frames = SCENARIO_FRAMES / (SAMPLE_RATE as usize / 60);
    let idle_rpm = 4500.0;
    let max_rpm = 17000.0;

    let idle: Vec<(f64, f64, f64, f32, f64, i32, f32, &str)> = (0..frames)
        .map(|_| (idle_rpm, idle_rpm, max_rpm, 0.0, 0.0, 0, 0.0, "asphalt"))
        .collect();

    let redline: Vec<(f64, f64, f64, f32, f64, i32, f32, &str)> = (0..frames)
        .map(|_| {
            (
                max_rpm - 500.0,
                idle_rpm,
                max_rpm,
                1.0,
                280.0,
                6,
                0.0,
                "asphalt",
            )
        })
        .collect();

    let sweep: Vec<(f64, f64, f64, f32, f64, i32, f32, &str)> = (0..frames)
        .map(|i| {
            let t = i as f64 / frames as f64;
            let ramp = if t < 0.5 { t * 2.0 } else { 2.0 - t * 2.0 };
            (
                idle_rpm + (max_rpm - idle_rpm) * ramp,
                idle_rpm,
                max_rpm,
                ramp as f32,
                280.0 * ramp,
                (1.0 + ramp * 5.0) as i32,
                0.0,
                "asphalt",
            )
        })
        .collect();

    let grass: Vec<(f64, f64, f64, f32, f64, i32, f32, &str)> = (0..frames)
        .map(|_| (6000.0, idle_rpm, max_rpm, 0.3, 40.0, 2, 0.4, "grass"))
        .collect();

    let grass_slip0: Vec<(f64, f64, f64, f32, f64, i32, f32, &str)> = (0..frames)
        .map(|_| (6000.0, idle_rpm, max_rpm, 0.3, 40.0, 2, 0.0, "grass"))
        .collect();

    let mut shift_up: Vec<(f64, f64, f64, f32, f64, i32, f32, &str)> = Vec::new();
    for i in 0..frames {
        let t = i as f64 / frames as f64;
        let (rpm, gear) = if t < 0.45 {
            (idle_rpm + (max_rpm - idle_rpm) * 0.8, 3)
        } else if t < 0.55 {
            (idle_rpm + (max_rpm - idle_rpm) * 0.6, 4)
        } else {
            (idle_rpm + (max_rpm - idle_rpm) * 0.7, 4)
        };
        shift_up.push((rpm, idle_rpm, max_rpm, 0.9, 180.0, gear, 0.0, "asphalt"));
    }

    let mut backfire: Vec<(f64, f64, f64, f32, f64, i32, f32, &str)> = Vec::new();
    for i in 0..frames {
        let t = i as f64 / frames as f64;
        let (throttle, rpm) = if t < 0.4 {
            (0.9f32, max_rpm - 1000.0)
        } else if t < 0.42 {
            (0.0f32, max_rpm - 1500.0)
        } else {
            (0.0f32, max_rpm - 3000.0 * (t - 0.42) / 0.58)
        };
        backfire.push((
            rpm.max(idle_rpm),
            idle_rpm,
            max_rpm,
            throttle,
            200.0,
            5,
            0.0,
            "asphalt",
        ));
    }

    // Warm-up in bench order.
    let warmup_blocks = ((3.0 * SAMPLE_RATE) as usize / BLOCK_SIZE).max(1);
    for _ in 0..warmup_blocks {
        let mut tmp_l = vec![0.0f32; BLOCK_SIZE];
        let mut tmp_r = vec![0.0f32; BLOCK_SIZE];
        engine.render(&mut tmp_l, &mut tmp_r, BLOCK_SIZE);
    }

    let isol_opt: Option<&str> = args.get(2).map(|s| s.as_str());

    // `denorm` mode: set the x86 MXCSR FTZ/DAZ flags (flush-to-zero,
    // denormals-are-zero) so subnormal float operations run at full speed.
    // This tests the hypothesis that the degradation is subnormal-float stalls
    // in the mixer/synth digital state after loud passages.
    if isol_opt == Some("denorm") {
        set_ftz_daz();
        for k in 0..3 {
            let _ = time_scenario(&mut engine, &redline);
        }
        let (m, p, w) = time_scenario(&mut engine, &grass);
        println!("denorm_grass_hot: median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("denorm_idle_hot : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        engine.set_synth_enabled(false);
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("denorm_idle_off : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
    }

    let skip_main = matches!(
        isol_opt,
        Some("fresh") | Some("rampf") | Some("hist") | Some("cal") | Some("split")
    );

    for (name, scen) in [
        ("idle", &idle),
        ("redline", &redline),
        ("sweep", &sweep),
        ("grass", &grass),
        ("shift_up", &shift_up),
        ("backfire", &backfire),
    ] {
        if isol_opt == Some("grass_only") && name != "grass" {
            continue;
        }
        if skip_main {
            break;
        }
        let (m, p, w) = time_scenario(&mut engine, scen);
        println!("{name:<12}: median {m:.4}% p95 {p:.4}% worst {w:.4}%");
    }
    if isol_opt == Some("grass_only") {
        let (m, p, w) = time_scenario(&mut engine, &grass_slip0);
        println!("grass_slip0  : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
    }
    if isol_opt == Some("acc") {
        let (m, p, w) = time_scenario(&mut engine, &grass);
        println!("grass_fresh  : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("idle2        : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, p, w) = time_scenario(&mut engine, &redline);
        println!("redline2     : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, p, w) = time_scenario(&mut engine, &sweep);
        println!("sweep2       : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, p, w) = time_scenario(&mut engine, &grass);
        println!("grass_after  : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
    }
    if isol_opt == Some("gensynth") {
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("idle_fresh_on   : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        for s in [&idle, &redline, &sweep, &grass, &shift_up, &backfire] {
            let _ = time_scenario(&mut engine, s);
        }
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("idle_after_on   : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        engine.set_synth_enabled(false);
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("idle_after_off  : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        engine.set_synth_enabled(true);
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("idle_after_on2  : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
    }
    if isol_opt == Some("ramp") {
        engine.set_synth_enabled(false);
        for k in 0..6 {
            let (m, p, w) = time_scenario(&mut engine, &idle);
            println!("idle_pass{k}  : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        }
    }
    if isol_opt == Some("fresh") {
        for k in 0..4 {
            let (m, p, w) = time_scenario(&mut engine, &idle);
            println!("fresh_idle{k} : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        }
        let (m, p, w) = time_scenario(&mut engine, &grass);
        println!("grass_cold  : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        for k in 0..3 {
            let _ = time_scenario(&mut engine, &redline);
        }
        let (m, p, w) = time_scenario(&mut engine, &grass);
        println!("grass_hot   : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, p, w) = time_scenario(&mut engine, &idle);
        println!("idle_hot    : median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, p, w) = time_scenario(&mut engine, &backfire);
        println!("backfire_hot: median {m:.4}% p95 {p:.4}% worst {w:.4}%");
        engine.set_synth_enabled(false);
        let (m, p, w) = time_scenario(&mut engine, &grass);
        println!("grass_hot_off: median {m:.4}% p95 {p:.4}% worst {w:.4}%");
    }
    if isol_opt == Some("cal") {
        let c0 = fp_cal();
        let m0 = mem_cal();
        println!("cal_cold: fp {c0:.3} ns/iter  mem {m0:.3} ns/iter");
        for k in 0..3 {
            let _ = time_scenario(&mut engine, &redline);
        }
        let c1 = fp_cal();
        let m1 = mem_cal();
        println!(
            "cal_hot : fp {c1:.3} ns/iter (x{:.2})  mem {m1:.3} ns/iter (x{:.2})",
            c1 / c0,
            m1 / m0
        );
        let (mp, p, w) = time_scenario(&mut engine, &grass);
        println!("grass_hot: median {mp:.4}% p95 {p:.4}% worst {w:.4}%");
        let (m, _, _) = time_scenario(&mut engine, &idle);
        println!("idle_hot0: median {m:.4}%");
        let (m, _, _) = time_scenario(&mut engine, &idle);
        println!("idle_hot1: median {m:.4}%");
    }
    if isol_opt == Some("hist") {
        let v = time_scenario_vec(&mut engine, &redline);
        println!(
            "redline med={:.3} p95={:.3} worst={:.3}",
            percentile(&v, 0.5),
            percentile(&v, 0.95),
            v.last().copied().unwrap_or(0.0)
        );
        println!("  dist={}", hist_str(&v));
    }
    if isol_opt == Some("histidle") {
        let v = time_scenario_vec(&mut engine, &idle);
        println!(
            "idle_fresh med={:.3} p95={:.3} worst={:.3}",
            percentile(&v, 0.5),
            percentile(&v, 0.95),
            v.last().copied().unwrap_or(0.0)
        );
        for k in 0..3 {
            let _ = time_scenario(&mut engine, &redline);
        }
        let id2 = time_scenario_vec(&mut engine, &idle);
        println!(
            "idle_hot  med={:.3} p95={:.3} worst={:.3}",
            percentile(&id2, 0.5),
            percentile(&id2, 0.95),
            id2.last().copied().unwrap_or(0.0)
        );
        println!("--- hot histogram ---");
        let mut id3 = id2.clone();
        id3.sort_by(|a, b| a.partial_cmp(b).unwrap());
        let n = id3.len();
        for (lo, hi) in [
            (0.0, 1.0),
            (1.0, 2.0),
            (2.0, 3.0),
            (3.0, 5.0),
            (5.0, 10.0),
            (10.0, f64::INFINITY),
        ] {
            let c = id3.iter().filter(|&&x| x >= lo && x < hi).count();
            println!("[{lo}, {hi}): {c} ({:.1}%)", 100.0 * c as f64 / n as f64);
        }
    }
    if isol_opt == Some("split") {
        let v = time_scenario_vec(&mut engine, &idle);
        println!("cold_on : hist={}", hist_str(&v));
        for k in 0..3 {
            let _ = time_scenario(&mut engine, &redline);
        }
        let v2 = time_scenario_vec(&mut engine, &idle);
        println!("hot_on  : hist={}", hist_str(&v2));
        engine.set_synth_enabled(false);
        let v3 = time_scenario_vec(&mut engine, &idle);
        println!("hot_off : hist={}", hist_str(&v3));
        engine.set_synth_enabled(true);
    }
    std::process::ExitCode::SUCCESS
}

fn hist_str(v: &[f64]) -> String {
    let mut s = v.to_vec();
    s.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let n = s.len();
    let med = s[n / 2];
    let mut out = format!("med={med:.3}");
    for (lo, hi) in [
        (0.0, 1.0),
        (1.0, 2.0),
        (2.0, 3.0),
        (3.0, 5.0),
        (5.0, 10.0),
        (10.0, f64::INFINITY),
    ] {
        let c = s.iter().filter(|&&x| x >= lo && x < hi).count();
        if c > 0 {
            out.push_str(&format!(" [{lo}-{hi}):{:.0}%", 100.0 * c as f64 / n as f64));
        }
    }
    out
}

fn percentile(sorted: &[f64], p: f64) -> f64 {
    let mut s = sorted.to_vec();
    s.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let idx = ((s.len() - 1) as f64 * p).round() as usize;
    s[idx]
}

/// Pure-FP calibration microbenchmark: measure sustained FP throughput, which
/// scales ~linearly with CPU clock. If this drops after the engine's redline
/// workload, the machine is clock-throttling (environment); if it stays flat
/// while `render` slows, the engine does more work (real cause).
fn fp_cal() -> f64 {
    let iters = 4_000_000usize;
    let mut acc = 0.0f64;
    let start = Instant::now();
    for i in 0..iters {
        let x = i as f64;
        acc += (x * 1.0000001).sin();
        acc -= (x * 0.9999999).cos().abs();
        acc = acc.max(-1e6).min(1e6);
    }
    let ns = start.elapsed().as_nanos() as f64;
    black_box(acc);
    ns / iters as f64
}

/// Memory-streaming calibration: read a large PCM buffer with a fractional
/// cursor and linear interpolation (mimics `read_looped`'s access pattern).
/// If this also degrades after heavy engine load, the slowdown is in the memory
/// subsystem; if it stays flat while `render` degrades, the engine does more work.
fn mem_cal() -> f64 {
    let buf: Vec<f32> = (0..262_144).map(|i| (i as f32) * 0.001).collect();
    let iters = 4_000_000usize;
    let mut cursor = 0.0f32;
    let mut acc = 0.0f32;
    let start = Instant::now();
    for _ in 0..iters {
        let p = cursor.clamp(0.0, (buf.len() - 2) as f32);
        let i0 = p.floor() as usize;
        let i1 = (i0 + 1).min(buf.len() - 1);
        let f = p - i0 as f32;
        acc = buf[i0] * (1.0 - f) + buf[i1] * f;
        cursor += 1.0;
        if cursor >= buf.len() as f32 {
            cursor = 0.0;
        }
    }
    let ns = start.elapsed().as_nanos() as f64;
    black_box(acc);
    ns / iters as f64
}

/// Set the x86 MXCSR registers to flush subnormal (denormal) floats to zero.
/// Isolates the "hot engine runs slower" hypothesis: subnormal floats on x86
/// take a massive per-op penalty unless FTZ (bit 15) and DAZ (bit 6) are set.
#[cfg(target_arch = "x86_64")]
fn set_ftz_daz() {
    let mut mxcsr: u32 = 0;
    unsafe {
        core::arch::asm!(
            "stmxcsr [{0}]",
            in(reg) &mut mxcsr,
            options(nostack, preserves_flags),
        );
    }
    mxcsr |= 0x8000 | 0x0040;
    unsafe {
        core::arch::asm!(
            "ldmxcsr [{0}]",
            in(reg) &mxcsr,
            options(nostack, preserves_flags),
        );
    }
}

#[cfg(not(target_arch = "x86_64"))]
fn set_ftz_daz() {}
