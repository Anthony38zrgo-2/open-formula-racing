/***************************************************************************

    file                 : tire_vertical_geometry.h
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

#ifndef TIRE_VERTICAL_GEOMETRY_H
#define TIRE_VERTICAL_GEOMETRY_H

struct TireVerticalChassisTransform
{
    double center_of_gravity_world_height_meters;
    double center_of_gravity_world_height_velocity_meters_per_second;
    double center_of_gravity_local_height_meters;
    double wheel_x_meters;
    double wheel_y_meters;
    double pitch_radians;
    double pitch_velocity_radians_per_second;
    double roll_radians;
    double roll_velocity_radians_per_second;
};

struct TireVerticalGeometryInput
{
    TireVerticalChassisTransform chassis;
    double ground_height_meters;
    double ground_height_velocity_meters_per_second;
    double tire_unloaded_radius_meters;
    double tire_unloaded_radius_velocity_meters_per_second;
    double bellcrank_ratio;
    double suspension_extension_meters;
    double suspension_extension_velocity_meters_per_second;
    double suspension_extension_acceleration_meters_per_second_squared;
};

struct TireVerticalGeometryOutput
{
    double chassis_reference_height_meters;
    double chassis_reference_height_velocity_meters_per_second;
    double chassis_reference_ground_clearance_meters;
    double chassis_reference_ground_clearance_velocity_meters_per_second;
    double wheel_center_height_meters;
    double wheel_center_velocity_meters_per_second;
    double tire_deflection_signed_meters;
    double tire_deflection_meters;
    double tire_deflection_velocity_meters_per_second;
    double suspension_extension_meters;
    double suspension_extension_velocity_meters_per_second;
    double suspension_position_legacy_meters;
    double suspension_velocity_legacy_meters_per_second;
    double suspension_acceleration_legacy_meters_per_second_squared;
    double suspension_compression_acceleration_meters_per_second_squared;
    bool geometric_contact;
};

void TireVerticalChassisReferenceCompute(const TireVerticalChassisTransform& chassis,
    double& reference_height_meters, double& reference_height_velocity_meters_per_second);

void TireVerticalGeometryCompute(const TireVerticalGeometryInput& input, TireVerticalGeometryOutput& output);

#endif
