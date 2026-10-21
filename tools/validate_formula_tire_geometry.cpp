#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <tgf.h>

#include "tire_vertical_geometry.h"

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

struct WheelGeometry
{
    double x_meters;
    double y_meters;
    double radius_meters;
    double bellcrank_ratio;
    double ride_height_meters;
    double unsprung_mass_kilograms;
};

static TireVerticalGeometryOutput Evaluate(const TireVerticalGeometryInput& input)
{
    TireVerticalGeometryOutput output = {};
    TireVerticalGeometryCompute(input, output);
    return output;
}

static TireVerticalGeometryInput MakeFlatInput(const WheelGeometry& wheel,
    double chassis_reference_height_meters, double ground_height_meters,
    double suspension_extension_meters, double suspension_extension_velocity_meters_per_second)
{
    TireVerticalGeometryInput input = {};
    input.chassis.center_of_gravity_local_height_meters = 0.245;
    input.chassis.center_of_gravity_world_height_meters =
        chassis_reference_height_meters + input.chassis.center_of_gravity_local_height_meters;
    input.chassis.wheel_x_meters = wheel.x_meters;
    input.chassis.wheel_y_meters = wheel.y_meters;
    input.ground_height_meters = ground_height_meters;
    input.tire_unloaded_radius_meters = wheel.radius_meters;
    input.bellcrank_ratio = wheel.bellcrank_ratio;
    input.suspension_extension_meters = suspension_extension_meters;
    input.suspension_extension_velocity_meters_per_second =
        suspension_extension_velocity_meters_per_second;
    return input;
}

static bool LoadWilliamsGeometry(const char* path, WheelGeometry wheels[4])
{
    void* parameters = GfParmReadFile(path, GFPARM_RMODE_STD);
    if (parameters == NULL)
    {
        std::printf("FAIL cannot read %s\n", path);
        failures += 1;
        return false;
    }

    const double front_repartition = GfParmGetNum(parameters, "Car",
        "front-rear weight repartition", NULL, 0.0);
    const double front_left_repartition = GfParmGetNum(parameters, "Car",
        "front right-left weight repartition", NULL, 0.5);
    const double rear_left_repartition = GfParmGetNum(parameters, "Car",
        "rear right-left weight repartition", NULL, 0.5);
    const double body_width = GfParmGetNum(parameters, "Car", "body width", NULL, 0.0);
    const double left_repartition = front_repartition * front_left_repartition
        + (1.0 - front_repartition) * rear_left_repartition;
    const double center_of_gravity_y = -left_repartition * body_width + body_width * 0.5;
    const double front_axle_x = GfParmGetNum(parameters, "Front Axle", "xpos", NULL, 0.0);
    const double rear_axle_x = GfParmGetNum(parameters, "Rear Axle", "xpos", NULL, 0.0);
    const double center_of_gravity_x = front_axle_x * front_repartition
        + rear_axle_x * (1.0 - front_repartition);

    std::printf("Williams loaded values: front_repartition=%.12g body_width=%.12g "
        "center_of_gravity_x=%.12g center_of_gravity_y=%.12g\n",
        front_repartition, body_width, center_of_gravity_x, center_of_gravity_y);

    const char* wheel_sections[4] = {
        "Front Right Wheel", "Front Left Wheel", "Rear Right Wheel", "Rear Left Wheel"
    };
    const char* suspension_sections[4] = {
        "Front Right Suspension", "Front Left Suspension", "Rear Right Suspension", "Rear Left Suspension"
    };

    for (int index = 0; index < 4; ++index)
    {
        const bool front = index < 2;
        const double axle_x = front ? front_axle_x : rear_axle_x;
        const double wheel_y = GfParmGetNum(parameters, wheel_sections[index], "ypos", NULL, 0.0);
        const double rim_diameter = GfParmGetNum(parameters, wheel_sections[index],
            "rim diameter", NULL, 0.0);
        const double tire_width = GfParmGetNum(parameters, wheel_sections[index],
            "tire width", NULL, 0.0);
        const double tire_height = GfParmGetNum(parameters, wheel_sections[index],
            "tire height", NULL, -1.0);
        const double tire_height_ratio = GfParmGetNum(parameters, wheel_sections[index],
            "tire height-width ratio", NULL, 0.0);

        wheels[index].x_meters = axle_x - center_of_gravity_x;
        wheels[index].y_meters = wheel_y - center_of_gravity_y;
        wheels[index].radius_meters = tire_height > 0.0
            ? rim_diameter * 0.5 + tire_height
            : rim_diameter * 0.5 + tire_width * tire_height_ratio;
        wheels[index].bellcrank_ratio = GfParmGetNum(parameters, suspension_sections[index],
            "bellcrank", NULL, 1.0);
        wheels[index].ride_height_meters = GfParmGetNum(parameters, wheel_sections[index],
            "ride height", NULL, 0.0);
        wheels[index].unsprung_mass_kilograms = GfParmGetNum(parameters, wheel_sections[index],
            "mass", NULL, 0.0);

        std::printf("wheel=%d x=%.12g y=%.12g radius=%.12g bellcrank=%.12g ride_height=%.12g mass=%.12g\n",
            index, wheels[index].x_meters, wheels[index].y_meters, wheels[index].radius_meters,
            wheels[index].bellcrank_ratio, wheels[index].ride_height_meters,
            wheels[index].unsprung_mass_kilograms);
    }

    GfParmReleaseHandle(parameters);
    return true;
}

static void CheckIdentityExample()
{
    TireVerticalGeometryInput input = {};
    input.chassis.center_of_gravity_world_height_meters = 0.050;
    input.tire_unloaded_radius_meters = 0.312;
    input.bellcrank_ratio = 1.0;
    input.suspension_extension_meters = 0.0586;

    const TireVerticalGeometryOutput output = Evaluate(input);
    CheckClose(output.chassis_reference_height_meters, 0.050, 1.0e-15,
        "identity chassis reference height");
    CheckClose(output.tire_deflection_signed_meters, 0.0086, 1.0e-15,
        "identity signed deflection");
    CheckClose(output.wheel_center_height_meters - input.ground_height_meters, 0.3034, 1.0e-15,
        "identity wheel center above ground");
    CheckClose(output.tire_deflection_signed_meters,
        input.suspension_extension_meters - output.chassis_reference_ground_clearance_meters,
        1.0e-15, "identity deflection from extension and clearance");
    CheckClose(output.suspension_extension_meters, input.suspension_extension_meters,
        1.0e-15, "identity keeps suspension extension");
    CheckClose(output.suspension_extension_velocity_meters_per_second,
        input.suspension_extension_velocity_meters_per_second, 1.0e-15,
        "identity keeps suspension extension velocity");
    Check(output.geometric_contact, "identity geometric contact");
}

static void CheckLoadedWilliamsGeometry(const WheelGeometry wheels[4])
{
    CheckClose(wheels[0].x_meters, 1.60395, 1.0e-7, "Williams front right recentered x");
    CheckClose(wheels[1].x_meters, 1.60395, 1.0e-7, "Williams front left recentered x");
    CheckClose(wheels[2].x_meters, -1.28605, 1.0e-7, "Williams rear right recentered x");
    CheckClose(wheels[3].x_meters, -1.28605, 1.0e-7, "Williams rear left recentered x");
    CheckClose(wheels[0].y_meters, -0.834987, 1.0e-7, "Williams front right recentered y");
    CheckClose(wheels[1].y_meters, 0.834987, 1.0e-7, "Williams front left recentered y");
    CheckClose(wheels[2].y_meters, -0.8, 1.0e-7, "Williams rear right recentered y");
    CheckClose(wheels[3].y_meters, 0.8, 1.0e-7, "Williams rear left recentered y");
    Check(wheels[0].radius_meters > 0.30 && wheels[0].radius_meters < 0.33,
        "Williams front radius in expected range");
    Check(wheels[2].radius_meters > 0.30 && wheels[2].radius_meters < 0.33,
        "Williams rear radius in expected range");

    for (int index = 0; index < 4; ++index)
    {
        const double deflection = index < 2 ? 0.0086 : 0.0090;
        const double clearance = wheels[index].ride_height_meters - deflection;
        TireVerticalGeometryInput input = MakeFlatInput(wheels[index], clearance, 0.0,
            wheels[index].ride_height_meters, 0.0);
        const TireVerticalGeometryOutput output = Evaluate(input);

        CheckClose(output.tire_deflection_signed_meters, deflection, 1.0e-15,
            "Williams loaded signed deflection");
        CheckClose(output.tire_deflection_meters, deflection, 1.0e-15,
            "Williams loaded published deflection");
        CheckClose(output.wheel_center_height_meters, wheels[index].radius_meters - deflection,
            1.0e-15, "Williams loaded wheel center above ground");
        CheckClose(output.suspension_position_legacy_meters,
            wheels[index].bellcrank_ratio * wheels[index].ride_height_meters, 1.0e-15,
            "Williams loaded suspension position");
        Check(output.geometric_contact, "Williams loaded geometric contact");
    }
}

static void CheckTranslationInvariance(const WheelGeometry wheels[4])
{
    const double base_clearance = 0.0414;
    const double base_extension = 0.05;
    const double tire_stiffness = 200000.0;
    const double tire_damping = 1200.0;

    for (int index = 0; index < 4; ++index)
    {
        TireVerticalGeometryInput base = MakeFlatInput(wheels[index], base_clearance, 0.0,
            base_extension, 0.0);
        const TireVerticalGeometryOutput base_output = Evaluate(base);
        const double base_load = tire_stiffness * base_output.tire_deflection_meters
            + tire_damping * base_output.tire_deflection_velocity_meters_per_second;

        TireVerticalGeometryInput translated = base;
        translated.chassis.center_of_gravity_world_height_meters += 0.123;
        translated.ground_height_meters += 0.123;
        const TireVerticalGeometryOutput translated_output = Evaluate(translated);
        const double translated_load = tire_stiffness * translated_output.tire_deflection_meters
            + tire_damping * translated_output.tire_deflection_velocity_meters_per_second;

        CheckClose(translated_output.tire_deflection_signed_meters,
            base_output.tire_deflection_signed_meters, 1.0e-15,
            "translation keeps signed deflection");
        CheckClose(translated_load, base_load, 1.0e-12, "translation keeps tire load");
        CheckClose(translated_output.wheel_center_height_meters,
            base_output.wheel_center_height_meters + 0.123, 1.0e-15,
            "translation moves wheel center");

        TireVerticalGeometryInput translated_velocity = base;
        translated_velocity.chassis.center_of_gravity_world_height_velocity_meters_per_second += 0.5;
        translated_velocity.ground_height_velocity_meters_per_second += 0.5;
        const TireVerticalGeometryOutput velocity_output = Evaluate(translated_velocity);
        CheckClose(velocity_output.tire_deflection_velocity_meters_per_second,
            base_output.tire_deflection_velocity_meters_per_second, 1.0e-15,
            "translation keeps deflection velocity");
        CheckClose(velocity_output.wheel_center_velocity_meters_per_second,
            base_output.wheel_center_velocity_meters_per_second + 0.5, 1.0e-15,
            "translation moves wheel center velocity");
    }
}

static void CheckSeparateMotions(const WheelGeometry& wheel)
{
    TireVerticalGeometryInput input = MakeFlatInput(wheel, 0.0414, 0.0, 0.05, 0.0);
    TireVerticalGeometryOutput output = Evaluate(input);
    CheckClose(output.tire_deflection_signed_meters, 0.0086, 1.0e-15,
        "separate motions initial deflection");

    TireVerticalGeometryInput ground_up = input;
    ground_up.ground_height_meters += 0.02;
    output = Evaluate(ground_up);
    CheckClose(output.tire_deflection_signed_meters, 0.0286, 1.0e-15, "road rise increases deflection");
    CheckClose(output.wheel_center_height_meters, wheel.radius_meters - 0.0086, 1.0e-15,
        "road rise keeps wheel center");
    CheckClose(output.suspension_velocity_legacy_meters_per_second, 0.0, 1.0e-15,
        "road rise keeps suspension velocity");

    TireVerticalGeometryInput chassis_down = input;
    chassis_down.chassis.center_of_gravity_world_height_meters -= 0.02;
    output = Evaluate(chassis_down);
    CheckClose(output.tire_deflection_signed_meters, 0.0286, 1.0e-15,
        "chassis down increases deflection");
    CheckClose(output.wheel_center_height_meters, wheel.radius_meters - 0.0086 - 0.02, 1.0e-15,
        "chassis down moves wheel center");
    CheckClose(output.suspension_velocity_legacy_meters_per_second, 0.0, 1.0e-15,
        "chassis down keeps suspension velocity");

    TireVerticalGeometryInput extension = input;
    extension.suspension_extension_meters += 0.01;
    output = Evaluate(extension);
    CheckClose(output.tire_deflection_signed_meters, 0.0186, 1.0e-15,
        "suspension extension increases deflection");
    CheckClose(output.wheel_center_height_meters, wheel.radius_meters - 0.0086 - 0.01, 1.0e-15,
        "suspension extension lowers wheel center");
    CheckClose(output.suspension_position_legacy_meters, wheel.bellcrank_ratio * 0.06, 1.0e-15,
        "suspension extension moves suspension position");

    TireVerticalGeometryInput compression = input;
    compression.suspension_extension_meters -= 0.02;
    output = Evaluate(compression);
    CheckClose(output.tire_deflection_signed_meters, -0.0114, 1.0e-15,
        "suspension compression can lose contact");
    CheckClose(output.tire_deflection_meters, 0.0, 1.0e-15,
        "suspension compression publishes zero deflection");
    Check(!output.geometric_contact, "suspension compression reports no geometric contact");
}

static void CheckVelocitySigns(const WheelGeometry& wheel)
{
    TireVerticalGeometryInput input = MakeFlatInput(wheel, 0.0414, 0.0, 0.05, 0.0);
    input.chassis.center_of_gravity_world_height_velocity_meters_per_second = 0.1;
    input.ground_height_velocity_meters_per_second = -0.05;
    input.suspension_extension_velocity_meters_per_second = 0.2;

    TireVerticalGeometryOutput output = Evaluate(input);
    CheckClose(output.wheel_center_velocity_meters_per_second, -0.1, 1.0e-15,
        "positive extension velocity lowers wheel center");
    CheckClose(output.tire_deflection_velocity_meters_per_second, 0.05, 1.0e-15,
        "deflection velocity combines road chassis and extension");
    CheckClose(output.suspension_velocity_legacy_meters_per_second,
        -wheel.bellcrank_ratio * 0.2, 1.0e-15,
        "positive extension velocity is negative compression");

    input.suspension_extension_velocity_meters_per_second = -0.2;
    output = Evaluate(input);
    CheckClose(output.wheel_center_velocity_meters_per_second, 0.3, 1.0e-15,
        "negative extension velocity raises wheel center");
    CheckClose(output.tire_deflection_velocity_meters_per_second, -0.35, 1.0e-15,
        "compression velocity signs");
    CheckClose(output.suspension_velocity_legacy_meters_per_second,
        wheel.bellcrank_ratio * 0.2, 1.0e-15,
        "compression velocity is positive suspension velocity");
}

static void CheckBellcrankScaling(const WheelGeometry& wheel)
{
    const double ratios[3] = { 1.0, 0.8759227555360516, 0.37049714771142384 };
    for (int index = 0; index < 3; ++index)
    {
        TireVerticalGeometryInput input = MakeFlatInput(wheel, 0.0414, 0.0, 0.0586, -0.3);
        input.bellcrank_ratio = ratios[index];
        input.suspension_extension_acceleration_meters_per_second_squared = 2.0;
        const TireVerticalGeometryOutput output = Evaluate(input);
        CheckClose(output.suspension_position_legacy_meters, ratios[index] * 0.0586, 1.0e-15,
            "bellcrank scales suspension position");
        CheckClose(output.suspension_velocity_legacy_meters_per_second, ratios[index] * 0.3, 1.0e-15,
            "bellcrank scales suspension compression velocity");
        CheckClose(output.suspension_acceleration_legacy_meters_per_second_squared,
            ratios[index] * 2.0, 1.0e-15,
            "bellcrank scales legacy suspension acceleration");
        CheckClose(output.suspension_compression_acceleration_meters_per_second_squared,
            -ratios[index] * 2.0, 1.0e-15,
            "bellcrank scales physical compression acceleration");
    }
}

static void CheckChassisTransformDerivative(const WheelGeometry& wheel)
{
    for (int sign = -1; sign <= 1; sign += 2)
    {
        TireVerticalGeometryInput input = MakeFlatInput(wheel, 0.0414, 0.0, 0.05, 0.0);
        input.chassis.pitch_radians = sign * 0.017;
        input.chassis.roll_radians = -sign * 0.023;
        input.chassis.center_of_gravity_world_height_velocity_meters_per_second = 0.4;
        input.chassis.pitch_velocity_radians_per_second = -sign * 0.7;
        input.chassis.roll_velocity_radians_per_second = sign * 0.9;

        const TireVerticalGeometryOutput output = Evaluate(input);
        const double expected_height = input.chassis.center_of_gravity_world_height_meters
            - input.chassis.center_of_gravity_local_height_meters
            - wheel.x_meters * std::sin(input.chassis.pitch_radians)
            + wheel.y_meters * std::sin(input.chassis.roll_radians);
        const double expected_velocity = input.chassis.center_of_gravity_world_height_velocity_meters_per_second
            - wheel.x_meters * std::cos(input.chassis.pitch_radians)
                * input.chassis.pitch_velocity_radians_per_second
            + wheel.y_meters * std::cos(input.chassis.roll_radians)
                * input.chassis.roll_velocity_radians_per_second;

        CheckClose(output.chassis_reference_height_meters, expected_height, 1.0e-15,
            "chassis reference height matches direct transform");
        CheckClose(output.chassis_reference_height_velocity_meters_per_second,
            expected_velocity, 1.0e-15, "chassis reference velocity matches direct derivative");

        const double elapsed_seconds = 1.0e-7;
        TireVerticalGeometryInput advanced = input;
        advanced.chassis.center_of_gravity_world_height_meters +=
            input.chassis.center_of_gravity_world_height_velocity_meters_per_second * elapsed_seconds;
        advanced.chassis.pitch_radians +=
            input.chassis.pitch_velocity_radians_per_second * elapsed_seconds;
        advanced.chassis.roll_radians +=
            input.chassis.roll_velocity_radians_per_second * elapsed_seconds;
        const TireVerticalGeometryOutput advanced_output = Evaluate(advanced);
        const double finite_difference =
            (advanced_output.chassis_reference_height_meters - output.chassis_reference_height_meters)
            / elapsed_seconds;
        CheckClose(finite_difference, output.chassis_reference_height_velocity_meters_per_second,
            1.0e-6, "chassis reference velocity matches finite difference");
    }
}

static void CheckFourWheelHeaveRollPitch(const WheelGeometry wheels[4])
{
    const double base_clearance = 0.0414;
    TireVerticalGeometryOutput base_outputs[4];
    for (int index = 0; index < 4; ++index)
    {
        base_outputs[index] = Evaluate(MakeFlatInput(wheels[index], base_clearance, 0.0, 0.05, 0.0));
        CheckClose(base_outputs[index].tire_deflection_signed_meters, 0.0086, 1.0e-15,
            "four wheel flat base deflection");
    }

    for (int sign = -1; sign <= 1; sign += 2)
    {
        const double angle = sign * 0.01;
        for (int index = 0; index < 4; ++index)
        {
            TireVerticalGeometryInput input = MakeFlatInput(wheels[index], base_clearance, 0.0, 0.05, 0.0);
            input.chassis.roll_radians = angle;
            const TireVerticalGeometryOutput output = Evaluate(input);
            const double expected = base_outputs[index].tire_deflection_signed_meters
                - wheels[index].y_meters * std::sin(angle);
            CheckClose(output.tire_deflection_signed_meters, expected, 1.0e-15,
                "roll changes deflection by wheel y");
        }

        for (int index = 0; index < 4; ++index)
        {
            TireVerticalGeometryInput input = MakeFlatInput(wheels[index], base_clearance, 0.0, 0.05, 0.0);
            input.chassis.pitch_radians = angle;
            const TireVerticalGeometryOutput output = Evaluate(input);
            const double expected = base_outputs[index].tire_deflection_signed_meters
                + wheels[index].x_meters * std::sin(angle);
            CheckClose(output.tire_deflection_signed_meters, expected, 1.0e-15,
                "pitch changes deflection by wheel x");
        }
    }

    for (int sign = -1; sign <= 1; sign += 2)
    {
        for (int index = 0; index < 4; ++index)
        {
            TireVerticalGeometryInput input = MakeFlatInput(wheels[index],
                base_clearance + sign * 0.01, 0.0, 0.05, 0.0);
            const TireVerticalGeometryOutput output = Evaluate(input);
            CheckClose(output.tire_deflection_signed_meters,
                base_outputs[index].tire_deflection_signed_meters - sign * 0.01, 1.0e-15,
                "heave changes every wheel deflection equally");
        }
    }
}

static void CheckAirborneState(const WheelGeometry& wheel)
{
    TireVerticalGeometryInput input = MakeFlatInput(wheel, 0.10, 0.0, 0.05, 0.5);
    TireVerticalGeometryOutput output = Evaluate(input);
    CheckClose(output.tire_deflection_signed_meters, -0.05, 1.0e-15,
        "airborne signed deflection is negative");
    CheckClose(output.tire_deflection_meters, 0.0, 1.0e-15,
        "airborne published deflection is zero");
    Check(!output.geometric_contact, "airborne has no geometric contact");
    CheckClose(output.wheel_center_velocity_meters_per_second, -0.5, 1.0e-15,
        "airborne wheel center keeps extension velocity");
    CheckClose(output.suspension_extension_meters, 0.05, 1.0e-15,
        "airborne keeps suspension extension");
    CheckClose(output.suspension_extension_velocity_meters_per_second, 0.5, 1.0e-15,
        "airborne keeps suspension extension velocity");

    input.chassis.center_of_gravity_world_height_velocity_meters_per_second = -2.0;
    output = Evaluate(input);
    CheckClose(output.tire_deflection_velocity_meters_per_second, 2.5, 1.0e-15,
        "airborne chassis down increases deflection velocity");
    CheckClose(output.wheel_center_velocity_meters_per_second, -2.5, 1.0e-15,
        "airborne chassis down moves wheel center");
    CheckClose(output.suspension_extension_meters, 0.05, 1.0e-15,
        "airborne chassis motion keeps suspension extension");
    CheckClose(output.suspension_extension_velocity_meters_per_second, 0.5, 1.0e-15,
        "airborne chassis motion keeps suspension extension velocity");

    input.chassis.center_of_gravity_world_height_velocity_meters_per_second = 2.0;
    output = Evaluate(input);
    CheckClose(output.tire_deflection_velocity_meters_per_second, -1.5, 1.0e-15,
        "airborne chassis up decreases deflection velocity");
    CheckClose(output.wheel_center_velocity_meters_per_second, 1.5, 1.0e-15,
        "airborne chassis up moves wheel center the other way");

    input.chassis.center_of_gravity_world_height_meters =
        0.05 + input.chassis.center_of_gravity_local_height_meters;
    input.chassis.center_of_gravity_world_height_velocity_meters_per_second = 0.0;
    input.suspension_extension_meters = 0.15;
    input.suspension_extension_velocity_meters_per_second = 0.0;
    output = Evaluate(input);
    Check(output.geometric_contact, "wheel center below contact height restores geometric contact");
    CheckClose(output.tire_deflection_signed_meters, 0.10, 1.0e-15,
        "restored contact deflection");
}

static void CheckContactBoundary(const WheelGeometry& wheel)
{
    TireVerticalGeometryInput input = MakeFlatInput(wheel, 0.05, 0.0, 0.05, 0.0);
    TireVerticalGeometryOutput output = Evaluate(input);
    Check(output.geometric_contact, "zero clearance is geometric contact");
    CheckClose(output.tire_deflection_signed_meters, 0.0, 1.0e-15, "zero clearance deflection");

    input.chassis.center_of_gravity_world_height_meters += 1.0e-7;
    output = Evaluate(input);
    Check(!output.geometric_contact, "positive clearance is no contact");
    CheckClose(output.tire_deflection_meters, 0.0, 1.0e-15, "positive clearance published zero");

    input.chassis.center_of_gravity_world_height_meters -= 2.0e-7;
    output = Evaluate(input);
    Check(output.geometric_contact, "negative clearance is geometric contact");
    CheckClose(output.tire_deflection_signed_meters, 1.0e-7, 1.0e-15,
        "negative clearance signed deflection");
}

static void CheckRadiusVelocity(const WheelGeometry& wheel)
{
    TireVerticalGeometryInput input = MakeFlatInput(wheel, 0.0414, 0.0, 0.05, 0.0);
    input.chassis.center_of_gravity_world_height_velocity_meters_per_second = 0.1;
    input.ground_height_velocity_meters_per_second = 0.05;
    input.suspension_extension_velocity_meters_per_second = 0.3;
    input.tire_unloaded_radius_velocity_meters_per_second = 0.002;
    const TireVerticalGeometryOutput output = Evaluate(input);
    CheckClose(output.wheel_center_velocity_meters_per_second, -0.198, 1.0e-15,
        "radius velocity enters wheel center velocity");
    CheckClose(output.tire_deflection_velocity_meters_per_second, 0.25, 1.0e-15,
        "radius velocity cancels from deflection velocity");
}

int main(int argument_count, char** arguments)
{
    const char* default_path =
        "implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml";
    const char* path = argument_count > 1 ? arguments[1] : default_path;

    GfInit();
    GfLogSetLevelThreshold(GfLogger::eWarning);

    WheelGeometry wheels[4] = {};
    if (!LoadWilliamsGeometry(path, wheels))
    {
        return EXIT_FAILURE;
    }

    CheckIdentityExample();
    CheckLoadedWilliamsGeometry(wheels);
    CheckTranslationInvariance(wheels);
    CheckSeparateMotions(wheels[0]);
    CheckVelocitySigns(wheels[0]);
    CheckBellcrankScaling(wheels[0]);
    CheckChassisTransformDerivative(wheels[0]);
    CheckFourWheelHeaveRollPitch(wheels);
    CheckAirborneState(wheels[0]);
    CheckContactBoundary(wheels[0]);
    CheckRadiusVelocity(wheels[0]);

    std::printf("checks=%d failures=%d\n", checks, failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
