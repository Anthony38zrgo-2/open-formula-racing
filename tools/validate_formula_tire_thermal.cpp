#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "tire_thermal_model.h"
#include "tire_wear_model.h"

static int failures = 0;

static void Check(bool condition, const char* name)
{
    if (!condition)
    {
        std::printf("FAIL %s\n", name);
        failures += 1;
    }
    else
    {
        std::printf("PASS %s\n", name);
    }
}

static TireModelConfiguration BaseConfiguration()
{
    TireModelConfiguration configuration;
    TireModelConfigurationSetDefaults(configuration);
    configuration.thermal[0].initial_temperature_celsius = 25.0;
    configuration.thermal[0].ambient_fallback_celsius = 25.0;
    configuration.thermal[0].track_fallback_celsius = 35.0;
    configuration.thermal[0].slip_heat_efficiency = 1.0;
    configuration.wear[0].tread_wear_per_megajoule_millimeters = 1.0;
    configuration.pressure.cold_absolute_pascals[0] = 130000.0 + 101325.0;
    configuration.pressure.cold_absolute_pascals[1] = 130000.0 + 101325.0;
    configuration.pressure.cold_absolute_pascals[2] = 130000.0 + 101325.0;
    configuration.pressure.cold_absolute_pascals[3] = 130000.0 + 101325.0;
    configuration.pressure.reference_hot_gauge_pascals[0] = 160000.0;
    configuration.pressure.reference_hot_gauge_pascals[1] = 160000.0;
    configuration.pressure.reference_hot_gauge_pascals[2] = 150000.0;
    configuration.pressure.reference_hot_gauge_pascals[3] = 150000.0;
    configuration.pressure.ambient_absolute_pascals = 101325.0;
    configuration.pressure.reference_temperature_kelvin = 298.15;
    configuration.pressure.valid = 1;
    return configuration;
}

static TireThermalInput BaseThermalInput()
{
    TireThermalInput input;
    input.normal_force_newtons = 3000.0;
    input.longitudinal_force_newtons = 800.0;
    input.lateral_force_newtons = 1200.0;
    input.slip_lateral_force_newtons = 1000.0;
    input.slip_power_watts = 5000.0;
    input.slip_velocity_longitudinal_meters_per_second = 2.0;
    input.slip_velocity_lateral_meters_per_second = 1.5;
    input.empirical_deformation_meters = 0.008;
    input.empirical_deformation_limit_meters = 0.035;
    input.vehicle_speed_meters_per_second = 40.0;
    input.ambient_temperature_kelvin = 298.15;
    input.track_temperature_kelvin = 308.15;
    input.camber_radians = -0.02;
    input.contact_fraction = 1.0;
    input.brake_carcass_heat_watts = 0.0;
    input.brake_gas_heat_watts = 0.0;
    input.zone_contact_weights[0] = 1.0;
    input.zone_contact_weights[1] = 2.0;
    input.zone_contact_weights[2] = 1.0;
    return input;
}

int main()
{
    TireModelConfiguration configuration = BaseConfiguration();
    TireThermalConfiguration thermal = configuration.thermal[0];
    TirePressureConfiguration pressure = configuration.pressure;

    {
        TireThermalState state;
        TireThermalModelReset(state, thermal, pressure, 0);
        Check(fabs(state.pressure_absolute_pascals - pressure.cold_absolute_pascals[0]) < 1.0,
            "cold pressure at reference temperature");
        Check(fabs(state.tread_inner_kelvin - 298.15) < 1.0e-9, "initial temperature");
    }

    {
        TireThermalState state;
        TireThermalModelReset(state, thermal, pressure, 0);
        state.tread_inner_kelvin = 350.0;
        state.tread_center_kelvin = 350.0;
        state.tread_outer_kelvin = 350.0;
        state.carcass_kelvin = 350.0;
        state.gas_kelvin = 350.0;
        TireThermalInput input = BaseThermalInput();
        input.slip_power_watts = 0.0;
        input.normal_force_newtons = 0.0;
        input.vehicle_speed_meters_per_second = 0.0;
        input.ambient_temperature_kelvin = 350.0;
        input.track_temperature_kelvin = 350.0;
        input.contact_fraction = 0.0;
        input.zone_contact_weights[0] = 0.0;
        input.zone_contact_weights[1] = 0.0;
        input.zone_contact_weights[2] = 0.0;
        TireThermalModelUpdate(state, thermal, pressure, 0, input, 10.0);
        Check(fabs(state.tread_inner_kelvin - 350.0) < 1.0e-9
            && fabs(state.carcass_kelvin - 350.0) < 1.0e-9
            && fabs(state.gas_kelvin - 350.0) < 1.0e-9, "isolated thermal equilibrium");
    }

    {
        TireThermalState state;
        TireThermalModelReset(state, thermal, pressure, 0);
        TireThermalInput input = BaseThermalInput();
        TireThermalModelUpdate(state, thermal, pressure, 0, input, 10.0);
        double residual = fabs(state.last_balance_residual_joules);
        double deposited = fmax(state.accumulated_deposited_joules, 1.0);
        Check(residual / deposited < 1.0e-6, "thermal energy balance residual");
        Check(state.tread_center_kelvin > 298.15, "slip power heats tread");
        Check(state.update_count == 1, "single thermal update per call");
    }

    {
        double absolute130 = TirePressureFromGas(130000.0 + 101325.0, 298.15, 298.15, 0.0, 0.04, 101325.0);
        double absolute100 = TirePressureFromGas(130000.0 + 101325.0, 298.15, 373.15, 0.0, 0.04, 101325.0);
        Check(fabs(absolute130 - 231325.0) < 1.0, "130 kPa cold absolute conversion");
        Check(fabs(TirePressureConvertToAbsolutePascals(130.0, "kPa", true, 101325.0) - 231325.0) < 1.0,
            "gauge kPa boundary conversion");
        Check(fabs(TirePressureConvertToAbsolutePascals(231325.0, "Pa", false, 101325.0) - 231325.0) < 1.0,
            "absolute Pa boundary conversion");
        Check(absolute100 > absolute130, "ideal gas pressure rises with temperature");
        double ratio = absolute100 / absolute130;
        Check(fabs(ratio - 373.15 / 298.15) < 1.0e-9, "gas pressure temperature ratio");
    }

    {
        TireThermalState state;
        TireThermalModelReset(state, thermal, pressure, 0);
        state.gas_kelvin = 373.15;
        TireThermalInput input = BaseThermalInput();
        input.slip_power_watts = 0.0;
        input.normal_force_newtons = 0.0;
        input.contact_fraction = 0.0;
        input.zone_contact_weights[0] = 0.0;
        input.zone_contact_weights[1] = 0.0;
        input.zone_contact_weights[2] = 0.0;
        TireThermalModelUpdate(state, thermal, pressure, 0, input, 0.01);
        Check(state.pressure_absolute_pascals > pressure.cold_absolute_pascals[0], "hot gas raises pressure");
        Check(state.pressure_absolute_pascals > 0.0, "pressure remains positive");
    }

    {
        TireThermalConfiguration extreme = thermal;
        extreme.maximum_pressure_gauge_pascals = 260000.0;
        extreme.minimum_pressure_gauge_pascals = 55000.0;
        TirePressureConfiguration extremePressure = pressure;
        extremePressure.cold_absolute_pascals[0] = 1103161.16691;
        TireThermalState state;
        TireThermalModelReset(state, extreme, extremePressure, 0);
        TireThermalInput input = BaseThermalInput();
        input.slip_power_watts = 0.0;
        input.normal_force_newtons = 0.0;
        input.contact_fraction = 0.0;
        input.zone_contact_weights[0] = 0.0;
        input.zone_contact_weights[1] = 0.0;
        input.zone_contact_weights[2] = 0.0;
        TireThermalModelUpdate(state, extreme, extremePressure, 0, input, 0.01);
        Check(std::isfinite(state.pressure_absolute_pascals), "160 psi input remains finite");
        Check(state.pressure_absolute_pascals > 0.0, "160 psi input remains positive");
        Check(state.pressure_clamp_count > 0, "160 psi input records clamp diagnostic");
    }

    {
        TireWearConfiguration wear = configuration.wear[0];
        TireWearState state;
        TireWearModelReset(state, wear);
        TireWearInput input;
        input.friction_power_watts = 0.0;
        input.zone_contact_weights[0] = 0.25;
        input.zone_contact_weights[1] = 0.5;
        input.zone_contact_weights[2] = 0.25;
        input.tread_inner_kelvin = 368.15;
        input.tread_center_kelvin = 368.15;
        input.tread_outer_kelvin = 368.15;
        input.surface_friction_coefficient = 2.7;
        input.elapsed_seconds = 10.0;
        TireWearModelUpdate(state, wear, input);
        Check(state.inner_wear_fraction == 0.0 && state.center_wear_fraction == 0.0
            && state.outer_wear_fraction == 0.0, "zero slip work produces no wear");

        input.friction_power_watts = 10000.0;
        input.elapsed_seconds = 0.05;
        for (int step = 0; step < 2000; ++step)
        {
            TireWearModelUpdate(state, wear, input);
        }
        Check(state.inner_wear_fraction > 0.0 && state.center_wear_fraction > 0.0,
            "slip work produces wear");
        Check(state.wear_grip_scale < 1.0, "wear reduces grip");
        double previous = state.inner_wear_fraction;
        TireWearModelUpdate(state, wear, input);
        Check(state.inner_wear_fraction >= previous, "wear is non recoverable");

        state.tread_inner_millimeters = wear.minimum_tread_depth_millimeters;
        state.tread_center_millimeters = wear.minimum_tread_depth_millimeters;
        state.tread_outer_millimeters = wear.minimum_tread_depth_millimeters;
        TireWearModelUpdate(state, wear, input);
        Check(state.punctured == 1, "full wear produces persistent puncture");
        Check(fabs(state.wear_grip_scale - 0.25) < 1.0e-9, "puncture grip floor");
        TireWearModelUpdate(state, wear, input);
        Check(state.punctured == 1, "puncture persists");
        TireWearModelReset(state, wear);
        Check(state.punctured == 0 && state.wear_grip_scale == 1.0, "wear reset restores new tire");
    }

    if (failures != 0)
    {
        std::printf("%d thermal validation failures\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("thermal validation passed\n");
    return EXIT_SUCCESS;
}
