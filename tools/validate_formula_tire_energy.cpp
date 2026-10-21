#include <cmath>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "tire_thermal_model.h"

int main()
{
    TireModelConfiguration configuration;
    TireModelConfigurationSetDefaults(configuration);
    configuration.pressure.valid = 1;
    configuration.pressure.reference_temperature_kelvin = 298.15;
    configuration.pressure.ambient_absolute_pascals = 101325.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        configuration.pressure.cold_absolute_pascals[wheel] = 231325.0;
        configuration.pressure.reference_hot_gauge_pascals[wheel] = 160000.0;
    }
    int failures = 0;
    for (int axle = 0; axle < 2; ++axle)
    {
        const TireThermalConfiguration& thermal = configuration.thermal[axle];
        for (double temperature : {293.15, 303.15, 353.15})
        {
            for (double normalLoad : {0.0, 1200.0, 3000.0})
            {
                for (double speed : {0.0, 60.0})
                {
                    TireThermalState state = {};
                    TireThermalModelReset(state, thermal, configuration.pressure, axle * 2);
                    state.tread_inner_kelvin = state.tread_center_kelvin = state.tread_outer_kelvin = temperature;
                    state.carcass_kelvin = state.gas_kelvin = temperature;
                    TireThermalInput input = {};
                    input.normal_force_newtons = normalLoad;
                    input.contact_fraction = normalLoad > 0.0 ? 1.0 : 0.0;
                    input.slip_power_watts = normalLoad > 0.0 && speed > 0.0 ? 2500.0 : 0.0;
                    input.vehicle_speed_meters_per_second = speed;
                    input.ambient_temperature_kelvin = 303.15;
                    input.track_temperature_kelvin = 308.15;
                    input.empirical_deformation_meters = 0.008;
                    input.empirical_deformation_limit_meters = 0.04;
                    input.zone_contact_weights[0] = 1.0;
                    input.zone_contact_weights[1] = 2.0;
                    input.zone_contact_weights[2] = 1.0;
                    const double duration = 0.002;
                    TireThermalModelUpdate(state, thermal, configuration.pressure, axle * 2, input, duration);
                    const double measured = thermal.tread_zone_heat_capacity_joules_per_kelvin
                        * (state.tread_inner_kelvin + state.tread_center_kelvin + state.tread_outer_kelvin - 3.0 * temperature)
                        + thermal.carcass_heat_capacity_joules_per_kelvin * (state.carcass_kelvin - temperature)
                        + thermal.gas_heat_capacity_joules_per_kelvin * (state.gas_kelvin - temperature);
                    const double externalEnergy = state.last_external_power_watts * duration;
                    const bool passed = std::isfinite(measured) && fabs(measured - externalEnergy) < 1.0e-6
                        && fabs(state.last_balance_residual_joules) < 1.0e-6
                        && fabs(state.accumulated_deposited_joules - input.slip_power_watts
                            * thermal.slip_heat_efficiency * duration) < 1.0e-6;
                    std::printf("%s coupled thermal energy axle=%d temperature=%.2f load=%.0f speed=%.0f residual=%.9g\n",
                        passed ? "PASS" : "FAIL", axle, temperature, normalLoad, speed, measured - externalEnergy);
                    failures += !passed;
                    TireThermalState before = state;
                    TireThermalModelUpdate(state, thermal, configuration.pressure, axle * 2, input, 0.0);
                    const bool unchanged = std::memcmp(&before, &state, sizeof(state)) == 0;
                    std::printf("%s zero elapsed thermal state unchanged\n", unchanged ? "PASS" : "FAIL");
                    failures += !unchanged;
                }
            }
        }
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
