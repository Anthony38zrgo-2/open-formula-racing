/***************************************************************************

    file                 : tire_model_configuration.h
    copyright            : (C) 2026 by Open Formula contributors

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TIRE_MODEL_CONFIGURATION_H
#define TIRE_MODEL_CONFIGURATION_H

struct TireForceConfiguration
{
    double lateral_peak_slip_angle_rad;
    double longitudinal_peak_slip_ratio;
    double lateral_slide_grip_ratio;
    double longitudinal_slide_grip_ratio;
    double falloff_sharpness;
    double budget_blend_width;
    double combined_lateral_peak_migration;
    double combined_longitudinal_peak_migration;
    double contact_ramp_seconds;
    double minimum_relaxation_seconds;
    double relaxation_recovery_scale;
    double slip_loss_seconds;
    double slip_recovery_seconds;
    double contact_patch_meters;
    double relaxation_length_meters;
    double pneumatic_trail_meters;
    double camber_thrust_gain;
    double load_sensitivity_exponent;
    double longitudinal_grip_ratio;
    double compound_friction;
    int relaxation_enabled;
    int peak_migration_enabled;
};

struct TireContactConfiguration
{
    double radius_meters;
    double width_meters;
    double wheel_mass_kilograms;
    double static_load_newtons;
    double radial_stiffness_newtons_per_meter;
    double radial_damping_ratio;
    double maximum_deflection_meters;
    double rolling_radius_deflection_factor;
    double contact_patch_load_gain;
    double contact_patch_deflection_gain;
    double partial_contact_stiffness_floor;
};

struct TireThermalConfiguration
{
    double initial_temperature_celsius;
    double ambient_fallback_celsius;
    double track_fallback_celsius;
    double optimal_tread_temperature_celsius;
    double optimal_tread_window_celsius;
    double overheat_tread_temperature_celsius;
    double optimal_carcass_temperature_celsius;
    double tread_zone_heat_capacity_joules_per_kelvin;
    double carcass_heat_capacity_joules_per_kelvin;
    double gas_heat_capacity_joules_per_kelvin;
    double tread_to_carcass_conductance_watts_per_kelvin;
    double carcass_to_gas_conductance_watts_per_kelvin;
    double tread_to_air_conductance_watts_per_kelvin;
    double carcass_to_air_conductance_watts_per_kelvin;
    double road_conductance_watts_per_kelvin;
    double lateral_tread_conductance_watts_per_kelvin;
    double slip_heat_efficiency;
    double carcass_hysteresis_efficiency;
    double volume_deflection_gain;
    double minimum_pressure_gauge_pascals;
    double maximum_pressure_gauge_pascals;
};

struct TireWearConfiguration
{
    double initial_tread_depth_millimeters;
    double minimum_tread_depth_millimeters;
    double tread_wear_per_megajoule_millimeters;
    double cold_tread_temperature_celsius;
    double optimal_tread_temperature_celsius;
    double overheat_tread_temperature_celsius;
    double extreme_tread_temperature_celsius;
    double cold_temperature_wear_multiplier;
    double overheat_temperature_wear_multiplier;
    double extreme_temperature_wear_multiplier;
    double friction_reference_coefficient;
    double friction_sensitivity_exponent;
    double inner_shoulder_wear_multiplier;
    double center_wear_multiplier;
    double outer_shoulder_wear_multiplier;
    double cliff_start_wear_fraction;
    double end_of_life_grip_scale;
    double progressive_grip_loss_fraction;
};

struct TirePressureConfiguration
{
    double reference_hot_gauge_pascals[4];
    double cold_absolute_pascals[4];
    double reference_temperature_kelvin;
    double ambient_absolute_pascals;
    double compliance_sensitivity;
    double response_sensitivity;
    double rolling_resistance_sensitivity;
    int convention_gauge;
    int valid;
};

struct TireModelConfiguration
{
    int enabled;
    int schema_version;
    TireForceConfiguration force[2];
    TireContactConfiguration contact[2];
    TireThermalConfiguration thermal[2];
    TireWearConfiguration wear[2];
    TirePressureConfiguration pressure;
    double wet_grip_scale;
};

void TireModelConfigurationSetDefaults(TireModelConfiguration& configuration);
bool TireModelConfigurationRead(TireModelConfiguration& configuration, void* handle,
    const double static_load_newtons[4], const double radius_meters[4],
    const double width_meters[4], const double wheel_mass_kilograms[4],
    double ambient_pressure_pascals, double ambient_temperature_kelvin);
double TirePressureConvertToAbsolutePascals(double value, const char* unit, bool gauge,
    double ambient_pressure_pascals);

#endif
