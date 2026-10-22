#include "sim.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

tdble SimDeltaTime = 0.002;

tdble SimEngineUpdateRpm(tCar*, tdble axle_angular_speed)
{
    return axle_angular_speed;
}

static double differentialWheelSpeedDifference(void* parameters, bool original_configuration, double input_torque, double coast_bias_override = -1.0, double initial_speed_difference = 10.0, double preload_override = -1.0)
{
    tCar car = {};
    tCarElt vehicle = {};
    tDynAxis input_axes[2] = {};
    tDynAxis output_axes[2] = {};
    car.params = parameters;
    car.carElt = &vehicle;
    tDifferential& differential = car.transmission.differential[TRANS_REAR_DIFF];
    for (int axle_index = 0; axle_index < 2; ++axle_index)
    {
        input_axes[axle_index].I = output_axes[axle_index].I = 1.0;
        input_axes[axle_index].spinVel = axle_index == 0 ? 90.0 + initial_speed_difference : 90.0;
        differential.inAxis[axle_index] = &input_axes[axle_index];
        differential.outAxis[axle_index] = &output_axes[axle_index];
    }
    SimDifferentialConfig(&car, TRANS_REAR_DIFF);
    SimDifferentialReConfig(&car, TRANS_REAR_DIFF);
    if (original_configuration)
        differential.type = DIFF_LIMITED_SLIP;
    if (coast_bias_override >= 0.0)
        differential.dCoastSlipMax = coast_bias_override;
    if (preload_override >= 0.0)
        differential.preload_torque = preload_override;
    differential.in.Tq = input_torque;
    SimDifferentialUpdate(&car, &differential, 0);
    return std::abs(output_axes[0].spinVel - output_axes[1].spinVel);
}

int main(int argument_count, char** arguments)
{
    if (argument_count != 2)
        return EXIT_FAILURE;
    GfInit();
    void* parameters = GfParmReadFile(arguments[1], GFPARM_RMODE_STD);
    if (!parameters)
        return EXIT_FAILURE;
    const double original_coast_difference = differentialWheelSpeedDifference(parameters, true, -200.0);
    const double revised_coast_difference = differentialWheelSpeedDifference(parameters, false, -200.0);
    const double no_coast_difference = differentialWheelSpeedDifference(parameters, false, -200.0, 0.0, 10.0, 0.0);
    const double high_coast_difference = differentialWheelSpeedDifference(parameters, false, -200.0, 0.30, 10.0, 0.0);
    const double original_power_difference = differentialWheelSpeedDifference(parameters, true, 200.0);
    const double revised_power_difference = differentialWheelSpeedDifference(parameters, false, 200.0);
    const double preload_difference = differentialWheelSpeedDifference(parameters, false, 0.0);
    const double reverse_preload_difference = differentialWheelSpeedDifference(parameters, false, 0.0, -1.0, -10.0);
    const double synchronization_difference = differentialWheelSpeedDifference(parameters, false, 0.0, -1.0, 0.001);
    const double no_preload_difference = differentialWheelSpeedDifference(parameters, false, 0.0, -1.0, 10.0, 0.0);
    const double symmetric_difference = differentialWheelSpeedDifference(parameters, false, 0.0, -1.0, 0.0);
    const bool passed = revised_coast_difference < original_coast_difference &&
        revised_coast_difference > 0.0 &&
        revised_coast_difference < no_coast_difference &&
        high_coast_difference < revised_coast_difference &&
        std::abs((original_power_difference - revised_power_difference) - 0.04) < 0.00001 &&
        std::abs(preload_difference - 9.96) < 0.00001 &&
        std::abs(reverse_preload_difference - preload_difference) < 0.00001 &&
        synchronization_difference < 0.000001 && symmetric_difference < 0.000001 &&
        std::abs(no_preload_difference - 10.0) < 0.000001;
    std::cout << "coast wheel speed difference: previous=" << original_coast_difference << " revised=" << revised_coast_difference
              << "; no coast difference=" << no_coast_difference
              << "; coast bias 0.30 difference=" << high_coast_difference
              << "; zero input preload difference=" << preload_difference
              << "; synchronization difference=" << synchronization_difference
              << "; power: previous=" << original_power_difference << " revised=" << revised_power_difference
              << (passed ? " PASS" : " FAIL") << '\n';
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
