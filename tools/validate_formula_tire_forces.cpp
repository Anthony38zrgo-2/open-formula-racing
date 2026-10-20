#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "tire_force_model.h"

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

static TireForceConfiguration BaseConfiguration()
{
    TireModelConfiguration configuration;
    TireModelConfigurationSetDefaults(configuration);
    TireForceConfiguration force = configuration.force[0];
    force.relaxation_enabled = 0;
    force.peak_migration_enabled = 0;
    force.contact_ramp_seconds = 0.0;
    force.compound_friction = 2.7;
    force.longitudinal_grip_ratio = 0.95;
    force.lateral_peak_slip_angle_rad = 0.115;
    force.longitudinal_peak_slip_ratio = 0.12;
    force.lateral_slide_grip_ratio = 0.88;
    force.longitudinal_slide_grip_ratio = 0.88;
    return force;
}

static TireForceInput BaseInput()
{
    TireForceInput input;
    input.normal_force_newtons = 4000.0;
    input.surface_friction_multiplier = 1.0;
    input.surface_stiffness = 5.0;
    input.surface_rolling_resistance = 0.002;
    input.effective_radius_meters = 0.32;
    input.forward_velocity_meters_per_second = 30.0;
    input.lateral_velocity_meters_per_second = 0.0;
    input.spin_velocity_radians_per_second = 30.0 / 0.32;
    input.camber_radians = 0.0;
    input.contact_fraction = 1.0;
    input.contact_patch_ratio = 1.0;
    input.load_sensitivity_scale = 1.0;
    input.thermal_grip_scale = 1.0;
    input.wear_grip_scale = 1.0;
    input.external_grip_scale = 1.0;
    input.force_stiffness_scale = 1.0;
    input.relaxation_length_scale = 1.0;
    input.pneumatic_trail_scale = 1.0;
    input.rolling_resistance_scale = 1.0;
    input.elapsed_seconds = 0.002;
    input.wheel_right = 0;
    return input;
}

static TireForceOutput Evaluate(TireForceState& state, const TireForceConfiguration& configuration,
    const TireForceInput& input)
{
    TireForceOutput output;
    TireForceModelEvaluate(state, configuration, input, output);
    return output;
}

static double LateralSweepPeak(TireForceConfiguration configuration, bool& peakAtExpected)
{
    TireForceState state;
    TireForceModelReset(state, configuration, 0.32);
    double best = 0.0;
    double bestAlpha = 0.0;
    for (int step = 0; step <= 600; ++step)
    {
        double alpha = -0.30 + step * 0.001;
        TireForceInput input = BaseInput();
        input.lateral_velocity_meters_per_second = -tan(alpha) * input.forward_velocity_meters_per_second;
        TireForceOutput output = Evaluate(state, configuration, input);
        if (fabs(output.lateral_force_newtons) > fabs(best))
        {
            best = output.lateral_force_newtons;
            bestAlpha = alpha;
        }
    }
    peakAtExpected = fabs(fabs(bestAlpha) - configuration.lateral_peak_slip_angle_rad) <= 0.004;
    return fabs(best);
}

static double LongitudinalSweepPeak(TireForceConfiguration configuration, bool& peakAtExpected)
{
    TireForceState state;
    TireForceModelReset(state, configuration, 0.32);
    double best = 0.0;
    double bestKappa = 0.0;
    for (int step = 0; step <= 800; ++step)
    {
        double kappa = -0.20 + step * 0.001;
        TireForceInput input = BaseInput();
        double wheelVelocity = kappa >= 0.0
            ? input.forward_velocity_meters_per_second / (1.0 - kappa)
            : input.forward_velocity_meters_per_second * (1.0 + kappa);
        input.spin_velocity_radians_per_second = wheelVelocity / input.effective_radius_meters;
        TireForceOutput output = Evaluate(state, configuration, input);
        if (fabs(output.longitudinal_force_newtons) > fabs(best))
        {
            best = output.longitudinal_force_newtons;
            bestKappa = kappa;
        }
    }
    peakAtExpected = fabs(fabs(bestKappa) - configuration.longitudinal_peak_slip_ratio) <= 0.004;
    return fabs(best);
}

int main()
{
    TireForceConfiguration configuration = BaseConfiguration();
    bool peakAtExpected = false;
    double lateralPeak = LateralSweepPeak(configuration, peakAtExpected);
    Check(fabs(lateralPeak) > 10000.0 && fabs(lateralPeak) < 11000.0, "lateral peak capacity");
    Check(peakAtExpected, "lateral peak slip location");
    double longitudinalPeak = LongitudinalSweepPeak(configuration, peakAtExpected);
    Check(fabs(longitudinalPeak) > 9500.0 && fabs(longitudinalPeak) < 10500.0, "longitudinal peak capacity");
    Check(peakAtExpected, "longitudinal peak slip location");

    {
        TireForceState state;
        TireForceModelReset(state, configuration, 0.32);
        TireForceInput input = BaseInput();
        input.forward_velocity_meters_per_second = 30.0;
        input.spin_velocity_radians_per_second = 30.0 / 0.32;
        input.lateral_velocity_meters_per_second = -0.75 * 30.0;
        TireForceOutput low = Evaluate(state, configuration, input);
        input.lateral_velocity_meters_per_second = -3.0 * 30.0;
        TireForceOutput high = Evaluate(state, configuration, input);
        Check(fabs(high.lateral_force_newtons) < fabs(low.lateral_force_newtons), "post peak lateral falloff");
        Check(fabs(high.lateral_force_newtons) > 0.80 * fabs(low.lateral_force_newtons), "lateral sliding plateau");
    }

    {
        TireForceState state;
        TireForceModelReset(state, configuration, 0.32);
        TireForceInput input = BaseInput();
        input.lateral_velocity_meters_per_second = -tan(0.115) * 30.0;
        TireForceOutput output = Evaluate(state, configuration, input);
        Check(output.aligning_torque_newton_meters > 0.0, "aligning torque sign");
        double peakTorque = fabs(output.aligning_torque_newton_meters);
        input.lateral_velocity_meters_per_second = -tan(0.55) * 30.0;
        output = Evaluate(state, configuration, input);
        Check(fabs(output.aligning_torque_newton_meters) < peakTorque, "aligning torque collapses with saturation");
    }

    {
        TireForceState state;
        TireForceModelReset(state, configuration, 0.32);
        TireForceInput input = BaseInput();
        input.lateral_velocity_meters_per_second = -tan(0.40) * 30.0;
        double wheelVelocity = 30.0 / 0.25;
        input.spin_velocity_radians_per_second = wheelVelocity / 0.32;
        TireForceOutput output = Evaluate(state, configuration, input);
        Check(output.combined_utilization <= 1.000001, "combined utilization budget");
        Check(std::isfinite(output.longitudinal_force_newtons) && std::isfinite(output.lateral_force_newtons),
            "combined forces finite");
        double normalizedForce = sqrt(pow(output.longitudinal_force_newtons
            / fmax(output.longitudinal_capacity_newtons, 1.0), 2.0)
            + pow(output.lateral_force_newtons / fmax(output.lateral_capacity_newtons, 1.0), 2.0));
        Check(normalizedForce <= 1.0001, "combined ellipse projection");
    }

    {
        double previousMu = 1.0e9;
        bool monotone = true;
        double zeroForce = -1.0;
        for (int step = 0; step <= 5; ++step)
        {
            double loads[6] = { 0.0, 100.0, 1200.0, 3200.0, 6000.0, 12000.0 };
            TireForceState state;
            TireForceModelReset(state, configuration, 0.32);
            TireForceInput input = BaseInput();
            input.normal_force_newtons = loads[step];
            input.lateral_velocity_meters_per_second = -tan(0.115) * 30.0;
            double loadRatio = fmax(loads[step] / 4000.0, 0.05);
            input.load_sensitivity_scale = fmin(1.20, fmax(0.75, pow(loadRatio, -0.08)));
            TireForceOutput output = Evaluate(state, configuration, input);
            if (step == 0)
            {
                zeroForce = fabs(output.lateral_force_newtons);
            }
            double mu = output.lateral_capacity_newtons / fmax(loads[step], 1.0);
            if (step > 1 && mu > previousMu + 1.0e-9)
            {
                monotone = false;
            }
            if (step > 0)
            {
                previousMu = mu;
            }
        }
        Check(zeroForce <= 1.0e-9, "zero load produces zero force");
        Check(monotone, "load sensitivity decreases effective mu");
    }

    {
        TireForceState rightState;
        TireForceState leftState;
        TireForceModelReset(rightState, configuration, 0.32);
        TireForceModelReset(leftState, configuration, 0.32);
        TireForceInput right = BaseInput();
        TireForceInput left = BaseInput();
        right.camber_radians = -0.02;
        left.camber_radians = 0.02;
        right.wheel_right = 1;
        left.wheel_right = 0;
        TireForceOutput rightOutput = Evaluate(rightState, configuration, right);
        TireForceOutput leftOutput = Evaluate(leftState, configuration, left);
        Check(fabs(rightOutput.lateral_force_newtons + leftOutput.lateral_force_newtons) < 10.0,
            "symmetric camber cancels lateral thrust");
    }

    {
        TireForceState state;
        TireForceModelReset(state, configuration, 0.32);
        TireForceInput input = BaseInput();
        input.forward_velocity_meters_per_second = -10.0;
        input.lateral_velocity_meters_per_second = 1.0;
        input.spin_velocity_radians_per_second = -10.0 / 0.32;
        TireForceOutput output = Evaluate(state, configuration, input);
        Check(output.lateral_force_newtons < 0.0, "reverse lateral force opposes motion");
        input.spin_velocity_radians_per_second = -12.0 / 0.32;
        output = Evaluate(state, configuration, input);
        Check(output.longitudinal_force_newtons < 0.0, "reverse longitudinal force sign");
    }

    {
        TireForceConfiguration relaxed = configuration;
        relaxed.relaxation_enabled = 1;
        relaxed.minimum_relaxation_seconds = 0.045;
        TireForceState state;
        TireForceModelReset(state, relaxed, 0.32);
        TireForceInput input = BaseInput();
        input.lateral_velocity_meters_per_second = -tan(0.115) * 30.0;
        TireForceOutput first = Evaluate(state, relaxed, input);
        TireForceOutput steady;
        for (int step = 0; step < 2000; ++step)
        {
            steady = Evaluate(state, relaxed, input);
        }
        Check(fabs(first.lateral_force_newtons) < fabs(steady.lateral_force_newtons) * 0.75,
            "relaxation delays force build");
        Check(fabs(fabs(steady.lateral_force_newtons) - lateralPeak) / lateralPeak < 0.02,
            "relaxation converges to steady force");    }

    {
        TireForceConfiguration migrated = configuration;
        migrated.peak_migration_enabled = 1;
        TireForceState state;
        TireForceModelReset(state, migrated, 0.32);
        TireForceInput input = BaseInput();
        input.lateral_velocity_meters_per_second = -tan(0.20) * 30.0;
        input.spin_velocity_radians_per_second = (30.0 / 0.85) / 0.32;
        TireForceOutput previous = Evaluate(state, migrated, input);
        bool finite = true;
        bool monotone = true;
        for (int step = 0; step < 200; ++step)
        {
            TireForceOutput current = Evaluate(state, migrated, input);
            if (!std::isfinite(current.longitudinal_force_newtons)
                || !std::isfinite(current.lateral_force_newtons))
            {
                finite = false;
            }
            if (fabs(current.lateral_force_newtons) > fabs(previous.lateral_force_newtons) + 1.0)
            {
                monotone = false;
            }
            previous = current;
        }
        Check(finite, "peak migration finite");
        Check(monotone, "peak migration monotone demand");
    }

    if (failures != 0)
    {
        std::printf("%d force validation failures\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("force validation passed\n");
    return EXIT_SUCCESS;
}
