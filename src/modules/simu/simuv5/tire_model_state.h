/***************************************************************************

    file                 : tire_model_state.h
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

#ifndef TIRE_MODEL_STATE_H
#define TIRE_MODEL_STATE_H

struct TireForceState
{
    double slip_angle_rad;
    double slip_ratio;
    double effective_slip_angle_rad;
    double effective_slip_ratio;
    double post_peak_decay_lateral;
    double post_peak_decay_longitudinal;
    double contact_blend;
    double longitudinal_force;
    double lateral_force;
    double aligning_torque;
    double wheel_reaction_torque;
    double rolling_resistance_force;
    double combined_demand;
    double combined_utilization;
    double lateral_capacity;
    double longitudinal_capacity;
    double effective_lateral_peak;
    double effective_longitudinal_peak;
    double slip_velocity_longitudinal;
    double slip_velocity_lateral;
    double slip_power;
    int regime;
};

struct TireContactState
{
    double empirical_deformation;
    double effective_radius;
    double contact_patch;
    double load_sensitivity_scale;
    double empirical_deformation_limit;
    double contact_fraction;
    long long clamp_count;
};

struct TireThermalState
{
    double tread_inner_kelvin;
    double tread_center_kelvin;
    double tread_outer_kelvin;
    double carcass_kelvin;
    double gas_kelvin;
    double pressure_absolute_pascals;
    double last_slip_power_watts;
    double last_external_power_watts;
    double last_balance_residual_joules;
    double accumulated_deposited_joules;
    double accumulated_external_joules;
    long long pressure_clamp_count;
    long long temperature_clamp_count;
    long long update_count;
};

struct TireWearState
{
    double tread_inner_millimeters;
    double tread_center_millimeters;
    double tread_outer_millimeters;
    double inner_wear_fraction;
    double center_wear_fraction;
    double outer_wear_fraction;
    double wear_grip_scale;
    double accumulated_friction_work_joules;
    double last_friction_power_watts;
    double last_temperature_multipliers[3];
    int punctured;
};

struct TireWheelModelState
{
    TireForceState force;
    TireContactState contact;
    TireThermalState thermal;
    TireWearState wear;
    int initialized;
};

#endif
