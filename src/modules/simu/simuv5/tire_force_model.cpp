/***************************************************************************

    file                 : tire_force_model.cpp
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

#include "tire_force_model.h"

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

static double SignValue(double value)
{
    if (value > 0.0)
    {
        return 1.0;
    }
    if (value < 0.0)
    {
        return -1.0;
    }
    return 0.0;
}

static double AdvanceMemory(double current, double target, double loss_seconds,
    double recovery_seconds, double elapsed_seconds)
{
    double tau = target > current ? loss_seconds : recovery_seconds;
    if (tau <= 1.0e-6 || elapsed_seconds <= 0.0)
    {
        return target;
    }
    double fraction = 1.0 - exp(-elapsed_seconds / tau);
    return ClampValue(current + (target - current) * fraction, 0.0, 1.0);
}

double TirePureCurveCoefficient(double slip_absolute, double peak_slip, double slide_grip_ratio,
    double rise_gamma, double falloff_sharpness)
{
    double peak = peak_slip > 1.0e-6 ? peak_slip : 1.0e-6;
    double slide = ClampValue(slide_grip_ratio, 0.0, 1.0);
    double gamma = rise_gamma > 1.2 ? rise_gamma : 1.2;
    double normalized = slip_absolute > 0.0 ? slip_absolute / peak : 0.0;
    if (normalized <= 1.0)
    {
        return ClampValue(1.0 - pow(1.0 - normalized, gamma), 0.0, 1.0);
    }
    double sharpness = ClampValue(falloff_sharpness, 0.1, 16.0);
    double distance = sharpness * (normalized - 1.0);
    double decay = distance * distance / (1.0 + distance * distance);
    return ClampValue(slide + (1.0 - slide) * (1.0 - decay), 0.0, 1.0);
}

double TireCombinedBudgetScale(double utilization, double blend_width)
{
    double value = utilization > 0.0 ? utilization : 0.0;
    double width = ClampValue(blend_width, 0.0, 0.5);
    if (width <= 1.0e-9)
    {
        return value > 1.0 ? 1.0 / value : 1.0;
    }
    double lower = 1.0 - width;
    double upper = 1.0 + width;
    if (value <= lower)
    {
        return 1.0;
    }
    if (value >= upper)
    {
        return 1.0 / value;
    }
    double span = upper - lower;
    double t = (value - lower) / span;
    double end_value = 1.0 / upper;
    double end_slope = -1.0 / (upper * upper);
    double t2 = t * t;
    double t3 = t2 * t;
    double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    double h01 = -2.0 * t3 + 3.0 * t2;
    double h11 = t3 - t2;
    double result = h00 + h01 * end_value + h11 * span * end_slope;
    return ClampValue(result, end_value, 1.0);
}

static double CurveDecayWeight(double slip_absolute, double peak_slip, double falloff_sharpness)
{
    double peak = peak_slip > 1.0e-6 ? peak_slip : 1.0e-6;
    double normalized = slip_absolute > 0.0 ? slip_absolute / peak : 0.0;
    if (normalized <= 1.0)
    {
        return 0.0;
    }
    double sharpness = ClampValue(falloff_sharpness, 0.1, 16.0);
    double distance = sharpness * (normalized - 1.0);
    return distance * distance / (1.0 + distance * distance);
}

void TireForceModelReset(TireForceState& state, const TireForceConfiguration& configuration,
    double effective_radius_meters)
{
    state.slip_angle_rad = 0.0;
    state.slip_ratio = 0.0;
    state.effective_slip_angle_rad = 0.0;
    state.effective_slip_ratio = 0.0;
    state.post_peak_decay_lateral = 0.0;
    state.post_peak_decay_longitudinal = 0.0;
    state.contact_blend = 0.0;
    state.longitudinal_force = 0.0;
    state.lateral_force = 0.0;
    state.aligning_torque = 0.0;
    state.wheel_reaction_torque = 0.0;
    state.rolling_resistance_force = 0.0;
    state.combined_demand = 0.0;
    state.combined_utilization = 0.0;
    state.lateral_capacity = 0.0;
    state.longitudinal_capacity = 0.0;
    state.effective_lateral_peak = configuration.lateral_peak_slip_angle_rad;
    state.effective_longitudinal_peak = configuration.longitudinal_peak_slip_ratio;
    state.slip_velocity_longitudinal = 0.0;
    state.slip_velocity_lateral = 0.0;
    state.slip_power = 0.0;
    state.regime = 0;
}

void TireForceModelEvaluate(TireForceState& state, const TireForceConfiguration& configuration,
    const TireForceInput& input, TireForceOutput& output)
{
    double elapsed = input.elapsed_seconds > 0.0 ? input.elapsed_seconds : 0.0;
    double normal_force = input.normal_force_newtons > 0.0 ? input.normal_force_newtons : 0.0;
    double contact_fraction = ClampValue(input.contact_fraction, 0.0, 1.0);
    bool loaded = normal_force > 1.0e-4 && contact_fraction > 0.0;
    double ramp_seconds = ClampValue(configuration.contact_ramp_seconds, 0.0, 1.0);
    if (ramp_seconds <= 1.0e-9)
    {
        state.contact_blend = loaded ? 1.0 : 0.0;
    }
    else
    {
        double target = loaded ? 1.0 : 0.0;
        double fraction = 1.0 - exp(-elapsed / ramp_seconds);
        state.contact_blend = ClampValue(state.contact_blend + (target - state.contact_blend) * fraction, 0.0, 1.0);
    }

    output.longitudinal_force_newtons = 0.0;
    output.lateral_force_newtons = 0.0;
    output.slip_lateral_force_newtons = 0.0;
    output.aligning_torque_newton_meters = 0.0;
    output.wheel_reaction_torque_newton_meters = 0.0;
    output.rolling_resistance_force_newtons = 0.0;
    output.slip_velocity_longitudinal_meters_per_second = 0.0;
    output.slip_velocity_lateral_meters_per_second = 0.0;
    output.slip_power_watts = 0.0;
    output.combined_demand = 0.0;
    output.combined_utilization = 0.0;
    output.lateral_capacity_newtons = 0.0;
    output.longitudinal_capacity_newtons = 0.0;
    output.effective_lateral_peak_radians = configuration.lateral_peak_slip_angle_rad;
    output.effective_longitudinal_peak = configuration.longitudinal_peak_slip_ratio;
    output.slip_angle_radians = state.slip_angle_rad;
    output.slip_ratio = state.slip_ratio;
    output.contact_blend = state.contact_blend;
    output.regime = 0;

    if (!loaded && state.contact_blend <= 1.0e-6)
    {
        double decay = exp(-elapsed / 0.08);
        state.slip_angle_rad = 0.0;
        state.slip_ratio = 0.0;
        state.effective_slip_angle_rad *= decay;
        state.effective_slip_ratio *= decay;
        state.post_peak_decay_lateral *= exp(-elapsed / 0.25);
        state.post_peak_decay_longitudinal *= exp(-elapsed / 0.25);
        state.longitudinal_force = 0.0;
        state.lateral_force = 0.0;
        state.aligning_torque = 0.0;
        state.wheel_reaction_torque = 0.0;
        state.rolling_resistance_force = 0.0;
        state.combined_demand = 0.0;
        state.combined_utilization = 0.0;
        state.slip_power = 0.0;
        return;
    }

    double forward_velocity = input.forward_velocity_meters_per_second;
    double lateral_velocity = input.lateral_velocity_meters_per_second;
    double effective_radius = input.effective_radius_meters > 0.05 ? input.effective_radius_meters : 0.05;
    double wheel_velocity = input.spin_velocity_radians_per_second * effective_radius;
    double target_alpha = ClampValue(atan2(-lateral_velocity, fmax(fabs(forward_velocity), 0.5)), -0.75, 0.75);
    double denominator = fmax(fmax(fabs(forward_velocity), fabs(wheel_velocity)), 1.0);
    double target_kappa = ClampValue((wheel_velocity - forward_velocity) / denominator, -3.0, 3.0);
    state.slip_angle_rad = target_alpha;
    state.slip_ratio = target_kappa;

    double planar_speed = sqrt(forward_velocity * forward_velocity + lateral_velocity * lateral_velocity);
    double patch_ratio = ClampValue(input.contact_patch_ratio, 0.7, 1.5);
    double relaxation_length = configuration.relaxation_length_meters > 0.0
        ? configuration.relaxation_length_meters : 0.5;
    relaxation_length *= sqrt(patch_ratio) * (input.relaxation_length_scale > 0.0 ? input.relaxation_length_scale : 1.0);

    if (!configuration.relaxation_enabled)
    {
        state.effective_slip_angle_rad = target_alpha;
        state.effective_slip_ratio = target_kappa;
    }
    else
    {
        double relaxation_tau = fmax(relaxation_length / fmax(planar_speed, 4.0),
            ClampValue(configuration.minimum_relaxation_seconds, 0.0, 0.5));
        double recovery_scale = ClampValue(configuration.relaxation_recovery_scale, 0.1, 5.0);
        bool lateral_recovering = fabs(state.effective_slip_angle_rad) > fabs(target_alpha);
        bool longitudinal_recovering = fabs(state.effective_slip_ratio) > fabs(target_kappa);
        double lateral_tau = lateral_recovering ? relaxation_tau * recovery_scale : relaxation_tau;
        double longitudinal_tau = longitudinal_recovering ? relaxation_tau * recovery_scale : relaxation_tau;
        double lateral_fraction = elapsed > 0.0 ? 1.0 - exp(-elapsed / fmax(lateral_tau, 1.0e-4)) : 1.0;
        double longitudinal_fraction = elapsed > 0.0 ? 1.0 - exp(-elapsed / fmax(longitudinal_tau, 1.0e-4)) : 1.0;
        state.effective_slip_angle_rad += (target_alpha - state.effective_slip_angle_rad) * lateral_fraction;
        state.effective_slip_ratio += (target_kappa - state.effective_slip_ratio) * longitudinal_fraction;
    }

    double physical_camber = input.wheel_right ? input.camber_radians : -input.camber_radians;
    double camber_thrust = (input.wheel_right ? -physical_camber : physical_camber)
        * configuration.camber_thrust_gain;
    double alpha_with_camber = state.effective_slip_angle_rad + camber_thrust;

    double mu = configuration.compound_friction * input.surface_friction_multiplier
        * input.load_sensitivity_scale * input.thermal_grip_scale * input.wear_grip_scale
        * input.external_grip_scale;
    if (mu < 0.0)
    {
        mu = 0.0;
    }
    double longitudinal_ratio = ClampValue(configuration.longitudinal_grip_ratio, 0.10, 1.50);
    double lateral_capacity = mu * normal_force;
    double longitudinal_capacity = mu * longitudinal_ratio * normal_force;
    state.lateral_capacity = lateral_capacity;
    state.longitudinal_capacity = longitudinal_capacity;
    output.lateral_capacity_newtons = lateral_capacity;
    output.longitudinal_capacity_newtons = longitudinal_capacity;

    double stiffness_scale = sqrt(ClampValue(input.surface_stiffness / 5.0, 0.15, 2.5));
    double rise_gamma = ClampValue(2.0 * stiffness_scale * sqrt(patch_ratio) * input.force_stiffness_scale,
        1.2, 4.0);
    double peak_lateral = configuration.lateral_peak_slip_angle_rad > 1.0e-6
        ? configuration.lateral_peak_slip_angle_rad : 1.0e-6;
    double peak_longitudinal = configuration.longitudinal_peak_slip_ratio > 1.0e-6
        ? configuration.longitudinal_peak_slip_ratio : 1.0e-6;

    double lateral_demand = ClampValue(fabs(alpha_with_camber) / peak_lateral, 0.0, 1.0);
    double longitudinal_demand = ClampValue(fabs(state.effective_slip_ratio) / peak_longitudinal, 0.0, 1.0);
    double migrated_lateral_peak = peak_lateral;
    double migrated_longitudinal_peak = peak_longitudinal;
    if (configuration.peak_migration_enabled)
    {
        migrated_lateral_peak = peak_lateral * fmax(0.1,
            1.0 - ClampValue(configuration.combined_lateral_peak_migration, 0.0, 0.9) * longitudinal_demand);
        migrated_longitudinal_peak = peak_longitudinal * fmax(0.1,
            1.0 - ClampValue(configuration.combined_longitudinal_peak_migration, 0.0, 0.9) * lateral_demand);
    }
    state.effective_lateral_peak = migrated_lateral_peak;
    state.effective_longitudinal_peak = migrated_longitudinal_peak;
    output.effective_lateral_peak_radians = migrated_lateral_peak;
    output.effective_longitudinal_peak = migrated_longitudinal_peak;

    double lateral_base = TirePureCurveCoefficient(fabs(alpha_with_camber), peak_lateral,
        configuration.lateral_slide_grip_ratio, rise_gamma, configuration.falloff_sharpness);
    double longitudinal_base = TirePureCurveCoefficient(fabs(state.effective_slip_ratio), peak_longitudinal,
        configuration.longitudinal_slide_grip_ratio, rise_gamma, configuration.falloff_sharpness);
    double lateral_curve = TirePureCurveCoefficient(fabs(alpha_with_camber), migrated_lateral_peak,
        configuration.lateral_slide_grip_ratio, rise_gamma, configuration.falloff_sharpness);
    double longitudinal_curve = TirePureCurveCoefficient(fabs(state.effective_slip_ratio), migrated_longitudinal_peak,
        configuration.longitudinal_slide_grip_ratio, rise_gamma, configuration.falloff_sharpness);
    double lateral_decay_target = CurveDecayWeight(fabs(alpha_with_camber), migrated_lateral_peak,
        configuration.falloff_sharpness);
    double longitudinal_decay_target = CurveDecayWeight(fabs(state.effective_slip_ratio), migrated_longitudinal_peak,
        configuration.falloff_sharpness);
    if (configuration.peak_migration_enabled)
    {
        state.post_peak_decay_lateral = AdvanceMemory(state.post_peak_decay_lateral, lateral_decay_target,
            configuration.slip_loss_seconds, configuration.slip_recovery_seconds, elapsed);
        state.post_peak_decay_longitudinal = AdvanceMemory(state.post_peak_decay_longitudinal,
            longitudinal_decay_target, configuration.slip_loss_seconds, configuration.slip_recovery_seconds,
            elapsed);
    }
    else
    {
        state.post_peak_decay_lateral = lateral_decay_target;
        state.post_peak_decay_longitudinal = longitudinal_decay_target;
    }

    double lateral_flat = lateral_curve
        / fmax(1.0e-6, 1.0 - lateral_decay_target * (1.0 - configuration.lateral_slide_grip_ratio));
    double longitudinal_flat = longitudinal_curve
        / fmax(1.0e-6, 1.0 - longitudinal_decay_target * (1.0 - configuration.longitudinal_slide_grip_ratio));
    double lateral_coefficient = lateral_flat
        * (1.0 - state.post_peak_decay_lateral * (1.0 - configuration.lateral_slide_grip_ratio));
    double longitudinal_coefficient = longitudinal_flat
        * (1.0 - state.post_peak_decay_longitudinal * (1.0 - configuration.longitudinal_slide_grip_ratio));
    lateral_coefficient = ClampValue(lateral_coefficient, 0.0, 1.0);
    longitudinal_coefficient = ClampValue(longitudinal_coefficient, 0.0, 1.0);

    double longitudinal_candidate = longitudinal_capacity * longitudinal_coefficient
        * SignValue(state.effective_slip_ratio);
    double lateral_candidate = lateral_capacity * lateral_coefficient * SignValue(alpha_with_camber);
    double lateral_slip_candidate = lateral_capacity * lateral_coefficient * SignValue(state.effective_slip_angle_rad);

    double base_norm = sqrt(longitudinal_base * longitudinal_base + lateral_base * lateral_base);
    double migrated_norm = sqrt(longitudinal_coefficient * longitudinal_coefficient
        + lateral_coefficient * lateral_coefficient);
    double demand = fmax(base_norm, migrated_norm);
    double budget_scale = TireCombinedBudgetScale(demand, configuration.budget_blend_width);
    double longitudinal_force = longitudinal_candidate * budget_scale;
    double lateral_force = lateral_candidate * budget_scale;
    double lateral_slip_force = lateral_slip_candidate * budget_scale;

    state.combined_demand = demand;
    state.combined_utilization = ClampValue(migrated_norm * budget_scale * state.contact_blend, 0.0, 1.0);
    state.regime = (CurveDecayWeight(fabs(alpha_with_camber), migrated_lateral_peak, configuration.falloff_sharpness) > 0.0 ? 1 : 0)
        | (CurveDecayWeight(fabs(state.effective_slip_ratio), migrated_longitudinal_peak, configuration.falloff_sharpness) > 0.0 ? 2 : 0);

    double rolling_resistance = normal_force * input.surface_rolling_resistance
        * (input.rolling_resistance_scale > 0.0 ? input.rolling_resistance_scale : 1.0);
    double blend = state.contact_blend;
    longitudinal_force *= blend;
    lateral_force *= blend;
    lateral_slip_force *= blend;
    rolling_resistance *= blend;

    double normalized_longitudinal = longitudinal_capacity > 1.0e-9
        ? ClampValue(fabs(longitudinal_force) / longitudinal_capacity, 0.0, 1.0) : 0.0;
    double normalized_lateral = lateral_capacity > 1.0e-9
        ? ClampValue(fabs(lateral_force) / lateral_capacity, 0.0, 1.0) : 0.0;
    double trail = configuration.pneumatic_trail_meters
        * exp(-2.5 * normalized_lateral)
        * fmax(0.0, 1.0 - 0.65 * normalized_longitudinal * normalized_longitudinal)
        * sqrt(contact_fraction)
        * (input.pneumatic_trail_scale > 0.0 ? input.pneumatic_trail_scale : 1.0);
    double lateral_peak_excess = ClampValue((fabs(alpha_with_camber)
        / fmax(migrated_lateral_peak, 1.0e-6) - 1.0) / 1.5, 0.0, 1.0);
    trail *= fmax(0.15, 1.0 - 0.75 * lateral_peak_excess);
    double aligning_torque = lateral_force * trail;

    double slip_velocity_longitudinal = wheel_velocity - forward_velocity;
    double slip_velocity_lateral = -lateral_velocity;
    double slip_power = longitudinal_force * slip_velocity_longitudinal
        + lateral_slip_force * slip_velocity_lateral;
    if (slip_power < 0.0)
    {
        slip_power = 0.0;
    }

    state.longitudinal_force = longitudinal_force;
    state.lateral_force = lateral_force;
    state.aligning_torque = aligning_torque;
    state.wheel_reaction_torque = longitudinal_force * effective_radius;
    state.rolling_resistance_force = rolling_resistance;
    state.slip_velocity_longitudinal = slip_velocity_longitudinal;
    state.slip_velocity_lateral = slip_velocity_lateral;
    state.slip_power = slip_power;

    output.longitudinal_force_newtons = longitudinal_force;
    output.lateral_force_newtons = lateral_force;
    output.slip_lateral_force_newtons = lateral_slip_force;
    output.aligning_torque_newton_meters = aligning_torque;
    output.wheel_reaction_torque_newton_meters = state.wheel_reaction_torque;
    output.rolling_resistance_force_newtons = rolling_resistance;
    output.slip_velocity_longitudinal_meters_per_second = slip_velocity_longitudinal;
    output.slip_velocity_lateral_meters_per_second = slip_velocity_lateral;
    output.slip_power_watts = slip_power;
    output.combined_demand = state.combined_demand;
    output.combined_utilization = state.combined_utilization;
    output.slip_angle_radians = state.slip_angle_rad;
    output.slip_ratio = state.slip_ratio;
    output.contact_blend = state.contact_blend;
    output.regime = state.regime;
}
