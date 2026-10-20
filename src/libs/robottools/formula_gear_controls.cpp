#include "formula_gear_controls.h"
#include "formula_steering_filter.h"

#include <SDL.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <map>
#include <tgf.h>

namespace
{
struct FormulaGearControlState
{
    bool controller_shift_up = false;
    bool controller_shift_down = false;
    double previous_time = -1000.0;
    double previous_throttle = 0.0;
    double previous_brake = 0.0;
    double previous_steering = 0.0;
    double steering_diagnostic_elapsed = 0.0;
};

std::map<int, FormulaGearControlState> formula_gear_control_states;
SDL_GameController* formula_game_controller = nullptr;

bool formulaGearControlsEnabled(tCarElt* car)
{
    return std::strcmp(GfParmGetStr(car->_carHandle, "Formula Controls", "simultaneous gear controls", "no"), "yes") == 0;
}

void discoverFormulaGameController(tCarElt* car)
{
    if (formula_game_controller && !SDL_GameControllerGetAttached(formula_game_controller))
    {
        SDL_GameControllerClose(formula_game_controller);
        formula_game_controller = nullptr;
    }
    if (formula_game_controller)
        return;
    if (!(SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER))
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    for (int controller_index = 0; controller_index < SDL_NumJoysticks(); controller_index++)
    {
        if (!SDL_IsGameController(controller_index))
            continue;
        const char* preferred_controller_name = GfParmGetStr(car->_carHandle, "Formula Controls", "controller name", "");
        if (*preferred_controller_name && std::strcmp(SDL_GameControllerNameForIndex(controller_index), preferred_controller_name) != 0)
            continue;
        formula_game_controller = SDL_GameControllerOpen(controller_index);
        if (formula_game_controller)
        {
            GfLogInfo("Formula controls: left stick steering, A shifts up, X shifts down, RT throttle, LT brake; controller=%s\n", SDL_GameControllerName(formula_game_controller));
            break;
        }
    }
}
}

void resetFormulaGearControls(tCarElt* car)
{
    formula_gear_control_states.erase(car->index);
    if (!formulaGearControlsEnabled(car))
        return;
    discoverFormulaGameController(car);
    GfLogInfo("Formula gear controls: keyboard A shifts up, Z shifts down; sequential gears R, N through 6\n");
}

bool applyFormulaGearControls(tCarElt* car, bool keyboard_shift_up, bool keyboard_shift_down, double current_time)
{
    if (!formulaGearControlsEnabled(car))
        return false;
    discoverFormulaGameController(car);
    if (formula_game_controller)
        SDL_GameControllerUpdate();
    const bool controller_shift_up = formula_game_controller && SDL_GameControllerGetButton(formula_game_controller, SDL_CONTROLLER_BUTTON_A);
    const bool controller_shift_down = formula_game_controller && SDL_GameControllerGetButton(formula_game_controller, SDL_CONTROLLER_BUTTON_X);
    FormulaGearControlState& state = formula_gear_control_states[car->index];
    if (current_time < state.previous_time)
        state = FormulaGearControlState();
    const bool shift_up = keyboard_shift_up || (controller_shift_up && !state.controller_shift_up);
    const bool shift_down = keyboard_shift_down || (controller_shift_down && !state.controller_shift_down);
    if (shift_up != shift_down)
    {
        car->_gearCmd = std::max(-car->_gearOffset, std::min(car->_gearNb - 1, car->_gearCmd + (shift_up ? 1 : -1)));
        GfLogInfo("Formula gear command: %d (%s)\n", car->_gearCmd, keyboard_shift_up || keyboard_shift_down ? "keyboard" : "controller");
    }
    state.controller_shift_up = controller_shift_up;
    state.controller_shift_down = controller_shift_down;
    state.previous_time = current_time;
    return true;
}

void applyFormulaPedalControls(tCarElt* car, bool keyboard_throttle, bool keyboard_brake, double delta_time)
{
    if (std::strcmp(GfParmGetStr(car->_carHandle, "Formula Controls", "simultaneous pedal controls", "no"), "yes") != 0)
        return;
    discoverFormulaGameController(car);
    if (formula_game_controller)
        SDL_GameControllerUpdate();
    const double throttle_trigger = formula_game_controller ? std::max(0, static_cast<int>(SDL_GameControllerGetAxis(formula_game_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT))) / 32767.0 : 0.0;
    const double brake_trigger = formula_game_controller ? std::max(0, static_cast<int>(SDL_GameControllerGetAxis(formula_game_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT))) / 32767.0 : 0.0;
    const double throttle_target = keyboard_throttle ? 1.0 : std::max(0.0, (throttle_trigger - 0.05) / 0.95);
    const double brake_target = keyboard_brake ? 1.0 : std::max(0.0, (brake_trigger - 0.05) / 0.95);
    FormulaGearControlState& state = formula_gear_control_states[car->index];
    state.previous_throttle = throttle_target > 0.0 ? throttle_target : std::max(0.0, state.previous_throttle - std::max(0.0, delta_time) * 3.0);
    state.previous_brake = brake_target > 0.0 ? brake_target : std::max(0.0, state.previous_brake - std::max(0.0, delta_time) * 6.0);
    car->_accelCmd = static_cast<float>(state.previous_throttle);
    car->_brakeCmd = static_cast<float>(state.previous_brake);
}

void applyFormulaSteeringControls(tCarElt* car, bool keyboard_left, bool keyboard_right, double delta_time)
{
    if (std::strcmp(GfParmGetStr(car->_carHandle, "Formula Controls", "simultaneous steering controls", "no"), "yes") != 0)
        return;
    discoverFormulaGameController(car);
    if (formula_game_controller)
        SDL_GameControllerUpdate();
    const int axis_value = formula_game_controller ? SDL_GameControllerGetAxis(formula_game_controller, SDL_CONTROLLER_AXIS_LEFTX) : 0;
    const double raw_axis = axis_value / (axis_value < 0 ? 32768.0 : 32767.0);
    const double controller_strength = std::max(0.0, (std::abs(raw_axis) - 0.05) / 0.95);
    const double left_strength = keyboard_left ? 1.0 : (raw_axis < 0.0 ? controller_strength : 0.0);
    const double right_strength = keyboard_right ? 1.0 : (raw_axis > 0.0 ? controller_strength : 0.0);
    const double steering_axis = left_strength - right_strength;
    const double deadzoned_strength = std::max(0.0, (std::abs(steering_axis) - 0.05) / 0.95);
    double steering_target = std::copysign(std::pow(deadzoned_strength, 1.8), steering_axis);
    FormulaGearControlState& state = formula_gear_control_states[car->index];
    const double application_rate = std::max(0.01, static_cast<double>(GfParmGetNum(car->_carHandle, "Formula Controls", "steering application rate", nullptr, 6.85)));
    const double reversal_rate = std::max(0.01, static_cast<double>(GfParmGetNum(car->_carHandle, "Formula Controls", "steering reversal rate", nullptr, 6.85)));
    const double centering_rate = std::max(0.01, static_cast<double>(GfParmGetNum(car->_carHandle, "Formula Controls", "steering centering rate", nullptr, 6.85)));
    const double speed_sensitivity = std::max(0.0, static_cast<double>(GfParmGetNum(car->_carHandle, "Formula Controls", "steering speed sensitivity", nullptr, 0.0)));
    const double response_seconds = std::max(0.0, static_cast<double>(GfParmGetNum(car->_carHandle, "Formula Controls", "steering response time", nullptr, 0.0)));
    if (!keyboard_left && !keyboard_right)
        steering_target /= 1.0 + speed_sensitivity * std::abs(car->_speed_x);
    if (keyboard_left || keyboard_right)
        state.previous_steering = steering_target;
    else
        state.previous_steering = advanceFormulaSteering(state.previous_steering, steering_target, delta_time,
                                                       application_rate, reversal_rate, centering_rate, response_seconds);
    state.steering_diagnostic_elapsed += std::max(0.0, delta_time);
    if (GfParmGetNum(car->_carHandle, "Formula Controls", "diagnostic telemetry", nullptr, 0.0) > 0.0 &&
        state.steering_diagnostic_elapsed >= 0.05)
    {
        state.steering_diagnostic_elapsed = 0.0;
        GfLogInfo("Formula steering: car=%d raw=%.6f target=%.6f filtered=%.6f speed=%.6f elapsed=%.6f attached=%d\n",
                  car->index, raw_axis, steering_target, state.previous_steering, car->_speed_x, delta_time,
                  formula_game_controller != nullptr);
    }
    car->_steerCmd = static_cast<float>(state.previous_steering);
}
