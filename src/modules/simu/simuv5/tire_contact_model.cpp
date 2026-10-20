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

void TireContactModelUpdate(TireContactState& state, const TireContactConfiguration& configuration,
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
    double elapsed = ClampValue(input.elapsed_seconds, 0.0, 0.05);
    int step_count = (int)ceil(elapsed / 0.001);
    if (step_count < 1)
    {
        step_count = 1;
    }
    if (step_count > 64)
    {
        step_count = 64;
    }
    double step_seconds = elapsed > 0.0 ? elapsed / (double)step_count : 0.0;

    if (!input.grounded || normal_force <= 1.0e-6)
    {
        double previous = state.deflection;
        double decay_seconds = 0.05;
        double fraction = elapsed > 0.0 ? 1.0 - exp(-elapsed / decay_seconds) : 1.0;
        state.deflection += (0.0 - state.deflection) * fraction;
        state.deflection_velocity = elapsed > 0.0 ? (state.deflection - previous) / elapsed : 0.0;
        state.contact_fraction = 0.0;
    }
    else
    {
        for (int step = 0; step < step_count; ++step)
        {
            double spring_force = stiffness * state.deflection;
            double damping_force = damping * state.deflection_velocity;
            double acceleration = (normal_force - spring_force - damping_force) / mass;
            state.deflection_velocity += acceleration * step_seconds;
            state.deflection += state.deflection_velocity * step_seconds;
            if (state.deflection < 0.0)
            {
                state.deflection = 0.0;
                if (state.deflection_velocity < 0.0)
                {
                    state.deflection_velocity = 0.0;
                }
            }
            else if (state.deflection > maximum_deflection)
            {
                state.deflection = maximum_deflection;
                if (state.deflection_velocity > 0.0)
                {
                    state.deflection_velocity = 0.0;
                }
                state.clamp_count += 1;
            }
        }
        state.contact_fraction = ClampValue(normal_force
            / fmax(configuration.static_load_newtons * 0.08, 1.0), 0.0, 1.0);
        if (state.contact_fraction < 0.35)
        {
            state.contact_fraction = 0.35;
        }
    }

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
