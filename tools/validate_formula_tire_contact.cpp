#include <cmath>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include "tire_contact_model.h"

int main()
{
    TireModelConfiguration configuration;
    TireModelConfigurationSetDefaults(configuration);
    TireMechanicalModifiers modifiers = {};
    modifiers.vertical_stiffness_scale = 1.0;
    modifiers.vertical_damping_scale = 1.0;
    modifiers.maximum_deflection_scale = 1.0;
    modifiers.contact_patch_scale = 1.0;
    TireContactState state;
    TireContactModelReset(state, configuration.contact[0], modifiers);
    int failures = 0;
    for (double load : {0.0, 1500.0, 6000.0})
    {
        for (double displacement : {-0.01, 0.0, 0.008, 0.2})
        {
            TireContactInput input = {};
            input.normal_force_newtons = load;
            input.base_contact_patch_meters = 0.12;
            input.load_sensitivity_exponent = 0.92;
            input.deflection_meters = displacement;
            input.deflection_velocity_meters_per_second = -0.03;
            input.grounded = load > 0.0 ? 1 : 0;
            TireContactOutput output;
            TireContactModelEvaluate(state, configuration.contact[0], modifiers, input, output);
            const double expected = fmax(0.0, fmin(displacement, output.maximum_deflection_meters));
            const bool passed = fabs(output.deflection_meters - expected) < 1.0e-12
                && output.deflection_velocity_meters_per_second == -0.03
                && output.effective_radius_meters <= configuration.contact[0].radius_meters
                && output.effective_radius_meters >= 0.86 * configuration.contact[0].radius_meters
                && std::isfinite(output.contact_patch_meters)
                && (load > 0.0 ? output.contact_fraction > 0.0 : output.contact_fraction == 0.0);
            std::printf("%s geometric contact load=%.0f displacement=%.3f\n",
                passed ? "PASS" : "FAIL", load, displacement);
            failures += !passed;
            TireContactOutput repeated;
            TireContactModelEvaluate(state, configuration.contact[0], modifiers, input, repeated);
            const bool stable = repeated.deflection_meters == output.deflection_meters
                && repeated.effective_radius_meters == output.effective_radius_meters
                && repeated.contact_patch_meters == output.contact_patch_meters;
            std::printf("%s repeated evaluation never integrates a second carcass state\n", stable ? "PASS" : "FAIL");
            failures += !stable;
        }
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
