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
#include <cstdlib>
#include <cstring>

#include "tire_vertical_solver.h"

static const int TireVerticalSystemSize = 7;
static const int TireVerticalMaximumConstraintCount = 4 + 4 + TIRE_VERTICAL_MAX_CHASSIS_CONTACTS;
static const int TireVerticalMaximumSystemSize = TireVerticalSystemSize + TireVerticalMaximumConstraintCount;
static const int TireVerticalMaximumNewtonIterations = 60;
static const int TireVerticalMaximumActiveSetIterations = 24;
static const int TireVerticalMaximumLineSearchIterations = 10;
static const double TireVerticalResidualTolerance = 1.0e-7;
static const double TireVerticalChassisResidualTolerance = 1.0e-5;
static const double TireVerticalLimitToleranceMeters = 1.0e-9;
static const double TireVerticalMultiplierTolerance = 1.0e-7;
static const double TireVerticalDamperVelocityLimit = 10.0;
static const double TireVerticalRankTolerance = 1.0e-8;
static const double TireVerticalReactionTolerance = 1.0e-9;
// Only roundoff-sized overlap is tolerated. An inconsistent initial geometry
// must not be turned into a physical separation velocity proportional to 1/dt.
static const double TireVerticalInitialGapTolerance = 1.0e-7;

static bool FiniteValues(const double* values, int count)
{
    for (int i = 0; i < count; ++i)
        if (!std::isfinite(values[i])) return false;
    return true;
}

bool TireVerticalStateIsFinite(const TireVerticalSolverState& state)
{
    return FiniteValues(state.body_position, 3) && FiniteValues(state.body_velocity, 3)
        && FiniteValues(state.suspension_extension_meters, 4)
        && FiniteValues(state.suspension_extension_velocity_meters_per_second, 4);
}

static bool FiniteSpring(const TireVerticalSpringConfiguration& spring)
{
    const double values[] = { spring.spring_constant_newtons_per_meter,
        spring.preload_newtons, spring.reference_position_meters,
        spring.minimum_position_meters, spring.maximum_position_meters,
        spring.bellcrank_ratio, spring.inertance_kilograms };
    return FiniteValues(values, 7);
}

static bool FiniteDamper(const TireVerticalDamperConfiguration& damper)
{
    const TireVerticalDamperSegmentConfiguration segments[] = {damper.bump, damper.rebound};
    for (const auto& segment : segments)
    {
        const double values[] = {segment.slow_coefficient_newtons_per_second_per_meter,
            segment.fast_coefficient_newtons_per_second_per_meter,
            segment.transition_velocity_meters_per_second, segment.slow_intercept_newtons,
            segment.fast_intercept_newtons};
        if (!FiniteValues(values, 5)) return false;
    }
    return std::isfinite(damper.efficiency);
}

static bool FiniteConfiguration(const TireVerticalSolverConfiguration& configuration)
{
    const double values[] = { configuration.total_mass_kilograms,
        configuration.roll_inertia_kilogram_meters_squared,
        configuration.pitch_inertia_kilogram_meters_squared,
        configuration.gravity_meters_per_second_squared };
    if (!FiniteValues(values, 4) || !FiniteValues(configuration.wheel_x_meters, 4)
        || !FiniteValues(configuration.wheel_y_meters, 4)
        || !FiniteValues(configuration.unsprung_mass_kilograms, 4)
        || !FiniteValues(configuration.external_body_generalized_force, 3)
        || !FiniteValues(configuration.external_wheel_generalized_force, 4)
        || configuration.chassis_contact_count < 0
        || configuration.chassis_contact_count > TIRE_VERTICAL_MAX_CHASSIS_CONTACTS) return false;
    for (const auto& corner : configuration.corner)
        if (!FiniteSpring(corner.spring) || !FiniteDamper(corner.damper)
            || !std::isfinite(corner.unloaded_radius_meters)) return false;
    for (const auto& axle : configuration.axle)
        if (!FiniteSpring(axle.heave_spring) || !FiniteDamper(axle.heave_damper)
            || !std::isfinite(axle.anti_roll_stiffness_newtons_per_meter)) return false;
    for (int i = 0; i < configuration.chassis_contact_count; ++i)
    {
        const auto& point = configuration.chassis_contact[i];
        if (!std::isfinite(point.x_meters) || !std::isfinite(point.y_meters)
            || !std::isfinite(point.z_meters)) return false;
    }
    return true;
}

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
            if (!std::isfinite(matrix[row * stride + column])) return 0;
            augmented[row][column] = matrix[row * stride + column];
        }
        if (!std::isfinite(right_hand_side[row])) return 0;
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
        if (!std::isfinite(largest) || largest < 1.0e-14)
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
        if (!std::isfinite(solution[row])) return 0;
    }
    return 1;
}

struct TireVerticalForceEvaluation
{
    double generalized_force[7];
    double jacobian_position[7][7];
    double jacobian_velocity[7][7];
    double suspension_force_newtons[4];
    double wheel_center_height_meters[4];
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
        evaluation.suspension_force_newtons[wheel] = suspension.force_newtons[wheel];
        evaluation.wheel_center_height_meters[wheel] = chassis_reference_height
            + configuration.corner[wheel].unloaded_radius_meters
            - state.suspension_extension_meters[wheel];
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

    if (!FiniteConfiguration(configuration)) return fail("configuration contains nonfinite values");
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
    if (configuration.chassis_contact_count < 0
        || configuration.chassis_contact_count > TIRE_VERTICAL_MAX_CHASSIS_CONTACTS)
    {
        return fail("chassis contact count is out of range");
    }
    for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
    {
        const TireVerticalChassisContactConfiguration& chassis_contact =
            configuration.chassis_contact[contact];
        if (!chassis_contact.enabled)
        {
            continue;
        }
        if (!std::isfinite(chassis_contact.x_meters)
            || !std::isfinite(chassis_contact.y_meters)
            || !std::isfinite(chassis_contact.z_meters)
            || std::fabs(chassis_contact.x_meters) > 10.0
            || std::fabs(chassis_contact.y_meters) > 10.0
            || std::fabs(chassis_contact.z_meters) > 2.0)
        {
            return fail("chassis contact geometry is invalid");
        }
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

static double TireVerticalTireGapFromPositions(
    const TireVerticalSolverConfiguration& configuration, int wheel,
    const TireVerticalSolverState& state, double ground_height_meters);

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
        const double ground = input.ground_height_meters[wheel]
            + input.ground_height_velocity_meters_per_second[wheel] * input.elapsed_seconds;
        diagnostics.tire_gap_meters[wheel] =
            TireVerticalTireGapFromPositions(configuration, wheel, state, ground);
        diagnostics.tire_contact_impulse_newton_seconds[wheel] = 0.0;
        diagnostics.tire_contact_support_seconds[wheel] = 0.0;
        diagnostics.tire_contact_supported_during_step[wheel] = 0;
        diagnostics.tire_contact_at_end[wheel] = 0;
        diagnostics.tire_normal_force_newtons[wheel] = 0.0;
        diagnostics.suspension_force_newtons[wheel] =
            evaluation.suspension_force_newtons[wheel];
        diagnostics.wheel_center_height_meters[wheel] =
            evaluation.wheel_center_height_meters[wheel];
        diagnostics.tire_contact[wheel] = 0;
    }
    for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
    {
        const TireVerticalChassisContactConfiguration& chassis_contact =
            configuration.chassis_contact[contact];
        if (!chassis_contact.enabled)
        {
            diagnostics.chassis_contact_gap_meters[contact] = 0.0;
            continue;
        }
        const double ground = input.chassis_ground_height_meters[contact]
            + input.chassis_ground_height_velocity_meters_per_second[contact]
                * input.elapsed_seconds;
        diagnostics.chassis_contact_gap_meters[contact] =
            state.body_position[0] + chassis_contact.z_meters
            - chassis_contact.x_meters * state.body_position[2]
            + chassis_contact.y_meters * state.body_position[1]
            - ground;
    }
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

static double TireVerticalLimitPosition(const TireVerticalSolverConfiguration& configuration,
    int wheel, int kind);

static void TireVerticalConstraintJacobian(const TireVerticalSolverConfiguration& configuration,
    int kind, int index, double jacobian[7])
{
    std::memset(jacobian, 0, sizeof(double) * 7);
    if (kind == TIRE_VERTICAL_CONSTRAINT_LOWER || kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
    {
        jacobian[3 + index] = 1.0;
        return;
    }
    if (kind == TIRE_VERTICAL_CONSTRAINT_CHASSIS)
    {
        const TireVerticalChassisContactConfiguration& contact =
            configuration.chassis_contact[index];
        jacobian[0] = 1.0;
        jacobian[1] = contact.y_meters;
        jacobian[2] = -contact.x_meters;
        return;
    }
    if (kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
    {
        jacobian[0] = 1.0;
        jacobian[1] = configuration.wheel_y_meters[index];
        jacobian[2] = -configuration.wheel_x_meters[index];
        jacobian[3 + index] = -1.0;
    }
}

static double TireVerticalEndGroundHeight(double ground_height_meters,
    double ground_velocity_meters_per_second, double elapsed_seconds)
{
    return ground_height_meters + ground_velocity_meters_per_second * elapsed_seconds;
}

static double TireVerticalTireGapFromPositions(
    const TireVerticalSolverConfiguration& configuration, int wheel,
    const TireVerticalSolverState& state, double ground_height_meters)
{
    return state.body_position[0]
        - configuration.wheel_x_meters[wheel] * state.body_position[2]
        + configuration.wheel_y_meters[wheel] * state.body_position[1]
        - state.suspension_extension_meters[wheel]
        - ground_height_meters;
}

static double TireVerticalTireNormalVelocity(
    const TireVerticalSolverConfiguration& configuration, int wheel,
    const TireVerticalSolverState& state, double ground_velocity_meters_per_second)
{
    return state.body_velocity[0]
        - configuration.wheel_x_meters[wheel] * state.body_velocity[2]
        + configuration.wheel_y_meters[wheel] * state.body_velocity[1]
        - state.suspension_extension_velocity_meters_per_second[wheel]
        - ground_velocity_meters_per_second;
}

static double TireVerticalChassisGapFromPositions(
    const TireVerticalSolverConfiguration& configuration, int index,
    const TireVerticalSolverState& state, double ground_height_meters)
{
    const TireVerticalChassisContactConfiguration& contact =
        configuration.chassis_contact[index];
    return state.body_position[0] + contact.z_meters
        - contact.x_meters * state.body_position[2]
        + contact.y_meters * state.body_position[1]
        - ground_height_meters;
}

static double TireVerticalChassisNormalVelocity(
    const TireVerticalSolverConfiguration& configuration, int index,
    const TireVerticalSolverState& state, double ground_velocity_meters_per_second)
{
    const TireVerticalChassisContactConfiguration& contact =
        configuration.chassis_contact[index];
    return state.body_velocity[0]
        + contact.y_meters * state.body_velocity[1]
        - contact.x_meters * state.body_velocity[2]
        - ground_velocity_meters_per_second;
}

static double TireVerticalConstraintResidual(const TireVerticalSolverConfiguration& configuration,
    int kind, int index, const TireVerticalSolverState& previous_state,
    const TireVerticalSolverState& state, const TireVerticalSolverStepInput& input)
{
    TireVerticalSolverState end_state;
    BuildStateAtEndOfStep(previous_state, state, input.elapsed_seconds, end_state);
    if (kind == TIRE_VERTICAL_CONSTRAINT_CHASSIS)
    {
        const double ground = TireVerticalEndGroundHeight(
            input.chassis_ground_height_meters[index],
            input.chassis_ground_height_velocity_meters_per_second[index],
            input.elapsed_seconds);
        const double initial_gap = TireVerticalChassisGapFromPositions(configuration, index,
            previous_state, input.chassis_ground_height_meters[index]);
        return (TireVerticalChassisGapFromPositions(configuration, index, end_state, ground)
            - std::min(initial_gap, 0.0)) / input.elapsed_seconds;
    }
    if (kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
    {
        const double ground = TireVerticalEndGroundHeight(
            input.ground_height_meters[index],
            input.ground_height_velocity_meters_per_second[index],
            input.elapsed_seconds);
        const double initial_gap = TireVerticalTireGapFromPositions(configuration, index,
            previous_state, input.ground_height_meters[index]);
        return (TireVerticalTireGapFromPositions(configuration, index, end_state, ground)
            - std::min(initial_gap, 0.0)) / input.elapsed_seconds;
    }
    const double limit = TireVerticalLimitPosition(configuration, index, kind);
    return (end_state.suspension_extension_meters[index] - limit) / input.elapsed_seconds;
}

struct TireVerticalConstraintBasis
{
    int indices[TireVerticalMaximumConstraintCount];
    int kinds[TireVerticalMaximumConstraintCount];
    int count;
};

static void TireVerticalBuildConstraintBasis(
    const TireVerticalSolverConfiguration& configuration,
    const int* active_indices, const int* active_kinds, int active_count,
    TireVerticalConstraintBasis& basis)
{
    double vectors[TireVerticalMaximumConstraintCount][7] = {};
    double lengths[TireVerticalMaximumConstraintCount] = {};
    int used[TireVerticalMaximumConstraintCount] = {};
    double orthonormal[7][7] = {};
    double maximum_length = 0.0;

    basis.count = 0;
    for (int constraint = 0; constraint < active_count; ++constraint)
    {
        TireVerticalConstraintJacobian(configuration, active_kinds[constraint],
            active_indices[constraint], vectors[constraint]);
        lengths[constraint] = VectorNorm(vectors[constraint], 7);
        maximum_length = std::max(maximum_length, lengths[constraint]);
    }
    const double tolerance = TireVerticalRankTolerance * std::max(maximum_length, 1.0);

    while (basis.count < 7)
    {
        int best = -1;
        double best_length = tolerance;
        // A newly violated affine inequality must enter the basis. Keeping
        // only the longest old rows can repeatedly discard a higher parallel
        // floor point and cycle forever between the same two support planes.
        if (!basis.count && active_count > 0 && lengths[active_count - 1] > tolerance)
        {
            best = active_count - 1;
            best_length = lengths[best];
        }
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            if (!basis.count && best >= 0) break;
            if (!used[constraint] && lengths[constraint] > best_length)
            {
                best = constraint;
                best_length = lengths[constraint];
            }
        }
        if (best < 0)
        {
            break;
        }
        used[best] = 1;
        basis.indices[basis.count] = active_indices[best];
        basis.kinds[basis.count] = active_kinds[best];
        for (int component = 0; component < 7; ++component)
        {
            orthonormal[basis.count][component] = vectors[best][component] / best_length;
        }
        basis.count += 1;

        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            if (used[constraint])
            {
                continue;
            }
            double projection = 0.0;
            for (int component = 0; component < 7; ++component)
            {
                projection += vectors[constraint][component]
                    * orthonormal[basis.count - 1][component];
            }
            for (int component = 0; component < 7; ++component)
            {
                vectors[constraint][component] -=
                    projection * orthonormal[basis.count - 1][component];
            }
            lengths[constraint] = VectorNorm(vectors[constraint], 7);
        }
    }
}


static int TireVerticalReconstructReactions(
    const TireVerticalSolverConfiguration& configuration,
    const int* active_indices, const int* active_kinds, int active_count,
    const TireVerticalConstraintBasis& basis, const double* basis_multipliers,
    double* reactions)
{
    double required[7] = {};
    for (int entry = 0; entry < basis.count; ++entry)
    {
        double row[7] = {};
        TireVerticalConstraintJacobian(configuration, basis.kinds[entry], basis.indices[entry], row);
        for (int j = 0; j < 7; ++j) required[j] += row[j] * basis_multipliers[entry];
    }
    double columns[TireVerticalMaximumConstraintCount][7] = {};
    int signs[TireVerticalMaximumConstraintCount] = {};
    bool free_column[TireVerticalMaximumConstraintCount] = {};
    for (int i = 0; i < active_count; ++i)
    {
        TireVerticalConstraintJacobian(configuration, active_kinds[i], active_indices[i], columns[i]);
        signs[i] = active_kinds[i] == TIRE_VERTICAL_CONSTRAINT_UPPER ? -1 : 1;
        for (double& v : columns[i]) v *= signs[i];
        free_column[i] = true;
        reactions[i] = 0.0;
    }
    // Work in the actual column span. A 7x7 normal matrix is singular for
    // every contact set with rank below seven, including a single floor point.
    for (int iteration = 0; iteration <= active_count; ++iteration)
    {
        double orthogonal[7][7] = {};
        int rank = 0;
        for (int i = 0; i < active_count; ++i)
        {
            if (!free_column[i]) continue;
            double v[7];
            std::memcpy(v, columns[i], sizeof(v));
            for (int pass = 0; pass < 2; ++pass)
                for (int k = 0; k < rank; ++k)
                {
                    double dot = 0.0;
                    for (int j = 0; j < 7; ++j) dot += v[j] * orthogonal[k][j];
                    for (int j = 0; j < 7; ++j) v[j] -= dot * orthogonal[k][j];
                }
            double norm = VectorNorm(v, 7);
            if (norm > TireVerticalRankTolerance && rank < 7)
            {
                for (int j = 0; j < 7; ++j) orthogonal[rank][j] = v[j] / norm;
                ++rank;
            }
        }
        if (!rank) return VectorNorm(required, 7) <= TireVerticalReactionTolerance;
        double projected[TireVerticalMaximumConstraintCount][7] = {};
        double rhs[7] = {}, remaining[7];
        std::memcpy(remaining, required, sizeof(remaining));
        for (int k = 0; k < rank; ++k)
        {
            for (int j = 0; j < 7; ++j) rhs[k] += orthogonal[k][j] * required[j];
            for (int j = 0; j < 7; ++j) remaining[j] -= rhs[k] * orthogonal[k][j];
            for (int i = 0; i < active_count; ++i)
                if (free_column[i])
                    for (int j = 0; j < 7; ++j)
                        projected[i][k] += orthogonal[k][j] * columns[i][j];
        }
        const double closure_tolerance = 1.0e-8 * std::max(1.0, VectorNorm(required, 7));
        if (VectorNorm(remaining, 7) > closure_tolerance) return 0;
        double normal[7][7] = {};
        for (int k = 0; k < rank; ++k)
            for (int l = 0; l < rank; ++l)
                for (int i = 0; i < active_count; ++i)
                    if (free_column[i]) normal[k][l] += projected[i][k] * projected[i][l];
        double dual[7] = {};
        if (!SolveLinearSystem(&normal[0][0], 7, rhs, dual, rank)) return 0;
        double magnitude[TireVerticalMaximumConstraintCount] = {};
        int negative = -1;
        double smallest = -TireVerticalReactionTolerance;
        for (int i = 0; i < active_count; ++i)
            if (free_column[i])
            {
                for (int k = 0; k < rank; ++k) magnitude[i] += projected[i][k] * dual[k];
                if (magnitude[i] < smallest) { smallest = magnitude[i]; negative = i; }
            }
        if (negative >= 0) { free_column[negative] = false; continue; }
        double reconstructed[7] = {};
        for (int i = 0; i < active_count; ++i)
        {
            reactions[i] = signs[i] * magnitude[i];
            for (int j = 0; j < 7; ++j) reconstructed[j] += columns[i][j] * magnitude[i];
        }
        for (int j = 0; j < 7; ++j) reconstructed[j] -= required[j];
        return FiniteValues(reactions, active_count)
            && VectorNorm(reconstructed, 7) <= closure_tolerance;
    }
    return 0;
}
static double TireVerticalComputeResidual(const TireVerticalSolverConfiguration& configuration,
    const double mass_matrix[7][7], const TireVerticalSolverState& previous_state,
    const TireVerticalSolverState& trial_state, const TireVerticalSolverStepInput& input,
    const TireVerticalConstraintBasis& basis, const double* multipliers, double* residual)
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
    for (int constraint = 0; constraint < basis.count; ++constraint)
    {
        double jacobian[7] = {};
        TireVerticalConstraintJacobian(configuration, basis.kinds[constraint],
            basis.indices[constraint], jacobian);
        for (int row = 0; row < 7; ++row)
        {
            residual[row] -= input.elapsed_seconds * jacobian[row]
                * multipliers[constraint];
        }
    }
    for (int constraint = 0; constraint < basis.count; ++constraint)
    {
        residual[7 + constraint] = TireVerticalConstraintResidual(configuration,
            basis.kinds[constraint], basis.indices[constraint], previous_state,
            trial_state, input);
    }
    return VectorNorm(residual, 7 + basis.count);
}

static int SolveNewtonSystem(const TireVerticalSolverConfiguration& configuration,
    const double mass_matrix[7][7], const TireVerticalSolverState& previous_state,
    const TireVerticalSolverStepInput& input, const int* active_indices,
    const int* active_kinds, int active_count, TireVerticalSolverState& state,
    double* multipliers, double* residual_norm, int* iterations)
{
    TireVerticalConstraintBasis basis;
    TireVerticalBuildConstraintBasis(configuration, active_indices, active_kinds,
        active_count, basis);

    double basis_multipliers[TireVerticalMaximumConstraintCount] = {};
    for (int entry = 0; entry < basis.count; ++entry)
    {
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            if (active_kinds[constraint] == basis.kinds[entry]
                && active_indices[constraint] == basis.indices[entry])
            {
                basis_multipliers[entry] = multipliers[constraint];
                break;
            }
        }
    }

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
        for (int constraint = 0; constraint < basis.count; ++constraint)
        {
            double jacobian[7] = {};
            TireVerticalConstraintJacobian(configuration, basis.kinds[constraint],
                basis.indices[constraint], jacobian);
            for (int row = 0; row < 7; ++row)
            {
                residual[row] -= input.elapsed_seconds * jacobian[row]
                    * basis_multipliers[constraint];
            }
        }
        for (int constraint = 0; constraint < basis.count; ++constraint)
        {
            residual[7 + constraint] = TireVerticalConstraintResidual(configuration,
                basis.kinds[constraint], basis.indices[constraint], previous_state,
                state, input);
        }

        const double norm = VectorNorm(residual, 7 + basis.count);
        *residual_norm = norm;
        if (!std::isfinite(norm) || !TireVerticalStateIsFinite(evaluation_state)) return 0;
        const double residual_tolerance = configuration.chassis_contact_count > 0
            ? TireVerticalChassisResidualTolerance : TireVerticalResidualTolerance;
        if (norm <= residual_tolerance)
        {
            *iterations = total_iterations;
            if (!TireVerticalReconstructReactions(configuration, active_indices, active_kinds,
                    active_count, basis, basis_multipliers, multipliers))
            {
                /* No admissible minimum-norm reconstruction with the current
                 * active set.  Keep the independent basis solution so the
                 * active-set loop can remove contacts with the wrong sign or
                 * revise the set instead of failing the whole interval. */
                for (int constraint = 0; constraint < active_count; ++constraint)
                {
                    multipliers[constraint] = 0.0;
                }
                for (int entry = 0; entry < basis.count; ++entry)
                {
                    for (int constraint = 0; constraint < active_count; ++constraint)
                    {
                        if (active_kinds[constraint] == basis.kinds[entry]
                            && active_indices[constraint] == basis.indices[entry])
                        {
                            multipliers[constraint] = basis_multipliers[entry];
                            break;
                        }
                    }
                }
            }
            return 1;
        }

        const int system_size = 7 + basis.count;
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
        for (int constraint = 0; constraint < basis.count; ++constraint)
        {
            double constraint_jacobian[7] = {};
            TireVerticalConstraintJacobian(configuration, basis.kinds[constraint],
                basis.indices[constraint], constraint_jacobian);
            for (int row = 0; row < 7; ++row)
            {
                jacobian[row][7 + constraint] -= input.elapsed_seconds
                    * constraint_jacobian[row];
                jacobian[7 + constraint][row] += constraint_jacobian[row];
            }
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
            for (int constraint = 0; constraint < basis.count; ++constraint)
            {
                trial_multipliers[constraint] = basis_multipliers[constraint]
                    + step_scale * delta[7 + constraint];
            }

            double trial_residual[TireVerticalMaximumSystemSize] = {};
            const double trial_norm = TireVerticalComputeResidual(configuration, mass_matrix,
                previous_state, trial_state, input, basis, trial_multipliers, trial_residual);
            if (std::isfinite(trial_norm) && TireVerticalStateIsFinite(trial_state)
                && trial_norm < norm)
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
        for (int constraint = 0; constraint < basis.count; ++constraint)
        {
            basis_multipliers[constraint] = trial_multipliers[constraint];
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
    const TireVerticalSolverState& previous_state, const TireVerticalSolverState& next_state,
    const TireVerticalSolverStepInput& input)
{
    double loss = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double bellcrank = configuration.corner[wheel].spring.bellcrank_ratio;
        const double average_extension_velocity = 0.5
            * (previous_state.suspension_extension_velocity_meters_per_second[wheel]
                + next_state.suspension_extension_velocity_meters_per_second[wheel]);
        const double velocity = -bellcrank * average_extension_velocity;
        const double damper_force = TireVerticalDamperForce(configuration.corner[wheel].damper,
            velocity);
        loss += -bellcrank * damper_force * average_extension_velocity
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
        const double average_heave_velocity = -0.25
            * (left_bellcrank
                    * (previous_state.suspension_extension_velocity_meters_per_second[left_wheel]
                        + next_state.suspension_extension_velocity_meters_per_second[left_wheel])
                + right_bellcrank
                    * (previous_state.suspension_extension_velocity_meters_per_second[right_wheel]
                        + next_state.suspension_extension_velocity_meters_per_second[right_wheel]));
        const double heave_damper_force = TireVerticalDamperForce(
            axle_configuration.heave_damper, average_heave_velocity);
        const double heave_bellcrank = axle_configuration.heave_spring.bellcrank_ratio;
        const double left_force = 0.5 * heave_bellcrank * left_bellcrank
            * heave_damper_force;
        const double right_force = 0.5 * heave_bellcrank * right_bellcrank
            * heave_damper_force;
        const double average_left_velocity = 0.5
            * (previous_state.suspension_extension_velocity_meters_per_second[left_wheel]
                + next_state.suspension_extension_velocity_meters_per_second[left_wheel]);
        const double average_right_velocity = 0.5
            * (previous_state.suspension_extension_velocity_meters_per_second[right_wheel]
                + next_state.suspension_extension_velocity_meters_per_second[right_wheel]);
        loss += -(left_force * average_left_velocity
            + right_force * average_right_velocity) * input.elapsed_seconds;
    }
    return std::max(0.0, loss);
}

int TireVerticalSolverStep(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& previous_state, const TireVerticalSolverStepInput& input,
    TireVerticalSolverState& next_state, TireVerticalSolverDiagnostics& diagnostics)
{
    std::memset(&diagnostics, 0, sizeof(diagnostics));
    next_state = previous_state;
    diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_NUMERICAL;
    if (!FiniteConfiguration(configuration) || !TireVerticalStateIsFinite(previous_state)
        || !std::isfinite(input.elapsed_seconds) || input.elapsed_seconds < 0.0
        || !FiniteValues(input.ground_height_meters, 4)
        || !FiniteValues(input.ground_height_velocity_meters_per_second, 4)
        || !FiniteValues(input.chassis_ground_height_meters, configuration.chassis_contact_count)
        || !FiniteValues(input.chassis_ground_height_velocity_meters_per_second,
            configuration.chassis_contact_count))
    {
        diagnostics.failed = 1;
        diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_NONFINITE;
        return 0;
    }
    if (!(input.elapsed_seconds > 0.0))
    {
        next_state = previous_state;
        diagnostics.converged = 1;
        diagnostics.no_op = 1;
        diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_NONE;
        return 1;
    }

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double extension = previous_state.suspension_extension_meters[wheel];
        if (TireVerticalTireGapFromPositions(configuration, wheel, previous_state,
                input.ground_height_meters[wheel]) < -TireVerticalInitialGapTolerance
            || extension < TireVerticalLimitPosition(configuration, wheel,
                TIRE_VERTICAL_CONSTRAINT_LOWER) - TireVerticalInitialGapTolerance
            || extension > TireVerticalLimitPosition(configuration, wheel,
                TIRE_VERTICAL_CONSTRAINT_UPPER) + TireVerticalInitialGapTolerance)
        {
            diagnostics.failed = 1;
            diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_INITIAL_GEOMETRY;
            return 0;
        }
    }
    for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
    {
        if (configuration.chassis_contact[contact].enabled
            && TireVerticalChassisGapFromPositions(configuration, contact, previous_state,
                input.chassis_ground_height_meters[contact]) < -TireVerticalInitialGapTolerance)
        {
            diagnostics.failed = 1;
            diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_INITIAL_GEOMETRY;
            return 0;
        }
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
        diagnostics.failed = 1;
        return 0;
    }

    const double elapsed = input.elapsed_seconds;
    TireVerticalSolverState state = previous_state;
    int active_indices[TireVerticalMaximumConstraintCount] = {};
    int active_kinds[TireVerticalMaximumConstraintCount] = {};
    double multipliers[TireVerticalMaximumConstraintCount] = {};
    int active_count = 0;
    int total_iterations = 0;
    double residual_norm = 0.0;

    const auto find_active = [&](int kind, int index) -> int
    {
        for (int constraint = 0; constraint < active_count; ++constraint)
        {
            if (active_kinds[constraint] == kind && active_indices[constraint] == index)
            {
                return constraint;
            }
        }
        return -1;
    };
    const auto add_active = [&](int kind, int index) -> int
    {
        if (find_active(kind, index) >= 0)
        {
            return 1;
        }
        if (active_count >= TireVerticalMaximumConstraintCount)
        {
            return 0;
        }
        active_indices[active_count] = index;
        active_kinds[active_count] = kind;
        multipliers[active_count] = 0.0;
        active_count += 1;
        return 1;
    };
    const auto remove_active = [&](int position)
    {
        for (int index = position; index < active_count - 1; ++index)
        {
            active_indices[index] = active_indices[index + 1];
            active_kinds[index] = active_kinds[index + 1];
            multipliers[index] = multipliers[index + 1];
        }
        active_count -= 1;
    };

    /* Seed the active set with constraints that already touch at the start of
     * the interval.  Everything else is added by the feasibility loop. */
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double gap = TireVerticalTireGapFromPositions(configuration, wheel,
            previous_state, input.ground_height_meters[wheel]);
        const double normal_velocity = TireVerticalTireNormalVelocity(configuration, wheel,
            previous_state, input.ground_height_velocity_meters_per_second[wheel]);
        if (gap <= 1.0e-6 && normal_velocity <= 0.0
            && !add_active(TIRE_VERTICAL_CONSTRAINT_TIRE, wheel))
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            return 0;
        }
    }
    for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
    {
        if (!configuration.chassis_contact[contact].enabled)
        {
            continue;
        }
        const double gap = TireVerticalChassisGapFromPositions(configuration, contact,
            previous_state, input.chassis_ground_height_meters[contact]);
        const double normal_velocity = TireVerticalChassisNormalVelocity(configuration,
            contact, previous_state, input.chassis_ground_height_velocity_meters_per_second[contact]);
        if (gap <= 1.0e-6 && normal_velocity <= 0.0
            && !add_active(TIRE_VERTICAL_CONSTRAINT_CHASSIS, contact))
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            return 0;
        }
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double position = previous_state.suspension_extension_meters[wheel];
        const double lower = TireVerticalLimitPosition(configuration, wheel,
            TIRE_VERTICAL_CONSTRAINT_LOWER);
        const double upper = TireVerticalLimitPosition(configuration, wheel,
            TIRE_VERTICAL_CONSTRAINT_UPPER);
        if (position <= lower + TireVerticalLimitToleranceMeters
            && !add_active(TIRE_VERTICAL_CONSTRAINT_LOWER, wheel))
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            return 0;
        }
        if (position >= upper - TireVerticalLimitToleranceMeters
            && !add_active(TIRE_VERTICAL_CONSTRAINT_UPPER, wheel))
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            return 0;
        }
    }

    int converged_active_set = 0;
    for (int active_set_iteration = 0;
        active_set_iteration < TireVerticalMaximumActiveSetIterations;
        ++active_set_iteration)
    {
                int iterations = 0;
        const int solved = SolveNewtonSystem(configuration, mass_matrix, previous_state,
            input, active_indices, active_kinds, active_count, state, multipliers,
            &residual_norm, &iterations);
        total_iterations += iterations;
        if (!solved)
        {
            next_state = previous_state;
            diagnostics.newton_iterations = total_iterations;
            diagnostics.residual_norm = residual_norm;
            diagnostics.failed = 1;
            return 0;
        }

        TireVerticalSolverState end_state;
        BuildStateAtEndOfStep(previous_state, state, elapsed, end_state);

        int changed = 0;

        for (int constraint = active_count - 1; constraint >= 0; --constraint)
        {
            const int kind = active_kinds[constraint];
            const int index = active_indices[constraint];
            int valid = 0;
            if (kind == TIRE_VERTICAL_CONSTRAINT_LOWER
                || kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
            {
                const double limit = TireVerticalLimitPosition(configuration, index, kind);
                const double position = end_state.suspension_extension_meters[index];
                const int valid_sign = kind == TIRE_VERTICAL_CONSTRAINT_LOWER
                    ? multipliers[constraint] >= -TireVerticalMultiplierTolerance
                    : multipliers[constraint] <= TireVerticalMultiplierTolerance;
                valid = valid_sign && std::fabs(position - limit) <= 1.0e-8;
            }
            else if (kind == TIRE_VERTICAL_CONSTRAINT_CHASSIS)
            {
                const double ground = TireVerticalEndGroundHeight(
                    input.chassis_ground_height_meters[index],
                    input.chassis_ground_height_velocity_meters_per_second[index], elapsed);
                const double gap = TireVerticalChassisGapFromPositions(configuration,
                    index, end_state, ground);
                valid = multipliers[constraint] >= -TireVerticalMultiplierTolerance
                    && std::fabs(gap) <= 1.0e-7;
            }
            else if (kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
            {
                const double ground = TireVerticalEndGroundHeight(
                    input.ground_height_meters[index],
                    input.ground_height_velocity_meters_per_second[index], elapsed);
                const double gap = TireVerticalTireGapFromPositions(configuration,
                    index, end_state, ground);
                valid = multipliers[constraint] >= -TireVerticalMultiplierTolerance
                    && std::fabs(gap) <= 1.0e-7;
            }
            if (!valid)
            {

                remove_active(constraint);
                changed = 1;
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
            const double position = end_state.suspension_extension_meters[wheel];
            if (position < lower - TireVerticalLimitToleranceMeters
                && find_active(TIRE_VERTICAL_CONSTRAINT_LOWER, wheel) < 0)
            {
                if (!add_active(TIRE_VERTICAL_CONSTRAINT_LOWER, wheel))
                {
                    next_state = previous_state;
                    diagnostics.failed = 1;
                    return 0;
                }
                state.suspension_extension_velocity_meters_per_second[wheel] =
                    (lower - previous_state.suspension_extension_meters[wheel]) / elapsed;
                changed = 1;
            }
            if (position > upper + TireVerticalLimitToleranceMeters
                && find_active(TIRE_VERTICAL_CONSTRAINT_UPPER, wheel) < 0)
            {
                if (!add_active(TIRE_VERTICAL_CONSTRAINT_UPPER, wheel))
                {
                    next_state = previous_state;
                    diagnostics.failed = 1;
                    return 0;
                }
                state.suspension_extension_velocity_meters_per_second[wheel] =
                    (upper - previous_state.suspension_extension_meters[wheel]) / elapsed;
                changed = 1;
            }
        }
        if (changed)
        {
            continue;
        }

        for (int wheel = 0; wheel < 4; ++wheel)
        {
            if (find_active(TIRE_VERTICAL_CONSTRAINT_TIRE, wheel) >= 0)
            {
                continue;
            }
            const double ground_previous = input.ground_height_meters[wheel];
            const double ground_end = TireVerticalEndGroundHeight(ground_previous,
                input.ground_height_velocity_meters_per_second[wheel], elapsed);
            const double gap_previous = TireVerticalTireGapFromPositions(configuration,
                wheel, previous_state, ground_previous);
            const double gap_end = TireVerticalTireGapFromPositions(configuration,
                wheel, end_state, ground_end);
            // The constraint residual preserves a roundoff-sized initial
            // overlap. Test feasibility in that same affine frame. A contact
            // released for tensile reaction must not be re-added just because
            // the *previous* gap was slightly negative while it separates.
            const double constraint_gap = gap_end - std::min(gap_previous, 0.0);
            const double normal_velocity = TireVerticalTireNormalVelocity(configuration,
                wheel, state, input.ground_height_velocity_meters_per_second[wheel]);
            if (constraint_gap < -TireVerticalLimitToleranceMeters
                || (constraint_gap <= TireVerticalLimitToleranceMeters && normal_velocity < 0.0))
            {
                if (!add_active(TIRE_VERTICAL_CONSTRAINT_TIRE, wheel))
                {
                    next_state = previous_state;
                    diagnostics.failed = 1;
                    return 0;
                }
                changed = 1;
            }
        }
        if (changed)
        {
            continue;
        }

        for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
        {
            if (!configuration.chassis_contact[contact].enabled
                || find_active(TIRE_VERTICAL_CONSTRAINT_CHASSIS, contact) >= 0)
            {
                continue;
            }
            const double ground_previous = input.chassis_ground_height_meters[contact];
            const double ground_end = TireVerticalEndGroundHeight(ground_previous,
                input.chassis_ground_height_velocity_meters_per_second[contact], elapsed);
            const double gap_previous = TireVerticalChassisGapFromPositions(configuration,
                contact, previous_state, ground_previous);
            const double gap_end = TireVerticalChassisGapFromPositions(configuration,
                contact, end_state, ground_end);
            const double constraint_gap = gap_end - std::min(gap_previous, 0.0);
            const double normal_velocity = TireVerticalChassisNormalVelocity(configuration,
                contact, state,
                input.chassis_ground_height_velocity_meters_per_second[contact]);
            if (constraint_gap < -TireVerticalLimitToleranceMeters
                || (constraint_gap <= TireVerticalLimitToleranceMeters && normal_velocity < 0.0))
            {
                if (!add_active(TIRE_VERTICAL_CONSTRAINT_CHASSIS, contact))
                {
                    next_state = previous_state;
                    diagnostics.failed = 1;
                    return 0;
                }
                changed = 1;
            }
        }
        if (!changed)
        {
            converged_active_set = 1;
            break;
        }
    }

    if (!converged_active_set)
    {
                next_state = previous_state;
        diagnostics.newton_iterations = total_iterations;
        diagnostics.residual_norm = residual_norm;
        diagnostics.failed = 1;
        diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_ACTIVE_SET;
        diagnostics.active_constraint_count = active_count;
        for (int i = 0; i < active_count; ++i)
        {
            diagnostics.active_constraint_kind[i] = active_kinds[i];
            diagnostics.active_constraint_index[i] = active_indices[i];
            diagnostics.constraint_impulse_newton_seconds[i] = elapsed * multipliers[i];
        }
        return 0;
    }

    TireVerticalSolverState end_state;
    BuildStateAtEndOfStep(previous_state, state, elapsed, end_state);
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double lower = TireVerticalLimitPosition(configuration, wheel,
            TIRE_VERTICAL_CONSTRAINT_LOWER);
        const double upper = TireVerticalLimitPosition(configuration, wheel,
            TIRE_VERTICAL_CONSTRAINT_UPPER);
        if (end_state.suspension_extension_meters[wheel] < lower - 1.0e-7
            || end_state.suspension_extension_meters[wheel] > upper + 1.0e-7)
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            return 0;
        }
    }
    for (int constraint = 0; constraint < active_count; ++constraint)
    {
        const int kind = active_kinds[constraint];
        const double multiplier = multipliers[constraint];
        if ((kind == TIRE_VERTICAL_CONSTRAINT_LOWER
                    && multiplier < -TireVerticalMultiplierTolerance)
                || (kind == TIRE_VERTICAL_CONSTRAINT_UPPER
                    && multiplier > TireVerticalMultiplierTolerance)
                || ((kind == TIRE_VERTICAL_CONSTRAINT_TIRE
                        || kind == TIRE_VERTICAL_CONSTRAINT_CHASSIS)
                    && multiplier < -TireVerticalMultiplierTolerance))
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            return 0;
        }
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double ground = TireVerticalEndGroundHeight(input.ground_height_meters[wheel],
            input.ground_height_velocity_meters_per_second[wheel], elapsed);
        const double gap = TireVerticalTireGapFromPositions(configuration, wheel,
            end_state, ground);
        const int active = find_active(TIRE_VERTICAL_CONSTRAINT_TIRE, wheel) >= 0;
        if ((active && gap < -1.0e-5) || (!active && gap < -1.0e-4))
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            return 0;
        }
    }
    for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
    {
        if (!configuration.chassis_contact[contact].enabled)
        {
            continue;
        }
        const double ground = TireVerticalEndGroundHeight(
            input.chassis_ground_height_meters[contact],
            input.chassis_ground_height_velocity_meters_per_second[contact], elapsed);
        const double gap = TireVerticalChassisGapFromPositions(configuration, contact,
            end_state, ground);
        if (!std::isfinite(gap) || gap < -TireVerticalInitialGapTolerance)
        {
            next_state = previous_state;
            diagnostics.failed = 1;
            diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_FEASIBILITY;
            return 0;
        }
    }

    if (!TireVerticalStateIsFinite(end_state) || !FiniteValues(multipliers, active_count))
    {
        diagnostics.failed = 1;
        diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_NONFINITE;
        return 0;
    }
    next_state = end_state;

    TireVerticalForceEvaluation evaluation;
    EvaluateForces(configuration, next_state, input, evaluation);
    TireVerticalSuspensionEvaluation suspension;
    EvaluateSuspension(configuration, next_state.suspension_extension_meters,
        next_state.suspension_extension_velocity_meters_per_second, suspension);
    FillDiagnostics(configuration, next_state, input, suspension, evaluation, diagnostics);

    diagnostics.converged = 1;
    diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_NONE;
    diagnostics.newton_iterations = total_iterations;
    diagnostics.residual_norm = residual_norm;
    diagnostics.active_constraint_count = active_count;
    double constraint_loss = 0.0;
    double constraint_work = 0.0;
    double chassis_loss = 0.0;
    double chassis_work = 0.0;
    double chassis_ground_work = 0.0;
    double tire_loss = 0.0;
    double tire_work = 0.0;
    double tire_ground_work = 0.0;
    for (int constraint = 0; constraint < active_count; ++constraint)
    {
        const int kind = active_kinds[constraint];
        const int index = active_indices[constraint];
        diagnostics.active_constraint_index[constraint] = index;
        diagnostics.active_constraint_kind[constraint] = kind;
        diagnostics.constraint_impulse_newton_seconds[constraint] = elapsed * multipliers[constraint];
        if (kind == TIRE_VERTICAL_CONSTRAINT_CHASSIS)
        {
            diagnostics.active_constraint_wheel[constraint] = -1;
            const TireVerticalChassisContactConfiguration& contact =
                configuration.chassis_contact[index];
            const double previous_body_velocity = previous_state.body_velocity[0]
                + contact.y_meters * previous_state.body_velocity[1]
                - contact.x_meters * previous_state.body_velocity[2];
            const double next_body_velocity = next_state.body_velocity[0]
                + contact.y_meters * next_state.body_velocity[1]
                - contact.x_meters * next_state.body_velocity[2];
            const double average_velocity = 0.5
                * (previous_body_velocity + next_body_velocity);
            const double work = elapsed * multipliers[constraint] * average_velocity;
            chassis_loss += std::max(0.0, -work);
            chassis_work += work;
            chassis_ground_work += elapsed * multipliers[constraint]
                * input.chassis_ground_height_velocity_meters_per_second[index];

            const bool gap_ok = diagnostics.chassis_contact_gap_meters[index] >= -1.0e-5;
            diagnostics.chassis_contact_active[index] =
                (multipliers[constraint] >= -TireVerticalMultiplierTolerance
                    && gap_ok) ? 1 : 0;
            diagnostics.chassis_contact_impulse_newton_seconds[index] =
                diagnostics.chassis_contact_active[index]
                    ? elapsed * multipliers[constraint] : 0.0;
        }
        else if (kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
        {
            diagnostics.active_constraint_wheel[constraint] = index;
            const double previous_normal_velocity = TireVerticalTireNormalVelocity(
                configuration, index, previous_state,
                input.ground_height_velocity_meters_per_second[index]);
            const double next_normal_velocity = TireVerticalTireNormalVelocity(
                configuration, index, next_state,
                input.ground_height_velocity_meters_per_second[index]);
            const double average_velocity = 0.5
                * (previous_normal_velocity + next_normal_velocity);
            const double work = elapsed * multipliers[constraint] * average_velocity;
            tire_loss += std::max(0.0, -work);
            tire_work += work;
            tire_ground_work += elapsed * multipliers[constraint]
                * input.ground_height_velocity_meters_per_second[index];
            diagnostics.tire_contact_impulse_newton_seconds[index] =
                elapsed * multipliers[constraint];
            diagnostics.tire_contact_support_seconds[index] = elapsed;
            diagnostics.tire_contact_supported_during_step[index] = 1;
            diagnostics.tire_normal_force_newtons[index] = multipliers[constraint];
        }
        else
        {
            diagnostics.active_constraint_wheel[constraint] = index;
            const double average_velocity = 0.5
                * (previous_state.suspension_extension_velocity_meters_per_second[index]
                    + next_state.suspension_extension_velocity_meters_per_second[index]);
            const double work = elapsed * multipliers[constraint] * average_velocity;
            constraint_loss += std::max(0.0, -work);
            constraint_work += work;
        }
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double ground = TireVerticalEndGroundHeight(input.ground_height_meters[wheel],
            input.ground_height_velocity_meters_per_second[wheel], elapsed);
        const double gap = TireVerticalTireGapFromPositions(configuration, wheel,
            next_state, ground);
        diagnostics.tire_gap_meters[wheel] = gap;
        diagnostics.tire_contact_at_end[wheel] = gap <= 1.0e-5 ? 1 : 0;
        diagnostics.tire_contact[wheel] = diagnostics.tire_contact_at_end[wheel];
        if (diagnostics.tire_contact_supported_during_step[wheel])
        {
            diagnostics.tire_contact[wheel] = 1;
        }
    }
    diagnostics.constraint_energy_loss_joules = constraint_loss;
    diagnostics.constraint_work_joules = constraint_work;
    diagnostics.chassis_contact_energy_loss_joules = chassis_loss;
    diagnostics.chassis_contact_work_joules = chassis_work;
    diagnostics.chassis_contact_ground_work_joules = chassis_ground_work;
    diagnostics.tire_contact_energy_loss_joules = tire_loss;
    diagnostics.tire_contact_work_joules = tire_work;
    diagnostics.tire_contact_ground_work_joules = tire_ground_work;
    diagnostics.damper_energy_loss_joules =
        TireVerticalCornerDamperLoss(configuration, previous_state, next_state, input);
    diagnostics.tire_damping_energy_loss_joules = 0.0;
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

static int SolveVerticalEquilibrium(const TireVerticalSolverConfiguration& configuration,
    const double ground_height_meters[4], const int active_contact[4],
    TireVerticalSolverState& state, double normal_force_newtons[4],
    double gap_meters[4], double* residual_norm, int* iterations)
{
    double q[7] = {
        state.body_position[0], state.body_position[1], state.body_position[2],
        state.suspension_extension_meters[0], state.suspension_extension_meters[1],
        state.suspension_extension_meters[2], state.suspension_extension_meters[3]
    };
    TireVerticalSolverStepInput static_input = {};
    static_input.elapsed_seconds = 1.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        static_input.ground_height_meters[wheel] = ground_height_meters[wheel];
    }

    for (int iteration = 0; iteration < 80; ++iteration)
    {
        TireVerticalSolverState trial_state = state;
        trial_state.body_position[0] = q[0];
        trial_state.body_position[1] = q[1];
        trial_state.body_position[2] = q[2];
        for (int body = 0; body < 3; ++body)
        {
            trial_state.body_velocity[body] = 0.0;
        }
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            trial_state.suspension_extension_meters[wheel] = q[3 + wheel];
            trial_state.suspension_extension_velocity_meters_per_second[wheel] = 0.0;
        }
        TireVerticalForceEvaluation evaluation;
        EvaluateForces(configuration, trial_state, static_input, evaluation);
        TireVerticalSuspensionEvaluation suspension;
        EvaluateSuspension(configuration, trial_state.suspension_extension_meters,
            trial_state.suspension_extension_velocity_meters_per_second, suspension);

        double residual[7] = {};
        double sprung_weight = configuration.total_mass_kilograms;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            sprung_weight -= configuration.unsprung_mass_kilograms[wheel];
        }
        residual[0] = -sprung_weight * configuration.gravity_meters_per_second_squared;
        residual[1] = 0.0;
        residual[2] = 0.0;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            const double force = suspension.force_newtons[wheel];
            residual[0] += force;
            residual[1] += configuration.wheel_y_meters[wheel] * force;
            residual[2] -= configuration.wheel_x_meters[wheel] * force;
        }
        for (int body = 0; body < 3; ++body)
        {
            residual[body] += configuration.external_body_generalized_force[body];
        }
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            gap_meters[wheel] = TireVerticalTireGapFromPositions(configuration, wheel,
                trial_state, ground_height_meters[wheel]);
            normal_force_newtons[wheel] = active_contact[wheel]
                ? evaluation.generalized_force[3 + wheel] : 0.0;
            residual[3 + wheel] = active_contact[wheel]
                ? gap_meters[wheel] : evaluation.generalized_force[3 + wheel];
        }
        const double norm = VectorNorm(residual, 7);
        *residual_norm = norm;
        if (norm <= 1.0e-7)
        {
            state = trial_state;
            for (int body = 0; body < 3; ++body)
            {
                state.body_velocity[body] = 0.0;
            }
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                state.suspension_extension_velocity_meters_per_second[wheel] = 0.0;
            }
            *iterations = iteration;
            return 1;
        }

        double jacobian[7][7] = {};
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            for (int column = 0; column < 7; ++column)
            {
                jacobian[3 + wheel][column] = evaluation.jacobian_position[3 + wheel][column];
            }
        }
        for (int extension = 0; extension < 4; ++extension)
        {
            double force_derivative = 0.0;
            double roll_derivative = 0.0;
            double pitch_derivative = 0.0;
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                const double derivative = suspension.force_derivative_extension[wheel][extension];
                force_derivative += derivative;
                roll_derivative += configuration.wheel_y_meters[wheel] * derivative;
                pitch_derivative -= configuration.wheel_x_meters[wheel] * derivative;
            }
            jacobian[0][3 + extension] = force_derivative;
            jacobian[1][3 + extension] = roll_derivative;
            jacobian[2][3 + extension] = pitch_derivative;
        }
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            if (active_contact[wheel])
            {
                jacobian[3 + wheel][0] = 1.0;
                jacobian[3 + wheel][1] = configuration.wheel_y_meters[wheel];
                jacobian[3 + wheel][2] = -configuration.wheel_x_meters[wheel];
                jacobian[3 + wheel][3 + wheel] = -1.0;
            }
        }

        double delta[7] = {};
        if (!SolveLinearSystem(&jacobian[0][0], 7, residual, delta, 7))
        {
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
            TireVerticalSuspensionEvaluation candidate_suspension;
            EvaluateSuspension(configuration, trial_state.suspension_extension_meters,
                trial_state.suspension_extension_velocity_meters_per_second,
                candidate_suspension);
            double candidate_residual[7] = {};
            double candidate_sprung_weight = configuration.total_mass_kilograms;
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                candidate_sprung_weight -= configuration.unsprung_mass_kilograms[wheel];
            }
            candidate_residual[0] = -candidate_sprung_weight
                * configuration.gravity_meters_per_second_squared;
            candidate_residual[1] = 0.0;
            candidate_residual[2] = 0.0;
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                const double force = candidate_suspension.force_newtons[wheel];
                candidate_residual[0] += force;
                candidate_residual[1] += configuration.wheel_y_meters[wheel] * force;
                candidate_residual[2] -= configuration.wheel_x_meters[wheel] * force;
            }
            for (int body = 0; body < 3; ++body)
            {
                candidate_residual[body] += configuration.external_body_generalized_force[body];
            }
            for (int wheel = 0; wheel < 4; ++wheel)
            {
                const double gap = TireVerticalTireGapFromPositions(configuration, wheel,
                    trial_state, ground_height_meters[wheel]);
                candidate_residual[3 + wheel] = active_contact[wheel]
                    ? gap : candidate_evaluation.generalized_force[3 + wheel];
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
            return 0;
        }
        for (int index = 0; index < 7; ++index)
        {
            q[index] = best_q[index];
        }
    }

    return 0;
}

int TireVerticalSolverInitializeEquilibrium(const TireVerticalSolverConfiguration& configuration,
    const double ground_height_meters[4], TireVerticalSolverState& output_state,
    TireVerticalSolverDiagnostics& diagnostics)
{
    std::memset(&diagnostics, 0, sizeof(diagnostics));
    if (!FiniteConfiguration(configuration) || !FiniteValues(ground_height_meters, 4))
    {
        diagnostics.failed = 1;
        diagnostics.failure_reason = TIRE_VERTICAL_FAILURE_NONFINITE;
        return 0;
    }
    TireVerticalSolverState state = output_state;
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
    double average_height = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double bellcrank = configuration.corner[wheel].spring.bellcrank_ratio;
        state.suspension_extension_meters[wheel] =
            configuration.corner[wheel].spring.reference_position_meters / bellcrank;
        average_height += state.suspension_extension_meters[wheel]
            + ground_height_meters[wheel];
    }
    state.body_position[0] = 0.25 * average_height;

    int active_contact[4] = { 1, 1, 1, 1 };
    double normal_force_newtons[4] = {};
    double gap_meters[4] = {};
    double residual_norm = 0.0;
    int iterations = 0;

    for (int active_set_iteration = 0; active_set_iteration < 8; ++active_set_iteration)
    {
        if (!SolveVerticalEquilibrium(configuration, ground_height_meters, active_contact,
                state, normal_force_newtons, gap_meters, &residual_norm, &iterations))
        {
            diagnostics.converged = 0;
            diagnostics.residual_norm = residual_norm;
            return 0;
        }

        int changed = 0;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            if (active_contact[wheel] && normal_force_newtons[wheel] < -1.0e-6)
            {
                active_contact[wheel] = 0;
                changed = 1;
            }
            else if (!active_contact[wheel] && gap_meters[wheel] < -1.0e-6)
            {
                active_contact[wheel] = 1;
                changed = 1;
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
        if (state.suspension_extension_meters[wheel] < lower - 1.0e-6
            || state.suspension_extension_meters[wheel] > upper + 1.0e-6)
        {
            diagnostics.converged = 0;
            return 0;
        }
    }

    TireVerticalSolverStepInput static_input = {};
    static_input.elapsed_seconds = 1.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        static_input.ground_height_meters[wheel] = ground_height_meters[wheel];
    }
    TireVerticalForceEvaluation evaluation;
    EvaluateForces(configuration, state, static_input, evaluation);
    TireVerticalSuspensionEvaluation suspension;
    EvaluateSuspension(configuration, state.suspension_extension_meters,
        state.suspension_extension_velocity_meters_per_second, suspension);
    FillDiagnostics(configuration, state, static_input, suspension, evaluation, diagnostics);

    if (!TireVerticalStateIsFinite(state) || !FiniteValues(normal_force_newtons, 4)) return 0;
    output_state = state;
    diagnostics.converged = 1;
    diagnostics.newton_iterations = iterations;
    diagnostics.residual_norm = residual_norm;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        diagnostics.tire_normal_force_newtons[wheel] = normal_force_newtons[wheel];
        diagnostics.tire_gap_meters[wheel] = gap_meters[wheel];
        diagnostics.tire_contact[wheel] = active_contact[wheel] ? 1 : 0;
        diagnostics.tire_contact_at_end[wheel] = active_contact[wheel] ? 1 : 0;
        diagnostics.tire_contact_supported_during_step[wheel] = active_contact[wheel] ? 1 : 0;
        diagnostics.suspension_force_newtons[wheel] = suspension.force_newtons[wheel];
        diagnostics.wheel_center_height_meters[wheel] =
            state.body_position[0]
            - configuration.wheel_x_meters[wheel] * state.body_position[2]
            + configuration.wheel_y_meters[wheel] * state.body_position[1]
            + configuration.corner[wheel].unloaded_radius_meters
            - state.suspension_extension_meters[wheel];
    }
    for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
    {
        if (!configuration.chassis_contact[contact].enabled)
        {
            continue;
        }
        diagnostics.chassis_contact_gap_meters[contact] =
            TireVerticalChassisGapFromPositions(configuration, contact, state,
                static_input.chassis_ground_height_meters[contact]);
    }
    return 1;
}

static void EvaluateQuarterCarForces(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& state, const TireVerticalQuarterCarStepInput& input,
    double generalized_force[2], double jacobian_position[2][2],
    double jacobian_velocity[2][2], double& suspension_force,
    double& gap, double& gap_velocity, double& center_height)
{
    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    const double position = bellcrank * state.suspension_extension_meters;
    const double damper_velocity = -bellcrank
        * state.suspension_extension_velocity_meters_per_second;
    suspension_force = TireVerticalCornerSuspensionForce(configuration.corner,
        state.suspension_extension_meters,
        state.suspension_extension_velocity_meters_per_second);

    generalized_force[0] = -(configuration.sprung_mass_kilograms
        + configuration.unsprung_mass_kilograms)
        * configuration.gravity_meters_per_second_squared;
    generalized_force[1] = suspension_force
        + configuration.unsprung_mass_kilograms
            * configuration.gravity_meters_per_second_squared;

    std::memset(jacobian_position, 0, sizeof(double) * 4);
    std::memset(jacobian_velocity, 0, sizeof(double) * 4);
    const double spring_force = TireVerticalSpringForce(configuration.corner.spring, position);
    if (spring_force > 0.0)
    {
        jacobian_position[1][1] = bellcrank * bellcrank
            * configuration.corner.spring.spring_constant_newtons_per_meter;
    }
    jacobian_velocity[1][1] = -bellcrank * bellcrank
        * TireVerticalDamperSlope(configuration.corner.damper, damper_velocity);

    center_height = state.body_position_meters
        + configuration.corner.unloaded_radius_meters
        - state.suspension_extension_meters;
    gap = state.body_position_meters - state.suspension_extension_meters
        - input.ground_height_meters;
    gap_velocity = state.body_velocity_meters_per_second
        - state.suspension_extension_velocity_meters_per_second
        - input.ground_height_velocity_meters_per_second;
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

static double QuarterCarLimitPosition(const TireVerticalQuarterCarConfiguration& configuration,
    int kind)
{
    return kind == TIRE_VERTICAL_CONSTRAINT_LOWER
        ? configuration.corner.spring.minimum_position_meters
            / configuration.corner.spring.bellcrank_ratio
        : configuration.corner.spring.maximum_position_meters
            / configuration.corner.spring.bellcrank_ratio;
}

static int SolveQuarterCarNewton(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& previous_state,
    const TireVerticalQuarterCarStepInput& input, int active_kind,
    TireVerticalQuarterCarState& state, double& multiplier, double& residual_norm,
    int& iterations)
{
    double mass_matrix[2][2] = {};
    BuildQuarterCarMassMatrix(configuration, mass_matrix);
    const int system_size = active_kind == TIRE_VERTICAL_CONSTRAINT_NONE ? 2 : 3;

    for (int iteration = 0; iteration < TireVerticalMaximumNewtonIterations; ++iteration)
    {
        TireVerticalQuarterCarState evaluation_state = state;
        evaluation_state.body_position_meters = previous_state.body_position_meters
            + input.elapsed_seconds * state.body_velocity_meters_per_second;
        evaluation_state.suspension_extension_meters =
            previous_state.suspension_extension_meters
            + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second;

        double generalized_force[2] = {};
        double jacobian_position[2][2] = {};
        double jacobian_velocity[2][2] = {};
        double suspension_force = 0.0;
        double gap = 0.0;
        double gap_velocity = 0.0;
        double center_height = 0.0;
        EvaluateQuarterCarForces(configuration, evaluation_state, input,
            generalized_force, jacobian_position, jacobian_velocity,
            suspension_force, gap, gap_velocity, center_height);

        double residual[3] = {};
        for (int row = 0; row < 2; ++row)
        {
            const double previous_velocity = row == 0
                ? previous_state.body_velocity_meters_per_second
                : previous_state.suspension_extension_velocity_meters_per_second;
            const double velocity = row == 0
                ? state.body_velocity_meters_per_second
                : state.suspension_extension_velocity_meters_per_second;
            residual[row] = mass_matrix[row][0]
                * (state.body_velocity_meters_per_second
                    - previous_state.body_velocity_meters_per_second)
                + mass_matrix[row][1]
                * (state.suspension_extension_velocity_meters_per_second
                    - previous_state.suspension_extension_velocity_meters_per_second)
                - input.elapsed_seconds * generalized_force[row];
            (void)previous_velocity;
            (void)velocity;
        }
        if (active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
        {
            residual[0] -= input.elapsed_seconds * multiplier;
            residual[1] += input.elapsed_seconds * multiplier;
            residual[2] = gap;
        }
        else if (active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
            || active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
        {
            residual[1] -= input.elapsed_seconds * multiplier;
            residual[2] = previous_state.suspension_extension_meters
                + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second
                - QuarterCarLimitPosition(configuration, active_kind);
        }

        const double norm = VectorNorm(residual, system_size);
        residual_norm = norm;
        if (norm <= TireVerticalResidualTolerance)
        {
            iterations = iteration;
            return 1;
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
        if (active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
        {
            jacobian[0][2] = -input.elapsed_seconds;
            jacobian[1][2] = input.elapsed_seconds;
            jacobian[2][0] = input.elapsed_seconds;
            jacobian[2][1] = -input.elapsed_seconds;
        }
        else if (active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
            || active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
        {
            jacobian[1][2] = -input.elapsed_seconds;
            jacobian[2][1] = input.elapsed_seconds;
        }

        double delta[3] = {};
        if (!SolveLinearSystem(&jacobian[0][0], 3, residual, delta, system_size))
        {
            iterations = iteration;
            return 0;
        }
        for (int index = 0; index < system_size; ++index)
        {
            delta[index] = -delta[index];
        }

        double step_scale = 1.0;
        double best_norm = norm;
        TireVerticalQuarterCarState best_state = state;
        double best_multiplier = multiplier;
        int accepted = 0;
        for (int line_search = 0; line_search < 10; ++line_search)
        {
            TireVerticalQuarterCarState trial_state = state;
            trial_state.body_velocity_meters_per_second += step_scale * delta[0];
            trial_state.suspension_extension_velocity_meters_per_second += step_scale * delta[1];
            const double trial_multiplier = active_kind == TIRE_VERTICAL_CONSTRAINT_NONE
                ? 0.0 : multiplier + step_scale * delta[2];

            TireVerticalQuarterCarState trial_evaluation = trial_state;
            trial_evaluation.body_position_meters = previous_state.body_position_meters
                + input.elapsed_seconds * trial_state.body_velocity_meters_per_second;
            trial_evaluation.suspension_extension_meters =
                previous_state.suspension_extension_meters
                + input.elapsed_seconds
                    * trial_state.suspension_extension_velocity_meters_per_second;
            double trial_force[2] = {};
            double trial_jacobian_position[2][2] = {};
            double trial_jacobian_velocity[2][2] = {};
            double trial_suspension = 0.0;
            double trial_gap = 0.0;
            double trial_gap_velocity = 0.0;
            double trial_center = 0.0;
            EvaluateQuarterCarForces(configuration, trial_evaluation, input,
                trial_force, trial_jacobian_position, trial_jacobian_velocity,
                trial_suspension, trial_gap, trial_gap_velocity, trial_center);

            double trial_residual[3] = {};
            for (int row = 0; row < 2; ++row)
            {
                trial_residual[row] = mass_matrix[row][0]
                    * (trial_state.body_velocity_meters_per_second
                        - previous_state.body_velocity_meters_per_second)
                    + mass_matrix[row][1]
                    * (trial_state.suspension_extension_velocity_meters_per_second
                        - previous_state.suspension_extension_velocity_meters_per_second)
                    - input.elapsed_seconds * trial_force[row];
            }
            if (active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
            {
                trial_residual[0] -= input.elapsed_seconds * trial_multiplier;
                trial_residual[1] += input.elapsed_seconds * trial_multiplier;
                trial_residual[2] = trial_gap;
            }
            else if (active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
                || active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
            {
                trial_residual[1] -= input.elapsed_seconds * trial_multiplier;
                trial_residual[2] = previous_state.suspension_extension_meters
                    + input.elapsed_seconds
                        * trial_state.suspension_extension_velocity_meters_per_second
                    - QuarterCarLimitPosition(configuration, active_kind);
            }
            const double trial_norm = VectorNorm(trial_residual, system_size);
            if (trial_norm < best_norm)
            {
                best_norm = trial_norm;
                best_state = trial_state;
                best_multiplier = trial_multiplier;
                accepted = 1;
                break;
            }
            step_scale *= 0.5;
        }
        if (!accepted)
        {
            iterations = iteration;
            return 0;
        }
        state = best_state;
        multiplier = best_multiplier;
        residual_norm = best_norm;
    }

    iterations = TireVerticalMaximumNewtonIterations;
    return 0;
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
    state.body_position_meters = state.suspension_extension_meters + ground_height_meters;

    TireVerticalQuarterCarStepInput static_input = {};
    static_input.elapsed_seconds = 1.0;
    static_input.ground_height_meters = ground_height_meters;

    /* Solve the constrained static equilibrium with the tire in contact. */
    double multiplier = (configuration.sprung_mass_kilograms
        + configuration.unsprung_mass_kilograms)
        * configuration.gravity_meters_per_second_squared;
    double residual_norm = 0.0;
    for (int iteration = 0; iteration < 80; ++iteration)
    {
        TireVerticalQuarterCarState trial = state;
        double force[2] = {};
        double jacobian_position[2][2] = {};
        double jacobian_velocity[2][2] = {};
        double suspension_force = 0.0;
        double gap = 0.0;
        double gap_velocity = 0.0;
        double center = 0.0;
        EvaluateQuarterCarForces(configuration, trial, static_input, force,
            jacobian_position, jacobian_velocity, suspension_force, gap,
            gap_velocity, center);
        double residual[3] = {
            -force[0] - multiplier,
            -force[1] + multiplier,
            gap
        };
        const double norm = VectorNorm(residual, 3);
        residual_norm = norm;
        if (norm <= 1.0e-7)
        {
            state = trial;
            diagnostics.converged = 1;
            diagnostics.newton_iterations = iteration;
            diagnostics.residual_norm = residual_norm;
            diagnostics.tire_normal_force_newtons = multiplier;
            diagnostics.tire_contact_impulse_newton_seconds = 0.0;
            diagnostics.tire_contact_support_seconds = 0.0;
            diagnostics.tire_contact_supported_during_step = 1;
            diagnostics.suspension_force_newtons = suspension_force;
            diagnostics.tire_gap_meters = gap;
            diagnostics.wheel_center_height_meters = center;
            diagnostics.tire_contact = 1;
            return 1;
        }

        double jacobian[3][3] = {};
        for (int row = 0; row < 2; ++row)
        {
            for (int column = 0; column < 2; ++column)
            {
                jacobian[row][column] = -jacobian_position[row][column];
            }
        }
        jacobian[0][2] = -1.0;
        jacobian[1][2] = 1.0;
        jacobian[2][0] = 1.0;
        jacobian[2][1] = -1.0;

        double delta[3] = {};
        if (!SolveLinearSystem(&jacobian[0][0], 3, residual, delta, 3))
        {
            return 0;
        }
        for (int index = 0; index < 3; ++index)
        {
            delta[index] = -delta[index];
        }
        double step_scale = 1.0;
        double best_norm = norm;
        TireVerticalQuarterCarState best_state = state;
        double best_multiplier = multiplier;
        int accepted = 0;
        for (int line_search = 0; line_search < 12; ++line_search)
        {
            TireVerticalQuarterCarState candidate = state;
            candidate.body_position_meters += step_scale * delta[0];
            candidate.suspension_extension_meters += step_scale * delta[1];
            const double candidate_multiplier = multiplier + step_scale * delta[2];
            double candidate_force[2] = {};
            double candidate_jacobian_position[2][2] = {};
            double candidate_jacobian_velocity[2][2] = {};
            double candidate_suspension = 0.0;
            double candidate_gap = 0.0;
            double candidate_gap_velocity = 0.0;
            double candidate_center = 0.0;
            EvaluateQuarterCarForces(configuration, candidate, static_input,
                candidate_force, candidate_jacobian_position, candidate_jacobian_velocity,
                candidate_suspension, candidate_gap, candidate_gap_velocity, candidate_center);
            double candidate_residual[3] = {
                -candidate_force[0] - candidate_multiplier,
                -candidate_force[1] + candidate_multiplier,
                candidate_gap
            };
            const double candidate_norm = VectorNorm(candidate_residual, 3);
            if (candidate_norm < best_norm)
            {
                best_norm = candidate_norm;
                best_state = candidate;
                best_multiplier = candidate_multiplier;
                accepted = 1;
                break;
            }
            step_scale *= 0.5;
        }
        if (!accepted)
        {
            return 0;
        }
        state = best_state;
        multiplier = best_multiplier;
    }
    return 0;
}

int TireVerticalQuarterCarStep(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& previous_state, const TireVerticalQuarterCarStepInput& input,
    TireVerticalQuarterCarState& next_state, TireVerticalQuarterCarDiagnostics& diagnostics)
{
    std::memset(&diagnostics, 0, sizeof(diagnostics));
    next_state = previous_state;
    const double finite_values[] = { previous_state.body_position_meters,
        previous_state.body_velocity_meters_per_second, previous_state.suspension_extension_meters,
        previous_state.suspension_extension_velocity_meters_per_second, input.elapsed_seconds,
        input.ground_height_meters, input.ground_height_velocity_meters_per_second,
        configuration.sprung_mass_kilograms, configuration.unsprung_mass_kilograms,
        configuration.gravity_meters_per_second_squared };
    if (!FiniteValues(finite_values, 10) || !FiniteSpring(configuration.corner.spring)
        || !FiniteDamper(configuration.corner.damper) || input.elapsed_seconds < 0.0) return 0;
    if (!(input.elapsed_seconds > 0.0))
    {
        next_state = previous_state;
        diagnostics.converged = 1;
        diagnostics.no_op = 1;
        return 1;
    }

    TireVerticalQuarterCarState state = previous_state;
    int active_kind = TIRE_VERTICAL_CONSTRAINT_NONE;
    double multiplier = 0.0;
    double residual_norm = 0.0;
    int total_iterations = 0;

    const double gap_previous = previous_state.body_position_meters
        - previous_state.suspension_extension_meters - input.ground_height_meters;
    if (gap_previous < -TireVerticalInitialGapTolerance) return 0;
    const double normal_velocity_previous = previous_state.body_velocity_meters_per_second
        - previous_state.suspension_extension_velocity_meters_per_second
        - input.ground_height_velocity_meters_per_second;
    if (gap_previous <= 1.0e-6 && normal_velocity_previous <= 0.0)
    {
        active_kind = TIRE_VERTICAL_CONSTRAINT_TIRE;
        multiplier = (configuration.sprung_mass_kilograms
            + configuration.unsprung_mass_kilograms)
            * configuration.gravity_meters_per_second_squared;
    }
    else
    {
        const double lower = QuarterCarLimitPosition(configuration,
            TIRE_VERTICAL_CONSTRAINT_LOWER);
        const double upper = QuarterCarLimitPosition(configuration,
            TIRE_VERTICAL_CONSTRAINT_UPPER);
        if (previous_state.suspension_extension_meters <= lower + 1.0e-9)
        {
            active_kind = TIRE_VERTICAL_CONSTRAINT_LOWER;
        }
        else if (previous_state.suspension_extension_meters >= upper - 1.0e-9)
        {
            active_kind = TIRE_VERTICAL_CONSTRAINT_UPPER;
        }
    }

    for (int active_set_iteration = 0; active_set_iteration < 8; ++active_set_iteration)
    {
        int iterations = 0;
        if (!SolveQuarterCarNewton(configuration, previous_state, input, active_kind,
                state, multiplier, residual_norm, iterations))
        {
            next_state = previous_state;
            diagnostics.newton_iterations = total_iterations + iterations;
            diagnostics.residual_norm = residual_norm;
            return 0;
        }
        total_iterations += iterations;

        TireVerticalQuarterCarState end_state = state;
        end_state.body_position_meters = previous_state.body_position_meters
            + input.elapsed_seconds * state.body_velocity_meters_per_second;
        end_state.suspension_extension_meters = previous_state.suspension_extension_meters
            + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second;
        const double ground_end = input.ground_height_meters
            + input.ground_height_velocity_meters_per_second * input.elapsed_seconds;
        const double gap_end = end_state.body_position_meters
            - end_state.suspension_extension_meters - ground_end;

        int changed = 0;
        if (active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
        {
            if (multiplier < -TireVerticalMultiplierTolerance || gap_end > 1.0e-5)
            {
                active_kind = TIRE_VERTICAL_CONSTRAINT_NONE;
                multiplier = 0.0;
                changed = 1;
            }
        }
        else if (active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
            || active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
        {
            const double limit = QuarterCarLimitPosition(configuration, active_kind);
            const int valid_sign = active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
                ? multiplier >= -TireVerticalMultiplierTolerance
                : multiplier <= TireVerticalMultiplierTolerance;
            if (!valid_sign || std::fabs(end_state.suspension_extension_meters - limit) > 1.0e-8)
            {
                active_kind = TIRE_VERTICAL_CONSTRAINT_NONE;
                multiplier = 0.0;
                changed = 1;
            }
        }
        else
        {
            if (gap_end < -1.0e-9)
            {
                active_kind = TIRE_VERTICAL_CONSTRAINT_TIRE;
                multiplier = (configuration.sprung_mass_kilograms
                    + configuration.unsprung_mass_kilograms)
                    * configuration.gravity_meters_per_second_squared;
                changed = 1;
            }
            else
            {
                const double lower = QuarterCarLimitPosition(configuration,
                    TIRE_VERTICAL_CONSTRAINT_LOWER);
                const double upper = QuarterCarLimitPosition(configuration,
                    TIRE_VERTICAL_CONSTRAINT_UPPER);
                if (end_state.suspension_extension_meters < lower - 1.0e-9)
                {
                    active_kind = TIRE_VERTICAL_CONSTRAINT_LOWER;
                    multiplier = 0.0;
                    changed = 1;
                }
                else if (end_state.suspension_extension_meters > upper + 1.0e-9)
                {
                    active_kind = TIRE_VERTICAL_CONSTRAINT_UPPER;
                    multiplier = 0.0;
                    changed = 1;
                }
            }
        }
        if (!changed)
        {
            break;
        }
    }

    TireVerticalQuarterCarState end_state = state;
    end_state.body_position_meters = previous_state.body_position_meters
        + input.elapsed_seconds * state.body_velocity_meters_per_second;
    end_state.suspension_extension_meters = previous_state.suspension_extension_meters
        + input.elapsed_seconds * state.suspension_extension_velocity_meters_per_second;
    const double ground_end = input.ground_height_meters
        + input.ground_height_velocity_meters_per_second * input.elapsed_seconds;
    const double gap_end = end_state.body_position_meters
        - end_state.suspension_extension_meters - ground_end;
    if (gap_end < -1.0e-5)
    {
        next_state = previous_state;
        diagnostics.residual_norm = residual_norm;
        return 0;
    }
    const double lower = QuarterCarLimitPosition(configuration, TIRE_VERTICAL_CONSTRAINT_LOWER);
    const double upper = QuarterCarLimitPosition(configuration, TIRE_VERTICAL_CONSTRAINT_UPPER);
    if (end_state.suspension_extension_meters < lower - 1.0e-7
        || end_state.suspension_extension_meters > upper + 1.0e-7)
    {
        next_state = previous_state;
        diagnostics.residual_norm = residual_norm;
        return 0;
    }

    next_state = end_state;
    double force[2] = {};
    double jacobian_position[2][2] = {};
    double jacobian_velocity[2][2] = {};
    double suspension_force = 0.0;
    double gap = 0.0;
    double gap_velocity = 0.0;
    double center = 0.0;
    EvaluateQuarterCarForces(configuration, next_state, input, force,
        jacobian_position, jacobian_velocity, suspension_force, gap,
        gap_velocity, center);

    diagnostics.converged = 1;
    diagnostics.newton_iterations = total_iterations;
    diagnostics.residual_norm = residual_norm;
    diagnostics.tire_normal_force_newtons =
        active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE ? multiplier : 0.0;
    diagnostics.tire_contact_impulse_newton_seconds =
        active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE
            ? input.elapsed_seconds * multiplier : 0.0;
    diagnostics.tire_contact_support_seconds =
        active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE ? input.elapsed_seconds : 0.0;
    diagnostics.tire_contact_supported_during_step =
        active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE ? 1 : 0;
    diagnostics.suspension_force_newtons = suspension_force;
    diagnostics.tire_gap_meters = gap;
    diagnostics.wheel_center_height_meters = center;
    diagnostics.tire_contact = gap <= 1.0e-5 ? 1 : 0;
    diagnostics.lower_limit_active = active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER;
    diagnostics.upper_limit_active = active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER;
    diagnostics.limit_impulse_newton_seconds =
        (active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
            || active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
            ? input.elapsed_seconds * multiplier : 0.0;

    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    const double average_extension_velocity = 0.5
        * (previous_state.suspension_extension_velocity_meters_per_second
            + next_state.suspension_extension_velocity_meters_per_second);
    const double damper_velocity = -bellcrank * average_extension_velocity;
    const double damper_force = TireVerticalDamperForce(configuration.corner.damper,
        damper_velocity);
    diagnostics.damper_energy_loss_joules = std::max(0.0,
        -bellcrank * damper_force * average_extension_velocity * input.elapsed_seconds);
    diagnostics.tire_damping_energy_loss_joules = 0.0;

    double constraint_loss = 0.0;
    double constraint_work = 0.0;
    if (active_kind == TIRE_VERTICAL_CONSTRAINT_TIRE)
    {
        const double previous_normal_velocity = previous_state.body_velocity_meters_per_second
            - previous_state.suspension_extension_velocity_meters_per_second
            - input.ground_height_velocity_meters_per_second;
        const double next_normal_velocity = next_state.body_velocity_meters_per_second
            - next_state.suspension_extension_velocity_meters_per_second
            - input.ground_height_velocity_meters_per_second;
        const double average_velocity = 0.5 * (previous_normal_velocity + next_normal_velocity);
        const double work = input.elapsed_seconds * multiplier * average_velocity;
        constraint_loss += std::max(0.0, -work);
        constraint_work += work;
    }
    else if (active_kind == TIRE_VERTICAL_CONSTRAINT_LOWER
        || active_kind == TIRE_VERTICAL_CONSTRAINT_UPPER)
    {
        const double average_velocity = 0.5
            * (previous_state.suspension_extension_velocity_meters_per_second
                + next_state.suspension_extension_velocity_meters_per_second);
        const double work = input.elapsed_seconds * multiplier * average_velocity;
        constraint_loss += std::max(0.0, -work);
        constraint_work += work;
    }
    diagnostics.constraint_energy_loss_joules = constraint_loss;
    diagnostics.constraint_work_joules = constraint_work;
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

    const double bellcrank = configuration.corner.spring.bellcrank_ratio;
    potential += TireVerticalSpringPotentialJoules(configuration.corner.spring,
        bellcrank * state.suspension_extension_meters);
    (void)ground_height_meters;
    return kinetic + potential;
}
