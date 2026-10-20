/***************************************************************************

    file                 : tire_wear_model.h
    copyright            : (C) 2026 by Open Formula contributors

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TIRE_WEAR_MODEL_H
#define TIRE_WEAR_MODEL_H

#include "tire_model_configuration.h"
#include "tire_model_state.h"

struct TireWearInput
{
    double friction_power_watts;
    double zone_contact_weights[3];
    double tread_inner_kelvin;
    double tread_center_kelvin;
    double tread_outer_kelvin;
    double surface_friction_coefficient;
    double elapsed_seconds;
};

void TireWearModelReset(TireWearState& state, const TireWearConfiguration& configuration);
void TireWearModelUpdate(TireWearState& state, const TireWearConfiguration& configuration,
    const TireWearInput& input);
double TireWearTemperatureMultiplier(double temperature_celsius,
    const TireWearConfiguration& configuration);
double TireWearGripScale(double wear_fraction, const TireWearConfiguration& configuration);

#endif
