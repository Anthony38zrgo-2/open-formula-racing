#pragma once

#include <algorithm>
#include <cmath>

inline double integrateFormulaSteeringResponse(double previous_command, double target_command, double elapsed_seconds,
                                              double transition_rate, double response_seconds)
{
    const double command_difference = target_command - previous_command;
    double remaining_distance = std::abs(command_difference);
    if (response_seconds <= 0.0)
        remaining_distance = std::max(0.0, remaining_distance - transition_rate * elapsed_seconds);
    else
    {
        const double exponential_distance = transition_rate * response_seconds;
        const double linear_seconds = std::max(0.0, (remaining_distance - exponential_distance) / transition_rate);
        const double applied_linear_seconds = std::min(elapsed_seconds, linear_seconds);
        remaining_distance -= transition_rate * applied_linear_seconds;
        remaining_distance *= std::exp(-(elapsed_seconds - applied_linear_seconds) / response_seconds);
    }
    return target_command - std::copysign(remaining_distance, command_difference);
}

inline double advanceFormulaSteering(double previous_command, double target_command, double elapsed_seconds,
                                     double application_rate, double reversal_rate, double centering_rate,
                                     double response_seconds = 0.0)
{
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds <= 0.0)
        return previous_command;
    if (previous_command * target_command < 0.0)
    {
        double centering_seconds = std::abs(previous_command) / reversal_rate;
        if (response_seconds > 0.0)
        {
            const double initial_distance = std::abs(target_command - previous_command);
            const double final_distance = std::abs(target_command);
            const double exponential_distance = reversal_rate * response_seconds;
            centering_seconds = std::max(0.0, (initial_distance - std::max(final_distance, exponential_distance)) / reversal_rate);
            if (final_distance < exponential_distance)
                centering_seconds += response_seconds * std::log(std::min(initial_distance, exponential_distance) / final_distance);
        }
        if (elapsed_seconds < centering_seconds)
            return integrateFormulaSteeringResponse(previous_command, target_command, elapsed_seconds, reversal_rate, response_seconds);
        elapsed_seconds -= centering_seconds;
        previous_command = 0.0;
    }
    const double transition_rate = target_command == 0.0 ? centering_rate : application_rate;
    return integrateFormulaSteeringResponse(previous_command, target_command, elapsed_seconds, transition_rate, response_seconds);
}
