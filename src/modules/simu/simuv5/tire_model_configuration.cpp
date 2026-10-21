/***************************************************************************

    file                 : tire_model_configuration.cpp
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

#include <cmath>
#include <cstdio>
#include <cstring>

#include <tgf.h>

#include "tire_model_configuration.h"

static double ReadNumber(void* handle, const char* section, const char* name, double fallback)
{
    return (double)GfParmGetNum(handle, section, name, (char*)NULL, (tdble)fallback);
}

static double ReadTemperatureCelsius(void* handle, const char* section, const char* name,
    double fallback_celsius)
{
    double kelvin = ReadNumber(handle, section, name, fallback_celsius + 273.15);
    return kelvin - 273.15;
}

static const char* ReadText(void* handle, const char* section, const char* name, const char* fallback)
{
    return GfParmGetStr(handle, section, name, (char*)fallback);
}

static bool ReadYes(void* handle, const char* section, const char* name, bool fallback)
{
    const char* value = ReadText(handle, section, name, fallback ? "yes" : "no");
    return value != NULL && (strcmp(value, "yes") == 0 || strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
}

static bool IsCoupledModel(const char* value)
{
    if (value == NULL)
    {
        return false;
    }
    return strcmp(value, "transient coupled") == 0 || strcmp(value, "coupled") == 0
        || strcmp(value, "transient_coupled") == 0 || strcmp(value, "transient-coupled") == 0;
}

static void SetForceDefaults(TireForceConfiguration& force)
{
    force.lateral_peak_slip_angle_rad = 0.115;
    force.longitudinal_peak_slip_ratio = 0.12;
    force.lateral_slide_grip_ratio = 0.88;
    force.longitudinal_slide_grip_ratio = 0.88;
    force.falloff_sharpness = 2.0;
    force.budget_blend_width = 0.10;
    force.combined_lateral_peak_migration = 0.35;
    force.combined_longitudinal_peak_migration = 0.25;
    force.contact_ramp_seconds = 0.06;
    force.minimum_relaxation_seconds = 0.045;
    force.relaxation_recovery_scale = 1.6;
    force.slip_loss_seconds = 0.06;
    force.slip_recovery_seconds = 0.25;
    force.contact_patch_meters = 0.35;
    force.relaxation_length_meters = 0.60;
    force.pneumatic_trail_meters = 0.08;
    force.camber_thrust_gain = 0.18;
    force.load_sensitivity_exponent = 0.92;
    force.longitudinal_grip_ratio = 0.95;
    force.compound_friction = 2.70;
    force.relaxation_enabled = 1;
    force.peak_migration_enabled = 1;
}

static void SetContactDefaults(TireContactConfiguration& contact, int axle)
{
    contact.radius_meters = axle == 0 ? 0.312 : 0.322;
    contact.width_meters = axle == 0 ? 0.308 : 0.389;
    contact.wheel_mass_kilograms = axle == 0 ? 13.0 : 16.0;
    contact.static_load_newtons = axle == 0 ? 1300.0 : 1700.0;
    contact.radial_stiffness_newtons_per_meter = contact.static_load_newtons / 0.008;
    contact.radial_damping_ratio = 0.45;
    contact.maximum_deflection_meters = contact.radius_meters * 0.12;
    contact.rolling_radius_deflection_factor = 0.68;
    contact.contact_patch_load_gain = 0.12;
    contact.contact_patch_deflection_gain = 0.20;
    contact.partial_contact_stiffness_floor = 0.35;
}

static void SetThermalDefaults(TireThermalConfiguration& thermal, int axle)
{
    thermal.initial_temperature_celsius = 70.0;
    thermal.ambient_fallback_celsius = 25.0;
    thermal.track_fallback_celsius = 35.0;
    thermal.optimal_tread_temperature_celsius = 95.0;
    thermal.optimal_tread_window_celsius = 12.0;
    thermal.overheat_tread_temperature_celsius = 120.0;
    thermal.optimal_carcass_temperature_celsius = 80.0;
    thermal.tread_zone_heat_capacity_joules_per_kelvin = axle == 0 ? 2100.0 : 2800.0;
    thermal.carcass_heat_capacity_joules_per_kelvin = axle == 0 ? 5300.0 : 8000.0;
    thermal.gas_heat_capacity_joules_per_kelvin = axle == 0 ? 750.0 : 1200.0;
    thermal.tread_to_carcass_conductance_watts_per_kelvin = axle == 0 ? 75.0 : 64.0;
    thermal.carcass_to_gas_conductance_watts_per_kelvin = axle == 0 ? 50.0 : 36.0;
    thermal.tread_to_air_conductance_watts_per_kelvin = axle == 0 ? 4.5 : 7.0;
    thermal.carcass_to_air_conductance_watts_per_kelvin = axle == 0 ? 3.5 : 4.5;
    thermal.road_conductance_watts_per_kelvin = 60.0;
    thermal.lateral_tread_conductance_watts_per_kelvin = axle == 0 ? 24.0 : 22.0;
    thermal.slip_heat_efficiency = axle == 0 ? 1.1 : 0.9;
    thermal.carcass_hysteresis_efficiency = 1.0;
    thermal.volume_deflection_gain = 0.04;
    thermal.minimum_pressure_gauge_pascals = 55000.0;
    thermal.maximum_pressure_gauge_pascals = 260000.0;
}

static void SetWearDefaults(TireWearConfiguration& wear, int axle)
{
    wear.initial_tread_depth_millimeters = 6.0;
    wear.minimum_tread_depth_millimeters = 0.5;
    wear.tread_wear_per_megajoule_millimeters = axle == 0 ? 0.9 : 1.05;
    wear.cold_tread_temperature_celsius = 60.0;
    wear.optimal_tread_temperature_celsius = 95.0;
    wear.overheat_tread_temperature_celsius = 120.0;
    wear.extreme_tread_temperature_celsius = 160.0;
    wear.cold_temperature_wear_multiplier = 0.45;
    wear.overheat_temperature_wear_multiplier = 1.6;
    wear.extreme_temperature_wear_multiplier = 3.0;
    wear.friction_reference_coefficient = 2.7;
    wear.friction_sensitivity_exponent = 0.6;
    wear.inner_shoulder_wear_multiplier = 1.0;
    wear.center_wear_multiplier = 0.85;
    wear.outer_shoulder_wear_multiplier = 1.0;
    wear.cliff_start_wear_fraction = 0.75;
    wear.end_of_life_grip_scale = 0.55;
    wear.progressive_grip_loss_fraction = 0.05;
}

void TireModelConfigurationSetDefaults(TireModelConfiguration& configuration)
{
    configuration.enabled = 0;
    configuration.schema_version = 1;
    configuration.wet_grip_scale = 0.60;
    for (int axle = 0; axle < 2; ++axle)
    {
        SetForceDefaults(configuration.force[axle]);
        SetContactDefaults(configuration.contact[axle], axle);
        SetThermalDefaults(configuration.thermal[axle], axle);
        SetWearDefaults(configuration.wear[axle], axle);
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        configuration.pressure.cold_absolute_pascals[wheel] = 231300.0;
        configuration.pressure.reference_hot_gauge_pascals[wheel] = wheel < 2 ? 160000.0 : 150000.0;
    }
    configuration.pressure.reference_temperature_kelvin = 298.15;
    configuration.pressure.ambient_absolute_pascals = 101325.0;
    configuration.pressure.compliance_sensitivity = 0.72;
    configuration.pressure.response_sensitivity = -0.18;
    configuration.pressure.rolling_resistance_sensitivity = -0.35;
    configuration.pressure.convention_gauge = 1;
    configuration.pressure.valid = 0;
}

static void ReadForceConfiguration(TireModelConfiguration& configuration, void* handle, int axle)
{
    const char* section = axle == 0 ? "Formula Tires/Front Force" : "Formula Tires/Rear Force";
    TireForceConfiguration& force = configuration.force[axle];
    force.lateral_peak_slip_angle_rad = ReadNumber(handle, section, "lateral peak slip angle", force.lateral_peak_slip_angle_rad);
    force.longitudinal_peak_slip_ratio = ReadNumber(handle, section, "longitudinal peak slip ratio", force.longitudinal_peak_slip_ratio);
    force.lateral_slide_grip_ratio = ReadNumber(handle, section, "lateral slide grip ratio", force.lateral_slide_grip_ratio);
    force.longitudinal_slide_grip_ratio = ReadNumber(handle, section, "longitudinal slide grip ratio", force.longitudinal_slide_grip_ratio);
    force.falloff_sharpness = ReadNumber(handle, section, "falloff sharpness", force.falloff_sharpness);
    force.budget_blend_width = ReadNumber(handle, section, "budget blend width", force.budget_blend_width);
    force.combined_lateral_peak_migration = ReadNumber(handle, section, "combined lateral peak migration", force.combined_lateral_peak_migration);
    force.combined_longitudinal_peak_migration = ReadNumber(handle, section, "combined longitudinal peak migration", force.combined_longitudinal_peak_migration);
    force.contact_ramp_seconds = ReadNumber(handle, section, "contact ramp seconds", force.contact_ramp_seconds);
    force.minimum_relaxation_seconds = ReadNumber(handle, section, "minimum relaxation seconds", force.minimum_relaxation_seconds);
    force.relaxation_recovery_scale = ReadNumber(handle, section, "relaxation recovery scale", force.relaxation_recovery_scale);
    force.slip_loss_seconds = ReadNumber(handle, section, "slip loss seconds", force.slip_loss_seconds);
    force.slip_recovery_seconds = ReadNumber(handle, section, "slip recovery seconds", force.slip_recovery_seconds);
    force.contact_patch_meters = ReadNumber(handle, section, "contact patch", force.contact_patch_meters);
    force.relaxation_length_meters = ReadNumber(handle, section, "relaxation length", force.relaxation_length_meters);
    force.pneumatic_trail_meters = ReadNumber(handle, section, "pneumatic trail", force.pneumatic_trail_meters);
    force.camber_thrust_gain = ReadNumber(handle, section, "camber thrust gain", force.camber_thrust_gain);
    force.load_sensitivity_exponent = ReadNumber(handle, section, "load sensitivity exponent", force.load_sensitivity_exponent);
    force.longitudinal_grip_ratio = ReadNumber(handle, "Formula Tires", "longitudinal grip ratio", force.longitudinal_grip_ratio);
    force.compound_friction = ReadNumber(handle, "Formula Tires", "compound friction", force.compound_friction);
    force.relaxation_enabled = ReadYes(handle, section, "relaxation enabled", force.relaxation_enabled != 0) ? 1 : 0;
    force.peak_migration_enabled = ReadYes(handle, section, "peak migration enabled", force.peak_migration_enabled != 0) ? 1 : 0;
}

static void ReadContactConfiguration(TireModelConfiguration& configuration, void* handle, int axle,
    const double static_load_newtons[4], const double radius_meters[4],
    const double width_meters[4], const double wheel_mass_kilograms[4])
{
    const char* section = axle == 0 ? "Formula Tires/Front Contact" : "Formula Tires/Rear Contact";
    TireContactConfiguration& contact = configuration.contact[axle];
    int wheel = axle * 2;
    contact.static_load_newtons = 0.5 * (static_load_newtons[wheel] + static_load_newtons[wheel + 1]);
    contact.radius_meters = 0.5 * (radius_meters[wheel] + radius_meters[wheel + 1]);
    contact.width_meters = 0.5 * (width_meters[wheel] + width_meters[wheel + 1]);
    contact.wheel_mass_kilograms = 0.5 * (wheel_mass_kilograms[wheel] + wheel_mass_kilograms[wheel + 1]);
    contact.radial_stiffness_newtons_per_meter = ReadNumber(handle, section, "radial stiffness",
        contact.static_load_newtons / 0.008);
    contact.radial_damping_ratio = ReadNumber(handle, section, "radial damping ratio", contact.radial_damping_ratio);
    contact.maximum_deflection_meters = ReadNumber(handle, section, "maximum deflection",
        contact.radius_meters * 0.12);
    contact.rolling_radius_deflection_factor = ReadNumber(handle, section, "rolling radius deflection factor",
        contact.rolling_radius_deflection_factor);
    contact.contact_patch_load_gain = ReadNumber(handle, section, "contact patch load gain",
        contact.contact_patch_load_gain);
    contact.contact_patch_deflection_gain = ReadNumber(handle, section, "contact patch deflection gain",
        contact.contact_patch_deflection_gain);
    contact.partial_contact_stiffness_floor = ReadNumber(handle, section, "partial contact stiffness floor",
        contact.partial_contact_stiffness_floor);
    if (configuration.force[axle].contact_patch_meters <= 0.0)
    {
        configuration.force[axle].contact_patch_meters = contact.width_meters * 0.8;
    }
    if (configuration.force[axle].relaxation_length_meters <= 0.0)
    {
        configuration.force[axle].relaxation_length_meters = configuration.force[axle].contact_patch_meters
            * (axle == 0 ? 1.65 : 1.80);
    }
    if (configuration.force[axle].pneumatic_trail_meters <= 0.0)
    {
        configuration.force[axle].pneumatic_trail_meters = configuration.force[axle].contact_patch_meters
            * (axle == 0 ? 0.22 : 0.24);
    }
}

static void ReadThermalConfiguration(TireModelConfiguration& configuration, void* handle, int axle)
{
    const char* section = axle == 0 ? "Formula Tires/Front Thermal" : "Formula Tires/Rear Thermal";
    TireThermalConfiguration& thermal = configuration.thermal[axle];
    thermal.initial_temperature_celsius = ReadTemperatureCelsius(handle, section, "initial temperature", thermal.initial_temperature_celsius);
    thermal.ambient_fallback_celsius = ReadTemperatureCelsius(handle, section, "ambient fallback", thermal.ambient_fallback_celsius);
    thermal.track_fallback_celsius = ReadTemperatureCelsius(handle, section, "track fallback", thermal.track_fallback_celsius);
    thermal.optimal_tread_temperature_celsius = ReadTemperatureCelsius(handle, section, "optimal tread temperature", thermal.optimal_tread_temperature_celsius);
    thermal.optimal_tread_window_celsius = ReadNumber(handle, section, "optimal tread window", thermal.optimal_tread_window_celsius);
    thermal.overheat_tread_temperature_celsius = ReadTemperatureCelsius(handle, section, "overheat tread temperature", thermal.overheat_tread_temperature_celsius);
    thermal.optimal_carcass_temperature_celsius = ReadTemperatureCelsius(handle, section, "optimal carcass temperature", thermal.optimal_carcass_temperature_celsius);
    thermal.tread_zone_heat_capacity_joules_per_kelvin = ReadNumber(handle, section, "tread zone heat capacity", thermal.tread_zone_heat_capacity_joules_per_kelvin);
    thermal.carcass_heat_capacity_joules_per_kelvin = ReadNumber(handle, section, "carcass heat capacity", thermal.carcass_heat_capacity_joules_per_kelvin);
    thermal.gas_heat_capacity_joules_per_kelvin = ReadNumber(handle, section, "gas heat capacity", thermal.gas_heat_capacity_joules_per_kelvin);
    thermal.tread_to_carcass_conductance_watts_per_kelvin = ReadNumber(handle, section, "tread to carcass conductance", thermal.tread_to_carcass_conductance_watts_per_kelvin);
    thermal.carcass_to_gas_conductance_watts_per_kelvin = ReadNumber(handle, section, "carcass to gas conductance", thermal.carcass_to_gas_conductance_watts_per_kelvin);
    thermal.tread_to_air_conductance_watts_per_kelvin = ReadNumber(handle, section, "tread to air conductance", thermal.tread_to_air_conductance_watts_per_kelvin);
    thermal.carcass_to_air_conductance_watts_per_kelvin = ReadNumber(handle, section, "carcass to air conductance", thermal.carcass_to_air_conductance_watts_per_kelvin);
    thermal.road_conductance_watts_per_kelvin = ReadNumber(handle, section, "road conductance", thermal.road_conductance_watts_per_kelvin);
    thermal.lateral_tread_conductance_watts_per_kelvin = ReadNumber(handle, section, "lateral tread conductance", thermal.lateral_tread_conductance_watts_per_kelvin);
    thermal.slip_heat_efficiency = ReadNumber(handle, section, "slip heat efficiency", thermal.slip_heat_efficiency);
    thermal.carcass_hysteresis_efficiency = ReadNumber(handle, section, "carcass hysteresis efficiency", thermal.carcass_hysteresis_efficiency);
    thermal.volume_deflection_gain = ReadNumber(handle, section, "volume deflection gain", thermal.volume_deflection_gain);
    thermal.minimum_pressure_gauge_pascals = ReadNumber(handle, section, "minimum pressure gauge", thermal.minimum_pressure_gauge_pascals);
    thermal.maximum_pressure_gauge_pascals = ReadNumber(handle, section, "maximum pressure gauge", thermal.maximum_pressure_gauge_pascals);
}

static void ReadWearConfiguration(TireModelConfiguration& configuration, void* handle, int axle)
{
    const char* section = axle == 0 ? "Formula Tires/Front Wear" : "Formula Tires/Rear Wear";
    TireWearConfiguration& wear = configuration.wear[axle];
    wear.initial_tread_depth_millimeters = ReadNumber(handle, section, "initial tread depth", wear.initial_tread_depth_millimeters);
    wear.minimum_tread_depth_millimeters = ReadNumber(handle, section, "minimum tread depth", wear.minimum_tread_depth_millimeters);
    wear.tread_wear_per_megajoule_millimeters = ReadNumber(handle, section, "tread wear per megajoule", wear.tread_wear_per_megajoule_millimeters);
    wear.cold_tread_temperature_celsius = ReadTemperatureCelsius(handle, section, "cold tread temperature", wear.cold_tread_temperature_celsius);
    wear.optimal_tread_temperature_celsius = ReadTemperatureCelsius(handle, section, "optimal tread temperature", wear.optimal_tread_temperature_celsius);
    wear.overheat_tread_temperature_celsius = ReadTemperatureCelsius(handle, section, "overheat tread temperature", wear.overheat_tread_temperature_celsius);
    wear.extreme_tread_temperature_celsius = ReadTemperatureCelsius(handle, section, "extreme tread temperature", wear.extreme_tread_temperature_celsius);
    wear.cold_temperature_wear_multiplier = ReadNumber(handle, section, "cold temperature multiplier", wear.cold_temperature_wear_multiplier);
    wear.overheat_temperature_wear_multiplier = ReadNumber(handle, section, "overheat temperature multiplier", wear.overheat_temperature_wear_multiplier);
    wear.extreme_temperature_wear_multiplier = ReadNumber(handle, section, "extreme temperature multiplier", wear.extreme_temperature_wear_multiplier);
    wear.friction_reference_coefficient = ReadNumber(handle, section, "friction reference coefficient", wear.friction_reference_coefficient);
    wear.friction_sensitivity_exponent = ReadNumber(handle, section, "friction sensitivity exponent", wear.friction_sensitivity_exponent);
    wear.inner_shoulder_wear_multiplier = ReadNumber(handle, section, "inner shoulder wear multiplier", wear.inner_shoulder_wear_multiplier);
    wear.center_wear_multiplier = ReadNumber(handle, section, "center wear multiplier", wear.center_wear_multiplier);
    wear.outer_shoulder_wear_multiplier = ReadNumber(handle, section, "outer shoulder wear multiplier", wear.outer_shoulder_wear_multiplier);
    wear.cliff_start_wear_fraction = ReadNumber(handle, section, "cliff start wear fraction", wear.cliff_start_wear_fraction);
    wear.end_of_life_grip_scale = ReadNumber(handle, section, "end of life grip scale", wear.end_of_life_grip_scale);
    wear.progressive_grip_loss_fraction = ReadNumber(handle, section, "progressive grip loss fraction", wear.progressive_grip_loss_fraction);
}

double TirePressureConvertToAbsolutePascals(double value, const char* unit, bool gauge,
    double ambient_pressure_pascals)
{
    double pascals = value;
    if (unit != NULL)
    {
        if (strcmp(unit, "kPa") == 0 || strcmp(unit, "kpa") == 0)
        {
            pascals = value * 1000.0;
        }
        else if (strcmp(unit, "psi") == 0 || strcmp(unit, "PSI") == 0)
        {
            pascals = value * 6894.757293168;
        }
        else if (strcmp(unit, "bar") == 0)
        {
            pascals = value * 100000.0;
        }
    }
    if (gauge)
    {
        pascals += ambient_pressure_pascals;
    }
    return pascals;
}

static double ReadPressureValue(void* handle, const char* section, const char* name,
    bool gauge, double ambient, double fallback_pascals)
{
    double pascals = (double)GfParmGetNum(handle, section, name, (char*)"Pa", (tdble)fallback_pascals);
    return gauge ? pascals + ambient : pascals;
}

bool TireModelConfigurationRead(TireModelConfiguration& configuration, void* handle,
    const double static_load_newtons[4], const double radius_meters[4],
    const double width_meters[4], const double wheel_mass_kilograms[4],
    double ambient_pressure_pascals, double ambient_temperature_kelvin)
{
    TireModelConfigurationSetDefaults(configuration);
    const char* selector = ReadText(handle, "Formula Controls", "tire model", "transient coupled");
    configuration.enabled = IsCoupledModel(selector) ? 1 : 0;
    if (!configuration.enabled)
    {
        GfLogWarning("Unsupported tire model '%s': simuv5 only supports transient coupled\n", selector);
        return false;
    }

    configuration.schema_version = (int)ReadNumber(handle, "Formula Tires", "schema version", 1.0);
    if (configuration.schema_version != 1)
    {
        GfLogWarning("Unsupported coupled tire configuration schema %d\n", configuration.schema_version);
        configuration.enabled = 0;
        return false;
    }

    configuration.wet_grip_scale = ReadNumber(handle, "Formula Tires", "wet grip scale", configuration.wet_grip_scale);
    for (int axle = 0; axle < 2; ++axle)
    {
        ReadForceConfiguration(configuration, handle, axle);
        ReadContactConfiguration(configuration, handle, axle, static_load_newtons, radius_meters,
            width_meters, wheel_mass_kilograms);
        ReadThermalConfiguration(configuration, handle, axle);
        ReadWearConfiguration(configuration, handle, axle);
    }

    const char* convention = ReadText(handle, "Formula Tires", "pressure convention", NULL);
    const char* unit = ReadText(handle, "Formula Tires", "pressure unit", "Pa");
    bool gauge = convention != NULL && strcmp(convention, "gauge") == 0;
    bool absolute = convention != NULL && strcmp(convention, "absolute") == 0;
    configuration.pressure.convention_gauge = gauge ? 1 : 0;
    configuration.pressure.ambient_absolute_pascals = ReadNumber(handle, "Formula Tires", "ambient pressure",
        ambient_pressure_pascals > 0.0 ? ambient_pressure_pascals : 101325.0);
    configuration.pressure.reference_temperature_kelvin = ReadNumber(handle, "Formula Tires",
        "reference temperature", ambient_temperature_kelvin > 150.0 ? ambient_temperature_kelvin : 298.15);
    configuration.pressure.compliance_sensitivity = ReadNumber(handle, "Formula Tires",
        "compliance sensitivity", configuration.pressure.compliance_sensitivity);
    configuration.pressure.response_sensitivity = ReadNumber(handle, "Formula Tires",
        "response sensitivity", configuration.pressure.response_sensitivity);
    configuration.pressure.rolling_resistance_sensitivity = ReadNumber(handle, "Formula Tires",
        "rolling resistance sensitivity", configuration.pressure.rolling_resistance_sensitivity);

    static const char* wheel_names[4] = {
        "cold pressure front right", "cold pressure front left",
        "cold pressure rear right", "cold pressure rear left"
    };
    static const char* reference_names[4] = {
        "reference hot pressure front right", "reference hot pressure front left",
        "reference hot pressure rear right", "reference hot pressure rear left"
    };
    static const double default_cold_gauge[4] = { 130000.0, 130000.0, 130000.0, 130000.0 };
    static const double default_hot_gauge[4] = { 160000.0, 160000.0, 150000.0, 150000.0 };

    configuration.pressure.valid = (gauge || absolute) ? 1 : 0;
    bool explicit_cold_pressures = true;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        if (!GfParmExistsParam(handle, "Formula Tires", wheel_names[wheel]))
        {
            explicit_cold_pressures = false;
        }
    }
    if (!explicit_cold_pressures)
    {
        configuration.pressure.valid = 0;
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        configuration.pressure.cold_absolute_pascals[wheel] = ReadPressureValue(handle, "Formula Tires",
            wheel_names[wheel], gauge, configuration.pressure.ambient_absolute_pascals,
            default_cold_gauge[wheel]);
        configuration.pressure.reference_hot_gauge_pascals[wheel] = ReadPressureValue(handle, "Formula Tires",
            reference_names[wheel], true, configuration.pressure.ambient_absolute_pascals,
            default_hot_gauge[wheel]) - configuration.pressure.ambient_absolute_pascals;
        if (!(configuration.pressure.cold_absolute_pascals[wheel] > 0.0)
            || !(configuration.pressure.reference_hot_gauge_pascals[wheel] > 20000.0))
        {
            configuration.pressure.valid = 0;
        }
    }
    if (configuration.pressure.reference_temperature_kelvin <= 150.0)
    {
        configuration.pressure.valid = 0;
    }
    if (!configuration.pressure.valid)
    {
        GfLogWarning("Coupled tire model requires an explicit pressure convention, unit and valid cold/reference pressures\n");
        configuration.enabled = 0;
        return false;
    }
    double cold_gauge = configuration.pressure.cold_absolute_pascals[0]
        - configuration.pressure.ambient_absolute_pascals;
    if (cold_gauge > 400000.0)
    {
        GfLogWarning("Coupled tire cold pressure %.3f Pa gauge is outside the calibrated range; treating it as robustness input\n",
            cold_gauge);
    }
    GfLogInfo("Coupled tire model enabled: schema=%d cold_abs=%.3f Pa reference_hot_gauge=%.3f Pa pressure_unit=%s\n",
        configuration.schema_version, configuration.pressure.cold_absolute_pascals[0],
        configuration.pressure.reference_hot_gauge_pascals[0], unit != NULL ? unit : "Pa");
    return true;
}
