#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "tire_contact_model.h"

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

static TireContactConfiguration BaseConfiguration()
{
    TireModelConfiguration configuration;
    TireModelConfigurationSetDefaults(configuration);
    return configuration.contact[0];
}

static TireMechanicalModifiers IdentityModifiers()
{
    TireMechanicalModifiers modifiers;
    modifiers.vertical_stiffness_scale = 1.0;
    modifiers.vertical_damping_scale = 1.0;
    modifiers.maximum_deflection_scale = 1.0;
    modifiers.contact_patch_scale = 1.0;
    modifiers.relaxation_length_scale = 1.0;
    modifiers.rolling_resistance_scale = 1.0;
    modifiers.force_stiffness_scale = 1.0;
    modifiers.grip_scale = 1.0;
    modifiers.pneumatic_trail_scale = 1.0;
    modifiers.wear_grip_scale = 1.0;
    return modifiers;
}

static TireContactOutput Update(TireContactState& state, const TireContactConfiguration& configuration,
    const TireMechanicalModifiers& modifiers, double force, double dt, bool grounded)
{
    TireContactInput input;
    input.normal_force_newtons = force;
    input.base_contact_patch_meters = configuration.width_meters * 0.8;
    input.load_sensitivity_exponent = 0.92;
    input.elapsed_seconds = dt;
    input.grounded = grounded ? 1 : 0;
    TireContactOutput output;
    TireContactModelUpdate(state, configuration, modifiers, input, output);
    return output;
}

static double SimulateFrequency(double frequency, TireContactConfiguration configuration)
{
    TireMechanicalModifiers modifiers = IdentityModifiers();
    TireContactState state;
    TireContactModelReset(state, configuration, modifiers);
    double dt = 1.0 / frequency;
    double finalDeflection = 0.0;
    for (int step = 0; step < (int)(5.0 * frequency); ++step)
    {
        double time = step * dt;
        double force = 3000.0 + 3000.0 * sin(2.0 * M_PI * frequency * time);
        TireContactOutput output = Update(state, configuration, modifiers, fmax(force, 0.0), dt, true);
        if (!std::isfinite(output.deflection_meters) || !std::isfinite(output.deflection_velocity_meters_per_second))
        {
            return NAN;
        }
        finalDeflection = output.deflection_meters;
    }
    return finalDeflection;
}

int main()
{
    TireContactConfiguration configuration = BaseConfiguration();
    TireMechanicalModifiers modifiers = IdentityModifiers();

    {
        TireContactState state;
        TireContactModelReset(state, configuration, modifiers);
        TireContactOutput output;
        for (int step = 0; step < 3000; ++step)
        {
            output = Update(state, configuration, modifiers, 3000.0, 0.002, true);
        }
        double expected = 3000.0 / configuration.radial_stiffness_newtons_per_meter;
        Check(fabs(output.deflection_meters - expected) / expected < 0.05, "static deflection equilibrium");
        Check(fabs(output.load_sensitivity_scale - 1.0) < 0.15, "static load sensitivity near unity");
        Check(output.deflection_meters >= 0.0 && output.deflection_meters <= output.maximum_deflection_meters,
            "static deflection bounded");
    }

    {
        TireContactState state;
        TireContactModelReset(state, configuration, modifiers);
        TireContactOutput output = Update(state, configuration, modifiers, 0.0, 0.002, false);
        for (int step = 0; step < 500; ++step)
        {
            output = Update(state, configuration, modifiers, 0.0, 0.002, false);
        }
        Check(output.deflection_meters < 1.0e-6, "airborne deflection decays");
        Check(output.contact_fraction == 0.0, "airborne contact fraction zero");
    }

    {
        TireContactState state;
        TireContactModelReset(state, configuration, modifiers);
        TireContactOutput output;
        for (int step = 0; step < 3000; ++step)
        {
            output = Update(state, configuration, modifiers, 30000.0, 0.002, true);
        }
        Check(output.deflection_meters <= output.maximum_deflection_meters + 1.0e-9, "deflection clamp bounded");
        Check(output.clamp_count > 0, "deflection clamp detected");
    }

    {
        double d120 = SimulateFrequency(120.0, configuration);
        double d240 = SimulateFrequency(240.0, configuration);
        double d480 = SimulateFrequency(480.0, configuration);
        double d500 = SimulateFrequency(500.0, configuration);
        double d1000 = SimulateFrequency(1000.0, configuration);
        Check(std::isfinite(d120) && std::isfinite(d240) && std::isfinite(d480)
            && std::isfinite(d500) && std::isfinite(d1000), "multi frequency finite response");
        Check(fabs(d500 - d1000) / fmax(fabs(d1000), 1.0e-6) < 0.05, "500 and 1000 Hz convergence");
        Check(fabs(d240 - d480) / fmax(fabs(d480), 1.0e-6) < 0.15, "240 and 480 Hz bounded difference");
    }

    {
        TireMechanicalModifiers soft = IdentityModifiers();
        TireMechanicalModifiers stiff = IdentityModifiers();
        stiff.vertical_stiffness_scale = 1.6;
        stiff.maximum_deflection_scale = 0.65;
        TireContactState softState;
        TireContactState stiffState;
        TireContactModelReset(softState, configuration, soft);
        TireContactModelReset(stiffState, configuration, stiff);
        TireContactOutput softOutput;
        TireContactOutput stiffOutput;
        for (int step = 0; step < 2000; ++step)
        {
            softOutput = Update(softState, configuration, soft, 3000.0, 0.002, true);
            stiffOutput = Update(stiffState, configuration, stiff, 3000.0, 0.002, true);
        }
        Check(stiffOutput.deflection_meters < softOutput.deflection_meters, "higher pressure reduces deflection");
        Check(stiffOutput.maximum_deflection_meters < softOutput.maximum_deflection_meters,
            "higher pressure reduces maximum deflection");
    }

    if (failures != 0)
    {
        std::printf("%d contact validation failures\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("contact validation passed\n");
    return EXIT_SUCCESS;
}
