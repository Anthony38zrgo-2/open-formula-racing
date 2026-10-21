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
    const double limit = fmax(configuration.empirical_deformation_limit_meters
        * modifiers.empirical_deformation_limit_scale, 0.001);
    const double compliance = fmax(configuration.empirical_load_compliance_meters_per_newton
        * modifiers.empirical_compliance_scale, 1.0e-12);
    state.empirical_deformation = ClampValue(
        configuration.static_load_newtons * compliance, 0.0, limit);
    state.empirical_deformation_limit = limit;
    state.effective_radius = configuration.radius_meters
        - state.empirical_deformation
            * ClampValue(configuration.rolling_radius_load_deflection_factor, 0.0, 1.0);
    state.effective_radius = ClampValue(state.effective_radius,
        configuration.radius_meters * 0.86, configuration.radius_meters);
    state.contact_patch = configuration.width_meters * 0.8;
    state.load_sensitivity_scale = 1.0;
    state.contact_fraction = 0.0;
    state.clamp_count = 0;
}

void TireContactModelEvaluate(TireContactState& state, const TireContactConfiguration& configuration,
    const TireMechanicalModifiers& modifiers, const TireContactInput& input, TireContactOutput& output)
{
    const double limit = fmax(configuration.empirical_deformation_limit_meters
        * modifiers.empirical_deformation_limit_scale, 0.001);
    const double compliance = fmax(configuration.empirical_load_compliance_meters_per_newton
        * modifiers.empirical_compliance_scale, 1.0e-12);
    const double normal_force = fmax(input.normal_force_newtons, 0.0);

    /* Algebraic, quasi-static estimate used only by contact patch, rolling
     * radius and thermal volume.  It is not integrated and never feeds the
     * vertical solver or the geometric wheel centre. */
    const double raw_deformation = normal_force * compliance;
    state.empirical_deformation = ClampValue(raw_deformation, 0.0, limit);
    if (raw_deformation > limit)
    {
        state.clamp_count += 1;
    }
    state.empirical_deformation_limit = limit;
    state.contact_fraction = input.grounded
        ? ClampValue(input.contact_fraction, 0.0, 1.0) : 0.0;

    const double deformation_ratio = ClampValue(state.empirical_deformation / limit, 0.0, 1.5);
    state.effective_radius = configuration.radius_meters
        - state.empirical_deformation
            * ClampValue(configuration.rolling_radius_load_deflection_factor, 0.0, 1.0);
    state.effective_radius = ClampValue(state.effective_radius,
        configuration.radius_meters * 0.86, configuration.radius_meters);

    const double load_ratio = fmax(normal_force
        / fmax(configuration.static_load_newtons, 1.0), 0.05);
    const double patch_scale = ClampValue(
        1.0 + configuration.contact_patch_load_gain * (load_ratio - 1.0)
            + configuration.contact_patch_empirical_deformation_gain * deformation_ratio,
        0.70, 1.45);
    state.contact_patch = fmax(input.base_contact_patch_meters, 0.03) * patch_scale
        * ClampValue(modifiers.contact_patch_scale, 0.0, 2.0);
    const double exponent = ClampValue(input.load_sensitivity_exponent, 0.5, 1.2);
    state.load_sensitivity_scale = ClampValue(pow(load_ratio, exponent - 1.0), 0.75, 1.20);

    output.empirical_deformation_meters = state.empirical_deformation;
    output.effective_radius_meters = state.effective_radius;
    output.contact_patch_meters = state.contact_patch;
    output.load_sensitivity_scale = state.load_sensitivity_scale;
    output.empirical_deformation_limit_meters = limit;
    output.contact_fraction = state.contact_fraction;
    output.clamp_count = state.clamp_count;
}
