#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <tgf.h>
#include "formula_gear_controls.h"

int main()
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
    car._carHandle = GfParmReadFile("formula-pedal-validation.xml", GFPARM_RMODE_CREAT);
    GfParmSetStr(car._carHandle, "Formula Controls", "simultaneous gear controls", "yes");
    GfParmSetStr(car._carHandle, "Formula Controls", "simultaneous pedal controls", "yes");
    GfParmSetStr(car._carHandle, "Formula Controls", "simultaneous steering controls", "yes");
    GfParmSetStr(car._carHandle, "Formula Controls", "controller name", descriptor.name);
    int failures = 0;
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
    verify_steering("full left", -32768, false, false, 0.02, 1.0);
    verify_steering("full right", 32767, false, false, 0.02, -1.0);
    verify_steering("return to center", 0, false, false, 0.02, -0.863);
    verify_steering("fully centered", 0, false, false, 1.0, 0.0);
    verify_steering("stick drift deadzone", 1000, false, false, 0.02, 0.0);
    verify_steering("partial steering", 16384, false, false, 0.02, -0.233927637843754);
    verify_steering("keyboard left", 0, true, false, 0.02, 1.0);
    verify_steering("keyboard right", 0, false, true, 0.02, -1.0);
    SDL_JoystickClose(joystick);
    SDL_JoystickDetachVirtual(controller_index);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
