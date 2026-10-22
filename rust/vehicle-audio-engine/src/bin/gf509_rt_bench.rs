use std::path::Path;
use std::time::Instant;

use vehicle_audio_engine::{DiagnosticMode, VehicleAudioEngine};

fn percentile(sorted: &[u64], fraction: f32) -> u64 {
    let index = ((sorted.len() - 1) as f32 * fraction).round() as usize;
    sorted[index]
}

fn main() -> Result<(), String> {
    let bank = Path::new("../sounds/banks/commons");
    let assets = Path::new("../audio/v10_gf509");
    let mut engine = VehicleAudioEngine::new(bank).map_err(|error| error.to_string())?;
    engine.enable_v10_gf509(assets)?;
    engine.set_diagnostic_mode(DiagnosticMode::Mix);
    engine.set_state(12_000.0, 1_000.0, 15_000.0, 0.92, 220.0, 5, 0.0, "asphalt");
    engine.set_gf509_physics(0.88, 1.0 / 120.0)?;

    println!("frames,median_us,p95_us,worst_us,deadline_us,worst_percent");
    for frames in [128usize, 256, 512, 1024] {
        let mut left = vec![0.0; frames];
        let mut right = vec![0.0; frames];
        for _ in 0..20 {
            engine.render(&mut left, &mut right, frames);
        }
        let mut samples = Vec::with_capacity(300);
        for _ in 0..300 {
            let start = Instant::now();
            engine.render(&mut left, &mut right, frames);
            samples.push(start.elapsed().as_nanos() as u64);
        }
        samples.sort_unstable();
        let median = percentile(&samples, 0.50);
        let p95 = percentile(&samples, 0.95);
        let worst = *samples.last().unwrap();
        let deadline = frames as u64 * 1_000_000_000 / 44_100;
        println!(
            "{frames},{:.3},{:.3},{:.3},{:.3},{:.2}",
            median as f64 / 1000.0,
            p95 as f64 / 1000.0,
            worst as f64 / 1000.0,
            deadline as f64 / 1000.0,
            worst as f64 / deadline as f64 * 100.0
        );
    }
    let diagnostics = engine.continuous_diagnostics();
    println!(
        "source={},received_rpm={:.1},rendered_rpm={:.1},peak_pre={:.5},reduction_db={:.3},underruns={},errors={}",
        diagnostics.source,
        diagnostics.received_rpm,
        diagnostics.rendered_rpm,
        diagnostics.peak_pre_protection,
        diagnostics.protection_reduction_db,
        diagnostics.underruns,
        diagnostics.asset_or_render_errors
    );
    Ok(())
}
