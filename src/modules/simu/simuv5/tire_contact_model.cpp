/***************************************************************************

    file                 : tire_contact_model.cpp
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

#include "tire_contact_model.h"

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

void TireContactModelReset(TireContactState& state, const TireContactConfiguration& configuration,
    const TireMechanicalModifiers& modifiers)
{
    state.deflection = 0.0;
    state.deflection_velocity = 0.0;
    state.effective_radius = configuration.radius_meters;
    state.contact_patch = configuration.width_meters * 0.8;
    state.load_sensitivity_scale = 1.0;
    state.vertical_stiffness = configuration.radial_stiffness_newtons_per_meter
        * modifiers.vertical_stiffness_scale;
    state.vertical_damping = 2.0 * ClampValue(configuration.radial_damping_ratio, 0.05, 1.5)
        * sqrt(state.vertical_stiffness * fmax(configuration.wheel_mass_kilograms, 1.0))
        * modifiers.vertical_damping_scale;
    state.maximum_deflection = configuration.maximum_deflection_meters
        * modifiers.maximum_deflection_scale;
    state.contact_fraction = 0.0;
    state.clamp_count = 0;
}

void TireContactModelEvaluate(TireContactState& state, const TireContactConfiguration& configuration,
    const TireMechanicalModifiers& modifiers, const TireContactInput& input, TireContactOutput& output)
{
    double stiffness = fmax(configuration.radial_stiffness_newtons_per_meter
        * modifiers.vertical_stiffness_scale, 1000.0);
    double maximum_deflection = fmax(configuration.maximum_deflection_meters
        * modifiers.maximum_deflection_scale, 0.001);
    double mass = fmax(configuration.wheel_mass_kilograms, 1.0);
    double damping = 2.0 * ClampValue(configuration.radial_damping_ratio, 0.05, 1.5)
        * sqrt(stiffness * mass) * modifiers.vertical_damping_scale;
    double normal_force = fmax(input.normal_force_newtons, 0.0);
    state.deflection = ClampValue(input.deflection_meters, 0.0, maximum_deflection);
    state.deflection_velocity = input.deflection_velocity_meters_per_second;
    if (input.deflection_meters > maximum_deflection)
        state.clamp_count += 1;
    state.contact_fraction = input.grounded && normal_force > 1.0e-6
        ? ClampValue(normal_force / fmax(configuration.static_load_newtons * 0.08, 1.0), 0.35, 1.0)
        : 0.0;

    double deflection_ratio = ClampValue(state.deflection / maximum_deflection, 0.0, 1.5);
    state.effective_radius = configuration.radius_meters
        - state.deflection * ClampValue(configuration.rolling_radius_deflection_factor, 0.0, 1.0);
    state.effective_radius = ClampValue(state.effective_radius,
        configuration.radius_meters * 0.86, configuration.radius_meters);
    double load_ratio = fmax(normal_force / fmax(configuration.static_load_newtons, 1.0), 0.05);
    double patch_scale = ClampValue(1.0 + configuration.contact_patch_load_gain * (load_ratio - 1.0)
        + configuration.contact_patch_deflection_gain * deflection_ratio, 0.70, 1.45);
    state.contact_patch = fmax(input.base_contact_patch_meters, 0.03) * patch_scale
        * ClampValue(modifiers.contact_patch_scale, 0.0, 2.0);
    double exponent = ClampValue(input.load_sensitivity_exponent, 0.5, 1.2);
    state.load_sensitivity_scale = ClampValue(pow(load_ratio, exponent - 1.0), 0.75, 1.20);
    state.vertical_stiffness = stiffness;
    state.vertical_damping = damping;
    state.maximum_deflection = maximum_deflection;

    output.deflection_meters = state.deflection;
    output.deflection_velocity_meters_per_second = state.deflection_velocity;
    output.effective_radius_meters = state.effective_radius;
    output.contact_patch_meters = state.contact_patch;
    output.load_sensitivity_scale = state.load_sensitivity_scale;
    output.vertical_stiffness_newtons_per_meter = stiffness;
    output.vertical_damping_newtons_per_meter = damping;
    output.maximum_deflection_meters = maximum_deflection;
    output.contact_fraction = state.contact_fraction;
    output.clamp_count = state.clamp_count;
}
