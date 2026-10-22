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

#include <cstdint>

#define OFP_ABI_VERSION 4u
#define OFP_MAX_BODY_CONTACTS 32
#define OFP_WHEELS 4
#define OFP_FLOOR_PROBES 2

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
    double tread_temperature_c;
    double carcass_temperature_c;
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
    double engine_rpm_rad_s;
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
};

// Backend mask bits reported by the Rust library.
#define OFP_BACKEND_SUSPENSION_SIMPLIFIED (1u << 0)
#define OFP_BACKEND_AERO_SIMPLIFIED (1u << 1)
#define OFP_BACKEND_TIRE_RELAXATION (1u << 2)
#define OFP_BACKEND_SLOW_STATES_SIMPLIFIED (1u << 3)

typedef void* OfpHandle;

#endif // RUST_ABI_H
