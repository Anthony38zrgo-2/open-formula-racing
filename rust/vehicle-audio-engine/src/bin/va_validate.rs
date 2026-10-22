//! Validate, summarize, or migrate `sound_mixer_config.json`.

use vehicle_audio_engine::{
    config::{resolve_config_json, SoundMixerConfig},
    VehicleSoundBank,
};

fn print_summary(config: &SoundMixerConfig) {
    println!("=== Sound Mixer Config Summary ===");
    println!("Schema version: {}", config.schema_version());
    match config {
        SoundMixerConfig::V1(cfg) => {
            println!("Mode: Legacy (v1)");
            println!("Gains configured: {}", cfg.gains.len());
            println!("Exhaust enabled: {}", cfg.exhaust.enabled.unwrap_or(true));
        }
        SoundMixerConfig::V2(cfg) => {
            println!("Mode: Full Sound Design (v2)");
            println!("Resolved sounds: {}", cfg.sounds.len());
            println!("Reverb buses: {}", cfg.reverb_buses.len());
            println!("Master output_db: {:.1} dB", cfg.master.output_db);
            println!(
                "Hot reload: {}",
                if cfg.hot_reload.enabled {
                    "enabled"
                } else {
                    "disabled"
                }
            );
            for (key, sound) in &cfg.sounds {
                println!(
                    "  {key}: vol={:.2}, pan={:+.2}, adsr={}, eq={}, tube={}, reverb={}",
                    sound.volume,
                    sound.pan,
                    if sound.adsr.enabled { "on" } else { "off" },
                    if sound.eq.enabled { "on" } else { "off" },
                    if sound.eq.tube_color.enabled {
                        "on"
                    } else {
                        "bypass"
                    },
                    if sound.reverb.enabled {
                        sound.reverb.bus.as_str()
                    } else {
                        "off"
                    },
                );
            }
        }
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: va_validate <config_path> [--check] [--bank <dir>] [--migrate] [--output <path>]");
        std::process::exit(1);
    }
    let check = args.iter().any(|arg| arg == "--check");
    let migrate = args.iter().any(|arg| arg == "--migrate");
    let output = args
        .windows(2)
        .find(|pair| pair[0] == "--output")
        .map(|pair| pair[1].as_str());
    let bank_dir = args
        .windows(2)
        .find(|pair| pair[0] == "--bank")
        .map(|pair| pair[1].as_str());
    let raw = std::fs::read_to_string(&args[1]).unwrap_or_else(|error| {
        eprintln!("ERROR: cannot read {}: {error}", args[1]);
        std::process::exit(1);
    });
    let bank_keys = bank_dir.map_or_else(Vec::new, |path| {
        VehicleSoundBank::load(std::path::Path::new(path))
            .unwrap_or_else(|error| {
                eprintln!("ERROR: cannot load bank {path}: {error}");
                std::process::exit(1);
            })
            .keys()
    });
    let result = resolve_config_json(&raw, &bank_keys);
    for warning in &result.warnings {
        println!("WARNING: {}: {}", warning.path, warning.message);
    }
    for error in &result.errors {
        eprintln!("ERROR: {}: {}", error.path, error.message);
    }
    if !result.is_valid() {
        eprintln!("Configuration has {} error(s)", result.errors.len());
        std::process::exit(1);
    }
    if check {
        println!("Configuration is valid");
        return;
    }
    if migrate {
        let json = serde_json::to_string_pretty(&result.resolved.to_v2()).expect("serialize v2");
        if let Some(path) = output {
            std::fs::write(path, json).unwrap_or_else(|error| {
                eprintln!("ERROR: cannot write {path}: {error}");
                std::process::exit(1);
            });
            println!("Migrated config written to {path}");
        } else {
            println!("{json}");
        }
        return;
    }
    print_summary(&result.resolved);
}
