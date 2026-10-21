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
    modifiers.empirical_compliance_scale = 1.0;
    modifiers.empirical_deformation_limit_scale = 1.0;
    modifiers.contact_patch_scale = 1.0;
    TireContactState state;
    TireContactModelReset(state, configuration.contact[0], modifiers);
    int failures = 0;
    for (double load : {0.0, 1500.0, 6000.0})
    {
        for (double contact_fraction : {0.0, 0.5, 1.0})
        {
            TireContactInput input = {};
            input.normal_force_newtons = load;
            input.base_contact_patch_meters = 0.12;
            input.load_sensitivity_exponent = 0.92;
            input.contact_fraction = contact_fraction;
            input.grounded = contact_fraction > 0.0 ? 1 : 0;
            TireContactOutput output;
            TireContactModelEvaluate(state, configuration.contact[0], modifiers, input, output);

            const double expected = fmax(0.0, fmin(
                load * configuration.contact[0].empirical_load_compliance_meters_per_newton,
                output.empirical_deformation_limit_meters));
            const bool passed = fabs(output.empirical_deformation_meters - expected) < 1.0e-12
                && output.effective_radius_meters <= configuration.contact[0].radius_meters
                && output.effective_radius_meters >= 0.86 * configuration.contact[0].radius_meters
                && std::isfinite(output.contact_patch_meters)
                && fabs(output.contact_fraction
                    - (input.grounded ? contact_fraction : 0.0)) < 1.0e-12;
            std::printf("%s algebraic contact load=%.0f fraction=%.2f deformation=%.9f\n",
                passed ? "PASS" : "FAIL", load, contact_fraction,
                output.empirical_deformation_meters);
            failures += !passed;

            TireContactOutput repeated;
            TireContactModelEvaluate(state, configuration.contact[0], modifiers, input, repeated);
            const bool stable = repeated.empirical_deformation_meters
                    == output.empirical_deformation_meters
                && repeated.effective_radius_meters == output.effective_radius_meters
                && repeated.contact_patch_meters == output.contact_patch_meters;
            std::printf("%s repeated evaluation never integrates a second carcass state\n",
                stable ? "PASS" : "FAIL");
            failures += !stable;
        }
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
