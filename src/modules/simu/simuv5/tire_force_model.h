/***************************************************************************

    file                 : tire_force_model.h
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

#ifndef TIRE_FORCE_MODEL_H
#define TIRE_FORCE_MODEL_H

#include "tire_model_configuration.h"
#include "tire_model_state.h"

struct TireForceInput
{
    double normal_force_newtons;
    double surface_friction_multiplier;
    double surface_stiffness;
    double surface_rolling_resistance;
    double effective_radius_meters;
    double forward_velocity_meters_per_second;
    double lateral_velocity_meters_per_second;
    double spin_velocity_radians_per_second;
    double camber_radians;
    double contact_fraction;
    double contact_patch_ratio;
    double load_sensitivity_scale;
    double thermal_grip_scale;
    double wear_grip_scale;
    double external_grip_scale;
    double force_stiffness_scale;
    double relaxation_length_scale;
    double pneumatic_trail_scale;
    double rolling_resistance_scale;
    double elapsed_seconds;
    int wheel_right;
};

struct TireForceOutput
{
    double longitudinal_force_newtons;
    double lateral_force_newtons;
    double slip_lateral_force_newtons;
    double aligning_torque_newton_meters;
    double wheel_reaction_torque_newton_meters;
    double rolling_resistance_force_newtons;
    double slip_velocity_longitudinal_meters_per_second;
    double slip_velocity_lateral_meters_per_second;
    double slip_power_watts;
    double combined_demand;
    double combined_utilization;
    double lateral_capacity_newtons;
    double longitudinal_capacity_newtons;
    double effective_lateral_peak_radians;
    double effective_longitudinal_peak;
    double slip_angle_radians;
    double slip_ratio;
    double contact_blend;
    int regime;
};

void TireForceModelReset(TireForceState& state, const TireForceConfiguration& configuration,
    double effective_radius_meters);
void TireForceModelEvaluate(TireForceState& state, const TireForceConfiguration& configuration,
    const TireForceInput& input, TireForceOutput& output);
double TirePureCurveCoefficient(double slip_absolute, double peak_slip, double slide_grip_ratio,
    double rise_gamma, double falloff_sharpness);
double TireCombinedBudgetScale(double utilization, double blend_width);

#endif
