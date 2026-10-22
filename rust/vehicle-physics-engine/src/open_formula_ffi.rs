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

pub const OFP_ABI_VERSION: u32 = 4;
pub const OFP_MAX_CONTACTS: usize = 32;
pub const OFP_WHEELS: usize = 4;
pub const OFP_FLOOR_PROBES: usize = 2;

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
    pub tread_temperature_c: f64,
    pub carcass_temperature_c: f64,
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
}

impl OfpWheelState {
    pub fn zeroed() -> Self {
        Self::default()
    }
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
    pub engine_rpm_rad_s: f64,
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
            engine_rpm_rad_s: 0.0,
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
    mix_layout(&mut hash, std::mem::size_of::<OfpSnapshot>() as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpWheelState>() as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpBodyContact>() as u64);
    mix_layout(&mut hash, std::mem::size_of::<OfpControlInput>() as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, wheels) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, floor_clearance) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpSnapshot, body_contact_impulse_ns) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpWheelState, contact_point) as u64);
    mix_layout(&mut hash, std::mem::offset_of!(OfpWheelState, ray_distance) as u64);
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
