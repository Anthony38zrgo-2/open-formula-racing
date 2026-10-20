/***************************************************************************

    file                 : tire_model.cpp
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

#include "sim.h"
#include "tire_contact_model.h"
#include "tire_force_model.h"
#include "tire_thermal_model.h"
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

static double PhysicalCamber(const tWheel *wheel, int index)
{
    return (index % 2 == 0) ? wheel->relPos.ax : -wheel->relPos.ax;
}

static double RemainingTreadFraction(const TireWheelModelState& state)
{
    return ClampValue(1.0 - (state.wear.inner_wear_fraction + state.wear.center_wear_fraction
        + state.wear.outer_wear_fraction) / 3.0, 0.0, 1.0);
}

void SimTireModelPublishWheel(tCar *car, int index)
{
    if (!car->formula_tire_model_enabled)
    {
        return;
    }
    tWheel *wheel = &car->wheel[index];
    TireWheelModelState& state = wheel->formula_tire_model;
    car->carElt->_tyreT_in(index) = (tdble)state.thermal.tread_inner_kelvin;
    car->carElt->_tyreT_mid(index) = (tdble)state.thermal.tread_center_kelvin;
    car->carElt->_tyreT_out(index) = (tdble)state.thermal.tread_outer_kelvin;
    car->carElt->_tyreT_opt(index) = (tdble)(car->formula_tire_configuration.thermal[index / 2]
        .optimal_tread_temperature_celsius + 273.15);
    car->carElt->_tyreCondition(index) = (tdble)ClampValue(state.wear.wear_grip_scale, 0.0, 1.0);
    car->carElt->_tyreTreadDepth(index) = (tdble)RemainingTreadFraction(state);
    car->carElt->_tyreCurrentPressure(index) = (tdble)state.thermal.pressure_absolute_pascals;
    car->carElt->_tyreCompound(index) = wheel->tireSet;
}

void SimTireModelResetWheel(tCar *car, int index, bool new_tire)
{
    if (!car->formula_tire_model_enabled)
    {
        return;
    }
    tWheel *wheel = &car->wheel[index];
    int axle = index / 2;
    TireModelConfiguration& configuration = car->formula_tire_configuration;
    TireWheelModelState& state = wheel->formula_tire_model;
    TireMechanicalModifiers modifiers;
    TireThermalModelReset(state.thermal, configuration.thermal[axle], configuration.pressure, index);
    TireThermalModelMechanicalModifiers(state.thermal, configuration.thermal[axle],
        configuration.pressure, index, modifiers);
    TireContactModelReset(state.contact, configuration.contact[axle], modifiers);
    TireForceModelReset(state.force, configuration.force[axle], state.contact.effective_radius);
    TireWearModelReset(state.wear, configuration.wear[axle]);
    state.initialized = 1;
    wheel->formula_tire_model_grounded = 0;
    wheel->formula_tire_model_zone_weights[0] = 0.0;
    wheel->formula_tire_model_zone_weights[1] = 0.0;
    wheel->formula_tire_model_zone_weights[2] = 0.0;
    GfLogInfo("Coupled tire reset wheel=%d temperature=%.3f K pressure=%.3f Pa\n",
        index, state.thermal.tread_center_kelvin, state.thermal.pressure_absolute_pascals);
    wheel->Ttire = (tdble)((state.thermal.tread_inner_kelvin + state.thermal.tread_center_kelvin
        + state.thermal.tread_outer_kelvin) / 3.0);
    wheel->currentTemperature = wheel->Ttire;
    wheel->currentPressure = (tdble)state.thermal.pressure_absolute_pascals;
    wheel->currentWear = 0.0;
    wheel->currentGraining = 0.0;
    wheel->currentGripFactor = 1.0;
    wheel->treadDepth = 1.0;
    wheel->camber = (tdble)PhysicalCamber(wheel, index);
    if (!new_tire)
    {
        state.force.contact_blend = 1.0;
    }
    SimTireModelPublishWheel(car, index);
}

void SimTireModelConfigure(tCar *car)
{
    double static_load_newtons[4];
    double radius_meters[4];
    double width_meters[4];
    double wheel_mass_kilograms[4];
    for (int index = 0; index < 4; ++index)
    {
        static_load_newtons[index] = car->wheel[index].weight0;
        radius_meters[index] = car->wheel[index].radius;
        width_meters[index] = car->carElt->_tireWidth(index);
        wheel_mass_kilograms[index] = car->wheel[index].mass;
        radius_meters[index] = car->carElt->_wheelRadius(index);
    }
    car->formula_tire_model_enabled = TireModelConfigurationRead(car->formula_tire_configuration,
        car->params, static_load_newtons, radius_meters, width_meters, wheel_mass_kilograms,
        SimAirPressure, Tair) ? 1 : 0;
    if (!car->formula_tire_model_enabled)
    {
        GfLogInfo("SimuV5 tire model: legacy\n");
        return;
    }
    GfLogInfo("SimuV5 tire model: transient coupled\n");
    GfLogInfo("Coupled tire thermal initial front=%.3f C rear=%.3f C ambient=%.3f C\n",
        car->formula_tire_configuration.thermal[0].initial_temperature_celsius,
        car->formula_tire_configuration.thermal[1].initial_temperature_celsius,
        car->formula_tire_configuration.thermal[0].ambient_fallback_celsius);
    for (int index = 0; index < 4; ++index)
    {
        SimTireModelResetWheel(car, index, false);
    }
}

void SimTireModelUpdateWheel(tCar *car, int index)
{
    tWheel *wheel = &car->wheel[index];
    TireWheelModelState& state = wheel->formula_tire_model;
    int axle = index / 2;
    TireModelConfiguration& configuration = car->formula_tire_configuration;
    double elapsed = car->formula_tire_model_enabled ? (double)SimDeltaTime : 0.0;
    if (!state.initialized)
    {
        SimTireModelResetWheel(car, index, false);
    }

    TireMechanicalModifiers modifiers;
    TireThermalModelMechanicalModifiers(state.thermal, configuration.thermal[axle],
        configuration.pressure, index, modifiers);
    double base_contact_patch = configuration.force[axle].contact_patch_meters;
    TireContactInput contact_input;
    contact_input.normal_force_newtons = wheel->forces.z;
    contact_input.base_contact_patch_meters = base_contact_patch;
    contact_input.load_sensitivity_exponent = configuration.force[axle].load_sensitivity_exponent;
    contact_input.elapsed_seconds = elapsed;
    contact_input.grounded = wheel->formula_tire_model_grounded;
    TireContactOutput contact_output;
    TireContactModelUpdate(state.contact, configuration.contact[axle], modifiers, contact_input,
        contact_output);

    double waz = wheel->steer + wheel->staticPos.az;
    double cos_angle = cos(waz);
    double sin_angle = sin(waz);
    double forward_velocity = wheel->bodyVel.x * cos_angle + wheel->bodyVel.y * sin_angle;
    double lateral_velocity = -wheel->bodyVel.x * sin_angle + wheel->bodyVel.y * cos_angle;
    double vehicle_speed = sqrt(wheel->bodyVel.x * wheel->bodyVel.x
        + wheel->bodyVel.y * wheel->bodyVel.y);
    double wet_scale = 1.0 - (1.0 - ClampValue(configuration.wet_grip_scale, 0.0, 1.0))
        * ClampValue((double)SimRain / 3.0, 0.0, 1.0);

    TireForceInput force_input;
    force_input.normal_force_newtons = wheel->forces.z;
    force_input.surface_friction_multiplier = wheel->ground_friction;
    force_input.surface_stiffness = ClampValue(5.0 * wheel->ground_friction, 0.15, 15.0);
    force_input.surface_rolling_resistance = wheel->ground_rolling_resistance;
    force_input.effective_radius_meters = contact_output.effective_radius_meters;
    force_input.forward_velocity_meters_per_second = forward_velocity;
    force_input.lateral_velocity_meters_per_second = lateral_velocity;
    force_input.spin_velocity_radians_per_second = wheel->spinVel;
    force_input.camber_radians = wheel->relPos.ax;
    force_input.contact_fraction = contact_output.contact_fraction;
    force_input.contact_patch_ratio = contact_output.contact_patch_meters / fmax(base_contact_patch, 0.03);
    force_input.load_sensitivity_scale = contact_output.load_sensitivity_scale;
    force_input.thermal_grip_scale = modifiers.grip_scale;
    force_input.wear_grip_scale = state.wear.wear_grip_scale;
    force_input.external_grip_scale = wet_scale;
    force_input.force_stiffness_scale = modifiers.force_stiffness_scale;
    force_input.relaxation_length_scale = modifiers.relaxation_length_scale;
    force_input.pneumatic_trail_scale = modifiers.pneumatic_trail_scale;
    force_input.rolling_resistance_scale = modifiers.rolling_resistance_scale;
    force_input.elapsed_seconds = elapsed;
    force_input.wheel_right = index % 2 == 0 ? 1 : 0;
    TireForceOutput force_output;
    TireForceModelEvaluate(state.force, configuration.force[axle], force_input, force_output);

    wheel->forces.x = (tdble)(force_output.longitudinal_force_newtons * cos_angle
        - force_output.lateral_force_newtons * sin_angle);
    wheel->forces.y = (tdble)(force_output.longitudinal_force_newtons * sin_angle
        + force_output.lateral_force_newtons * cos_angle);
    wheel->spinTq = (tdble)force_output.wheel_reaction_torque_newton_meters;
    wheel->torqueAlign = (tdble)force_output.aligning_torque_newton_meters;
    wheel->rollRes = (tdble)force_output.rolling_resistance_force_newtons;
    wheel->sa = (tdble)force_output.slip_angle_radians;
    wheel->sx = (tdble)force_output.slip_ratio;
    wheel->tireSlip = (tdble)ClampValue(sqrt(force_output.slip_ratio * force_output.slip_ratio
        + sin(force_output.slip_angle_radians) * sin(force_output.slip_angle_radians)), 0.0, 150.0);
    wheel->feedBack.spinVel = wheel->spinVel;
    wheel->feedBack.Tq = wheel->spinTq;
    wheel->feedBack.brkTq = wheel->brake.Tq;

    car->carElt->_wheelFx(index) = wheel->forces.x;
    car->carElt->_wheelFy(index) = wheel->forces.y;
    car->carElt->_wheelFz(index) = wheel->forces.z;
    car->carElt->_wheelSlipNorm(index) = wheel->tireSlip;
    car->carElt->_wheelSlipSide(index) = (tdble)(sin(force_output.slip_angle_radians) * vehicle_speed);
    car->carElt->_wheelSlipAccel(index) = (tdble)(force_output.slip_ratio * vehicle_speed);
    car->carElt->_reaction[index] = wheel->forces.z;
    car->carElt->_tyreEffMu(index) = (tdble)(wheel->forces.z > 1.0
        ? force_output.lateral_capacity_newtons / wheel->forces.z : 0.0);
    if (vehicle_speed < 2.0)
    {
        car->carElt->_skid[index] = 0.0;
    }
    else
    {
        car->carElt->_skid[index] = (tdble)ClampValue(wheel->tireSlip * wheel->forces.z * 0.0002,
            0.0, 1.0);
    }
    car->carElt->priv.wheel[index].rollRes = wheel->rollRes;

    double physical_camber = PhysicalCamber(wheel, index);
    double raw_zone_weights[3] = { 0.0, 0.0, 0.0 };
    if (wheel->formula_tire_model_grounded)
    {
        raw_zone_weights[0] = 1.0;
        raw_zone_weights[1] = 2.0;
        raw_zone_weights[2] = 1.0;
    }
    double normalized_zone_weights[3];
    double gauge_pressure = state.thermal.pressure_absolute_pascals
        - configuration.pressure.ambient_absolute_pascals;
    double pressure_ratio = ClampValue(gauge_pressure
        / fmax(configuration.pressure.reference_hot_gauge_pascals[index], 20000.0), 0.45, 1.80);
    TireThermalZoneWeights(raw_zone_weights, pressure_ratio, physical_camber,
        normalized_zone_weights);
    wheel->formula_tire_model_zone_weights[0] = normalized_zone_weights[0];
    wheel->formula_tire_model_zone_weights[1] = normalized_zone_weights[1];
    wheel->formula_tire_model_zone_weights[2] = normalized_zone_weights[2];

    double track_temperature = Ttrack > 250.0 ? (double)Ttrack
        : configuration.thermal[axle].track_fallback_celsius + 273.15;
    TireThermalInput thermal_input;
    thermal_input.normal_force_newtons = wheel->forces.z;
    thermal_input.longitudinal_force_newtons = force_output.longitudinal_force_newtons;
    thermal_input.lateral_force_newtons = force_output.lateral_force_newtons;
    thermal_input.slip_lateral_force_newtons = force_output.slip_lateral_force_newtons;
    thermal_input.slip_power_watts = force_output.slip_power_watts;
    thermal_input.slip_velocity_longitudinal_meters_per_second = force_output.slip_velocity_longitudinal_meters_per_second;
    thermal_input.slip_velocity_lateral_meters_per_second = force_output.slip_velocity_lateral_meters_per_second;
    thermal_input.deflection_meters = contact_output.deflection_meters;
    thermal_input.deflection_velocity_meters_per_second = contact_output.deflection_velocity_meters_per_second;
    thermal_input.maximum_deflection_meters = contact_output.maximum_deflection_meters;
    thermal_input.vehicle_speed_meters_per_second = vehicle_speed;
    thermal_input.ambient_temperature_kelvin = Tair;
    thermal_input.track_temperature_kelvin = track_temperature;
    thermal_input.camber_radians = physical_camber;
    thermal_input.contact_fraction = contact_output.contact_fraction;
    thermal_input.brake_carcass_heat_watts = 0.0;
    thermal_input.brake_gas_heat_watts = 0.0;
    thermal_input.zone_contact_weights[0] = raw_zone_weights[0];
    thermal_input.zone_contact_weights[1] = raw_zone_weights[1];
    thermal_input.zone_contact_weights[2] = raw_zone_weights[2];
    TireThermalModelUpdate(state.thermal, configuration.thermal[axle], configuration.pressure,
        index, thermal_input, elapsed);

    TireWearInput wear_input;
    wear_input.friction_power_watts = force_output.slip_power_watts;
    wear_input.zone_contact_weights[0] = normalized_zone_weights[0];
    wear_input.zone_contact_weights[1] = normalized_zone_weights[1];
    wear_input.zone_contact_weights[2] = normalized_zone_weights[2];
    wear_input.tread_inner_kelvin = state.thermal.tread_inner_kelvin;
    wear_input.tread_center_kelvin = state.thermal.tread_center_kelvin;
    wear_input.tread_outer_kelvin = state.thermal.tread_outer_kelvin;
    wear_input.surface_friction_coefficient = wheel->ground_friction;
    wear_input.elapsed_seconds = elapsed;
    TireWearModelUpdate(state.wear, configuration.wear[axle], wear_input);

    wheel->Ttire = (tdble)((state.thermal.tread_inner_kelvin + state.thermal.tread_center_kelvin
        + state.thermal.tread_outer_kelvin) / 3.0);
    wheel->currentTemperature = wheel->Ttire;
    wheel->currentPressure = (tdble)state.thermal.pressure_absolute_pascals;
    wheel->currentWear = (tdble)(1.0 - RemainingTreadFraction(state));
    wheel->currentGraining = 0.0;
    wheel->currentGripFactor = (tdble)ClampValue(state.wear.wear_grip_scale
        * TireThermalGripScale(wheel->Ttire, configuration.thermal[axle]), 0.0, 1.0);
    wheel->treadDepth = (tdble)RemainingTreadFraction(state);
    wheel->camber = (tdble)physical_camber;
    wheel->Topt = (tdble)(configuration.thermal[axle].optimal_tread_temperature_celsius + 273.15);
    wheel->hysteresis_energy = 0.0;
    wheel->lateral_heating_energy = (tdble)force_output.slip_power_watts;
    wheel->longitudinal_heating_energy = 0.0;
    wheel->convection_energy = (tdble)state.thermal.last_external_power_watts;
    wheel->additional_cooling_energy = 0.0;
    wheel->thermal_capacity = (tdble)(3.0 * configuration.thermal[axle]
        .tread_zone_heat_capacity_joules_per_kelvin
        + configuration.thermal[axle].carcass_heat_capacity_joules_per_kelvin
        + configuration.thermal[axle].gas_heat_capacity_joules_per_kelvin);
    SimTireModelPublishWheel(car, index);
}
