/***************************************************************************

    file                 : tire_contact_model.h
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

#ifndef TIRE_CONTACT_MODEL_H
#define TIRE_CONTACT_MODEL_H

#include "tire_model_configuration.h"
#include "tire_model_state.h"
#include "tire_thermal_model.h"

struct TireContactInput
{
    double normal_force_newtons;
    double base_contact_patch_meters;
    double load_sensitivity_exponent;
    double contact_fraction;
    int grounded;
};

struct TireContactOutput
{
    double empirical_deformation_meters;
    double effective_radius_meters;
    double contact_patch_meters;
    double load_sensitivity_scale;
    double empirical_deformation_limit_meters;
    double contact_fraction;
    long long clamp_count;
};

void TireContactModelReset(TireContactState& state, const TireContactConfiguration& configuration,
    const TireMechanicalModifiers& modifiers);
void TireContactModelEvaluate(TireContactState& state, const TireContactConfiguration& configuration,
    const TireMechanicalModifiers& modifiers, const TireContactInput& input, TireContactOutput& output);

#endif
