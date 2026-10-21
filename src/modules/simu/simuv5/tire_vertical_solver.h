/***************************************************************************

    file                 : tire_vertical_solver.h
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

#ifndef TIRE_VERTICAL_SOLVER_H
#define TIRE_VERTICAL_SOLVER_H

struct TireVerticalSpringConfiguration
{
    double spring_constant_newtons_per_meter;
    double preload_newtons;
    double reference_position_meters;
    double minimum_position_meters;
    double maximum_position_meters;
    double bellcrank_ratio;
    double inertance_kilograms;
};

struct TireVerticalDamperSegmentConfiguration
{
    double slow_coefficient_newtons_per_second_per_meter;
    double fast_coefficient_newtons_per_second_per_meter;
    double transition_velocity_meters_per_second;
    double slow_intercept_newtons;
    double fast_intercept_newtons;
};

struct TireVerticalDamperConfiguration
{
    TireVerticalDamperSegmentConfiguration bump;
    TireVerticalDamperSegmentConfiguration rebound;
    double efficiency;
};

struct TireVerticalCornerConfiguration
{
    TireVerticalSpringConfiguration spring;
    TireVerticalDamperConfiguration damper;
    double tire_radial_stiffness_newtons_per_meter;
    double tire_radial_damping_newtons_per_second_per_meter;
    double unloaded_radius_meters;
};

struct TireVerticalAxleConfiguration
{
    double anti_roll_stiffness_newtons_per_meter;
    int anti_roll_enabled;
    TireVerticalSpringConfiguration heave_spring;
    TireVerticalDamperConfiguration heave_damper;
    int heave_enabled;
};

struct TireVerticalSolverConfiguration
{
    double total_mass_kilograms;
    double roll_inertia_kilogram_meters_squared;
    double pitch_inertia_kilogram_meters_squared;
    double gravity_meters_per_second_squared;
    double wheel_x_meters[4];
    double wheel_y_meters[4];
    double unsprung_mass_kilograms[4];
    TireVerticalCornerConfiguration corner[4];
    TireVerticalAxleConfiguration axle[2];
    double external_body_generalized_force[3];
    double external_wheel_generalized_force[4];
};

struct TireVerticalSolverState
{
    double body_position[3];
    double body_velocity[3];
    double suspension_extension_meters[4];
    double suspension_extension_velocity_meters_per_second[4];
};

struct TireVerticalSolverStepInput
{
    double elapsed_seconds;
    double ground_height_meters[4];
    double ground_height_velocity_meters_per_second[4];
};

enum TireVerticalConstraintKind
{
    TIRE_VERTICAL_CONSTRAINT_NONE = 0,
    TIRE_VERTICAL_CONSTRAINT_LOWER = 1,
    TIRE_VERTICAL_CONSTRAINT_UPPER = 2
};

struct TireVerticalSolverDiagnostics
{
    int converged;
    int newton_iterations;
    double residual_norm;
    int active_constraint_count;
    int active_constraint_wheel[8];
    int active_constraint_kind[8];
    double constraint_impulse_newton_seconds[8];
    double constraint_energy_loss_joules;
    double damper_energy_loss_joules;
    double tire_damping_energy_loss_joules;
    double tire_normal_force_newtons[4];
    double suspension_force_newtons[4];
    double tire_deflection_meters[4];
    double wheel_center_height_meters[4];
    int tire_contact[4];
    double mass_matrix[7][7];
    int mass_matrix_positive_definite;
    int sprung_mass_matrix_positive_definite;
};

struct TireVerticalQuarterCarConfiguration
{
    double sprung_mass_kilograms;
    double unsprung_mass_kilograms;
    double gravity_meters_per_second_squared;
    TireVerticalCornerConfiguration corner;
};

struct TireVerticalQuarterCarState
{
    double body_position_meters;
    double body_velocity_meters_per_second;
    double suspension_extension_meters;
    double suspension_extension_velocity_meters_per_second;
};

struct TireVerticalQuarterCarStepInput
{
    double elapsed_seconds;
    double ground_height_meters;
    double ground_height_velocity_meters_per_second;
};

struct TireVerticalQuarterCarDiagnostics
{
    int converged;
    int newton_iterations;
    double residual_norm;
    double tire_normal_force_newtons;
    double suspension_force_newtons;
    double tire_deflection_meters;
    double wheel_center_height_meters;
    int tire_contact;
    int lower_limit_active;
    int upper_limit_active;
    double limit_impulse_newton_seconds;
    double constraint_energy_loss_joules;
    double damper_energy_loss_joules;
    double tire_damping_energy_loss_joules;
};

double TireVerticalSpringForce(const TireVerticalSpringConfiguration& configuration, double position_meters);
double TireVerticalSpringPotentialJoules(const TireVerticalSpringConfiguration& configuration, double position_meters);
double TireVerticalDamperForce(const TireVerticalDamperConfiguration& configuration, double velocity_meters_per_second);
double TireVerticalDamperSlope(const TireVerticalDamperConfiguration& configuration, double velocity_meters_per_second);
double TireVerticalCornerSuspensionForce(const TireVerticalCornerConfiguration& configuration,
    double extension_meters, double extension_velocity_meters_per_second);

void TireVerticalSuspensionForces(const TireVerticalSolverConfiguration& configuration,
    const double suspension_extension_meters[4],
    const double suspension_extension_velocity_meters_per_second[4],
    double suspension_force_newtons[4]);

int TireVerticalSolverConfigurationValidate(const TireVerticalSolverConfiguration& configuration,
    char* error_message, int error_message_size);

int TireVerticalSolverInitializeEquilibrium(const TireVerticalSolverConfiguration& configuration,
    const double ground_height_meters[4], TireVerticalSolverState& state,
    TireVerticalSolverDiagnostics& diagnostics);

int TireVerticalSolverStep(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& previous_state, const TireVerticalSolverStepInput& input,
    TireVerticalSolverState& next_state, TireVerticalSolverDiagnostics& diagnostics);

double TireVerticalSystemEnergyJoules(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const double ground_height_meters[4]);

double TireVerticalJacobianError(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const TireVerticalSolverStepInput& input);

int TireVerticalQuarterCarInitializeEquilibrium(const TireVerticalQuarterCarConfiguration& configuration,
    double ground_height_meters, TireVerticalQuarterCarState& state,
    TireVerticalQuarterCarDiagnostics& diagnostics);

int TireVerticalQuarterCarStep(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& previous_state, const TireVerticalQuarterCarStepInput& input,
    TireVerticalQuarterCarState& next_state, TireVerticalQuarterCarDiagnostics& diagnostics);

double TireVerticalQuarterCarEnergyJoules(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& state, double ground_height_meters);

#endif
