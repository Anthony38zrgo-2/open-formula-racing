#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <tgf.h>
#include "formula_gear_controls.h"
#include "formula_steering_filter.h"

int main(int argument_count, char** arguments)
{
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT, "0x1209/0x1997");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
        return EXIT_FAILURE;
    SDL_VirtualJoystickDesc descriptor = {};
    descriptor.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    descriptor.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    descriptor.naxes = SDL_CONTROLLER_AXIS_MAX;
    descriptor.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    descriptor.vendor_id = 0x1209;
    descriptor.product_id = 0x1997;
    descriptor.name = "Formula pedal regression controller";
    descriptor.axis_mask = (1 << SDL_CONTROLLER_AXIS_MAX) - 1;
    descriptor.button_mask = (1 << SDL_CONTROLLER_BUTTON_MAX) - 1;
    const int controller_index = SDL_JoystickAttachVirtualEx(&descriptor);
    SDL_Joystick* joystick = SDL_JoystickOpen(controller_index);
    if (!joystick)
        return EXIT_FAILURE;
    GfInit();
    tCarElt car = {};
    car._gearNb = 7;
    car._gearOffset = 1;
    car._carHandle = GfParmReadFile("formula-pedal-validation.xml", GFPARM_RMODE_CREAT);
    GfParmSetStr(car._carHandle, "Formula Controls", "simultaneous gear controls", "yes");
    GfParmSetStr(car._carHandle, "Formula Controls", "simultaneous pedal controls", "yes");
    GfParmSetStr(car._carHandle, "Formula Controls", "simultaneous steering controls", "yes");
    GfParmSetStr(car._carHandle, "Formula Controls", "controller name", descriptor.name);
    int failures = 0;
    auto verify_gear = [&](const char* label, bool keyboard_up, bool keyboard_down, bool controller_up, bool controller_down, int expected_gear)
    {
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, controller_up);
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_X, controller_down);
        SDL_JoystickUpdate();
        applyFormulaGearControls(&car, keyboard_up, keyboard_down, 1.0);
        const bool passed = car._gearCmd == expected_gear;
        std::cout << label << ": gear=" << car._gearCmd << (passed ? " PASS" : " FAIL") << '\n';
        failures += !passed;
    };
    verify_gear("keyboard neutral to reverse", false, true, false, false, -1);
    verify_gear("reverse lower bound", false, true, false, false, -1);
    verify_gear("keyboard reverse to neutral", true, false, false, false, 0);
    verify_gear("keyboard neutral to first", true, false, false, false, 1);
    verify_gear("controller first to neutral", false, false, false, true, 0);
    verify_gear("held button does not shift twice", false, false, false, true, 0);
    verify_gear("release controller button", false, false, false, false, 0);
    verify_gear("controller neutral to reverse", false, false, false, true, -1);
    verify_gear("release in reverse", false, false, false, false, -1);
    verify_gear("controller reverse to neutral", false, false, true, false, 0);
    car._gearCmd = 6;
    verify_gear("highest forward gear bound", true, false, false, false, 6);
    car._gearCmd = 0;
    car._gearOffset = 0;
    verify_gear("vehicle without reverse stays neutral", false, true, false, false, 0);
    car._gearOffset = 1;
    auto verify = [&](const char* label, Sint16 throttle, Sint16 brake, bool keyboard_throttle, bool keyboard_brake,
                      double delta_time, double expected_throttle, double expected_brake)
    {
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, throttle);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, brake);
        SDL_JoystickUpdate();
        applyFormulaPedalControls(&car, keyboard_throttle, keyboard_brake, delta_time);
        const bool passed = std::abs(car._accelCmd - expected_throttle) < 0.001 && std::abs(car._brakeCmd - expected_brake) < 0.001;
        std::cout << label << ": throttle=" << car._accelCmd << " brake=" << car._brakeCmd << (passed ? " PASS" : " FAIL") << '\n';
        failures += !passed;
    };
    verify("released triggers", -32768, -32768, false, false, 0.02, 0.0, 0.0);
    verify("right trigger only", 32767, -32768, false, false, 0.02, 1.0, 0.0);
    verify("both triggers independent", 32767, 32767, false, false, 0.02, 1.0, 1.0);
    verify("partial throttle and full brake", 0, 32767, false, false, 0.02, (0.5 - 0.05) / 0.95, 1.0);
    verify("release smoothing", -32768, -32768, false, false, 0.02, (0.5 - 0.05) / 0.95 - 0.06, 0.88);
    verify("keyboard fallback", -32768, -32768, true, true, 0.02, 1.0, 1.0);
    verify("full release", -32768, -32768, false, false, 1.0, 0.0, 0.0);
    auto verify_steering = [&](const char* label, Sint16 axis, bool left, bool right, double delta_time, double expected)
    {
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, axis);
        SDL_JoystickUpdate();
        applyFormulaSteeringControls(&car, left, right, delta_time);
        const bool passed = std::abs(car._steerCmd - expected) < 0.001;
        std::cout << label << ": steering=" << car._steerCmd << (passed ? " PASS" : " FAIL") << '\n';
        failures += !passed;
    };
    verify_steering("centered stick", 0, false, false, 0.02, 0.0);
    verify_steering("full left begins continuously", -32768, false, false, 0.02, 0.137);
    verify_steering("full right reverses continuously", 32767, false, false, 0.02, 0.0);
    verify_steering("return to center", 0, false, false, 0.02, 0.0);
    verify_steering("fully centered", 0, false, false, 1.0, 0.0);
    verify_steering("stick drift deadzone", 1000, false, false, 0.02, 0.0);
    verify_steering("partial steering begins continuously", 16384, false, false, 0.02, -0.137);
    verify_steering("partial steering reaches target", 16384, false, false, 0.02, -0.233927637843754);
    verify_steering("keyboard left", 0, true, false, 0.02, 1.0);
    verify_steering("keyboard right", 0, false, true, 0.02, -1.0);
    for (double elapsed_seconds : {0.001, 0.002, 0.01, 0.02, 0.05})
    {
        double filtered_command = 1.0;
        const int step_count = static_cast<int>(std::round(0.4 / elapsed_seconds));
        for (int step_index = 0; step_index < step_count; ++step_index)
            filtered_command = advanceFormulaSteering(filtered_command, -1.0, elapsed_seconds, 3.0, 4.0, 5.0);
        const bool passed = std::abs(filtered_command + 0.45) < 0.000001;
        std::cout << "reversal timestep=" << elapsed_seconds << " command=" << filtered_command << (passed ? " PASS" : " FAIL") << '\n';
        failures += !passed;
    }
    for (double target_command : {-1.0, -0.03, 0.0, 0.03, 1.0})
    {
        double reference_command = 0.6;
        for (int step_index = 0; step_index < 2000; ++step_index)
            reference_command = advanceFormulaSteering(reference_command, target_command, 0.0005, 1.5, 2.0, 3.0, 0.16);
        for (double elapsed_seconds : {0.001, 0.002, 0.01, 0.02, 0.05})
        {
            double filtered_command = 0.6;
            for (int step_index = 0; step_index < static_cast<int>(std::round(1.0 / elapsed_seconds)); ++step_index)
                filtered_command = advanceFormulaSteering(filtered_command, target_command, elapsed_seconds, 1.5, 2.0, 3.0, 0.16);
            const bool passed = std::abs(filtered_command - reference_command) < 0.000001;
            std::cout << "smoothed response target=" << target_command << " timestep=" << elapsed_seconds << (passed ? " PASS" : " FAIL") << '\n';
            failures += !passed;
        }
    }
    if (argument_count == 2)
    {
        tCarElt profile_car = {};
        profile_car.index = 1;
        profile_car._carHandle = GfParmReadFile(arguments[1], GFPARM_RMODE_STD);
        if (!profile_car._carHandle)
            return EXIT_FAILURE;
        GfParmSetStr(profile_car._carHandle, "Formula Controls", "controller name", descriptor.name);
        failures += GfParmGetNum(profile_car._carHandle, "Gearbox/gears/r", "ratio", nullptr, 0.0) >= 0.0;
        for (double speed_kilometers_per_hour : {0.0, 100.0, 200.0, 300.0})
        {
            resetFormulaGearControls(&profile_car);
            profile_car._speed_x = speed_kilometers_per_hour / 3.6;
            SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, -32768);
            SDL_JoystickUpdate();
            applyFormulaSteeringControls(&profile_car, false, false, 0.02);
            failures += profile_car._steerCmd <= 0.0 || profile_car._steerCmd > 0.03001;
            for (int step_index = 0; step_index < 150; ++step_index)
                applyFormulaSteeringControls(&profile_car, false, false, 0.02);
            const double expected_command = 1.0 / (1.0 + 0.018 * speed_kilometers_per_hour / 3.6);
            const bool passed = std::abs(profile_car._steerCmd - expected_command) < 0.001;
            std::cout << "installed profile speed=" << speed_kilometers_per_hour << " command=" << profile_car._steerCmd
                      << " expected=" << expected_command << (passed ? " PASS" : " FAIL") << '\n';
            failures += !passed;
            applyFormulaSteeringControls(&profile_car, true, false, 0.02);
            failures += std::abs(profile_car._steerCmd - 1.0) > 0.001;
        }
    }
    SDL_JoystickClose(joystick);
    SDL_JoystickDetachVirtual(controller_index);
    applyFormulaSteeringControls(&car, false, false, 1.0);
    failures += std::abs(car._steerCmd) > 0.001;
    applyFormulaSteeringControls(&car, true, false, 0.02);
    failures += std::abs(car._steerCmd - 1.0) > 0.001;
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
