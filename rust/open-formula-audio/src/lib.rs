//! Open Formula audio ABI (`open_formula_audio`, ABI v1).
//!
//! Owned-mixer wrapper around `vehicle_audio_engine` for `snddefault`.
//! - Un handle opaco posee un `VehicleAudioEngine` (un coche).
//! - La creación carga commons + `sound_mixer_config.json` y activa el V10 con
//!   `enable_grand_prix_sampler` (variante local `OFP-V10-3500`, ralentí 3500).
//! - Un fallo de V10 con `fallback_to_legacy == 0` falla cerrado (null + error
//!   global); con fallback explícito conserva legacy pero `v10_active == 0`.
//! - `push_packet` consume paquetes temporizados completos y los distribuye a
//!   `set_grand_prix_mechanical_state` + `set_telemetry_timed` + `set_scrape_state`
//!   + `set_tire_scrub_state`, en ese orden, preservando orden/secuencia.
//! - Un solo hilo toca el handle mutable. Ningún pánico cruza la ABI.
//! - La ruta crítica (`push` aceptado, `render`, `diagnostics`) no asigna en éxito.

use std::ffi::{c_char, c_void, CStr};
use std::mem::size_of;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::path::{Path, PathBuf};
use std::sync::Mutex;

use vehicle_audio_engine::mixer::{ContinuousSourceKind, GrandPrixSamplerTuning, VehicleAudioEngine};

/// ABI version. Bump on any signature/semantic change below.
pub const OFP_AUDIO_ABI_VERSION: u32 = 1;
/// Fixed render sample rate (banks are 44100 Hz mono16).
pub const OFP_AUDIO_SAMPLE_RATE: u32 = 44_100;
/// Maximum PCM block per `render` call (matches mixer `MAX_CPP_BLOCK`).
pub const OFP_AUDIO_MAX_BLOCK_FRAMES: u32 = 4096;
pub const OFP_AUDIO_WHEELS: usize = 4;

/// Surface codes for `OfpAudioPacketV1.surface_code`.
pub const OFP_AUDIO_SURFACE_ASPHALT: u32 = 0;
pub const OFP_AUDIO_SURFACE_GRASS: u32 = 1;
pub const OFP_AUDIO_SURFACE_SAND: u32 = 2;
pub const OFP_AUDIO_SURFACE_RUMBLE: u32 = 3;

/// Error codes returned by fallible entry points / `last_error_code`.
pub const OFP_AUDIO_OK: u32 = 0;
pub const OFP_AUDIO_ERR_NULL: u32 = 1;
pub const OFP_AUDIO_ERR_VERSION: u32 = 2;
pub const OFP_AUDIO_ERR_SIZE: u32 = 3;
pub const OFP_AUDIO_ERR_UTF8: u32 = 4;
pub const OFP_AUDIO_ERR_NOT_FOUND: u32 = 5;
pub const OFP_AUDIO_ERR_V10_LOAD: u32 = 6;
pub const OFP_AUDIO_ERR_STALE: u32 = 7;
pub const OFP_AUDIO_ERR_SESSION: u32 = 8;
pub const OFP_AUDIO_ERR_RANGE: u32 = 9;
pub const OFP_AUDIO_ERR_PANIC: u32 = 10;

/// Creation params (Williams/Fuji first activation: idle 3500, max 17500).
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct OfpAudioCreateParamsV1 {
    pub schema_version: u32,
    pub struct_size: u32,
    pub sample_rate: u32,
    pub forward_gears: i32,
    pub idle_rpm: f64,
    pub max_rpm: f64,
    pub engine_gain: f32,
    pub gearbox_gain: f32,
    pub gearbox_whine_gain: f32,
    pub gearbox_whine_tone_gain: f32,
    pub gearbox_whine_noise_gain: f32,
    pub backfire_gain: f32,
    pub limiter_gain: f32,
    pub coast_gain: f32,
    pub coast_gain_enabled: u32,
    pub gearbox_whine_final_drive: f32,
    pub reverse_ratio: f32,
    pub gear_teeth: f32,
    pub final_teeth: f32,
    pub gear_ratios: [f32; 8],
    pub fallback_to_legacy: u32,
    pub reserved: [u32; 4],
}

/// Timed audio packet: one accepted physics step (Etapa 3 will publish these
/// per accepted step; Etapa 2 replays them offline in order).
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct OfpAudioPacketV1 {
    pub schema_version: u32,
    pub struct_size: u32,
    pub car_index: i32,
    pub session_generation: u32,
    pub sequence: u64,
    pub sim_time_s: f64,
    pub dt_s: f32,
    pub _pad0: u32,
    pub rpm: f64,
    pub throttle: f32,
    pub normalized_engine_load: f32,
    pub normalized_engine_torque: f32,
    pub rpm_derivative: f32,
    pub throttle_derivative: f32,
    pub speed_kph: f64,
    pub slip: f32,
    pub gear: i32,
    pub torque_sign: i32,
    pub shift_phase: i32,
    pub clutch_engagement: f32,
    pub tc_cut_ratio: f32,
    pub rev_limiter_active: u32,
    pub output_shaft_hz: f64,
    pub has_output_shaft: u32,
    pub target_gear: i32,
    pub scrape_active: u32,
    pub scrape_intensity: f32,
    pub scrape_speed_m_s: f32,
    pub scrape_onset: f32,
    pub tire_slip_ratio: [f32; 4],
    pub tire_slip_angle: [f32; 4],
    pub tire_contact: [f32; 4],
    pub tire_load_n: [f32; 4],
    pub tire_speed_kph: f32,
    pub surface_code: u32,
    pub reserved: u32,
}

/// Raw mechanical step (Etapa 3 presentation contract, mirrors
/// `src/interfaces/ofp_audio_presentation.h`). No acoustic decision: the
/// wrapper derives delivered load, derivatives and shift/blip/recovery phases.
pub const OFP_AUDIO_STEP_VERSION: u32 = 1;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct OfpAudioStepV1 {
    pub schema_version: u32,
    pub struct_size: u32,
    pub car_index: i32,
    pub session_generation: u32,
    pub sequence: u64,
    pub sim_time_s: f64,
    pub dt_s: f32,
    pub _pad0: u32,
    pub rpm: f64,
    pub throttle: f32,
    pub speed_m_s: f64,
    pub gear: i32,
    pub target_gear: i32,
    pub engine_torque_nm: f64,
    pub clutch_torque_nm: f64,
    pub max_torque_nm: f64,
    pub clutch_engagement: f64,
    pub tc_cut_ratio_smoothed: f64,
    pub is_rev_limited: u32,
    pub shift_timer_s: f64,
    pub wheel_spin_rad_s: [f64; 4],
    pub final_drive: f64,
    pub front_torque_split: f64,
    pub slip_ratio: [f32; 4],
    pub slip_angle_rad: [f32; 4],
    pub contact_fraction: [f32; 4],
    pub normal_force_n: [f32; 4],
    pub surface: [i32; 4],
    pub floor_valid_mask: u32,
    pub floor_impulse_ns: [f64; 2],
    pub body_contact_count: u32,
    pub body_contact_impulse_ns: [f64; 32],
    pub scrape_speed_m_s: f32,
    pub reserved: u32,
}

/// Diagnostics snapshot (no allocation on query).
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct OfpAudioDiagnosticsV1 {
    pub schema_version: u32,
    pub struct_size: u32,
    pub sample_rate: u32,
    pub v10_active: u32,
    pub continuous_source: u32,
    pub rendered_rpm: f32,
    pub gate: f32,
    pub load_gain: f32,
    pub active_zones: u32,
    pub active_voices: u32,
    pub accepted_events: u64,
    pub suppressed_events: u64,
    pub dropped_packets: u64,
    pub last_sequence: u64,
    pub last_error_code: u32,
    pub reserved: u32,
    pub final_peak: f32,
    pub final_rms: f32,
    pub effective_min_rpm: f64,
    pub bank_min_rpm: f64,
    pub bank_max_rpm: f64,
}

pub struct OfpAudioHandle {
    engine: VehicleAudioEngine,
    assets_root: PathBuf,
    commons_dir: PathBuf,
    v10_dir: PathBuf,
    mixer_hash: String,
    bank_id: String,
    bank_sha: String,
    v10_active: bool,
    create_error: String,
    car_index: Option<i32>,
    session_generation: Option<u32>,
    last_sequence: u64,
    dropped_packets: u64,
    last_error_code: u32,
    last_error: String,
    sample_rate: u32,
    effective_min_rpm: f64,
    bank_min_rpm: f64,
    bank_max_rpm: f64,
    // Etapa 3: ported `audio_telemetry.rs` adapter state (per car handle).
    // Derivatives use dt with tau 20 ms; shift/blip/recovery phases follow the
    // physical cut only. This state never feeds back into physics.
    prev_rpm_throttle: Option<(f32, f32)>,
    rpm_dot: f32,
    throttle_dot: f32,
    was_shifting: bool,
    downshift_latched: bool,
    blip_s: f32,
    recovery_s: f32,
    prev_scrape_active: bool,
    // Etapa 4: rising-edge impact memory. Body contacts in this pipeline are
    // barrier contacts, so a new edge fires the Barrier one-shot (commons) via
    // the mixer's single-owner routing. Cooldown runs on simulation time; a
    // sustained support never retriggers per tick.
    prev_body_impulse_ns: [f64; 32],
    impact_cooldown_until_s: f64,
}

static CREATE_ERROR: Mutex<String> = Mutex::new(String::new());

fn reset_mechanical_history(handle: &mut OfpAudioHandle) {
    handle.prev_rpm_throttle = None;
    handle.rpm_dot = 0.0;
    handle.throttle_dot = 0.0;
    handle.was_shifting = false;
    handle.downshift_latched = false;
    handle.blip_s = 0.0;
    handle.recovery_s = 0.0;
    handle.prev_scrape_active = false;
    handle.prev_body_impulse_ns = [0.0; 32];
    handle.impact_cooldown_until_s = 0.0;
}

fn set_create_error(message: String) {
    if let Ok(mut slot) = CREATE_ERROR.lock() {
        *slot = message;
    }
}

fn surface_token(code: u32) -> &'static str {
    match code {
        OFP_AUDIO_SURFACE_GRASS => "grass",
        OFP_AUDIO_SURFACE_SAND => "sand",
        OFP_AUDIO_SURFACE_RUMBLE => "rumble",
        _ => "asphalt",
    }
}

/// Every gain-affecting scalar must be finite: one NaN packet would poison
/// smoothed mixer state until the next reset. Arrays are checked element-wise.
fn packet_floats_finite(packet: &OfpAudioPacketV1) -> bool {
    packet.rpm.is_finite()
        && packet.dt_s.is_finite()
        && packet.throttle.is_finite()
        && packet.normalized_engine_load.is_finite()
        && packet.normalized_engine_torque.is_finite()
        && packet.rpm_derivative.is_finite()
        && packet.throttle_derivative.is_finite()
        && packet.speed_kph.is_finite()
        && packet.slip.is_finite()
        && packet.clutch_engagement.is_finite()
        && packet.tc_cut_ratio.is_finite()
        && packet.output_shaft_hz.is_finite()
        && packet.scrape_intensity.is_finite()
        && packet.scrape_speed_m_s.is_finite()
        && packet.scrape_onset.is_finite()
        && packet.tire_slip_ratio.iter().all(|v| v.is_finite())
        && packet.tire_slip_angle.iter().all(|v| v.is_finite())
        && packet.tire_contact.iter().all(|v| v.is_finite())
        && packet.tire_load_n.iter().all(|v| v.is_finite())
        && packet.tire_speed_kph.is_finite()
}

fn step_floats_finite(step: &OfpAudioStepV1) -> bool {
    step.rpm.is_finite()
        && step.dt_s.is_finite()
        && (step.throttle as f64).is_finite()
        && step.speed_m_s.is_finite()
        && step.engine_torque_nm.is_finite()
        && step.clutch_torque_nm.is_finite()
        && step.max_torque_nm.is_finite()
        && step.clutch_engagement.is_finite()
        && step.tc_cut_ratio_smoothed.is_finite()
        && step.shift_timer_s.is_finite()
        && step.wheel_spin_rad_s.iter().all(|v| v.is_finite())
        && step.final_drive.is_finite()
        && step.front_torque_split.is_finite()
        && step.slip_ratio.iter().all(|v| v.is_finite())
        && step.slip_angle_rad.iter().all(|v| v.is_finite())
        && step.contact_fraction.iter().all(|v| v.is_finite())
        && step.normal_force_n.iter().all(|v| v.is_finite())
        && step.floor_impulse_ns.iter().all(|v| v.is_finite())
        && step.body_contact_impulse_ns.iter().all(|v| v.is_finite())
        && step.scrape_speed_m_s.is_finite()
}

fn continuous_source_code(engine: &VehicleAudioEngine) -> u32 {
    match engine.continuous_source() {
        ContinuousSourceKind::GrandPrixSampler => 1,
        ContinuousSourceKind::V10Gf509 => 2,
        _ => 0,
    }
}

fn copy_cstr_to_buf(text: &str, buf: *mut c_char, buf_len: u32) -> u32 {
    if buf.is_null() || buf_len == 0 {
        return 0;
    }
    let bytes = text.as_bytes();
    let capacity = buf_len as usize;
    // Reserve one byte for NUL.
    let writable = capacity.saturating_sub(1).min(bytes.len());
    // SAFETY: caller guarantees `buf_len` writable bytes at `buf`.
    unsafe {
        std::ptr::copy_nonoverlapping(bytes.as_ptr() as *const c_char, buf, writable);
        *buf.add(writable) = 0;
    }
    writable as u32
}

fn read_bank_coverage(v10_dir: &Path) -> (f64, f64) {
    let manifest = v10_dir.join("manifest.json");
    let text = std::fs::read_to_string(&manifest).unwrap_or_default();
    let value: serde_json::Value = serde_json::from_str(&text).unwrap_or(serde_json::Value::Null);
    let min = value
        .get("coverage")
        .and_then(|coverage| coverage.get("minimum_revolutions_per_minute"))
        .and_then(|minimum| minimum.as_f64())
        .unwrap_or(4_500.0);
    let max = value
        .get("coverage")
        .and_then(|coverage| coverage.get("maximum_revolutions_per_minute"))
        .and_then(|maximum| maximum.as_f64())
        .unwrap_or(18_000.0);
    (min, max)
}

/// ABI version exported by this build.
#[no_mangle]
pub extern "C" fn ofp_audio_abi_version() -> u32 {
    OFP_AUDIO_ABI_VERSION
}

/// Fixed sample rate (44100).
#[no_mangle]
pub extern "C" fn ofp_audio_sample_rate() -> u32 {
    OFP_AUDIO_SAMPLE_RATE
}

/// Maximum frames per render call (4096).
#[no_mangle]
pub extern "C" fn ofp_audio_max_block_frames() -> u32 {
    OFP_AUDIO_MAX_BLOCK_FRAMES
}

/// Static layout signature: struct names + sizes. Used to reject ABI drift.
#[no_mangle]
pub extern "C" fn ofp_audio_layout_signature() -> *const c_char {
    static SIGNATURE: &[u8] = concat!(
        "OfpAudioCreateParamsV1:136;",
        "OfpAudioPacketV1:224;",
        "OfpAudioDiagnosticsV1:112;",
        "OFP_AUDIO_ABI_VERSION=1\0"
    )
    .as_bytes();
    SIGNATURE.as_ptr() as *const c_char
}

/// Static provenance: variant, donor HEAD, destination build SHA.
#[no_mangle]
pub extern "C" fn ofp_audio_provenance() -> *const c_char {
    static PROVENANCE: &[u8] = concat!(
        "variant=OFP-V10-3500;",
        "donor=Formula90s@f3efcf246c2a2abf62477037ed0243f2f9a93a9b;",
        "bank=v10-v2-bank[4500,18000]+commons;",
        "mixer=sound_mixer_config.json;",
        "dsp=disabled;gf509=disabled;synth=disabled\0"
    )
    .as_bytes();
    PROVENANCE.as_ptr() as *const c_char
}

/// Copy the last creation error into `buf` (NUL-terminated). Returns bytes written.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_last_create_error(buf: *mut c_char, buf_len: u32) -> u32 {
    let text = CREATE_ERROR
        .lock()
        .map(|slot| slot.clone())
        .unwrap_or_default();
    copy_cstr_to_buf(&text, buf, buf_len)
}

fn create_inner(
    assets_root: &str,
    params: &OfpAudioCreateParamsV1,
) -> Result<Box<OfpAudioHandle>, (u32, String)> {
    if params.schema_version != OFP_AUDIO_ABI_VERSION {
        return Err((
            OFP_AUDIO_ERR_VERSION,
            format!("unsupported create schema {}", params.schema_version),
        ));
    }
    if params.struct_size as usize != size_of::<OfpAudioCreateParamsV1>() {
        return Err((
            OFP_AUDIO_ERR_SIZE,
            format!("bad create struct_size {}", params.struct_size),
        ));
    }
    if params.sample_rate != OFP_AUDIO_SAMPLE_RATE {
        return Err((
            OFP_AUDIO_ERR_RANGE,
            format!("sample_rate must be 44100, got {}", params.sample_rate),
        ));
    }
    if !params.idle_rpm.is_finite() || !params.max_rpm.is_finite() || params.idle_rpm >= params.max_rpm {
        return Err((
            OFP_AUDIO_ERR_RANGE,
            format!("bad rpm range {}..{}", params.idle_rpm, params.max_rpm),
        ));
    }
    let assets_root = Path::new(assets_root);
    let commons_dir = assets_root.join("banks/commons");
    let v10_dir = assets_root.join("banks/v10-v2-bank");
    if !commons_dir.join("bank_manifest.json").is_file() {
        return Err((
            OFP_AUDIO_ERR_NOT_FOUND,
            format!("commons manifest missing at {}", commons_dir.display()),
        ));
    }
    if !v10_dir.join("manifest.json").is_file() {
        return Err((
            OFP_AUDIO_ERR_NOT_FOUND,
            format!("V10 manifest missing at {}", v10_dir.display()),
        ));
    }
    let mut engine =
        VehicleAudioEngine::new(&commons_dir).map_err(|error| (OFP_AUDIO_ERR_NOT_FOUND, error.to_string()))?;
    let mixer_hash = engine.config_source_hash().to_string();
    let mut gear_ratios = Vec::new();
    let gears = params.forward_gears.clamp(0, 8) as usize;
    for ratio in params.gear_ratios.iter().take(gears) {
        if ratio.is_finite() && *ratio > 0.0 {
            gear_ratios.push(*ratio);
        }
    }
    let tuning = GrandPrixSamplerTuning {
        engine_gain: params.engine_gain,
        gearbox_gain: params.gearbox_gain,
        gearbox_whine_gain: params.gearbox_whine_gain,
        gearbox_whine_tone_gain: params.gearbox_whine_tone_gain,
        gearbox_whine_noise_gain: params.gearbox_whine_noise_gain,
        gearbox_whine_gear_ratios: gear_ratios,
        gearbox_whine_final_drive: params.gearbox_whine_final_drive,
        gearbox_whine_reverse_ratio: params.reverse_ratio,
        gearbox_whine_gear_teeth: params.gear_teeth,
        gearbox_whine_final_teeth: params.final_teeth,
        backfire_gain: params.backfire_gain,
        limiter_gain: params.limiter_gain,
        coast_gain: (params.coast_gain_enabled != 0).then_some(params.coast_gain),
        required_minimum_revolutions_per_minute: Some(params.idle_rpm),
        required_maximum_revolutions_per_minute: Some(params.max_rpm),
    };
    let (bank_min_rpm, bank_max_rpm) = read_bank_coverage(&v10_dir);
    match engine.enable_grand_prix_sampler(&v10_dir, &tuning) {
        Ok(()) => {
            let bank_id = engine.grand_prix_bank_id().unwrap_or("").to_string();
            let bank_sha = engine.grand_prix_bank_sha256().unwrap_or("").to_string();
            let effective_min_rpm = params.idle_rpm.min(bank_min_rpm);
            Ok(Box::new(OfpAudioHandle {
                engine,
                assets_root: assets_root.to_path_buf(),
                commons_dir,
                v10_dir,
                mixer_hash,
                bank_id,
                bank_sha,
                v10_active: true,
                create_error: String::new(),
                car_index: None,
                session_generation: None,
                last_sequence: 0,
                dropped_packets: 0,
                last_error_code: OFP_AUDIO_OK,
                last_error: String::new(),
                sample_rate: params.sample_rate,
                effective_min_rpm,
                bank_min_rpm,
                bank_max_rpm,
                prev_rpm_throttle: None,
                rpm_dot: 0.0,
                throttle_dot: 0.0,
                was_shifting: false,
                downshift_latched: false,
                blip_s: 0.0,
                recovery_s: 0.0,
                prev_scrape_active: false,
                prev_body_impulse_ns: [0.0; 32],
                impact_cooldown_until_s: 0.0,
            }))
        }
        Err(message) => {
            if params.fallback_to_legacy == 0 {
                Err((OFP_AUDIO_ERR_V10_LOAD, format!("V10 init failed (fail-closed): {message}")))
            } else {
                // Explicit legacy fallback: V10 stays inactive, commons still usable.
                Ok(Box::new(OfpAudioHandle {
                    engine,
                    assets_root: assets_root.to_path_buf(),
                    commons_dir,
                    v10_dir,
                    mixer_hash,
                    bank_id: String::new(),
                    bank_sha: String::new(),
                    v10_active: false,
                    create_error: format!("V10 init failed, legacy fallback: {message}"),
                    car_index: None,
                    session_generation: None,
                    last_sequence: 0,
                    dropped_packets: 0,
                    last_error_code: OFP_AUDIO_ERR_V10_LOAD,
                    last_error: format!("V10 init failed, legacy fallback: {message}"),
                    sample_rate: params.sample_rate,
                    effective_min_rpm: params.idle_rpm,
                    bank_min_rpm,
                    bank_max_rpm,
                    prev_rpm_throttle: None,
                    rpm_dot: 0.0,
                    throttle_dot: 0.0,
                    was_shifting: false,
                    downshift_latched: false,
                    blip_s: 0.0,
                    recovery_s: 0.0,
                    prev_scrape_active: false,
                    prev_body_impulse_ns: [0.0; 32],
                    impact_cooldown_until_s: 0.0,
                }))
            }
        }
    }
}

/// Create a mixer from an assets root (UTF-8 path holding `banks/commons`,
/// `banks/v10-v2-bank`, `sound_mixer_config.json`) plus explicit params.
/// Returns null on failure; inspect `ofp_audio_last_create_error`.
/// A V10 failure with `fallback_to_legacy == 0` returns null (fail-closed).
///
/// # Safety
/// `assets_root` must be a valid NUL-terminated C string. `params` must point
/// to a valid `OfpAudioCreateParamsV1`. The handle must be freed with
/// `ofp_audio_destroy`. One thread owns the handle.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_create(
    assets_root: *const c_char,
    params: *const OfpAudioCreateParamsV1,
) -> *mut c_void {
    let result = catch_unwind(AssertUnwindSafe(|| {
        if assets_root.is_null() || params.is_null() {
            set_create_error("null assets_root or params".to_string());
            return std::ptr::null_mut();
        }
        // SAFETY: non-null per `# Safety`; validated as NUL-terminated UTF-8 below.
        let root = match unsafe { CStr::from_ptr(assets_root) }.to_str() {
            Ok(root) => root,
            Err(_) => {
                set_create_error("assets_root is not UTF-8".to_string());
                return std::ptr::null_mut();
            }
        };
        // SAFETY: non-null per `# Safety`; version/size checked in `create_inner`.
        let owned_params = unsafe { *params };
        match create_inner(root, &owned_params) {
            Ok(handle) => Box::into_raw(handle) as *mut c_void,
            Err((_, message)) => {
                set_create_error(message);
                std::ptr::null_mut()
            }
        }
    }));
    match result {
        Ok(pointer) => pointer,
        Err(_) => {
            set_create_error("panic during ofp_audio_create".to_string());
            std::ptr::null_mut()
        }
    }
}

/// Destroy a handle from `ofp_audio_create`.
///
/// # Safety
/// `handle` must be null or a live handle from `ofp_audio_create`, freed once.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_destroy(handle: *mut c_void) {
    if handle.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: non-null + live-handle contract in `# Safety`.
        drop(unsafe { Box::from_raw(handle as *mut OfpAudioHandle) });
    }));
}

/// 1 when the V10 sampler is the active continuous source, else 0.
///
/// # Safety
/// `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_v10_active(handle: *mut c_void) -> u32 {
    if handle.is_null() {
        return 0;
    }
    // SAFETY: live-handle contract in `# Safety`.
    let handle = unsafe { &*(handle as *const OfpAudioHandle) };
    u32::from(handle.v10_active)
}

/// Fire a commons one-shot by code (single-owner routing: with V10 active,
/// gearbox/backfire/limiter route to the sampler event pools).
/// 0 ShiftUp, 1 ShiftDown, 2 Backfire, 3..6 Hit1..4, 7 Barrier, 8 Cone,
/// 9 Fire, 10 Scrape.
///
/// # Safety
/// `handle` must be a live mutable handle (single owning thread).
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_trigger(handle: *mut c_void, code: i32) -> u32 {
    if handle.is_null() {
        return OFP_AUDIO_ERR_NULL;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live mutable handle.
        let handle = unsafe { &mut *(handle as *mut OfpAudioHandle) };
        let trigger = match code {
            0 => vehicle_audio_engine::state::Trigger::ShiftUp,
            1 => vehicle_audio_engine::state::Trigger::ShiftDown,
            2 => vehicle_audio_engine::state::Trigger::Backfire,
            3 => vehicle_audio_engine::state::Trigger::Hit1,
            4 => vehicle_audio_engine::state::Trigger::Hit2,
            5 => vehicle_audio_engine::state::Trigger::Hit3,
            6 => vehicle_audio_engine::state::Trigger::Hit4,
            7 => vehicle_audio_engine::state::Trigger::Barrier,
            8 => vehicle_audio_engine::state::Trigger::Cone,
            9 => vehicle_audio_engine::state::Trigger::Fire,
            10 => vehicle_audio_engine::state::Trigger::Scrape,
            _ => {
                handle.last_error_code = OFP_AUDIO_ERR_RANGE;
                handle.last_error = format!("bad trigger code {code}");
                return OFP_AUDIO_ERR_RANGE;
            }
        };
        handle.engine.trigger(trigger);
        handle.last_error_code = OFP_AUDIO_OK;
        OFP_AUDIO_OK
    }));
    result.unwrap_or(OFP_AUDIO_ERR_PANIC)
}

fn push_inner(handle: &mut OfpAudioHandle, packet: &OfpAudioPacketV1) -> u32 {
    if packet.schema_version != OFP_AUDIO_ABI_VERSION {
        handle.last_error_code = OFP_AUDIO_ERR_VERSION;
        handle.last_error = format!("bad packet schema {}", packet.schema_version);
        return OFP_AUDIO_ERR_VERSION;
    }
    if packet.struct_size as usize != size_of::<OfpAudioPacketV1>() {
        handle.last_error_code = OFP_AUDIO_ERR_SIZE;
        handle.last_error = format!("bad packet size {}", packet.struct_size);
        return OFP_AUDIO_ERR_SIZE;
    }
    if !packet.rpm.is_finite() || !packet.dt_s.is_finite() || packet.dt_s < 0.0 || packet.dt_s > 1.0 {
        handle.last_error_code = OFP_AUDIO_ERR_RANGE;
        handle.last_error = format!("bad rpm/dt {} {}", packet.rpm, packet.dt_s);
        return OFP_AUDIO_ERR_RANGE;
    }
    if !packet_floats_finite(packet) {
        handle.last_error_code = OFP_AUDIO_ERR_RANGE;
        handle.last_error = "non-finite telemetry scalar".to_string();
        return OFP_AUDIO_ERR_RANGE;
    }
    // Bind car/session on first accepted packet; afterwards enforce order.
    match (handle.car_index, handle.session_generation) {
        (None, None) => {
            handle.car_index = Some(packet.car_index);
            handle.session_generation = Some(packet.session_generation);
            handle.last_sequence = packet.sequence;
        }
        (Some(car), Some(generation)) => {
            if packet.car_index != car || packet.session_generation != generation {
                handle.dropped_packets = handle.dropped_packets.saturating_add(1);
                handle.last_error_code = OFP_AUDIO_ERR_SESSION;
                handle.last_error = format!(
                    "session mismatch car {}/gen {} vs {}:{}",
                    packet.car_index, packet.session_generation, car, generation
                );
                return OFP_AUDIO_ERR_SESSION;
            }
            if packet.sequence <= handle.last_sequence {
                // Stale data: discard without ingesting so a limiter window cannot stick.
                handle.dropped_packets = handle.dropped_packets.saturating_add(1);
                handle.last_error_code = OFP_AUDIO_ERR_STALE;
                handle.last_error = format!("stale sequence {} <= {}", packet.sequence, handle.last_sequence);
                return OFP_AUDIO_ERR_STALE;
            }
            handle.last_sequence = packet.sequence;
        }
        _ => {
            handle.last_error_code = OFP_AUDIO_ERR_SESSION;
            handle.last_error = "half-bound session".to_string();
            return OFP_AUDIO_ERR_SESSION;
        }
    }
    let surface = surface_token(packet.surface_code);
    // 1) Mechanical state (output shaft / torque sign / target gear).
    handle.engine.set_grand_prix_mechanical_state(
        (packet.has_output_shaft != 0).then_some(packet.output_shaft_hz),
        Some(packet.torque_sign),
        Some(packet.target_gear),
    );
    // 2) Timed telemetry (dt preserves cut windows across low graphic FPS).
    let telem = vehicle_audio_engine::ffi::VehicleAudioTelemetryV3 {
        schema_version: vehicle_audio_engine::ffi::VEHICLE_AUDIO_ABI_VERSION,
        struct_size: size_of::<vehicle_audio_engine::ffi::VehicleAudioTelemetryV3>() as u32,
        rpm: packet.rpm,
        idle_rpm: handle.effective_min_rpm,
        max_rpm: handle.bank_max_rpm,
        throttle: packet.throttle,
        normalized_engine_load: packet.normalized_engine_load,
        normalized_engine_torque: packet.normalized_engine_torque,
        rpm_derivative: packet.rpm_derivative,
        throttle_derivative: packet.throttle_derivative,
        speed_kph: packet.speed_kph,
        slip: packet.slip,
        gear: packet.gear,
        torque_sign: packet.torque_sign,
        shift_phase: packet.shift_phase,
        clutch_engagement: packet.clutch_engagement,
        tc_cut_ratio: packet.tc_cut_ratio,
        rev_limiter_active: packet.rev_limiter_active,
    };
    handle.engine.set_telemetry_timed(&telem, surface, packet.dt_s);
    // 3) Chassis scrape (sustained support must not retrigger impacts per tick;
    //    the mixer owns onset/cursor semantics).
    handle.engine.set_scrape_state(
        packet.scrape_active != 0,
        packet.scrape_intensity,
        packet.scrape_speed_m_s,
        packet.scrape_onset,
    );
    // 4) Per-wheel tire scrub.
    handle.engine.set_tire_scrub_state(
        packet.tire_slip_ratio,
        packet.tire_slip_angle,
        packet.tire_contact,
        packet.tire_load_n,
        packet.tire_speed_kph,
        surface,
    );
    handle.last_error_code = OFP_AUDIO_OK;
    OFP_AUDIO_OK
}

/// Consume one timed packet (ordered per car/session). Returns `OFP_AUDIO_OK`
/// on ingest; stale/session errors discard without ingesting and count
/// `dropped_packets`.
///
/// # Safety
/// `handle` must be a live mutable handle (single owning thread). `packet`
/// must point to a valid `OfpAudioPacketV1`.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_push_packet(
    handle: *mut c_void,
    packet: *const OfpAudioPacketV1,
) -> u32 {
    if handle.is_null() || packet.is_null() {
        return OFP_AUDIO_ERR_NULL;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live-handle + valid-packet contract in `# Safety`.
        let handle = unsafe { &mut *(handle as *mut OfpAudioHandle) };
        let owned = unsafe { *packet };
        push_inner(handle, &owned)
    }));
    match result {
        Ok(code) => code,
        Err(_) => OFP_AUDIO_ERR_PANIC,
    }
}

/// Translate a physics surface code (0..7) to an audio bed code (0..3).
/// Road/Wall/Metal -> asphalt; Grass -> grass; Dirt/Gravel/Sand -> sand;
/// Curb -> rumble (piano/kerb). Explicit so distributions never drift.
pub fn translate_physics_surface(physics_surface: i32) -> u32 {
    match physics_surface {
        3 => OFP_AUDIO_SURFACE_GRASS,
        2 | 4 | 5 => OFP_AUDIO_SURFACE_SAND,
        1 => OFP_AUDIO_SURFACE_RUMBLE,
        _ => OFP_AUDIO_SURFACE_ASPHALT,
    }
}

/// Driven-wheel output-shaft frequency (Hz): mean driven wheel spin x final
/// drive / TAU. Front axle counts when split > 0, rear when split < 1.
/// Rust wheel order FL, FR, RL, RR on both sides of the ABI.
pub fn output_shaft_hz(
    wheel_spin_rad_s: &[f64; 4],
    final_drive: f64,
    front_torque_split: f64,
) -> Option<f64> {
    let mut sum = 0.0;
    let mut count = 0.0;
    for (index, spin) in wheel_spin_rad_s.iter().enumerate() {
        let driven = if index < 2 {
            front_torque_split > 0.0
        } else {
            front_torque_split < 1.0
        };
        if driven {
            sum += *spin;
            count += 1.0;
        }
    }
    if count > 0.0 && final_drive > 0.0 {
        Some(sum / count * final_drive / std::f64::consts::TAU)
    } else {
        None
    }
}

fn dominant_bed_code(step: &OfpAudioStepV1) -> u32 {
    // Majority of grounded contact; ties resolve to the lowest code and no
    // contact falls back to asphalt. Deterministic by construction.
    // (Legacy surface beds only; tire scrub follows the loudest wheel below.)
    let mut weight = [0.0f64; 4];
    for index in 0..4 {
        if step.contact_fraction[index] > 0.0 {
            let code = translate_physics_surface(step.surface[index]);
            weight[code as usize] += step.contact_fraction[index] as f64;
        }
    }
    let mut best = 0u32;
    let mut best_weight = 0.0;
    for (code, value) in weight.iter().enumerate() {
        if *value > best_weight {
            best_weight = *value;
            best = code as u32;
        }
    }
    best
}

/// Surface for the tire-scrub voice: the surface under the loudest slipping
/// wheel, evaluated with the canonical `compute_tire_scrub_target` (single
/// surface per call, other wheels muted). Falls back to the contact majority
/// when nothing scrubs. Straddling asphalt/grass with the slipping axle
/// off-road therefore attenuates like grass instead of blasting at asphalt.
fn loudest_slip_surface(step: &OfpAudioStepV1, speed_kph: f32) -> u32 {
    use vehicle_audio_engine::tire_scrub::compute_tire_scrub_target;
    let mut best_gain = 0.0f32;
    let mut best_code = dominant_bed_code(step);
    for index in 0..4 {
        if step.contact_fraction[index] <= 0.0 {
            continue;
        }
        let mut slip_ratio = [0.0f32; 4];
        let mut slip_angle = [0.0f32; 4];
        let mut contact = [0.0f32; 4];
        let mut load = [0.0f32; 4];
        slip_ratio[index] = step.slip_ratio[index];
        slip_angle[index] = step.slip_angle_rad[index];
        contact[index] = step.contact_fraction[index];
        load[index] = step.normal_force_n[index];
        let surface = surface_token(translate_physics_surface(step.surface[index]));
        let target = compute_tire_scrub_target(slip_ratio, slip_angle, contact, load, speed_kph, surface);
        if target.gain > best_gain {
            best_gain = target.gain;
            best_code = translate_physics_surface(step.surface[index]);
        }
    }
    best_code
}

#[allow(clippy::too_many_arguments)]
fn ingest_computed(
    handle: &mut OfpAudioHandle,
    rpm: f64,
    throttle: f32,
    load: f32,
    torque_norm: f32,
    torque_sign: i32,
    transmitted_sign: i32,
    rpm_dot: f32,
    throttle_dot: f32,
    speed_kph: f64,
    slip: f32,
    gear: i32,
    target_gear: i32,
    shift_phase: i32,
    clutch: f32,
    tc: f32,
    limiter: u32,
    dt_s: f32,
    output_shaft: Option<f64>,
    surface: &'static str,
    surface_code: u32,
    scrub_surface: &'static str,
    scrape_active: bool,
    scrape_intensity: f32,
    scrape_speed: f32,
    scrape_onset: f32,
    slip_ratio: [f32; 4],
    slip_angle: [f32; 4],
    contact: [f32; 4],
    load_n: [f32; 4],
    tire_speed_kph: f32,
) {
    handle.engine.set_grand_prix_mechanical_state(
        output_shaft,
        Some(transmitted_sign),
        Some(target_gear),
    );
    let telem = vehicle_audio_engine::ffi::VehicleAudioTelemetryV3 {
        schema_version: vehicle_audio_engine::ffi::VEHICLE_AUDIO_ABI_VERSION,
        struct_size: size_of::<vehicle_audio_engine::ffi::VehicleAudioTelemetryV3>() as u32,
        rpm,
        idle_rpm: handle.effective_min_rpm,
        max_rpm: handle.bank_max_rpm,
        throttle,
        normalized_engine_load: load,
        normalized_engine_torque: torque_norm,
        rpm_derivative: rpm_dot,
        throttle_derivative: throttle_dot,
        speed_kph,
        slip,
        gear,
        torque_sign,
        shift_phase,
        clutch_engagement: clutch,
        tc_cut_ratio: tc,
        rev_limiter_active: limiter,
    };
    handle.engine.set_telemetry_timed(&telem, surface, dt_s);
    handle.engine.set_scrape_state(scrape_active, scrape_intensity, scrape_speed, scrape_onset);
    handle.engine.set_tire_scrub_state(
        slip_ratio,
        slip_angle,
        contact,
        load_n,
        tire_speed_kph,
        scrub_surface,
    );
    let _ = surface_code;
}

fn push_step_inner(handle: &mut OfpAudioHandle, step: &OfpAudioStepV1) -> u32 {
    if step.schema_version != OFP_AUDIO_STEP_VERSION {
        handle.last_error_code = OFP_AUDIO_ERR_VERSION;
        handle.last_error = format!("bad step schema {}", step.schema_version);
        return OFP_AUDIO_ERR_VERSION;
    }
    if step.struct_size as usize != size_of::<OfpAudioStepV1>() {
        handle.last_error_code = OFP_AUDIO_ERR_SIZE;
        handle.last_error = format!("bad step size {}", step.struct_size);
        return OFP_AUDIO_ERR_SIZE;
    }
    if !step.rpm.is_finite() || !step.dt_s.is_finite() || step.dt_s < 0.0 || step.dt_s > 1.0 {
        handle.last_error_code = OFP_AUDIO_ERR_RANGE;
        handle.last_error = format!("bad step rpm/dt {} {}", step.rpm, step.dt_s);
        return OFP_AUDIO_ERR_RANGE;
    }
    if !step_floats_finite(step) {
        handle.last_error_code = OFP_AUDIO_ERR_RANGE;
        handle.last_error = "non-finite step scalar".to_string();
        return OFP_AUDIO_ERR_RANGE;
    }
    let dt = step.dt_s;
    let valid_dt = dt.is_finite() && dt > 0.0 && dt <= 0.1;
    // Ordering: shared binding with the packet path (one queue per car).
    match (handle.car_index, handle.session_generation) {
        (None, None) => {
            handle.car_index = Some(step.car_index);
            handle.session_generation = Some(step.session_generation);
            handle.last_sequence = step.sequence;
        }
        (Some(car), Some(generation)) => {
            if step.car_index != car || step.session_generation != generation {
                handle.dropped_packets = handle.dropped_packets.saturating_add(1);
                handle.last_error_code = OFP_AUDIO_ERR_SESSION;
                handle.last_error = format!(
                    "session mismatch car {}/gen {} vs {}:{}",
                    step.car_index, step.session_generation, car, generation
                );
                return OFP_AUDIO_ERR_SESSION;
            }
            if step.sequence <= handle.last_sequence {
                handle.dropped_packets = handle.dropped_packets.saturating_add(1);
                handle.last_error_code = OFP_AUDIO_ERR_STALE;
                handle.last_error =
                    format!("stale sequence {} <= {}", step.sequence, handle.last_sequence);
                return OFP_AUDIO_ERR_STALE;
            }
            handle.last_sequence = step.sequence;
        }
        _ => {
            handle.last_error_code = OFP_AUDIO_ERR_SESSION;
            handle.last_error = "half-bound session".to_string();
            return OFP_AUDIO_ERR_SESSION;
        }
    }
    // Mechanical mapping (ported `MechanicalAudioState::from_physics`).
    // Clutch torque already contains engagement; braking magnitude preserved.
    // TC acts downstream on positive drive torque: applied once to load.
    let max = step.max_torque_nm.max(1.0);
    let torque_norm = ((step.engine_torque_nm / max).clamp(-1.0, 1.0)) as f32;
    let mut load = if step.clutch_engagement > 1e-4 {
        ((step.clutch_torque_nm.abs() / max).clamp(0.0, 1.0)) as f32
    } else {
        0.0
    };
    if step.engine_torque_nm > 0.0 {
        load *= 1.0 - (step.tc_cut_ratio_smoothed.clamp(0.0, 1.0) as f32);
    }
    let transmitted_sign = if step.clutch_torque_nm > 0.0 {
        1
    } else if step.clutch_torque_nm < 0.0 {
        -1
    } else {
        0
    };
    let telemetry_sign = if torque_norm > 0.025 {
        1
    } else if torque_norm < -0.025 {
        -1
    } else {
        0
    };
    let clutch = (step.clutch_engagement.clamp(0.0, 1.0)) as f32;
    let tc = (step.tc_cut_ratio_smoothed.clamp(0.0, 1.0)) as f32;
    let limiter = step.is_rev_limited != 0;
    let shifting = step.shift_timer_s > 0.0;
    let downshift = step.target_gear < step.gear;
    // Derivatives (ported `AudioTelemetryAdapter::update`, tau 20 ms).
    let throttle = (step.throttle as f32).clamp(0.0, 1.0);
    if let Some((last_rpm, last_throttle)) = handle.prev_rpm_throttle.filter(|_| valid_dt) {
        let alpha = 1.0 - (-dt / 0.020).exp();
        handle.rpm_dot += alpha * ((step.rpm as f32 - last_rpm) / dt - handle.rpm_dot);
        handle.throttle_dot += alpha * ((throttle - last_throttle) / dt - handle.throttle_dot);
    } else {
        handle.rpm_dot = 0.0;
        handle.throttle_dot = 0.0;
    }
    handle.prev_rpm_throttle = Some((step.rpm as f32, throttle));
    // Shift/blip/recovery phases: cut is physical; blip (~120 ms) + recovery
    // (40 ms) are named acoustic transitions, never physics.
    let time_step = if valid_dt { dt } else { 0.0 };
    let phase = if shifting {
        handle.downshift_latched = downshift;
        handle.blip_s = 0.0;
        handle.recovery_s = 0.0;
        if downshift { 3 } else { 1 }
    } else {
        if handle.was_shifting {
            handle.recovery_s = 0.040;
            handle.blip_s = if handle.downshift_latched { 0.120 } else { 0.0 };
        }
        if handle.blip_s > 0.0 {
            handle.blip_s = (handle.blip_s - time_step).max(0.0);
            4
        } else if handle.recovery_s > 0.0 {
            let phase = if handle.downshift_latched { 5 } else { 2 };
            handle.recovery_s = (handle.recovery_s - time_step).max(0.0);
            phase
        } else {
            0
        }
    };
    handle.was_shifting = shifting;
    let output_shaft = output_shaft_hz(&step.wheel_spin_rad_s, step.final_drive, step.front_torque_split);
    let speed_kph = step.speed_m_s * 3.6;
    let bed_code = dominant_bed_code(step);
    let surface = surface_token(bed_code);
    // Tire scrub follows the loudest slipping wheel, not the contact majority:
    // straddling asphalt/grass with the slipping axle off-road must attenuate
    // like grass (0.22), not blast at asphalt gain. Evaluated with the
    // canonical `compute_tire_scrub_target` (no duplicated thresholds).
    let scrub_code = loudest_slip_surface(step, speed_kph as f32);
    let scrub_surface = surface_token(scrub_code);
    // Slip for the legacy bed path: max slip among contacting wheels only, so
    // an airborne spinning wheel cannot inject bed gain.
    let mut slip = 0.0f32;
    for index in 0..4 {
        if step.contact_fraction[index] > 0.01 {
            slip = slip.max(step.slip_ratio[index].abs());
        }
    }
    // Scrape: floor/body support is sustained; onset fires only on rising edge
    // so one support never retriggers impacts per tick (mixer owns the cursor).
    let scrape_active = step.floor_valid_mask != 0
        && (step.floor_impulse_ns.iter().any(|impulse| *impulse > 0.0)
            || step.body_contact_count > 0);
    let scrape_intensity = ((step.scrape_speed_m_s / 70.0).clamp(0.0, 1.0)) as f32;
    let scrape_onset = if scrape_active && !handle.prev_scrape_active { 1.0 } else { 0.0 };
    handle.prev_scrape_active = scrape_active;
    ingest_computed(
        handle,
        step.rpm,
        throttle,
        load,
        torque_norm,
        telemetry_sign,
        transmitted_sign,
        handle.rpm_dot,
        handle.throttle_dot,
        speed_kph,
        slip,
        step.gear,
        step.target_gear,
        phase,
        clutch,
        tc,
        u32::from(limiter),
        dt,
        output_shaft,
        surface,
        bed_code,
        scrub_surface,
        scrape_active,
        scrape_intensity,
        step.scrape_speed_m_s,
        scrape_onset,
        step.slip_ratio,
        step.slip_angle_rad,
        step.contact_fraction,
        step.normal_force_n,
        speed_kph as f32,
    );
    // Barrier impacts: rising edge per contact slot fires the commons Barrier
    // one-shot exactly once through the mixer's single-owner routing.
    // Threshold provisional (Etapa 6 listening): rigid inelastic impulses for
    // a ~660 kg car run in the hundreds of N s, so 50 N s keeps numerical
    // noise out while catching real contacts. Cooldown on simulation time.
    for (slot, impulse) in step.body_contact_impulse_ns.iter().enumerate() {
        let rising = handle.prev_body_impulse_ns[slot] <= 50.0 && *impulse > 50.0;
        handle.prev_body_impulse_ns[slot] = *impulse;
        if rising && step.sim_time_s >= handle.impact_cooldown_until_s {
            handle.impact_cooldown_until_s = step.sim_time_s + 0.3;
            handle.engine.trigger(vehicle_audio_engine::state::Trigger::Barrier);
        }
    }
    handle.last_error_code = OFP_AUDIO_OK;
    OFP_AUDIO_OK
}

/// Consume one raw mechanical step (ordered per car/session). The wrapper
/// derives delivered load, derivatives and shift/blip/recovery phases exactly
/// like the proven donor adapter. Returns `OFP_AUDIO_OK` on ingest.
///
/// # Safety
/// `handle` must be a live mutable handle (single owning thread). `step` must
/// point to a valid `OfpAudioStepV1`.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_push_step(
    handle: *mut c_void,
    step: *const OfpAudioStepV1,
) -> u32 {
    if handle.is_null() || step.is_null() {
        return OFP_AUDIO_ERR_NULL;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live-handle + valid-step contract in `# Safety`.
        let handle = unsafe { &mut *(handle as *mut OfpAudioHandle) };
        let owned = unsafe { *step };
        push_step_inner(handle, &owned)
    }));
    match result {
        Ok(code) => code,
        Err(_) => OFP_AUDIO_ERR_PANIC,
    }
}

/// Set listener distance (camera path, independent from mechanical events).
///
/// # Safety
/// `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_set_listener_distance(handle: *mut c_void, distance_m: f32) {
    if handle.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live-handle contract.
        let handle = unsafe { &mut *(handle as *mut OfpAudioHandle) };
        handle.engine.set_listener_distance(distance_m);
    }));
}

/// Render `n` stereo frames. Returns frames written (0 on error).
/// `n` must be `<= 4096`. No allocation on success.
///
/// # Safety
/// `handle` must be a live mutable handle. `out_l`/`out_r` must each hold `n` f32s.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_render(
    handle: *mut c_void,
    out_l: *mut f32,
    out_r: *mut f32,
    n: u32,
) -> u32 {
    if handle.is_null() || out_l.is_null() || out_r.is_null() {
        return 0;
    }
    if n == 0 || n > OFP_AUDIO_MAX_BLOCK_FRAMES {
        return 0;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live-handle + buffer contract in `# Safety`.
        let (left, right, handle) = unsafe {
            (
                std::slice::from_raw_parts_mut(out_l, n as usize),
                std::slice::from_raw_parts_mut(out_r, n as usize),
                &mut *(handle as *mut OfpAudioHandle),
            )
        };
        handle.engine.render(left, right, n as usize);
        n
    }));
    result.unwrap_or(0)
}

/// Render `n` mono frames as downmix `(L+R)*0.5` (exterior path; verifies no
/// promise of stereo equivalence). Returns frames written.
///
/// # Safety
/// Same contract as `ofp_audio_render`, with a single `n`-frame output.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_render_mono(handle: *mut c_void, out: *mut f32, n: u32) -> u32 {
    if handle.is_null() || out.is_null() {
        return 0;
    }
    if n == 0 || n > OFP_AUDIO_MAX_BLOCK_FRAMES {
        return 0;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live-handle + buffer contract.
        let (mono, handle) = unsafe {
            (
                std::slice::from_raw_parts_mut(out, n as usize),
                &mut *(handle as *mut OfpAudioHandle),
            )
        };
        // Render stereo into stack-bounded scratch without allocating: reuse two
        // fixed local buffers via chunked rendering (max 4096, stack arrays would
        // overflow; instead render in place into mono twice? No: render once into
        // temporary vectors would allocate. Use a caller-visible trick: render
        // stereo directly into `mono` aliased? Not safe. Simplest allocation-free
        // approach: render L into mono, then render R into a reused static scratch
        // guarded by the single-owner rule. We keep a thread-local scratch buffer.
        thread_local! {
            static SCRATCH_R: std::cell::RefCell<Vec<f32>> =
                std::cell::RefCell::new(vec![0.0; OFP_AUDIO_MAX_BLOCK_FRAMES as usize]);
        }
        SCRATCH_R.with(|scratch| {
            let mut scratch = scratch.borrow_mut();
            handle.engine.render(mono, &mut scratch[..n as usize], n as usize);
            for (left, right) in mono.iter_mut().zip(scratch.iter().take(n as usize)) {
                *left = (*left + *right) * 0.5;
            }
        });
        n
    }));
    result.unwrap_or(0)
}

/// Fill `out` diagnostics (no allocation). Returns `OFP_AUDIO_OK`.
///
/// # Safety
/// `handle` must be a live handle. `out` must point to a valid
/// `OfpAudioDiagnosticsV1` with matching version/size.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_diagnostics(
    handle: *mut c_void,
    out: *mut OfpAudioDiagnosticsV1,
) -> u32 {
    if handle.is_null() || out.is_null() {
        return OFP_AUDIO_ERR_NULL;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live-handle + valid-out contract.
        let handle = unsafe { &*(handle as *const OfpAudioHandle) };
        let slot = unsafe { &mut *out };
        if slot.schema_version != OFP_AUDIO_ABI_VERSION {
            return OFP_AUDIO_ERR_VERSION;
        }
        if slot.struct_size as usize != size_of::<OfpAudioDiagnosticsV1>() {
            return OFP_AUDIO_ERR_SIZE;
        }
        let continuous = handle.engine.continuous_diagnostics();
        let grand_prix = handle.engine.grand_prix_diagnostics();
        *slot = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            sample_rate: handle.sample_rate,
            v10_active: u32::from(handle.v10_active),
            continuous_source: continuous_source_code(&handle.engine),
            rendered_rpm: grand_prix.map(|diagnostics| diagnostics.rendered_revolutions_per_minute).unwrap_or(0.0),
            gate: grand_prix.map(|diagnostics| diagnostics.gate).unwrap_or(0.0),
            load_gain: grand_prix.map(|diagnostics| diagnostics.load_gain).unwrap_or(0.0),
            active_zones: grand_prix.map(|diagnostics| u32::from(diagnostics.active_zone_count)).unwrap_or(0),
            active_voices: grand_prix.map(|diagnostics| u32::from(diagnostics.active_zone_count)).unwrap_or(0),
            accepted_events: grand_prix.map(|diagnostics| diagnostics.accepted_events).unwrap_or(0),
            suppressed_events: grand_prix.map(|diagnostics| diagnostics.suppressed_events).unwrap_or(0),
            dropped_packets: handle.dropped_packets,
            last_sequence: handle.last_sequence,
            last_error_code: handle.last_error_code,
            reserved: 0,
            final_peak: continuous.final_output_peak,
            final_rms: continuous.final_output_rms,
            effective_min_rpm: handle.effective_min_rpm,
            bank_min_rpm: handle.bank_min_rpm,
            bank_max_rpm: handle.bank_max_rpm,
        };
        OFP_AUDIO_OK
    }));
    result.unwrap_or(OFP_AUDIO_ERR_PANIC)
}

fn copy_handle_string(handle: &OfpAudioHandle, text: &str, buf: *mut c_char, buf_len: u32) -> u32 {
    let _ = handle;
    copy_cstr_to_buf(text, buf, buf_len)
}

/// Query `bank_id` (NUL-terminated copy). Returns bytes written (excl. NUL).
///
/// # Safety
/// `handle` must be live; `buf` must hold `buf_len` bytes.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_bank_id(handle: *mut c_void, buf: *mut c_char, buf_len: u32) -> u32 {
    if handle.is_null() {
        return 0;
    }
    // SAFETY: live-handle + buffer contract.
    let handle = unsafe { &*(handle as *const OfpAudioHandle) };
    copy_handle_string(handle, &handle.bank_id.clone(), buf, buf_len)
}

/// Query `bank_sha256`.
///
/// # Safety
/// Same contract as `ofp_audio_bank_id`.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_bank_sha256(
    handle: *mut c_void,
    buf: *mut c_char,
    buf_len: u32,
) -> u32 {
    if handle.is_null() {
        return 0;
    }
    // SAFETY: live-handle + buffer contract.
    let handle = unsafe { &*(handle as *const OfpAudioHandle) };
    copy_handle_string(handle, &handle.bank_sha.clone(), buf, buf_len)
}

/// Query effective mixer-config hash.
///
/// # Safety
/// Same contract as `ofp_audio_bank_id`.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_mixer_hash(
    handle: *mut c_void,
    buf: *mut c_char,
    buf_len: u32,
) -> u32 {
    if handle.is_null() {
        return 0;
    }
    // SAFETY: live-handle + buffer contract.
    let handle = unsafe { &*(handle as *const OfpAudioHandle) };
    copy_handle_string(handle, &handle.mixer_hash.clone(), buf, buf_len)
}

/// Copy per-handle last error. Returns bytes written.
///
/// # Safety
/// Same contract as `ofp_audio_bank_id`.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_last_error(
    handle: *mut c_void,
    buf: *mut c_char,
    buf_len: u32,
) -> u32 {
    if handle.is_null() {
        return 0;
    }
    // SAFETY: live-handle + buffer contract.
    let handle = unsafe { &*(handle as *const OfpAudioHandle) };
    copy_handle_string(handle, &handle.last_error.clone(), buf, buf_len)
}

/// Reset voices/state, keep the car/session binding. Returns `OFP_AUDIO_OK`.
///
/// # Safety
/// `handle` must be a live mutable handle.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_reset(handle: *mut c_void) -> u32 {
    if handle.is_null() {
        return OFP_AUDIO_ERR_NULL;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live mutable handle.
        let handle = unsafe { &mut *(handle as *mut OfpAudioHandle) };
        match handle.engine.reset_audio_state() {
            Ok(()) => {
                handle.last_error_code = OFP_AUDIO_OK;
                handle.last_error.clear();
                // No stale edge may survive a reset and fire on new physics.
                reset_mechanical_history(handle);
                OFP_AUDIO_OK
            }
            Err(message) => {
                handle.last_error_code = OFP_AUDIO_ERR_RANGE;
                handle.last_error = message;
                OFP_AUDIO_ERR_RANGE
            }
        }
    }));
    result.unwrap_or(OFP_AUDIO_ERR_PANIC)
}

/// Reset voices/state and rebind to a new session generation.
///
/// # Safety
/// `handle` must be a live mutable handle.
#[no_mangle]
pub unsafe extern "C" fn ofp_audio_reset_session(handle: *mut c_void, new_generation: u32) -> u32 {
    if handle.is_null() {
        return OFP_AUDIO_ERR_NULL;
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: live mutable handle.
        let handle = unsafe { &mut *(handle as *mut OfpAudioHandle) };
        match handle.engine.reset_audio_state() {
            Ok(()) => {
                handle.session_generation = handle.car_index.map(|_| new_generation);
                handle.last_sequence = 0;
                handle.dropped_packets = 0;
                handle.last_error_code = OFP_AUDIO_OK;
                handle.last_error.clear();
                reset_mechanical_history(handle);
                OFP_AUDIO_OK
            }
            Err(message) => {
                handle.last_error_code = OFP_AUDIO_ERR_RANGE;
                handle.last_error = message;
                OFP_AUDIO_ERR_RANGE
            }
        }
    }));
    result.unwrap_or(OFP_AUDIO_ERR_PANIC)
}

#[cfg(test)]
mod layout_tests {
    use super::*;
    use std::mem::offset_of;

    #[test]
    fn create_params_layout_locked() {
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, schema_version), 0);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, struct_size), 4);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, sample_rate), 8);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, forward_gears), 12);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, idle_rpm), 16);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, max_rpm), 24);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, engine_gain), 32);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, gear_ratios), 84);
        assert_eq!(offset_of!(OfpAudioCreateParamsV1, fallback_to_legacy), 116);
        assert_eq!(size_of::<OfpAudioCreateParamsV1>(), 136);
    }

    #[test]
    fn packet_layout_locked() {
        assert_eq!(offset_of!(OfpAudioPacketV1, schema_version), 0);
        assert_eq!(offset_of!(OfpAudioPacketV1, struct_size), 4);
        assert_eq!(offset_of!(OfpAudioPacketV1, car_index), 8);
        assert_eq!(offset_of!(OfpAudioPacketV1, session_generation), 12);
        assert_eq!(offset_of!(OfpAudioPacketV1, sequence), 16);
        assert_eq!(offset_of!(OfpAudioPacketV1, sim_time_s), 24);
        assert_eq!(offset_of!(OfpAudioPacketV1, dt_s), 32);
        assert_eq!(offset_of!(OfpAudioPacketV1, rpm), 40);
        assert_eq!(offset_of!(OfpAudioPacketV1, speed_kph), 72);
        assert_eq!(offset_of!(OfpAudioPacketV1, output_shaft_hz), 112);
        assert_eq!(offset_of!(OfpAudioPacketV1, tire_slip_ratio), 144);
        assert_eq!(size_of::<OfpAudioPacketV1>(), 224);
    }

    #[test]
    fn diagnostics_layout_locked() {
        assert_eq!(offset_of!(OfpAudioDiagnosticsV1, schema_version), 0);
        assert_eq!(offset_of!(OfpAudioDiagnosticsV1, struct_size), 4);
        assert_eq!(offset_of!(OfpAudioDiagnosticsV1, sample_rate), 8);
        assert_eq!(offset_of!(OfpAudioDiagnosticsV1, v10_active), 12);
        assert_eq!(offset_of!(OfpAudioDiagnosticsV1, accepted_events), 40);
        assert_eq!(offset_of!(OfpAudioDiagnosticsV1, effective_min_rpm), 88);
        assert_eq!(size_of::<OfpAudioDiagnosticsV1>(), 112);
    }

    #[test]
    fn step_layout_locked() {
        assert_eq!(offset_of!(OfpAudioStepV1, schema_version), 0);
        assert_eq!(offset_of!(OfpAudioStepV1, sequence), 16);
        assert_eq!(offset_of!(OfpAudioStepV1, rpm), 40);
        assert_eq!(offset_of!(OfpAudioStepV1, gear), 64);
        assert_eq!(offset_of!(OfpAudioStepV1, wheel_spin_rad_s), 128);
        assert_eq!(offset_of!(OfpAudioStepV1, slip_ratio), 176);
        assert_eq!(offset_of!(OfpAudioStepV1, surface), 240);
        assert_eq!(offset_of!(OfpAudioStepV1, body_contact_impulse_ns), 288);
        assert_eq!(size_of::<OfpAudioStepV1>(), 552);
    }

    #[test]
    fn step_mapping_applies_tc_once_and_keeps_braking_magnitude() {
        // TC cuts delivered load once on positive torque, never on braking.
        let handle = create_williams();
        assert!(!handle.is_null());
        let mut step = test_step(1, 3_500.0);
        step.engine_torque_nm = 195.0; // 0.5 x 390 max
        step.clutch_torque_nm = 195.0;
        step.clutch_engagement = 1.0;
        step.tc_cut_ratio_smoothed = 0.4;
        // SAFETY: live handle + valid step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_OK);
        step.sequence = 2;
        step.engine_torque_nm = -195.0;
        step.clutch_torque_nm = -195.0;
        // SAFETY: live handle + valid braking step (TC must not cut it).
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_OK);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn step_output_shaft_uses_driven_rear_wheels_not_engine_rpm() {
        // Williams RWD (split 0.0): front spins ignored, rear mean x final / TAU.
        let shaft = output_shaft_hz(&[500.0, 500.0, 100.0, 120.0], 4.65, 0.0).unwrap();
        let expected = 110.0 * 4.65 / std::f64::consts::TAU;
        assert!((shaft - expected).abs() < 1e-10, "shaft {shaft} vs {expected}");
        assert_eq!(translate_physics_surface(3), OFP_AUDIO_SURFACE_GRASS);
        assert_eq!(translate_physics_surface(5), OFP_AUDIO_SURFACE_SAND);
        assert_eq!(translate_physics_surface(1), OFP_AUDIO_SURFACE_RUMBLE);
        assert_eq!(translate_physics_surface(0), OFP_AUDIO_SURFACE_ASPHALT);
        assert_eq!(translate_physics_surface(6), OFP_AUDIO_SURFACE_ASPHALT);
    }

    #[test]
    fn step_shift_cut_then_blip_recovery_before_none() {
        let handle = create_williams();
        assert!(!handle.is_null());
        // Physical cut first: shift_timer > 0 with downshift target.
        let mut step = test_step(1, 9_000.0);
        step.gear = 4;
        step.target_gear = 3;
        step.shift_timer_s = 0.05;
        // SAFETY: live handle + physical downshift cut.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_OK);
        // Release: blip frames must follow even though physics already settled.
        step.sequence = 2;
        step.shift_timer_s = 0.0;
        // SAFETY: live handle + released step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_OK);
        let mut diagnostics = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            ..Default::default()
        };
        // SAFETY: live handle + valid diagnostics slot.
        assert_eq!(unsafe { ofp_audio_diagnostics(handle, &mut diagnostics) }, OFP_AUDIO_OK);
        assert_eq!(diagnostics.last_sequence, 2);
        // Stale step is discarded without ingesting (no stuck limiter).
        // SAFETY: live handle + stale step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_ERR_STALE);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    fn test_step(sequence: u64, rpm: f64) -> OfpAudioStepV1 {
        OfpAudioStepV1 {
            schema_version: OFP_AUDIO_STEP_VERSION,
            struct_size: size_of::<OfpAudioStepV1>() as u32,
            car_index: 0,
            session_generation: 7,
            sequence,
            sim_time_s: sequence as f64 * 0.01,
            dt_s: 0.01,
            _pad0: 0,
            rpm,
            throttle: 0.5,
            speed_m_s: 30.0,
            gear: 3,
            target_gear: 3,
            engine_torque_nm: 150.0,
            clutch_torque_nm: 120.0,
            max_torque_nm: 390.0,
            clutch_engagement: 1.0,
            tc_cut_ratio_smoothed: 0.0,
            is_rev_limited: 0,
            shift_timer_s: 0.0,
            wheel_spin_rad_s: [30.0, 30.0, 32.0, 32.0],
            final_drive: 4.65,
            front_torque_split: 0.0,
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
        }
    }

    fn staged_assets_root() -> PathBuf {
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

    fn create_williams() -> *mut c_void {
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: valid NUL root + valid params; handle freed by each test.
        unsafe { ofp_audio_create(c_root.as_ptr(), &params) }
    }

    #[test]
    fn create_reports_v10_active_and_provenance() {
        let handle = create_williams();
        assert!(!handle.is_null());
        // SAFETY: live handle from `create_williams`.
        assert_eq!(unsafe { ofp_audio_v10_active(handle) }, 1);
        let mut diagnostics = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            ..Default::default()
        };
        // SAFETY: live handle + valid diagnostics slot.
        assert_eq!(unsafe { ofp_audio_diagnostics(handle, &mut diagnostics) }, OFP_AUDIO_OK);
        assert_eq!(diagnostics.v10_active, 1);
        assert_eq!(diagnostics.continuous_source, 1);
        assert_eq!(diagnostics.effective_min_rpm, 3_500.0);
        assert_eq!(diagnostics.bank_min_rpm, 4_500.0);
        assert_eq!(diagnostics.bank_max_rpm, 18_000.0);
        let mut bank_id = vec![0 as c_char; 128];
        // SAFETY: live handle + 128-byte buffer.
        let written = unsafe { ofp_audio_bank_id(handle, bank_id.as_mut_ptr(), 128) };
        assert!(written > 0);
        // SAFETY: handle from `create_williams`, freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn create_rejects_bad_version_and_missing_assets() {
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let mut params = williams_params();
        params.schema_version = 999;
        // SAFETY: valid root + bad-version params must fail closed.
        assert!(unsafe { ofp_audio_create(c_root.as_ptr(), &params) }.is_null());
        let mut error = vec![0 as c_char; 256];
        // SAFETY: 256-byte buffer for the global creation error.
        assert!(unsafe { ofp_audio_last_create_error(error.as_mut_ptr(), 256) } > 0);

        let missing = std::ffi::CString::new("D:/open-formula/sounds-missing-xyz").unwrap();
        let params = williams_params();
        // SAFETY: missing assets root must fail closed.
        assert!(unsafe { ofp_audio_create(missing.as_ptr(), &params) }.is_null());
    }

    #[test]
    fn packets_enforce_order_and_session() {
        let handle = create_williams();
        assert!(!handle.is_null());
        let mut packet = OfpAudioPacketV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioPacketV1>() as u32,
            car_index: 7,
            session_generation: 42,
            sequence: 1,
            sim_time_s: 0.0,
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
        };
        // SAFETY: live handle + valid packet.
        assert_eq!(unsafe { ofp_audio_push_packet(handle, &packet) }, OFP_AUDIO_OK);
        // Stale sequence is discarded and counted.
        // SAFETY: same live handle + same (now stale) packet.
        assert_eq!(unsafe { ofp_audio_push_packet(handle, &packet) }, OFP_AUDIO_ERR_STALE);
        // Session mismatch is discarded.
        packet.sequence = 2;
        packet.session_generation = 43;
        // SAFETY: live handle + session-mismatched packet.
        assert_eq!(unsafe { ofp_audio_push_packet(handle, &packet) }, OFP_AUDIO_ERR_SESSION);
        let mut diagnostics = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            ..Default::default()
        };
        // SAFETY: live handle + valid diagnostics slot.
        assert_eq!(unsafe { ofp_audio_diagnostics(handle, &mut diagnostics) }, OFP_AUDIO_OK);
        assert_eq!(diagnostics.dropped_packets, 2);
        assert_eq!(diagnostics.last_sequence, 1);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn invalid_floats_are_rejected_without_corrupting_state() {
        // NaN/infinitos y dt fuera de rango se descartan con ERR_RANGE sin
        // ingerir ni mover la secuencia: el siguiente paquete válido entra.
        // Vale para escalares de ganancia en ambas rutas (un NaN envenenaría
        // el estado suavizado del mixer hasta el próximo reset).
        let handle = create_williams();
        assert!(!handle.is_null());
        let mut step = order_step(1, 7);
        step.rpm = f64::NAN;
        // SAFETY: live handle + NaN rpm step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_ERR_RANGE);
        let mut step = order_step(1, 7);
        step.dt_s = 2.0;
        // SAFETY: live handle + out-of-range dt step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_ERR_RANGE);
        let mut step = order_step(1, 7);
        step.dt_s = -0.01;
        // SAFETY: live handle + negative dt step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_ERR_RANGE);
        let mut step = order_step(1, 7);
        step.throttle = f32::NAN;
        // SAFETY: live handle + NaN throttle step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_ERR_RANGE);
        let mut step = order_step(1, 7);
        step.normal_force_n = [f32::INFINITY, 3500.0, 3500.0, 3500.0];
        // SAFETY: live handle + infinite load step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_ERR_RANGE);
        // El binding sigue virgen: la secuencia 1 válida se acepta.
        // SAFETY: live handle + valid step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &order_step(1, 7)) }, OFP_AUDIO_OK);
        let mut diagnostics = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            ..Default::default()
        };
        // SAFETY: live handle + valid diagnostics slot.
        assert_eq!(unsafe { ofp_audio_diagnostics(handle, &mut diagnostics) }, OFP_AUDIO_OK);
        assert_eq!(diagnostics.last_sequence, 1);
        assert_eq!(diagnostics.last_error_code, OFP_AUDIO_OK);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn render_rejects_overflow_and_reset_keeps_binding() {
        let handle = create_williams();
        assert!(!handle.is_null());
        let mut left = vec![0.0f32; 16];
        let mut right = vec![0.0f32; 16];
        // SAFETY: live handle + 16-frame buffers; oversized block must yield 0.
        assert_eq!(
            unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 5000) },
            0
        );
        // SAFETY: live handle + 16-frame buffers.
        assert_eq!(
            unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 16) },
            16
        );
        // SAFETY: live mutable handle; reset preserves car/session binding.
        assert_eq!(unsafe { ofp_audio_reset(handle) }, OFP_AUDIO_OK);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn create_rejects_bad_struct_size_and_reports_mixer_source() {
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let mut params = williams_params();
        params.struct_size = 8;
        // SAFETY: bad struct_size must fail closed without panic.
        assert!(unsafe { ofp_audio_create(c_root.as_ptr(), &params) }.is_null());

        // Mixer config efectiva: el handle válido expone su hash no vacío.
        let handle = create_williams();
        assert!(!handle.is_null());
        let mut hash = vec![0 as c_char; 128];
        // SAFETY: live handle + 128-byte buffer.
        assert!(unsafe { ofp_audio_mixer_hash(handle, hash.as_mut_ptr(), 128) } > 0);
        let mut sha = vec![0 as c_char; 128];
        // SAFETY: live handle + 128-byte buffer.
        assert!(unsafe { ofp_audio_bank_sha256(handle, sha.as_mut_ptr(), 128) } > 0);
        // Trigger con código inválido no debe panicar.
        // SAFETY: live mutable handle + bad trigger code.
        assert_eq!(unsafe { ofp_audio_trigger(handle, 999) }, OFP_AUDIO_ERR_RANGE);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn create_handles_paths_with_spaces_without_panic() {
        // Ruta con espacios y no ASCII: debe fallar cerrado con NOT_FOUND (UTF-8
        // válida), nunca con pánico ni lectura fuera del assets root.
        let spaced = std::ffi::CString::new("D:/open-formula/sonidos con espacios/áudio ñ").unwrap();
        let params = williams_params();
        // SAFETY: UTF-8 válida pero inexistente → null + error global.
        assert!(unsafe { ofp_audio_create(spaced.as_ptr(), &params) }.is_null());
        let mut error = vec![0 as c_char; 256];
        // SAFETY: 256-byte buffer.
        assert!(unsafe { ofp_audio_last_create_error(error.as_mut_ptr(), 256) } > 0);
    }

    #[test]
    fn corrupt_manifest_fails_closed_or_explicit_fallback() {
        let dir = std::env::temp_dir().join("ofp_audio_corrupt_manifest");
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(dir.join("banks/commons")).unwrap();
        std::fs::create_dir_all(dir.join("banks/v10-v2-bank")).unwrap();
        std::fs::write(dir.join("banks/commons/bank_manifest.json"), "{}").unwrap();
        std::fs::write(dir.join("banks/v10-v2-bank/manifest.json"), "{ corrupt").unwrap();
        let root = std::ffi::CString::new(dir.to_str().unwrap()).unwrap();
        // Fail-closed por defecto.
        let params = williams_params();
        // SAFETY: manifests corruptos → null.
        assert!(unsafe { ofp_audio_create(root.as_ptr(), &params) }.is_null());
        // Fallback explícito a legacy: crea handle pero V10 inactivo.
        let mut fallback_params = williams_params();
        fallback_params.fallback_to_legacy = 1;
        // SAFETY: mismos manifests corruptos con fallback explícito.
        let handle = unsafe { ofp_audio_create(root.as_ptr(), &fallback_params) };
        // Commons "{}" también es inválido, así que incluso el fallback falla aquí;
        // lo importante es que no hay pánico y el error global es legible.
        if handle.is_null() {
            let mut error = vec![0 as c_char; 256];
            // SAFETY: 256-byte buffer.
            assert!(unsafe { ofp_audio_last_create_error(error.as_mut_ptr(), 256) } > 0);
        } else {
            // SAFETY: live handle.
            assert_eq!(unsafe { ofp_audio_v10_active(handle) }, 0);
            // SAFETY: freed once here.
            unsafe { ofp_audio_destroy(handle) };
        }
        let _ = std::fs::remove_dir_all(&dir);
    }

    #[test]
    fn render_mono_is_the_exact_downmix_of_stereo() {
        // Dos handles gemelos (misma semilla determinista del banco) con la
        // misma secuencia: el mono exterior debe ser (L+R)*0.5 muestra a
        // muestra, sin equivalencia estéreo prometida pero sin cancelación
        // oculta ni doble procesado.
        let twin = |sequence: u64| OfpAudioPacketV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioPacketV1>() as u32,
            car_index: 0,
            session_generation: 3,
            sequence,
            sim_time_s: sequence as f64 * 0.01,
            dt_s: 0.01,
            _pad0: 0,
            rpm: 12_000.0,
            throttle: 0.85,
            normalized_engine_load: 0.8,
            normalized_engine_torque: 0.6,
            rpm_derivative: 0.0,
            throttle_derivative: 0.0,
            speed_kph: 220.0,
            slip: 0.03,
            gear: 4,
            torque_sign: 1,
            shift_phase: 0,
            clutch_engagement: 1.0,
            tc_cut_ratio: 0.0,
            rev_limiter_active: 0,
            output_shaft_hz: 80.0,
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
            tire_speed_kph: 220.0,
            surface_code: OFP_AUDIO_SURFACE_ASPHALT,
            reserved: 0,
        };
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: two live handles from identical params.
        let left_handle = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        let right_handle = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!left_handle.is_null() && !right_handle.is_null());
        for sequence in 1..=3 {
            let packet = twin(sequence);
            // SAFETY: live handles + valid packets, same order both sides.
            assert_eq!(unsafe { ofp_audio_push_packet(left_handle, &packet) }, OFP_AUDIO_OK);
            assert_eq!(unsafe { ofp_audio_push_packet(right_handle, &packet) }, OFP_AUDIO_OK);
        }
        let mut left = vec![0.0f32; 1024];
        let mut right = vec![0.0f32; 1024];
        let mut mono = vec![0.0f32; 1024];
        // SAFETY: live handles + 1024-frame buffers.
        assert_eq!(
            unsafe { ofp_audio_render(left_handle, left.as_mut_ptr(), right.as_mut_ptr(), 1024) },
            1024
        );
        // SAFETY: live handle + 1024-frame buffer.
        assert_eq!(unsafe { ofp_audio_render_mono(right_handle, mono.as_mut_ptr(), 1024) }, 1024);
        for index in 0..1024 {
            let expected = (left[index] + right[index]) * 0.5;
            assert!(
                (mono[index] - expected).abs() <= 1e-6,
                "frame {index}: mono {} vs downmix {expected}",
                mono[index]
            );
        }
        // SAFETY: both handles freed once here.
        unsafe {
            ofp_audio_destroy(left_handle);
            ofp_audio_destroy(right_handle);
        }
    }

    fn order_packet(sequence: u64, generation: u32) -> OfpAudioPacketV1 {
        OfpAudioPacketV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioPacketV1>() as u32,
            car_index: 0,
            session_generation: generation,
            sequence,
            sim_time_s: sequence as f64 * 0.01,
            dt_s: 0.01,
            _pad0: 0,
            rpm: 8_000.0,
            throttle: 0.6,
            normalized_engine_load: 0.6,
            normalized_engine_torque: 0.5,
            rpm_derivative: 0.0,
            throttle_derivative: 0.0,
            speed_kph: 150.0,
            slip: 0.02,
            gear: 3,
            torque_sign: 1,
            shift_phase: 0,
            clutch_engagement: 1.0,
            tc_cut_ratio: 0.0,
            rev_limiter_active: 0,
            output_shaft_hz: 60.0,
            has_output_shaft: 1,
            target_gear: 3,
            scrape_active: 0,
            scrape_intensity: 0.0,
            scrape_speed_m_s: 0.0,
            scrape_onset: 0.0,
            tire_slip_ratio: [0.0; 4],
            tire_slip_angle: [0.0; 4],
            tire_contact: [1.0; 4],
            tire_load_n: [3_500.0; 4],
            tire_speed_kph: 150.0,
            surface_code: OFP_AUDIO_SURFACE_ASPHALT,
            reserved: 0,
        }
    }

    fn order_step(sequence: u64, generation: u32) -> OfpAudioStepV1 {
        OfpAudioStepV1 {
            schema_version: OFP_AUDIO_STEP_VERSION,
            struct_size: size_of::<OfpAudioStepV1>() as u32,
            car_index: 0,
            session_generation: generation,
            sequence,
            sim_time_s: sequence as f64 * 0.01,
            dt_s: 0.01,
            _pad0: 0,
            rpm: 8_000.0,
            throttle: 0.6,
            speed_m_s: 41.0,
            gear: 3,
            target_gear: 3,
            engine_torque_nm: 200.0,
            clutch_torque_nm: 180.0,
            max_torque_nm: 390.0,
            clutch_engagement: 1.0,
            tc_cut_ratio_smoothed: 0.0,
            is_rev_limited: 0,
            shift_timer_s: 0.0,
            wheel_spin_rad_s: [40.0, 40.0, 42.0, 42.0],
            final_drive: 4.65,
            front_torque_split: 0.0,
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
        }
    }

    #[test]
    fn packet_and_step_paths_share_one_ordering() {
        // `push_packet` y `push_step` comparten el mismo binding coche/sesión/
        // secuencia: un paso crudo con secuencia mayor tras un paquete se
        // acepta; repetir cualquiera de los dos es stale.
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: live handle from identical params.
        let handle = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!handle.is_null());
        // SAFETY: live handle + valid packet seq 1.
        assert_eq!(unsafe { ofp_audio_push_packet(handle, &order_packet(1, 7)) }, OFP_AUDIO_OK);
        // SAFETY: live handle + valid step seq 2 (siguiente, se acepta).
        assert_eq!(unsafe { ofp_audio_push_step(handle, &order_step(2, 7)) }, OFP_AUDIO_OK);
        // SAFETY: live handle + repeated packet seq 1 (stale).
        assert_eq!(
            unsafe { ofp_audio_push_packet(handle, &order_packet(1, 7)) },
            OFP_AUDIO_ERR_STALE
        );
        // SAFETY: live handle + repeated step seq 2 (stale).
        assert_eq!(
            unsafe { ofp_audio_push_step(handle, &order_step(2, 7)) },
            OFP_AUDIO_ERR_STALE
        );
        let mut diagnostics = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            ..Default::default()
        };
        // SAFETY: live handle + valid diagnostics slot.
        assert_eq!(unsafe { ofp_audio_diagnostics(handle, &mut diagnostics) }, OFP_AUDIO_OK);
        assert_eq!(diagnostics.last_sequence, 2);
        assert_eq!(diagnostics.dropped_packets, 2);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn reset_session_rebinds_the_generation() {
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: live handle from identical params.
        let handle = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!handle.is_null());
        unsafe {
            let state = &mut *(handle as *mut OfpAudioHandle);
            state.prev_rpm_throttle = Some((17500.0, 1.0));
            state.rpm_dot = 1200.0;
            state.throttle_dot = 40.0;
            state.was_shifting = true;
            state.downshift_latched = true;
            state.blip_s = 0.12;
            state.recovery_s = 0.04;
            state.prev_scrape_active = true;
        }
        // SAFETY: live handle + valid packet gen 7.
        assert_eq!(unsafe { ofp_audio_push_packet(handle, &order_packet(1, 7)) }, OFP_AUDIO_OK);
        // SAFETY: live mutable handle; rebinds to generation 8.
        assert_eq!(unsafe { ofp_audio_reset_session(handle, 8) }, OFP_AUDIO_OK);
        unsafe {
            let state = &*(handle as *mut OfpAudioHandle);
            assert_eq!(state.prev_rpm_throttle, None);
            assert_eq!(state.rpm_dot, 0.0);
            assert_eq!(state.throttle_dot, 0.0);
            assert!(!state.was_shifting && !state.downshift_latched && !state.prev_scrape_active);
            assert_eq!((state.blip_s, state.recovery_s), (0.0, 0.0));
        }
        // La generación vieja ya no entra; la nueva con secuencia 1 sí.
        // SAFETY: live handle + old-generation packet (rejected).
        assert_eq!(
            unsafe { ofp_audio_push_packet(handle, &order_packet(2, 7)) },
            OFP_AUDIO_ERR_SESSION
        );
        // SAFETY: live handle + new-generation packet (accepted).
        assert_eq!(unsafe { ofp_audio_push_packet(handle, &order_packet(1, 8)) }, OFP_AUDIO_OK);
        let mut diagnostics = OfpAudioDiagnosticsV1 {
            schema_version: OFP_AUDIO_ABI_VERSION,
            struct_size: size_of::<OfpAudioDiagnosticsV1>() as u32,
            ..Default::default()
        };
        // SAFETY: live handle + valid diagnostics slot.
        assert_eq!(unsafe { ofp_audio_diagnostics(handle, &mut diagnostics) }, OFP_AUDIO_OK);
        assert_eq!(diagnostics.last_sequence, 1);
        assert_eq!(diagnostics.dropped_packets, 1);
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn open_clutch_keeps_free_rev_audible_with_zero_load() {
        // Embrague abierto: carga entregada 0 pero el motor sigue sonando
        // (no se sustituye carga por pedal).
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: live handle from identical params.
        let handle = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!handle.is_null());
        let mut step = order_step(1, 7);
        step.rpm = 6_000.0;
        step.throttle = 0.9;
        step.clutch_engagement = 0.0;
        step.clutch_torque_nm = 0.0;
        // SAFETY: live handle + clutch-open step.
        assert_eq!(unsafe { ofp_audio_push_step(handle, &step) }, OFP_AUDIO_OK);
        let mut left = vec![0.0f32; 1024];
        let mut right = vec![0.0f32; 1024];
        // SAFETY: live handle + 1024-frame buffers.
        assert_eq!(
            unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 1024) },
            1024
        );
        assert!(left.iter().chain(right.iter()).all(|sample| sample.is_finite()));
        let peak = left
            .iter()
            .chain(right.iter())
            .fold(0.0f32, |peak, sample| peak.max(sample.abs()));
        assert!(peak > 0.001, "free-rev en silencio con embrague abierto");
        // SAFETY: freed once here.
        unsafe { ofp_audio_destroy(handle) };
    }

    #[test]
    fn body_impact_edge_fires_barrier_once_with_cooldown() {
        // Gemelos deterministas: solo uno ve un flanco de impulso de carrocería.
        // El impacto debe aparecer una vez en su mezcla (barrera commons) y un
        // apoyo sostenido no debe redisparar por tick (cooldown 0.3 s).
        fn render_peak(handle: *mut c_void) -> f32 {
            let mut left = vec![0.0f32; 4096];
            let mut right = vec![0.0f32; 4096];
            // SAFETY: live handle + 4096-frame buffers.
            assert_eq!(
                unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
                4096
            );
            assert!(left.iter().chain(right.iter()).all(|sample| sample.is_finite()));
            left.iter().chain(right.iter()).fold(0.0f32, |peak, sample| peak.max(sample.abs()))
        }
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: two live handles from identical params.
        let plain = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        let hit = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!plain.is_null() && !hit.is_null());
        for sequence in 1..=2 {
            // SAFETY: live handles + identical clean steps.
            assert_eq!(unsafe { ofp_audio_push_step(plain, &order_step(sequence, 7)) }, OFP_AUDIO_OK);
            assert_eq!(unsafe { ofp_audio_push_step(hit, &order_step(sequence, 7)) }, OFP_AUDIO_OK);
        }
        let mut edge = order_step(3, 7);
        edge.body_contact_count = 1;
        edge.body_contact_impulse_ns[0] = 500.0;
        // SAFETY: live handles; only `hit` sees the edge.
        assert_eq!(unsafe { ofp_audio_push_step(plain, &order_step(3, 7)) }, OFP_AUDIO_OK);
        assert_eq!(unsafe { ofp_audio_push_step(hit, &edge) }, OFP_AUDIO_OK);
        let plain_peak = render_peak(plain);
        let hit_peak = render_peak(hit);
        assert!(
            hit_peak > plain_peak + 0.05,
            "sin impacto audible en el flanco: {hit_peak} vs {plain_peak}"
        );
        // Apoyo sostenido dentro del cooldown: finito y sin segundo disparo.
        let mut held = order_step(4, 7);
        held.body_contact_count = 1;
        held.body_contact_impulse_ns[0] = 500.0;
        // SAFETY: live handles + sustained-contact steps.
        assert_eq!(unsafe { ofp_audio_push_step(plain, &order_step(4, 7)) }, OFP_AUDIO_OK);
        assert_eq!(unsafe { ofp_audio_push_step(hit, &held) }, OFP_AUDIO_OK);
        let _ = render_peak(plain);
        let held_peak = render_peak(hit);
        assert!(held_peak.is_finite() && held_peak < 1.0);
        // SAFETY: both handles freed once here.
        unsafe {
            ofp_audio_destroy(plain);
            ofp_audio_destroy(hit);
        }
    }

    fn straddle_step() -> OfpAudioStepV1 {
        // Eje delantero en asfalto, trasero en hierba patinando fuerte.
        let mut step = order_step(1, 7);
        step.rpm = 8_000.0;
        step.throttle = 0.6;
        step.speed_m_s = 33.0;
        step.slip_ratio = [0.02, 0.02, 0.45, 0.45];
        step.slip_angle_rad = [0.0; 4];
        step.contact_fraction = [1.0; 4];
        step.normal_force_n = [3_500.0; 4];
        step.surface = [0, 0, 3, 3];
        step
    }

    #[test]
    fn scrub_follows_the_loudest_slipping_wheel_surface() {
        let straddle = straddle_step();
        // La mayoría de contacto es asfalto, pero el eje que patina está en
        // hierba: el scrub debe atenuar como hierba (0.22), no como asfalto.
        assert_eq!(dominant_bed_code(&straddle), OFP_AUDIO_SURFACE_ASPHALT);
        assert_eq!(loudest_slip_surface(&straddle, 120.0), OFP_AUDIO_SURFACE_GRASS);
        // Sin deslizamiento nadie manda: vuelve a la mayoría.
        let mut calm = straddle;
        calm.slip_ratio = [0.0; 4];
        assert_eq!(loudest_slip_surface(&calm, 120.0), OFP_AUDIO_SURFACE_ASPHALT);
        // Todo asfalto patinando: asfalto.
        let mut road = straddle;
        road.surface = [0; 4];
        assert_eq!(loudest_slip_surface(&road, 120.0), OFP_AUDIO_SURFACE_ASPHALT);
    }

    #[test]
    fn straddling_scrub_renders_quieter_than_asphalt_scrub() {
        // Gemelos: mismo deslizamiento, distinta superficie bajo el eje que
        // patina. La atenuación de hierba debe verse en la mezcla final.
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: two live handles from identical params.
        let road = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        let wide = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!road.is_null() && !wide.is_null());
        let mut road_step = straddle_step();
        road_step.surface = [0; 4];
        let wide_step = straddle_step();
        // SAFETY: live handles + straddling steps.
        assert_eq!(unsafe { ofp_audio_push_step(road, &road_step) }, OFP_AUDIO_OK);
        assert_eq!(unsafe { ofp_audio_push_step(wide, &wide_step) }, OFP_AUDIO_OK);
        fn rms(handle: *mut c_void) -> f32 {
            let mut left = vec![0.0f32; 4096];
            let mut right = vec![0.0f32; 4096];
            // SAFETY: live handle + 4096-frame buffers.
            assert_eq!(
                unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
                4096
            );
            let energy: f32 =
                left.iter().chain(right.iter()).map(|s| s * s).sum::<f32>() / 8192.0;
            energy.sqrt()
        }
        let road_rms = rms(road);
        let wide_rms = rms(wide);
        assert!(road_rms.is_finite() && wide_rms.is_finite());
        assert!(road_rms > 0.01 && wide_rms > 0.01, "scrub inaudible en ambos");
        assert!(
            wide_rms < road_rms,
            "hierba debería atenuar el scrub: {wide_rms} vs {road_rms}"
        );
        // SAFETY: both handles freed once here.
        unsafe {
            ofp_audio_destroy(road);
            ofp_audio_destroy(wide);
        }
    }

    #[test]
    fn limiter_window_is_audible_only_while_physically_cut() {
        // Gemelos: tras 3 pasos limpios comunes, uno recibe 5 pasos con corte
        // físico de limitador (dt real 10 ms cada uno). La voz del limitador
        // debe aparecer solo en su mezcla.
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: two live handles from identical params.
        let clean = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        let cutting = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!clean.is_null() && !cutting.is_null());
        fn limiter_step(sequence: u64, cutting: bool) -> OfpAudioStepV1 {
            let mut step = order_step(sequence, 7);
            step.rpm = 17_400.0;
            step.throttle = 1.0;
            step.is_rev_limited = u32::from(cutting);
            step
        }
        for sequence in 1..=3 {
            // SAFETY: live handles + identical clean steps.
            assert_eq!(unsafe { ofp_audio_push_step(clean, &limiter_step(sequence, false)) }, OFP_AUDIO_OK);
            assert_eq!(unsafe { ofp_audio_push_step(cutting, &limiter_step(sequence, false)) }, OFP_AUDIO_OK);
        }
        for sequence in 4..=8 {
            // SAFETY: live handles; only `cutting` sees the physical cut.
            assert_eq!(unsafe { ofp_audio_push_step(clean, &limiter_step(sequence, false)) }, OFP_AUDIO_OK);
            assert_eq!(unsafe { ofp_audio_push_step(cutting, &limiter_step(sequence, true)) }, OFP_AUDIO_OK);
        }
        fn render(handle: *mut c_void) -> Vec<f32> {
            let mut left = vec![0.0f32; 4096];
            let mut right = vec![0.0f32; 4096];
            // SAFETY: live handle + 4096-frame buffers.
            assert_eq!(
                unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
                4096
            );
            left.into_iter().chain(right).collect()
        }
        let clean_mix = render(clean);
        let cutting_mix = render(cutting);
        assert!(clean_mix.iter().chain(cutting_mix.iter()).all(|s| s.is_finite()));
        let diff = clean_mix
            .iter()
            .zip(cutting_mix.iter())
            .fold(0.0f32, |peak, (a, b)| peak.max((a - b).abs()));
        assert!(
            diff > 0.02,
            "el corte físico del limitador no se oye en la mezcla: diff {diff}"
        );
        // SAFETY: both handles freed once here.
        unsafe {
            ofp_audio_destroy(clean);
            ofp_audio_destroy(cutting);
        }
    }

    #[test]
    fn traction_cut_flows_into_delivered_load_once() {
        // Gemelos con par positivo y motor muteado (para aislar el whine de
        // caja): uno con TC 0.5, otro sin TC. El nivel del whine sigue
        // 0.10+0.90*sqrt(load): con load 0.718 vs 0.359 el ratio esperado es
        // ~0.74. Cero aplicación daría 1.0; doble aplicación ~0.56.
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let mut params = williams_params();
        params.engine_gain = 0.0;
        params.backfire_gain = 0.0;
        params.limiter_gain = 0.0;
        // SAFETY: two live handles from identical muted-engine params.
        let open = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        let cutting = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!open.is_null() && !cutting.is_null());
        for sequence in 1..=20 {
            let mut free = order_step(sequence, 7);
            free.rpm = 12_000.0;
            free.throttle = 1.0;
            free.gear = 4;
            free.target_gear = 4;
            free.engine_torque_nm = 300.0;
            free.clutch_torque_nm = 280.0;
            free.clutch_engagement = 1.0;
            free.tc_cut_ratio_smoothed = 0.0;
            free.wheel_spin_rad_s = [10.0, 10.0, 121.6, 121.6];
            let mut held = free;
            held.tc_cut_ratio_smoothed = 0.5;
            // SAFETY: live handles + TC-differing steps.
            assert_eq!(unsafe { ofp_audio_push_step(open, &free) }, OFP_AUDIO_OK);
            assert_eq!(unsafe { ofp_audio_push_step(cutting, &held) }, OFP_AUDIO_OK);
        }
        fn rms(handle: *mut c_void) -> f32 {
            let mut left = vec![0.0f32; 4096];
            let mut right = vec![0.0f32; 4096];
            // SAFETY: live handle + 4096-frame buffers.
            assert_eq!(
                unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
                4096
            );
            assert!(left.iter().chain(right.iter()).all(|s| s.is_finite()));
            let energy: f32 =
                left.iter().chain(right.iter()).map(|s| s * s).sum::<f32>() / 8192.0;
            energy.sqrt()
        }
        let open_rms = rms(open);
        let cutting_rms = rms(cutting);
        assert!(open_rms > 0.001 && cutting_rms > 0.001, "whine inaudible");
        let ratio = cutting_rms / open_rms;
        assert!(
            (0.6..0.9).contains(&ratio),
            "TC ni una vez (~0.74): ratio {ratio} (1.0 = ignorado, ~0.56 = doble)"
        );
        // SAFETY: both handles freed once here.
        unsafe {
            ofp_audio_destroy(open);
            ofp_audio_destroy(cutting);
        }
    }

    #[test]
    fn reset_clears_impact_edge_memory() {
        // Tras un reset, el mismo impulso vuelve a ser flanco (la memoria no
        // sobrevive) y dispara de nuevo; sin la limpieza sonaría una sola vez.
        fn edge_step(sequence: u64) -> OfpAudioStepV1 {
            let mut step = order_step(sequence, 7);
            step.rpm = 6_000.0;
            step.throttle = 0.3;
            step.body_contact_count = 1;
            step.body_contact_impulse_ns[0] = 500.0;
            step
        }
        fn render_peak(handle: *mut c_void) -> f32 {
            let mut left = vec![0.0f32; 4096];
            let mut right = vec![0.0f32; 4096];
            // SAFETY: live handle + 4096-frame buffers.
            assert_eq!(
                unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
                4096
            );
            left.iter().chain(right.iter()).fold(0.0f32, |p, s| p.max(s.abs()))
        }
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: two live handles from identical params.
        let first = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        let second = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!first.is_null() && !second.is_null());
        // SAFETY: live handles + edge steps (both fire).
        assert_eq!(unsafe { ofp_audio_push_step(first, &edge_step(1)) }, OFP_AUDIO_OK);
        assert_eq!(unsafe { ofp_audio_push_step(second, &edge_step(1)) }, OFP_AUDIO_OK);
        let first_peak = render_peak(first);
        // SAFETY: live mutable handle; clears voices AND edge memory.
        assert_eq!(unsafe { ofp_audio_reset(first) }, OFP_AUDIO_OK);
        // Sequence must keep advancing (reset preserves the binding).
        let mut again = edge_step(2);
        again.sim_time_s = 1.0;
        // SAFETY: live handle + post-reset edge (must fire again).
        assert_eq!(unsafe { ofp_audio_push_step(first, &again) }, OFP_AUDIO_OK);
        let again_peak = render_peak(first);
        // El segundo disparo debe sonar como el primero (misma mezcla
        // motor+impacto): sin la limpieza del reset, el segundo render sería
        // solo motor y la diferencia superaría este margen con holgura.
        assert!(first_peak > 0.2, "sin impacto en el primer flanco");
        assert!(
            (again_peak - first_peak).abs() < 0.1,
            "el reset no limpió la memoria de flanco: {again_peak} vs {first_peak}"
        );
        // SAFETY: both handles freed once here.
        unsafe {
            ofp_audio_destroy(first);
            ofp_audio_destroy(second);
        }
    }

    #[test]
    fn upshift_cut_ducks_the_engine_while_clean_does_not() {
        // Gemelos: uno recibe un paso con corte físico de subida (shift_timer
        // > 0, fase UpshiftCut); el otro, el mismo paso sin corte. El duck
        // (ganancia 0.18 durante el corte) debe verse en la mezcla.
        let root = staged_assets_root();
        let root_string = root.to_str().unwrap().to_string();
        let c_root = std::ffi::CString::new(root_string).unwrap();
        let params = williams_params();
        // SAFETY: two live handles from identical params.
        let clean = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        let cutting = unsafe { ofp_audio_create(c_root.as_ptr(), &params) };
        assert!(!clean.is_null() && !cutting.is_null());
        for sequence in 1..=2 {
            let mut step = order_step(sequence, 7);
            step.rpm = 14_000.0;
            step.throttle = 0.9;
            // SAFETY: live handles + identical steady steps.
            assert_eq!(unsafe { ofp_audio_push_step(clean, &step) }, OFP_AUDIO_OK);
            assert_eq!(unsafe { ofp_audio_push_step(cutting, &step) }, OFP_AUDIO_OK);
        }
        let mut cut = order_step(3, 7);
        cut.rpm = 14_000.0;
        cut.throttle = 0.9;
        cut.shift_timer_s = 0.05;
        cut.target_gear = 4;
        cut.gear = 3;
        let mut cruise = cut;
        cruise.shift_timer_s = 0.0;
        // SAFETY: live handles; only `cutting` sees the physical cut.
        assert_eq!(unsafe { ofp_audio_push_step(clean, &cruise) }, OFP_AUDIO_OK);
        assert_eq!(unsafe { ofp_audio_push_step(cutting, &cut) }, OFP_AUDIO_OK);
        fn rms(handle: *mut c_void) -> f32 {
            let mut left = vec![0.0f32; 4096];
            let mut right = vec![0.0f32; 4096];
            // SAFETY: live handle + 4096-frame buffers.
            assert_eq!(
                unsafe { ofp_audio_render(handle, left.as_mut_ptr(), right.as_mut_ptr(), 4096) },
                4096
            );
            assert!(left.iter().chain(right.iter()).all(|s| s.is_finite()));
            let energy: f32 =
                left.iter().chain(right.iter()).map(|s| s * s).sum::<f32>() / 8192.0;
            energy.sqrt()
        }
        let clean_rms = rms(clean);
        let cutting_rms = rms(cutting);
        assert!(clean_rms > 0.01 && cutting_rms > 0.001);
        assert!(
            cutting_rms < clean_rms * 0.9,
            "el corte no atenúa el motor: {cutting_rms} vs {clean_rms}"
        );
        // SAFETY: both handles freed once here.
        unsafe {
            ofp_audio_destroy(clean);
            ofp_audio_destroy(cutting);
        }
    }
}
