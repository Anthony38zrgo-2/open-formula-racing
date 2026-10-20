/***************************************************************************

    file                 : tire_thermal_model.h
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

#ifndef TIRE_THERMAL_MODEL_H
#define TIRE_THERMAL_MODEL_H

#include "tire_model_configuration.h"
#include "tire_model_state.h"

struct TireMechanicalModifiers
{
    double vertical_stiffness_scale;
    double vertical_damping_scale;
    double maximum_deflection_scale;
    double contact_patch_scale;
    double relaxation_length_scale;
    double rolling_resistance_scale;
    double force_stiffness_scale;
    double grip_scale;
    double pneumatic_trail_scale;
    double wear_grip_scale;
};

struct TireThermalInput
{
    double normal_force_newtons;
    double longitudinal_force_newtons;
    double lateral_force_newtons;
    double slip_lateral_force_newtons;
    double slip_power_watts;
    double slip_velocity_longitudinal_meters_per_second;
    double slip_velocity_lateral_meters_per_second;
    double deflection_meters;
    double deflection_velocity_meters_per_second;
    double maximum_deflection_meters;
    double vehicle_speed_meters_per_second;
    double ambient_temperature_kelvin;
    double track_temperature_kelvin;
    double camber_radians;
    double contact_fraction;
    double brake_carcass_heat_watts;
    double brake_gas_heat_watts;
    double zone_contact_weights[3];
};

void TireThermalModelReset(TireThermalState& state, const TireThermalConfiguration& configuration,
    const TirePressureConfiguration& pressure, int wheel_index);
void TireThermalModelMechanicalModifiers(const TireThermalState& state,
    const TireThermalConfiguration& configuration, const TirePressureConfiguration& pressure,
    int wheel_index, TireMechanicalModifiers& modifiers);
void TireThermalModelUpdate(TireThermalState& state, const TireThermalConfiguration& configuration,
    const TirePressureConfiguration& pressure, int wheel_index, const TireThermalInput& input,
    double elapsed_seconds);
double TireThermalGripScale(double tread_temperature_kelvin,
    const TireThermalConfiguration& configuration);
double TirePressureFromGas(double cold_absolute_pascals, double reference_temperature_kelvin,
    double gas_temperature_kelvin, double deflection_ratio, double volume_deflection_gain,
    double ambient_absolute_pascals);
void TireThermalZoneWeights(const double raw_weights[3], double pressure_ratio,
    double camber_radians, double result[3]);

#endif
