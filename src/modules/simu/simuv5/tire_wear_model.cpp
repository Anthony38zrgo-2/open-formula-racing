/***************************************************************************

    file                 : tire_wear_model.cpp
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

#include "tire_wear_model.h"

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

static const double KelvinOffset = 273.15;

double TireWearTemperatureMultiplier(double temperature_celsius,
    const TireWearConfiguration& configuration)
{
    double cold = configuration.cold_tread_temperature_celsius;
    double optimal = fmax(configuration.optimal_tread_temperature_celsius, cold + 1.0);
    double overheat = fmax(configuration.overheat_tread_temperature_celsius, optimal + 1.0);
    double extreme = fmax(configuration.extreme_tread_temperature_celsius, overheat + 1.0);
    double cold_multiplier = fmax(configuration.cold_temperature_wear_multiplier, 0.0);
    double overheat_multiplier = fmax(configuration.overheat_temperature_wear_multiplier, 0.0);
    double extreme_multiplier = fmax(configuration.extreme_temperature_wear_multiplier, 0.0);
    if (temperature_celsius <= cold)
    {
        return cold_multiplier;
    }
    if (temperature_celsius <= optimal)
    {
        return cold_multiplier + (1.0 - cold_multiplier)
            * (temperature_celsius - cold) / (optimal - cold);
    }
    if (temperature_celsius <= overheat)
    {
        return 1.0 + (overheat_multiplier - 1.0)
            * (temperature_celsius - optimal) / (overheat - optimal);
    }
    if (temperature_celsius <= extreme)
    {
        return overheat_multiplier + (extreme_multiplier - overheat_multiplier)
            * (temperature_celsius - overheat) / (extreme - overheat);
    }
    return extreme_multiplier;
}

double TireWearGripScale(double wear_fraction, const TireWearConfiguration& configuration)
{
    double wear = ClampValue(wear_fraction, 0.0, 1.0);
    double end_of_life = ClampValue(configuration.end_of_life_grip_scale, 0.0, 1.0);
    double progressive = ClampValue(configuration.progressive_grip_loss_fraction, 0.0, 1.0 - end_of_life);
    double cliff_start = ClampValue(configuration.cliff_start_wear_fraction, 0.0, 1.0);
    double cliff_span = fmax(1.0 - cliff_start, 1.0e-6);
    double cliff_progress = ClampValue((wear - cliff_start) / cliff_span, 0.0, 1.0);
    double smoothed = cliff_progress * cliff_progress * (3.0 - 2.0 * cliff_progress);
    double cliff_loss = smoothed * fmax(1.0 - progressive - end_of_life, 0.0);
    return ClampValue(1.0 - progressive * wear - cliff_loss, 0.0, 1.0);
}

void TireWearModelReset(TireWearState& state, const TireWearConfiguration& configuration)
{
    double depth = fmax(configuration.initial_tread_depth_millimeters, 0.0);
    state.tread_inner_millimeters = depth;
    state.tread_center_millimeters = depth;
    state.tread_outer_millimeters = depth;
    state.inner_wear_fraction = 0.0;
    state.center_wear_fraction = 0.0;
    state.outer_wear_fraction = 0.0;
    state.wear_grip_scale = 1.0;
    state.accumulated_friction_work_joules = 0.0;
    state.last_friction_power_watts = 0.0;
    state.last_temperature_multipliers[0] = 1.0;
    state.last_temperature_multipliers[1] = 1.0;
    state.last_temperature_multipliers[2] = 1.0;
    state.punctured = 0;
}

static void UpdateWearFractions(TireWearState& state, const TireWearConfiguration& configuration)
{
    double usable = fmax(configuration.initial_tread_depth_millimeters
        - configuration.minimum_tread_depth_millimeters, 1.0e-6);
    state.inner_wear_fraction = ClampValue((configuration.initial_tread_depth_millimeters
        - state.tread_inner_millimeters) / usable, 0.0, 1.0);
    state.center_wear_fraction = ClampValue((configuration.initial_tread_depth_millimeters
        - state.tread_center_millimeters) / usable, 0.0, 1.0);
    state.outer_wear_fraction = ClampValue((configuration.initial_tread_depth_millimeters
        - state.tread_outer_millimeters) / usable, 0.0, 1.0);
}

void TireWearModelUpdate(TireWearState& state, const TireWearConfiguration& configuration,
    const TireWearInput& input)
{
    double elapsed = ClampValue(input.elapsed_seconds, 0.0, 0.05);
    double friction_power = fmax(input.friction_power_watts, 0.0);
    state.accumulated_friction_work_joules += friction_power * elapsed;
    state.last_friction_power_watts = friction_power;
    if (state.punctured)
    {
        state.wear_grip_scale = 0.25;
        return;
    }
    double usable = configuration.initial_tread_depth_millimeters
        - configuration.minimum_tread_depth_millimeters;
    if (usable <= 0.0)
    {
        state.wear_grip_scale = 1.0;
        return;
    }

    double raw_weights[3] = { input.zone_contact_weights[0], input.zone_contact_weights[1],
        input.zone_contact_weights[2] };
    double weight_sum = fmax(raw_weights[0], 0.0) + fmax(raw_weights[1], 0.0) + fmax(raw_weights[2], 0.0);
    double zone_weights[3] = { 0.0, 0.0, 0.0 };
    if (weight_sum > 1.0e-9)
    {
        zone_weights[0] = fmax(raw_weights[0], 0.0) / weight_sum;
        zone_weights[1] = fmax(raw_weights[1], 0.0) / weight_sum;
        zone_weights[2] = fmax(raw_weights[2], 0.0) / weight_sum;
    }
    double friction_factor = ClampValue(pow(fmax(input.surface_friction_coefficient, 0.0)
        / fmax(configuration.friction_reference_coefficient, 1.0e-3),
        ClampValue(configuration.friction_sensitivity_exponent, 0.0, 4.0)), 0.05, 4.0);
    double zone_multipliers[3] = {
        fmax(configuration.inner_shoulder_wear_multiplier, 0.0),
        fmax(configuration.center_wear_multiplier, 0.0),
        fmax(configuration.outer_shoulder_wear_multiplier, 0.0)
    };
    double temperatures[3] = { input.tread_inner_kelvin, input.tread_center_kelvin,
        input.tread_outer_kelvin };
    double treads[3] = { state.tread_inner_millimeters, state.tread_center_millimeters,
        state.tread_outer_millimeters };

    for (int zone = 0; zone < 3; ++zone)
    {
        double temperature = temperatures[zone] - KelvinOffset;
        double temperature_multiplier = TireWearTemperatureMultiplier(temperature, configuration);
        state.last_temperature_multipliers[zone] = temperature_multiplier;
        double zone_power = friction_power * zone_weights[zone];
        double wear_millimeters = configuration.tread_wear_per_megajoule_millimeters
            * (zone_power * elapsed / 1000000.0) * temperature_multiplier * friction_factor
            * zone_multipliers[zone];
        treads[zone] = fmax(treads[zone] - wear_millimeters,
            configuration.minimum_tread_depth_millimeters);
    }
    state.tread_inner_millimeters = treads[0];
    state.tread_center_millimeters = treads[1];
    state.tread_outer_millimeters = treads[2];
    UpdateWearFractions(state, configuration);

    double grip = 0.0;
    double fractions[3] = { state.inner_wear_fraction, state.center_wear_fraction,
        state.outer_wear_fraction };
    for (int zone = 0; zone < 3; ++zone)
    {
        grip += zone_weights[zone] * TireWearGripScale(fractions[zone], configuration);
    }
    if (zone_weights[0] + zone_weights[1] + zone_weights[2] <= 1.0e-9)
    {
        grip = TireWearGripScale((state.inner_wear_fraction + state.center_wear_fraction
            + state.outer_wear_fraction) / 3.0, configuration);
    }
    state.wear_grip_scale = ClampValue(grip, 0.0, 1.0);

    double remaining = 1.0 - (state.inner_wear_fraction + state.center_wear_fraction
        + state.outer_wear_fraction) / 3.0;
    if (remaining <= 1.0e-6)
    {
        state.punctured = 1;
        state.wear_grip_scale = 0.25;
    }
}
