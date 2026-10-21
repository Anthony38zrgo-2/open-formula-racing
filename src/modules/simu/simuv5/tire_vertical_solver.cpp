/***************************************************************************

    file                 : tire_vertical_solver.cpp
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
#include <cstdio>
#include <cstring>

#include "tire_vertical_solver.h"

static const int TireVerticalSystemSize = 7;
static const int TireVerticalMaximumConstraintCount = 4;
static const int TireVerticalMaximumSystemSize = TireVerticalSystemSize + TireVerticalMaximumConstraintCount;
static const int TireVerticalMaximumNewtonIterations = 60;
static const int TireVerticalMaximumActiveSetIterations = 8;
static const int TireVerticalMaximumLineSearchIterations = 10;
static const double TireVerticalResidualTolerance = 1.0e-7;
static const double TireVerticalLimitToleranceMeters = 1.0e-9;
static const double TireVerticalMultiplierTolerance = 1.0e-7;
static const double TireVerticalDamperVelocityLimit = 10.0;

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

static double VectorNorm(const double* values, int count)
{
    double sum = 0.0;
    for (int index = 0; index < count; ++index)
    {
        sum += values[index] * values[index];
    }
    return std::sqrt(sum);
}

double TireVerticalSpringForce(const TireVerticalSpringConfiguration& configuration, double position_meters)
{
    const double force = configuration.spring_constant_newtons_per_meter
        * (position_meters - configuration.reference_position_meters)
        + configuration.preload_newtons;
    return force > 0.0 ? force : 0.0;
}

double TireVerticalSpringPotentialJoules(const TireVerticalSpringConfiguration& configuration, double position_meters)
{
    const double spring_constant = configuration.spring_constant_newtons_per_meter;
    const double displacement = position_meters - configuration.reference_position_meters;
    if (spring_constant == 0.0)
    {
        if (configuration.preload_newtons <= 0.0)
        {
            return 0.0;
        }
        return -configuration.preload_newtons * displacement;
    }

    const double free_displacement = -configuration.preload_newtons / spring_constant;
    const double force = spring_constant * displacement + configuration.preload_newtons;
    if (force <= 0.0)
    {
        return 0.0;
    }

    return -(0.5 * spring_constant
        * (displacement * displacement - free_displacement * free_displacement)
        + configuration.preload_newtons * (displacement - free_displacement));
}

double TireVerticalDamperForce(const TireVerticalDamperConfiguration& configuration,
    double velocity_meters_per_second)
{
    double velocity = ClampValue(velocity_meters_per_second,
        -TireVerticalDamperVelocityLimit, TireVerticalDamperVelocityLimit);
    const TireVerticalDamperSegmentConfiguration& segment =
        velocity < 0.0 ? configuration.rebound : configuration.bump;
    const double magnitude = std::fabs(velocity);
    const double force = magnitude < segment.transition_velocity_meters_per_second
        ? segment.slow_coefficient_newtons_per_second_per_meter * magnitude
            + segment.slow_intercept_newtons
        : segment.fast_coefficient_newtons_per_second_per_meter * magnitude
            + segment.fast_intercept_newtons;
    return force * SignValue(velocity) * configuration.efficiency;
}

double TireVerticalDamperSlope(const TireVerticalDamperConfiguration& configuration,
    double velocity_meters_per_second)
{
    if (std::fabs(velocity_meters_per_second) >= TireVerticalDamperVelocityLimit)
    {
        return 0.0;
    }
    const TireVerticalDamperSegmentConfiguration& segment =
        velocity_meters_per_second < 0.0 ? configuration.rebound : configuration.bump;
    const double magnitude = std::fabs(velocity_meters_per_second);
    const double coefficient = magnitude < segment.transition_velocity_meters_per_second
        ? segment.slow_coefficient_newtons_per_second_per_meter
        : segment.fast_coefficient_newtons_per_second_per_meter;
    return coefficient * configuration.efficiency;
}

double TireVerticalCornerSuspensionForce(const TireVerticalCornerConfiguration& configuration,
    double extension_meters, double extension_velocity_meters_per_second)
{
    const double bellcrank = configuration.spring.bellcrank_ratio;
    const double position = bellcrank * extension_meters;
    const double velocity = -bellcrank * extension_velocity_meters_per_second;
    return bellcrank * (TireVerticalSpringForce(configuration.spring, position)
        + TireVerticalDamperForce(configuration.damper, velocity));
}

struct TireVerticalSuspensionEvaluation
{
    double force_newtons[4];
    double force_derivative_extension[4][4];
    double force_derivative_extension_velocity[4][4];
    double damper_generalized_force[4];
    double damper_velocity[4];
    double damper_slope[4];
};

static void EvaluateSuspension(const TireVerticalSolverConfiguration& configuration,
    const double suspension_extension_meters[4],
    const double suspension_extension_velocity_meters_per_second[4],
    TireVerticalSuspensionEvaluation& evaluation)
{
    std::memset(&evaluation, 0, sizeof(evaluation));

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const TireVerticalCornerConfiguration& corner = configuration.corner[wheel];
        const double bellcrank = corner.spring.bellcrank_ratio;
        const double position = bellcrank * suspension_extension_meters[wheel];
        const double velocity = -bellcrank * suspension_extension_velocity_meters_per_second[wheel];
        const double spring_force = TireVerticalSpringForce(corner.spring, position);
        const double damper_force = TireVerticalDamperForce(corner.damper, velocity);
        const double damper_slope = TireVerticalDamperSlope(corner.damper, velocity);

        evaluation.force_newtons[wheel] += bellcrank * (spring_force + damper_force);
        evaluation.damper_generalized_force[wheel] = bellcrank * damper_force;
        evaluation.damper_velocity[wheel] = velocity;
        evaluation.damper_slope[wheel] = damper_slope;

        if (spring_force > 0.0)
        {
            evaluation.force_derivative_extension[wheel][wheel] +=
                bellcrank * bellcrank * corner.spring.spring_constant_newtons_per_meter;
        }
        evaluation.force_derivative_extension_velocity[wheel][wheel] +=
            -bellcrank * bellcrank * damper_slope;
    }

    for (int axle = 0; axle < 2; ++axle)
    {
        const int right_wheel = 2 * axle;
        const int left_wheel = 2 * axle + 1;
        const TireVerticalAxleConfiguration& axle_configuration = configuration.axle[axle];
        const double right_bellcrank =
            configuration.corner[right_wheel].spring.bellcrank_ratio;
        const double left_bellcrank =
            configuration.corner[left_wheel].spring.bellcrank_ratio;

        if (axle_configuration.anti_roll_enabled)
        {
            const double stiffness = axle_configuration.anti_roll_stiffness_newtons_per_meter;
            const double difference = left_bellcrank * suspension_extension_meters[left_wheel]
                - right_bellcrank * suspension_extension_meters[right_wheel];

            evaluation.force_newtons[left_wheel] -=
                stiffness * difference * left_bellcrank;
            evaluation.force_newtons[right_wheel] +=
                stiffness * difference * right_bellcrank;

            evaluation.force_derivative_extension[left_wheel][left_wheel] -=
                stiffness * left_bellcrank * left_bellcrank;
            evaluation.force_derivative_extension[left_wheel][right_wheel] +=
                stiffness * left_bellcrank * right_bellcrank;
            evaluation.force_derivative_extension[right_wheel][left_wheel] +=
                stiffness * right_bellcrank * left_bellcrank;
            evaluation.force_derivative_extension[right_wheel][right_wheel] -=
                stiffness * right_bellcrank * right_bellcrank;
        }

        if (axle_configuration.heave_enabled)
        {
            const TireVerticalSpringConfiguration& heave_spring =
                axle_configuration.heave_spring;
            const double heave_bellcrank = heave_spring.bellcrank_ratio;
            const double heave_position = 0.5
                * (left_bellcrank * suspension_extension_meters[left_wheel]
                    + right_bellcrank * suspension_extension_meters[right_wheel]);
            const double heave_velocity = -0.5
                * (left_bellcrank * suspension_extension_velocity_meters_per_second[left_wheel]
                    + right_bellcrank * suspension_extension_velocity_meters_per_second[right_wheel]);
            const double heave_spring_force =
                TireVerticalSpringForce(heave_spring, heave_position);
            const double heave_damper_force =
                TireVerticalDamperForce(axle_configuration.heave_damper, heave_velocity);
            const double heave_damper_slope =
                TireVerticalDamperSlope(axle_configuration.heave_damper, heave_velocity);
            const double heave_force = heave_spring_force + heave_damper_force;

            evaluation.force_newtons[left_wheel] +=
                0.5 * heave_bellcrank * left_bellcrank * heave_force;
            evaluation.force_newtons[right_wheel] +=
                0.5 * heave_bellcrank * right_bellcrank * heave_force;

            if (heave_spring_force > 0.0)
            {
                const double spring_derivative = 0.25 * heave_bellcrank
                    * heave_spring.spring_constant_newtons_per_meter;
                evaluation.force_derivative_extension[left_wheel][left_wheel] +=
                    spring_derivative * left_bellcrank * left_bellcrank;
                evaluation.force_derivative_extension[left_wheel][right_wheel] +=
                    spring_derivative * left_bellcrank * right_bellcrank;
                evaluation.force_derivative_extension[right_wheel][left_wheel] +=
                    spring_derivative * right_bellcrank * left_bellcrank;
                evaluation.force_derivative_extension[right_wheel][right_wheel] +=
                    spring_derivative * right_bellcrank * right_bellcrank;
            }

            const double damper_derivative = 0.25 * heave_bellcrank * heave_damper_slope;
            evaluation.force_derivative_extension_velocity[left_wheel][left_wheel] -=
                damper_derivative * left_bellcrank * left_bellcrank;
            evaluation.force_derivative_extension_velocity[left_wheel][right_wheel] -=
                damper_derivative * left_bellcrank * right_bellcrank;
            evaluation.force_derivative_extension_velocity[right_wheel][left_wheel] -=
                damper_derivative * right_bellcrank * left_bellcrank;
            evaluation.force_derivative_extension_velocity[right_wheel][right_wheel] -=
                damper_derivative * right_bellcrank * right_bellcrank;
        }
    }
}

void TireVerticalSuspensionForces(const TireVerticalSolverConfiguration& configuration,
    const double suspension_extension_meters[4],
    const double suspension_extension_velocity_meters_per_second[4],
    double suspension_force_newtons[4])
{
    TireVerticalSuspensionEvaluation evaluation;
    EvaluateSuspension(configuration, suspension_extension_meters,
        suspension_extension_velocity_meters_per_second, evaluation);
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        suspension_force_newtons[wheel] = evaluation.force_newtons[wheel];
    }
}

static void BuildMassMatrix(const TireVerticalSolverConfiguration& configuration, double mass_matrix[7][7])
{
    std::memset(mass_matrix, 0, sizeof(double) * 49);
    mass_matrix[0][0] = configuration.total_mass_kilograms;
    mass_matrix[1][1] = configuration.roll_inertia_kilogram_meters_squared;
    mass_matrix[2][2] = configuration.pitch_inertia_kilogram_meters_squared;

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double jacobian[3] = {
            1.0, configuration.wheel_y_meters[wheel], -configuration.wheel_x_meters[wheel]
        };
        const double mass = configuration.unsprung_mass_kilograms[wheel];
        for (int row = 0; row < 3; ++row)
        {
            mass_matrix[row][3 + wheel] = -mass * jacobian[row];
            mass_matrix[3 + wheel][row] = -mass * jacobian[row];
        }
        mass_matrix[3 + wheel][3 + wheel] = mass;
        const double bellcrank = configuration.corner[wheel].spring.bellcrank_ratio;
        mass_matrix[3 + wheel][3 + wheel] -=
            bellcrank * bellcrank * configuration.corner[wheel].spring.inertance_kilograms;
    }
}

static int CholeskyPositiveDefinite(const double matrix[7][7])
{
    double lower[7][7] = {};
    for (int row = 0; row < 7; ++row)
    {
        for (int column = 0; column <= row; ++column)
        {
            double sum = matrix[row][column];
            for (int inner = 0; inner < column; ++inner)
            {
                sum -= lower[row][inner] * lower[column][inner];
            }
            if (row == column)
            {
                if (sum <= 0.0)
                {
                    return 0;
                }
                lower[row][column] = std::sqrt(sum);
            }
            else
            {
                lower[row][column] = sum / lower[column][column];
            }
        }
    }
    return 1;
}

static void BuildSprungMassMatrix(const TireVerticalSolverConfiguration& configuration,
    double sprung_mass_matrix[3][3])
{
    std::memset(sprung_mass_matrix, 0, sizeof(double) * 9);
    sprung_mass_matrix[0][0] = configuration.total_mass_kilograms;
    sprung_mass_matrix[1][1] = configuration.roll_inertia_kilogram_meters_squared;
    sprung_mass_matrix[2][2] = configuration.pitch_inertia_kilogram_meters_squared;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double jacobian[3] = {
            1.0, configuration.wheel_y_meters[wheel], -configuration.wheel_x_meters[wheel]
        };
        const double mass = configuration.unsprung_mass_kilograms[wheel];
        for (int row = 0; row < 3; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                sprung_mass_matrix[row][column] -= mass * jacobian[row] * jacobian[column];
            }
        }
    }
}

static int CholeskyPositiveDefinite3(const double matrix[3][3])
{
    double lower[3][3] = {};
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column <= row; ++column)
        {
            double sum = matrix[row][column];
            for (int inner = 0; inner < column; ++inner)
            {
                sum -= lower[row][inner] * lower[column][inner];
            }
            if (row == column)
            {
                if (sum <= 0.0)
                {
                    return 0;
                }
                lower[row][column] = std::sqrt(sum);
            }
            else
            {
                lower[row][column] = sum / lower[column][column];
            }
        }
    }
    return 1;
}

static int SolveLinearSystem(const double* matrix, int stride, const double* right_hand_side,
    double* solution, int size)
{
    double augmented[TireVerticalMaximumSystemSize][TireVerticalMaximumSystemSize] = {};
    double values[TireVerticalMaximumSystemSize] = {};
    for (int row = 0; row < size; ++row)
    {
        for (int column = 0; column < size; ++column)
        {
            augmented[row][column] = matrix[row * stride + column];
        }
        values[row] = right_hand_side[row];
    }

    for (int column = 0; column < size; ++column)
    {
        int pivot = column;
        double largest = std::fabs(augmented[column][column]);
        for (int row = column + 1; row < size; ++row)
        {
            const double candidate = std::fabs(augmented[row][column]);
            if (candidate > largest)
            {
                largest = candidate;
                pivot = row;
            }
        }
        if (largest < 1.0e-14)
        {
            return 0;
        }
        if (pivot != column)
        {
            for (int inner = column; inner < size; ++inner)
            {
                std::swap(augmented[column][inner], augmented[pivot][inner]);
            }
            std::swap(values[column], values[pivot]);
        }
        for (int row = column + 1; row < size; ++row)
        {
            const double factor = augmented[row][column] / augmented[column][column];
            if (factor == 0.0)
            {
                continue;
            }
            for (int inner = column; inner < size; ++inner)
            {
                augmented[row][inner] -= factor * augmented[column][inner];
            }
            values[row] -= factor * values[column];
        }
    }

    for (int row = size - 1; row >= 0; --row)
    {
        double sum = values[row];
        for (int column = row + 1; column < size; ++column)
        {
            sum -= augmented[row][column] * solution[column];
        }
        solution[row] = sum / augmented[row][row];
    }
    return 1;
}

struct TireVerticalForceEvaluation
{
    double generalized_force[7];
    double jacobian_position[7][7];
    double jacobian_velocity[7][7];
    double tire_normal_force_newtons[4];
    double suspension_force_newtons[4];
    double tire_deflection_meters[4];
    double tire_deflection_velocity_meters_per_second[4];
    double wheel_center_height_meters[4];
    int tire_contact[4];
};

static void EvaluateForces(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const TireVerticalSolverStepInput& input,
    TireVerticalForceEvaluation& evaluation)
{
    std::memset(&evaluation, 0, sizeof(evaluation));

    TireVerticalSuspensionEvaluation suspension;
    EvaluateSuspension(configuration, state.suspension_extension_meters,
        state.suspension_extension_velocity_meters_per_second, suspension);

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double jacobian[3] = {
            1.0, configuration.wheel_y_meters[wheel], -configuration.wheel_x_meters[wheel]
        };
        const double chassis_reference_height = jacobian[0] * state.body_position[0]
            + jacobian[1] * state.body_position[1]
            + jacobian[2] * state.body_position[2];
        const double chassis_reference_velocity = jacobian[0] * state.body_velocity[0]
            + jacobian[1] * state.body_velocity[1]
            + jacobian[2] * state.body_velocity[2];
        const double clearance = chassis_reference_height - input.ground_height_meters[wheel];
        const double deflection = state.suspension_extension_meters[wheel] - clearance;
        const double deflection_velocity = input.ground_height_velocity_meters_per_second[wheel]
            + state.suspension_extension_velocity_meters_per_second[wheel]
            - chassis_reference_velocity;

        double normal_force = 0.0;
        double normal_derivative_position[7] = {};
        double normal_derivative_velocity[7] = {};
        const TireVerticalCornerConfiguration& corner = configuration.corner[wheel];
        if (deflection > 0.0)
        {
            const double linear_force = corner.tire_radial_stiffness_newtons_per_meter * deflection
                + corner.tire_radial_damping_newtons_per_second_per_meter * deflection_velocity;
            if (linear_force > 0.0)
            {
                normal_force = linear_force;
                evaluation.tire_contact[wheel] = 1;
                for (int body = 0; body < 3; ++body)
                {
                    normal_derivative_position[body] =
                        -corner.tire_radial_stiffness_newtons_per_meter * jacobian[body];
                    normal_derivative_velocity[body] =
                        -corner.tire_radial_damping_newtons_per_second_per_meter * jacobian[body];
                }
                normal_derivative_position[3 + wheel] =
                    corner.tire_radial_stiffness_newtons_per_meter;
                normal_derivative_velocity[3 + wheel] =
                    corner.tire_radial_damping_newtons_per_second_per_meter;
            }
        }

        evaluation.tire_normal_force_newtons[wheel] = normal_force;
        evaluation.suspension_force_newtons[wheel] = suspension.force_newtons[wheel];
        evaluation.tire_deflection_meters[wheel] = deflection;
        evaluation.tire_deflection_velocity_meters_per_second[wheel] = deflection_velocity;
        evaluation.wheel_center_height_meters[wheel] = chassis_reference_height
            + corner.unloaded_radius_meters - state.suspension_extension_meters[wheel];

        for (int body = 0; body < 3; ++body)
        {
            evaluation.generalized_force[body] += jacobian[body] * normal_force;
            for (int column = 0; column < 7; ++column)
            {
                evaluation.jacobian_position[body][column] +=
                    jacobian[body] * normal_derivative_position[column];
                evaluation.jacobian_velocity[body][column] +=
                    jacobian[body] * normal_derivative_velocity[column];
            }
        }
        evaluation.generalized_force[3 + wheel] -= normal_force;
        for (int column = 0; column < 7; ++column)
        {
            evaluation.jacobian_position[3 + wheel][column] -=
                normal_derivative_position[column];
            evaluation.jacobian_velocity[3 + wheel][column] -=
                normal_derivative_velocity[column];
        }
    }

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        evaluation.generalized_force[3 + wheel] += suspension.force_newtons[wheel];
        for (int column = 0; column < 4; ++column)
        {
            evaluation.jacobian_position[3 + wheel][3 + column] +=
                suspension.force_derivative_extension[wheel][column];
            evaluation.jacobian_velocity[3 + wheel][3 + column] +=
                suspension.force_derivative_extension_velocity[wheel][column];
        }
    }

    evaluation.generalized_force[0] -= configuration.total_mass_kilograms
        * configuration.gravity_meters_per_second_squared;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        evaluation.generalized_force[3 + wheel] +=
            configuration.unsprung_mass_kilograms[wheel]
            * configuration.gravity_meters_per_second_squared;
        evaluation.generalized_force[3 + wheel] +=
            configuration.external_wheel_generalized_force[wheel];
    }
    for (int body = 0; body < 3; ++body)
    {
        evaluation.generalized_force[body] += configuration.external_body_generalized_force[body];
    }
}

int TireVerticalSolverConfigurationValidate(const TireVerticalSolverConfiguration& configuration,
    char* error_message, int error_message_size)
{
    if (error_message != NULL && error_message_size > 0)
    {
        error_message[0] = '\0';
    }
    const auto fail = [&](const char* message) -> int
    {
        if (error_message != NULL && error_message_size > 0)
        {
            std::snprintf(error_message, error_message_size, "%s", message);
        }
        return 0;
    };

    if (!(configuration.total_mass_kilograms > 0.0))
    {
        return fail("total mass must be positive");
    }
    if (!(configuration.roll_inertia_kilogram_meters_squared > 0.0)
        || !(configuration.pitch_inertia_kilogram_meters_squared > 0.0))
    {
        return fail("roll and pitch inertias must be positive");
    }
    if (!(configuration.gravity_meters_per_second_squared > 0.0))
    {
        return fail("gravity must be positive");
    }

    double total_unsprung_mass = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const TireVerticalCornerConfiguration& corner = configuration.corner[wheel];
        if (!(configuration.unsprung_mass_kilograms[wheel] > 0.0))
        {
            return fail("unsprung mass must be positive");
        }
        total_unsprung_mass += configuration.unsprung_mass_kilograms[wheel];
        if (!(corner.spring.bellcrank_ratio > 0.0))
        {
            return fail("bellcrank ratio must be positive");
        }
        if (!(corner.spring.maximum_position_meters > corner.spring.minimum_position_meters))
        {
            return fail("suspension limits are invalid");
        }
        if (!(corner.tire_radial_stiffness_newtons_per_meter > 0.0))
        {
            return fail("tire radial stiffness must be positive");
        }
        if (corner.tire_radial_damping_newtons_per_second_per_meter < 0.0)
        {
            return fail("tire radial damping must not be negative");
        }
        if (!(corner.unloaded_radius_meters > 0.0))
        {
            return fail("tire unloaded radius must be positive");
        }
        if (corner.spring.inertance_kilograms < 0.0)
        {
            return fail("suspension inertance must not be negative");
        }
    }
    if (total_unsprung_mass >= configuration.total_mass_kilograms)
    {
        return fail("unsprung mass must be smaller than total mass");
    }

    double sprung_mass_matrix[3][3] = {};
    BuildSprungMassMatrix(configuration, sprung_mass_matrix);
    if (!CholeskyPositiveDefinite3(sprung_mass_matrix))
    {
        return fail("sprung mass matrix is not positive definite");
    }

    double mass_matrix[7][7] = {};
    BuildMassMatrix(configuration, mass_matrix);
    if (!CholeskyPositiveDefinite(mass_matrix))
    {
        return fail("assembled mass matrix is not positive definite");
    }

    for (int axle = 0; axle < 2; ++axle)
    {
        if (configuration.axle[axle].anti_roll_enabled
            && configuration.axle[axle].anti_roll_stiffness_newtons_per_meter < 0.0)
        {
            return fail("anti-roll stiffness must not be negative");
        }
        if (configuration.axle[axle].heave_enabled
            && configuration.axle[axle].heave_spring.inertance_kilograms != 0.0)
        {
            return fail("heave inertance is not supported by this solver");
        }
    }
    return 1;
}

static void FillDiagnostics(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const TireVerticalSolverStepInput& input,
    const TireVerticalSuspensionEvaluation& suspension,
    const TireVerticalForceEvaluation& evaluation, TireVerticalSolverDiagnostics& diagnostics)
{
    diagnostics.active_constraint_count = 0;
    diagnostics.constraint_energy_loss_joules = 0.0;
    diagnostics.damper_energy_loss_joules = 0.0;
    diagnostics.tire_damping_energy_loss_joules = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        diagnostics.tire_normal_force_newtons[wheel] =
            evaluation.tire_normal_force_newtons[wheel];
        diagnostics.suspension_force_newtons[wheel] =
            evaluation.suspension_force_newtons[wheel];
        diagnostics.tire_deflection_meters[wheel] =
            evaluation.tire_deflection_meters[wheel];
        diagnostics.wheel_center_height_meters[wheel] =
            evaluation.wheel_center_height_meters[wheel];
        diagnostics.tire_contact[wheel] = evaluation.tire_contact[wheel];
    }
    (void)configuration;
    (void)state;
    (void)input;
    (void)suspension;
}

static void BuildStateAtEndOfStep(const TireVerticalSolverState& previous_state,
    const TireVerticalSolverState& velocity_state, double elapsed_seconds,
    TireVerticalSolverState& end_state)
{
    end_state = velocity_state;
    for (int body = 0; body < 3; ++body)
    {
        end_state.body_position[body] = previous_state.body_position[body]
            + elapsed_seconds * velocity_state.body_velocity[body];
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        end_state.suspension_extension_meters[wheel] =
            previous_state.suspension_extension_meters[wheel]
            + elapsed_seconds * velocity_state.suspension_extension_velocity_meters_per_second[wheel];
    }
}

static double TireVerticalComputeResidual(const TireVerticalSolverConfiguration& configuration,
    const double mass_matrix[7][7], const TireVerticalSolverState& previous_state,
    const TireVerticalSolverState& trial_state, const TireVerticalSolverStepInput& input,
    const int* active_wheels, const int* active_kinds, const double* multipliers,
    int active_count, double* residual)
{
    TireVerticalSolverState evaluation_state;
    BuildStateAtEndOfStep(previous_state, trial_state, input.elapsed_seconds, evaluation_state);
    TireVerticalForceEvaluation evaluation;
    EvaluateForces(configuration, evaluation_state, input, evaluation);

    for (int row = 0; row < 7; ++row)
    {
        double accumulation = 0.0;
        for (int column = 0; column < 7; ++column)
        {
            const double previous_velocity = column < 3
                ? previous_state.body_velocity[column]
                : previous_state.suspension_extension_velocity_meters_per_second[column - 3];
            const double trial_velocity = column < 3
                ? trial_state.body_velocity[column]
                : trial_state.suspension_extension_velocity_meters_per_second[column - 3];
            accumulation += mass_matrix[row][column] * (trial_velocity - previous_velocity);
        }
        residual[row] = accumulation - input.elapsed_seconds * evaluation.generalized_force[row];
    }
    for (int constraint = 0; constraint < active_count; ++constraint)
    {
        residual[3 + active_wheels[constraint]] -=
            input.elapsed_seconds * multipliers[constraint];
    }
    for (int constraint = 0; constraint < active_count; ++constraint)
    {
        const int wheel = active_wheels[constraint];
        const double limit = active_kinds[constraint] == TIRE_VERTICAL_CONSTRAINT_LOWER
            ? configuration.corner[wheel].spring.minimum_position_meters
                / configuration.corner[wheel].spring.bellcrank_ratio
            : configuration.corner[wheel].spring.maximum_position_meters
                / configuration.corner[wheel].spring.bellcrank_ratio;
        residual[7 + constraint] = previous_state.suspension_extension_meters[wheel]
            + input.elapsed_seconds
                * trial_state.suspension_extension_velocity_meters_per_second[wheel]
            - limit;
    }
    return VectorNorm(residual, 7 + active_count);
}

static int SolveNewtonSystem(const TireVerticalSolverConfiguration& configuration,
    const double mass_matrix[7][7], const TireVerticalSolverState& previous_state,
    const TireVerticalSolverStepInput& input, const int* active_wheels,
    const int* active_kinds, int active_count, TireVerticalSolverState& state,
    double* multipliers, double* residual_norm, int* iterations)
{
    int total_iterations = 0;
    for (int iteration = 0; iteration < TireVerticalMaximumNewtonIterations; ++iteration)
    {
        TireVerticalSolverState evaluation_state;
        BuildStateAtEndOfStep(previous_state, state, input.elapsed_seconds, evaluation_state);
        TireVerticalForceEvaluation evaluation;
        EvaluateForces(configuration, evaluation_state, input, evaluation);

        double residual[TireVerticalMaximumSystemSize] = {};
        for (int row = 0; row < 7; ++row)
        {
            double accumulation = 0.0;
            for (int column = 0; column < 7; ++column)
            {
                const double previous_velocity = column < 3
                    ? previous_state.body_velocity[column]
                    : previous_state.suspension_extension_velocity_meters_per_second[column - 3];
                const double trial_velocity = column < 3
                    ? state.body_velocity[column]
                    : state.suspension_extension_velocity_meters_per_second[column - 3];
                accumulation += mass_matrix[row][column] * (trial_velocity - previous_velocity);
            }
            residual[row] = accumulation - input.elapsed_seconds * evaluation.generalized_force[row];
        }
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            residual[3 + active_wheels[constraint]] -=
                input.elapsed_seconds * multipliers[constraint];
        }
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            const int wheel = active_wheels[constraint];
            const double limit = active_kinds[constraint] == TIRE_VERTICAL_CONSTRAINT_LOWER
                ? configuration.corner[wheel].spring.minimum_position_meters
                    / configuration.corner[wheel].spring.bellcrank_ratio
                : configuration.corner[wheel].spring.maximum_position_meters
                    / configuration.corner[wheel].spring.bellcrank_ratio;
            residual[7 + constraint] = previous_state.suspension_extension_meters[wheel]
                + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second[wheel]
                - limit;
        }

        const double norm = VectorNorm(residual, 7 + active_count);
        *residual_norm = norm;
        if (norm <= TireVerticalResidualTolerance)
        {
            *iterations = total_iterations;
            return 1;
        }

        const int system_size = 7 + active_count;
        double jacobian[TireVerticalMaximumSystemSize][TireVerticalMaximumSystemSize] = {};
        for (int row = 0; row < 7; ++row)
        {
            for (int column = 0; column < 7; ++column)
            {
                jacobian[row][column] = mass_matrix[row][column]
                    - input.elapsed_seconds
                        * (input.elapsed_seconds * evaluation.jacobian_position[row][column]
                            + evaluation.jacobian_velocity[row][column]);
            }
        }
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            const int wheel = active_wheels[constraint];
            jacobian[3 + wheel][7 + constraint] = -input.elapsed_seconds;
            jacobian[7 + constraint][3 + wheel] = input.elapsed_seconds;
        }

        double delta[TireVerticalMaximumSystemSize] = {};
        if (!SolveLinearSystem(&jacobian[0][0], TireVerticalMaximumSystemSize,
                residual, delta, system_size))
        {
            *iterations = total_iterations;
            return 0;
        }
        for (int index = 0; index < system_size; ++index)
        {
            delta[index] = -delta[index];
        }

        double step_scale = 1.0;
        double accepted_norm = norm;
        TireVerticalSolverState trial_state = state;
        double trial_multipliers[TireVerticalMaximumConstraintCount] = {};
        int accepted = 0;
        for (int line_search = 0; line_search < TireVerticalMaximumLineSearchIterations; ++line_search)
        {
            trial_state = state;
            for (int body = 0; body < 3; ++body)
            {
                trial_state.body_velocity[body] += step_scale * delta[body];
            }
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                trial_state.suspension_extension_velocity_meters_per_second[wheel] +=
                    step_scale * delta[3 + wheel];
            }
            BuildStateAtEndOfStep(previous_state, trial_state, input.elapsed_seconds, trial_state);
            for (int constraint = 0; constraint < active_count; ++constraint)
            {
                trial_multipliers[constraint] = multipliers[constraint]
                    + step_scale * delta[7 + constraint];
            }

            double trial_residual[TireVerticalMaximumSystemSize] = {};
            const double trial_norm = TireVerticalComputeResidual(configuration, mass_matrix,
                previous_state, trial_state, input, active_wheels, active_kinds,
                trial_multipliers, active_count, trial_residual);
            if (trial_norm < norm || line_search == TireVerticalMaximumLineSearchIterations - 1)
            {
                accepted_norm = trial_norm;
                accepted = 1;
                break;
            }
            step_scale *= 0.5;
        }
        if (!accepted)
        {
            *iterations = total_iterations;
            return 0;
        }

        state = trial_state;
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            multipliers[constraint] = trial_multipliers[constraint];
        }
        total_iterations += 1;
        *residual_norm = accepted_norm;
    }
    *iterations = total_iterations;
    return 0;
}

static double TireVerticalLimitPosition(const TireVerticalSolverConfiguration& configuration,
    int wheel, int kind)
{
    const TireVerticalSpringConfiguration& spring = configuration.corner[wheel].spring;
    return kind == TIRE_VERTICAL_CONSTRAINT_LOWER
        ? spring.minimum_position_meters / spring.bellcrank_ratio
        : spring.maximum_position_meters / spring.bellcrank_ratio;
}

static double TireVerticalCornerDamperLoss(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const TireVerticalSolverStepInput& input)
{
    double loss = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double bellcrank = configuration.corner[wheel].spring.bellcrank_ratio;
        const double velocity = -bellcrank * state.suspension_extension_velocity_meters_per_second[wheel];
        const double damper_force = TireVerticalDamperForce(configuration.corner[wheel].damper, velocity);
        loss += -bellcrank * damper_force * state.suspension_extension_velocity_meters_per_second[wheel]
            * input.elapsed_seconds;
    }
    for (int axle = 0; axle < 2; ++axle)
    {
        const TireVerticalAxleConfiguration& axle_configuration = configuration.axle[axle];
        if (!axle_configuration.heave_enabled)
        {
            continue;
        }
        const int right_wheel = 2 * axle;
        const int left_wheel = 2 * axle + 1;
        const double right_bellcrank =
            configuration.corner[right_wheel].spring.bellcrank_ratio;
        const double left_bellcrank =
            configuration.corner[left_wheel].spring.bellcrank_ratio;
        const double heave_velocity = -0.5
            * (left_bellcrank * state.suspension_extension_velocity_meters_per_second[left_wheel]
                + right_bellcrank * state.suspension_extension_velocity_meters_per_second[right_wheel]);
        const double heave_damper_force = TireVerticalDamperForce(
            axle_configuration.heave_damper, heave_velocity);
        const double heave_bellcrank = axle_configuration.heave_spring.bellcrank_ratio;
        const double left_force = 0.5 * heave_bellcrank * left_bellcrank
            * heave_damper_force;
        const double right_force = 0.5 * heave_bellcrank * right_bellcrank
            * heave_damper_force;
        loss += -(left_force * state.suspension_extension_velocity_meters_per_second[left_wheel]
            + right_force * state.suspension_extension_velocity_meters_per_second[right_wheel])
            * input.elapsed_seconds;
    }
    return std::max(0.0, loss);
}

static double TireVerticalTireDampingLoss(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const TireVerticalSolverStepInput& input)
{
    double loss = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double jacobian[3] = {
            1.0, configuration.wheel_y_meters[wheel], -configuration.wheel_x_meters[wheel]
        };
        const double chassis_reference_velocity = jacobian[0] * state.body_velocity[0]
            + jacobian[1] * state.body_velocity[1]
            + jacobian[2] * state.body_velocity[2];
        const double deflection_velocity = input.ground_height_velocity_meters_per_second[wheel]
            + state.suspension_extension_velocity_meters_per_second[wheel]
            - chassis_reference_velocity;
        loss += configuration.corner[wheel].tire_radial_damping_newtons_per_second_per_meter
            * deflection_velocity * deflection_velocity * input.elapsed_seconds;
    }
    return loss;
}

int TireVerticalSolverStep(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& previous_state, const TireVerticalSolverStepInput& input,
    TireVerticalSolverState& next_state, TireVerticalSolverDiagnostics& diagnostics)
{
    std::memset(&diagnostics, 0, sizeof(diagnostics));
    if (!(input.elapsed_seconds > 0.0))
    {
        next_state = previous_state;
        diagnostics.converged = 0;
        return 0;
    }

    double mass_matrix[7][7] = {};
    BuildMassMatrix(configuration, mass_matrix);
    diagnostics.mass_matrix_positive_definite = CholeskyPositiveDefinite(mass_matrix);
    double sprung_mass_matrix[3][3] = {};
    BuildSprungMassMatrix(configuration, sprung_mass_matrix);
    diagnostics.sprung_mass_matrix_positive_definite =
        CholeskyPositiveDefinite3(sprung_mass_matrix);
    std::memcpy(diagnostics.mass_matrix, mass_matrix, sizeof(mass_matrix));
    if (!diagnostics.mass_matrix_positive_definite
        || !diagnostics.sprung_mass_matrix_positive_definite)
    {
        next_state = previous_state;
        return 0;
    }

    TireVerticalSolverState state = previous_state;
    int active_wheels[TireVerticalMaximumConstraintCount] = {};
    int active_kinds[TireVerticalMaximumConstraintCount] = {};
    double multipliers[TireVerticalMaximumConstraintCount] = {};
    int active_count = 0;
    int total_iterations = 0;
    double residual_norm = 0.0;

    for (int active_set_iteration = 0;
        active_set_iteration < TireVerticalMaximumActiveSetIterations;
        ++active_set_iteration)
    {
        int iterations = 0;
        const int solved = SolveNewtonSystem(configuration, mass_matrix, previous_state,
            input, active_wheels, active_kinds, active_count, state, multipliers,
            &residual_norm, &iterations);
        total_iterations += iterations;
        if (!solved)
        {
            next_state = previous_state;
            diagnostics.newton_iterations = total_iterations;
            diagnostics.residual_norm = residual_norm;
            diagnostics.converged = 0;
            return 0;
        }

        int changed = 0;
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            const int wheel = active_wheels[constraint];
            const int kind = active_kinds[constraint];
            const double limit = TireVerticalLimitPosition(configuration, wheel, kind);
            const double position = previous_state.suspension_extension_meters[wheel]
                + input.elapsed_seconds
                    * state.suspension_extension_velocity_meters_per_second[wheel];
            const int valid_sign = kind == TIRE_VERTICAL_CONSTRAINT_LOWER
                ? multipliers[constraint] >= -TireVerticalMultiplierTolerance
                : multipliers[constraint] <= TireVerticalMultiplierTolerance;
            const int valid_position = std::fabs(position - limit) <= 1.0e-8;
            if (!valid_sign || !valid_position)
            {
                for (int index = constraint; index < active_count - 1; ++index)
                {
                    active_wheels[index] = active_wheels[index + 1];
                    active_kinds[index] = active_kinds[index + 1];
                    multipliers[index] = multipliers[index + 1];
                }
                active_count -= 1;
                changed = 1;
                break;
            }
        }
        if (changed)
        {
            continue;
        }

        for (int wheel = 0; wheel < 4; ++wheel)
        {
            const double lower = TireVerticalLimitPosition(configuration, wheel,
                TIRE_VERTICAL_CONSTRAINT_LOWER);
            const double upper = TireVerticalLimitPosition(configuration, wheel,
                TIRE_VERTICAL_CONSTRAINT_UPPER);
            const double position = previous_state.suspension_extension_meters[wheel]
                + input.elapsed_seconds
                    * state.suspension_extension_velocity_meters_per_second[wheel];
            if (position < lower - TireVerticalLimitToleranceMeters)
            {
                int already_active = 0;
                for (int constraint = 0; constraint < active_count; ++constraint)
                {
                    already_active = already_active
                        || (active_wheels[constraint] == wheel
                            && active_kinds[constraint] == TIRE_VERTICAL_CONSTRAINT_LOWER);
                }
                if (!already_active && active_count < TireVerticalMaximumConstraintCount)
                {
                    active_wheels[active_count] = wheel;
                    active_kinds[active_count] = TIRE_VERTICAL_CONSTRAINT_LOWER;
                    multipliers[active_count] = 0.0;
                    state.suspension_extension_velocity_meters_per_second[wheel] =
                        (lower - previous_state.suspension_extension_meters[wheel])
                        / input.elapsed_seconds;
                    active_count += 1;
                    changed = 1;
                    break;
                }
            }
            if (position > upper + TireVerticalLimitToleranceMeters)
            {
                int already_active = 0;
                for (int constraint = 0; constraint < active_count; ++constraint)
                {
                    already_active = already_active
                        || (active_wheels[constraint] == wheel
                            && active_kinds[constraint] == TIRE_VERTICAL_CONSTRAINT_UPPER);
                }
                if (!already_active && active_count < TireVerticalMaximumConstraintCount)
                {
                    active_wheels[active_count] = wheel;
                    active_kinds[active_count] = TIRE_VERTICAL_CONSTRAINT_UPPER;
                    multipliers[active_count] = 0.0;
                    state.suspension_extension_velocity_meters_per_second[wheel] =
                        (upper - previous_state.suspension_extension_meters[wheel])
                        / input.elapsed_seconds;
                    active_count += 1;
                    changed = 1;
                    break;
                }
            }
        }
        if (!changed)
        {
            break;
        }
    }

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double lower = TireVerticalLimitPosition(configuration, wheel,
            TIRE_VERTICAL_CONSTRAINT_LOWER);
        const double upper = TireVerticalLimitPosition(configuration, wheel,
            TIRE_VERTICAL_CONSTRAINT_UPPER);
        const double position = previous_state.suspension_extension_meters[wheel]
            + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second[wheel];
        if (position < lower - 1.0e-7 || position > upper + 1.0e-7)
        {
            next_state = previous_state;
            diagnostics.newton_iterations = total_iterations;
            diagnostics.residual_norm = residual_norm;
            diagnostics.converged = 0;
            return 0;
        }
    }
    for (int constraint = 0; constraint < active_count; ++constraint)
    {
        const int kind = active_kinds[constraint];
        const double multiplier = multipliers[constraint];
        if ((kind == TIRE_VERTICAL_CONSTRAINT_LOWER && multiplier < -1.0e-7)
            || (kind == TIRE_VERTICAL_CONSTRAINT_UPPER && multiplier > 1.0e-7))
        {
            next_state = previous_state;
            diagnostics.newton_iterations = total_iterations;
            diagnostics.residual_norm = residual_norm;
            diagnostics.converged = 0;
            return 0;
        }
    }

    for (int body = 0; body < 3; ++body)
    {
        next_state.body_position[body] = previous_state.body_position[body]
            + input.elapsed_seconds * state.body_velocity[body];
        next_state.body_velocity[body] = state.body_velocity[body];
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        next_state.suspension_extension_meters[wheel] =
            previous_state.suspension_extension_meters[wheel]
            + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second[wheel];
        next_state.suspension_extension_velocity_meters_per_second[wheel] =
            state.suspension_extension_velocity_meters_per_second[wheel];
    }

    TireVerticalForceEvaluation evaluation;
    EvaluateForces(configuration, next_state, input, evaluation);
    TireVerticalSuspensionEvaluation suspension;
    EvaluateSuspension(configuration, next_state.suspension_extension_meters,
        next_state.suspension_extension_velocity_meters_per_second, suspension);
    FillDiagnostics(configuration, next_state, input, suspension, evaluation, diagnostics);

    diagnostics.converged = 1;
    diagnostics.newton_iterations = total_iterations;
    diagnostics.residual_norm = residual_norm;
    diagnostics.active_constraint_count = active_count;
    double constraint_loss = 0.0;
    for (int constraint = 0; constraint < active_count; ++constraint)
    {
        const int wheel = active_wheels[constraint];
        diagnostics.active_constraint_wheel[constraint] = wheel;
        diagnostics.active_constraint_kind[constraint] = active_kinds[constraint];
        diagnostics.constraint_impulse_newton_seconds[constraint] =
            input.elapsed_seconds * multipliers[constraint];
        const double average_velocity = 0.5
            * (previous_state.suspension_extension_velocity_meters_per_second[wheel]
                + next_state.suspension_extension_velocity_meters_per_second[wheel]);
        const double work = input.elapsed_seconds * multipliers[constraint] * average_velocity;
        constraint_loss += std::max(0.0, -work);
    }
    diagnostics.constraint_energy_loss_joules = constraint_loss;
    diagnostics.damper_energy_loss_joules =
        TireVerticalCornerDamperLoss(configuration, next_state, input);
    diagnostics.tire_damping_energy_loss_joules =
        TireVerticalTireDampingLoss(configuration, next_state, input);
    return 1;
}

double TireVerticalSystemEnergyJoules(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const double ground_height_meters[4])
{
    double mass_matrix[7][7] = {};
    BuildMassMatrix(configuration, mass_matrix);
    double velocity[7] = {};
    for (int body = 0; body < 3; ++body)
    {
        velocity[body] = state.body_velocity[body];
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        velocity[3 + wheel] = state.suspension_extension_velocity_meters_per_second[wheel];
    }
    double kinetic = 0.0;
    for (int row = 0; row < 7; ++row)
    {
        for (int column = 0; column < 7; ++column)
        {
            kinetic += 0.5 * velocity[row] * mass_matrix[row][column] * velocity[column];
        }
    }

    double total_unsprung_mass = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        total_unsprung_mass += configuration.unsprung_mass_kilograms[wheel];
    }
    const double sprung_mass = configuration.total_mass_kilograms - total_unsprung_mass;
    double potential = sprung_mass * configuration.gravity_meters_per_second_squared
        * state.body_position[0];

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double jacobian[3] = {
            1.0, configuration.wheel_y_meters[wheel], -configuration.wheel_x_meters[wheel]
        };
        const double chassis_reference_height = jacobian[0] * state.body_position[0]
            + jacobian[1] * state.body_position[1]
            + jacobian[2] * state.body_position[2];
        const double center_height = chassis_reference_height
            + configuration.corner[wheel].unloaded_radius_meters
            - state.suspension_extension_meters[wheel];
        potential += configuration.unsprung_mass_kilograms[wheel]
            * configuration.gravity_meters_per_second_squared * center_height;

        const double clearance = chassis_reference_height - ground_height_meters[wheel];
        const double deflection = state.suspension_extension_meters[wheel] - clearance;
        if (deflection > 0.0)
        {
            potential += 0.5 * configuration.corner[wheel].tire_radial_stiffness_newtons_per_meter
                * deflection * deflection;
        }

        const double bellcrank = configuration.corner[wheel].spring.bellcrank_ratio;
        potential += TireVerticalSpringPotentialJoules(configuration.corner[wheel].spring,
            bellcrank * state.suspension_extension_meters[wheel]);
    }

    for (int axle = 0; axle < 2; ++axle)
    {
        const TireVerticalAxleConfiguration& axle_configuration = configuration.axle[axle];
        const int right_wheel = 2 * axle;
        const int left_wheel = 2 * axle + 1;
        if (axle_configuration.anti_roll_enabled)
        {
            const double difference =
                configuration.corner[left_wheel].spring.bellcrank_ratio
                    * state.suspension_extension_meters[left_wheel]
                - configuration.corner[right_wheel].spring.bellcrank_ratio
                    * state.suspension_extension_meters[right_wheel];
            potential += 0.5 * axle_configuration.anti_roll_stiffness_newtons_per_meter
                * difference * difference;
        }
        if (axle_configuration.heave_enabled)
        {
            const double heave_position = 0.5
                * (configuration.corner[left_wheel].spring.bellcrank_ratio
                        * state.suspension_extension_meters[left_wheel]
                    + configuration.corner[right_wheel].spring.bellcrank_ratio
                        * state.suspension_extension_meters[right_wheel]);
            potential += axle_configuration.heave_spring.bellcrank_ratio
                * TireVerticalSpringPotentialJoules(axle_configuration.heave_spring,
                    heave_position);
        }
    }

    return kinetic + potential;
}

double TireVerticalJacobianError(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& state, const TireVerticalSolverStepInput& input)
{
    TireVerticalForceEvaluation base_evaluation;
    EvaluateForces(configuration, state, input, base_evaluation);
    double maximum_error = 0.0;
    const double step = 1.0e-6;

    for (int column = 0; column < 7; ++column)
    {
        TireVerticalSolverState positive_state = state;
        TireVerticalSolverState negative_state = state;
        if (column < 3)
        {
            positive_state.body_position[column] += step;
            negative_state.body_position[column] -= step;
        }
        else
        {
            positive_state.suspension_extension_meters[column - 3] += step;
            negative_state.suspension_extension_meters[column - 3] -= step;
        }
        TireVerticalForceEvaluation positive_evaluation;
        TireVerticalForceEvaluation negative_evaluation;
        EvaluateForces(configuration, positive_state, input, positive_evaluation);
        EvaluateForces(configuration, negative_state, input, negative_evaluation);
        for (int row = 0; row < 7; ++row)
        {
            const double numerical = (positive_evaluation.generalized_force[row]
                - negative_evaluation.generalized_force[row]) / (2.0 * step);
            maximum_error = std::max(maximum_error,
                std::fabs(numerical - base_evaluation.jacobian_position[row][column]));
        }
    }

    for (int column = 0; column < 7; ++column)
    {
        TireVerticalSolverState positive_state = state;
        TireVerticalSolverState negative_state = state;
        if (column < 3)
        {
            positive_state.body_velocity[column] += step;
            negative_state.body_velocity[column] -= step;
        }
        else
        {
            positive_state.suspension_extension_velocity_meters_per_second[column - 3] += step;
            negative_state.suspension_extension_velocity_meters_per_second[column - 3] -= step;
        }
        TireVerticalForceEvaluation positive_evaluation;
        TireVerticalForceEvaluation negative_evaluation;
        EvaluateForces(configuration, positive_state, input, positive_evaluation);
        EvaluateForces(configuration, negative_state, input, negative_evaluation);
        for (int row = 0; row < 7; ++row)
        {
            const double numerical = (positive_evaluation.generalized_force[row]
                - negative_evaluation.generalized_force[row]) / (2.0 * step);
            maximum_error = std::max(maximum_error,
                std::fabs(numerical - base_evaluation.jacobian_velocity[row][column]));
        }
    }
    return maximum_error;
}

int TireVerticalSolverInitializeEquilibrium(const TireVerticalSolverConfiguration& configuration,
    const double ground_height_meters[4], TireVerticalSolverState& state,
    TireVerticalSolverDiagnostics& diagnostics)
{
    std::memset(&diagnostics, 0, sizeof(diagnostics));
    double mass_matrix[7][7] = {};
    BuildMassMatrix(configuration, mass_matrix);
    diagnostics.mass_matrix_positive_definite = CholeskyPositiveDefinite(mass_matrix);
    double sprung_mass_matrix[3][3] = {};
    BuildSprungMassMatrix(configuration, sprung_mass_matrix);
    diagnostics.sprung_mass_matrix_positive_definite =
        CholeskyPositiveDefinite3(sprung_mass_matrix);
    std::memcpy(diagnostics.mass_matrix, mass_matrix, sizeof(mass_matrix));
    if (!diagnostics.mass_matrix_positive_definite
        || !diagnostics.sprung_mass_matrix_positive_definite)
    {
        return 0;
    }

    std::memset(&state, 0, sizeof(state));
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double bellcrank = configuration.corner[wheel].spring.bellcrank_ratio;
        state.suspension_extension_meters[wheel] =
            configuration.corner[wheel].spring.reference_position_meters / bellcrank;
    }

    const double average_normal_force = configuration.total_mass_kilograms
        * configuration.gravity_meters_per_second_squared * 0.25;
    double average_chassis_height = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double deflection = average_normal_force
            / configuration.corner[wheel].tire_radial_stiffness_newtons_per_meter;
        average_chassis_height += (state.suspension_extension_meters[wheel] - deflection
            + ground_height_meters[wheel]) * 0.25;
    }
    state.body_position[0] = average_chassis_height;

    TireVerticalSolverStepInput static_input = {};
    static_input.elapsed_seconds = 1.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        static_input.ground_height_meters[wheel] = ground_height_meters[wheel];
    }

    TireVerticalSolverState trial_state = state;
    double q[7] = {};
    q[0] = state.body_position[0];
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        q[3 + wheel] = state.suspension_extension_meters[wheel];
    }
    for (int iteration = 0; iteration < 80; ++iteration)
    {
        trial_state.body_position[0] = q[0];
        trial_state.body_position[1] = q[1];
        trial_state.body_position[2] = q[2];
        trial_state.body_velocity[0] = 0.0;
        trial_state.body_velocity[1] = 0.0;
        trial_state.body_velocity[2] = 0.0;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            trial_state.suspension_extension_meters[wheel] = q[3 + wheel];
            trial_state.suspension_extension_velocity_meters_per_second[wheel] = 0.0;
        }

        TireVerticalForceEvaluation evaluation;
        EvaluateForces(configuration, trial_state, static_input, evaluation);
        double residual[7] = {};
        for (int row = 0; row < 7; ++row)
        {
            residual[row] = evaluation.generalized_force[row];
        }
        const double norm = VectorNorm(residual, 7);
        diagnostics.residual_norm = norm;
        if (norm <= 1.0e-6)
        {
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                const double lower = configuration.corner[wheel].spring.minimum_position_meters
                    / configuration.corner[wheel].spring.bellcrank_ratio;
                const double upper = configuration.corner[wheel].spring.maximum_position_meters
                    / configuration.corner[wheel].spring.bellcrank_ratio;
                if (trial_state.suspension_extension_meters[wheel] < lower - 1.0e-6
                    || trial_state.suspension_extension_meters[wheel] > upper + 1.0e-6)
                {
                    diagnostics.converged = 0;
                    return 0;
                }
            }
            state = trial_state;
            FillDiagnostics(configuration, state, static_input, TireVerticalSuspensionEvaluation(),
                evaluation, diagnostics);
            diagnostics.converged = 1;
            diagnostics.newton_iterations = iteration;
            for (int body = 0; body < 3; ++body)
            {
                state.body_velocity[body] = 0.0;
            }
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                state.suspension_extension_velocity_meters_per_second[wheel] = 0.0;
            }
            return 1;
        }

        double jacobian[7][7] = {};
        std::memcpy(jacobian, evaluation.jacobian_position, sizeof(jacobian));
        double delta[7] = {};
        if (!SolveLinearSystem(&jacobian[0][0], 7, residual, delta, 7))
        {
            diagnostics.converged = 0;
            return 0;
        }
        for (int index = 0; index < 7; ++index)
        {
            delta[index] = -delta[index];
        }

        double step_scale = 1.0;
        double best_norm = norm;
        double best_q[7] = {};
        int accepted = 0;
        for (int line_search = 0; line_search < 12; ++line_search)
        {
            double candidate_q[7] = {};
            for (int index = 0; index < 7; ++index)
            {
                candidate_q[index] = q[index] + step_scale * delta[index];
            }
            trial_state.body_position[0] = candidate_q[0];
            trial_state.body_position[1] = candidate_q[1];
            trial_state.body_position[2] = candidate_q[2];
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                trial_state.suspension_extension_meters[wheel] = candidate_q[3 + wheel];
            }
            TireVerticalForceEvaluation candidate_evaluation;
            EvaluateForces(configuration, trial_state, static_input, candidate_evaluation);
            double candidate_residual[7] = {};
            for (int row = 0; row < 7; ++row)
            {
                candidate_residual[row] = candidate_evaluation.generalized_force[row];
            }
            const double candidate_norm = VectorNorm(candidate_residual, 7);
            if (candidate_norm < best_norm)
            {
                best_norm = candidate_norm;
                std::memcpy(best_q, candidate_q, sizeof(best_q));
                accepted = 1;
                break;
            }
            step_scale *= 0.5;
        }
        if (!accepted)
        {
            diagnostics.converged = 0;
            return 0;
        }
        for (int index = 0; index < 7; ++index)
        {
            q[index] = best_q[index];
        }
    }

    diagnostics.converged = 0;
    return 0;
}

static void EvaluateQuarterCarForces(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& state, const TireVerticalQuarterCarStepInput& input,
    double force[2], double jacobian_position[2][2], double jacobian_velocity[2][2],
    double& normal_force, double& suspension_force, double& deflection,
    double& deflection_velocity, double& center_height, int& contact)
{
    const double chassis_reference_height = state.body_position_meters;
    const double chassis_reference_velocity = state.body_velocity_meters_per_second;
    deflection = state.suspension_extension_meters
        - (chassis_reference_height - input.ground_height_meters);
    deflection_velocity = input.ground_height_velocity_meters_per_second
        + state.suspension_extension_velocity_meters_per_second
        - chassis_reference_velocity;
    normal_force = 0.0;
    contact = 0;
    std::memset(jacobian_position, 0, sizeof(double) * 4);
    std::memset(jacobian_velocity, 0, sizeof(double) * 4);
    if (deflection > 0.0)
    {
        const double linear_force = configuration.corner.tire_radial_stiffness_newtons_per_meter
            * deflection
            + configuration.corner.tire_radial_damping_newtons_per_second_per_meter
                * deflection_velocity;
        if (linear_force > 0.0)
        {
            normal_force = linear_force;
            contact = 1;
            jacobian_position[0][0] = -configuration.corner.tire_radial_stiffness_newtons_per_meter;
            jacobian_position[0][1] = configuration.corner.tire_radial_stiffness_newtons_per_meter;
            jacobian_position[1][0] = configuration.corner.tire_radial_stiffness_newtons_per_meter;
            jacobian_position[1][1] = -configuration.corner.tire_radial_stiffness_newtons_per_meter;
            jacobian_velocity[0][0] =
                -configuration.corner.tire_radial_damping_newtons_per_second_per_meter;
            jacobian_velocity[0][1] =
                configuration.corner.tire_radial_damping_newtons_per_second_per_meter;
            jacobian_velocity[1][0] =
                configuration.corner.tire_radial_damping_newtons_per_second_per_meter;
            jacobian_velocity[1][1] =
                -configuration.corner.tire_radial_damping_newtons_per_second_per_meter;
        }
    }

    suspension_force = TireVerticalCornerSuspensionForce(configuration.corner,
        state.suspension_extension_meters, state.suspension_extension_velocity_meters_per_second);
    center_height = chassis_reference_height + configuration.corner.unloaded_radius_meters
        - state.suspension_extension_meters;

    force[0] = normal_force
        - (configuration.sprung_mass_kilograms + configuration.unsprung_mass_kilograms)
            * configuration.gravity_meters_per_second_squared;
    force[1] = suspension_force - normal_force
        + configuration.unsprung_mass_kilograms
            * configuration.gravity_meters_per_second_squared;

    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    const double position = bellcrank * state.suspension_extension_meters;
    const double spring_force = TireVerticalSpringForce(configuration.corner.spring, position);
    const double damper_velocity = -bellcrank
        * state.suspension_extension_velocity_meters_per_second;
    const double damper_slope = TireVerticalDamperSlope(configuration.corner.damper, damper_velocity);
    if (spring_force > 0.0)
    {
        const double derivative = bellcrank * bellcrank
            * configuration.corner.spring.spring_constant_newtons_per_meter;
        jacobian_position[1][1] += derivative;
    }
    jacobian_velocity[1][1] += -bellcrank * bellcrank * damper_slope;
}

static void BuildQuarterCarMassMatrix(const TireVerticalQuarterCarConfiguration& configuration,
    double mass_matrix[2][2])
{
    const double total_mass = configuration.sprung_mass_kilograms
        + configuration.unsprung_mass_kilograms;
    mass_matrix[0][0] = total_mass;
    mass_matrix[0][1] = -configuration.unsprung_mass_kilograms;
    mass_matrix[1][0] = -configuration.unsprung_mass_kilograms;
    mass_matrix[1][1] = configuration.unsprung_mass_kilograms;
    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    mass_matrix[1][1] -= bellcrank * bellcrank
        * configuration.corner.spring.inertance_kilograms;
}

static int QuarterCarMassMatrixPositiveDefinite(const TireVerticalQuarterCarConfiguration& configuration)
{
    double mass_matrix[2][2] = {};
    BuildQuarterCarMassMatrix(configuration, mass_matrix);
    return mass_matrix[0][0] > 0.0
        && mass_matrix[0][0] * mass_matrix[1][1] - mass_matrix[0][1] * mass_matrix[1][0] > 0.0;
}

int TireVerticalQuarterCarInitializeEquilibrium(const TireVerticalQuarterCarConfiguration& configuration,
    double ground_height_meters, TireVerticalQuarterCarState& state,
    TireVerticalQuarterCarDiagnostics& diagnostics)
{
    std::memset(&diagnostics, 0, sizeof(diagnostics));
    if (!QuarterCarMassMatrixPositiveDefinite(configuration))
    {
        return 0;
    }

    std::memset(&state, 0, sizeof(state));
    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    state.suspension_extension_meters =
        configuration.corner.spring.reference_position_meters / bellcrank;
    const double normal_force_guess = (configuration.sprung_mass_kilograms
        + configuration.unsprung_mass_kilograms)
        * configuration.gravity_meters_per_second_squared;
    state.body_position_meters = state.suspension_extension_meters
        - normal_force_guess / configuration.corner.tire_radial_stiffness_newtons_per_meter
        + ground_height_meters;

    TireVerticalQuarterCarStepInput static_input = {};
    static_input.elapsed_seconds = 1.0;
    static_input.ground_height_meters = ground_height_meters;

    double q[2] = { state.body_position_meters, state.suspension_extension_meters };
    for (int iteration = 0; iteration < 80; ++iteration)
    {
        TireVerticalQuarterCarState trial_state = state;
        trial_state.body_position_meters = q[0];
        trial_state.suspension_extension_meters = q[1];
        double force[2] = {};
        double jacobian_position[2][2] = {};
        double jacobian_velocity[2][2] = {};
        double normal_force = 0.0;
        double suspension_force = 0.0;
        double deflection = 0.0;
        double deflection_velocity = 0.0;
        double center_height = 0.0;
        int contact = 0;
        EvaluateQuarterCarForces(configuration, trial_state, static_input, force,
            jacobian_position, jacobian_velocity, normal_force, suspension_force,
            deflection, deflection_velocity, center_height, contact);
        const double residual[2] = { force[0], force[1] };
        const double norm = VectorNorm(residual, 2);
        diagnostics.residual_norm = norm;
        if (norm <= 1.0e-7)
        {
            const double lower = configuration.corner.spring.minimum_position_meters
                / configuration.corner.spring.bellcrank_ratio;
            const double upper = configuration.corner.spring.maximum_position_meters
                / configuration.corner.spring.bellcrank_ratio;
            if (trial_state.suspension_extension_meters < lower - 1.0e-6
                || trial_state.suspension_extension_meters > upper + 1.0e-6)
            {
                diagnostics.converged = 0;
                return 0;
            }
            state = trial_state;
            state.body_velocity_meters_per_second = 0.0;
            state.suspension_extension_velocity_meters_per_second = 0.0;
            diagnostics.converged = 1;
            diagnostics.newton_iterations = iteration;
            diagnostics.tire_normal_force_newtons = normal_force;
            diagnostics.suspension_force_newtons = suspension_force;
            diagnostics.tire_deflection_meters = deflection;
            diagnostics.wheel_center_height_meters = center_height;
            diagnostics.tire_contact = contact;
            return 1;
        }

        double delta[2] = {};
        if (!SolveLinearSystem(&jacobian_position[0][0], 2, residual, delta, 2))
        {
            diagnostics.converged = 0;
            return 0;
        }
        delta[0] = -delta[0];
        delta[1] = -delta[1];

        double step_scale = 1.0;
        double best_norm = norm;
        double best_q[2] = {};
        int accepted = 0;
        for (int line_search = 0; line_search < 12; ++line_search)
        {
            const double candidate_q[2] = {
                q[0] + step_scale * delta[0], q[1] + step_scale * delta[1]
            };
            trial_state.body_position_meters = candidate_q[0];
            trial_state.suspension_extension_meters = candidate_q[1];
            EvaluateQuarterCarForces(configuration, trial_state, static_input, force,
                jacobian_position, jacobian_velocity, normal_force, suspension_force,
                deflection, deflection_velocity, center_height, contact);
            const double candidate_residual[2] = { force[0], force[1] };
            const double candidate_norm = VectorNorm(candidate_residual, 2);
            if (candidate_norm < best_norm)
            {
                best_norm = candidate_norm;
                best_q[0] = candidate_q[0];
                best_q[1] = candidate_q[1];
                accepted = 1;
                break;
            }
            step_scale *= 0.5;
        }
        if (!accepted)
        {
            diagnostics.converged = 0;
            return 0;
        }
        q[0] = best_q[0];
        q[1] = best_q[1];
    }

    diagnostics.converged = 0;
    return 0;
}

static double QuarterCarLimitPosition(const TireVerticalQuarterCarConfiguration& configuration,
    int kind)
{
    return kind == TIRE_VERTICAL_CONSTRAINT_LOWER
        ? configuration.corner.spring.minimum_position_meters
            / configuration.corner.spring.bellcrank_ratio
        : configuration.corner.spring.maximum_position_meters
            / configuration.corner.spring.bellcrank_ratio;
}

int TireVerticalQuarterCarStep(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& previous_state, const TireVerticalQuarterCarStepInput& input,
    TireVerticalQuarterCarState& next_state, TireVerticalQuarterCarDiagnostics& diagnostics)
{
    std::memset(&diagnostics, 0, sizeof(diagnostics));
    if (!(input.elapsed_seconds > 0.0))
    {
        next_state = previous_state;
        diagnostics.converged = 0;
        return 0;
    }

    double mass_matrix[2][2] = {};
    BuildQuarterCarMassMatrix(configuration, mass_matrix);
    TireVerticalQuarterCarState state = previous_state;
    int active_kind = TIRE_VERTICAL_CONSTRAINT_NONE;
    double multiplier = 0.0;
    int total_iterations = 0;
    double residual_norm = 0.0;

    for (int active_set_iteration = 0; active_set_iteration < 8; ++active_set_iteration)
    {
        int iteration_count = 0;
        int solved = 0;
        for (int iteration = 0; iteration < TireVerticalMaximumNewtonIterations; ++iteration)
        {
            iteration_count += 1;
            TireVerticalQuarterCarState evaluation_state = state;
            evaluation_state.body_position_meters = previous_state.body_position_meters
                + input.elapsed_seconds * state.body_velocity_meters_per_second;
            evaluation_state.suspension_extension_meters =
                previous_state.suspension_extension_meters
                + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second;
            double force[2] = {};
            double jacobian_position[2][2] = {};
            double jacobian_velocity[2][2] = {};
            double normal_force = 0.0;
            double suspension_force = 0.0;
            double deflection = 0.0;
            double deflection_velocity = 0.0;
            double center_height = 0.0;
            int contact = 0;
            EvaluateQuarterCarForces(configuration, evaluation_state, input, force,
                jacobian_position, jacobian_velocity, normal_force, suspension_force,
                deflection, deflection_velocity, center_height, contact);

            double residual[3] = {};
            residual[0] = mass_matrix[0][0] * (state.body_velocity_meters_per_second
                    - previous_state.body_velocity_meters_per_second)
                + mass_matrix[0][1] * (state.suspension_extension_velocity_meters_per_second
                    - previous_state.suspension_extension_velocity_meters_per_second)
                - input.elapsed_seconds * force[0];
            residual[1] = mass_matrix[1][0] * (state.body_velocity_meters_per_second
                    - previous_state.body_velocity_meters_per_second)
                + mass_matrix[1][1] * (state.suspension_extension_velocity_meters_per_second
                    - previous_state.suspension_extension_velocity_meters_per_second)
                - input.elapsed_seconds * force[1];
            if (active_kind != TIRE_VERTICAL_CONSTRAINT_NONE)
            {
                residual[1] -= input.elapsed_seconds * multiplier;
                const double limit = QuarterCarLimitPosition(configuration, active_kind);
                residual[2] = previous_state.suspension_extension_meters
                    + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second
                    - limit;
            }
            const int system_size = active_kind == TIRE_VERTICAL_CONSTRAINT_NONE ? 2 : 3;
            const double norm = VectorNorm(residual, system_size);
            residual_norm = norm;
            if (norm <= TireVerticalResidualTolerance)
            {
                solved = 1;
                break;
            }

            double jacobian[3][3] = {};
            for (int row = 0; row < 2; ++row)
            {
                for (int column = 0; column < 2; ++column)
                {
                    jacobian[row][column] = mass_matrix[row][column]
                        - input.elapsed_seconds
                            * (input.elapsed_seconds * jacobian_position[row][column]
                                + jacobian_velocity[row][column]);
                }
            }
            if (active_kind != TIRE_VERTICAL_CONSTRAINT_NONE)
            {
                jacobian[1][2] = -input.elapsed_seconds;
                jacobian[2][1] = input.elapsed_seconds;
            }

            double delta[3] = {};
            if (!SolveLinearSystem(&jacobian[0][0], 3, residual, delta, system_size))
            {
                break;
            }
            for (int index = 0; index < system_size; ++index)
            {
                delta[index] = -delta[index];
            }

            double step_scale = 1.0;
            TireVerticalQuarterCarState trial_state = state;
            double trial_multiplier = multiplier;
            double best_norm = norm;
            int accepted = 0;
            for (int line_search = 0; line_search < 10; ++line_search)
            {
                trial_state = state;
                trial_state.body_velocity_meters_per_second += step_scale * delta[0];
                trial_state.suspension_extension_velocity_meters_per_second += step_scale * delta[1];
                if (active_kind != TIRE_VERTICAL_CONSTRAINT_NONE)
                {
                    trial_multiplier = multiplier + step_scale * delta[2];
                }
                TireVerticalQuarterCarState trial_evaluation_state = trial_state;
                trial_evaluation_state.body_position_meters = previous_state.body_position_meters
                    + input.elapsed_seconds * trial_state.body_velocity_meters_per_second;
                trial_evaluation_state.suspension_extension_meters =
                    previous_state.suspension_extension_meters
                    + input.elapsed_seconds
                        * trial_state.suspension_extension_velocity_meters_per_second;
                EvaluateQuarterCarForces(configuration, trial_evaluation_state, input, force,
                    jacobian_position, jacobian_velocity, normal_force, suspension_force,
                    deflection, deflection_velocity, center_height, contact);
                double trial_residual[3] = {};
                trial_residual[0] = mass_matrix[0][0] * (trial_state.body_velocity_meters_per_second
                        - previous_state.body_velocity_meters_per_second)
                    + mass_matrix[0][1] * (trial_state.suspension_extension_velocity_meters_per_second
                        - previous_state.suspension_extension_velocity_meters_per_second)
                    - input.elapsed_seconds * force[0];
                trial_residual[1] = mass_matrix[1][0] * (trial_state.body_velocity_meters_per_second
                        - previous_state.body_velocity_meters_per_second)
                    + mass_matrix[1][1] * (trial_state.suspension_extension_velocity_meters_per_second
                        - previous_state.suspension_extension_velocity_meters_per_second)
                    - input.elapsed_seconds * force[1];
                if (active_kind != TIRE_VERTICAL_CONSTRAINT_NONE)
                {
                    trial_residual[1] -= input.elapsed_seconds * trial_multiplier;
                    const double limit = QuarterCarLimitPosition(configuration, active_kind);
                    trial_residual[2] = previous_state.suspension_extension_meters
                        + input.elapsed_seconds
                            * trial_state.suspension_extension_velocity_meters_per_second
                        - limit;
                }
                const double trial_norm = VectorNorm(trial_residual, system_size);
                if (trial_norm < best_norm)
                {
                    best_norm = trial_norm;
                    accepted = 1;
                    break;
                }
                step_scale *= 0.5;
            }
            if (!accepted)
            {
                break;
            }
            state = trial_state;
            multiplier = trial_multiplier;
            residual_norm = best_norm;
        }
        total_iterations += iteration_count;
        if (!solved)
        {
            next_state = previous_state;
            diagnostics.newton_iterations = total_iterations;
            diagnostics.residual_norm = residual_norm;
            diagnostics.converged = 0;
            return 0;
        }

        const double position = previous_state.suspension_extension_meters
            + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second;
        int changed = 0;
        if (active_kind == TIRE_VERTICAL_CONSTRAINT_NONE)
        {
            const double lower = QuarterCarLimitPosition(configuration,
                TIRE_VERTICAL_CONSTRAINT_LOWER);
            const double upper = QuarterCarLimitPosition(configuration,
                TIRE_VERTICAL_CONSTRAINT_UPPER);
            if (position < lower - TireVerticalLimitToleranceMeters)
            {
                active_kind = TIRE_VERTICAL_CONSTRAINT_LOWER;
                multiplier = 0.0;
                state.suspension_extension_velocity_meters_per_second =
                    (lower - previous_state.suspension_extension_meters) / input.elapsed_seconds;
                changed = 1;
            }
            else if (position > upper + TireVerticalLimitToleranceMeters)
            {
                active_kind = TIRE_VERTICAL_CONSTRAINT_UPPER;
                multiplier = 0.0;
                state.suspension_extension_velocity_meters_per_second =
                    (upper - previous_state.suspension_extension_meters) / input.elapsed_seconds;
                changed = 1;
            }
        }
        else
        {
            const double limit = QuarterCarLimitPosition(configuration, active_kind);
            const int valid_sign = active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
                ? multiplier >= -TireVerticalMultiplierTolerance
                : multiplier <= TireVerticalMultiplierTolerance;
            const int valid_position = std::fabs(position - limit) <= 1.0e-8;
            if (!valid_sign || !valid_position)
            {
                active_kind = TIRE_VERTICAL_CONSTRAINT_NONE;
                multiplier = 0.0;
                changed = 1;
            }
        }
        if (!changed)
        {
            break;
        }
    }

    const double final_position = previous_state.suspension_extension_meters
        + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second;
    const double lower_limit = QuarterCarLimitPosition(configuration,
        TIRE_VERTICAL_CONSTRAINT_LOWER);
    const double upper_limit = QuarterCarLimitPosition(configuration,
        TIRE_VERTICAL_CONSTRAINT_UPPER);
    const int valid_limit_sign = active_kind == TIRE_VERTICAL_CONSTRAINT_NONE
        || (active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
            && multiplier >= -TireVerticalMultiplierTolerance)
        || (active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER
            && multiplier <= TireVerticalMultiplierTolerance);
    if (final_position < lower_limit - 1.0e-7 || final_position > upper_limit + 1.0e-7
        || !valid_limit_sign)
    {
        next_state = previous_state;
        diagnostics.newton_iterations = total_iterations;
        diagnostics.residual_norm = residual_norm;
        diagnostics.converged = 0;
        return 0;
    }

    next_state.body_position_meters = previous_state.body_position_meters
        + input.elapsed_seconds * state.body_velocity_meters_per_second;
    next_state.body_velocity_meters_per_second = state.body_velocity_meters_per_second;
    next_state.suspension_extension_meters = final_position;
    next_state.suspension_extension_velocity_meters_per_second =
        state.suspension_extension_velocity_meters_per_second;

    double force[2] = {};
    double jacobian_position[2][2] = {};
    double jacobian_velocity[2][2] = {};
    double normal_force = 0.0;
    double suspension_force = 0.0;
    double deflection = 0.0;
    double deflection_velocity = 0.0;
    double center_height = 0.0;
    int contact = 0;
    EvaluateQuarterCarForces(configuration, next_state, input, force,
        jacobian_position, jacobian_velocity, normal_force, suspension_force,
        deflection, deflection_velocity, center_height, contact);

    diagnostics.converged = 1;
    diagnostics.newton_iterations = total_iterations;
    diagnostics.residual_norm = residual_norm;
    diagnostics.tire_normal_force_newtons = normal_force;
    diagnostics.suspension_force_newtons = suspension_force;
    diagnostics.tire_deflection_meters = deflection;
    diagnostics.wheel_center_height_meters = center_height;
    diagnostics.tire_contact = contact;
    diagnostics.lower_limit_active = active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER;
    diagnostics.upper_limit_active = active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER;
    diagnostics.limit_impulse_newton_seconds = input.elapsed_seconds * multiplier;
    if (active_kind != TIRE_VERTICAL_CONSTRAINT_NONE)
    {
        const double average_velocity = 0.5
            * (previous_state.suspension_extension_velocity_meters_per_second
                + next_state.suspension_extension_velocity_meters_per_second);
        diagnostics.constraint_energy_loss_joules =
            std::max(0.0, -input.elapsed_seconds * multiplier * average_velocity);
    }

    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    const double damper_velocity = -bellcrank
        * next_state.suspension_extension_velocity_meters_per_second;
    const double damper_force = TireVerticalDamperForce(configuration.corner.damper,
        damper_velocity);
    diagnostics.damper_energy_loss_joules = std::max(0.0,
        -bellcrank * damper_force
            * next_state.suspension_extension_velocity_meters_per_second
            * input.elapsed_seconds);
    diagnostics.tire_damping_energy_loss_joules =
        configuration.corner.tire_radial_damping_newtons_per_second_per_meter
        * deflection_velocity * deflection_velocity * input.elapsed_seconds;
    return 1;
}

double TireVerticalQuarterCarEnergyJoules(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& state, double ground_height_meters)
{
    double mass_matrix[2][2] = {};
    BuildQuarterCarMassMatrix(configuration, mass_matrix);
    const double velocity[2] = {
        state.body_velocity_meters_per_second,
        state.suspension_extension_velocity_meters_per_second
    };
    double kinetic = 0.0;
    for (int row = 0; row < 2; ++row)
    {
        for (int column = 0; column < 2; ++column)
        {
            kinetic += 0.5 * velocity[row] * mass_matrix[row][column] * velocity[column];
        }
    }

    const double center_height = state.body_position_meters
        + configuration.corner.unloaded_radius_meters
        - state.suspension_extension_meters;
    double potential = configuration.sprung_mass_kilograms
        * configuration.gravity_meters_per_second_squared * state.body_position_meters
        + configuration.unsprung_mass_kilograms
            * configuration.gravity_meters_per_second_squared * center_height;

    const double deflection = state.suspension_extension_meters
        - (state.body_position_meters - ground_height_meters);
    if (deflection > 0.0)
    {
        potential += 0.5 * configuration.corner.tire_radial_stiffness_newtons_per_meter
            * deflection * deflection;
    }
    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    potential += TireVerticalSpringPotentialJoules(configuration.corner.spring,
        bellcrank * state.suspension_extension_meters);
    return kinetic + potential;
}
