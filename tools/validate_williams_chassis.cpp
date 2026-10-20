#include "sim.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

static void configureChassisAxle(tCar& car, tCarElt& vehicle, void* parameters, int axle_index)
{
    const char* wheel_sections[] = {"Front Right Wheel", "Front Left Wheel", "Rear Right Wheel", "Rear Left Wheel"};
    const char* suspension_sections[] = {"Front Right Suspension", "Front Left Suspension", "Rear Right Suspension", "Rear Left Suspension"};
    car.params = parameters;
    car.carElt = &vehicle;
    const double mass = GfParmGetNum(parameters, "Car", "mass", nullptr, 0.0);
    const double front_distribution = GfParmGetNum(parameters, "Car", "front-rear weight repartition", nullptr, 0.0);
    SimAxleConfig(&car, axle_index);
    SimAxleReConfig(&car, axle_index, 0.0);
    for (int side_index = 0; side_index < 2; ++side_index)
    {
        const int wheel_index = 2 * axle_index + side_index;
        tWheel& wheel = car.wheel[wheel_index];
        wheel.weight0 = mass * G * (axle_index == 0 ? front_distribution : 1.0 - front_distribution) * 0.5;
        wheel.staticPos.y = GfParmGetNum(parameters, wheel_sections[wheel_index], "ypos", nullptr, 0.0);
        wheel.mass = GfParmGetNum(parameters, wheel_sections[wheel_index], "mass", nullptr, 0.0);
        SimSuspConfig(&car, parameters, suspension_sections[wheel_index], &wheel.susp, wheel_index);
        SimSuspReConfig(&car, &wheel.susp, wheel_index, wheel.weight0, vehicle.setup.rideHeight[wheel_index].value);
        wheel.susp.x = wheel.susp.spring.x0;
    }
}

static double axlePerturbationEnergy(const tCar& car, int axle_index)
{
    double energy = 0.0;
    for (int side_index = 0; side_index < 2; ++side_index)
    {
        const tWheel& wheel = car.wheel[2 * axle_index + side_index];
        const double displacement = wheel.susp.x - wheel.susp.spring.x0;
        energy += 0.5 * wheel.mass * wheel.susp.v * wheel.susp.v - 0.5 * wheel.susp.spring.K * displacement * displacement;
    }
    const double difference = car.wheel[2 * axle_index].susp.x - car.wheel[2 * axle_index + 1].susp.x;
    return energy + 0.5 * car.axle[axle_index].arbSusp.spring.K * difference * difference;
}

static bool checkAxleDynamics(void* parameters, int axle_index, bool bar_only, bool include_bar_reaction)
{
    tCar car = {};
    tCarElt vehicle = {};
    configureChassisAxle(car, vehicle, parameters, axle_index);
    for (int side_index = 0; side_index < 2; ++side_index)
    {
        tWheel& wheel = car.wheel[2 * axle_index + side_index];
        const double direction = side_index == 0 ? 1.0 : -1.0;
        wheel.susp.x += direction * 0.001;
        wheel.susp.v = direction * 0.02;
        if (bar_only)
        {
            wheel.susp.spring.K = 0.0;
            wheel.susp.damper.bump.C1 = wheel.susp.damper.bump.C2 = wheel.susp.damper.bump.b2 = 0.0;
            wheel.susp.damper.rebound.C1 = wheel.susp.damper.rebound.C2 = wheel.susp.damper.rebound.b2 = 0.0;
        }
    }
    const double initial_energy = axlePerturbationEnergy(car, axle_index);
    double maximum_energy = initial_energy;
    const double elapsed_seconds = 0.002;
    const int step_count = bar_only ? 100 : 1000;
    for (int step_index = 0; step_index < step_count; ++step_index)
    {
        for (int side_index = 0; side_index < 2; ++side_index)
        {
            tWheel& wheel = car.wheel[2 * axle_index + side_index];
            wheel.susp.x -= wheel.susp.v * elapsed_seconds;
            wheel.susp.state = 0;
            SimSuspCheckIn(&wheel.susp);
        }
        SimAxleUpdate(&car, axle_index);
        for (int side_index = 0; side_index < 2; ++side_index)
        {
            tWheel& wheel = car.wheel[2 * axle_index + side_index];
            SimSuspUpdate(&wheel.susp);
            const double reaction = wheel.susp.force + (include_bar_reaction ? wheel.axleFz : 0.0);
            wheel.susp.v -= elapsed_seconds * (reaction - wheel.weight0) / wheel.mass;
            if (!std::isfinite(wheel.susp.v) || wheel.susp.state != 0)
                return false;
        }
        maximum_energy = std::max(maximum_energy, axlePerturbationEnergy(car, axle_index));
    }
    const double final_energy = axlePerturbationEnergy(car, axle_index);
    const bool passed = include_bar_reaction ? maximum_energy < initial_energy * 1.5 &&
        (bar_only || final_energy < initial_energy * 0.02) : final_energy > initial_energy * 2.0;
    std::cout << "axle=" << axle_index << " bar_only=" << bar_only << " bar_reaction=" << include_bar_reaction
              << " initial_energy=" << initial_energy << " maximum_energy=" << maximum_energy << " final_energy=" << final_energy
              << (passed ? " PASS" : " FAIL") << '\n';
    return passed;
}

int main(int argument_count, char** arguments)
{
    if (argument_count != 2)
        return EXIT_FAILURE;
    GfInit();
    GfLogSetLevelThreshold(GfLogger::eWarning);
    void* parameters = GfParmReadFile(arguments[1], GFPARM_RMODE_STD);
    if (!parameters)
        return EXIT_FAILURE;
    int failures = 0;
    failures += std::strcmp(GfParmGetStr(parameters, "Features", "fixed wheel force", "no"), "yes") != 0;
    const double ground_center_height = GfParmGetNum(parameters, "Car", "GC height", nullptr, 0.0) +
        GfParmGetNum(parameters, "Front Right Wheel", "ride height", nullptr, 0.0);
    failures += std::abs(ground_center_height - 0.295) > 0.00001;
    for (int axle_index = 0; axle_index < 2; ++axle_index)
    {
        tCar car = {};
        tCarElt vehicle = {};
        configureChassisAxle(car, vehicle, parameters, axle_index);
        const tWheel& wheel = car.wheel[2 * axle_index];
        const double expected_bump = axle_index == 0 ? 0.025 : 0.03;
        const double expected_droop = axle_index == 0 ? 0.085 : 0.09;
        failures += std::abs(wheel.susp.spring.x0 - wheel.susp.spring.packers - expected_bump) > 0.000001;
        failures += std::abs(wheel.susp.spring.xMax - wheel.susp.spring.x0 - expected_droop) > 0.000001;
        const double largest_damping = std::max(wheel.susp.damper.bump.C1, wheel.susp.damper.rebound.C1);
        const double antisymmetric_stiffness = -wheel.susp.spring.K + 2.0 * car.axle[axle_index].arbSusp.spring.K;
        const double integration_stability_ratio = largest_damping * 0.002 / wheel.mass +
            antisymmetric_stiffness * 0.002 * 0.002 / (2.0 * wheel.mass);
        std::cout << "axle=" << axle_index << " wheel_spring=" << -wheel.susp.spring.K << " rebound=" << wheel.susp.damper.rebound.C1
                  << " integration_stability_ratio=" << integration_stability_ratio << '\n';
        failures += integration_stability_ratio >= 2.0;
        failures += !checkAxleDynamics(parameters, axle_index, false, true);
        failures += !checkAxleDynamics(parameters, axle_index, true, true);
        failures += !checkAxleDynamics(parameters, axle_index, true, false);
    }
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
