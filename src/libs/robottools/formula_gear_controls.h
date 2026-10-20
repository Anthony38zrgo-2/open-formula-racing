#ifndef FORMULA_GEAR_CONTROLS_H
#define FORMULA_GEAR_CONTROLS_H

#include <car.h>

bool applyFormulaGearControls(tCarElt* car, bool keyboard_shift_up, bool keyboard_shift_down, double current_time);
void resetFormulaGearControls(tCarElt* car);
void applyFormulaPedalControls(tCarElt* car, bool keyboard_throttle, bool keyboard_brake, double delta_time);
void applyFormulaSteeringControls(tCarElt* car, bool keyboard_left, bool keyboard_right, double delta_time);

#endif
