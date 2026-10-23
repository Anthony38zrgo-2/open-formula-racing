//! Versioned C ABI for the Open Formula Rust physics backend.
//!
//! Every symbol is `ofp_*` and every structure is `#[repr(C)]`, fixed-size and owned
//! by the caller. The DLL never returns Rust references, never unwinds across the
//! boundary (all panics are caught by the exported shims) and never allocates memory
//! that the caller must free.

use std::ffi::{CStr, CString, c_char, c_void};
use std::panic::{catch_unwind, AssertUnwindSafe};

use crate::open_formula::{OpenFormulaSimulation, set_track_ground_mesh, track_ground};
use crate::types::{Mat3, Vec3};

pub const OFP_ABI_VERSION: u32 = 7;
pub const OFP_MAX_CONTACTS: usize = 32;
pub const OFP_WHEELS: usize = 4;
pub const OFP_FLOOR_PROBES: usize = 2;

/// `OfpSnapshot::flags` bits describing which optional telemetry blocks are structurally
/// present in the running backend (not whether a value is plausible).
pub const OFP_FLAG_POWERTRAIN_THERMAL: u32 = 1 << 0;
pub const OFP_FLAG_WHEEL_THERMAL: u32 = 1 << 1;
pub const OFP_FLAG_BRAKE_THERMAL: u32 = 1 << 2;

#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct OfpVec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OfpVehicleSpecs {
    pub forward_gears: i32,
    pub has_reverse: i32,
    pub final_drive: f64,
    pub reverse_ratio: f64,
    pub gear_ratios: [f64; 8],
    pub tire_width_m: [f64; OFP_WHEELS],
    // ABI v5: active-profile metadata the presentation layer must not duplicate.
    pub max_rpm: f64,
    pub idle_rpm: f64,
    pub fuel_capacity_kg: f64,
    pub fuel_density_kg_per_litre: f64,
    pub steering_max_angle_rad: f64,
    pub tread_optimal_temperature_c: [f64; OFP_WHEELS],
    pub tread_optimal_window_c: [f64; OFP_WHEELS],
    pub tread_overheat_temperature_c: [f64; OFP_WHEELS],
    pub carcass_optimal_temperature_c: [f64; OFP_WHEELS],
    pub brake_optimal_min_temperature_c: f64,
    pub brake_optimal_max_temperature_c: f64,
    pub brake_fade_start_temperature_c: f64,
    pub brake_critical_temperature_c: f64,
    pub water_optimal_min_temperature_c: f64,
    pub water_optimal_max_temperature_c: f64,
    pub water_derating_temperature_c: f64,
    pub water_critical_temperature_c: f64,
    pub oil_optimal_min_temperature_c: f64,
    pub oil_optimal_max_temperature_c: f64,
    pub oil_derating_temperature_c: f64,
    pub oil_critical_temperature_c: f64,
    // ABI v6: audio presentation references (Etapa 3). Appended so v5 offsets
    // are preserved. The presentation layer must not duplicate these constants.
    pub max_torque_nm: f64,
    pub front_torque_split: f64,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OfpControlInput {
    pub throttle: f64,
    pub brake: f64,
    pub handbrake: f64,
    pub clutch: f64,
    pub steering: f64,
    pub gear_request: i32,
    pub reserved: i32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OfpBodyContact {
    pub activation: i32,
    pub surface: i32,
    pub point: OfpVec3,
    pub normal: OfpVec3,
    pub penetration_m: f64,
    pub friction: f64,
    pub restitution: f64,
    pub damage: f64,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OfpWheelState {
    pub grounded: i32,
    pub surface: i32,
    pub ray_hits: i32,
    pub ray_hit_mask: u32,
    pub spin_rad_s: f64,
    pub steer_angle_rad: f64,
    pub slip_ratio: f64,
    pub slip_angle_rad: f64,
    pub longitudinal_force_n: f64,
    pub lateral_force_n: f64,
    pub aligning_torque_nm: f64,
    pub normal_force_n: f64,
    pub suspension_travel_m: f64,
    pub suspension_velocity_m_s: f64,
    pub spring_force_n: f64,
    pub damper_force_n: f64,
    pub contact_fraction: f64,
    /// Body-relative visual camber; tire telemetry keeps road-effective camber separately.
    pub dynamic_camber_rad: f64,
    pub toe_rad: f64,
    pub tire_deflection_m: f64,
    /// Lumped tread centre temperature (legacy single-zone field; the three zones below
    /// are the authoritative independent measurements).
    pub tread_temperature_c: f64,
    pub carcass_temperature_c: f64,
    /// Gauge pressure: measured relative to the profile atmospheric pressure.
    pub pressure_kpa_gauge: f64,
    pub wear_remaining_fraction: f64,
    pub wear_grip_scale: f64,
    pub brake_torque_nm: f64,
    pub contact_point: OfpVec3,
    pub contact_normal: OfpVec3,
    pub hub_position: OfpVec3,
    pub spring_length_m: f64,
    pub tire_radius_m: f64,
    pub ray_distance: [f64; 3],
    // ABI v5: independent thermal nodes of this corner.
    pub tread_inner_temperature_c: f64,
    pub tread_outer_temperature_c: f64,
    pub gas_temperature_c: f64,
    pub brake_disc_temperature_c: f64,
    pub brake_rim_temperature_c: f64,
    pub brake_efficiency: f64,
}

impl OfpWheelState {
    pub fn zeroed() -> Self {
        Self::default()
    }
}

/// ABI v7: what one corner's vertical solve measured, including the constraint that
/// held it and the gap it still had to close. Measurements only; never an input.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OfpWheelDiagnostics {
    pub ground_detected: i32,
    pub ground_binding: i32,
    pub droop_stop_active: i32,
    pub bump_stop_active: i32,
    pub trapped_at_stop: i32,
    pub reserved0: i32,
    /// Current unsprung-coordinate compression (m).
    pub compression_m: f64,
    /// Gap to the ground limit along the suspension axis (m, + = airborne, NaN = no
    /// ground plane inside any ray).
    pub ground_separation_m: f64,
    /// The same gap projected on the true contact normal (m).
    pub ground_separation_normal_m: f64,
    /// Distance the rays still needed beyond their own length (m, + = no ground).
    pub ray_shortfall_m: f64,
    /// Compression the ground demanded at the start of the interval (m, NaN = none).
    pub ground_demand_m: f64,
    /// Compression the ground demands at the end of the interval (m).
    pub ground_demand_predicted_m: f64,
    /// Maximum compression the linkage admits (m).
    pub max_compression_m: f64,
    /// Minimum compression the linkage admits (m).
    pub min_compression_m: f64,
    /// `max_compression_m - ground_demand_predicted_m` (m, negative = infeasible).
    pub headroom_to_bump_m: f64,
    /// Travel from design (m, + = bump).
    pub travel_m: f64,
    /// Axial velocity along the body up axis (m/s, + = compressing).
    pub axial_velocity_m_s: f64,
    /// Spring force at the wheel (N, + pushes the wheel down).
    pub spring_force_n: f64,
    /// Damper force at the wheel (N, + pushes the wheel down).
    pub damper_force_n: f64,
    /// Anti-roll force at the wheel (N, signed).
    pub antiroll_force_n: f64,
    /// Travel-stop force at the wheel (N, signed).
    pub stop_force_n: f64,
    /// Motion ratio r = ds/dq (dimensionless).
    pub motion_ratio: f64,
    /// Signed sum of the four elements at the wheel (N).
    pub element_force_sum_n: f64,
    /// Accepted ground reaction impulse over the step (N s).
    pub contact_impulse_ns: f64,
    /// Accepted droop/extension stop impulse (N s, <= 0).
    pub droop_stop_impulse_ns: f64,
    /// Accepted bump/compression stop impulse (N s, >= 0).
    pub bump_stop_impulse_ns: f64,
    /// Chassis-side attachment point (world, m).
    pub anchor_position: OfpVec3,
    /// Attachment-point velocity (world, m/s).
    pub anchor_velocity: OfpVec3,
    /// Attachment-point acceleration (world, m/s^2).
    pub anchor_acceleration: OfpVec3,
    /// Attachment acceleration projected on the suspension axis (m/s^2).
    pub axial_anchor_acceleration_m_s2: f64,
}

/// ABI v7: one axle's simultaneous solve.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OfpAxleDiagnostics {
    /// Fixed-point iterations consumed by the last axle substep.
    pub iterations: u32,
    /// Non-zero when every projected residual closed within tolerance.
    pub converged: i32,
    /// Substeps the axle integrated in the last step.
    pub substeps: u32,
    pub reserved0: i32,
    /// Worst projected residual of the last axle solve (m/s).
    pub residual_m_s: f64,
    /// Anti-roll stiffness the axle used (N/m).
    pub antiroll_stiffness_n_per_m: f64,
    /// Anti-roll force on the axle's first corner (N).
    pub antiroll_force_n: f64,
    /// Sum of both corners' ground reactions (N).
    pub normal_load_n: f64,
}

/// ABI v7: the constraint combination that rejected the step, with the corner that
/// produced it. `active != 0` means the published state is frozen at the last
/// accepted step.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct OfpFailureDiagnostics {
    /// 1 when the published state is frozen by a rejection.
    pub active: i32,
    /// 1 ground beyond bump limit, 2 axle not converged, 3 body/corner not
    /// converged, 4 non-finite state, 0 none.
    pub code: i32,
    /// Offending corner 0..3 (FL/FR/RL/RR) or -1 when not per wheel.
    pub wheel: i32,
    /// 0 front, 1 rear, -1 unknown.
    pub axle: i32,
    /// Iteration that produced the rejection (0 = unknown).
    pub iteration: u32,
    /// Subdivision depth the transaction reached before giving up.
    pub subdivisions: u32,
    /// Compression the ground demanded at the end of the interval (m).
    pub demand_m: f64,
    /// Maximum compression the linkage admits (m).
    pub limit_m: f64,
    /// `demand_m - limit_m` (m, positive = the incompatibility).
    pub excess_m: f64,
    /// Rate of change of the ground limit (m/s).
    pub limit_velocity_m_s: f64,
    /// Worst projected residual of the failing solve (m/s).
    pub residual_m_s: f64,
    /// Both corners of the failing axle.
    pub axle_wheel: [i32; 2],
    pub reserved1: i32,
    pub reserved2: i32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct OfpSnapshot {
    pub abi_version: u32,
    pub flags: u32,
    pub physics_hz: i32,
    pub gear: i32,
    pub target_gear: i32,
    pub backends: u32,
    pub rejected_steps: u32,
    pub sim_time_s: f64,
    pub position: OfpVec3,
    pub linear_velocity: OfpVec3,
    pub angular_velocity: OfpVec3,
    pub linear_acceleration: OfpVec3,
    pub basis_x: OfpVec3,
    pub basis_y: OfpVec3,
    pub basis_z: OfpVec3,
    /// Engine speed in revolutions per minute (ABI v5 renamed this field: the previous
    /// `engine_rpm_rad_s` name was wrong and invited a double conversion).
    pub engine_rpm: f64,
    pub engine_torque_nm: f64,
    pub clutch_engagement: f64,
    pub fuel_kg: f64,
    pub fuel_capacity_kg: f64,
    pub throttle: f64,
    pub brake: f64,
    pub steer_input: f64,
    pub steering_angle_rad: f64,
    pub aero_front_downforce_n: f64,
    pub aero_floor_downforce_n: f64,
    pub aero_rear_downforce_n: f64,
    pub aero_drag_n: f64,
    /// Coolant (water) temperature in degrees Celsius.
    pub water_temperature_celsius: f64,
    /// Oil temperature in degrees Celsius.
    pub oil_temperature_celsius: f64,
    pub wheels: [OfpWheelState; OFP_WHEELS],
    pub floor_clearance: [f64; OFP_FLOOR_PROBES],
    pub floor_valid_mask: u32,
    pub floor_impulse_ns: [f64; OFP_FLOOR_PROBES],
    pub floor_contact_point: [OfpVec3; OFP_FLOOR_PROBES],
    pub floor_contact_normal: [OfpVec3; OFP_FLOOR_PROBES],
    pub wheel_ray_queries: u64,
    pub wheel_ray_hits: u64,
    pub floor_ray_queries: u64,
    pub floor_ray_hits: u64,
    pub body_contact_count: u32,
    pub body_contact_impulse_ns: [f64; OFP_MAX_CONTACTS],
    // ABI v6: raw audio-mechanical state (Etapa 3). Appended after the v5 tail
    // so every v5 offset is preserved. No acoustic decision is made here; the
    // audio wrapper derives load/derivatives/shift phases from these fields.
    // Clutch-transmitted torque (signed, N·m). Sign owns the transmission side.
    pub clutch_torque_nm: f64,
    // Smoothed physical traction-control cut [0,1] (already applied downstream
    // on positive drive torque by the physics model).
    pub tc_cut_ratio_smoothed: f64,
    // Discrete physical rev-limiter state (1 when cutting). Windows are timed
    // by `step_dt_s`, never replaced by the last graphic frame.
    pub is_rev_limited: u32,
    // Remaining physical shift-cut time (s). >0 means a real cut is in progress.
    pub shift_timer_s: f64,
    // Real duration of the accepted step (s). Audio cut windows use this clock.
    pub step_dt_s: f64,
    // Per-handle accepted-step counter. Audio packets reuse it for ordering.
    pub sequence: u64,
    // ABI v7: vertical-suspension diagnostics (appended; every v6 offset preserved).
    /// Chassis angular acceleration (world, rad/s^2).
    pub angular_acceleration: OfpVec3,
    /// One record per corner in FL, FR, RL, RR order.
    pub wheel_diagnostics: [OfpWheelDiagnostics; OFP_WHEELS],
    /// Front and rear axle solves.
    pub axle_diagnostics: [OfpAxleDiagnostics; 2],
    /// Why the last step was rejected, and which corner produced it.
    pub failure: OfpFailureDiagnostics,
    /// Records currently buffered by the trace ring (0 when tracing is off).
    pub trace_records: u32,
    pub reserved_v7: u32,
}

impl OfpSnapshot {
    pub fn zeroed() -> Self {
        Self {
            abi_version: OFP_ABI_VERSION,
            flags: 0,
            physics_hz: 0,
            gear: 0,
            target_gear: 0,
            backends: 0,
            rejected_steps: 0,
            sim_time_s: 0.0,
            position: OfpVec3::default(),
            linear_velocity: OfpVec3::default(),
            angular_velocity: OfpVec3::default(),
            linear_acceleration: OfpVec3::default(),
            basis_x: OfpVec3 { x: 1.0, y: 0.0, z: 0.0 },
            basis_y: OfpVec3 { x: 0.0, y: 1.0, z: 0.0 },
            basis_z: OfpVec3 { x: 0.0, y: 0.0, z: 1.0 },
            engine_rpm: 0.0,
            engine_torque_nm: 0.0,
            clutch_engagement: 0.0,
            fuel_kg: 0.0,
            fuel_capacity_kg: 0.0,
            throttle: 0.0,
            brake: 0.0,
            steer_input: 0.0,
            steering_angle_rad: 0.0,
            aero_front_downforce_n: 0.0,
            aero_floor_downforce_n: 0.0,
            aero_rear_downforce_n: 0.0,
            aero_drag_n: 0.0,
            water_temperature_celsius: 0.0,
            oil_temperature_celsius: 0.0,
            wheels: [OfpWheelState::zeroed(); OFP_WHEELS],
            floor_clearance: [0.0; OFP_FLOOR_PROBES],
            floor_valid_mask: 0,
            floor_impulse_ns: [0.0; OFP_FLOOR_PROBES],
            floor_contact_point: [OfpVec3::default(); OFP_FLOOR_PROBES],
            floor_contact_normal: [OfpVec3::default(); OFP_FLOOR_PROBES],
            wheel_ray_queries: 0,
            wheel_ray_hits: 0,
            floor_ray_queries: 0,
            floor_ray_hits: 0,
            body_contact_count: 0,
            body_contact_impulse_ns: [0.0; OFP_MAX_CONTACTS],
            clutch_torque_nm: 0.0,
            tc_cut_ratio_smoothed: 0.0,
            is_rev_limited: 0,
            shift_timer_s: 0.0,
            step_dt_s: 0.0,
            sequence: 0,
            angular_acceleration: OfpVec3::default(),
            wheel_diagnostics: [OfpWheelDiagnostics::default(); OFP_WHEELS],
            axle_diagnostics: [OfpAxleDiagnostics::default(); 2],
            failure: OfpFailureDiagnostics::default(),
            trace_records: 0,
            reserved_v7: 0,
        }
    }
}

/// Per-handle failure text, so the host can report the concrete cause.
type Handle = OpenFormulaSimulation;

fn handle_ref<'a>(pointer: *const c_void) -> Option<&'a Handle> {
    if pointer.is_null() {
        None
    } else {
        // SAFETY: the ABI contract requires a pointer returned by `ofp_create` that has
        // not been destroyed. The reference lives only for this call.
        Some(unsafe { &*(pointer as *const Handle) })
    }
}

fn handle_mut<'a>(pointer: *mut c_void) -> Option<&'a mut Handle> {
    if pointer.is_null() {
        None
    } else {
        // SAFETY: see `handle_ref`; mutable access is exclusive for the call duration.
        Some(unsafe { &mut *(pointer as *mut Handle) })
    }
}

fn write_error(target: *mut c_char, capacity: u32, message: &str) {
    if target.is_null() || capacity == 0 {
        return;
    }
    let mut bytes: Vec<u8> = message.as_bytes().to_vec();
    bytes.truncate(capacity as usize - 1);
    // SAFETY: the caller guarantees a writable buffer of `capacity` bytes.
    unsafe {
        std::ptr::copy_nonoverlapping(bytes.as_ptr(), target as *mut u8, bytes.len());
        *(target as *mut u8).add(bytes.len()) = 0;
    }
}

fn read_c_string<'a>(pointer: *const c_char) -> Option<&'a str> {
    if pointer.is_null() {
        return None;
    }
    // SAFETY: the caller guarantees a NUL-terminated string valid for the call.
    unsafe { CStr::from_ptr(pointer) }.to_str().ok()
}

// ---------------------------------------------------------------------------
// Lifetime, provenance and world
// ---------------------------------------------------------------------------

#[no_mangle]
pub extern "C" fn ofp_abi_version() -> u32 {
    OFP_ABI_VERSION
}

/// Static provenance string: source commit of the copied physics, the destination
/// build hash and the embedded profile identifier.
/// Layout signature: FNV-1a over the sizes and offsets that define the ABI. The host
/// computes the same value from its own declarations; a mismatch aborts the load
/// instead of silently reinterpreting memory.
fn mix_layout(hash: &mut u64, value: u64) {
    *hash ^= value;
    *hash = hash.wrapping_mul(0x0000_0100_0000_01b3);
}

#[no_mangle]
pub extern "C" fn ofp_abi_signature() -> u64 {
    let mut hash: u64 = 0xcbf2_9ce4_8422_2325;
    mix_layout(&mut hash, std::mem::size_of::<OfpVehicleSpecs>() as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpVehicleSpecs, tire_width_m) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpVehicleSpecs, tread_optimal_temperature_c) as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpSnapshot>() as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpWheelState>() as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpBodyContact>() as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpControlInput>() as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, wheels) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, floor_clearance) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, water_temperature_celsius) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, body_contact_impulse_ns) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpWheelState, tread_temperature_c) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpWheelState, contact_point) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpWheelState, ray_distance) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpWheelState, brake_disc_temperature_c) as u64);
    // ABI v6 tail (appended; v5 prefix untouched).
    mix_layout(&mut hash, std::mem::offset_of!(OfpVehicleSpecs, max_torque_nm) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, clutch_torque_nm) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, sequence) as u64);
    // ABI v7 tail (appended; every v6 offset untouched).
    mix_layout(&mut hash, std::mem::size_of::<OfpWheelDiagnostics>() as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpFailureDiagnostics>() as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, angular_acceleration) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, wheel_diagnostics) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, axle_diagnostics) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, failure) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, trace_records) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpWheelDiagnostics, anchor_position) as u64);
    hash
}

#[no_mangle]
pub extern "C" fn ofp_build_provenance() -> *const c_char {
    static PROVENANCE: std::sync::OnceLock<CString> = std::sync::OnceLock::new();
    PROVENANCE
        .get_or_init(|| {
            let text = format!(
                "open-formula-rust-physics abi={} source_commit={} build={} profile=williams-fw19-hockenheim",
                OFP_ABI_VERSION,
                env!("PHYSICS_SOURCE_COMMIT"),
                env!("GIT_HASH")
            );
            CString::new(text).unwrap_or_else(|_| CString::new("abi").unwrap())
        })
        .as_ptr()
}

#[no_mangle]
pub extern "C" fn ofp_load_track_ground_mesh(path: *const c_char) -> i32 {
    let result = catch_unwind(AssertUnwindSafe(|| -> Result<i32, String> {
        let path = read_c_string(path).ok_or_else(|| "ground mesh path is not valid UTF-8".to_string())?;
        let count = set_track_ground_mesh(path)?;
        Ok(count as i32)
    }));
    match result {
        Ok(Ok(count)) => count,
        Ok(Err(_)) => -1,
        Err(_) => -2,
    }
}

#[no_mangle]
pub extern "C" fn ofp_track_ground_triangles() -> i32 {
    track_ground().map(|world| world.triangle_count() as i32).unwrap_or(-1)
}

#[no_mangle]
pub extern "C" fn ofp_create(
    profile_path: *const c_char,
    origin: OfpVec3,
    yaw_radians: f64,
    fuel_kg: f64,
) -> *mut c_void {
    let result = catch_unwind(AssertUnwindSafe(|| -> Result<*mut c_void, String> {
        let path = match profile_path {
            p if p.is_null() => None,
            p => Some(std::path::PathBuf::from(
                read_c_string(p).ok_or_else(|| "profile path is not valid UTF-8".to_string())?,
            )),
        };
        let fuel = if fuel_kg > 0.0 { Some(fuel_kg) } else { None };
        let simulation = OpenFormulaSimulation::create(
            path.as_deref(),
            Vec3::new(origin.x, origin.y, origin.z),
            yaw_radians,
            fuel,
        )?;
        Ok(Box::into_raw(Box::new(simulation)) as *mut c_void)
    }));
    match result {
        Ok(Ok(pointer)) => pointer,
        Ok(Err(error)) => { eprintln!("Rust physics create failed: {error}"); std::ptr::null_mut() },
        Err(_) => { eprintln!("Rust physics create panicked"); std::ptr::null_mut() },
    }
}

#[no_mangle]
pub extern "C" fn ofp_destroy(handle: *mut c_void) {
    if handle.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: `handle` came from `Box::into_raw` in `ofp_create` and is destroyed once.
        drop(unsafe { Box::from_raw(handle as *mut Handle) });
    }));
}

#[no_mangle]
pub extern "C" fn ofp_reset(handle: *mut c_void, origin: OfpVec3, yaw_radians: f64) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        match handle_mut(handle) {
            Some(simulation) => {
                simulation.reset(Vec3::new(origin.x, origin.y, origin.z), yaw_radians);
                0
            }
            None => -1,
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

#[no_mangle]
pub extern "C" fn ofp_set_pose(
    handle: *mut c_void,
    origin: OfpVec3,
    basis_x: OfpVec3,
    basis_y: OfpVec3,
    basis_z: OfpVec3,
) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        match handle_mut(handle) {
            Some(simulation) => {
                let basis = Mat3::from_cols(vec3(basis_x), vec3(basis_y), vec3(basis_z));
                simulation.set_pose(Vec3::new(origin.x, origin.y, origin.z), basis);
                0
            }
            None => -1,
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

fn vec3(value: OfpVec3) -> Vec3 {
    Vec3::new(value.x, value.y, value.z)
}

#[no_mangle]
pub extern "C" fn ofp_step(
    handle: *mut c_void,
    input: *const OfpControlInput,
    contacts: *const OfpBodyContact,
    contact_count: u32,
    duration_seconds: f64,
) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        let simulation = match handle_mut(handle) {
            Some(simulation) => simulation,
            None => return -1,
        };
        let input = if input.is_null() {
            OfpControlInput::default()
        } else {
            // SAFETY: the caller guarantees a readable `OfpControlInput`.
            unsafe { *input }
        };
        let count = (contact_count as usize).min(OFP_MAX_CONTACTS);
        let contacts: Vec<OfpBodyContact> = if contacts.is_null() || count == 0 {
            Vec::new()
        } else {
            // SAFETY: the caller guarantees `count` readable contacts.
            unsafe { std::slice::from_raw_parts(contacts, count) }.to_vec()
        };
        match simulation.step(&input, &contacts, duration_seconds) {
            Ok(()) => 0,
            Err(_) => 1,
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

#[no_mangle]
pub extern "C" fn ofp_snapshot(handle: *const c_void, out: *mut OfpSnapshot) -> i32 {
    if out.is_null() {
        return -1;
    }
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        match handle_ref(handle) {
            Some(simulation) => {
                let snapshot = simulation.snapshot();
                // SAFETY: `out` is non-null and writable per the ABI contract.
                unsafe { *out = snapshot };
                0
            }
            None => -1,
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

#[no_mangle]
pub extern "C" fn ofp_last_error(
    handle: *const c_void,
    buffer: *mut c_char,
    capacity: u32,
) -> i32 {
    match handle_ref(handle) {
        Some(simulation) => {
            write_error(buffer, capacity, simulation.last_error());
            simulation.last_error().len() as i32
        }
        None => {
            write_error(buffer, capacity, "invalid physics handle");
            -1
        }
    }
}

#[no_mangle]
pub extern "C" fn ofp_backend_mask(handle: *const c_void) -> u32 {
    handle_ref(handle).map(|simulation| simulation.backends()).unwrap_or(0)
}

// ---------------------------------------------------------------------------
// Setup transaction
// ---------------------------------------------------------------------------

/// Serializes the current resolved configuration, the base profile and the editable
/// field descriptor into `buffer`. Returns the required length (truncated copy when the
/// buffer is too small).
#[no_mangle]
pub extern "C" fn ofp_setup_document(
    handle: *const c_void,
    buffer: *mut c_char,
    capacity: u32,
) -> i32 {
    let simulation = match handle_ref(handle) {
        Some(simulation) => simulation,
        None => return -1,
    };
    let text = simulation.setup_document_json();
    let bytes = text.as_bytes();
    if !buffer.is_null() && capacity > 0 {
        let length = bytes.len().min(capacity as usize - 1);
        // SAFETY: the caller guarantees a writable buffer of `capacity` bytes.
        unsafe {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer as *mut u8, length);
            *(buffer as *mut u8).add(length) = 0;
        }
    }
    bytes.len() as i32
}

/// Copies transmission and tire geometry from the current authoritative configuration.
#[no_mangle]
pub extern "C" fn ofp_vehicle_specs(handle: *const c_void, out: *mut OfpVehicleSpecs) -> i32 {
    if out.is_null() { return -1; }
    let simulation = match handle_ref(handle) { Some(s) => s, None => return -1 };
    // SAFETY: the caller supplies one writable OfpVehicleSpecs with the checked ABI.
    unsafe { *out = simulation.vehicle_specs(); }
    0
}

/// Tab-delimited descriptors (path, label, unit, value, minimum, maximum, step, editable).
#[no_mangle]
pub extern "C" fn ofp_setup_fields(handle: *const c_void, buffer: *mut c_char, capacity: u32) -> i32 {
    let simulation = match handle_ref(handle) { Some(s) => s, None => return -1 };
    let text = simulation.setup_fields_text();
    write_error(buffer, capacity, &text);
    text.len() as i32
}

#[no_mangle]
pub extern "C" fn ofp_fuel_density(handle: *const c_void) -> f64 {
    handle_ref(handle).map(|s| s.fuel_density_kg_per_litre()).unwrap_or(0.0)
}

/// Applies a dotted-path numeric patch. Returns 0 on success. On failure nothing is
/// committed and `ofp_last_error` carries the reason.
#[no_mangle]
pub extern "C" fn ofp_apply_setup(handle: *mut c_void, text: *const c_char) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        let simulation = match handle_mut(handle) {
            Some(simulation) => simulation,
            None => return -1,
        };
        let text = match read_c_string(text) {
            Some(text) => text,
            None => {
                simulation.set_error("setup patch is not valid UTF-8");
                return 1;
            }
        };
        match simulation.apply_setup_json(text) {
            Ok(()) => 0,
            Err(error) => {
                simulation.set_error(&error);
                1
            }
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

#[no_mangle]
pub extern "C" fn ofp_reset_setup(handle: *mut c_void) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        let simulation = match handle_mut(handle) {
            Some(simulation) => simulation,
            None => return -1,
        };
        match simulation.reset_setup() {
            Ok(()) => 0,
            Err(error) => {
                simulation.set_error(&error);
                1
            }
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

#[no_mangle]
pub extern "C" fn ofp_pit_fuel(handle: *mut c_void, kilograms: f64) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        let simulation = match handle_mut(handle) {
            Some(simulation) => simulation,
            None => return -1,
        };
        match simulation.pit_fuel(kilograms) {
            Ok(()) => 0,
            Err(error) => {
                simulation.set_error(&error);
                1
            }
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

#[no_mangle]
pub extern "C" fn ofp_pit_tires(handle: *mut c_void) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        match handle_mut(handle) {
            Some(simulation) => {
                simulation.pit_tires();
                0
            }
            None => -1,
        }
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

/// Vehicle-local geometry the host needs for presentation and placement.
#[no_mangle]
pub extern "C" fn ofp_vehicle_geometry(
    handle: *const c_void,
    out_center_of_mass: *mut OfpVec3,
    out_inertia: *mut OfpVec3,
    out_spawn_height: *mut f64,
    out_mass: *mut f64,
) -> i32 {
    match handle_ref(handle) {
        Some(simulation) => {
            let geometry = simulation.geometry();
            // SAFETY: each output is optional; non-null pointers are writable.
            unsafe {
                if !out_center_of_mass.is_null() {
                    *out_center_of_mass = OfpVec3 {
                        x: geometry.center_of_mass.x,
                        y: geometry.center_of_mass.y,
                        z: geometry.center_of_mass.z,
                    };
                }
                if !out_inertia.is_null() {
                    *out_inertia = OfpVec3 {
                        x: geometry.inertia.x,
                        y: geometry.inertia.y,
                        z: geometry.inertia.z,
                    };
                }
                if !out_spawn_height.is_null() {
                    *out_spawn_height = geometry.spawn_height;
                }
                if !out_mass.is_null() {
                    *out_mass = geometry.mass;
                }
            }
            0
        }
        None => -1,
    }
}

/// ABI v7: turns the substep/iteration trace on and points it at `directory`.
///
/// The ring is process-wide (one vehicle per session is instrumented) and costs one
/// relaxed atomic load per record when it is off. `directory == NULL` keeps the
/// records in memory so `ofp_trace_dump` can still write them elsewhere.
#[no_mangle]
pub extern "C" fn ofp_trace_configure(directory: *const c_char) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        let path = read_c_string(directory).map(|value| value.to_string());
        crate::vertical_diagnostics::configure(path.as_deref());
        0
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

/// ABI v7: writes every buffered trace record to `path`. Returns the row count, or a
/// negative code when tracing is off or the file cannot be written.
#[no_mangle]
pub extern "C" fn ofp_trace_dump(path: *const c_char) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        let Some(path) = read_c_string(path) else { return -1 };
        match crate::vertical_diagnostics::trace_dump(path) {
            Some(rows) => rows as i32,
            None => -2,
        }
    })) {
        Ok(code) => code,
        Err(_) => -3,
    }
}

/// ABI v7: records currently buffered by the ring (0 when tracing is off).
#[no_mangle]
pub extern "C" fn ofp_trace_buffered() -> u32 {
    crate::vertical_diagnostics::buffered() as u32
}

/// ABI v7: the effective setup actually in force, one `FIELD`/`DERIVED` line per
/// value, with units, admitted limits and the derived coefficients the solver used.
/// Returns the byte count, or a negative code when the handle is invalid.
#[no_mangle]
pub extern "C" fn ofp_effective_setup(
    handle: *const c_void,
    buffer: *mut c_char,
    capacity: u32,
) -> i32 {
    match catch_unwind(AssertUnwindSafe(|| -> i32 {
        let simulation = match handle_ref(handle) {
            Some(simulation) => simulation,
            None => return -1,
        };
        let text = simulation.effective_setup_report();
        if buffer.is_null() || capacity == 0 {
            return text.len() as i32;
        }
        let mut bytes = text.into_bytes();
        bytes.truncate(capacity as usize - 1);
        let written = bytes.len() as i32;
        // SAFETY: the caller guarantees a writable buffer of `capacity` bytes.
        unsafe {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer as *mut u8, bytes.len());
            *(buffer as *mut u8).add(bytes.len()) = 0;
        }
        written
    })) {
        Ok(code) => code,
        Err(_) => -2,
    }
}

/// Tooling/session boundary: wait for queued windows (never during a physics tick).
#[no_mangle]
pub extern "C" fn ofp_trace_flush() { let _ = catch_unwind(AssertUnwindSafe(crate::vertical_diagnostics::flush_writer)); }
/// Host must call before unloading this DLL to join the background writer.
#[no_mangle]
pub extern "C" fn ofp_trace_shutdown() { let _ = catch_unwind(AssertUnwindSafe(crate::vertical_diagnostics::shutdown_writer)); }
