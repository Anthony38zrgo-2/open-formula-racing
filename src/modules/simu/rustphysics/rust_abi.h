/*
    rust_abi.h -- C ABI of the Open Formula Rust physics library.

    This header mirrors `rust/vehicle-physics-engine/src/open_formula_ffi.rs`.
    Every structure is fixed-size, `#[repr(C)]` on the Rust side and must stay in
    exact field order. The library is loaded at run time with LoadLibrary, so the
    destination never links a Rust static library or a C++ runtime across the
    boundary.
*/

#ifndef RUST_ABI_H
#define RUST_ABI_H

#include <cstddef>
#include <cstdint>

#define OFP_ABI_VERSION 7u
#define OFP_MAX_BODY_CONTACTS 32
#define OFP_WHEELS 4
#define OFP_FLOOR_PROBES 2

//! `OfpSnapshot::flags` bits: which optional telemetry blocks the running backend
//! structurally provides (not whether a value is plausible).
#define OFP_FLAG_POWERTRAIN_THERMAL (1u << 0)
#define OFP_FLAG_WHEEL_THERMAL (1u << 1)
#define OFP_FLAG_BRAKE_THERMAL (1u << 2)

struct OfpVec3
{
    double x;
    double y;
    double z;
};

// Static runtime metadata, in Rust wheel order FL, FR, RL, RR.
struct OfpVehicleSpecs
{
    std::int32_t forward_gears;
    std::int32_t has_reverse;
    double final_drive;
    double reverse_ratio;
    double gear_ratios[8];
    double tire_width_m[OFP_WHEELS];
    // ABI v5: active-profile metadata the presentation layer must not duplicate.
    double max_rpm;
    double idle_rpm;
    double fuel_capacity_kg;
    double fuel_density_kg_per_litre;
    double steering_max_angle_rad;
    double tread_optimal_temperature_c[OFP_WHEELS];
    double tread_optimal_window_c[OFP_WHEELS];
    double tread_overheat_temperature_c[OFP_WHEELS];
    double carcass_optimal_temperature_c[OFP_WHEELS];
    double brake_optimal_min_temperature_c;
    double brake_optimal_max_temperature_c;
    double brake_fade_start_temperature_c;
    double brake_critical_temperature_c;
    double water_optimal_min_temperature_c;
    double water_optimal_max_temperature_c;
    double water_derating_temperature_c;
    double water_critical_temperature_c;
    double oil_optimal_min_temperature_c;
    double oil_optimal_max_temperature_c;
    double oil_derating_temperature_c;
    double oil_critical_temperature_c;
    // ABI v6: audio presentation references (appended; v5 offsets preserved).
    double max_torque_nm;
    double front_torque_split;
};

struct OfpControlInput
{
    double throttle;
    double brake;
    double handbrake;
    double clutch;
    double steering;
    std::int32_t gear_request;
    std::int32_t reserved;
};

struct OfpBodyContact
{
    std::int32_t activation;
    std::int32_t surface;
    OfpVec3 point;
    OfpVec3 normal;
    double penetration_m;
    double friction;
    double restitution;
    double damage;
};

struct OfpWheelState
{
    std::int32_t grounded;
    std::int32_t surface;
    std::int32_t ray_hits;
    std::uint32_t ray_hit_mask;
    double spin_rad_s;
    double steer_angle_rad;
    double slip_ratio;
    double slip_angle_rad;
    double longitudinal_force_n;
    double lateral_force_n;
    double aligning_torque_nm;
    double normal_force_n;
    double suspension_travel_m;
    double suspension_velocity_m_s;
    double spring_force_n;
    double damper_force_n;
    double contact_fraction;
    double dynamic_camber_rad;
    double toe_rad;
    double tire_deflection_m;
    //! Lumped tread centre temperature; the three zone fields below are the
    //! authoritative independent measurements.
    double tread_temperature_c;
    double carcass_temperature_c;
    //! Gauge pressure: measured relative to the profile atmospheric pressure.
    double pressure_kpa_gauge;
    double wear_remaining_fraction;
    double wear_grip_scale;
    double brake_torque_nm;
    OfpVec3 contact_point;
    OfpVec3 contact_normal;
    OfpVec3 hub_position;
    double spring_length_m;
    double tire_radius_m;
    double ray_distance[3];
    // ABI v5: independent thermal nodes of this corner.
    double tread_inner_temperature_c;
    double tread_outer_temperature_c;
    double gas_temperature_c;
    double brake_disc_temperature_c;
    double brake_rim_temperature_c;
    double brake_efficiency;
};

// ABI v7: what one corner's vertical solve measured. Measurements only; the host
// must never feed these back as inputs.
struct OfpWheelDiagnostics
{
    std::int32_t ground_detected;
    std::int32_t ground_binding;
    std::int32_t droop_stop_active;
    std::int32_t bump_stop_active;
    std::int32_t trapped_at_stop;
    std::int32_t reserved0;
    //! Current unsprung-coordinate compression (m).
    double compression_m;
    //! Gap to the ground limit along the suspension axis (m, + = airborne,
    //! NaN = no ground plane inside any ray).
    double ground_separation_m;
    //! The same gap projected on the true contact normal (m).
    double ground_separation_normal_m;
    //! Distance the rays still needed beyond their own length (m, + = no ground).
    double ray_shortfall_m;
    //! Compression the ground demanded at the start of the interval (m, NaN = none).
    double ground_demand_m;
    //! Compression the ground demands at the end of the interval (m).
    double ground_demand_predicted_m;
    //! Maximum compression the linkage admits (m).
    double max_compression_m;
    //! Minimum compression the linkage admits (m).
    double min_compression_m;
    //! `max_compression_m - ground_demand_predicted_m` (m, negative = infeasible).
    double headroom_to_bump_m;
    //! Travel from design (m, + = bump).
    double travel_m;
    //! Axial velocity along the body up axis (m/s, + = compressing).
    double axial_velocity_m_s;
    //! Spring force at the wheel (N, + pushes the wheel down).
    double spring_force_n;
    //! Damper force at the wheel (N, + pushes the wheel down).
    double damper_force_n;
    //! Anti-roll force at the wheel (N, signed).
    double antiroll_force_n;
    //! Travel-stop force at the wheel (N, signed).
    double stop_force_n;
    //! Motion ratio r = ds/dq (dimensionless).
    double motion_ratio;
    //! Signed sum of the four elements at the wheel (N).
    double element_force_sum_n;
    //! Accepted ground reaction impulse over the step (N s).
    double contact_impulse_ns;
    //! Accepted droop/extension stop impulse (N s, <= 0).
    double droop_stop_impulse_ns;
    //! Accepted bump/compression stop impulse (N s, >= 0).
    double bump_stop_impulse_ns;
    //! Chassis-side attachment point (world, m).
    OfpVec3 anchor_position;
    //! Attachment-point velocity (world, m/s).
    OfpVec3 anchor_velocity;
    //! Attachment-point acceleration (world, m/s^2).
    OfpVec3 anchor_acceleration;
    //! Attachment acceleration projected on the suspension axis (m/s^2).
    double axial_anchor_acceleration_m_s2;
};

// ABI v7: one axle's simultaneous solve.
struct OfpAxleDiagnostics
{
    std::uint32_t iterations;
    //! Non-zero when every projected residual closed within tolerance.
    std::int32_t converged;
    std::uint32_t substeps;
    std::int32_t reserved0;
    //! Worst projected residual of the last axle solve (m/s).
    double residual_m_s;
    //! Anti-roll stiffness the axle used (N/m).
    double antiroll_stiffness_n_per_m;
    //! Anti-roll force on the axle's first corner (N).
    double antiroll_force_n;
    //! Sum of both corners' ground reactions (N).
    double normal_load_n;
};

// ABI v7: why the last step was rejected. `active != 0` means the published state is
// frozen at the last accepted step and will not advance again.
struct OfpFailureDiagnostics
{
    std::int32_t active;
    //! 1 ground beyond bump limit, 2 axle not converged, 3 body/corner not
    //! converged, 4 non-finite state, 0 none.
    std::int32_t code;
    //! Offending corner 0..3 (FL/FR/RL/RR) or -1 when not per wheel.
    std::int32_t wheel;
    //! 0 front, 1 rear, -1 unknown.
    std::int32_t axle;
    //! Iteration that produced the rejection (0 = unknown).
    std::uint32_t iteration;
    //! Subdivision depth the transaction reached before giving up.
    std::uint32_t subdivisions;
    //! Compression the ground demanded at the end of the interval (m).
    double demand_m;
    //! Maximum compression the linkage admits (m).
    double limit_m;
    //! `demand_m - limit_m` (m, positive = the incompatibility).
    double excess_m;
    //! Rate of change of the ground limit (m/s).
    double limit_velocity_m_s;
    //! Worst projected residual of the failing solve (m/s).
    double residual_m_s;
    //! Both corners of the failing axle.
    std::int32_t axle_wheel[2];
    std::int32_t reserved1;
    std::int32_t reserved2;
};

struct OfpSnapshot
{
    std::uint32_t abi_version;
    std::uint32_t flags;
    std::int32_t physics_hz;
    std::int32_t gear;
    std::int32_t target_gear;
    std::uint32_t backends;
    std::uint32_t rejected_steps;
    double sim_time_s;
    OfpVec3 position;
    OfpVec3 linear_velocity;
    OfpVec3 angular_velocity;
    OfpVec3 linear_acceleration;
    OfpVec3 basis_x;
    OfpVec3 basis_y;
    OfpVec3 basis_z;
    //! Engine speed in revolutions per minute. ABI v5 renamed this field from
    //! `engine_rpm_rad_s`, which described the wrong unit and invited a double
    //! conversion; the host converts to rad/s exactly once for `tCarElt`.
    double engine_rpm;
    double engine_torque_nm;
    double clutch_engagement;
    double fuel_kg;
    double fuel_capacity_kg;
    double throttle;
    double brake;
    double steer_input;
    double steering_angle_rad;
    double aero_front_downforce_n;
    double aero_floor_downforce_n;
    double aero_rear_downforce_n;
    double aero_drag_n;
    //! Coolant (water) temperature in degrees Celsius.
    double water_temperature_celsius;
    //! Oil temperature in degrees Celsius.
    double oil_temperature_celsius;
    OfpWheelState wheels[OFP_WHEELS];
    double floor_clearance[OFP_FLOOR_PROBES];
    std::uint32_t floor_valid_mask;
    double floor_impulse_ns[OFP_FLOOR_PROBES];
    OfpVec3 floor_contact_point[OFP_FLOOR_PROBES];
    OfpVec3 floor_contact_normal[OFP_FLOOR_PROBES];
    std::uint64_t wheel_ray_queries;
    std::uint64_t wheel_ray_hits;
    std::uint64_t floor_ray_queries;
    std::uint64_t floor_ray_hits;
    std::uint32_t body_contact_count;
    double body_contact_impulse_ns[OFP_MAX_BODY_CONTACTS];
    // ABI v6: raw audio-mechanical state (appended; v5 offsets preserved).
    // No acoustic decision is made in the physics integrator; the audio wrapper
    // derives delivered load, derivatives and shift phases from these fields.
    double clutch_torque_nm;
    double tc_cut_ratio_smoothed;
    std::uint32_t is_rev_limited;
    double shift_timer_s;
    double step_dt_s;
    std::uint64_t sequence;
    // ABI v7: vertical-suspension diagnostics (appended; every v6 offset preserved).
    //! Chassis angular acceleration (world, rad/s^2).
    OfpVec3 angular_acceleration;
    //! One record per corner in FL, FR, RL, RR order.
    OfpWheelDiagnostics wheel_diagnostics[OFP_WHEELS];
    //! Front and rear axle solves.
    OfpAxleDiagnostics axle_diagnostics[2];
    //! Why the last step was rejected, and which corner produced it.
    OfpFailureDiagnostics failure;
    //! Records currently buffered by the trace ring (0 when tracing is off).
    std::uint32_t trace_records;
    std::uint32_t reserved_v7;
};

// Backend mask bits reported by the Rust library.
#define OFP_BACKEND_SUSPENSION_SIMPLIFIED (1u << 0)
#define OFP_BACKEND_AERO_SIMPLIFIED (1u << 1)
#define OFP_BACKEND_TIRE_RELAXATION (1u << 2)
#define OFP_BACKEND_SLOW_STATES_SIMPLIFIED (1u << 3)

#ifdef __cplusplus
/* v6 layout locks (verified by `ofp_audio_v6_probe`): the v6 tail is appended,
   so every v5 offset is preserved. Any drift breaks compilation here instead
   of silently reinterpreting memory at load time. */
static_assert(sizeof(OfpVehicleSpecs) == 400, "OfpVehicleSpecs must stay 400 bytes (ABI v6)");
static_assert(sizeof(OfpWheelDiagnostics) == 264, "OfpWheelDiagnostics must stay 264 bytes (ABI v7)");
static_assert(sizeof(OfpAxleDiagnostics) == 48, "OfpAxleDiagnostics must stay 48 bytes (ABI v7)");
static_assert(sizeof(OfpFailureDiagnostics) == 80, "OfpFailureDiagnostics must stay 80 bytes (ABI v7)");
static_assert(sizeof(OfpSnapshot) == 3480, "OfpSnapshot must stay 3480 bytes (ABI v7)");
static_assert(offsetof(OfpSnapshot, clutch_torque_nm) == 2168, "v6 tail must start at 2168");
static_assert(offsetof(OfpSnapshot, sequence) == 2208, "sequence must stay at 2208");
static_assert(offsetof(OfpSnapshot, angular_acceleration) == 2216, "v7 tail must start at 2216");
static_assert(offsetof(OfpSnapshot, wheel_diagnostics) == 2240, "wheel_diagnostics must stay at 2240");
static_assert(offsetof(OfpSnapshot, axle_diagnostics) == 3296, "axle_diagnostics must stay at 3296");
static_assert(offsetof(OfpSnapshot, failure) == 3392, "failure must stay at 3392");
static_assert(offsetof(OfpSnapshot, trace_records) == 3472, "trace_records must stay at 3472");
static_assert(offsetof(OfpWheelDiagnostics, anchor_position) == 184, "anchor_position must stay at 184");
#endif

typedef void* OfpHandle;

#endif // RUST_ABI_H
