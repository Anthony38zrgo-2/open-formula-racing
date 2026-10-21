#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <tgf.h>

#include "sim.h"
#include "tire_model_configuration.h"
#include "tire_vertical_geometry.h"
#include "tire_vertical_solver.h"

static int checks = 0;
static int failures = 0;

static void Check(bool condition, const char* name)
{
    checks += 1;
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

static void CheckClose(double actual, double expected, double tolerance, const char* name)
{
    checks += 1;
    const bool passed = std::fabs(actual - expected) <= tolerance;
    if (!passed)
    {
        std::printf("FAIL %s actual=%.17g expected=%.17g tolerance=%.3g\n",
            name, actual, expected, tolerance);
        failures += 1;
    }
    else
    {
        std::printf("PASS %s\n", name);
    }
}

static TireVerticalQuarterCarConfiguration BuildQuarterCarConfiguration()
{
    TireVerticalQuarterCarConfiguration configuration = {};
    configuration.sprung_mass_kilograms = 150.0;
    configuration.unsprung_mass_kilograms = 13.0;
    configuration.gravity_meters_per_second_squared = 9.81;

    TireVerticalCornerConfiguration& corner = configuration.corner;
    corner.spring.spring_constant_newtons_per_meter = -100000.0;
    corner.spring.preload_newtons = 150.0 * 9.81;
    corner.spring.reference_position_meters = 0.05;
    corner.spring.minimum_position_meters = 0.02;
    corner.spring.maximum_position_meters = 0.15;
    corner.spring.bellcrank_ratio = 1.0;
    corner.spring.inertance_kilograms = 0.0;

    corner.damper.bump.slow_coefficient_newtons_per_second_per_meter = 2000.0;
    corner.damper.bump.fast_coefficient_newtons_per_second_per_meter = 1000.0;
    corner.damper.bump.transition_velocity_meters_per_second = 0.15;
    corner.damper.bump.slow_intercept_newtons = 0.0;
    corner.damper.bump.fast_intercept_newtons = (2000.0 - 1000.0) * 0.15;
    corner.damper.rebound.slow_coefficient_newtons_per_second_per_meter = 3000.0;
    corner.damper.rebound.fast_coefficient_newtons_per_second_per_meter = 1500.0;
    corner.damper.rebound.transition_velocity_meters_per_second = 0.15;
    corner.damper.rebound.slow_intercept_newtons = 0.0;
    corner.damper.rebound.fast_intercept_newtons = (3000.0 - 1500.0) * 0.15;
    corner.damper.efficiency = 1.0;

    corner.tire_radial_stiffness_newtons_per_meter = 165000.0;
    corner.tire_radial_damping_newtons_per_second_per_meter = 1300.0;
    corner.unloaded_radius_meters = 0.312;
    return configuration;
}

static void CheckNativeSuspensionLaw()
{
    TireVerticalQuarterCarConfiguration configuration = BuildQuarterCarConfiguration();
    tSuspension native = {};
    native.spring.K = static_cast<tdble>(
        configuration.corner.spring.spring_constant_newtons_per_meter);
    native.spring.F0 = static_cast<tdble>(configuration.corner.spring.preload_newtons);
    native.spring.x0 = static_cast<tdble>(configuration.corner.spring.reference_position_meters);
    native.spring.bellcrank = static_cast<tdble>(configuration.corner.spring.bellcrank_ratio);
    native.damper.bump.C1 = static_cast<tdble>(
        configuration.corner.damper.bump.slow_coefficient_newtons_per_second_per_meter);
    native.damper.bump.C2 = static_cast<tdble>(
        configuration.corner.damper.bump.fast_coefficient_newtons_per_second_per_meter);
    native.damper.bump.v1 = static_cast<tdble>(
        configuration.corner.damper.bump.transition_velocity_meters_per_second);
    native.damper.bump.b1 = static_cast<tdble>(
        configuration.corner.damper.bump.slow_intercept_newtons);
    native.damper.bump.b2 = static_cast<tdble>(
        configuration.corner.damper.bump.fast_intercept_newtons);
    native.damper.rebound.C1 = static_cast<tdble>(
        configuration.corner.damper.rebound.slow_coefficient_newtons_per_second_per_meter);
    native.damper.rebound.C2 = static_cast<tdble>(
        configuration.corner.damper.rebound.fast_coefficient_newtons_per_second_per_meter);
    native.damper.rebound.v1 = static_cast<tdble>(
        configuration.corner.damper.rebound.transition_velocity_meters_per_second);
    native.damper.rebound.b1 = static_cast<tdble>(
        configuration.corner.damper.rebound.slow_intercept_newtons);
    native.damper.rebound.b2 = static_cast<tdble>(
        configuration.corner.damper.rebound.fast_intercept_newtons);
    native.damper.efficiency = 1.0;
    native.inertance = 0.0;
    native.a = 0.0;

    const double positions[5] = { 0.02, 0.035, 0.05, 0.09, 0.15 };
    const double velocities[5] = { -0.5, -0.1, 0.0, 0.1, 0.5 };
    double largest_error = 0.0;
    for (int position_index = 0; position_index < 5; ++position_index)
    {
        for (int velocity_index = 0; velocity_index < 5; ++velocity_index)
        {
            native.x = static_cast<tdble>(positions[position_index]);
            native.v = static_cast<tdble>(velocities[velocity_index]);
            native.force = 0.0;
            SimSuspUpdate(&native);
            const double expected = TireVerticalCornerSuspensionForce(configuration.corner,
                positions[position_index], -velocities[velocity_index]);
            largest_error = std::max(largest_error,
                std::fabs(static_cast<double>(native.force) - expected));
        }
    }
    Check(largest_error < 2.0e-3, "native suspension law matches pure evaluator");

    double largest_potential_error = 0.0;
    const double potential_step = 1.0e-6;
    for (int position_index = 0; position_index < 5; ++position_index)
    {
        const double position = positions[position_index];
        const double numerical_force = -(TireVerticalSpringPotentialJoules(
                configuration.corner.spring, position + potential_step)
            - TireVerticalSpringPotentialJoules(configuration.corner.spring,
                position - potential_step)) / (2.0 * potential_step);
        largest_potential_error = std::max(largest_potential_error,
            std::fabs(numerical_force
                - TireVerticalSpringForce(configuration.corner.spring, position)));
    }
    Check(largest_potential_error < 1.0e-6,
        "spring potential derivative matches native spring force");
}

static double QuarterCarNumericalLoss(const TireVerticalQuarterCarConfiguration& configuration,
    double initial_energy, double final_energy, double road_work, double damper_loss,
    double tire_loss, double constraint_loss)
{
    (void)configuration;
    return -(final_energy - initial_energy - road_work + damper_loss + tire_loss
        + constraint_loss);
}

static void RunQuarterCarSimulation(const TireVerticalQuarterCarConfiguration& configuration,
    const TireVerticalQuarterCarState& initial_state, double duration_seconds,
    double elapsed_seconds, double ground_height_meters, double ground_velocity,
    TireVerticalQuarterCarState& final_state, double& initial_energy, double& final_energy,
    double& numerical_loss, double& maximum_absolute_suspension, int& step_failures)
{
    TireVerticalQuarterCarState state = initial_state;
    initial_energy = TireVerticalQuarterCarEnergyJoules(configuration, state,
        ground_height_meters);
    double road_work = 0.0;
    double damper_loss = 0.0;
    double tire_loss = 0.0;
    double constraint_loss = 0.0;
    maximum_absolute_suspension = std::fabs(state.suspension_extension_meters);
    step_failures = 0;

    const int step_count = static_cast<int>(std::lround(duration_seconds / elapsed_seconds));
    for (int step = 0; step < step_count; ++step)
    {
        TireVerticalQuarterCarStepInput input = {};
        input.elapsed_seconds = elapsed_seconds;
        input.ground_height_meters = ground_height_meters;
        input.ground_height_velocity_meters_per_second = ground_velocity;

        TireVerticalQuarterCarDiagnostics diagnostics;
        TireVerticalQuarterCarState next_state;
        const double old_normal_force = 0.0;
        (void)old_normal_force;
        if (!TireVerticalQuarterCarStep(configuration, state, input, next_state, diagnostics))
        {
            step_failures += 1;
            break;
        }
        road_work += diagnostics.tire_normal_force_newtons * ground_velocity * elapsed_seconds;
        damper_loss += diagnostics.damper_energy_loss_joules;
        tire_loss += diagnostics.tire_damping_energy_loss_joules;
        constraint_loss += diagnostics.constraint_energy_loss_joules;
        state = next_state;
        maximum_absolute_suspension = std::max(maximum_absolute_suspension,
            std::fabs(state.suspension_extension_meters));
    }

    final_state = state;
    final_energy = TireVerticalQuarterCarEnergyJoules(configuration, state, ground_height_meters);
    numerical_loss = QuarterCarNumericalLoss(configuration, initial_energy, final_energy,
        road_work, damper_loss, tire_loss, constraint_loss);
}

static void CheckQuarterCarEquilibrium()
{
    TireVerticalQuarterCarConfiguration configuration = BuildQuarterCarConfiguration();
    TireVerticalQuarterCarState state = {};
    TireVerticalQuarterCarDiagnostics diagnostics;
    Check(TireVerticalQuarterCarInitializeEquilibrium(configuration, 0.0, state, diagnostics),
        "quarter car equilibrium converges");
    CheckClose(state.body_position_meters, 0.040309, 2.0e-5,
        "quarter car equilibrium body height");
    CheckClose(state.suspension_extension_meters, 0.05, 1.0e-8,
        "quarter car equilibrium extension");
    CheckClose(diagnostics.tire_normal_force_newtons, 163.0 * 9.81, 1.0e-3,
        "quarter car equilibrium normal force");
    CheckClose(diagnostics.suspension_force_newtons, 150.0 * 9.81, 1.0e-3,
        "quarter car equilibrium suspension force");
    CheckClose(diagnostics.tire_deflection_meters,
        diagnostics.tire_normal_force_newtons / 165000.0, 1.0e-10,
        "quarter car equilibrium deflection");
    Check(diagnostics.tire_contact, "quarter car equilibrium contact");

    TireVerticalQuarterCarState positive = state;
    positive.suspension_extension_meters += 0.002;
    TireVerticalQuarterCarState final_state;
    double initial_energy = 0.0;
    double final_energy = 0.0;
    double numerical_loss = 0.0;
    double maximum_extension = 0.0;
    int step_failures = 0;
    RunQuarterCarSimulation(configuration, positive, 2.0, 0.002, 0.0, 0.0,
        final_state, initial_energy, final_energy, numerical_loss, maximum_extension,
        step_failures);
    Check(step_failures == 0, "quarter car positive perturbation steps converge");
    CheckClose(final_state.suspension_extension_meters, 0.05, 5.0e-5,
        "quarter car positive perturbation settles extension");
    Check(std::fabs(final_state.suspension_extension_velocity_meters_per_second) < 0.002,
        "quarter car positive perturbation settles velocity");
    Check(numerical_loss >= -1.0e-6, "quarter car positive perturbation no energy creation");
    Check(numerical_loss < 0.05 * std::fabs(initial_energy),
        "quarter car positive perturbation numerical loss bounded");

    TireVerticalQuarterCarState negative = state;
    negative.suspension_extension_meters -= 0.002;
    RunQuarterCarSimulation(configuration, negative, 2.0, 0.002, 0.0, 0.0,
        final_state, initial_energy, final_energy, numerical_loss, maximum_extension,
        step_failures);
    Check(step_failures == 0, "quarter car negative perturbation steps converge");
    CheckClose(final_state.suspension_extension_meters, 0.05, 5.0e-5,
        "quarter car negative perturbation settles extension");
    Check(numerical_loss >= -1.0e-6, "quarter car negative perturbation no energy creation");
    Check(numerical_loss < 0.05 * std::fabs(initial_energy),
        "quarter car negative perturbation numerical loss bounded");
}

static void CheckQuarterCarChassisImpulse()
{
    TireVerticalQuarterCarConfiguration configuration = BuildQuarterCarConfiguration();
    TireVerticalQuarterCarState state = {};
    TireVerticalQuarterCarDiagnostics diagnostics;
    TireVerticalQuarterCarInitializeEquilibrium(configuration, 0.0, state, diagnostics);
    state.body_velocity_meters_per_second = 0.1;

    TireVerticalQuarterCarState final_state;
    double initial_energy = 0.0;
    double final_energy = 0.0;
    double numerical_loss = 0.0;
    double maximum_extension = 0.0;
    int step_failures = 0;
    RunQuarterCarSimulation(configuration, state, 2.0, 0.002, 0.0, 0.0,
        final_state, initial_energy, final_energy, numerical_loss, maximum_extension,
        step_failures);
    Check(step_failures == 0, "quarter car chassis impulse steps converge");
    Check(std::fabs(final_state.body_velocity_meters_per_second) < 0.002,
        "quarter car chassis impulse settles body velocity");
    Check(std::fabs(final_state.suspension_extension_velocity_meters_per_second) < 0.002,
        "quarter car chassis impulse settles suspension velocity");
    Check(numerical_loss >= -1.0e-6, "quarter car chassis impulse no energy creation");
    Check(numerical_loss < 0.05 * std::fabs(initial_energy),
        "quarter car chassis impulse numerical loss bounded");
}

static void CheckQuarterCarLimits()
{
    TireVerticalQuarterCarConfiguration configuration = BuildQuarterCarConfiguration();
    TireVerticalQuarterCarState state = {};
    TireVerticalQuarterCarDiagnostics diagnostics;
    TireVerticalQuarterCarInitializeEquilibrium(configuration, 0.0, state, diagnostics);

    TireVerticalQuarterCarState compressed = state;
    compressed.suspension_extension_meters =
        configuration.corner.spring.minimum_position_meters;
    compressed.suspension_extension_velocity_meters_per_second = -2.0;
    TireVerticalQuarterCarState next_state;
    TireVerticalQuarterCarStepInput input = {};
    input.elapsed_seconds = 0.002;
    const int compressed_ok = TireVerticalQuarterCarStep(configuration, compressed, input,
        next_state, diagnostics);
    Check(compressed_ok, "quarter car lower limit step converges");
    Check(next_state.suspension_extension_meters
        >= configuration.corner.spring.minimum_position_meters - 1.0e-9,
        "quarter car lower limit holds extension");
    Check(diagnostics.lower_limit_active, "quarter car lower limit active");
    Check(diagnostics.limit_impulse_newton_seconds > 0.0,
        "quarter car lower limit impulse sign");
    Check(diagnostics.constraint_energy_loss_joules > 0.0,
        "quarter car lower limit dissipates energy");

    TireVerticalQuarterCarState extended = state;
    extended.body_position_meters += 0.2;
    extended.suspension_extension_meters =
        configuration.corner.spring.maximum_position_meters - 0.0005;
    extended.suspension_extension_velocity_meters_per_second = 2.0;
    const int extended_ok = TireVerticalQuarterCarStep(configuration, extended, input,
        next_state, diagnostics);
    Check(extended_ok, "quarter car upper limit step converges");
    Check(next_state.suspension_extension_meters
        <= configuration.corner.spring.maximum_position_meters + 1.0e-9,
        "quarter car upper limit holds extension");
    Check(diagnostics.upper_limit_active, "quarter car upper limit active");
    Check(diagnostics.limit_impulse_newton_seconds < 0.0,
        "quarter car upper limit impulse sign");
    Check(diagnostics.constraint_energy_loss_joules > 0.0,
        "quarter car upper limit dissipates energy");
}

static void CheckQuarterCarZeroTime()
{
    TireVerticalQuarterCarConfiguration configuration = BuildQuarterCarConfiguration();
    TireVerticalQuarterCarState state = {};
    TireVerticalQuarterCarDiagnostics diagnostics;
    TireVerticalQuarterCarInitializeEquilibrium(configuration, 0.0, state, diagnostics);
    const TireVerticalQuarterCarState before = state;
    TireVerticalQuarterCarStepInput input = {};
    input.elapsed_seconds = 0.0;
    TireVerticalQuarterCarState next_state;
    const int result = TireVerticalQuarterCarStep(configuration, state, input, next_state,
        diagnostics);
    Check(result == 0, "quarter car zero time rejected");
    Check(next_state.body_position_meters == before.body_position_meters
        && next_state.suspension_extension_meters == before.suspension_extension_meters,
        "quarter car zero time keeps state");
}

static void CheckQuarterCarContactLoss()
{
    TireVerticalQuarterCarConfiguration configuration = BuildQuarterCarConfiguration();
    TireVerticalQuarterCarState equilibrium = {};
    TireVerticalQuarterCarDiagnostics diagnostics;
    TireVerticalQuarterCarInitializeEquilibrium(configuration, 0.0, equilibrium, diagnostics);
    TireVerticalQuarterCarState state = equilibrium;
    state.body_position_meters += 0.05;
    state.body_velocity_meters_per_second = 0.1;
    const double initial_extension = state.suspension_extension_meters;

    int failures_in_flight = 0;
    int contact_during_flight = 0;
    for (int step = 0; step < 10; ++step)
    {
        TireVerticalQuarterCarStepInput input = {};
        input.elapsed_seconds = 0.002;
        TireVerticalQuarterCarState next_state;
        if (!TireVerticalQuarterCarStep(configuration, state, input, next_state, diagnostics))
        {
            failures_in_flight += 1;
            break;
        }
        state = next_state;
        contact_during_flight += diagnostics.tire_contact;
    }
    Check(failures_in_flight == 0, "quarter car contact loss steps converge");
    Check(contact_during_flight == 0, "quarter car contact loss stays airborne");
    Check(std::fabs(state.suspension_extension_meters - initial_extension) > 1.0e-6,
        "quarter car contact loss keeps suspension dynamics");

    state.body_position_meters = equilibrium.body_position_meters;
    state.body_velocity_meters_per_second = 0.0;
    TireVerticalQuarterCarState final_state;
    double initial_energy = 0.0;
    double final_energy = 0.0;
    double numerical_loss = 0.0;
    double maximum_extension = 0.0;
    int step_failures = 0;
    RunQuarterCarSimulation(configuration, state, 1.0, 0.002, 0.0, 0.0,
        final_state, initial_energy, final_energy, numerical_loss, maximum_extension,
        step_failures);
    Check(step_failures == 0, "quarter car contact recovery steps converge");
    Check(std::fabs(final_state.suspension_extension_meters
        - equilibrium.suspension_extension_meters) < 1.0e-3,
        "quarter car contact recovery settles extension");
    Check(std::fabs(final_state.body_position_meters
        - equilibrium.body_position_meters) < 1.0e-3,
        "quarter car contact recovery settles body");
    Check(numerical_loss >= -1.0e-6, "quarter car contact recovery no energy creation");
}

static void CheckQuarterCarRateComparison()
{
    TireVerticalQuarterCarConfiguration configuration = BuildQuarterCarConfiguration();
    TireVerticalQuarterCarState equilibrium = {};
    TireVerticalQuarterCarDiagnostics diagnostics;
    TireVerticalQuarterCarInitializeEquilibrium(configuration, 0.0, equilibrium, diagnostics);
    TireVerticalQuarterCarState state = equilibrium;
    state.suspension_extension_meters += 0.002;

    TireVerticalQuarterCarState coarse;
    TireVerticalQuarterCarState fine;
    double initial_energy = 0.0;
    double final_energy = 0.0;
    double numerical_loss = 0.0;
    double maximum_extension = 0.0;
    int step_failures = 0;
    RunQuarterCarSimulation(configuration, state, 0.2, 0.002, 0.0, 0.0,
        coarse, initial_energy, final_energy, numerical_loss, maximum_extension,
        step_failures);
    Check(step_failures == 0, "quarter car rate comparison coarse steps converge");
    RunQuarterCarSimulation(configuration, state, 0.2, 0.001, 0.0, 0.0,
        fine, initial_energy, final_energy, numerical_loss, maximum_extension,
        step_failures);
    Check(step_failures == 0, "quarter car rate comparison fine steps converge");
    CheckClose(coarse.suspension_extension_meters, fine.suspension_extension_meters,
        1.0e-3, "quarter car 500 and 1000 Hz extension sample");
    CheckClose(coarse.body_position_meters, fine.body_position_meters,
        1.0e-3, "quarter car 500 and 1000 Hz body sample");
}

static void CopyDamperConfiguration(const tDamper& native, TireVerticalDamperConfiguration& configuration)
{
    configuration.bump.slow_coefficient_newtons_per_second_per_meter = native.bump.C1;
    configuration.bump.fast_coefficient_newtons_per_second_per_meter = native.bump.C2;
    configuration.bump.transition_velocity_meters_per_second = native.bump.v1;
    configuration.bump.slow_intercept_newtons = native.bump.b1;
    configuration.bump.fast_intercept_newtons = native.bump.b2;
    configuration.rebound.slow_coefficient_newtons_per_second_per_meter = native.rebound.C1;
    configuration.rebound.fast_coefficient_newtons_per_second_per_meter = native.rebound.C2;
    configuration.rebound.transition_velocity_meters_per_second = native.rebound.v1;
    configuration.rebound.slow_intercept_newtons = native.rebound.b1;
    configuration.rebound.fast_intercept_newtons = native.rebound.b2;
    configuration.efficiency = native.efficiency;
}

static bool LoadWilliamsSolverConfiguration(const char* path,
    TireVerticalSolverConfiguration& configuration)
{
    void* parameters = GfParmReadFile(path, GFPARM_RMODE_STD);
    if (parameters == NULL)
    {
        std::printf("FAIL cannot read %s\n", path);
        failures += 1;
        return false;
    }

    const char* wheel_sections[4] = {
        "Front Right Wheel", "Front Left Wheel", "Rear Right Wheel", "Rear Left Wheel"
    };
    const char* suspension_sections[4] = {
        "Front Right Suspension", "Front Left Suspension", "Rear Right Suspension", "Rear Left Suspension"
    };

    const double car_mass = GfParmGetNum(parameters, "Car", "mass", NULL, 0.0);
    const double initial_fuel_liters = GfParmGetNum(parameters, "Car", "initial fuel", NULL, 0.0);
    const double fuel_mass_multiplier = GfParmGetNum(parameters, "Car", "fuel mass mult", NULL, 0.0);
    const double total_mass = car_mass + initial_fuel_liters * fuel_mass_multiplier;
    const double body_length = GfParmGetNum(parameters, "Car", "body length", NULL, 0.0);
    const double body_width = GfParmGetNum(parameters, "Car", "body width", NULL, 0.0);
    const double body_height = GfParmGetNum(parameters, "Car", "body height", NULL, 0.0);
    const double mass_repartition_coefficient =
        GfParmGetNum(parameters, "Car", "mass repartition coefficient", NULL, 0.0);
    const double front_repartition = GfParmGetNum(parameters, "Car",
        "front-rear weight repartition", NULL, 0.0);
    const double front_left_repartition = GfParmGetNum(parameters, "Car",
        "front right-left weight repartition", NULL, 0.5);
    const double rear_left_repartition = GfParmGetNum(parameters, "Car",
        "rear right-left weight repartition", NULL, 0.5);
    const double left_repartition = front_repartition * front_left_repartition
        + (1.0 - front_repartition) * rear_left_repartition;
    const double center_of_gravity_y = -left_repartition * body_width + body_width * 0.5;
    const double front_axle_x = GfParmGetNum(parameters, "Front Axle", "xpos", NULL, 0.0);
    const double rear_axle_x = GfParmGetNum(parameters, "Rear Axle", "xpos", NULL, 0.0);
    const double center_of_gravity_x = front_axle_x * front_repartition
        + rear_axle_x * (1.0 - front_repartition);
    const double inertia_scale = mass_repartition_coefficient * mass_repartition_coefficient;

    std::memset(&configuration, 0, sizeof(configuration));
    configuration.total_mass_kilograms = total_mass;
    configuration.roll_inertia_kilogram_meters_squared = car_mass * inertia_scale
        * (body_width * body_width + body_height * body_height) / 12.0;
    configuration.pitch_inertia_kilogram_meters_squared = car_mass * inertia_scale
        * (body_length * body_length + body_height * body_height) / 12.0;
    configuration.gravity_meters_per_second_squared = 9.81;

    double radii[4] = {};
    double widths[4] = {};
    double masses[4] = {};
    double static_loads[4] = {};
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const bool front = wheel < 2;
        const double axle_x = front ? front_axle_x : rear_axle_x;
        const double wheel_y = GfParmGetNum(parameters, wheel_sections[wheel], "ypos", NULL, 0.0);
        const double rim_diameter = GfParmGetNum(parameters, wheel_sections[wheel],
            "rim diameter", NULL, 0.0);
        const double tire_width = GfParmGetNum(parameters, wheel_sections[wheel],
            "tire width", NULL, 0.0);
        const double tire_height = GfParmGetNum(parameters, wheel_sections[wheel],
            "tire height", NULL, -1.0);
        const double tire_height_ratio = GfParmGetNum(parameters, wheel_sections[wheel],
            "tire height-width ratio", NULL, 0.0);

        configuration.wheel_x_meters[wheel] = axle_x - center_of_gravity_x;
        configuration.wheel_y_meters[wheel] = wheel_y - center_of_gravity_y;
        radii[wheel] = tire_height > 0.0
            ? rim_diameter * 0.5 + tire_height
            : rim_diameter * 0.5 + tire_width * tire_height_ratio;
        widths[wheel] = tire_width;
        masses[wheel] = GfParmGetNum(parameters, wheel_sections[wheel], "mass", NULL, 0.0);
        static_loads[wheel] = car_mass * 9.81 * (front ? front_repartition : 1.0 - front_repartition)
            * 0.5;
        configuration.unsprung_mass_kilograms[wheel] = masses[wheel];
        configuration.corner[wheel].unloaded_radius_meters = radii[wheel];
    }

    TireModelConfiguration tire_configuration;
    TireModelConfigurationSetDefaults(tire_configuration);
    if (!TireModelConfigurationRead(tire_configuration, parameters, static_loads, radii,
            widths, masses, 101325.0, 298.15))
    {
        std::printf("FAIL tire model configuration not readable\n");
        GfParmReleaseHandle(parameters);
        failures += 1;
        return false;
    }

    tCar car = {};
    tCarElt vehicle = {};
    car.params = parameters;
    car.carElt = &vehicle;
    for (int axle = 0; axle < 2; ++axle)
    {
        SimAxleConfig(&car, axle);
        SimAxleReConfig(&car, axle, 0.0);
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        tWheel& native_wheel = car.wheel[wheel];
        native_wheel.weight0 = static_loads[wheel];
        native_wheel.mass = masses[wheel];
        native_wheel.staticPos.y = GfParmGetNum(parameters, wheel_sections[wheel],
            "ypos", NULL, 0.0);
        SimSuspConfig(&car, parameters, suspension_sections[wheel], &native_wheel.susp, wheel);
        SimSuspReConfig(&car, &native_wheel.susp, wheel, native_wheel.weight0,
            vehicle.setup.rideHeight[wheel].value);
        native_wheel.susp.x = native_wheel.susp.spring.x0;

        TireVerticalCornerConfiguration& corner = configuration.corner[wheel];
        corner.spring.spring_constant_newtons_per_meter = native_wheel.susp.spring.K;
        corner.spring.preload_newtons = native_wheel.susp.spring.F0;
        corner.spring.reference_position_meters = native_wheel.susp.spring.x0;
        corner.spring.minimum_position_meters = native_wheel.susp.spring.packers;
        corner.spring.maximum_position_meters = native_wheel.susp.spring.xMax;
        corner.spring.bellcrank_ratio = native_wheel.susp.spring.bellcrank;
        corner.spring.inertance_kilograms = native_wheel.susp.inertance;
        CopyDamperConfiguration(native_wheel.susp.damper, corner.damper);

        const int axle = wheel / 2;
        const double stiffness = tire_configuration.contact[axle]
            .radial_stiffness_newtons_per_meter;
        const double damping_ratio = std::max(0.05,
            std::min(1.5, tire_configuration.contact[axle].radial_damping_ratio));
        corner.tire_radial_stiffness_newtons_per_meter = stiffness;
        corner.tire_radial_damping_newtons_per_second_per_meter =
            2.0 * damping_ratio * std::sqrt(stiffness * std::max(masses[wheel], 1.0));

        std::printf("Williams wheel=%d x=%.9f y=%.9f radius=%.9f mass=%.6f "
            "spring=%.6f preload=%.6f x0=%.9f limits=%.9f..%.9f bellcrank=%.9f "
            "tire_stiffness=%.6f tire_damping=%.6f\n",
            wheel, configuration.wheel_x_meters[wheel], configuration.wheel_y_meters[wheel],
            radii[wheel], masses[wheel], corner.spring.spring_constant_newtons_per_meter,
            corner.spring.preload_newtons, corner.spring.reference_position_meters,
            corner.spring.minimum_position_meters, corner.spring.maximum_position_meters,
            corner.spring.bellcrank_ratio, corner.tire_radial_stiffness_newtons_per_meter,
            corner.tire_radial_damping_newtons_per_second_per_meter);
    }

    for (int axle = 0; axle < 2; ++axle)
    {
        TireVerticalAxleConfiguration& axle_configuration = configuration.axle[axle];
        axle_configuration.anti_roll_stiffness_newtons_per_meter =
            car.axle[axle].arbSusp.spring.K;
        axle_configuration.anti_roll_enabled =
            axle_configuration.anti_roll_stiffness_newtons_per_meter > 0.0;

        const tSuspension& heave = car.axle[axle].heaveSusp;
        axle_configuration.heave_spring.spring_constant_newtons_per_meter = heave.spring.K;
        axle_configuration.heave_spring.preload_newtons = heave.spring.F0;
        axle_configuration.heave_spring.reference_position_meters = heave.spring.x0;
        axle_configuration.heave_spring.minimum_position_meters = heave.spring.packers;
        axle_configuration.heave_spring.maximum_position_meters = heave.spring.xMax;
        axle_configuration.heave_spring.bellcrank_ratio = heave.spring.bellcrank;
        axle_configuration.heave_spring.inertance_kilograms = heave.inertance;
        CopyDamperConfiguration(heave.damper, axle_configuration.heave_damper);
        axle_configuration.heave_enabled = heave.spring.K != 0.0
            || heave.spring.F0 != 0.0
            || heave.damper.bump.C1 != 0.0 || heave.damper.bump.C2 != 0.0
            || heave.damper.rebound.C1 != 0.0 || heave.damper.rebound.C2 != 0.0;

        std::printf("Williams axle=%d arb=%.6f heave_enabled=%d heave_K=%.6f heave_F0=%.6f "
            "heave_b=%.9f\n",
            axle, axle_configuration.anti_roll_stiffness_newtons_per_meter,
            axle_configuration.heave_enabled,
            axle_configuration.heave_spring.spring_constant_newtons_per_meter,
            axle_configuration.heave_spring.preload_newtons,
            axle_configuration.heave_spring.bellcrank_ratio);
    }

    GfParmReleaseHandle(parameters);

    char error_message[256] = {};
    if (!TireVerticalSolverConfigurationValidate(configuration, error_message,
            sizeof(error_message)))
    {
        std::printf("FAIL solver configuration: %s\n", error_message);
        failures += 1;
        return false;
    }
    return true;
}

struct FourWheelRunResult
{
    TireVerticalSolverState final_state;
    TireVerticalSolverState sample_state;
    double initial_energy_joules;
    double final_energy_joules;
    double numerical_loss_joules;
    double maximum_residual_newtons;
    double minimum_normal_force_after_settle_newtons;
    int contact_loss_steps_after_settle;
    int step_failures;
    int sampled;
};

static double GroundBumpHeight(double time_seconds)
{
    const double amplitude = 0.01;
    const double duration = 0.2;
    if (time_seconds <= 0.0 || time_seconds >= duration)
    {
        return 0.0;
    }
    const double sine = std::sin(3.14159265358979323846 * time_seconds / duration);
    return amplitude * sine * sine;
}

static double GroundBumpVelocity(double time_seconds)
{
    const double amplitude = 0.01;
    const double duration = 0.2;
    if (time_seconds <= 0.0 || time_seconds >= duration)
    {
        return 0.0;
    }
    return amplitude * 3.14159265358979323846 / duration
        * std::sin(2.0 * 3.14159265358979323846 * time_seconds / duration);
}

static void RunFourWheelSimulation(const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& initial_state, double duration_seconds,
    double elapsed_seconds, bool road_bump, double sample_time_seconds,
    FourWheelRunResult& result)
{
    std::memset(&result, 0, sizeof(result));
    TireVerticalSolverState state = initial_state;
    double ground_height[4] = {};
    double ground_velocity[4] = {};
    result.initial_energy_joules = TireVerticalSystemEnergyJoules(configuration, state,
        ground_height);
    result.minimum_normal_force_after_settle_newtons = 1.0e30;
    const double settle_time = duration_seconds * 0.5;
    double road_work = 0.0;
    double damper_loss = 0.0;
    double tire_loss = 0.0;
    double constraint_loss = 0.0;

    const int step_count = static_cast<int>(std::lround(duration_seconds / elapsed_seconds));
    for (int step = 0; step < step_count; ++step)
    {
        const double time_seconds = (step + 1) * elapsed_seconds;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            ground_height[wheel] = road_bump ? GroundBumpHeight(time_seconds) : 0.0;
            ground_velocity[wheel] = road_bump ? GroundBumpVelocity(time_seconds) : 0.0;
        }
        TireVerticalSolverStepInput input = {};
        input.elapsed_seconds = elapsed_seconds;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            input.ground_height_meters[wheel] = ground_height[wheel];
            input.ground_height_velocity_meters_per_second[wheel] = ground_velocity[wheel];
        }

        TireVerticalSolverState next_state;
        TireVerticalSolverDiagnostics diagnostics;
        if (!TireVerticalSolverStep(configuration, state, input, next_state, diagnostics))
        {
            result.step_failures += 1;
            break;
        }
        result.maximum_residual_newtons = std::max(result.maximum_residual_newtons,
            diagnostics.residual_norm);
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            road_work += diagnostics.tire_normal_force_newtons[wheel]
                * ground_velocity[wheel] * elapsed_seconds;
            if (time_seconds >= settle_time)
            {
                result.minimum_normal_force_after_settle_newtons = std::min(
                    result.minimum_normal_force_after_settle_newtons,
                    diagnostics.tire_normal_force_newtons[wheel]);
                if (!diagnostics.tire_contact[wheel])
                {
                    result.contact_loss_steps_after_settle += 1;
                }
            }
        }
        damper_loss += diagnostics.damper_energy_loss_joules;
        tire_loss += diagnostics.tire_damping_energy_loss_joules;
        constraint_loss += diagnostics.constraint_energy_loss_joules;
        state = next_state;
        if (!result.sampled && time_seconds >= sample_time_seconds)
        {
            result.sample_state = state;
            result.sampled = 1;
        }
    }

    result.final_state = state;
    result.final_energy_joules = TireVerticalSystemEnergyJoules(configuration, state,
        ground_height);
    result.numerical_loss_joules = -(result.final_energy_joules - result.initial_energy_joules
        - road_work + damper_loss + tire_loss + constraint_loss);
}

static void CheckFourWheelSmallAngleGeometry(const TireVerticalSolverConfiguration& configuration)
{
    for (int sign = -1; sign <= 1; sign += 2)
    {
        const double roll = sign * 0.005;
        const double pitch = -sign * 0.005;
        const double body_height = 0.05;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            const double linear_height = body_height
                - configuration.wheel_x_meters[wheel] * pitch
                + configuration.wheel_y_meters[wheel] * roll;
            TireVerticalGeometryInput input = {};
            input.chassis.center_of_gravity_world_height_meters = body_height;
            input.chassis.center_of_gravity_local_height_meters = 0.0;
            input.chassis.wheel_x_meters = configuration.wheel_x_meters[wheel];
            input.chassis.wheel_y_meters = configuration.wheel_y_meters[wheel];
            input.chassis.pitch_radians = pitch;
            input.chassis.roll_radians = roll;
            input.ground_height_meters = 0.0;
            input.tire_unloaded_radius_meters =
                configuration.corner[wheel].unloaded_radius_meters;
            input.bellcrank_ratio = configuration.corner[wheel].spring.bellcrank_ratio;
            input.suspension_extension_meters = 0.05;
            TireVerticalGeometryOutput output = {};
            TireVerticalGeometryCompute(input, output);
            CheckClose(linear_height, output.chassis_reference_height_meters, 1.0e-6,
                "small angle geometry height approximation");
            CheckClose(0.05 - linear_height, output.tire_deflection_signed_meters, 1.0e-6,
                "small angle geometry deflection approximation");
            CheckClose(linear_height + configuration.corner[wheel].unloaded_radius_meters - 0.05,
                output.wheel_center_height_meters, 1.0e-6,
                "small angle geometry wheel center approximation");
        }
    }
}

static void CheckFourWheelPerturbations(const TireVerticalSolverConfiguration& configuration)
{
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics equilibrium_diagnostics;
    TireVerticalSolverInitializeEquilibrium(configuration, ground_height, equilibrium,
        equilibrium_diagnostics);

    struct Perturbation
    {
        const char* name;
        int axis;
        double amount;
    };
    const Perturbation perturbations[6] = {
        { "heave positive", 0, 0.01 },
        { "heave negative", 0, -0.01 },
        { "roll positive", 1, 0.005 },
        { "roll negative", 1, -0.005 },
        { "pitch positive", 2, 0.005 },
        { "pitch negative", 2, -0.005 }
    };

    for (int index = 0; index < 6; ++index)
    {
        TireVerticalSolverState state = equilibrium;
        state.body_position[perturbations[index].axis] += perturbations[index].amount;
        FourWheelRunResult result;
        RunFourWheelSimulation(configuration, state, 2.0, 0.002, false, 1.0, result);
        Check(result.step_failures == 0, perturbations[index].name);
        CheckClose(result.final_state.body_position[perturbations[index].axis],
            equilibrium.body_position[perturbations[index].axis], 5.0e-4,
            perturbations[index].name);
        Check(std::fabs(result.final_state.body_velocity[perturbations[index].axis]) < 0.01,
            perturbations[index].name);
        Check(result.minimum_normal_force_after_settle_newtons > 0.0,
            perturbations[index].name);
        Check(result.contact_loss_steps_after_settle == 0, perturbations[index].name);
        Check(result.numerical_loss_joules >= -1.0e-5, perturbations[index].name);
        Check(result.numerical_loss_joules < 0.1 * std::fabs(result.initial_energy_joules),
            perturbations[index].name);
        Check(result.maximum_residual_newtons < 1.0e-6, perturbations[index].name);
        std::printf("perturbation %s final_error=%.9g numerical_loss=%.9g "
            "maximum_residual=%.9g minimum_normal_force=%.9g\n", perturbations[index].name,
            result.final_state.body_position[perturbations[index].axis]
                - equilibrium.body_position[perturbations[index].axis],
            result.numerical_loss_joules, result.maximum_residual_newtons,
            result.minimum_normal_force_after_settle_newtons);
    }
}

static void CheckFourWheelChassisImpulse(const TireVerticalSolverConfiguration& configuration)
{
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics equilibrium_diagnostics;
    TireVerticalSolverInitializeEquilibrium(configuration, ground_height, equilibrium,
        equilibrium_diagnostics);
    TireVerticalSolverState state = equilibrium;
    state.body_velocity[0] = 0.1;
    FourWheelRunResult result;
    RunFourWheelSimulation(configuration, state, 2.0, 0.002, false, 1.0, result);
    Check(result.step_failures == 0, "four wheel chassis impulse steps converge");
    Check(std::fabs(result.final_state.body_velocity[0]) < 0.01,
        "four wheel chassis impulse settles body velocity");
    Check(result.minimum_normal_force_after_settle_newtons > 0.0,
        "four wheel chassis impulse keeps contact");
    Check(result.numerical_loss_joules >= -1.0e-5,
        "four wheel chassis impulse no energy creation");
    Check(result.maximum_residual_newtons < 1.0e-6,
        "four wheel chassis impulse residual bounded");
}

static void CheckFourWheelRateComparison(const TireVerticalSolverConfiguration& configuration)
{
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics equilibrium_diagnostics;
    TireVerticalSolverInitializeEquilibrium(configuration, ground_height, equilibrium,
        equilibrium_diagnostics);
    TireVerticalSolverState state = equilibrium;
    state.body_position[1] = 0.005;

    FourWheelRunResult coarse;
    FourWheelRunResult fine;
    RunFourWheelSimulation(configuration, state, 1.0, 0.002, false, 0.5, coarse);
    RunFourWheelSimulation(configuration, state, 1.0, 0.001, false, 0.5, fine);
    Check(coarse.step_failures == 0 && fine.step_failures == 0,
        "four wheel rate comparison steps converge");
    Check(coarse.maximum_residual_newtons < 1.0e-6 && fine.maximum_residual_newtons < 1.0e-6,
        "four wheel rate comparison residual bounded");
    CheckClose(coarse.sample_state.body_position[1], fine.sample_state.body_position[1],
        2.0e-3, "four wheel 500 and 1000 Hz roll sample");
    CheckClose(coarse.sample_state.body_position[0], fine.sample_state.body_position[0],
        2.0e-3, "four wheel 500 and 1000 Hz heave sample");
    double maximum_extension_difference = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        CheckClose(coarse.sample_state.suspension_extension_meters[wheel],
            fine.sample_state.suspension_extension_meters[wheel], 2.0e-3,
            "four wheel 500 and 1000 Hz extension sample");
        maximum_extension_difference = std::max(maximum_extension_difference,
            std::fabs(coarse.sample_state.suspension_extension_meters[wheel]
                - fine.sample_state.suspension_extension_meters[wheel]));
    }
    std::printf("rate comparison roll_difference=%.9g heave_difference=%.9g "
        "maximum_extension_difference=%.9g\n",
        coarse.sample_state.body_position[1] - fine.sample_state.body_position[1],
        coarse.sample_state.body_position[0] - fine.sample_state.body_position[0],
        maximum_extension_difference);
}

static void CheckFourWheelRoadBump(const TireVerticalSolverConfiguration& configuration)
{
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics equilibrium_diagnostics;
    TireVerticalSolverInitializeEquilibrium(configuration, ground_height, equilibrium,
        equilibrium_diagnostics);

    FourWheelRunResult coarse;
    FourWheelRunResult fine;
    RunFourWheelSimulation(configuration, equilibrium, 1.0, 0.002, true, 0.3, coarse);
    RunFourWheelSimulation(configuration, equilibrium, 1.0, 0.001, true, 0.3, fine);
    Check(coarse.step_failures == 0 && fine.step_failures == 0,
        "four wheel road bump steps converge");
    Check(coarse.maximum_residual_newtons < 1.0e-6 && fine.maximum_residual_newtons < 1.0e-6,
        "four wheel road bump residual bounded");
    CheckClose(coarse.sample_state.body_position[0], fine.sample_state.body_position[0],
        3.0e-3, "four wheel road bump 500 and 1000 Hz heave sample");
    Check(coarse.numerical_loss_joules >= -1.0e-5,
        "four wheel road bump no energy creation");
    Check(coarse.numerical_loss_joules < 0.1 * std::fabs(coarse.initial_energy_joules),
        "four wheel road bump numerical loss bounded");
    Check(fine.numerical_loss_joules >= -1.0e-5,
        "four wheel road bump fine no energy creation");
    std::printf("road bump heave_difference=%.9g coarse_numerical_loss=%.9g "
        "fine_numerical_loss=%.9g\n",
        coarse.sample_state.body_position[0] - fine.sample_state.body_position[0],
        coarse.numerical_loss_joules, fine.numerical_loss_joules);
}

static void CheckFourWheelBumpStops(const TireVerticalSolverConfiguration& configuration)
{
    TireVerticalSolverConfiguration limit_configuration = configuration;
    limit_configuration.axle[0].anti_roll_enabled = 0;
    limit_configuration.axle[1].anti_roll_enabled = 0;

    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics equilibrium_diagnostics;
    TireVerticalSolverInitializeEquilibrium(limit_configuration, ground_height, equilibrium,
        equilibrium_diagnostics);

    TireVerticalSolverState compressed = equilibrium;
    compressed.suspension_extension_meters[0] =
        limit_configuration.corner[0].spring.minimum_position_meters
        / limit_configuration.corner[0].spring.bellcrank_ratio;
    compressed.suspension_extension_velocity_meters_per_second[0] = -2.0;
    TireVerticalSolverStepInput input = {};
    input.elapsed_seconds = 0.002;
    TireVerticalSolverState next_state;
    TireVerticalSolverDiagnostics diagnostics;
    const int compressed_ok = TireVerticalSolverStep(limit_configuration, compressed, input,
        next_state, diagnostics);
    Check(compressed_ok, "four wheel lower limit step converges");
    Check(next_state.suspension_extension_meters[0]
        >= limit_configuration.corner[0].spring.minimum_position_meters
            / limit_configuration.corner[0].spring.bellcrank_ratio - 1.0e-9,
        "four wheel lower limit holds extension");
    Check(diagnostics.active_constraint_count > 0, "four wheel lower limit active");
    Check(diagnostics.constraint_impulse_newton_seconds[0] > 0.0,
        "four wheel lower limit impulse sign");
    Check(diagnostics.constraint_energy_loss_joules > 0.0,
        "four wheel lower limit dissipates energy");
    const double lower_impulse = diagnostics.constraint_impulse_newton_seconds[0];
    const double lower_loss = diagnostics.constraint_energy_loss_joules;

    TireVerticalSolverState extended = equilibrium;
    extended.body_position[0] += 0.3;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        extended.suspension_extension_meters[wheel] =
            limit_configuration.corner[wheel].spring.maximum_position_meters
            / limit_configuration.corner[wheel].spring.bellcrank_ratio - 0.0005;
        extended.suspension_extension_velocity_meters_per_second[wheel] = 2.0;
    }
    const int extended_ok = TireVerticalSolverStep(limit_configuration, extended, input,
        next_state, diagnostics);
    Check(extended_ok, "four wheel upper limit step converges");
    Check(next_state.suspension_extension_meters[0]
        <= limit_configuration.corner[0].spring.maximum_position_meters
            / limit_configuration.corner[0].spring.bellcrank_ratio + 1.0e-9,
        "four wheel upper limit holds extension");
    Check(diagnostics.active_constraint_count > 0, "four wheel upper limit active");
    Check(diagnostics.constraint_impulse_newton_seconds[0] < 0.0,
        "four wheel upper limit impulse sign");
    Check(diagnostics.constraint_energy_loss_joules > 0.0,
        "four wheel upper limit dissipates energy");
    std::printf("bump stops lower_impulse=%.9g lower_loss=%.9g upper_impulse=%.9g "
        "upper_loss=%.9g\n", lower_impulse, lower_loss,
        diagnostics.constraint_impulse_newton_seconds[0],
        diagnostics.constraint_energy_loss_joules);
}

static void CheckFourWheelHeave(const TireVerticalSolverConfiguration& configuration)
{
    TireVerticalSolverConfiguration heave_configuration = configuration;
    for (int axle = 0; axle < 2; ++axle)
    {
        TireVerticalAxleConfiguration& axle_configuration = heave_configuration.axle[axle];
        axle_configuration.heave_enabled = 1;
        axle_configuration.heave_spring.spring_constant_newtons_per_meter = -100000.0;
        axle_configuration.heave_spring.preload_newtons = 0.0;
        axle_configuration.heave_spring.reference_position_meters = 0.05;
        axle_configuration.heave_spring.minimum_position_meters = -1.0;
        axle_configuration.heave_spring.maximum_position_meters = 1.0;
        axle_configuration.heave_spring.bellcrank_ratio = 1.0;
        axle_configuration.heave_spring.inertance_kilograms = 0.0;
        axle_configuration.heave_damper.bump.slow_coefficient_newtons_per_second_per_meter = 2000.0;
        axle_configuration.heave_damper.bump.fast_coefficient_newtons_per_second_per_meter = 1000.0;
        axle_configuration.heave_damper.bump.transition_velocity_meters_per_second = 0.1;
        axle_configuration.heave_damper.bump.slow_intercept_newtons = 0.0;
        axle_configuration.heave_damper.bump.fast_intercept_newtons = 100.0;
        axle_configuration.heave_damper.rebound.slow_coefficient_newtons_per_second_per_meter = 2500.0;
        axle_configuration.heave_damper.rebound.fast_coefficient_newtons_per_second_per_meter = 1200.0;
        axle_configuration.heave_damper.rebound.transition_velocity_meters_per_second = 0.1;
        axle_configuration.heave_damper.rebound.slow_intercept_newtons = 0.0;
        axle_configuration.heave_damper.rebound.fast_intercept_newtons = 130.0;
        axle_configuration.heave_damper.efficiency = 1.0;
    }

    char error_message[256] = {};
    Check(TireVerticalSolverConfigurationValidate(heave_configuration, error_message,
            sizeof(error_message)), "four wheel heave configuration valid");
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics diagnostics;
    Check(TireVerticalSolverInitializeEquilibrium(heave_configuration, ground_height,
            equilibrium, diagnostics), "four wheel heave equilibrium converges");
    Check(diagnostics.sprung_mass_matrix_positive_definite,
        "four wheel heave sprung mass matrix positive definite");

    TireVerticalSolverState jacobian_state = equilibrium;
    jacobian_state.body_velocity[0] = 0.01;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        jacobian_state.suspension_extension_meters[wheel] = 0.04;
        jacobian_state.suspension_extension_velocity_meters_per_second[wheel] = 0.01;
    }
    TireVerticalSolverStepInput jacobian_input = {};
    const double jacobian_error = TireVerticalJacobianError(heave_configuration,
        jacobian_state, jacobian_input);
    Check(jacobian_error < 1.0e-3, "four wheel heave jacobian matches finite difference");
    std::printf("four wheel heave jacobian maximum error=%.9g\n", jacobian_error);

    for (int sign = -1; sign <= 1; sign += 2)
    {
        TireVerticalSolverState state = equilibrium;
        state.body_position[0] += sign * 0.005;
        FourWheelRunResult result;
        RunFourWheelSimulation(heave_configuration, state, 2.0, 0.002, false, 1.0, result);
        Check(result.step_failures == 0, "four wheel heave perturbation steps converge");
        CheckClose(result.final_state.body_position[0], equilibrium.body_position[0],
            5.0e-4, "four wheel heave perturbation settles heave");
        Check(result.minimum_normal_force_after_settle_newtons > 0.0,
            "four wheel heave perturbation keeps contact");
        Check(result.numerical_loss_joules >= -1.0e-5,
            "four wheel heave perturbation no energy creation");
        Check(result.maximum_residual_newtons < 1.0e-6,
            "four wheel heave perturbation residual bounded");
    }
}

static void CheckFourWheelContactLoss(const TireVerticalSolverConfiguration& configuration)
{
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics equilibrium_diagnostics;
    TireVerticalSolverInitializeEquilibrium(configuration, ground_height, equilibrium,
        equilibrium_diagnostics);
    TireVerticalSolverState state = equilibrium;
    state.body_position[0] += 0.05;
    state.body_velocity[0] = 0.1;
    const double initial_extension = state.suspension_extension_meters[0];

    int failures_in_flight = 0;
    int contact_during_flight = 0;
    for (int step = 0; step < 10; ++step)
    {
        TireVerticalSolverStepInput input = {};
        input.elapsed_seconds = 0.002;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            input.ground_height_meters[wheel] = 0.0;
            input.ground_height_velocity_meters_per_second[wheel] = 0.0;
        }
        TireVerticalSolverState next_state;
        TireVerticalSolverDiagnostics diagnostics;
        if (!TireVerticalSolverStep(configuration, state, input, next_state, diagnostics))
        {
            failures_in_flight += 1;
            break;
        }
        state = next_state;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            contact_during_flight += diagnostics.tire_contact[wheel];
        }
    }
    Check(failures_in_flight == 0, "four wheel contact loss steps converge");
    Check(contact_during_flight == 0, "four wheel contact loss stays airborne");
    Check(std::fabs(state.suspension_extension_meters[0] - initial_extension) > 1.0e-6,
        "four wheel contact loss keeps suspension dynamics");

    state.body_position[0] = equilibrium.body_position[0];
    state.body_velocity[0] = 0.0;
    FourWheelRunResult result;
    RunFourWheelSimulation(configuration, state, 1.0, 0.002, false, 1.0, result);
    Check(result.step_failures == 0, "four wheel contact recovery steps converge");
    Check(result.minimum_normal_force_after_settle_newtons > 0.0,
        "four wheel contact recovery restores contact");
    CheckClose(result.final_state.suspension_extension_meters[0],
        equilibrium.suspension_extension_meters[0], 1.0e-3,
        "four wheel contact recovery settles extension");
    Check(result.numerical_loss_joules >= -1.0e-5,
        "four wheel contact recovery no energy creation");
    Check(result.maximum_residual_newtons < 1.0e-6,
        "four wheel contact recovery residual bounded");
}

static void CheckFourWheelSuspensionScaling()
{
    TireVerticalSolverConfiguration configuration = {};
    configuration.total_mass_kilograms = 600.0;
    configuration.roll_inertia_kilogram_meters_squared = 100.0;
    configuration.pitch_inertia_kilogram_meters_squared = 600.0;
    configuration.gravity_meters_per_second_squared = 9.81;
    const double bellcranks[4] = { 0.8, 0.75, 0.37, 0.37 };
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        configuration.wheel_x_meters[wheel] = wheel < 2 ? 1.6 : -1.28;
        configuration.wheel_y_meters[wheel] = wheel % 2 == 0 ? -0.8 : 0.8;
        configuration.unsprung_mass_kilograms[wheel] = 14.0;
        configuration.corner[wheel].spring.bellcrank_ratio = bellcranks[wheel];
        configuration.corner[wheel].spring.minimum_position_meters = 0.01;
        configuration.corner[wheel].spring.maximum_position_meters = 0.2;
        configuration.corner[wheel].tire_radial_stiffness_newtons_per_meter = 150000.0;
        configuration.corner[wheel].unloaded_radius_meters = 0.31;
    }

    configuration.axle[0].anti_roll_enabled = 1;
    configuration.axle[0].anti_roll_stiffness_newtons_per_meter = 100000.0;
    configuration.axle[1].anti_roll_enabled = 1;
    configuration.axle[1].anti_roll_stiffness_newtons_per_meter = 80000.0;

    const double extensions[4] = { 0.05, 0.045, 0.05, 0.045 };
    const double velocities[4] = { 0.0, 0.0, 0.0, 0.0 };
    double forces[4] = {};
    TireVerticalSuspensionForces(configuration, extensions, velocities, forces);
    const double front_delta = bellcranks[1] * extensions[1] - bellcranks[0] * extensions[0];
    const double rear_delta = bellcranks[3] * extensions[3] - bellcranks[2] * extensions[2];
    CheckClose(forces[0], 100000.0 * front_delta * bellcranks[0], 1.0e-9,
        "four wheel ARB front right");
    CheckClose(forces[1], -100000.0 * front_delta * bellcranks[1], 1.0e-9,
        "four wheel ARB front left");
    CheckClose(forces[2], 80000.0 * rear_delta * bellcranks[2], 1.0e-9,
        "four wheel ARB rear right");
    CheckClose(forces[3], -80000.0 * rear_delta * bellcranks[3], 1.0e-9,
        "four wheel ARB rear left");

    TireVerticalSolverConfiguration without_anti_roll = configuration;
    without_anti_roll.axle[0].anti_roll_enabled = 0;
    without_anti_roll.axle[1].anti_roll_enabled = 0;
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    const double energy_step = 1.0e-7;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        double plus_extensions[4] = { extensions[0], extensions[1], extensions[2], extensions[3] };
        double minus_extensions[4] = { extensions[0], extensions[1], extensions[2], extensions[3] };
        plus_extensions[wheel] += energy_step;
        minus_extensions[wheel] -= energy_step;
        TireVerticalSolverState plus_state = {};
        TireVerticalSolverState minus_state = {};
        plus_state.suspension_extension_meters[0] = plus_extensions[0];
        plus_state.suspension_extension_meters[1] = plus_extensions[1];
        plus_state.suspension_extension_meters[2] = plus_extensions[2];
        plus_state.suspension_extension_meters[3] = plus_extensions[3];
        minus_state.suspension_extension_meters[0] = minus_extensions[0];
        minus_state.suspension_extension_meters[1] = minus_extensions[1];
        minus_state.suspension_extension_meters[2] = minus_extensions[2];
        minus_state.suspension_extension_meters[3] = minus_extensions[3];
        const double enabled_difference =
            TireVerticalSystemEnergyJoules(configuration, plus_state, ground_height)
            - TireVerticalSystemEnergyJoules(configuration, minus_state, ground_height);
        const double disabled_difference =
            TireVerticalSystemEnergyJoules(without_anti_roll, plus_state, ground_height)
            - TireVerticalSystemEnergyJoules(without_anti_roll, minus_state, ground_height);
        const double numerical_force = -(enabled_difference - disabled_difference)
            / (2.0 * energy_step);
        CheckClose(numerical_force, forces[wheel], 1.0e-5,
            "four wheel ARB potential derivative");
    }

    TireVerticalSolverConfiguration heave_only = without_anti_roll;
    for (int axle = 0; axle < 2; ++axle)
    {
        TireVerticalAxleConfiguration& axle_configuration = heave_only.axle[axle];
        axle_configuration.heave_enabled = 1;
        axle_configuration.heave_spring.spring_constant_newtons_per_meter = -50000.0;
        axle_configuration.heave_spring.preload_newtons = 0.0;
        axle_configuration.heave_spring.reference_position_meters = 0.1;
        axle_configuration.heave_spring.minimum_position_meters = -1.0;
        axle_configuration.heave_spring.maximum_position_meters = 1.0;
        axle_configuration.heave_spring.bellcrank_ratio = 0.9;
    }
    TireVerticalSuspensionForces(heave_only, extensions, velocities, forces);
    for (int axle = 0; axle < 2; ++axle)
    {
        const int right_wheel = 2 * axle;
        const int left_wheel = 2 * axle + 1;
        const double heave_position = 0.5
            * (bellcranks[left_wheel] * extensions[left_wheel]
                + bellcranks[right_wheel] * extensions[right_wheel]);
        const double heave_spring_force = std::max(0.0, -50000.0 * (heave_position - 0.1));
        const double heave_force_right = 0.5 * 0.9 * bellcranks[right_wheel]
            * heave_spring_force;
        const double heave_force_left = 0.5 * 0.9 * bellcranks[left_wheel]
            * heave_spring_force;
        CheckClose(forces[right_wheel], heave_force_right, 1.0e-9,
            "four wheel heave right scaling");
        CheckClose(forces[left_wheel], heave_force_left, 1.0e-9,
            "four wheel heave left scaling");

        for (int wheel = right_wheel; wheel <= left_wheel; ++wheel)
        {
            double plus_extensions[4] = { extensions[0], extensions[1], extensions[2], extensions[3] };
            double minus_extensions[4] = { extensions[0], extensions[1], extensions[2], extensions[3] };
            plus_extensions[wheel] += energy_step;
            minus_extensions[wheel] -= energy_step;
            TireVerticalSolverState plus_state = {};
            TireVerticalSolverState minus_state = {};
            for (int index = 0; index < 4; ++index)
            {
                plus_state.suspension_extension_meters[index] = plus_extensions[index];
                minus_state.suspension_extension_meters[index] = minus_extensions[index];
            }
            const double enabled_difference =
                TireVerticalSystemEnergyJoules(heave_only, plus_state, ground_height)
                - TireVerticalSystemEnergyJoules(heave_only, minus_state, ground_height);
            const double disabled_difference =
                TireVerticalSystemEnergyJoules(without_anti_roll, plus_state, ground_height)
                - TireVerticalSystemEnergyJoules(without_anti_roll, minus_state, ground_height);
            const double numerical_force = -(enabled_difference - disabled_difference)
                / (2.0 * energy_step);
            CheckClose(numerical_force, forces[wheel], 1.0e-5,
                "four wheel heave potential derivative");
        }
    }
    std::printf("suspension scaling front_delta=%.9g rear_delta=%.9g "
        "heave_forces=%.9g,%.9g,%.9g,%.9g\n",
        front_delta, rear_delta, forces[0], forces[1], forces[2], forces[3]);
}

static void CheckFourWheelZeroTime(const TireVerticalSolverConfiguration& configuration)
{
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState state = {};
    TireVerticalSolverDiagnostics diagnostics;
    TireVerticalSolverInitializeEquilibrium(configuration, ground_height, state, diagnostics);
    const TireVerticalSolverState before = state;
    TireVerticalSolverStepInput input = {};
    input.elapsed_seconds = 0.0;
    TireVerticalSolverState next_state;
    const int result = TireVerticalSolverStep(configuration, state, input, next_state,
        diagnostics);
    Check(result == 0, "four wheel zero time rejected");
    Check(next_state.body_position[0] == before.body_position[0]
        && next_state.suspension_extension_meters[0] == before.suspension_extension_meters[0],
        "four wheel zero time keeps state");
}

static void CheckFourWheelEquilibrium(const TireVerticalSolverConfiguration& configuration)
{
    const double ground_height[4] = { 0.0, 0.0, 0.0, 0.0 };
    TireVerticalSolverState state = {};
    TireVerticalSolverDiagnostics diagnostics;
    Check(TireVerticalSolverInitializeEquilibrium(configuration, ground_height, state, diagnostics),
        "four wheel equilibrium converges");
    Check(diagnostics.mass_matrix_positive_definite,
        "four wheel equilibrium mass matrix positive definite");
    Check(diagnostics.sprung_mass_matrix_positive_definite,
        "four wheel equilibrium sprung mass matrix positive definite");
    Check(diagnostics.converged, "four wheel equilibrium diagnostics converged");
    Check(std::fabs(state.body_position[1]) < 1.0e-3,
        "four wheel equilibrium roll small");
    Check(std::fabs(state.body_position[2]) < 1.0e-2,
        "four wheel equilibrium pitch small");

    double normal_force_sum = 0.0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        normal_force_sum += diagnostics.tire_normal_force_newtons[wheel];
        Check(diagnostics.tire_contact[wheel], "four wheel equilibrium contact");
        CheckClose(diagnostics.tire_normal_force_newtons[wheel],
            diagnostics.suspension_force_newtons[wheel]
                + configuration.unsprung_mass_kilograms[wheel]
                    * configuration.gravity_meters_per_second_squared,
            1.0e-3, "four wheel equilibrium wheel balance");
        const double lower = configuration.corner[wheel].spring.minimum_position_meters
            / configuration.corner[wheel].spring.bellcrank_ratio;
        const double upper = configuration.corner[wheel].spring.maximum_position_meters
            / configuration.corner[wheel].spring.bellcrank_ratio;
        Check(state.suspension_extension_meters[wheel] >= lower - 1.0e-6
            && state.suspension_extension_meters[wheel] <= upper + 1.0e-6,
            "four wheel equilibrium extension within limits");
    }
    CheckClose(normal_force_sum, configuration.total_mass_kilograms
        * configuration.gravity_meters_per_second_squared, 1.0e-3,
        "four wheel equilibrium total normal force");

    const double base_energy = TireVerticalSystemEnergyJoules(configuration, state,
        ground_height);
    TireVerticalSolverState moving = state;
    moving.body_velocity[0] = 0.1;
    moving.body_velocity[1] = 0.01;
    moving.body_velocity[2] = -0.02;
    const double moving_energy = TireVerticalSystemEnergyJoules(configuration, moving,
        ground_height);
    const double expected_kinetic = 0.5 * configuration.total_mass_kilograms * 0.1 * 0.1
        + 0.5 * configuration.roll_inertia_kilogram_meters_squared * 0.01 * 0.01
        + 0.5 * configuration.pitch_inertia_kilogram_meters_squared * 0.02 * 0.02;
    CheckClose(moving_energy - base_energy, expected_kinetic, 1.0e-9,
        "four wheel frozen extension recovers rigid mass matrix");

    TireVerticalSolverState jacobian_state = state;
    jacobian_state.body_velocity[0] = 0.01;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        jacobian_state.suspension_extension_velocity_meters_per_second[wheel] = 0.01;
    }
    TireVerticalSolverStepInput jacobian_input = {};
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        jacobian_input.ground_height_meters[wheel] = 0.0;
        jacobian_input.ground_height_velocity_meters_per_second[wheel] = 0.0;
    }
    const double jacobian_error = TireVerticalJacobianError(configuration, jacobian_state,
        jacobian_input);
    Check(jacobian_error < 1.0e-3, "four wheel force jacobian matches finite difference");
    std::printf("four wheel jacobian maximum error=%.9g\n", jacobian_error);

    std::printf("Williams equilibrium: body=%.9f roll=%.9f pitch=%.9f "
        "L=%.9f,%.9f,%.9f,%.9f N=%.6f,%.6f,%.6f,%.6f S=%.6f,%.6f,%.6f,%.6f\n",
        state.body_position[0], state.body_position[1], state.body_position[2],
        state.suspension_extension_meters[0], state.suspension_extension_meters[1],
        state.suspension_extension_meters[2], state.suspension_extension_meters[3],
        diagnostics.tire_normal_force_newtons[0], diagnostics.tire_normal_force_newtons[1],
        diagnostics.tire_normal_force_newtons[2], diagnostics.tire_normal_force_newtons[3],
        diagnostics.suspension_force_newtons[0], diagnostics.suspension_force_newtons[1],
        diagnostics.suspension_force_newtons[2], diagnostics.suspension_force_newtons[3]);
}

int main(int argument_count, char** arguments)
{
    const char* default_path =
        "implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml";
    const char* path = argument_count > 1 ? arguments[1] : default_path;

    GfInit();
    GfLogSetLevelThreshold(GfLogger::eWarning);

    CheckNativeSuspensionLaw();
    CheckQuarterCarEquilibrium();
    CheckQuarterCarChassisImpulse();
    CheckQuarterCarLimits();
    CheckQuarterCarZeroTime();
    CheckQuarterCarContactLoss();
    CheckQuarterCarRateComparison();

    TireVerticalSolverConfiguration williams_configuration;
    if (LoadWilliamsSolverConfiguration(path, williams_configuration))
    {
        CheckFourWheelEquilibrium(williams_configuration);
        CheckFourWheelSmallAngleGeometry(williams_configuration);
        CheckFourWheelPerturbations(williams_configuration);
        CheckFourWheelChassisImpulse(williams_configuration);
        CheckFourWheelRateComparison(williams_configuration);
        CheckFourWheelRoadBump(williams_configuration);
        CheckFourWheelBumpStops(williams_configuration);
        CheckFourWheelHeave(williams_configuration);
        CheckFourWheelContactLoss(williams_configuration);
        CheckFourWheelSuspensionScaling();
        CheckFourWheelZeroTime(williams_configuration);
    }

    std::printf("checks=%d failures=%d\n", checks, failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
