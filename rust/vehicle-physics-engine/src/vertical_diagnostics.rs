//! Vertical-suspension telemetry: what each corner is doing, which constraint is
//! holding it and why a step was rejected.
//!
//! Every value here is a *measurement of the accepted or rejected solve*, never a
//! re-derivation from presentation state. The module has three layers:
//!
//! 1. `WheelVerticalDiagnostics` / `AxleVerticalDiagnostics` — published once per
//!    accepted step through the C ABI (host summary log).
//! 2. `FailureDiagnostics` — the concrete numbers of the constraint that rejected a
//!    step, including the corner and axle that produced it.
//! 3. `TraceRecord` ring — every body/corner iteration, axle substep, accepted tick
//!    and rejected candidate, so a transient loss of contact or a rejection can be
//!    reconstructed instead of guessed.
//!
//! Nothing in this module feeds forces. It may be disabled at zero cost: the hot
//! path reads one relaxed atomic before touching the ring.

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Mutex, OnceLock};

use crate::types::Vec3;

/// Trace capacity. Sized for the documented window (3 s before, 1 s after) at the
/// bounded memory budget. Dumps explicitly report when history was overwritten.
pub const TRACE_CAPACITY: usize = 20_480;

/// Seconds of history kept before an event.
pub const TRACE_PRE_SECONDS: f64 = 3.0;
/// Seconds recorded after an event before the window is written out.
pub const TRACE_POST_SECONDS: f64 = 1.0;
/// A wheel must stay airborne this long before the loss counts as an event.
pub const TRACE_CONTACT_LOSS_SECONDS: f64 = 0.25;

// ---------------------------------------------------------------------------
// Per-corner and per-axle diagnostics (published in the ABI snapshot)
// ---------------------------------------------------------------------------

/// Why a corner is or is not touching the ground, and what its element is doing.
#[derive(Debug, Clone, Copy, Default)]
pub struct WheelVerticalDiagnostics {
    /// At least one transverse ray reported ground for this corner.
    pub ground_detected: bool,
    /// The vertical solve applied a ground reaction this step.
    pub ground_binding: bool,
    /// The corner rested on its droop (extension) stop.
    pub droop_stop_active: bool,
    /// The corner rested on its bump (compression) stop.
    pub bump_stop_active: bool,
    /// True when the wheel centre was still travelling down at the bump stop.
    pub trapped_at_stop: bool,
    /// Current compression of the unsprung coordinate (m).
    pub compression_m: f64,
    /// Gap between the wheel centre and the ground limit along the suspension axis
    /// (m, positive = airborne, negative = the ray plane is above the wheel).
    pub ground_separation_m: f64,
    /// The same gap projected on the true contact normal (m).
    pub ground_separation_normal_m: f64,
    /// Distance the ray still had to travel beyond its length to find ground (m,
    /// positive = no ground within the ray, negative = ground was found).
    pub ray_shortfall_m: f64,
    /// Compression the ground would demand at the start of the interval (m).
    pub ground_demand_m: f64,
    /// Compression the ground would demand at the end of the interval (m).
    pub ground_demand_predicted_m: f64,
    /// Hard maximum compression admitted by the linkage (m).
    pub max_compression_m: f64,
    /// Hard minimum compression admitted by the linkage (m).
    pub min_compression_m: f64,
    /// `max_compression_m - ground_demand_predicted_m` (m, negative = infeasible).
    pub headroom_to_bump_m: f64,
    /// Travel from design (`compression - rest_compression`), m. Positive = bump.
    pub travel_m: f64,
    /// Axial velocity along the body up axis (m/s, positive = compressing).
    pub axial_velocity_m_s: f64,
    /// Spring force at the wheel (N, positive pushes the wheel down).
    pub spring_force_n: f64,
    /// Damper force at the wheel (N, positive pushes the wheel down).
    pub damper_force_n: f64,
    /// Anti-roll bar force at the wheel (N, signed; positive pushes it down).
    pub antiroll_force_n: f64,
    /// Travel-stop force at the wheel (N, signed).
    pub stop_force_n: f64,
    /// Motion ratio r = ds/dq at the current travel (dimensionless).
    pub motion_ratio: f64,
    /// Signed sum of the four elements at the wheel (N). Negative means the corner
    /// is being lifted by the elements (positive pushes down).
    pub element_force_sum_n: f64,
    pub normal_force_n: f64,
    pub contact_point: Vec3,
    pub contact_normal: Vec3,
    pub ray_hit_mask: u32,
    /// Moving hub-frame acceleration used by the solve, distinct from fixed mount.
    pub solve_frame_acceleration: Vec3,
    /// Accepted ground reaction impulse over the step (N s).
    pub contact_impulse_ns: f64,
    /// Duration over which this corner impulse was accumulated (may differ from host tick).
    pub contact_dt_s: f64,
    /// Accepted droop/extension stop impulse (N s, sign of the stop reaction).
    pub droop_stop_impulse_ns: f64,
    /// Accepted bump/compression stop impulse (N s).
    pub bump_stop_impulse_ns: f64,
    /// Anchor position in world space (m).
    pub anchor_position: Vec3,
    /// Anchor velocity in world space (m/s).
    pub anchor_velocity: Vec3,
    /// Anchor acceleration in world space (m/s^2).
    pub anchor_acceleration: Vec3,
    /// Anchor acceleration projected on the suspension axis (m/s^2). This is what
    /// the corner feels: gravity plus chassis motion.
    pub axial_anchor_acceleration_m_s2: f64,
}

/// One axle's simultaneous solve.
#[derive(Debug, Clone, Copy, Default)]
pub struct AxleVerticalDiagnostics {
    /// Fixed-point iterations consumed by the last axle substep.
    pub iterations: u32,
    /// All projected residuals closed within tolerance.
    pub converged: bool,
    /// Substeps the axle integrated in the last step.
    pub substeps: u32,
    /// Worst projected residual of the last axle solve (m/s).
    pub residual_m_s: f64,
    /// Anti-roll stiffness the axle used (N/m).
    pub antiroll_stiffness_n_per_m: f64,
    /// Anti-roll force acting on the first corner of the axle pair (N).
    pub antiroll_force_n: f64,
    /// Sum of the two corners' ground reactions (N).
    pub normal_load_n: f64,
}

/// Stable identity of the rejection reason, so the host never has to parse text.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum FailureCode {
    #[default]
    None = 0,
    /// The ground demanded more compression than the bump stop allows.
    GroundBeyondBumpLimit = 1,
    /// The axle fixed point could not satisfy contact and travel together.
    AxleNotConverged = 2,
    /// The body/corner transaction did not close its accelerations.
    BodyCornerNotConverged = 3,
    /// The step produced a non-finite state.
    NonFiniteState = 4,
}

impl FailureCode {
    pub fn as_i32(self) -> i32 {
        self as i32
    }

    pub fn label(self) -> &'static str {
        match self {
            FailureCode::None => "none",
            FailureCode::GroundBeyondBumpLimit => "ground_beyond_bump_limit",
            FailureCode::AxleNotConverged => "axle_not_converged",
            FailureCode::BodyCornerNotConverged => "body_corner_not_converged",
            FailureCode::NonFiniteState => "non_finite_state",
        }
    }
}

/// Exact numbers of the constraint combination that rejected the step.
#[derive(Debug, Clone, Copy, Default)]
pub struct FailureDiagnostics {
    pub code: FailureCode,
    /// Offending corner (0..3, FL/FR/RL/RR) or -1 when the failure is not per wheel.
    pub wheel: i32,
    /// 0 = front, 1 = rear, -1 = unknown.
    pub axle: i32,
    /// Body/corner iteration that produced the rejection (1-based, 0 = unknown).
    pub iteration: u32,
    /// Subdivision depth the transaction reached before giving up.
    pub subdivisions: u32,
    /// Compression the ground demanded at the end of the interval (m).
    pub demand_m: f64,
    /// Maximum compression the linkage admits (m).
    pub limit_m: f64,
    /// `demand_m - limit_m` (m, positive = the incompatibility).
    pub excess_m: f64,
    /// Rate of change of the ground limit (m/s) that produced the demand.
    pub limit_velocity_m_s: f64,
    /// Worst projected residual of the failing solve (m/s).
    pub residual_m_s: f64,
    /// Both corners of the failing axle, so the opposite wheel is never lost.
    pub axle_wheels: [i32; 2],
}

// ---------------------------------------------------------------------------
// Trace ring
// ---------------------------------------------------------------------------

pub mod kind {
    /// One accepted host tick (the last accepted state before any later event).
    pub const TICK: u8 = 1;
    /// One body/corner transaction iteration.
    pub const ITERATION: u8 = 2;
    /// One axle substep after its fixed point closed.
    pub const SUBSTEP: u8 = 3;
    /// The candidate state that was rejected. A later `TICK` is the last accepted.
    pub const REJECTED: u8 = 4;
    /// The state restored after a rejection (identical to the last accepted tick).
    pub const RESTORED: u8 = 5;
}

pub mod wheel_flag {
    pub const GROUNDED: u16 = 1 << 0;
    pub const BINDING: u16 = 1 << 1;
    pub const DROOP_STOP: u16 = 1 << 2;
    pub const BUMP_STOP: u16 = 1 << 3;
    pub const TRAPPED: u16 = 1 << 4;
}

/// Compact per-corner row of a trace record. SI units throughout.
#[derive(Debug, Clone, Copy, Default)]
pub struct TraceWheel {
    pub compression_m: f64,
    pub axial_velocity_m_s: f64,
    pub separation_m: f64,
    pub ground_demand_m: f64,
    pub max_compression_m: f64,
    pub min_compression_m: f64,
    pub spring_force_n: f64,
    pub damper_force_n: f64,
    pub antiroll_force_n: f64,
    pub stop_force_n: f64,
    pub normal_force_n: f64,
    pub element_force_sum_n: f64,
    pub contact_point: Vec3,
    pub contact_normal: Vec3,
    pub ray_hit_mask: u32,
    pub solve_frame_acceleration: Vec3,
    pub ground_demand_predicted_m: f64,
    pub anchor_position: Vec3,
    pub anchor_velocity: Vec3,
    pub anchor_acceleration: Vec3,
    pub contact_impulse_ns: f64,
    /// Duration over which this corner impulse was accumulated (may differ from host tick).
    pub contact_dt_s: f64,
    pub bump_stop_impulse_ns: f64,
    pub droop_stop_impulse_ns: f64,
    pub flags: u16,
}

/// One row of the ring. Fixed size so the buffer never reallocates while tracing.
#[derive(Debug, Clone, Copy, Default)]
pub struct TraceRecord {
    pub kind: u8,
    pub axle: i8,
    pub wheel: i8,
    pub flags: u32,
    pub iteration: u16,
    pub subdivisions: u16,
    pub sim_time_s: f64,
    pub step_dt_s: f64,
    pub sequence: u64,
    /// Body/corner residual of this iteration (m/s) when the record is an iteration.
    pub residual: f64,
    pub linear_acceleration: [f64; 3],
    pub angular_acceleration: [f64; 3],
    pub wheels: [TraceWheel; 4],
}

impl TraceRecord {
    pub fn is_empty(&self) -> bool {
        self.kind == 0
    }

    /// CSV header matching `to_csv_fields`. Kept next to the writer so the two can
    /// never drift apart.
    pub fn csv_header() -> String {
        let mut header = String::from(
            "kind,axle,wheel,flags,iteration,subdivisions,sim_time_s,step_dt_s,sequence,residual_m_s,\
             linear_accel_x,linear_accel_y,linear_accel_z,angular_accel_x,angular_accel_y,angular_accel_z",
        );
        for wheel in ["FL", "FR", "RL", "RR"] {
            for field in [
                "compression_m",
                "axial_velocity_m_s",
                "separation_m",
                "ground_demand_m",
                "max_compression_m",
                "min_compression_m",
                "spring_force_n",
                "damper_force_n",
                "antiroll_force_n",
                "stop_force_n",
                "normal_force_n",
                "contact_impulse_ns",
                "bump_stop_impulse_ns",
                "droop_stop_impulse_ns",
                "flags", "element_force_sum_n", "contact_x", "contact_y", "contact_z",
                "normal_x", "normal_y", "normal_z", "ray_hit_mask",
                "solve_frame_accel_x", "solve_frame_accel_y", "solve_frame_accel_z",
                "contact_dt_s", "ground_demand_predicted_m", "anchor_x", "anchor_y", "anchor_z",
                "anchor_vx", "anchor_vy", "anchor_vz", "anchor_ax", "anchor_ay", "anchor_az",
            ] {
                header.push(',');
                header.push_str(wheel);
                header.push('_');
                header.push_str(field);
            }
        }
        header
    }

    pub fn to_csv_line(&self) -> String {
        use std::fmt::Write as _;
        let mut line = String::with_capacity(560);
        let _ = write!(
            line,
            "{},{},{},{},{},{},{:.6},{:.6},{},{:.6e},{:.6e},{:.6e},{:.6e},{:.6e},{:.6e},{:.6e}",
            self.kind,
            self.axle,
            self.wheel,
            self.flags,
            self.iteration,
            self.subdivisions,
            self.sim_time_s,
            self.step_dt_s,
            self.sequence,
            self.residual,
            self.linear_acceleration[0],
            self.linear_acceleration[1],
            self.linear_acceleration[2],
            self.angular_acceleration[0],
            self.angular_acceleration[1],
            self.angular_acceleration[2],
        );
        for wheel in &self.wheels {
            let _ = write!(
                line,
                ",{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.3},{:.3},{:.3},{:.3},{:.3},{:.6},{:.6},{:.6},{}",
                wheel.compression_m,
                wheel.axial_velocity_m_s,
                wheel.separation_m,
                wheel.ground_demand_m,
                wheel.max_compression_m,
                wheel.min_compression_m,
                wheel.spring_force_n,
                wheel.damper_force_n,
                wheel.antiroll_force_n,
                wheel.stop_force_n,
                wheel.normal_force_n,
                wheel.contact_impulse_ns,
                wheel.bump_stop_impulse_ns,
                wheel.droop_stop_impulse_ns,
                wheel.flags,
            );
            let _ = write!(line, ",{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{},{:.6},{:.6},{:.6}",
                wheel.element_force_sum_n, wheel.contact_point.x, wheel.contact_point.y, wheel.contact_point.z,
                wheel.contact_normal.x, wheel.contact_normal.y, wheel.contact_normal.z, wheel.ray_hit_mask,
                wheel.solve_frame_acceleration.x, wheel.solve_frame_acceleration.y, wheel.solve_frame_acceleration.z);
            let _ = write!(line, ",{:.9},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6}",
                wheel.contact_dt_s, wheel.ground_demand_predicted_m,
                wheel.anchor_position.x, wheel.anchor_position.y, wheel.anchor_position.z,
                wheel.anchor_velocity.x, wheel.anchor_velocity.y, wheel.anchor_velocity.z,
                wheel.anchor_acceleration.x, wheel.anchor_acceleration.y, wheel.anchor_acceleration.z);
        }
        line
    }
}

/// Why a window was captured. Serialized into the dump metadata.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TriggerReason {
    ContactLoss,
    TravelStop,
    Rejection,
}

impl TriggerReason {
    pub fn label(self) -> &'static str {
        match self {
            TriggerReason::ContactLoss => "contact_loss",
            TriggerReason::TravelStop => "travel_stop",
            TriggerReason::Rejection => "rejection",
        }
    }
}

struct PendingWindow {
    reason: TriggerReason,
    trigger_time_s: f64,
    write_at_s: f64,
    wheel: i8,
}

#[derive(Default)]
struct TraceContext {
    sim_time_s: f64,
    step_dt_s: f64,
    sequence: u64,
}

struct TraceStore {
    records: Vec<TraceRecord>,
    head: usize,
    len: usize,
    directory: Option<String>,
    context: TraceContext,
    pending: Option<PendingWindow>,
    /// Sim time each corner lost its ground contact (NaN = currently grounded).
    contact_loss_since: [f64; 4],
    last_failure: FailureDiagnostics,
    loss_latched: [bool; 4],
    stop_latched: [bool; 4],
    supported_since: [f64; 4],
    overwritten_records: u64,
}

impl TraceStore {
    fn new() -> Self {
        Self {
            records: vec![TraceRecord::default(); TRACE_CAPACITY],
            head: 0,
            len: 0,
            directory: None,
            context: TraceContext::default(),
            pending: None,
            contact_loss_since: [f64::NAN; 4],
            last_failure: FailureDiagnostics::default(),
            loss_latched: [false; 4], stop_latched: [false; 4], supported_since: [f64::NAN; 4],
            overwritten_records: 0,
        }
    }

    fn clear(&mut self) {
        self.head = 0;
        self.len = 0;
        self.overwritten_records = 0;
        self.pending = None;
        self.loss_latched = [false; 4]; self.stop_latched = [false; 4]; self.supported_since = [f64::NAN; 4];
        self.contact_loss_since = [f64::NAN; 4];
        self.last_failure = FailureDiagnostics::default();
    }

    fn push(&mut self, record: TraceRecord) {
        if self.len == TRACE_CAPACITY { self.overwritten_records += 1; }
        self.records[self.head] = record;
        self.head = (self.head + 1) % TRACE_CAPACITY;
        if self.len < TRACE_CAPACITY {
            self.len += 1;
        }
    }

    fn observe(&mut self, sim_time_s: f64, wheels: [&WheelVerticalDiagnostics; 4]) {
    let mut firing: Option<(TriggerReason, i8)> = None;
    for (index, wheel) in wheels.iter().enumerate() {
        if has_support(wheel) {
            self.contact_loss_since[index] = f64::NAN;
            if self.supported_since[index].is_nan() { self.supported_since[index] = sim_time_s; }
            if sim_time_s - self.supported_since[index] >= 0.25 { self.loss_latched[index] = false; }
        } else {
            self.supported_since[index] = f64::NAN;
            let since = self.contact_loss_since[index];
            if since.is_nan() {
                self.contact_loss_since[index] = sim_time_s;
            } else if sim_time_s - since >= TRACE_CONTACT_LOSS_SECONDS && !self.loss_latched[index] && firing.is_none() && self.pending.is_none() {
                self.loss_latched[index] = true;
                firing = Some((TriggerReason::ContactLoss, index as i8));
            }
        }
        let stop_hit = wheel.bump_stop_impulse_ns > 0.0
            || wheel.droop_stop_impulse_ns.abs() > 0.0
            || wheel.bump_stop_active
            || wheel.droop_stop_active;
        if stop_hit && !self.stop_latched[index] && firing.is_none() && self.pending.is_none() {
            firing = Some((TriggerReason::TravelStop, index as i8));
        }
        self.stop_latched[index] = stop_hit;
    }
    if self.pending.is_some() {
        return;
    }
    if let Some((reason, wheel)) = firing {
        self.pending = Some(PendingWindow {
            reason,
            trigger_time_s: sim_time_s,
            write_at_s: sim_time_s + TRACE_POST_SECONDS,
            wheel,
        });
    }
    }

    fn oldest_index(&self) -> usize {
        if self.len < TRACE_CAPACITY {
            0
        } else {
            self.head
        }
    }
}

fn store() -> &'static Mutex<TraceStore> {
    static STORE: OnceLock<Mutex<TraceStore>> = OnceLock::new();
    STORE.get_or_init(|| Mutex::new(TraceStore::new()))
}

static TRACE_ENABLED: AtomicBool = AtomicBool::new(false);

/// Cheap gate read on every record. `Relaxed` is enough: the flag only decides
/// whether diagnostics are collected, never the physics result.
#[inline]
fn enabled() -> bool {
    TRACE_ENABLED.load(Ordering::Relaxed)
}

/// Enables the ring and points it at `directory`.
///
/// The directory is created on demand; when it is not writable the trace still
/// accumulates in memory and `trace_dump` can be called explicitly.
pub fn configure(directory: Option<&str>) {
    flush_writer();
    if !start_writer() { eprintln!("RUSTTRACE_WRITE_ERROR reason=cannot_start_writer"); }
    if let Ok(mut guard) = store().lock() {
        guard.directory = directory.map(|value| value.to_string());
        guard.clear();
    }
    TRACE_ENABLED.store(true, Ordering::Relaxed);
}

pub fn disable() {
    TRACE_ENABLED.store(false, Ordering::Relaxed);
}

pub fn is_enabled() -> bool {
    TRACE_ENABLED.load(Ordering::Relaxed)
}

/// Records the clock of the step about to run. Every subsequent record of that step
/// reuses it, so a record never mixes times.
pub fn begin_step(sim_time_s: f64, step_dt_s: f64, sequence: u64) {
    if !enabled() {
        return;
    }
    if let Ok(mut guard) = store().lock() {
        guard.context = TraceContext { sim_time_s, step_dt_s, sequence };
    }
}

/// Endpoint time and actual duration of the interval being solved. Sequence stays host-owned.
pub fn interval(endpoint_s: f64, dt_s: f64) {
    if !enabled() { return; }
    if let Ok(mut guard) = store().lock() {
        guard.context.sim_time_s = endpoint_s;
        guard.context.step_dt_s = dt_s;
    }
}

/// Number of records currently buffered (diagnostic of the diagnostics).
pub fn buffered() -> usize {
    if !enabled() {
        return 0;
    }
    store().lock().map(|guard| guard.len).unwrap_or(0)
}

/// Appends one record built from the live diagnostics.
pub fn record(kind: u8, axle: i8, wheel: i8, iteration: u16, subdivisions: u16,
              residual: f64, linear_acceleration: Vec3, angular_acceleration: Vec3,
              wheels: [&WheelVerticalDiagnostics; 4]) {
    if !enabled() {
        return;
    }
    let Ok(mut guard) = store().lock() else { return };
    let context = TraceContext {
        sim_time_s: guard.context.sim_time_s,
        step_dt_s: guard.context.step_dt_s,
        sequence: guard.context.sequence,
    };
    let mut record = TraceRecord {
        kind,
        axle,
        wheel,
        flags: 0,
        iteration,
        subdivisions,
        sim_time_s: context.sim_time_s,
        step_dt_s: context.step_dt_s,
        sequence: context.sequence,
        residual,
        linear_acceleration: [linear_acceleration.x, linear_acceleration.y, linear_acceleration.z],
        angular_acceleration: [angular_acceleration.x, angular_acceleration.y, angular_acceleration.z],
        wheels: [TraceWheel::default(); 4],
    };
    for (index, source) in wheels.iter().enumerate() {
        record.wheels[index] = TraceWheel {
            compression_m: source.compression_m,
            axial_velocity_m_s: source.axial_velocity_m_s,
            separation_m: source.ground_separation_m,
            ground_demand_m: source.ground_demand_m,
            max_compression_m: source.max_compression_m,
            min_compression_m: source.min_compression_m,
            spring_force_n: source.spring_force_n,
            damper_force_n: source.damper_force_n,
            antiroll_force_n: source.antiroll_force_n,
            stop_force_n: source.stop_force_n,
            normal_force_n: source.normal_force_n,
            element_force_sum_n: source.element_force_sum_n,
            contact_point: source.contact_point, contact_normal: source.contact_normal,
            ray_hit_mask: source.ray_hit_mask, solve_frame_acceleration: source.solve_frame_acceleration,
            ground_demand_predicted_m: source.ground_demand_predicted_m,
            anchor_position: source.anchor_position, anchor_velocity: source.anchor_velocity,
            anchor_acceleration: source.anchor_acceleration,
            contact_impulse_ns: source.contact_impulse_ns,
            contact_dt_s: source.contact_dt_s,
            bump_stop_impulse_ns: source.bump_stop_impulse_ns,
            droop_stop_impulse_ns: source.droop_stop_impulse_ns,
            flags: wheel_flags(source),
        };
    }
    guard.push(record);
}

fn wheel_flags(source: &WheelVerticalDiagnostics) -> u16 {
    let mut flags = 0;
    if source.ground_detected {
        flags |= wheel_flag::GROUNDED;
    }
    if source.ground_binding {
        flags |= wheel_flag::BINDING;
    }
    if source.droop_stop_active {
        flags |= wheel_flag::DROOP_STOP;
    }
    if source.bump_stop_active {
        flags |= wheel_flag::BUMP_STOP;
    }
    if source.trapped_at_stop {
        flags |= wheel_flag::TRAPPED;
    }
    flags
}

/// Stores the exact rejection numbers and appends the rejected candidate row.
pub fn record_failure(failure: FailureDiagnostics, iteration: u16, subdivisions: u16,
                      linear_acceleration: Vec3, angular_acceleration: Vec3,
                      wheels: [&WheelVerticalDiagnostics; 4]) {
    if !enabled() {
        return;
    }
    record(kind::REJECTED, failure.axle as i8, failure.wheel as i8, iteration,
           subdivisions, failure.residual_m_s, linear_acceleration, angular_acceleration, wheels);
    if let Ok(mut guard) = store().lock() {
        guard.last_failure = failure;
    }
}

fn has_support(wheel: &WheelVerticalDiagnostics) -> bool {
    wheel.ground_binding && wheel.normal_force_n > 1e-6
}

/// Runs the event detector for one accepted step. It never changes the simulation.
pub fn observe_accepted(sim_time_s: f64, wheels: [&WheelVerticalDiagnostics; 4]) {
    if !enabled() {
        return;
    }
    let Ok(mut guard) = store().lock() else { return };
    guard.observe(sim_time_s, wheels);
}


/// Queue a completed event window; no formatting or filesystem operations on this thread.
pub fn service_window(sim_time_s: f64) {
    if !enabled() { return; }
    let pending = {
        let Ok(mut guard) = store().lock() else { return };
        if guard.pending.as_ref().is_some_and(|p| sim_time_s >= p.write_at_s) {
            guard.pending.take()
        } else { None }
    };
    if let Some(p) = pending {
        let failure = store().lock().map(|g| g.last_failure).unwrap_or_default();
        let _ = queue_window(p.reason, p.trigger_time_s, p.wheel, failure);
    }
}

struct CapturedTrace {
    records: Vec<TraceRecord>,
    oldest: Option<f64>,
    overwritten: u64,
}

fn capture(from: f64) -> Option<CapturedTrace> {
    let guard = store().lock().ok()?;
    if guard.len == 0 { return None; }
    let oldest = Some(guard.records[guard.oldest_index()].sim_time_s);
    let mut records = Vec::with_capacity(guard.len);
    for offset in 0..guard.len {
        let record = guard.records[(guard.oldest_index()+offset)%TRACE_CAPACITY];
        if !record.is_empty() && record.sim_time_s >= from { records.push(record); }
    }
    Some(CapturedTrace { records, oldest, overwritten: guard.overwritten_records })
}

fn write_csv(path: &std::path::Path, captured: &CapturedTrace) -> std::io::Result<()> {
    use std::io::Write;
    let mut file = std::io::BufWriter::with_capacity(256*1024, std::fs::File::create(path)?);
    writeln!(file, "{}", TraceRecord::csv_header())?;
    for record in &captured.records { writeln!(file, "{}", record.to_csv_line())?; }
    file.flush()
}

/// Explicit offline/export API is synchronous, but formatting and IO never hold the ring mutex.
pub fn trace_dump(path: &str) -> Option<usize> { trace_dump_window(path, f64::NEG_INFINITY) }
pub fn trace_dump_window(path: &str, from: f64) -> Option<usize> {
    let captured = capture(from)?;
    write_csv(std::path::Path::new(path), &captured).ok()?;
    Some(captured.records.len())
}

struct WriteJob {
    captured: CapturedTrace,
    directory: String,
    reason: TriggerReason,
    trigger: f64,
    wheel: i8,
    failure: FailureDiagnostics,
    capture_ms: f64,
}
enum WriterCommand {
    Window(WriteJob), Flush(std::sync::mpsc::Sender<()>), Stop,
    #[cfg(test)] Pause(std::sync::mpsc::Receiver<()>),
}
struct Writer {
    sender: std::sync::mpsc::SyncSender<WriterCommand>,
    thread: std::thread::JoinHandle<()>,
}
fn writer() -> &'static Mutex<Option<Writer>> {
    static WRITER: OnceLock<Mutex<Option<Writer>>> = OnceLock::new();
    WRITER.get_or_init(|| Mutex::new(None))
}
static JOBS_IN_FLIGHT: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);
static DROPPED_WINDOWS: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);

fn start_writer() -> bool {
    let Ok(mut slot) = writer().lock() else { return false };
    if slot.is_some() { return true; }
    let (sender, receiver) = std::sync::mpsc::sync_channel(2);
    let thread = std::thread::Builder::new().name("ofp-telemetry-writer".into()).spawn(move || {
        while let Ok(command) = receiver.recv() {
            match command {
                WriterCommand::Window(job) => {
                    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| write_job(job)));
                    JOBS_IN_FLIGHT.fetch_sub(1, Ordering::Relaxed);
                    if !matches!(result, Ok(Ok(()))) { eprintln!("RUSTTRACE_WRITE_ERROR writer=ofp-telemetry-writer"); }
                }
                WriterCommand::Flush(done) => { let _ = done.send(()); }
                WriterCommand::Stop => break,
                #[cfg(test)] WriterCommand::Pause(resume) => { let _ = resume.recv(); }
            }
        }
    });
    match thread {
        Ok(thread) => { *slot = Some(Writer { sender, thread }); true }
        Err(_) => false,
    }
}

fn write_job(job: WriteJob) -> std::io::Result<()> {
    let started = std::time::Instant::now();
    std::fs::create_dir_all(&job.directory)?;
    let name = format!("rustphysics_{}_w{}_t{:08.3}", job.reason.label(), job.wheel.max(0), job.trigger);
    let csv = std::path::Path::new(&job.directory).join(format!("{name}.csv"));
    write_csv(&csv, &job.captured)?;
    let f = job.failure;
    let from = job.trigger - TRACE_PRE_SECONDS;
    let metadata = serde_json::json!({
        "reason": job.reason.label(), "trigger_time_s": job.trigger, "sim_time_s": job.trigger,
        "wheel": job.wheel, "window_pre_s": TRACE_PRE_SECONDS,
        "window_post_s": if job.reason == TriggerReason::Rejection { 0.0 } else { TRACE_POST_SECONDS },
        "window_from_s": from, "rows": job.captured.records.len(), "csv": csv.to_string_lossy().replace('\\', "/"),
        "oldest_retained_time_s": job.captured.oldest, "overwritten_records": job.captured.overwritten,
        "history_truncated": job.captured.overwritten > 0 && job.captured.oldest.is_some_and(|t| t > from),
        "failure_code": f.code.as_i32(), "failure_label": f.code.label(), "failure_axle": f.axle,
        "failure_wheel": f.wheel, "failure_iteration": f.iteration, "failure_subdivisions": f.subdivisions,
        "failure_demand_m": f.demand_m, "failure_limit_m": f.limit_m, "failure_excess_m": f.excess_m,
        "failure_limit_velocity_m_s": f.limit_velocity_m_s, "failure_residual_m_s": f.residual_m_s,
        "writer_thread": std::thread::current().name(), "capture_ms": job.capture_ms,
        "format_write_ms": started.elapsed().as_secs_f64()*1000.0,
        "dropped_windows": DROPPED_WINDOWS.load(Ordering::Relaxed),
        "state_note": "kind=4 rejected candidate; kind=5 restored accepted state"
    });
    std::fs::write(csv.with_extension("json"), serde_json::to_vec_pretty(&metadata)?)?;
    eprintln!("RUSTTRACE reason={} trigger_t={:.3} wheel={} rows={} writer=background capture_ms={:.3} write_ms={:.3} csv={}",
        job.reason.label(), job.trigger, job.wheel, job.captured.records.len(), job.capture_ms,
        started.elapsed().as_secs_f64()*1000.0, csv.display());
    Ok(())
}

fn queue_window(reason: TriggerReason, trigger: f64, wheel: i8, failure: FailureDiagnostics) -> Option<usize> {
    let started = std::time::Instant::now();
    // Reserve before copying: at most one active job plus one queued (~bounded memory).
    if JOBS_IN_FLIGHT.fetch_update(Ordering::Relaxed, Ordering::Relaxed, |n| (n < 2).then_some(n+1)).is_err() {
        DROPPED_WINDOWS.fetch_add(1, Ordering::Relaxed);
        eprintln!("RUSTTRACE_DROPPED reason=writer_busy trigger_t={trigger:.3}");
        return None;
    }
    let job = (|| {
        let directory = store().lock().ok()?.directory.clone()?;
        let captured = capture(trigger - TRACE_PRE_SECONDS)?;
        Some(WriteJob { captured, directory, reason, trigger, wheel, failure, capture_ms: started.elapsed().as_secs_f64()*1000.0 })
    })();
    let Some(job) = job else { JOBS_IN_FLIGHT.fetch_sub(1, Ordering::Relaxed); return None };
    let rows = job.captured.records.len();
    let sent = writer().lock().ok().and_then(|slot| slot.as_ref().map(|w| w.sender.try_send(WriterCommand::Window(job)).is_ok())).unwrap_or(false);
    if sent { Some(rows) } else {
        JOBS_IN_FLIGHT.fetch_sub(1, Ordering::Relaxed);
        DROPPED_WINDOWS.fetch_add(1, Ordering::Relaxed);
        eprintln!("RUSTTRACE_DROPPED reason=writer_unavailable trigger_t={trigger:.3}");
        None
    }
}

pub fn dump_rejection(failure: FailureDiagnostics, sim_time_s: f64) -> Option<usize> {
    queue_window(TriggerReason::Rejection, sim_time_s, failure.wheel as i8, failure)
}

/// Explicit tooling/session boundary only. Never called on the simulation hot path.
pub fn flush_writer() {
    let sender = writer().lock().ok().and_then(|slot| slot.as_ref().map(|w| w.sender.clone()));
    if let Some(sender) = sender {
        let (done, received) = std::sync::mpsc::channel();
        if sender.send(WriterCommand::Flush(done)).is_ok() { let _ = received.recv(); }
    }
}

/// Drain and join before FreeLibrary; no worker may execute code in an unloaded DLL.
pub fn shutdown_writer() {
    let slot = writer().lock().ok().and_then(|mut slot| slot.take());
    if let Some(writer) = slot { let _ = writer.sender.send(WriterCommand::Stop); let _ = writer.thread.join(); }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn csv_header_matches_row_field_count() {
        let header = TraceRecord::csv_header();
        let line = TraceRecord::default().to_csv_line();
        assert_eq!(header.split(',').count(), line.split(',').count());
    }

    #[test]
    fn ring_keeps_the_newest_capacity() {
        let mut store = TraceStore::new();
        for index in 0..(TRACE_CAPACITY + 5) {
            let mut record = TraceRecord::default();
            record.kind = kind::TICK;
            record.sequence = index as u64;
            store.push(record);
        }
        assert_eq!(store.len, TRACE_CAPACITY);
        let oldest = store.oldest_index();
        assert_eq!(store.records[oldest].sequence, 5);
        let newest = (store.head + TRACE_CAPACITY - 1) % TRACE_CAPACITY;
        assert_eq!(store.records[newest].sequence, (TRACE_CAPACITY + 4) as u64);
    }

    #[test]
    fn failure_labels_are_stable() {
        assert_eq!(FailureCode::None.label(), "none");
        assert_eq!(FailureCode::GroundBeyondBumpLimit.as_i32(), 1);
        assert_eq!(FailureCode::BodyCornerNotConverged.as_i32(), 3);
    }
}

#[cfg(test)]
mod regression_tests {
    use super::*;
    #[test]
    fn seeing_ground_does_not_mean_support() {
        let mut wheel = WheelVerticalDiagnostics { ground_detected: true, ..Default::default() };
        assert!(!has_support(&wheel));
        wheel.ground_binding = true;
        wheel.normal_force_n = 1200.0;
        assert!(has_support(&wheel));
    }
    #[test]
    fn loss_of_support_triggers_even_when_all_rays_see_ground() {
        let mut ring = TraceStore::new();
        let unloaded = WheelVerticalDiagnostics { ground_detected: true, ..Default::default() };
        ring.observe(2.0, [&unloaded; 4]);
        assert!(ring.pending.is_none());
        ring.observe(2.3, [&unloaded; 4]);
        let pending = ring.pending.as_ref().unwrap();
        assert_eq!(pending.reason, TriggerReason::ContactLoss);
        assert_eq!(pending.wheel, 0);
        assert_eq!(pending.write_at_s, 3.3);
    }
    #[test]
    fn overflow_is_counted() {
        let mut ring = TraceStore::new();
        for _ in 0..TRACE_CAPACITY + 17 { ring.push(TraceRecord::default()); }
        assert_eq!(ring.overwritten_records, 17);
        ring.clear();
        assert_eq!(ring.overwritten_records, 0);
    }
}

#[cfg(test)]
mod async_writer_tests {
    use super::*;
    #[test]
    fn bounded_writer_never_waits_for_io_and_owns_immutable_snapshot() {
        let dir = std::env::temp_dir().join(format!("ofp-writer-{}", std::process::id()));
        configure(Some(dir.to_str().unwrap()));
        let (resume, paused) = std::sync::mpsc::channel();
        writer().lock().unwrap().as_ref().unwrap().sender.send(WriterCommand::Pause(paused)).unwrap();
        let mut wheel = WheelVerticalDiagnostics { compression_m: 0.123, ..Default::default() };
        begin_step(5.0, 0.002, 1);
        record(kind::TICK, -1, -1, 0, 0, 0.0, Vec3::ZERO, Vec3::ZERO, [&wheel;4]);
        assert_eq!(queue_window(TriggerReason::ContactLoss, 5.0, 0, FailureDiagnostics::default()), Some(1));
        assert_eq!(queue_window(TriggerReason::ContactLoss, 6.0, 0, FailureDiagnostics::default()), Some(1));
        assert!(queue_window(TriggerReason::ContactLoss, 7.0, 0, FailureDiagnostics::default()).is_none());
        // Writer remains paused: recording must still proceed; no worker owns the ring lock.
        wheel.compression_m = 0.456;
        record(kind::TICK, -1, -1, 0, 0, 0.0, Vec3::ZERO, Vec3::ZERO, [&wheel;4]);
        resume.send(()).unwrap();
        flush_writer();
        let csv = std::fs::read_to_string(dir.join("rustphysics_contact_loss_w0_t0005.000.csv")).unwrap();
        assert!(csv.contains("0.123000"));
        assert!(!csv.contains("0.456000"));
        let metadata: serde_json::Value = serde_json::from_str(&std::fs::read_to_string(dir.join("rustphysics_contact_loss_w0_t0005.000.json")).unwrap()).unwrap();
        assert_eq!(metadata["writer_thread"], "ofp-telemetry-writer");
        assert!(metadata["capture_ms"].is_number());
        shutdown_writer();
        assert_eq!(JOBS_IN_FLIGHT.load(Ordering::Relaxed), 0);
        assert!(writer().lock().unwrap().is_none());
    }
    #[test]
    fn sustained_loss_does_not_retrigger_until_stable_support() {
        let mut ring = TraceStore::new();
        let air = WheelVerticalDiagnostics { ground_detected: true, ..Default::default() };
        let ground = WheelVerticalDiagnostics { ground_detected: true, ground_binding: true, normal_force_n: 1000., ..Default::default() };
        ring.observe(0., [&air;4]); ring.observe(0.3, [&air;4]);
        assert!(ring.pending.take().is_some());
        // All four corners get one event each, then the continuing episode stays latched.
        for t in [2.,4.,6.] { ring.observe(t, [&air;4]); ring.pending.take(); }
        ring.observe(8., [&air;4]); assert!(ring.pending.is_none());
        ring.observe(9., [&ground;4]); ring.observe(9.3, [&ground;4]);
        ring.observe(10., [&air;4]); ring.observe(10.3, [&air;4]);
        assert!(ring.pending.is_some());
    }
}
