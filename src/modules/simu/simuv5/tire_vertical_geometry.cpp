/***************************************************************************

    file                 : tire_vertical_geometry.cpp
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

#include <algorithm>
#include <cmath>

#include "tire_vertical_geometry.h"

void TireVerticalChassisReferenceCompute(const TireVerticalChassisTransform& chassis,
    double& reference_height_meters, double& reference_height_velocity_meters_per_second)
{
    reference_height_meters = chassis.center_of_gravity_world_height_meters
        - chassis.center_of_gravity_local_height_meters
        - chassis.wheel_x_meters * std::sin(chassis.pitch_radians)
        + chassis.wheel_y_meters * std::sin(chassis.roll_radians);

    reference_height_velocity_meters_per_second = chassis.center_of_gravity_world_height_velocity_meters_per_second
        - chassis.wheel_x_meters * std::cos(chassis.pitch_radians) * chassis.pitch_velocity_radians_per_second
        + chassis.wheel_y_meters * std::cos(chassis.roll_radians) * chassis.roll_velocity_radians_per_second;
}

void TireVerticalGeometryCompute(const TireVerticalGeometryInput& input, TireVerticalGeometryOutput& output)
{
    TireVerticalChassisReferenceCompute(input.chassis,
        output.chassis_reference_height_meters,
        output.chassis_reference_height_velocity_meters_per_second);

    output.chassis_reference_ground_clearance_meters =
        output.chassis_reference_height_meters - input.ground_height_meters;
    output.chassis_reference_ground_clearance_velocity_meters_per_second =
        output.chassis_reference_height_velocity_meters_per_second
        - input.ground_height_velocity_meters_per_second;

    output.wheel_center_height_meters = output.chassis_reference_height_meters
        + input.tire_unloaded_radius_meters
        - input.suspension_extension_meters;
    output.wheel_center_velocity_meters_per_second =
        output.chassis_reference_height_velocity_meters_per_second
        + input.tire_unloaded_radius_velocity_meters_per_second
        - input.suspension_extension_velocity_meters_per_second;

    output.tire_deflection_signed_meters =
        input.suspension_extension_meters
        - output.chassis_reference_ground_clearance_meters;
    output.tire_deflection_meters = std::max(0.0, output.tire_deflection_signed_meters);
    output.tire_deflection_velocity_meters_per_second =
        input.ground_height_velocity_meters_per_second
        + input.suspension_extension_velocity_meters_per_second
        - output.chassis_reference_height_velocity_meters_per_second;

    output.suspension_extension_meters = input.suspension_extension_meters;
    output.suspension_extension_velocity_meters_per_second =
        input.suspension_extension_velocity_meters_per_second;
    output.suspension_position_legacy_meters =
        input.bellcrank_ratio * input.suspension_extension_meters;
    output.suspension_velocity_legacy_meters_per_second =
        -input.bellcrank_ratio * input.suspension_extension_velocity_meters_per_second;
    output.suspension_acceleration_legacy_meters_per_second_squared =
        input.bellcrank_ratio * input.suspension_extension_acceleration_meters_per_second_squared;
    output.suspension_compression_acceleration_meters_per_second_squared =
        -input.bellcrank_ratio * input.suspension_extension_acceleration_meters_per_second_squared;

    output.geometric_contact = output.tire_deflection_signed_meters >= 0.0;
}
