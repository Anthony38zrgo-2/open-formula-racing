#include "sim.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

float SimAirDensity = 1.219f;
tCar* SimCarTable = nullptr;

int main(int argument_count, char** arguments)
{
    if (argument_count != 2)
        return EXIT_FAILURE;
    GfInit();
    tCar car = {};
    tCarElt vehicle = {};
    SimulationOptions simulation_options;
    simulation_options.aero_factor = 1.0;
    simulation_options.aero_damage = false;
    car.carElt = &vehicle;
    car.ctrl = &vehicle.ctrl;
    car.options = &simulation_options;
    car.params = GfParmReadFile(arguments[1], GFPARM_RMODE_STD);
    if (!car.params)
        return EXIT_FAILURE;
    SimAeroConfig(&car);
    car.wheelbase = 2.89;
    car.wheeltrack = 1.6;
    car.aero.floor_optimal_rake = 0.0;
    car.aero.hard_maximum_downforce = 0.0;
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
        car.wheel[wheel_index].rideHeight = 0.05 + car.aero.floor_optimal_height -
            (wheel_index < 2 ? car.aero.floor_front_clearance : car.aero.floor_rear_clearance);
    for (int wing_index = 0; wing_index < 2; ++wing_index)
    {
        SimWingConfig(&car, wing_index);
        SimWingReConfig(&car, wing_index);
    }
    tSituation situation = {};
    int failures = 0;
    for (double density : {1.0, 1.219, 1.29})
    {
        SimAirDensity = density;
        for (double speed : {0.0, 30.0, 60.0, 80.0, 100.0})
        {
            car.DynGC.vel.x = speed;
            SimAeroUpdate(&car, &situation);
            SimWingUpdate(&car, 0, &situation);
            SimWingUpdate(&car, 1, &situation);
            const double measured_drag = -(car.aero.drag + car.wing[0].forces.x + car.wing[1].forces.x + car.aero.floor_drag);
            const double expected_drag = 0.5 * SimAirDensity * (0.83 * 1.15 - 0.4309025 +
                0.5347 * 0.3801849367115 + 0.7122 * 0.2578540097634 + 1.1270866 * 0.13) * speed * speed;
            const double expected_front_downforce = 0.5 * SimAirDensity * speed * speed * 0.5347 * 1.33546919057;
            const double expected_rear_downforce = 0.5 * SimAirDensity * speed * speed * 0.7122 * 1.37118017945;
            const double total_downforce = -(car.wing[0].forces.z + car.wing[1].forces.z + car.aero.floor_downforce);
            const bool distribution_passed = speed == 0.0 ||
                (std::abs(-car.aero.floor_downforce / total_downforce - 0.4) < 0.00001 &&
                 std::abs(total_downforce - 0.5 * SimAirDensity * speed * speed * 2.8177165) < 0.02 &&
                 std::abs((car.wing[0].forces.z * 1.749 + car.wing[1].forces.z * -2.192 +
                    car.aero.floor_downforce * -0.298008) / -total_downforce -
                    (0.5347 * 0.915 * 1.749 + 0.7122 * 1.03 * -2.192 + 1.5949 * -0.298008) / 2.8177165) < 0.00001);
            const bool passed = distribution_passed && std::abs(measured_drag - expected_drag) < 0.01 &&
                std::abs(car.wing[0].forces.z + expected_front_downforce) < 0.01 &&
                std::abs(car.wing[1].forces.z + expected_rear_downforce) < 0.01 &&
                std::abs(car.aero.floor_downforce + 0.5 * SimAirDensity * speed * speed * car.aero.floor_lift_area) < 0.01;
            std::cout << "density=" << density << " speed=" << speed << " drag=" << measured_drag
                      << " expected=" << expected_drag << (passed ? " PASS" : " FAIL") << '\n';
            failures += !passed;
        }
    }
    car.DynGC.vel.x = 60.0;
    for (double clearance : {0.0, 0.004, 0.014, 0.052, 0.165, 0.3})
    {
        for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
            car.wheel[wheel_index].rideHeight = 0.05 + clearance -
                (wheel_index < 2 ? car.aero.floor_front_clearance : car.aero.floor_rear_clearance);
        SimAeroUpdate(&car, &situation);
        SimWingUpdate(&car, 0, &situation);
        SimWingUpdate(&car, 1, &situation);
        const bool passed = std::isfinite(car.aero.floor_downforce) && car.aero.floor_downforce <= 0.0 &&
            ((clearance > 0.0 && clearance < 0.165) || std::abs(car.aero.floor_downforce) < 0.001);
        std::cout << "floor clearance=" << clearance << " force=" << car.aero.floor_downforce << (passed ? " PASS" : " FAIL") << '\n';
        failures += !passed;
    }
    car.DynGCg.pos.ay = 70.0 * PI / 180.0;
    SimWingUpdate(&car, 0, &situation);
    SimWingUpdate(&car, 1, &situation);
    const bool stalled_wings_passed = car.wing[0].forces.z == 0.0 && car.wing[1].forces.z == 0.0 &&
        car.wing[0].forces.x < 0.0 && car.wing[1].forces.x < 0.0;
    std::cout << "high pitch stalls wings" << (stalled_wings_passed ? " PASS" : " FAIL") << '\n';
    failures += !stalled_wings_passed;
    car.DynGCg.pos.ay = 0.0;
    car.DynGC.vel.x = 200.0;
    car.mass = 605.0;
    car.aero.hard_maximum_downforce = 10800.0;
    car.aero.hard_maximum_load_ratio = 1.85;
    car.aero.soft_maximum_load_ratio = 1.65;
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
        car.wheel[wheel_index].rideHeight = 0.05 + car.aero.floor_optimal_height -
            (wheel_index < 2 ? car.aero.floor_front_clearance : car.aero.floor_rear_clearance);
    SimAeroUpdate(&car, &situation);
    SimWingUpdate(&car, 0, &situation);
    SimWingUpdate(&car, 1, &situation);
    const double limited_downforce = -(car.wing[0].forces.z + car.wing[1].forces.z + car.aero.floor_downforce);
    const bool force_limit_passed = limited_downforce <= 10800.01 && limited_downforce > 9000.0;
    std::cout << "authored force limit=" << limited_downforce << (force_limit_passed ? " PASS" : " FAIL") << '\n';
    failures += !force_limit_passed;
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
