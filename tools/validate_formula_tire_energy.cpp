#include "sim.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

tdble SimDeltaTime = 0.002;
tdble SimRain = 0.0;
double Tair = 303.15;
tdble Ttrack = 293.15;
float SimAirPressure = 101300.0f;
tdble simSkidFactor[6] = {};

struct TireThermalScenarioResult
{
    double ending_temperature;
    double first_convection_power;
    double first_hysteresis_power;
};

static TireThermalScenarioResult simulateTireThermalScenario(void* parameters, int wheel_index,
    double cooling_factor, double hysteresis_factor, double starting_celsius,
    double wheel_speed, double normal_load, double lateral_force, double longitudinal_force,
    double slip, double duration_seconds)
{
    tCar car = {};
    tCarElt vehicle = {};
    SimulationOptions simulation_options;
    simulation_options.tyre_temperature = true;
    vehicle.info.skillLevel = PRO;
    car.carElt = &vehicle;
    car.options = &simulation_options;
    car.params = parameters;
    car.features = FEAT_TIRETEMPDEG;
    tWheel& wheel = car.wheel[wheel_index];
    wheel.weight0 = wheel_index < 2 ? 1320.0 : 1647.0;
    SimWheelConfig(&car, wheel_index);
    wheel.opLoad = vehicle.setup.tireOpLoad[wheel_index].desired_value;
    wheel.camber = vehicle.setup.camber[wheel_index].desired_value;
    wheel.tireSpeedCoolFactor = cooling_factor;
    wheel.hysteresisFactor = hysteresis_factor;
    wheel.Ttire = starting_celsius + 273.15;
    wheel.currentPressure = wheel.pressure * wheel.Ttire / wheel.Tinit;
    wheel.spinVel = wheel_speed / wheel.radius;
    wheel.forces.z = normal_load;
    wheel.forces.y = lateral_force;
    wheel.forces.x = longitudinal_force;
    wheel.tireSlip = slip;
    TireThermalScenarioResult result = {};
    const int step_count = static_cast<int>(std::round(duration_seconds / SimDeltaTime));
    for (int step_index = 0; step_index < step_count; ++step_index)
    {
        SimWheelUpdateTire(&car, wheel_index);
        if (step_index == 0)
        {
            result.first_convection_power = wheel.convection_energy / SimDeltaTime;
            result.first_hysteresis_power = wheel.hysteresis_energy / SimDeltaTime;
        }
        if (!std::isfinite(wheel.Ttire) || wheel.Ttire < 250.0 || wheel.Ttire > 473.15)
            return {0.0, 0.0, 0.0};
    }
    result.ending_temperature = wheel.Ttire - 273.15;
    return result;
}

static bool validateLateralGrip(void* parameters, int wheel_index, double normal_load)
{
    tCar car = {};
    tCarElt vehicle = {};
    SimulationOptions simulation_options;
    simulation_options.tyre_temperature = false;
    car.carElt = &vehicle;
    car.options = &simulation_options;
    car.params = parameters;
    car.features = FEAT_SLOWGRIP | FEAT_FIXEDWHEELFORCE;
    vehicle.info.skillLevel = PRO;
    tWheel& wheel = car.wheel[wheel_index];
    wheel.weight0 = wheel_index < 2 ? 1320.0 : 1647.0;
    SimWheelConfig(&car, wheel_index);
    wheel.opLoad = vehicle.setup.tireOpLoad[wheel_index].desired_value;
    wheel.ground_friction = 1.0;
    wheel.susp.spring.bellcrank = 1.0;
    wheel.susp.spring.K = 0.0;
    wheel.susp.spring.F0 = normal_load;
    wheel.susp.inertance = 0.0;
    wheel.staticPos.ax = 0.0;
    wheel.staticPos.az = 0.0;
    double peak_force = 0.0;
    double peak_angle = 0.0;
    double sliding_force = 0.0;
    for (int angle_index = 0; angle_index <= 1000; ++angle_index)
    {
        const double angle = angle_index * 0.001;
        wheel.susp.v = 0.0;
        wheel.bodyVel.x = 30.0;
        wheel.bodyVel.y = 30.0 * std::tan(angle);
        wheel.spinVel = 30.0 / wheel.radius;
        SimWheelUpdateForce(&car, wheel_index);
        const double lateral_force = std::abs(wheel.forces.y);
        if (!std::isfinite(lateral_force))
            return false;
        if (lateral_force > peak_force)
        {
            peak_force = lateral_force;
            peak_angle = angle;
        }
        sliding_force = lateral_force;
    }
    const double peak_friction = peak_force / normal_load;
    const bool passed = std::abs(peak_angle - 0.115) < 0.003 &&
        sliding_force / peak_force >= 0.88 && peak_friction > 2.1 && peak_friction < 4.0;
    std::cout << "native tire grip wheel=" << wheel_index << " load=" << normal_load
              << " peak_friction=" << peak_friction << " peak_angle=" << peak_angle
              << " sliding_fraction=" << sliding_force / peak_force << (passed ? " PASS" : " FAIL") << '\n';
    return passed;
}

int main(int argument_count, char** arguments)
{
    GfInit();
    GfLogSetLevelThreshold(GfLogger::eWarning);
    int failures = 0;
    for (double temperature : {293.15, 303.15, 353.15})
    {
        for (double normal_load : {0.0, 1200.0, 3000.0})
        {
            tCar car = {};
            tCarElt vehicle = {};
            vehicle.info.skillLevel = static_cast<tSkillLevel>(4);
            car.carElt = &vehicle;
            tWheel& wheel = car.wheel[0];
            wheel.corrected_thermal_load = true;
            wheel.forces.z = normal_load;
            wheel.opLoad = 2000.0;
            wheel.pressure = wheel.currentPressure = 138000.0;
            wheel.camber = -0.035;
            wheel.radius = 0.32;
            wheel.spinVel = 60.0 / wheel.radius;
            wheel.Ttire = wheel.Tinit = temperature;
            wheel.Topt = 353.15;
            wheel.hysteresisFactor = 1.0;
            wheel.tireConvectionSurface = 0.9;
            wheel.tireSpeedCoolFactor = 1.0;
            wheel.coolingFactor = 0.000075;
            wheel.baseMass = 5.0;
            wheel.treadMass = 1.0;
            wheel.tireGasMass = 0.1;
            const double starting_temperature = wheel.Ttire;
            SimWheelUpdateTire(&car, 0);
            const double expected_energy = wheel.hysteresis_energy + wheel.lateral_heating_energy +
                wheel.longitudinal_heating_energy - wheel.convection_energy - wheel.additional_cooling_energy;
            const double measured_energy = (wheel.Ttire - starting_temperature) * wheel.thermal_capacity;
            const double rounding_tolerance = 2.0 * std::numeric_limits<tdble>::epsilon() * starting_temperature * wheel.thermal_capacity;
            bool passed = std::isfinite(wheel.Ttire) && std::abs(expected_energy - measured_energy) < rounding_tolerance;
            const double camber_degrees = std::abs(wheel.camber) * 180.0 / PI;
            const double adjusted_load = normal_load == 0.0 ? 0.0 : normal_load +
                (normal_load > wheel.opLoad ? -wheel.pressure / 5894.6 : wheel.pressure / 3894.6) * camber_degrees;
            const double expected_hysteresis = adjusted_load * 60.0 * SimDeltaTime * 0.05;
            passed = passed && std::abs(wheel.hysteresis_energy - expected_hysteresis) < 0.0001;
            if (normal_load == 0.0)
                passed = passed && wheel.hysteresis_energy == 0.0;
            if (temperature < Tair)
                passed = passed && wheel.Ttire >= starting_temperature;
            std::cout << "temperature=" << temperature << " load=" << normal_load << " balance_error="
                      << measured_energy - expected_energy << (passed ? " PASS" : " FAIL") << '\n';
            failures += !passed;
        }
    }
    if (argument_count == 2)
    {
        void* parameters = GfParmReadFile(arguments[1], GFPARM_RMODE_STD);
        if (!parameters)
            return EXIT_FAILURE;
        for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
            for (double normal_load : {1200.0, 3200.0, 6000.0})
                failures += !validateLateralGrip(parameters, wheel_index, normal_load);
        for (int wheel_index : {0, 2})
        {
            const char* section_name = wheel_index == 0 ? "Front Right Wheel" : "Rear Right Wheel";
            const double cooling_factor = GfParmGetNum(parameters, section_name, "tire speed cooling factor", nullptr, -1.0);
            const double hysteresis_factor = GfParmGetNum(parameters, section_name, "hysteresis", nullptr, -1.0);
            failures += std::abs(cooling_factor - 0.25) > 0.00001 || std::abs(hysteresis_factor - 1.25) > 0.00001;
            for (const char* scenario_name : {"hot straight", "cold corner", "cold braking"})
            {
                const bool hot_straight = std::strcmp(scenario_name, "hot straight") == 0;
                const bool cold_corner = std::strcmp(scenario_name, "cold corner") == 0;
                const double starting_celsius = hot_straight ? 80.0 : 30.0;
                const double speed = hot_straight ? 260.0 / 3.6 : cold_corner ? 45.0 : 35.0;
                const double load = hot_straight ? 1800.0 : 2500.0;
                const double lateral_force = cold_corner ? 2500.0 : 0.0;
                const double longitudinal_force = !hot_straight && !cold_corner ? 2500.0 : 0.0;
                const double slip = hot_straight ? 0.0 : cold_corner ? 0.05 : 0.1;
                const double duration = hot_straight ? 30.0 : 60.0;
                const auto previous = simulateTireThermalScenario(parameters, wheel_index, 1.0, 1.0, starting_celsius,
                    speed, load, lateral_force, longitudinal_force, slip, duration);
                const auto cooling_only = simulateTireThermalScenario(parameters, wheel_index, cooling_factor, 1.0, starting_celsius,
                    speed, load, lateral_force, longitudinal_force, slip, duration);
                const auto revised = simulateTireThermalScenario(parameters, wheel_index, cooling_factor, hysteresis_factor, starting_celsius,
                    speed, load, lateral_force, longitudinal_force, slip, duration);
                bool passed = previous.ending_temperature > 0.0 && revised.ending_temperature > cooling_only.ending_temperature &&
                    cooling_only.ending_temperature > previous.ending_temperature;
                if (hot_straight)
                    passed = passed && std::abs(revised.first_convection_power / previous.first_convection_power - 0.55) < 0.0001;
                passed = passed && std::abs(revised.first_hysteresis_power / previous.first_hysteresis_power - 1.25) < 0.0001;
                std::cout << "thermal scenario=" << scenario_name << " wheel=" << wheel_index << " duration=" << duration
                          << " previous_celsius=" << previous.ending_temperature << " cooling_only_celsius=" << cooling_only.ending_temperature
                          << " revised_celsius=" << revised.ending_temperature << (passed ? " PASS" : " FAIL") << '\n';
                failures += !passed;
            }
        }
    }
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
