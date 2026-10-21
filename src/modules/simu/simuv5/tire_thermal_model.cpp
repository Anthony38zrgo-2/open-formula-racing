/***************************************************************************

    file                 : tire_thermal_model.cpp
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

#include "tire_thermal_model.h"

static const double KelvinOffset = 273.15;

static double ClampValue(double value, double lower, double upper)
{
    if (value < lower)
    {
        return lower;
    }
    if (value > upper)
    {
        return upper;
    }
    return value;
}

static double FiniteTemperature(double value)
{
    if (!(value == value) || value > 1.0e6 || value < -1.0e6)
    {
        return 25.0 + KelvinOffset;
    }
    return ClampValue(value, -40.0 + KelvinOffset, 220.0 + KelvinOffset);
}

static double LinearInterpolate(double lower, double upper, double fraction)
{
    return lower + (upper - lower) * ClampValue(fraction, 0.0, 1.0);
}

double TireThermalGripScale(double tread_temperature_kelvin,
    const TireThermalConfiguration& configuration)
{
    double temperature = tread_temperature_kelvin - KelvinOffset;
    double optimal = configuration.optimal_tread_temperature_celsius;
    double window = fmax(configuration.optimal_tread_window_celsius, 2.0);
    double low_optimal = optimal - window * 0.5;
    double high_optimal = optimal + window * 0.5;
    double overheat = fmax(configuration.overheat_tread_temperature_celsius, high_optimal + 2.0);
    if (temperature <= 20.0)
    {
        return 0.68;
    }
    if (temperature < low_optimal)
    {
        return LinearInterpolate(0.68, 1.0, (temperature - 20.0) / fmax(low_optimal - 20.0, 1.0));
    }
    if (temperature <= high_optimal)
    {
        return 1.0;
    }
    if (temperature < overheat)
    {
        return LinearInterpolate(1.0, 0.94, (temperature - high_optimal) / fmax(overheat - high_optimal, 1.0));
    }
    return LinearInterpolate(0.94, 0.78, (temperature - overheat) / 35.0);
}

double TirePressureFromGas(double cold_absolute_pascals, double reference_temperature_kelvin,
    double gas_temperature_kelvin, double deflection_ratio, double volume_deflection_gain,
    double ambient_absolute_pascals)
{
    double cold_absolute = fmax(cold_absolute_pascals, 1.0);
    double reference = fmax(reference_temperature_kelvin, 150.0);
    double gas = fmax(gas_temperature_kelvin, 150.0);
    double volume_ratio = ClampValue(1.0 - ClampValue(volume_deflection_gain, 0.0, 0.10)
        * ClampValue(deflection_ratio, 0.0, 1.5), 0.92, 1.02);
    double absolute = cold_absolute * gas / reference / volume_ratio;
    return fmax(absolute, ambient_absolute_pascals + 1000.0);
}

void TireThermalZoneWeights(const double raw_weights[3], double pressure_ratio,
    double camber_radians, double result[3])
{
    result[0] = fmax(raw_weights[0], 0.0);
    result[1] = fmax(raw_weights[1], 0.0);
    result[2] = fmax(raw_weights[2], 0.0);
    double original_sum = result[0] + result[1] + result[2];
    if (original_sum <= 1.0e-9)
    {
        result[0] = 0.0;
        result[1] = 0.0;
        result[2] = 0.0;
        return;
    }
    double delta = ClampValue(pressure_ratio - 1.0, -0.55, 0.55);
    if (delta >= 0.0)
    {
        result[1] *= 1.0 + 0.85 * delta;
        result[0] *= 1.0 - 0.22 * delta;
        result[2] *= 1.0 - 0.22 * delta;
    }
    else
    {
        double low = -delta;
        result[0] *= 1.0 + 0.45 * low;
        result[2] *= 1.0 + 0.45 * low;
        result[1] *= 1.0 - 0.28 * low;
    }
    double camber_bias = ClampValue(-camber_radians / 0.080, -0.35, 0.35);
    result[0] *= 1.0 + camber_bias;
    result[2] *= 1.0 - camber_bias;
    double sum = result[0] + result[1] + result[2];
    if (sum <= 1.0e-9)
    {
        result[0] = 0.0;
        result[1] = 0.0;
        result[2] = 0.0;
        return;
    }
    result[0] /= sum;
    result[1] /= sum;
    result[2] /= sum;
}

void TireThermalModelReset(TireThermalState& state, const TireThermalConfiguration& configuration,
    const TirePressureConfiguration& pressure, int wheel_index)
{
    double initial_kelvin = configuration.initial_temperature_celsius + KelvinOffset;
    state.tread_inner_kelvin = initial_kelvin;
    state.tread_center_kelvin = initial_kelvin;
    state.tread_outer_kelvin = initial_kelvin;
    state.carcass_kelvin = initial_kelvin;
    state.gas_kelvin = initial_kelvin;
    state.pressure_absolute_pascals = TirePressureFromGas(pressure.cold_absolute_pascals[wheel_index],
        pressure.reference_temperature_kelvin, initial_kelvin, 0.0,
        configuration.volume_deflection_gain, pressure.ambient_absolute_pascals);
    state.last_slip_power_watts = 0.0;
    state.last_external_power_watts = 0.0;
    state.last_balance_residual_joules = 0.0;
    state.accumulated_deposited_joules = 0.0;
    state.accumulated_external_joules = 0.0;
    state.pressure_clamp_count = 0;
    state.temperature_clamp_count = 0;
    state.update_count = 0;
}

void TireThermalModelMechanicalModifiers(const TireThermalState& state,
    const TireThermalConfiguration& configuration, const TirePressureConfiguration& pressure,
    int wheel_index, TireMechanicalModifiers& modifiers)
{
    double gauge_pressure = state.pressure_absolute_pascals - pressure.ambient_absolute_pascals;
    double reference_gauge = fmax(pressure.reference_hot_gauge_pascals[wheel_index], 20000.0);
    double pressure_ratio = ClampValue(gauge_pressure / reference_gauge, 0.45, 1.80);
    double carcass_delta = state.carcass_kelvin - KelvinOffset
        - configuration.optimal_carcass_temperature_celsius;
    double carcass_structure = ClampValue(1.0 - 0.0015 * carcass_delta, 0.88, 1.12);
    double average_tread = (state.tread_inner_kelvin + state.tread_center_kelvin
        + state.tread_outer_kelvin) / 3.0;
    double thermal_grip = TireThermalGripScale(average_tread, configuration);
    double compliance = ClampValue(pow(pressure_ratio, ClampValue(pressure.compliance_sensitivity, -3.0, 3.0)),
        0.55, 1.80);
    modifiers.vertical_stiffness_scale = ClampValue(compliance * carcass_structure, 0.55, 1.65);
    modifiers.vertical_damping_scale = ClampValue(sqrt(compliance) * (2.0 - carcass_structure), 0.65, 1.55);
    modifiers.maximum_deflection_scale = ClampValue(1.0 / compliance, 0.65, 1.55);
    modifiers.contact_patch_scale = 1.0;
    modifiers.relaxation_length_scale = ClampValue(pow(pressure_ratio,
        ClampValue(pressure.response_sensitivity, -3.0, 3.0)), 0.80, 1.30);
    modifiers.rolling_resistance_scale = ClampValue(pow(pressure_ratio,
        ClampValue(pressure.rolling_resistance_sensitivity, -3.0, 3.0)), 0.75, 1.50);
    modifiers.force_stiffness_scale = ClampValue(0.92 + 0.08 * thermal_grip, 0.75, 1.25);
    modifiers.grip_scale = ClampValue(thermal_grip, 0.60, 1.03);
    modifiers.pneumatic_trail_scale = 1.0;
    modifiers.wear_grip_scale = 1.0;
}

static void ThermalSubstep(TireThermalState& state, const TireThermalConfiguration& configuration,
    const TirePressureConfiguration& pressure, int wheel_index, const TireThermalInput& input,
    double step_seconds, double& deposited_joules, double& external_joules)
{
    double tread_capacity = fmax(configuration.tread_zone_heat_capacity_joules_per_kelvin, 100.0);
    double carcass_capacity = fmax(configuration.carcass_heat_capacity_joules_per_kelvin, 500.0);
    double gas_capacity = fmax(configuration.gas_heat_capacity_joules_per_kelvin, 100.0);
    double tread_to_carcass = fmax(configuration.tread_to_carcass_conductance_watts_per_kelvin, 0.0);
    double carcass_to_gas = fmax(configuration.carcass_to_gas_conductance_watts_per_kelvin, 0.0);
    double tread_to_air = fmax(configuration.tread_to_air_conductance_watts_per_kelvin, 0.0);
    double carcass_to_air = fmax(configuration.carcass_to_air_conductance_watts_per_kelvin, 0.0);
    double road_conductance = fmax(configuration.road_conductance_watts_per_kelvin, 0.0);
    double lateral_conductance = fmax(configuration.lateral_tread_conductance_watts_per_kelvin, 0.0);
    double airflow = ClampValue(1.0 + fabs(input.vehicle_speed_meters_per_second) * 0.020, 1.0, 4.0);

    double gauge_pressure = state.pressure_absolute_pascals - pressure.ambient_absolute_pascals;
    double reference_gauge = fmax(pressure.reference_hot_gauge_pascals[wheel_index], 20000.0);
    double pressure_ratio = ClampValue(gauge_pressure / reference_gauge, 0.45, 1.80);
    double zone_weights[3];
    double contact_fraction = ClampValue(input.contact_fraction, 0.0, 1.0);
    TireThermalZoneWeights(input.zone_contact_weights, pressure_ratio, input.camber_radians, zone_weights);
    if (contact_fraction <= 1.0e-9)
    {
        zone_weights[0] = 0.0;
        zone_weights[1] = 0.0;
        zone_weights[2] = 0.0;
    }

    double slip_power = fmax(input.slip_power_watts, 0.0);
    double slip_heat_power = slip_power * ClampValue(configuration.slip_heat_efficiency, 0.0, 1.5);
    double old_tread[3] = {
        state.tread_inner_kelvin, state.tread_center_kelvin, state.tread_outer_kelvin
    };
    double old_carcass = state.carcass_kelvin;
    double old_gas = state.gas_kelvin;
    double tread_heat_power[3];
    double tread_to_carcass_power[3];
    double external_power = 0.0;
    double deposited_power = 0.0;

    for (int zone = 0; zone < 3; ++zone)
    {
        double supported = zone_weights[zone];
        double slip_power_zone = slip_heat_power * supported;
        double road_power = road_conductance * supported
            * (input.track_temperature_kelvin - old_tread[zone]);
        double air_power = tread_to_air * airflow
            * (input.ambient_temperature_kelvin - old_tread[zone]);
        double carcass_power = tread_to_carcass * (old_carcass - old_tread[zone]);
        double lateral_power = 0.0;
        if (zone == 0)
        {
            lateral_power = lateral_conductance * (old_tread[1] - old_tread[0]);
        }
        else if (zone == 1)
        {
            lateral_power = lateral_conductance * ((old_tread[0] - old_tread[1])
                + (old_tread[2] - old_tread[1]));
        }
        else
        {
            lateral_power = lateral_conductance * (old_tread[1] - old_tread[2]);
        }
        tread_to_carcass_power[zone] = carcass_power;
        tread_heat_power[zone] = slip_power_zone + road_power + air_power + carcass_power + lateral_power;
        deposited_power += slip_power_zone;
        external_power += road_power + air_power;
    }

    double carcass_from_tread = -(tread_to_carcass_power[0] + tread_to_carcass_power[1]
        + tread_to_carcass_power[2]);
    double flex_power = contact_fraction > 0.0
        ? fmax(input.normal_force_newtons, 0.0) * fabs(input.deflection_velocity_meters_per_second)
            * ClampValue(configuration.carcass_hysteresis_efficiency, 0.0, 1.0)
        : 0.0;
    double carcass_to_gas_power = carcass_to_gas * (old_carcass - old_gas);
    double carcass_air_power = carcass_to_air * sqrt(airflow)
        * (input.ambient_temperature_kelvin - old_carcass);
    double carcass_heat_power = carcass_from_tread + flex_power + input.brake_carcass_heat_watts
        - carcass_to_gas_power + carcass_air_power;
    double gas_heat_power = carcass_to_gas_power + input.brake_gas_heat_watts;

    double delta_tread[3];
    for (int zone = 0; zone < 3; ++zone)
    {
        delta_tread[zone] = tread_heat_power[zone] * step_seconds / tread_capacity;
    }
    double delta_carcass = carcass_heat_power * step_seconds / carcass_capacity;
    double delta_gas = gas_heat_power * step_seconds / gas_capacity;

    double total_capacity_delta = tread_capacity * (delta_tread[0] + delta_tread[1] + delta_tread[2])
        + carcass_capacity * delta_carcass + gas_capacity * delta_gas;
    double external_energy = (external_power + deposited_power + flex_power + carcass_air_power + input.brake_carcass_heat_watts
        + input.brake_gas_heat_watts) * step_seconds;
    state.last_balance_residual_joules += total_capacity_delta - external_energy;
    deposited_joules += slip_heat_power * step_seconds;
    external_joules += external_energy;

    state.tread_inner_kelvin = FiniteTemperature(state.tread_inner_kelvin + delta_tread[0]);
    state.tread_center_kelvin = FiniteTemperature(state.tread_center_kelvin + delta_tread[1]);
    state.tread_outer_kelvin = FiniteTemperature(state.tread_outer_kelvin + delta_tread[2]);
    state.carcass_kelvin = FiniteTemperature(state.carcass_kelvin + delta_carcass);
    state.gas_kelvin = FiniteTemperature(state.gas_kelvin + delta_gas);
}

void TireThermalModelUpdate(TireThermalState& state, const TireThermalConfiguration& configuration,
    const TirePressureConfiguration& pressure, int wheel_index, const TireThermalInput& input,
    double elapsed_seconds)
{
    if (!(elapsed_seconds > 0.0))
    {
        return;
    }
    double limited_seconds = ClampValue(elapsed_seconds, 0.0, 0.05);
    int step_count = (int)ceil(limited_seconds / 0.005);
    if (step_count < 1)
    {
        step_count = 1;
    }
    if (step_count > 32)
    {
        step_count = 32;
    }
    double step_seconds = limited_seconds / (double)step_count;
    double deposited_joules = 0.0;
    double external_joules = 0.0;
    state.last_balance_residual_joules = 0.0;
    for (int step = 0; step < step_count; ++step)
    {
        ThermalSubstep(state, configuration, pressure, wheel_index, input, step_seconds,
            deposited_joules, external_joules);
    }

    double deflection_ratio = input.maximum_deflection_meters > 1.0e-5
        ? ClampValue(input.deflection_meters / input.maximum_deflection_meters, 0.0, 1.5) : 0.0;
    double absolute_pressure = TirePressureFromGas(pressure.cold_absolute_pascals[wheel_index],
        pressure.reference_temperature_kelvin, state.gas_kelvin, deflection_ratio,
        configuration.volume_deflection_gain, pressure.ambient_absolute_pascals);
    double gauge_pressure = absolute_pressure - pressure.ambient_absolute_pascals;
    double clamped_gauge = ClampValue(gauge_pressure,
        fmax(configuration.minimum_pressure_gauge_pascals, 0.0),
        fmax(configuration.maximum_pressure_gauge_pascals,
            configuration.minimum_pressure_gauge_pascals + 1000.0));
    if (clamped_gauge != gauge_pressure)
    {
        state.pressure_clamp_count += 1;
    }
    state.pressure_absolute_pascals = clamped_gauge + pressure.ambient_absolute_pascals;
    state.last_slip_power_watts = fmax(input.slip_power_watts, 0.0);
    state.last_external_power_watts = external_joules / limited_seconds;
    state.accumulated_deposited_joules += deposited_joules;
    state.accumulated_external_joules += external_joules;
    state.update_count += 1;
}
