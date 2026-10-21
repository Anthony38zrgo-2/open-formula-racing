/***************************************************************************

    file                 : car.cpp
    created              : Sun Mar 19 00:05:43 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <limits>

#include "sim.h"
#include "tire_thermal_model.h"
#include "track_ground_mesh.h"

static double SimCarVerticalClamp(double value, double lower, double upper);
static void SimCarComputeCoupledContactVelocityForState(tCar *car, int wheel_index,
    const TireVerticalSolverState& state, const double ground_height_meters[4],
    double& velocity_x, double& velocity_y);

static const char *SuspSect[4] = {SECT_FRNTRGTSUSP, SECT_FRNTLFTSUSP, SECT_REARRGTSUSP, SECT_REARLFTSUSP};

// The "Simu" logger instance
GfLogger* PLogSimu = 0;

static bool
enable_tyreset(const tCar *car, const tCarSetup *setup)
{
    return car->options->tyre_temperature;
}

static bool
enable_compound_set(const tCar *car, const tCarSetup *setup)
{
    return (car->features & FEAT_COMPOUNDS) && car->options->compounds;
}

static bool
enable_front_wing_angle(const tCar *car, const tCarSetup *setup)
{
    return car->wing[0].WingType != -1
        && setup->wingAngle[0].min != setup->wingAngle[0].max;
}

static bool
enable_rear_wing_angle(const tCar *car, const tCarSetup *setup)
{
    return car->wing[1].WingType != -1
        && setup->wingAngle[1].min != setup->wingAngle[1].max;
}

void
SimCarConfig(tCar *car)
{
    void	*hdle = car->params;
    tdble	k;
    t3Dd	Irot;
    tdble	w;
    tdble	gcfrl, gcrrl, gcfr, gcrl;
    tdble	wf0, wr0;
    tdble	overallwidth;
    int		i;
    tCarElt	*carElt = car->carElt;

    car->options = new SimulationOptions;
    car->options->SetFromSkill (carElt->_skillLevel);
    //car->options->LoadFromFile (hdle);

    car->fuel_time = 0.0;
    car->fuel_consumption = 0.0;
    car->carElt->_fuelTotal = 0.0;
    car->carElt->_fuelInstant = 10.0;

    const char *enabling;
    tCarSetupItem *setupGcfr = &(car->carElt->setup.FRWeightRep);
    tCarSetupItem *setupGcfrl = &(car->carElt->setup.FRLWeightRep);
    tCarSetupItem *setupGcrrl = &(car->carElt->setup.RRLWeightRep);
    tCarSetupItem *setupFuel = &(car->carElt->setup.fuel);

    /* get features first */
    car->features = 0;
    car->TCL_BrakeScale = 125.0f;;
    car->TCL_AccelScale = 0.9f;
    car->TCL_SlipScale = 1.0f;
    car->ABS_SlipScale = 0.1f;
    car->ABS_BrakeScale = 1.0f;

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_AEROTOCG, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_AEROTOCG;
        GfLogInfo("SimuV5 feature AEROTOCG on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_SLOWGRIP, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_SLOWGRIP;
        GfLogInfo("SimuV5 feature SLOW GRIP on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_REALGEARCHANGE, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_REALGEARCHANGE;
        GfLogInfo("SimuV5 feature REAL GEAR CHANGE on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_REVLIMIT, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_REVLIMIT;
        GfLogInfo("SimuV5 feature REV LIMIT on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_TIRETEMPDEG, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_TIRETEMPDEG;
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_TIRECOMPOUNDS, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_COMPOUNDS;
        GfLogInfo("SimuV5 feature COMPOUNDS on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_FIXEDWHEELFORCE, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_FIXEDWHEELFORCE;
        GfLogInfo("SimuV5 feature FIXED WHEEL FORCE on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_TCLINSIMU, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_TCLINSIMU;
        GfLogInfo("SimuV5 feature TCL SIMU on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_ABSINSIMU, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_ABSINSIMU;
        GfLogInfo("SimuV5 feature ABS SIMU on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_ESPINSIMU, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_ESPINSIMU;
        GfLogInfo("SimuV5 feature ESP IMU on\n");
    }

    enabling = GfParmGetStr(hdle, SECT_FEATURES, PRM_LIMITEDGROUNDEFFECT, VAL_NO);
    if (strcmp(enabling, VAL_YES) == 0)
    {
        car->features = car->features | FEAT_LIMITEDGROUNDEFFECT;
        GfLogInfo("SimuV5 feature LIMITED GROUND EFFECT on\n");
    }

    /* continue with car parameters */
    setupGcfr->desired_value = setupGcfr->min = setupGcfr->max = 0.5;
    GfParmGetNumWithLimits(hdle, SECT_CAR, PRM_FRWEIGHTREP, (char*)NULL, &(setupGcfr->desired_value), &(setupGcfr->min), &(setupGcfr->max));
    setupGcfr->changed = true;
    setupGcfr->stepsize = 0.005f;
    gcfr = setupGcfr->desired_value;

    setupGcfrl->desired_value = setupGcfrl->min = setupGcfrl->max = 0.5;
    GfParmGetNumWithLimits(hdle, SECT_CAR, PRM_FRLWEIGHTREP, (char*)NULL, &(setupGcfrl->desired_value), &(setupGcfrl->min), &(setupGcfrl->max));
    setupGcfrl->changed = true;
    setupGcfrl->stepsize = 0.005f;
    gcfrl = setupGcfrl->desired_value;

    setupGcrrl->desired_value = setupGcrrl->min = setupGcrrl->max = 0.5;
    GfParmGetNumWithLimits(hdle, SECT_CAR, PRM_RRLWEIGHTREP, (char*)NULL, &(setupGcrrl->desired_value), &(setupGcrrl->min), &(setupGcrrl->max));
    setupGcrrl->changed = true;
    setupGcrrl->stepsize = 0.005f;
    gcrrl = setupGcrrl->desired_value;

    gcrl = gcfr * gcfrl + (1 - gcfr) * gcrrl;

    car->tank        = GfParmGetNum(hdle, SECT_CAR, PRM_TANK, (char*)NULL, 80);
    if (car->fuel > car->tank)
    {
        car->fuel = car->tank;
    }
    car->fuel_prev = car->fuel;

    car->fuelMass        = GfParmGetNum(hdle, SECT_CAR, PRM_FUELMASS, (char*)NULL, 1.0);

    setupFuel->desired_value = setupFuel->min = setupFuel->max = 80.0;
    GfParmGetNumWithLimits(hdle, SECT_CAR, PRM_FUEL, (char*)NULL, &(setupFuel->desired_value), &(setupFuel->min), &(setupFuel->max));
    setupFuel->min = 0.0;
    setupFuel->max = car->tank;
    setupFuel->changed = true;
    setupFuel->stepsize = 1.0;

    car->dimension.x = GfParmGetNum(hdle, SECT_CAR, PRM_LEN, (char*)NULL, 4.7f);
    car->dimension.y = GfParmGetNum(hdle, SECT_CAR, PRM_WIDTH, (char*)NULL, 1.9f);
    overallwidth     = GfParmGetNum(hdle, SECT_CAR, PRM_OVERALLWIDTH, (char*)NULL, car->dimension.y);
    car->dimension.z = GfParmGetNum(hdle, SECT_CAR, PRM_HEIGHT, (char*)NULL, 1.2f);
    car->mass        = GfParmGetNum(hdle, SECT_CAR, PRM_MASS, (char*)NULL, 1500);
    car->Minv        = (tdble) (1.0 / car->mass);
    car->statGC.y    = (tdble) (- gcrl * car->dimension.y + car->dimension.y / 2.0);
    car->statGC.z    = GfParmGetNum(hdle, SECT_CAR, PRM_GCHEIGHT, (char*)NULL, .5);
    k                = GfParmGetNum(hdle, SECT_CAR, PRM_CENTR, (char*)NULL, 1.0);
    carElt->_drvPos_x = GfParmGetNum(hdle, SECT_DRIVER, PRM_XPOS, (char*)NULL, 0.0);
    carElt->_drvPos_y = GfParmGetNum(hdle, SECT_DRIVER, PRM_YPOS, (char*)NULL, 0.0);
    carElt->_drvPos_z = GfParmGetNum(hdle, SECT_DRIVER, PRM_ZPOS, (char*)NULL, 0.0);
    carElt->_bonnetPos_x = GfParmGetNum(hdle, SECT_BONNET, PRM_XPOS, (char*)NULL, carElt->_drvPos_x);
    carElt->_bonnetPos_y = GfParmGetNum(hdle, SECT_BONNET, PRM_YPOS, (char*)NULL, carElt->_drvPos_y);
    carElt->_bonnetPos_z = GfParmGetNum(hdle, SECT_BONNET, PRM_ZPOS, (char*)NULL, carElt->_drvPos_z);

    /* rotational inertias */
    k = k * k;
    Irot.x = (car->mass * k * (car->dimension.y * car->dimension.y + car->dimension.z * car->dimension.z)) / 12.0;
    Irot.y = (car->mass * k * (car->dimension.x * car->dimension.x + car->dimension.z * car->dimension.z)) / 12.0;
    Irot.z = (car->mass * k * (car->dimension.y * car->dimension.y + car->dimension.x * car->dimension.x)) / 12.0;
    Irot.x = GfParmGetNum(hdle, SECT_CAR, PRM_ROLLROTINERTIA, (char*)NULL, Irot.x);
    Irot.y = GfParmGetNum(hdle, SECT_CAR, PRM_PITCHROTINERTIA, (char*)NULL, Irot.y);
    Irot.z = GfParmGetNum(hdle, SECT_CAR, PRM_YAWROTINERTIA, (char*)NULL, Irot.z);
    car->Iinv.x = (tdble) (1.0 / Irot.x);
    car->Iinv.y = (tdble) (1.0 / Irot.y);
    car->Iinv.z = (tdble) (1.0 / Irot.z);

    /* configure components */
    tCarSetupItem *setupSpring;
    tdble K[4], Kfheave, Krheave;

    for (i = 0; i < 4; i++)
    {
        setupSpring = &(car->carElt->setup.suspSpring[i]);
        setupSpring->desired_value = setupSpring->min = setupSpring-> max = 175000.0f;
        GfParmGetNumWithLimits(hdle, SuspSect[i], PRM_SPR, (char*)NULL, &(setupSpring->desired_value), &(setupSpring->min), &(setupSpring->max));
        setupSpring->changed = true;
        setupSpring->stepsize = 1000;
        K[i] = setupSpring->desired_value;
    }

    setupSpring = &(car->carElt->setup.heaveSpring[0]);
    setupSpring->desired_value = setupSpring->min = setupSpring-> max = 0.0f;
    GfParmGetNumWithLimits(hdle, SECT_FRNTHEAVE, PRM_SPR, (char*)NULL, &(setupSpring->desired_value), &(setupSpring->min), &(setupSpring->max));
    setupSpring->changed = true;
    setupSpring->stepsize = 1000;
    Kfheave = setupSpring->desired_value;
    setupSpring = &(car->carElt->setup.heaveSpring[1]);
    setupSpring->desired_value = setupSpring->min = setupSpring-> max = 0.0f;
    GfParmGetNumWithLimits(hdle, SECT_REARHEAVE, PRM_SPR, (char*)NULL, &(setupSpring->desired_value), &(setupSpring->min), &(setupSpring->max));
    setupSpring->changed = true;
    setupSpring->stepsize = 1000;
    Krheave = setupSpring->desired_value;

    /* wheel force bugfix is needed for heave springs */
    if ( (Kfheave > 0.0f) || (Krheave > 0.0f) )
        {car->features = car->features | FEAT_FIXEDWHEELFORCE;}

    w = car->mass * G;

    wf0 = w * gcfr;
    wr0 = w * (1 - gcfr);

    car->wheel[FRNT_RGT].weight0 = wf0 * gcrl * K[FRNT_RGT] / (K[FRNT_RGT] + 0.5f*Kfheave);
    car->wheel[FRNT_LFT].weight0 = wf0 * (1 - gcrl) * K[FRNT_LFT] / (K[FRNT_LFT] + 0.5f*Kfheave);
    car->wheel[REAR_RGT].weight0 = wr0 * gcrl * K[REAR_RGT] / (K[REAR_RGT] + 0.5f*Krheave);
    car->wheel[REAR_LFT].weight0 = wr0 * (1 - gcrl) * K[REAR_LFT] / (K[REAR_LFT] + 0.5f*Krheave);

    for (i = 0; i < 2; i++)
    {
        SimAxleConfig(car, i);
    }

    for (i = 0; i < 4; i++)
    {
        SimWheelConfig(car, i);
    }

    SimEngineConfig(car);
    SimTransmissionConfig(car);
    SimSteerConfig(car);
    SimBrakeSystemConfig(car);
    SimAeroConfig(car);
    car->formula_diagnostic_telemetry = GfParmGetNum(car->params, "Formula Controls", "diagnostic telemetry", NULL, 0.0) > 0.0;
    car->formula_diagnostic_elapsed = 0.0;
    car->formula_diagnostic_interval = std::max(0.001,
        (double)GfParmGetNum(car->params, "Formula Controls",
            "diagnostic telemetry interval", NULL, 0.05));
    car->formula_vertical_solver_initialized = 0;
    car->formula_vertical_failed = 0;
    car->formula_vertical_accepted_substeps = 0;
    car->formula_vertical_diagnostic_every_step =
        GfParmGetNum(car->params, "Formula Controls", "diagnostic vertical every step",
            NULL, 0.0) > 0.0;
    car->formula_vertical_authority_diagnostic =
        GfParmGetNum(car->params, "Formula Controls", "vertical authority diagnostic",
            NULL, 0.0) > 0.0;
    car->formula_vertical_authority_every_step =
        GfParmGetNum(car->params, "Formula Controls", "vertical authority every step",
            NULL, 0.0) > 0.0;
    car->formula_floor_contact_enabled =
        GfParmGetNum(car->params, "Formula Floor", "enabled", NULL, 0.0) > 0.0;
    car->formula_floor_contact_count = (int)GfParmGetNum(car->params, "Formula Floor",
        "contact count", NULL, 0.0);
    if (car->formula_floor_contact_count < 0)
    {
        car->formula_floor_contact_count = 0;
    }
    if (car->formula_floor_contact_count > TIRE_VERTICAL_MAX_CHASSIS_CONTACTS)
    {
        GfLogWarning("Formula Floor contact count %d exceeds capacity %d; truncated\n",
            car->formula_floor_contact_count, TIRE_VERTICAL_MAX_CHASSIS_CONTACTS);
        car->formula_floor_contact_count = TIRE_VERTICAL_MAX_CHASSIS_CONTACTS;
    }
    for (int contact_index = 0; contact_index < car->formula_floor_contact_count;
        ++contact_index)
    {
        char point_section[48] = {};
        std::snprintf(point_section, sizeof(point_section), "Formula Floor Point %d",
            contact_index);
        TireVerticalChassisContactConfiguration contact = {};
        contact.x_meters = (double)GfParmGetNum(car->params, point_section,
            "x", NULL, 0.0);
        contact.y_meters = (double)GfParmGetNum(car->params, point_section,
            "y", NULL, 0.0);
        contact.z_meters = (double)GfParmGetNum(car->params, point_section,
            "z", NULL, 0.0);
        const char* group = GfParmGetStr(car->params, point_section, "group", "floor");
        contact.group = std::strcmp(group, "front_wing") == 0 ? 1 : 0;
        contact.enabled = GfParmGetNum(car->params, point_section, "enabled",
            NULL, 0.0) > 0.0;
        car->formula_floor_contact[contact_index] = contact;
        car->formula_floor_previous_ground_height[contact_index] = 0.0;
    }
    int enabled_floor_contacts = 0;
    for (int contact_index = 0; contact_index < car->formula_floor_contact_count;
        ++contact_index)
    {
        enabled_floor_contacts += car->formula_floor_contact[contact_index].enabled ? 1 : 0;
    }
    GfLogInfo("Formula Floor: %d contacts, %d enabled, section=%d\n",
        car->formula_floor_contact_count, enabled_floor_contacts,
        car->formula_floor_contact_enabled);
    car->formula_diagnostic_controls_enabled =
        GfParmGetNum(car->params, "Formula Controls", "diagnostic controls",
            NULL, 0.0) > 0.0;
    car->formula_vertical_external_moment_source =
        GfParmGetNum(car->params, "Formula Controls", "external moment source",
            NULL, 1.0) > 0.0;
    car->formula_diagnostic_bump_amplitude_meters =
        (double)GfParmGetNum(car->params, "Formula Controls",
            "diagnostic bump amplitude", NULL, 0.0);
    car->formula_diagnostic_bump_frequency_hertz =
        (double)GfParmGetNum(car->params, "Formula Controls",
            "diagnostic bump frequency", NULL, 0.0);
    car->formula_diagnostic_bump_duration_seconds =
        (double)GfParmGetNum(car->params, "Formula Controls",
            "diagnostic bump duration", NULL, 0.0);
    car->formula_diagnostic_slope =
        (double)GfParmGetNum(car->params, "Formula Controls",
            "diagnostic slope", NULL, 0.0);
    car->formula_vertical_start_x = 0.0;
    car->formula_vertical_substep_seconds = SimCarVerticalClamp(
        (double)GfParmGetNum(car->params, "Formula Controls", "vertical substep seconds",
            NULL, 0.002), 0.0005, 0.01);
    car->formula_vertical_time = 0.0;
    std::memset(&car->formula_vertical_state, 0, sizeof(car->formula_vertical_state));
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        car->formula_vertical_previous_ground_height[wheel_index] = 0.0;
    }

    for (i = 0; i < 2; i++)
    {
        SimWingConfig(car, i);
    }

    /* Set the origin to GC */
    car->wheelbase = car->wheeltrack = 0;
    // Old code is misleading, because at this time staticPos.x still is relPos.x
    // car->statGC.x = car->wheel[FRNT_RGT].staticPos.x * gcfr + car->wheel[REAR_RGT].staticPos.x * (1 - gcfr);
    // Use the correct variable name instead
    car->statGC.x = car->wheel[FRNT_RGT].relPos.x * gcfr + car->wheel[REAR_RGT].relPos.x * (1 - gcfr);

    carElt->_dimension = car->dimension;
    carElt->_statGC = car->statGC;
    carElt->_tank = car->tank;
    for (i = 0; i < 4; i++)
    {
        carElt->priv.wheel[i].relPos = car->wheel[i].relPos;
    }

    for (i = 0; i < 4; i++)
    {
        car->wheel[i].staticPos.x -= car->statGC.x;
        car->wheel[i].staticPos.y -= car->statGC.y;
    }
    car->wheelbase = (tdble) ((car->wheel[FRNT_RGT].staticPos.x
                + car->wheel[FRNT_LFT].staticPos.x
                - car->wheel[REAR_RGT].staticPos.x
                - car->wheel[REAR_LFT].staticPos.x) / 2.0);
    car->wheeltrack = (tdble) ((-car->wheel[REAR_LFT].staticPos.y
                - car->wheel[FRNT_LFT].staticPos.y
                + car->wheel[FRNT_RGT].staticPos.y
                + car->wheel[REAR_RGT].staticPos.y) / 2.0);

    /* set corners pos */
    car->corner[FRNT_RGT].pos.x = (tdble) (car->dimension.x * .5 - car->statGC.x);
    car->corner[FRNT_RGT].pos.y = (tdble) (- overallwidth * .5 - car->statGC.y);
    car->corner[FRNT_RGT].pos.z = 0;

    car->corner[FRNT_LFT].pos.x = (tdble) (car->dimension.x * .5 - car->statGC.x);
    car->corner[FRNT_LFT].pos.y = (tdble) (overallwidth * .5 - car->statGC.y);
    car->corner[FRNT_LFT].pos.z = 0;

    car->corner[REAR_RGT].pos.x = (tdble) (- car->dimension.x * .5 - car->statGC.x);
    car->corner[REAR_RGT].pos.y = (tdble) (- overallwidth * .5 - car->statGC.y);
    car->corner[REAR_RGT].pos.z = 0;

    car->corner[REAR_LFT].pos.x = (tdble) (- car->dimension.x * .5 - car->statGC.x);
    car->corner[REAR_LFT].pos.y = (tdble) (overallwidth * .5 - car->statGC.y);
    car->corner[REAR_LFT].pos.z = 0;

    /* set wing positions */
    if (car->features & FEAT_AEROTOCG)
    {
        for (i = 0; i < 2; i++)
        {
            car->wing[i].staticPos.x -= car->statGC.x;
            car->wing[i].staticPos.y -= car->statGC.y;
            car->wing[i].staticPos.z -= car->statGC.z;
        }
    }

    SimCarReConfig(car);

    SimTireModelConfigure(car);
    GfLogInfo("SimuV5 vertical integration: coupled seven degree of freedom (only model)\n");

    /* initialize dashboardInstant */
    tPrivCar *priv = &(car->carElt->priv);
    tCarSetup *setup = &(car->carElt->setup);

    for (i = 0; i < NR_DI_INSTANT; i++)
    {
        priv->dashboardInstant[i].type = DI_NONE;
        priv->dashboardInstant[i].setup = NULL;
    }

    i = 0;

    if (setup->brakeRepartition.min != setup->brakeRepartition.max)
    {
        priv->dashboardInstant[i].type = DI_BRAKE_REPARTITION;
        priv->dashboardInstant[i].setup = &(setup->brakeRepartition);
        i++;
    }

    if (setup->arbSpring[0].min != setup->arbSpring[0].max)
    {
        priv->dashboardInstant[i].type = DI_FRONT_ANTIROLLBAR;
        priv->dashboardInstant[i].setup = &(setup->arbSpring[0]);
        i++;
    }

    if (setup->arbSpring[1].min != setup->arbSpring[1].max)
    {
        priv->dashboardInstant[i].type = DI_REAR_ANTIROLLBAR;
        priv->dashboardInstant[i].setup = &(setup->arbSpring[1]);
        i++;
    }

    if (setup->differentialType[0] == DIFF_ELECTRONIC_LSD)
    {
        if (setup->differentialMaxSlipBias[0].min != setup->differentialMaxSlipBias[0].max)
        {
            priv->dashboardInstant[i].type = DI_FRONT_DIFF_MAX_SLIP_BIAS;
            priv->dashboardInstant[i].setup = &(setup->differentialMaxSlipBias[0]);
            i++;
        }

        if (setup->differentialCoastMaxSlipBias[0].min != setup->differentialCoastMaxSlipBias[0].max)
        {
            priv->dashboardInstant[i].type = DI_FRONT_DIFF_COAST_MAX_SLIP_BIAS;
            priv->dashboardInstant[i].setup = &(setup->differentialCoastMaxSlipBias[0]);
            i++;
        }
    }

    if (setup->differentialType[1] == DIFF_ELECTRONIC_LSD)
    {
        if (setup->differentialMaxSlipBias[1].min != setup->differentialMaxSlipBias[1].max)
        {
            priv->dashboardInstant[i].type = DI_REAR_DIFF_MAX_SLIP_BIAS;
            priv->dashboardInstant[i].setup = &(setup->differentialMaxSlipBias[1]);
            i++;
        }

        if (setup->differentialCoastMaxSlipBias[1].min != setup->differentialCoastMaxSlipBias[1].max) {
            priv->dashboardInstant[i].type = DI_REAR_DIFF_COAST_MAX_SLIP_BIAS;
            priv->dashboardInstant[i].setup = &(setup->differentialCoastMaxSlipBias[1]);
            i++;
        }
    }

    if (setup->differentialType[2] == DIFF_ELECTRONIC_LSD)
    {
        if (setup->differentialMaxSlipBias[2].min != setup->differentialMaxSlipBias[2].max)
        {
            priv->dashboardInstant[i].type = DI_CENTRAL_DIFF_MAX_SLIP_BIAS;
            priv->dashboardInstant[i].setup = &(setup->differentialMaxSlipBias[2]);
            i++;
        }

        if (setup->differentialCoastMaxSlipBias[2].min != setup->differentialCoastMaxSlipBias[2].max) {
            priv->dashboardInstant[i].type = DI_CENTRAL_DIFF_COAST_MAX_SLIP_BIAS;
            priv->dashboardInstant[i].setup = &(setup->differentialCoastMaxSlipBias[2]);
            i++;
        }
    }
    priv->dashboardInstantNb = i;

    /* initialize dashboardRequest */
    setup->reqRepair.min = setup->reqRepair.value = setup->reqRepair.max = 0.0;
    setup->reqRepair.desired_value = 0.0;
    setup->reqRepair.stepsize = 500;
    setup->reqRepair.changed = false;

    setup->reqTireset.min = 0.0;
    setup->reqTireset.max = 1.0;
    setup->reqTireset.value = 1.0;
    setup->reqTireset.desired_value = 1.0; //1.0 means change tires, 0.0 keep old tires
    setup->reqTireset.stepsize = 1.0;
    setup->reqTireset.changed = false;

    setup->reqTirecompound.min = 1;
    setup->reqTirecompound.max = 5;
    setup->reqTirecompound.value = 3;
    setup->reqTirecompound.desired_value = 3;
    setup->reqTirecompound.stepsize = 1;
    setup->reqTirecompound.changed = false;

    setup->reqPenalty.min = 0.0;
    setup->reqPenalty.max = 1.0;
    setup->reqPenalty.value = 0.0;
    setup->reqPenalty.desired_value = 0.0; //0.0 means refuel/repair next, 1.0 means serve penalty next
    setup->reqPenalty.stepsize = 1.0;
    setup->reqPenalty.changed = false;

    const struct setup
    {
        int type;
        tCarSetupItem *setup;
        bool (*enable)(const tCar *, const tCarSetup *);
    } setups[] =
    {
        {DI_FUEL, &setup->fuel, NULL},
        {DI_REPAIR, &setup->reqRepair, NULL},
        {DI_TYRE_SET, &setup->reqTireset, enable_tyreset},
        {DI_COMPOUND_SET, &setup->reqTirecompound, enable_compound_set},
        {DI_FRONT_WING_ANGLE, &setup->wingAngle[0], enable_front_wing_angle},
        {DI_REAR_WING_ANGLE, &setup->wingAngle[1], enable_rear_wing_angle},
        {DI_PENALTY, &setup->reqPenalty, NULL}
    };

    size_t enabled = 0;

    for (size_t i = 0; i < sizeof setups / sizeof *setups; i++)
    {
        const struct setup &s = setups[i];

        if (!s.enable || s.enable(car, setup))
        {
            tDashboardItem &it = priv->dashboardRequest[enabled];

            it.type = s.type;
            it.setup = s.setup;
            enabled++;
        }
    }

    priv->dashboardRequestNb = enabled;
    priv->dashboardActiveItem = 0;
}

void
SimCarReConfig(tCar *car)
{
    int i;
    tdble	w;
    tdble	gcfrl, gcrrl, gcfr, gcrl;
    tdble	wf0, wr0;
    tdble K[4], Kfheave, Krheave;
    tCarSetupItem *setupSpring;
    tCarSetupItem *setupGcfr = &(car->carElt->setup.FRWeightRep);
    tCarSetupItem *setupGcfrl = &(car->carElt->setup.FRLWeightRep);
    tCarSetupItem *setupGcrrl = &(car->carElt->setup.RRLWeightRep);
    tCarSetupItem *setupFuel = &(car->carElt->setup.fuel);
    tCarSetupItem *setupCompound = &(car->carElt->setup.tireCompound);

    if (setupFuel->changed)
    {
        car->fuel = MIN(setupFuel->max, MAX(setupFuel->min, setupFuel->desired_value));
        if (car->fuel > car->tank) {car->fuel = car->tank;}
        setupFuel->value = car->fuel;
        setupFuel->changed = false;
    }

    if(setupCompound->changed)
    {
        setupCompound->value = MIN(setupCompound->max, MAX(setupCompound->min, setupCompound->desired_value));
        setupCompound->changed = true;
    }

    if (setupGcfr->changed)
    {
        gcfr = MIN(setupGcfr->max, MAX(setupGcfr->min, setupGcfr->desired_value));
        setupGcfr->value = gcfr;
        setupGcfr->changed = false;
    }
    else
    {
        gcfr = setupGcfr->value;
    }

    if (setupGcfrl->changed)
    {
        gcfrl = MIN(setupGcfrl->max, MAX(setupGcfrl->min, setupGcfrl->desired_value));
        setupGcfrl->value = gcfrl;
        setupGcfrl->changed = false;
    }
    else
    {
        gcfrl = setupGcfrl->value;
    }

    if (setupGcrrl->changed)
    {
        gcrrl = MIN(setupGcrrl->max, MAX(setupGcrrl->min, setupGcrrl->desired_value));
        setupGcrrl->value = gcrrl;
        setupGcrrl->changed = false;
    }
    else
    {
        gcrrl = setupGcrrl->value;
    }

    gcrl = gcfr * gcfrl + (1 - gcfr) * gcrrl;

    for (i = 0; i < 4; i++)
    {
        setupSpring = &(car->carElt->setup.suspSpring[i]);
        K[i] = MIN(setupSpring->max, MAX(setupSpring->min, setupSpring->desired_value));
    }

    setupSpring = &(car->carElt->setup.heaveSpring[0]);
    Kfheave = MIN(setupSpring->max, MAX(setupSpring->min, setupSpring->desired_value));
    setupSpring = &(car->carElt->setup.heaveSpring[1]);
    Krheave = MIN(setupSpring->max, MAX(setupSpring->min, setupSpring->desired_value));

    /* calculate wheel loads */
    w = car->mass * G;
    wf0 = w * gcfr;
    wr0 = w * (1 - gcfr);

    car->wheel[FRNT_RGT].weight0 = wf0 * gcrl * K[FRNT_RGT] / (K[FRNT_RGT] + 0.5f*Kfheave);
    car->wheel[FRNT_LFT].weight0 = wf0 * (1 - gcrl) * K[FRNT_LFT] / (K[FRNT_LFT] + 0.5f*Kfheave);
    car->wheel[REAR_RGT].weight0 = wr0 * gcrl * K[REAR_RGT] / (K[REAR_RGT] + 0.5f*Krheave);
    car->wheel[REAR_LFT].weight0 = wr0 * (1 - gcrl) * K[REAR_LFT] / (K[REAR_LFT] + 0.5f*Krheave);

    /* reconfigure components */
    if (Kfheave > 0.0f)
    {
        wf0 = (wf0 - car->wheel[FRNT_RGT].weight0 - car->wheel[FRNT_LFT].weight0);
        SimAxleReConfig(car, FRNT, wf0);
    }
    else
    {
        SimAxleReConfig(car, FRNT, 0.0f);
    }

    if (Krheave > 0.0f)
    {
        wr0 = (wr0 - car->wheel[REAR_RGT].weight0 - car->wheel[REAR_LFT].weight0);
        SimAxleReConfig(car, REAR, wr0);
    }
    else
    {
        SimAxleReConfig(car, REAR, 0.0f);
    }

    for (i = 0; i < 4; i++)
    {
        SimWheelReConfig(car, i);
    }

    SimEngineReConfig(car);
    SimTransmissionReConfig(car);
    SimSteerReConfig(car);
    SimBrakeSystemReConfig(car);

    for (i = 0; i < 2; i++)
    {
        SimWingReConfig(car, i);
    }
}

static void
SimCarUpdateForces(tCar *car)
{
    tForces	F;
    int		i;
    tdble	m, w, minv;
    tdble	SinTheta;
    tdble	Cosz, Sinz;
    tdble	v, R, Rv, Rm, Rx, Ry;
    tdble	desiredF, desiredTq;

    Cosz = car->Cosz = cos(car->DynGCg.pos.az);
    Sinz = car->Sinz = sin(car->DynGCg.pos.az);

    car->preDynGC = car->DynGCg;

    /* total mass */
    m = car->mass + (car->fuel * car->fuelMass);
    minv = (tdble) (1.0 / m);
    w = -m * G;

    /* Weight */
    SinTheta = (tdble) ((-car->wheel[FRNT_RGT].zRoad - car->wheel[FRNT_LFT].zRoad
        + car->wheel[REAR_RGT].zRoad + car->wheel[REAR_LFT].zRoad) / (2.0 * car->wheelbase));
    F.F.x = -w * SinTheta;
    SinTheta = (tdble) ((-car->wheel[FRNT_RGT].zRoad - car->wheel[REAR_RGT].zRoad
        + car->wheel[FRNT_LFT].zRoad + car->wheel[REAR_LFT].zRoad) / (2.0 * car->wheeltrack));
    F.F.y = -w * SinTheta;
    F.F.z = (tdble) (w - (F.F.x*F.F.x + F.F.y*F.F.y)/(2.0*w));/*Taylor-polinom of sqrt(w^2-F.F.x^2-F.F.y^2)*/
    F.M.x = F.M.y = F.M.z = 0;

    /* Wheels */
    for (i = 0; i < 4; i++) {
        /* forces */
        F.F.x += car->wheel[i].forces.x;
        F.F.y += car->wheel[i].forces.y;
        F.F.z += car->wheel[i].forces.z;
        /* moments */
        F.M.x += car->wheel[i].forces.z * car->wheel[i].staticPos.y +
            car->wheel[i].forces.y * (car->statGC.z + car->wheel[i].rideHeight);
        F.M.x += car->wheel[i].torques.x;
        F.M.y -= car->wheel[i].forces.z * car->wheel[i].staticPos.x +
            car->wheel[i].forces.x * (car->statGC.z + car->wheel[i].rideHeight);
        F.M.y += car->wheel[i].torques.y;
        F.M.z += -car->wheel[i].forces.x * car->wheel[i].staticPos.y +
            car->wheel[i].forces.y * car->wheel[i].staticPos.x;
        F.M.z += car->wheel[i].torques.z;
    }

    /* Aero Drag */
    F.F.x += car->aero.drag;

    /* Wings & Aero Downforce */
    for (i = 0; i < 2; i++) {
    /* forces */
    F.F.z += car->wing[i].forces.z + car->aero.lift[i];
    F.F.x += car->wing[i].forces.x;
    /* moments */
    F.M.y -= (car->wing[i].forces.z + car->aero.lift[i]) * car->wing[i].staticPos.x +
        car->wing[i].forces.x * car->wing[i].staticPos.z;
    }

    F.F.x += car->aero.floor_drag;
    F.F.z += car->aero.floor_downforce;
    F.M.y -= car->aero.floor_downforce * (car->aero.floor_position.x - car->statGC.x) +
        car->aero.floor_drag * (car->aero.floor_position.z - car->statGC.z);

    /* Rolling Resistance */
    v = car->DynGC.vel.xy;
    R = 0;
    for (i = 0; i < 4; i++) {
        R += car->wheel[i].rollRes;
    }
    if (v > 0.00001) {
        if (v > 10.0) {
            Rv = R / v;
        } else {
            Rv = (tdble) (R / 10.0);
        }
        if ((Rv * minv * SimDeltaTime) > v) {
            Rv = v * m / SimDeltaTime;
        }
    } else {
        Rv = 0;
    }
    Rx = Rv * car->DynGC.vel.x;
    Ry = Rv * car->DynGC.vel.y;

    F.F.x -= Rx;
    F.F.y -= Ry;

    if ((R * car->wheelbase / 2.0 * car->Iinv.z) > fabs(car->DynGCg.vel.az)) {
        Rm = car->DynGCg.vel.az / car->Iinv.z;
    } else {
        Rm = (tdble) (SIGN(car->DynGCg.vel.az) * R * car->wheelbase / 2.0);
    }
    F.M.z -= Rm;

    /* simulate sticking when car almost stationary */
    if ((car->features & FEAT_SLOWGRIP) && ( v < 0.1 ) )
    {
        w = -w; //make it positive
        /* desired force to stop sideway slide */
        desiredF = - m * car->DynGC.vel.y / SimDeltaTime;
        if ( (fabs(desiredF - F.F.y)) < w )
        {
            F.F.y = desiredF;
        }
        else if ( (desiredF - F.F.y) > 0.0 )
        {
            F.F.y += w;
        }
        else
        {
            F.F.y -= w;
        }

        /* desired torque to stop yaw */
        desiredTq = - car->DynGC.vel.az / ( SimDeltaTime * car->Iinv.z );
        if ( (fabs(desiredTq - F.M.z)) < 0.5 * w * car->wheelbase) {F.M.z = desiredTq;}
        else if ( (desiredTq - F.M.z) > 0.0 ) {F.M.z += 0.5f * w * car->wheelbase;}
        else {F.M.z -= 0.5f * w * car->wheelbase;}
        /* desired force to really stop the car when braking to 0 */
        if ( ((car->ctrl->brakeCmd > 0.05) || (car->ctrl->ebrakeCmd > 0.1) || (car->ctrl->brakeFrontLeftCmd > 0.02)
          || (car->ctrl->brakeFrontRightCmd > 0.02) || (car->ctrl->brakeRearLeftCmd > 0.02) || (car->ctrl->brakeRearRightCmd > 0.02) )
          && (car->ctrl->accelCmd * car->transmission.clutch.transferValue < 0.05) && (fabs(car->DynGC.vel.x) < 0.02) ) {
            desiredF = - m * car->DynGC.vel.x / SimDeltaTime;
            w *= 0.5;
            if ( (fabs(desiredF - F.F.x)) < w ) {F.F.x = desiredF;}
            else if ( (desiredF - F.F.x) > 0.0 ) {F.F.x += w;}
            else {F.F.x -= w;}
        }
    }

    /* compute accelerations */
    car->DynGC.acc.x = F.F.x * minv;
    car->DynGC.acc.y = F.F.y * minv;
    car->DynGC.acc.z = F.F.z * minv;

    car->DynGCg.acc.x = (F.F.x * Cosz - F.F.y * Sinz) * minv;
    car->DynGCg.acc.y = (F.F.x * Sinz + F.F.y * Cosz) * minv;
    car->DynGCg.acc.z = car->DynGC.acc.z;

    car->DynGCg.acc.ax = car->DynGC.acc.ax = F.M.x * car->Iinv.x;
    car->DynGCg.acc.ay = car->DynGC.acc.ay = F.M.y * car->Iinv.y;
    car->DynGCg.acc.az = car->DynGC.acc.az = F.M.z * car->Iinv.z;
}

static void
SimCarUpdateSpeed(tCar *car)
{
    // fuel consumption
    tdble delta_fuel = car->fuel_prev - car->fuel;
    car->fuel_prev = car->fuel;
    if (delta_fuel > 0) {
        car->carElt->_fuelTotal += delta_fuel;
    }
    tdble fi;
    tdble as = sqrt(car->airSpeed2);
    if (as<0.1) {
        fi = 99.9f;
    } else {
        fi = 100000 * delta_fuel / (as*SimDeltaTime);
    }
    tdble alpha = 0.1f;
    car->carElt->_fuelInstant = (tdble)((1.0-alpha)*car->carElt->_fuelInstant + alpha*fi);

    tdble	Cosz, Sinz;
    //tdble	mass;

    //mass = car->mass + car->fuel;

    Cosz = car->Cosz;
    Sinz = car->Sinz;

    car->DynGCg.vel.x += car->DynGCg.acc.x * SimDeltaTime;
    car->DynGCg.vel.y += car->DynGCg.acc.y * SimDeltaTime;

    car->DynGCg.vel.az += car->DynGCg.acc.az * SimDeltaTime;

    /* spin limitation */
    if (fabs(car->DynGCg.vel.az) > 9.0)
    {
        car->DynGCg.vel.az = (tdble) (SIGN(car->DynGCg.vel.az) * 9.0);
    }

    car->DynGC.vel.ax = car->DynGCg.vel.ax;
    car->DynGC.vel.ay = car->DynGCg.vel.ay;
    car->DynGC.vel.az = car->DynGCg.vel.az;

    car->DynGC.vel.x = car->DynGCg.vel.x * Cosz + car->DynGCg.vel.y * Sinz;
    car->DynGC.vel.y = -car->DynGCg.vel.x * Sinz + car->DynGCg.vel.y * Cosz;
    car->DynGC.vel.z = car->DynGCg.vel.z;

    /* 2D speed */
    car->DynGC.vel.xy = sqrt(car->DynGCg.vel.x * car->DynGCg.vel.x  +
         car->DynGCg.vel.y * car->DynGCg.vel.y);
}

void
SimCarUpdateWheelPos(tCar *car)
{
    int i;
    tdble vx;
    tdble vy;
    tdble Cosz, Sinz;

    Cosz = car->Cosz;
    Sinz = car->Sinz;
    vx = car->DynGC.vel.x;
    vy = car->DynGC.vel.y;

    /* Wheels data */
    for (i = 0; i < 4; i++) {
        tdble x = car->wheel[i].staticPos.x;
        tdble y = car->wheel[i].staticPos.y;
        tdble dx = x * Cosz - y * Sinz;
        tdble dy = x * Sinz + y * Cosz;

        car->wheel[i].pos.x = car->DynGCg.pos.x + dx;
        car->wheel[i].pos.y = car->DynGCg.pos.y + dy;
        car->wheel[i].pos.z = car->DynGCg.pos.z - car->statGC.z - x * sin(car->DynGCg.pos.ay) + y * sin(car->DynGCg.pos.ax);

        car->wheel[i].bodyVel.x = vx - car->DynGC.vel.az * y;
        car->wheel[i].bodyVel.y = vy + car->DynGC.vel.az * x;
    }
}

static double SimCarVerticalClamp(double value, double lower, double upper)
{
    if (value < lower)
    {
        return lower;
    }
    if (value > upper)
    {
        return upper;
    }
    return value;
}

void SimCarBuildVerticalConfiguration(tCar *car,
    TireVerticalSolverConfiguration& configuration)
{
    std::memset(&configuration, 0, sizeof(configuration));
    configuration.total_mass_kilograms = car->mass + car->fuel * car->fuelMass;
    configuration.roll_inertia_kilogram_meters_squared = 1.0 / car->Iinv.x;
    configuration.pitch_inertia_kilogram_meters_squared = 1.0 / car->Iinv.y;
    configuration.gravity_meters_per_second_squared = G;

    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        tWheel& wheel = car->wheel[wheel_index];
        TireVerticalCornerConfiguration& corner = configuration.corner[wheel_index];
        configuration.wheel_x_meters[wheel_index] = wheel.staticPos.x;
        configuration.wheel_y_meters[wheel_index] = wheel.staticPos.y;
        configuration.unsprung_mass_kilograms[wheel_index] = wheel.mass;

        corner.spring.spring_constant_newtons_per_meter = wheel.susp.spring.K;
        corner.spring.preload_newtons = wheel.susp.spring.F0;
        corner.spring.reference_position_meters = wheel.susp.spring.x0;
        corner.spring.minimum_position_meters = wheel.susp.spring.packers;
        corner.spring.maximum_position_meters = wheel.susp.spring.xMax;
        corner.spring.bellcrank_ratio = wheel.susp.spring.bellcrank;
        corner.spring.inertance_kilograms = wheel.susp.inertance;

        corner.damper.bump.slow_coefficient_newtons_per_second_per_meter =
            wheel.susp.damper.bump.C1;
        corner.damper.bump.fast_coefficient_newtons_per_second_per_meter =
            wheel.susp.damper.bump.C2;
        corner.damper.bump.transition_velocity_meters_per_second =
            wheel.susp.damper.bump.v1;
        corner.damper.bump.slow_intercept_newtons = wheel.susp.damper.bump.b1;
        corner.damper.bump.fast_intercept_newtons = wheel.susp.damper.bump.b2;
        corner.damper.rebound.slow_coefficient_newtons_per_second_per_meter =
            wheel.susp.damper.rebound.C1;
        corner.damper.rebound.fast_coefficient_newtons_per_second_per_meter =
            wheel.susp.damper.rebound.C2;
        corner.damper.rebound.transition_velocity_meters_per_second =
            wheel.susp.damper.rebound.v1;
        corner.damper.rebound.slow_intercept_newtons = wheel.susp.damper.rebound.b1;
        corner.damper.rebound.fast_intercept_newtons = wheel.susp.damper.rebound.b2;
        corner.damper.efficiency = wheel.susp.damper.efficiency;

        const int axle = wheel_index / 2;
        (void)axle;
        corner.unloaded_radius_meters = wheel.radius;
    }

    for (int axle = 0; axle < 2; ++axle)
    {
        TireVerticalAxleConfiguration& axle_configuration = configuration.axle[axle];
        axle_configuration.anti_roll_stiffness_newtons_per_meter =
            car->axle[axle].arbSusp.spring.K;
        axle_configuration.anti_roll_enabled =
            axle_configuration.anti_roll_stiffness_newtons_per_meter > 0.0;

        const tSuspension& heave = car->axle[axle].heaveSusp;
        axle_configuration.heave_spring.spring_constant_newtons_per_meter = heave.spring.K;
        axle_configuration.heave_spring.preload_newtons = heave.spring.F0;
        axle_configuration.heave_spring.reference_position_meters = heave.spring.x0;
        axle_configuration.heave_spring.minimum_position_meters = heave.spring.packers;
        axle_configuration.heave_spring.maximum_position_meters = heave.spring.xMax;
        axle_configuration.heave_spring.bellcrank_ratio = heave.spring.bellcrank;
        axle_configuration.heave_spring.inertance_kilograms = heave.inertance;
        axle_configuration.heave_damper.bump.slow_coefficient_newtons_per_second_per_meter =
            heave.damper.bump.C1;
        axle_configuration.heave_damper.bump.fast_coefficient_newtons_per_second_per_meter =
            heave.damper.bump.C2;
        axle_configuration.heave_damper.bump.transition_velocity_meters_per_second =
            heave.damper.bump.v1;
        axle_configuration.heave_damper.bump.slow_intercept_newtons = heave.damper.bump.b1;
        axle_configuration.heave_damper.bump.fast_intercept_newtons = heave.damper.bump.b2;
        axle_configuration.heave_damper.rebound.slow_coefficient_newtons_per_second_per_meter =
            heave.damper.rebound.C1;
        axle_configuration.heave_damper.rebound.fast_coefficient_newtons_per_second_per_meter =
            heave.damper.rebound.C2;
        axle_configuration.heave_damper.rebound.transition_velocity_meters_per_second =
            heave.damper.rebound.v1;
        axle_configuration.heave_damper.rebound.slow_intercept_newtons =
            heave.damper.rebound.b1;
        axle_configuration.heave_damper.rebound.fast_intercept_newtons =
            heave.damper.rebound.b2;
        axle_configuration.heave_damper.efficiency = heave.damper.efficiency;
        axle_configuration.heave_enabled = heave.spring.K != 0.0
            || heave.spring.F0 != 0.0
            || heave.damper.bump.C1 != 0.0 || heave.damper.bump.C2 != 0.0
            || heave.damper.rebound.C1 != 0.0 || heave.damper.rebound.C2 != 0.0;
    }

    configuration.chassis_contact_count = car->formula_floor_contact_count;
    for (int contact_index = 0; contact_index < car->formula_floor_contact_count;
        ++contact_index)
    {
        configuration.chassis_contact[contact_index] =
            car->formula_floor_contact[contact_index];
    }
}

void SimCarVerticalPublish(tCar *car,
    const TireVerticalSolverState& state, const TireVerticalSolverState& previous_state,
    const TireVerticalSolverDiagnostics& diagnostics,
    const double ground_height_meters[4],
    const double ground_velocity_meters_per_second[4], double elapsed_seconds)
{
    car->DynGCg.pos.z = state.body_position[0] + car->statGC.z;
    car->DynGCg.pos.ax = state.body_position[1];
    car->DynGCg.pos.ay = state.body_position[2];
    car->DynGCg.vel.z = state.body_velocity[0];
    car->DynGCg.vel.ax = state.body_velocity[1];
    car->DynGCg.vel.ay = state.body_velocity[2];
    car->DynGC.pos.z = car->DynGCg.pos.z;
    car->DynGC.pos.ax = car->DynGCg.pos.ax;
    car->DynGC.pos.ay = car->DynGCg.pos.ay;
    car->DynGC.vel.z = car->DynGCg.vel.z;
    car->DynGC.vel.ax = car->DynGCg.vel.ax;
    car->DynGC.vel.ay = car->DynGCg.vel.ay;

    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        tWheel& wheel = car->wheel[wheel_index];
        const double bellcrank = wheel.susp.spring.bellcrank;
        const double extension = state.suspension_extension_meters[wheel_index];
        const double extension_velocity =
            state.suspension_extension_velocity_meters_per_second[wheel_index];
        const double extension_acceleration = elapsed_seconds > 0.0
            ? (extension_velocity
                - previous_state.suspension_extension_velocity_meters_per_second[wheel_index])
                / elapsed_seconds
            : 0.0;
        const double chassis_reference_height = state.body_position[0]
            - wheel.staticPos.x * state.body_position[2]
            + wheel.staticPos.y * state.body_position[1];
        const double chassis_reference_velocity = state.body_velocity[0]
            - wheel.staticPos.x * state.body_velocity[2]
            + wheel.staticPos.y * state.body_velocity[1];
        (void)chassis_reference_velocity;
        const double clearance = chassis_reference_height - ground_height_meters[wheel_index];
        const double normal_force = diagnostics.tire_normal_force_newtons[wheel_index];
        const bool supported = diagnostics.tire_contact_supported_during_step[wheel_index] != 0;
        const bool contact_at_end = diagnostics.tire_contact_at_end[wheel_index] != 0;
        const double contact_fraction = elapsed_seconds > 0.0
            ? std::max(0.0, std::min(1.0,
                diagnostics.tire_contact_support_seconds[wheel_index] / elapsed_seconds))
            : 0.0;

        wheel.pos.z = chassis_reference_height;
        wheel.relPos.z = wheel.radius - extension;
        wheel.rideHeight = clearance;
        wheel.susp.x = bellcrank * extension;
        wheel.susp.v = -bellcrank * extension_velocity;
        wheel.susp.a = bellcrank * extension_acceleration;
        wheel.susp.force = diagnostics.suspension_force_newtons[wheel_index];
        wheel.susp.state = 0;
        if (extension <= wheel.susp.spring.packers / bellcrank + 1.0e-6)
        {
            wheel.susp.state |= SIM_SUSP_COMP;
        }
        if (extension >= wheel.susp.spring.xMax / bellcrank - 1.0e-6)
        {
            wheel.susp.state |= SIM_SUSP_EXT;
        }
        wheel.state = wheel.susp.state;
        if (!contact_at_end)
        {
            wheel.state |= SIM_WH_INAIR;
        }
        wheel.forces.z = normal_force;
        wheel.axleFz = 0.0;
        wheel.axleFz3rd = 0.0;
        wheel.formula_tire_model_grounded = supported ? 1 : 0;

        const double caster_camber = sin(wheel.staticPos.ay) * wheel.steer;
        if (wheel_index % 2)
        {
            wheel.relPos.ax = -wheel.staticPos.ax - caster_camber;
        }
        else
        {
            wheel.relPos.ax = wheel.staticPos.ax - caster_camber;
        }
        wheel.relPos.az = wheel.steer + wheel.staticPos.az;

        double contact_velocity_x = 0.0;
        double contact_velocity_y = 0.0;
        SimCarComputeCoupledContactVelocityForState(car, wheel_index, state,
            ground_height_meters, contact_velocity_x, contact_velocity_y);
        wheel.bodyVel.x = contact_velocity_x;
        wheel.bodyVel.y = contact_velocity_y;
        SimTireModelUpdateWheelWithVerticalState(car, wheel_index, normal_force,
            contact_fraction, supported ? 1 : 0);
        SimWheelUpdateDriverAssistsForCoupled(car, wheel_index);
    }

    car->formula_vertical_state = state;

    for (int contact_index = 0; contact_index < car->formula_floor_contact_count;
        ++contact_index)
    {
        car->formula_floor_gap_meters[contact_index] =
            diagnostics.chassis_contact_gap_meters[contact_index];
        car->formula_floor_impulse_newton_seconds[contact_index] =
            diagnostics.chassis_contact_impulse_newton_seconds[contact_index];
        car->formula_floor_active[contact_index] =
            diagnostics.chassis_contact_active[contact_index];
    }
    car->formula_floor_energy_loss_joules =
        diagnostics.chassis_contact_energy_loss_joules;

    if (car->formula_vertical_authority_diagnostic
        && car->formula_vertical_authority_every_step)
    {
        GfLogInfo("Formula vertical authority publish: time=%.6f dt=%.6f body=%.9f roll=%.9f pitch=%.9f vbody=%.9f vroll=%.9f vpitch=%.9f dyn_z=%.9f dyn_vz=%.9f ride=%.9f,%.9f,%.9f,%.9f packers=%.9f,%.9f,%.9f,%.9f\n",
            car->formula_vertical_time, elapsed_seconds,
            state.body_position[0], state.body_position[1], state.body_position[2],
            state.body_velocity[0], state.body_velocity[1], state.body_velocity[2],
            car->DynGCg.pos.z, car->DynGCg.vel.z,
            car->wheel[0].rideHeight, car->wheel[1].rideHeight,
            car->wheel[2].rideHeight, car->wheel[3].rideHeight,
            car->wheel[0].susp.spring.packers, car->wheel[1].susp.spring.packers,
            car->wheel[2].susp.spring.packers, car->wheel[3].susp.spring.packers);
    }
}

static void SimCarComputeCoupledContactVelocityForState(tCar *car, int wheel_index,
    const TireVerticalSolverState& state, const double ground_height_meters[4],
    double& velocity_x, double& velocity_y)
{
    const tWheel& wheel = car->wheel[wheel_index];
    const double contact_height = state.body_position[0] + car->statGC.z
        - ground_height_meters[wheel_index];
    velocity_x = car->DynGC.vel.x - car->DynGC.vel.az * wheel.staticPos.y
        - state.body_velocity[2] * contact_height;
    velocity_y = car->DynGC.vel.y + car->DynGC.vel.az * wheel.staticPos.x
        + state.body_velocity[1] * contact_height;
}

static void SimCarComputeVerticalExternalForces(tCar *car,
    const TireVerticalSolverState& state,
    const double provisional_force_x[4], const double provisional_force_y[4],
    double external[3])
{
    external[0] = 0.0;
    external[1] = 0.0;
    external[2] = 0.0;

    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        const tWheel& wheel = car->wheel[wheel_index];
        const double contact_height = state.body_position[0] + car->statGC.z - wheel.zRoad;
        external[1] += provisional_force_y[wheel_index] * contact_height + wheel.torques.x;
        external[2] -= provisional_force_x[wheel_index] * contact_height;
        external[2] += wheel.torques.y;
    }

    for (int wing_index = 0; wing_index < 2; ++wing_index)
    {
        const double vertical_force = car->wing[wing_index].forces.z
            + car->aero.lift[wing_index];
        external[0] += vertical_force;
        external[2] -= vertical_force * car->wing[wing_index].staticPos.x
            + car->wing[wing_index].forces.x * car->wing[wing_index].staticPos.z;
    }

    external[0] += car->aero.floor_downforce;
    external[2] -= car->aero.floor_downforce
        * (car->aero.floor_position.x - car->statGC.x)
        + car->aero.floor_drag * (car->aero.floor_position.z - car->statGC.z);
}

static void SimCarVerticalLogStep(tCar *car, double time_seconds,
    const TireVerticalSolverState& state, const TireVerticalSolverDiagnostics& diagnostics,
    const double ground_height_meters[4],
    const double ground_velocity_meters_per_second[4])
{
    if (!car->formula_vertical_diagnostic_every_step)
    {
        return;
    }
    GfLogInfo("Formula vertical step: time=%.6f car=%d body=%.9f roll=%.9f pitch=%.9f "
        "vbody=%.9f vroll=%.9f vpitch=%.9f residual=%.9f iterations=%d active=%d "
        "energy=%.6f\n", time_seconds, car->carElt->index,
        state.body_position[0], state.body_position[1], state.body_position[2],
        state.body_velocity[0], state.body_velocity[1], state.body_velocity[2],
        diagnostics.residual_norm, diagnostics.newton_iterations,
        diagnostics.active_constraint_count, car->formula_vertical_energy_joules);
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        const tWheel& wheel = car->wheel[wheel_index];
        const double extension = state.suspension_extension_meters[wheel_index];
        const double extension_velocity =
            state.suspension_extension_velocity_meters_per_second[wheel_index];
        const double chassis_reference_height = state.body_position[0]
            - wheel.staticPos.x * state.body_position[2]
            + wheel.staticPos.y * state.body_position[1];
        const double chassis_reference_velocity = state.body_velocity[0]
            - wheel.staticPos.x * state.body_velocity[2]
            + wheel.staticPos.y * state.body_velocity[1];
        const double gap = chassis_reference_height - ground_height_meters[wheel_index]
            - extension;
        const double gap_velocity = chassis_reference_velocity - extension_velocity
            - ground_velocity_meters_per_second[wheel_index];
        GfLogInfo("Formula vertical step wheel: time=%.6f car=%d wheel=%d H=%.9f G=%.9f "
            "C=%.9f L=%.9f Ldot=%.9f gap=%.9f gapdot=%.9f N=%.6f S=%.6f contact=%d\n",
            time_seconds, car->carElt->index, wheel_index,
            chassis_reference_height, ground_height_meters[wheel_index],
            chassis_reference_height + wheel.radius - extension, extension,
            extension_velocity, gap, gap_velocity,
            diagnostics.tire_normal_force_newtons[wheel_index],
            diagnostics.suspension_force_newtons[wheel_index],
            diagnostics.tire_contact[wheel_index]);
    }
}

static double SimCarSampleGroundHeightAt(const tCar* car, double world_x, double world_y)
{
    TrackGroundSample sample;
    if (simulation_track_ground_mesh.sample(world_x, world_y,
            car->DynGCg.pos.z, sample))
    {
        return sample.height;
    }
    tTrkLocPos position;
    RtTrackGlobal2Local(car->trkPos.seg, world_x, world_y, &position, TR_LPOS_MAIN);
    return RtTrackHeightL(&position);
}

static void SimCarChassisContactWorldPosition(const tCar* car,
    const TireVerticalChassisContactConfiguration& contact,
    double& world_x, double& world_y)
{
    const double cos_yaw = std::cos(car->DynGCg.pos.az);
    const double sin_yaw = std::sin(car->DynGCg.pos.az);
    world_x = car->DynGCg.pos.x
        + contact.x_meters * cos_yaw - contact.y_meters * sin_yaw;
    world_y = car->DynGCg.pos.y
        + contact.x_meters * sin_yaw + contact.y_meters * cos_yaw;
}

static bool SimCarSolveVerticalFrame(tCar *car,
    const TireVerticalSolverConfiguration& configuration,
    const TireVerticalSolverState& previous_state,
    const double ground_height_meters[4],
    const double ground_velocity_meters_per_second[4],
    const double chassis_ground_height_meters[TIRE_VERTICAL_MAX_CHASSIS_CONTACTS],
    const double chassis_ground_velocity_meters_per_second[TIRE_VERTICAL_MAX_CHASSIS_CONTACTS],
    int substep_count, double substep_seconds, bool final_pass,
    TireVerticalSolverState& final_state, TireVerticalSolverDiagnostics& final_diagnostics)
{
    TireVerticalSolverState current_state = previous_state;
    TireVerticalSolverDiagnostics diagnostics = {};
    const double frame_duration = substep_count * substep_seconds;
    double tire_impulse[4] = {};
    double tire_support[4] = {};
    int tire_at_end[4] = {};
    int total_iterations = 0;
    int accepted_substeps = 0;
    double last_residual = 0.0;
    const int maximum_depth = 6;

    struct Interval
    {
        double offset_seconds;
        double duration_seconds;
        int depth;
    };
    Interval stack[128] = {};
    int stack_size = 0;
    int next_substep = 0;
    while (stack_size > 0 || next_substep < substep_count)
    {
        if (!stack_size)
        {
            stack[stack_size++] = Interval{next_substep * substep_seconds,
                substep_seconds, 0};
            ++next_substep;
        }
        const Interval interval = stack[--stack_size];
        TireVerticalSolverStepInput input = {};
        input.elapsed_seconds = interval.duration_seconds;
        for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
        {
            input.ground_height_meters[wheel_index] = ground_height_meters[wheel_index]
                - ground_velocity_meters_per_second[wheel_index]
                    * (frame_duration - interval.offset_seconds);
            input.ground_height_velocity_meters_per_second[wheel_index] =
                ground_velocity_meters_per_second[wheel_index];
        }
        for (int contact_index = 0; contact_index < configuration.chassis_contact_count;
            ++contact_index)
        {
            input.chassis_ground_height_meters[contact_index] =
                chassis_ground_height_meters[contact_index]
                - chassis_ground_velocity_meters_per_second[contact_index]
                    * (frame_duration - interval.offset_seconds);
            input.chassis_ground_height_velocity_meters_per_second[contact_index] =
                chassis_ground_velocity_meters_per_second[contact_index];
        }

        TireVerticalSolverState next_state;
        const bool solved = TireVerticalSolverStep(configuration, current_state, input,
            next_state, diagnostics) != 0;
        if (!solved)
        {
            if (interval.depth >= maximum_depth
                || diagnostics.failure_reason == TIRE_VERTICAL_FAILURE_NONFINITE
                || diagnostics.failure_reason == TIRE_VERTICAL_FAILURE_INITIAL_GEOMETRY)
            {
                GfLogError("Coupled vertical step failed (%s) at offset=%.9f duration=%.9g residual=%.9g iterations=%d reason=%d\n",
                    final_pass ? "final" : "predictor", interval.offset_seconds,
                    interval.duration_seconds, diagnostics.residual_norm,
                    diagnostics.newton_iterations, diagnostics.failure_reason);
                GfLogError("Vertical failure state: xyz=%.17g,%.17g,%.17g yaw=%.17g "
                    "q=%.17g,%.17g,%.17g v=%.17g,%.17g,%.17g Q=%.17g,%.17g,%.17g\n",
                    (double)car->DynGCg.pos.x, (double)car->DynGCg.pos.y,
                    (double)car->DynGCg.pos.z, (double)car->DynGCg.pos.az,
                    current_state.body_position[0], current_state.body_position[1],
                    current_state.body_position[2], current_state.body_velocity[0],
                    current_state.body_velocity[1], current_state.body_velocity[2],
                    configuration.external_body_generalized_force[0],
                    configuration.external_body_generalized_force[1],
                    configuration.external_body_generalized_force[2]);
                for (int wheel = 0; wheel < 4; ++wheel)
                    GfLogError("Vertical failure wheel: i=%d L=%.17g vL=%.17g G=%.17g vG=%.17g\n",
                        wheel, current_state.suspension_extension_meters[wheel],
                        current_state.suspension_extension_velocity_meters_per_second[wheel],
                        input.ground_height_meters[wheel],
                        input.ground_height_velocity_meters_per_second[wheel]);
                for (int contact = 0; contact < configuration.chassis_contact_count; ++contact)
                    if (configuration.chassis_contact[contact].enabled)
                        GfLogError("Vertical failure floor: i=%d G=%.17g vG=%.17g\n", contact,
                            input.chassis_ground_height_meters[contact],
                            input.chassis_ground_height_velocity_meters_per_second[contact]);
                for (int constraint = 0; constraint < diagnostics.active_constraint_count; ++constraint)
                    GfLogError("Vertical failure constraint: kind=%d i=%d J=%.17g\n",
                        diagnostics.active_constraint_kind[constraint],
                        diagnostics.active_constraint_index[constraint],
                        diagnostics.constraint_impulse_newton_seconds[constraint]);
                car->formula_vertical_failed = 1;
                return false;
            }
            const double half = 0.5 * interval.duration_seconds;
            stack[stack_size++] = Interval{ interval.offset_seconds + half, half,
                interval.depth + 1 };
            stack[stack_size++] = Interval{ interval.offset_seconds, half,
                interval.depth + 1 };
            continue;
        }

        current_state = next_state;
        ++accepted_substeps;
        total_iterations += diagnostics.newton_iterations;
        last_residual = diagnostics.residual_norm;
        for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
        {
            tire_impulse[wheel_index] +=
                diagnostics.tire_contact_impulse_newton_seconds[wheel_index];
            tire_support[wheel_index] +=
                diagnostics.tire_contact_support_seconds[wheel_index];
            tire_at_end[wheel_index] = diagnostics.tire_contact_at_end[wheel_index];
        }
        if (final_pass)
        {
            const double log_time = car->formula_vertical_time
                + interval.offset_seconds + interval.duration_seconds;
            car->formula_vertical_energy_joules = TireVerticalSystemEnergyJoules(configuration,
                current_state, ground_height_meters);
            car->formula_vertical_residual = diagnostics.residual_norm;
            car->formula_vertical_newton_iterations = diagnostics.newton_iterations;
            car->formula_vertical_active_constraint_count =
                diagnostics.active_constraint_count;
            SimCarVerticalLogStep(car, log_time, current_state, diagnostics,
                ground_height_meters, ground_velocity_meters_per_second);
        }
    }

    final_state = current_state;
    if (final_pass) car->formula_vertical_accepted_substeps = accepted_substeps;
    final_diagnostics = diagnostics;
    final_diagnostics.newton_iterations = total_iterations;
    final_diagnostics.residual_norm = last_residual;
    final_diagnostics.converged = 1;
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        final_diagnostics.tire_contact_impulse_newton_seconds[wheel_index] =
            tire_impulse[wheel_index];
        final_diagnostics.tire_contact_support_seconds[wheel_index] =
            tire_support[wheel_index];
        final_diagnostics.tire_contact_supported_during_step[wheel_index] =
            tire_support[wheel_index] > 0.0 ? 1 : 0;
        final_diagnostics.tire_contact_at_end[wheel_index] = tire_at_end[wheel_index];
        final_diagnostics.tire_contact[wheel_index] =
            tire_support[wheel_index] > 0.0 ? 1 : tire_at_end[wheel_index];
        final_diagnostics.tire_normal_force_newtons[wheel_index] = frame_duration > 0.0
            ? tire_impulse[wheel_index] / frame_duration : 0.0;
    }
    return true;
}

void SimCarVerticalCoupledUpdate(tCar *car, tSituation *situation)
{
    if (car->formula_vertical_failed)
    {
        return;
    }
    double ground_height_meters[4] = {};
    double ground_velocity_meters_per_second[4] = {};
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        SimWheelUpdateRoad(car, wheel_index);
        ground_height_meters[wheel_index] = car->wheel[wheel_index].zRoad;
        SimBrakeUpdate(car, &car->wheel[wheel_index], &car->wheel[wheel_index].brake);
    }

    if (!car->formula_vertical_solver_initialized)
    {
        car->formula_vertical_start_x = car->DynGCg.pos.x;
    }

    double chassis_ground_height_meters[TIRE_VERTICAL_MAX_CHASSIS_CONTACTS] = {};
    double chassis_ground_velocity_meters_per_second[TIRE_VERTICAL_MAX_CHASSIS_CONTACTS] = {};
    for (int contact_index = 0; contact_index < car->formula_floor_contact_count;
        ++contact_index)
    {
        if (!car->formula_floor_contact[contact_index].enabled)
        {
            continue;
        }
        double world_x = 0.0;
        double world_y = 0.0;
        SimCarChassisContactWorldPosition(car, car->formula_floor_contact[contact_index],
            world_x, world_y);
        double height = SimCarSampleGroundHeightAt(car, world_x, world_y);
        if (car->formula_diagnostic_slope != 0.0)
        {
            height += car->formula_diagnostic_slope
                * (world_x - car->formula_vertical_start_x);
        }
        chassis_ground_height_meters[contact_index] = height;
        if (car->formula_vertical_solver_initialized && SimDeltaTime > 0.0)
        {
            chassis_ground_velocity_meters_per_second[contact_index] =
                (height - car->formula_floor_previous_ground_height[contact_index])
                / SimDeltaTime;
        }
    }

    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        tWheel& wheel = car->wheel[wheel_index];
        if (car->formula_diagnostic_slope != 0.0)
        {
            ground_height_meters[wheel_index] += car->formula_diagnostic_slope
                * (wheel.pos.x - car->formula_vertical_start_x);
            ground_velocity_meters_per_second[wheel_index] +=
                car->formula_diagnostic_slope * car->DynGCg.vel.x;
        }
        if (car->formula_diagnostic_bump_amplitude_meters != 0.0
            && car->formula_diagnostic_bump_frequency_hertz > 0.0
            && car->formula_diagnostic_bump_duration_seconds > 0.0
            && car->formula_vertical_time < car->formula_diagnostic_bump_duration_seconds)
        {
            const double phase = 2.0 * 3.14159265358979323846
                * car->formula_diagnostic_bump_frequency_hertz * car->formula_vertical_time
                + 2.0 * 3.14159265358979323846 * (wheel.staticPos.x / 4.0)
                + 2.0 * 3.14159265358979323846 * (wheel.staticPos.y / 2.0);
            ground_height_meters[wheel_index] +=
                car->formula_diagnostic_bump_amplitude_meters * std::sin(phase);
            ground_velocity_meters_per_second[wheel_index] +=
                car->formula_diagnostic_bump_amplitude_meters
                * 2.0 * 3.14159265358979323846
                * car->formula_diagnostic_bump_frequency_hertz * std::cos(phase);
        }
        wheel.zRoad = ground_height_meters[wheel_index];
    }

    double provisional_force_x[4] = {};
    double provisional_force_y[4] = {};
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        if (car->formula_vertical_external_moment_source)
        {
            double contact_velocity_x = 0.0;
            double contact_velocity_y = 0.0;
            SimCarComputeCoupledContactVelocityForState(car, wheel_index,
                car->formula_vertical_state, ground_height_meters,
                contact_velocity_x, contact_velocity_y);
            car->wheel[wheel_index].bodyVel.x = contact_velocity_x;
            car->wheel[wheel_index].bodyVel.y = contact_velocity_y;
            SimTireModelEvaluateProvisionalWheel(car, wheel_index,
                car->wheel[wheel_index].forces.z,
                &provisional_force_x[wheel_index], &provisional_force_y[wheel_index]);
        }
        else
        {
            provisional_force_x[wheel_index] = car->wheel[wheel_index].forces.x;
            provisional_force_y[wheel_index] = car->wheel[wheel_index].forces.y;
        }
    }

    TireVerticalSolverConfiguration configuration;
    SimCarBuildVerticalConfiguration(car, configuration);
    double external_generalized_force[3] = {};
    SimCarComputeVerticalExternalForces(car, car->formula_vertical_state,
        provisional_force_x, provisional_force_y, external_generalized_force);
    for (int body = 0; body < 3; ++body)
    {
        configuration.external_body_generalized_force[body] =
            external_generalized_force[body];
    }
    char error_message[256] = {};
    if (!TireVerticalSolverConfigurationValidate(configuration, error_message,
            sizeof(error_message)))
    {
        GfLogError("Coupled vertical configuration invalid: %s\n", error_message);
        car->formula_vertical_failed = 1;
        return;
    }

    TireVerticalSolverDiagnostics diagnostics;
    if (!car->formula_vertical_solver_initialized)
    {
        if (!TireVerticalSolverInitializeEquilibrium(configuration, ground_height_meters,
                car->formula_vertical_state, diagnostics))
        {
            GfLogError("Coupled vertical equilibrium initialization failed\n");
            car->formula_vertical_failed = 1;
            return;
        }
        TireVerticalSolverState previous_state = car->formula_vertical_state;
        SimCarVerticalPublish(car, car->formula_vertical_state, previous_state, diagnostics,
            ground_height_meters, ground_velocity_meters_per_second, 0.0);
        for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
        {
            car->formula_vertical_previous_ground_height[wheel_index] =
                ground_height_meters[wheel_index];
            car->formula_vertical_ground_velocity[wheel_index] = 0.0;
        }
        car->formula_vertical_energy_joules = TireVerticalSystemEnergyJoules(configuration,
            car->formula_vertical_state, ground_height_meters);
        car->formula_vertical_residual = diagnostics.residual_norm;
        car->formula_vertical_newton_iterations = diagnostics.newton_iterations;
        car->formula_vertical_active_constraint_count = diagnostics.active_constraint_count;
        car->formula_vertical_solver_initialized = 1;
        for (int contact = 0; contact < car->formula_floor_contact_count; ++contact)
            car->formula_floor_previous_ground_height[contact] = chassis_ground_height_meters[contact];
        SimCarVerticalLogStep(car, car->formula_vertical_time, car->formula_vertical_state,
            diagnostics, ground_height_meters, ground_velocity_meters_per_second);
        GfLogInfo("Coupled vertical equilibrium: body=%.9f roll=%.9f pitch=%.9f "
            "L=%.9f,%.9f,%.9f,%.9f N=%.6f,%.6f,%.6f,%.6f\n",
            car->formula_vertical_state.body_position[0],
            car->formula_vertical_state.body_position[1],
            car->formula_vertical_state.body_position[2],
            car->formula_vertical_state.suspension_extension_meters[0],
            car->formula_vertical_state.suspension_extension_meters[1],
            car->formula_vertical_state.suspension_extension_meters[2],
            car->formula_vertical_state.suspension_extension_meters[3],
            diagnostics.tire_normal_force_newtons[0],
            diagnostics.tire_normal_force_newtons[1],
            diagnostics.tire_normal_force_newtons[2],
            diagnostics.tire_normal_force_newtons[3]);
        return;
    }

    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        ground_velocity_meters_per_second[wheel_index] = SimDeltaTime > 0.0
            ? (ground_height_meters[wheel_index]
                - car->formula_vertical_previous_ground_height[wheel_index]) / SimDeltaTime
            : 0.0;
    }

    const double frame_elapsed_seconds = SimDeltaTime;
    int substep_count = 1;
    if (car->formula_vertical_substep_seconds > 0.0)
    {
        const double ratio = frame_elapsed_seconds / car->formula_vertical_substep_seconds;
        if (!std::isfinite(ratio) || ratio > std::numeric_limits<int>::max())
        {
            car->formula_vertical_failed = 1;
            return;
        }
        // SimDeltaTime is float; do not turn its rounding of 2ms into a
        // spurious fifth 0.5ms substep.
        substep_count = (int)std::ceil(ratio - 8.0 * std::numeric_limits<tdble>::epsilon()
            * std::max(1.0, ratio));
        substep_count = std::max(1, substep_count);
    }
    const double substep_seconds = frame_elapsed_seconds / (double)substep_count;

    TireVerticalSolverState frame_start_state = car->formula_vertical_state;
    TireVerticalSolverState final_state;
    TireVerticalSolverDiagnostics final_diagnostics;
    if (car->formula_vertical_external_moment_source)
    {
        TireVerticalSolverState predictor_state;
        TireVerticalSolverDiagnostics predictor_diagnostics;
        if (!SimCarSolveVerticalFrame(car, configuration, frame_start_state,
                ground_height_meters, ground_velocity_meters_per_second,
                chassis_ground_height_meters, chassis_ground_velocity_meters_per_second,
                substep_count, substep_seconds, false, predictor_state,
                predictor_diagnostics))
        {
            return;
        }

        double corrected_force_x[4] = {};
        double corrected_force_y[4] = {};
        for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
        {
            double contact_velocity_x = 0.0;
            double contact_velocity_y = 0.0;
            SimCarComputeCoupledContactVelocityForState(car, wheel_index,
                predictor_state, ground_height_meters, contact_velocity_x,
                contact_velocity_y);
            car->wheel[wheel_index].bodyVel.x = contact_velocity_x;
            car->wheel[wheel_index].bodyVel.y = contact_velocity_y;
            SimTireModelEvaluateProvisionalWheel(car, wheel_index,
                predictor_diagnostics.tire_normal_force_newtons[wheel_index],
                &corrected_force_x[wheel_index], &corrected_force_y[wheel_index]);
        }
        SimCarComputeVerticalExternalForces(car, predictor_state, corrected_force_x,
            corrected_force_y, external_generalized_force);
        for (int body = 0; body < 3; ++body)
        {
            configuration.external_body_generalized_force[body] =
                external_generalized_force[body];
        }

        if (!SimCarSolveVerticalFrame(car, configuration, frame_start_state,
                ground_height_meters, ground_velocity_meters_per_second,
                chassis_ground_height_meters, chassis_ground_velocity_meters_per_second,
                substep_count, substep_seconds, true, final_state, final_diagnostics))
        {
            return;
        }
    }
    else
    {
        if (!SimCarSolveVerticalFrame(car, configuration, frame_start_state,
                ground_height_meters, ground_velocity_meters_per_second,
                chassis_ground_height_meters, chassis_ground_velocity_meters_per_second,
                substep_count, substep_seconds, true, final_state, final_diagnostics))
        {
            return;
        }
    }

    car->formula_vertical_time += frame_elapsed_seconds;
    const double view_limit = std::numeric_limits<tdble>::max();
    bool publishable = TireVerticalStateIsFinite(final_state);
    for (int body = 0; body < 3; ++body)
        publishable = publishable && std::fabs(final_state.body_position[body]) < view_limit
            && std::fabs(final_state.body_velocity[body]) < view_limit;
    for (int wheel = 0; wheel < 4; ++wheel)
        publishable = publishable && std::isfinite(final_diagnostics.tire_normal_force_newtons[wheel])
            && std::fabs(final_diagnostics.tire_normal_force_newtons[wheel]) < view_limit
            && std::fabs(final_state.suspension_extension_meters[wheel]) < view_limit
            && std::fabs(final_state.suspension_extension_velocity_meters_per_second[wheel]) < view_limit;
    if (!publishable)
    {
        GfLogError("Coupled vertical result is not representable in vehicle views\n");
        car->formula_vertical_failed = 1;
        return;
    }
    SimCarVerticalPublish(car, final_state, frame_start_state, final_diagnostics,
        ground_height_meters, ground_velocity_meters_per_second, frame_elapsed_seconds);
    car->formula_vertical_energy_joules = TireVerticalSystemEnergyJoules(configuration,
        final_state, ground_height_meters);
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        car->formula_vertical_previous_ground_height[wheel_index] =
            ground_height_meters[wheel_index];
        car->formula_vertical_ground_velocity[wheel_index] =
            ground_velocity_meters_per_second[wheel_index];
    }
    for (int contact = 0; contact < car->formula_floor_contact_count; ++contact)
        car->formula_floor_previous_ground_height[contact] = chassis_ground_height_meters[contact];
    (void)situation;
}

void SimCarApplyDiagnosticControls(tCar *car, tSituation *situation)
{
    if (!car->formula_diagnostic_controls_enabled || situation == NULL)
    {
        return;
    }

    const double time_seconds = situation->currentTime;
    car->ctrl->steer = 0.0;
    car->ctrl->accelCmd = 0.0;
    car->ctrl->brakeCmd = 1.0;
    car->ctrl->clutchCmd = 1.0;
    car->ctrl->gear = 0;
    car->ctrl->ebrakeCmd = 0.0;

    if (time_seconds >= 1.0 && time_seconds < 3.0)
    {
        car->ctrl->gear = 1;
        car->ctrl->clutchCmd = 0.0;
        car->ctrl->accelCmd = 0.12;
        car->ctrl->brakeCmd = 0.0;
        if (time_seconds >= 2.0 && time_seconds < 2.5)
        {
            car->ctrl->steer = 0.05;
        }
    }
    else if (time_seconds >= 3.0 && time_seconds < 4.0)
    {
        car->ctrl->gear = 2;
        car->ctrl->clutchCmd = 0.0;
        car->ctrl->accelCmd = 0.12;
        car->ctrl->brakeCmd = 0.0;
    }
    else if (time_seconds >= 4.0 && time_seconds < 6.5)
    {
        car->ctrl->gear = 2;
        car->ctrl->clutchCmd = 1.0;
        car->ctrl->brakeCmd = 1.0;
    }
    else if (time_seconds >= 6.5 && time_seconds < 7.0)
    {
        car->ctrl->gear = 0;
        car->ctrl->clutchCmd = 1.0;
        car->ctrl->brakeCmd = 1.0;
    }
    else if (time_seconds >= 7.0 && time_seconds < 9.5)
    {
        car->ctrl->gear = -1;
        car->ctrl->clutchCmd = 0.0;
        car->ctrl->accelCmd = 0.10;
        car->ctrl->brakeCmd = 0.0;
    }
    else if (time_seconds >= 9.5 && time_seconds < 10.5)
    {
        car->ctrl->gear = -1;
        car->ctrl->clutchCmd = 1.0;
        car->ctrl->brakeCmd = 1.0;
    }
}

void
SimCarUpdatePos(tCar *car)
{
    tdble vx, vy;
    //tdble accx, accy;


    vx = car->DynGCg.vel.x;
    vy = car->DynGCg.vel.y;

    //accx = car->DynGCg.acc.x;
    //accy = car->DynGCg.acc.y;

    car->DynGCg.pos.x += vx * SimDeltaTime;
    car->DynGCg.pos.y += vy * SimDeltaTime;

    car->DynGCg.pos.az += car->DynGCg.vel.az * SimDeltaTime;

    FLOAT_NORM_PI_PI(car->DynGCg.pos.az);

    car->DynGC.pos.x = car->DynGCg.pos.x;
    car->DynGC.pos.y = car->DynGCg.pos.y;
    car->DynGC.pos.z = car->DynGCg.pos.z;

    car->DynGC.pos.ax = car->DynGCg.pos.ax;
    car->DynGC.pos.ay = car->DynGCg.pos.ay;
    car->DynGC.pos.az = car->DynGCg.pos.az;

    RtTrackGlobal2Local(car->trkPos.seg, car->DynGCg.pos.x, car->DynGCg.pos.y, &(car->trkPos), TR_LPOS_MAIN);
}

static void
SimCarUpdateCornerPos(tCar *car)
{
    tdble Cosz = car->Cosz;
    tdble Sinz = car->Sinz;
    tdble vx = car->DynGCg.vel.x;
    tdble vy = car->DynGCg.vel.y;
    int i;

    for (i = 0; i < 4; i++) {
        tdble x = car->corner[i].pos.x + car->statGC.x;
        tdble y = car->corner[i].pos.y + car->statGC.y;
        tdble dx = x * Cosz - y * Sinz;
        tdble dy = x * Sinz + y * Cosz;

        car->corner[i].pos.ax = car->DynGCg.pos.x + dx;
        car->corner[i].pos.ay = car->DynGCg.pos.y + dy;
        /*car->corner[i].pos.az = car->DynGC.pos.z - car->statGC.z + x * sin(car->DynGC.pos.ay) + y * sin(car->DynGC.pos.ax);*/

        /* add the body rotation to the wheel        */
        /* the speed is vel.az * r                   */
        /* where r = sqrt(x*x + y*y)                 */
        /* the tangent vector is -y / r and x / r    */
        // compute corner velocity at local frame
        car->corner[i].vel.ax = - car->DynGC.vel.az * y;
        car->corner[i].vel.ay = car->DynGC.vel.az * x;

        // rotate to global and add global center of mass velocity
        // note: global to local.
        car->corner[i].vel.x = vx
            + car->corner[i].vel.ax * Cosz - car->corner[i].vel.ay * Sinz;
        car->corner[i].vel.y = vy
            + car->corner[i].vel.ax * Sinz + car->corner[i].vel.ay * Cosz;

        // add local center of mass velocity
        car->corner[i].vel.ax += car->DynGC.vel.x;
        car->corner[i].vel.ay += car->DynGC.vel.y;
    }
}

void
SimTelemetryOut(tCar *car)
{
    int i;
    tdble Fzf, Fzr;

    if (car->ctrl->telemetryMode == 1) // Full data output
    {
        printf("-----------------------------\nCar: %d %s ---\n", car->carElt->index, car->carElt->_name);
        printf("Seg: %d (%s)  Ts:%f  Tr:%f\n",
            car->trkPos.seg->id, car->trkPos.seg->name, car->trkPos.toStart, car->trkPos.toRight);
        printf("---\nMx: %f  My: %f  Mz: %f (N/m)\n", car->DynGC.acc.ax, car->DynGC.acc.ay, car->DynGC.acc.az);
        printf("Wx: %f  Wy: %f  Wz: %f (rad/s)\n", car->DynGC.vel.ax, car->DynGC.vel.ay, car->DynGC.vel.az);
        printf("Ax: %f  Ay: %f  Az: %f (rad)\n", car->DynGCg.pos.ax, car->DynGCg.pos.ay, car->DynGCg.pos.az);
        printf("---\nAx: %f  Ay: %f  Az: %f (Gs)\n", car->DynGC.acc.x/9.81, car->DynGC.acc.y/9.81, car->DynGC.acc.z/9.81);
        printf("Vx: %f  Vy: %f  Vz: %f (m/s)\n", car->DynGC.vel.x, car->DynGC.vel.y, car->DynGC.vel.z);
        printf("Px: %f  Py: %f  Pz: %f (m)\n---\n", car->DynGCg.pos.x, car->DynGCg.pos.y, car->DynGCg.pos.z);
        printf("As: %f\n---\n", sqrt(car->airSpeed2));
        for (i = 0; i < 4; i++)
        {
            printf("wheel %d - RH:%f susp:%f zr:%.2f ", i, car->wheel[i].rideHeight, car->wheel[i].susp.x, car->wheel[i].zRoad);
            printf("sx:%f sa:%f w:%f ", car->wheel[i].sx, car->wheel[i].sa, car->wheel[i].spinVel);
            printf("fx:%f fy:%f fz:%f\n", car->wheel[i].forces.x, car->wheel[i].forces.y, car->wheel[i].forces.z);
        }
        Fzf = (tdble) ((car->aero.lift[0] + car->wing[0].forces.z) / 9.81);
        Fzr = (tdble) ((car->aero.lift[1] + car->wing[1].forces.z) / 9.81);
        printf("Aero Fx:%f Fz:%f Fzf=%f Fzr=%f ratio=%f\n", car->aero.drag / 9.81, Fzf + Fzr,
            Fzf, Fzr, (Fzf + Fzr) / (car->aero.drag + 0.1) * 9.81);
    }
    else if (car->ctrl->telemetryMode == 2)
    {
        // Mass from total and mass from wheels
        float MassTotal = car->mass + (car->fuel * car->fuelMass);
        float mass = car->wheel[FRNT_RGT].weight0 + car->wheel[FRNT_LFT].weight0 + car->wheel[REAR_RGT].weight0 + car->wheel[REAR_LFT].weight0;
        float massfactor = (mass + car->fuel*G) / mass;
        float MassTotal2 = massfactor/G * (car->wheel[0].weight0 + car->wheel[1].weight0 + car->wheel[2].weight0 + car->wheel[3].weight0);

        // Measurements of downforce at axles (Forces without part caused by weight)
        float MeasurementFront = car->wheel[0].forces.z + car->wheel[1].forces.z - massfactor * (car->wheel[0].weight0 + car->wheel[1].weight0);
        float MeasurementRear = car->wheel[2].forces.z + car->wheel[3].forces.z - massfactor * (car->wheel[2].weight0 + car->wheel[3].weight0);

        float RideHeightFront = (car->wheel[0].rideHeight + car->wheel[1].rideHeight)/2;
        float RideHeightRear = (car->wheel[2].rideHeight + car->wheel[3].rideHeight)/2;
        float hm = 3 * (RideHeightFront + RideHeightRear);
        hm = hm * hm;
        hm = hm * hm;
        hm = 2 * exp(-3 * hm);


        printf("Car spd:%.1f km/h %.2f m/s air spd:%.2f m/s spd2:%.2f m2/s2\n", car->DynGC.vel.x*3.6f, car->DynGC.vel.x, sqrt(car->airSpeed2), car->airSpeed2);
        printf("Car x:%.3f m z:%.3f m r:%.3f m zr:%.3f m\n", car->statGC.x, car->statGC.z, RideHeightRear, car->statGC.z + RideHeightRear);

        printf("Mass:%.0f kg fuel:%.2f kg total: %.2f kg / %.2f N\n", car->mass, car->fuel, MassTotal, MassTotal*G);
        printf("Mass:%.2f kg Mass:%.2f kg Delta:%.5f kg\n", MassTotal, MassTotal2, MassTotal - MassTotal2);

        printf("Ride height factor:%.3f\n", hm);

        printf("Wheel f x:%.3f m z:%.3f m\n", car->wheel[0].staticPos.x, car->wheel[0].staticPos.z);
        printf("Wheel r x:%.3f m z:%.3f m\n", car->wheel[2].staticPos.x, car->wheel[2].staticPos.z);

        printf("Wheel f - RH:%.3f m ", RideHeightFront);
        printf("Fx:%.3f Fz:%.3f N\n", car->wheel[0].forces.x + car->wheel[1].forces.x, MeasurementFront);
        printf("Wheel r - RH:%.3f m ", RideHeightRear);
        printf("Fx:%.3f Fz:%.3f N\n", car->wheel[2].forces.x + car->wheel[3].forces.x, MeasurementRear);

        printf("Wheel f - Tq:%.3f Nm\n", car->wheel[0].torques.y + car->wheel[1].torques.y);
        printf("Wheel r - Tq:%.3f Nm\n", car->wheel[2].torques.y + car->wheel[3].torques.y);

        printf("Wing f x:%.3f m z:%.3f m\n", car->wing[0].staticPos.x,car->wing[0].staticPos.z);
        printf("Wing r x:%.3f m z:%.3f m\n", car->wing[1].staticPos.x,car->wing[1].staticPos.z);

        tdble WFxf = (tdble) (car->wing[0].forces.x);
        tdble WFzf = (tdble) (car->wing[0].forces.z);
        printf("Wing f Fx:%.3f N Fz:%.3f N Fx:%.3f kg Fz:%.3f kg\n", WFxf, WFzf, WFxf/G, WFzf/G);

        tdble WFxr = (tdble) (car->wing[1].forces.x);
        tdble WFzr = (tdble) (car->wing[1].forces.z);
        printf("Wing r Fx:%.3f N Fz:%.3f N Fx:%.3f kg Fz:%.3f kg\n", WFxr, WFzr, WFxr/G, WFzr/G);

        Fzf = (tdble) ((car->aero.lift[0]*G + car->wing[0].forces.z));
        Fzr = (tdble) ((car->aero.lift[1]*G + car->wing[1].forces.z));
        tdble AFzf = (tdble) (car->aero.lift[0]*G);
        tdble AFzr = (tdble) (car->aero.lift[1]*G);
        printf("Aero Lift  Fzf=%.3f N Fzr=%.3f N Fz:%.3f N\n", AFzf, AFzr, AFzf + AFzr);
        printf("Aero Wing  Fzf=%.3f N Fzr=%.3f N Fz:%.3f N\n", Fzf-AFzf, Fzr-AFzr, Fzf + Fzr - AFzf - AFzr);
        printf("Aero Force Fzf=%.3f N Fzr=%.3f N Fz:%.3f N\n", Fzf, Fzr, Fzf + Fzr);

        printf("Aero Drag  Fx:%.3f N\n", car->aero.drag);

        printf("Downforce front:%.3f N\n", MeasurementFront);
        printf("Downforce rear:%.3f N\n", MeasurementRear);
        printf("Downforce total:%.3f N\n", MeasurementFront + MeasurementRear);

    }
    else if (car->ctrl->telemetryMode == 3)
    {
      for (i = 0; i < 4; i++)
      {
        /* forces */
        fprintf(stderr,"%d: fx:%.1f N  fy:%.1f N  fz:%.1f N\n", i, car->wheel[i].forces.x, car->wheel[i].forces.y, car->wheel[i].forces.z);
      }
    }
    else if (car->ctrl->telemetryMode == 4)
    {
        /* Balances */
        double ForceFront = car->wheel[0].forces.x + car->wheel[1].forces.x;
        double ForceRear  = car->wheel[2].forces.x + car->wheel[3].forces.x;
        double ForceRight = car->wheel[0].forces.x + car->wheel[2].forces.x;
        double ForceLeft  = car->wheel[1].forces.x + car->wheel[3].forces.x;
        double ForceTotal1 = MAX(0.1,fabs(ForceFront) + fabs(ForceRear));
        double ForceTotal2 = MAX(0.1,fabs(ForceLeft) + fabs(ForceRight));
        fprintf(stderr,"BxFR%+7.1f%% BxLR%+7.1f%% ", 100.0 * ForceFront / (ForceTotal1), 100.0 * ForceLeft / (ForceTotal2));

        ForceFront = car->wheel[0].forces.y + car->wheel[1].forces.y;
        ForceRear  = car->wheel[2].forces.y + car->wheel[3].forces.y;
        ForceRight = car->wheel[0].forces.y + car->wheel[2].forces.y;
        ForceLeft  = car->wheel[1].forces.y + car->wheel[3].forces.y;
        ForceTotal1 = MAX(0.1,fabs(ForceFront) + fabs(ForceRear));
        ForceTotal2 = MAX(0.1,fabs(ForceLeft) + fabs(ForceRight));
        fprintf(stderr,"ByFR%+7.1f%% ByLR%+7.1f%% ", 100.0 * ForceFront / (ForceTotal1), 100.0 * ForceLeft / (ForceTotal2));

        ForceFront = car->wheel[0].forces.z + car->wheel[1].forces.z;
        ForceRear  = car->wheel[2].forces.z + car->wheel[3].forces.z;
        ForceRight = car->wheel[0].forces.z + car->wheel[2].forces.z;
        ForceLeft  = car->wheel[1].forces.z + car->wheel[3].forces.z;
        ForceTotal1 = MAX(0.1,fabs(ForceFront) + fabs(ForceRear));
        ForceTotal2 = MAX(0.1,fabs(ForceLeft) + fabs(ForceRight));
        fprintf(stderr,"BzFR%+7.1f%% BzLR%+7.1f%%\n", 100.0 * ForceFront / (ForceTotal1), 100.0 * ForceLeft / (ForceTotal2));
    }
}

void
SimCarUpdate(tCar *car, tSituation * /* s */)
{
    SimCarUpdateForces(car);
    CHECK(car);
    SimCarUpdateSpeed(car);
    CHECK(car);
    SimCarUpdateCornerPos(car);
    CHECK(car);
    SimCarUpdatePos(car);
    CHECK(car);

    const TireVerticalSolverState authority_state_before = car->formula_vertical_state;
    const double authority_pos_z_before = car->DynGCg.pos.z;
    const double authority_vel_z_before = car->DynGCg.vel.z;
    const double authority_view_pos_z_before = car->DynGC.pos.z;
    const double authority_view_vel_z_before = car->DynGC.vel.z;
    double authority_ride_before[4] = {};
    double authority_packers[4] = {};
    for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
    {
        authority_ride_before[wheel_index] = car->wheel[wheel_index].rideHeight;
        authority_packers[wheel_index] = car->wheel[wheel_index].susp.spring.packers;
    }

    SimCarCollideZ(car);
    CHECK(car);

    if (car->formula_vertical_authority_diagnostic)
    {
        const bool changed = std::fabs(car->DynGCg.pos.z - authority_pos_z_before) > 1.0e-12
            || std::fabs(car->DynGCg.vel.z - authority_vel_z_before) > 1.0e-12
            || std::fabs(car->DynGC.pos.z - authority_view_pos_z_before) > 1.0e-12
            || std::fabs(car->DynGC.vel.z - authority_view_vel_z_before) > 1.0e-12
            || std::fabs(car->wheel[0].rideHeight - authority_ride_before[0]) > 1.0e-12
            || std::fabs(car->wheel[1].rideHeight - authority_ride_before[1]) > 1.0e-12
            || std::fabs(car->wheel[2].rideHeight - authority_ride_before[2]) > 1.0e-12
            || std::fabs(car->wheel[3].rideHeight - authority_ride_before[3]) > 1.0e-12;
        if (changed || car->formula_vertical_authority_every_step)
        {
            GfLogInfo("Formula vertical authority step: time=%.6f dt=%.6f state_z=%.9f state_vz=%.9f state_roll=%.9f state_pitch=%.9f pos_z=%.9f->%.9f delta=%.9f vel_z=%.9f->%.9f view_z=%.9f->%.9f view_vz=%.9f->%.9f ride=%.9f,%.9f,%.9f,%.9f->%.9f,%.9f,%.9f,%.9f packers=%.9f,%.9f,%.9f,%.9f\n",
                car->formula_vertical_time, SimDeltaTime,
                authority_state_before.body_position[0],
                authority_state_before.body_velocity[0],
                authority_state_before.body_position[1],
                authority_state_before.body_position[2],
                authority_pos_z_before, car->DynGCg.pos.z,
                car->DynGCg.pos.z - authority_pos_z_before,
                authority_vel_z_before, car->DynGCg.vel.z,
                authority_view_pos_z_before, car->DynGC.pos.z,
                authority_view_vel_z_before, car->DynGC.vel.z,
                authority_ride_before[0], authority_ride_before[1],
                authority_ride_before[2], authority_ride_before[3],
                car->wheel[0].rideHeight, car->wheel[1].rideHeight,
                car->wheel[2].rideHeight, car->wheel[3].rideHeight,
                authority_packers[0], authority_packers[1],
                authority_packers[2], authority_packers[3]);
        }
    }
    SimCarCollideXYScene(car);
    CHECK(car);

    /* update car->carElt->setup.reqRepair with damage */
    tCarSetupItem *repair = &(car->carElt->setup.reqRepair);
    if ((repair->desired_value > 0.0) && (repair->max == repair->desired_value))
    {
        repair->max = repair->desired_value = (tdble) car->dammage;
    }
    else
    {
        repair->max = (tdble) car->dammage;
    }
}

static bool SimCarFiniteDynamics(const tDynPt& dynamics)
{
    const double values[] = { dynamics.pos.x, dynamics.pos.y, dynamics.pos.z,
        dynamics.pos.ax, dynamics.pos.ay, dynamics.pos.az, dynamics.vel.x,
        dynamics.vel.y, dynamics.vel.z, dynamics.vel.ax, dynamics.vel.ay,
        dynamics.vel.az, dynamics.acc.x, dynamics.acc.y, dynamics.acc.z,
        dynamics.acc.ax, dynamics.acc.ay, dynamics.acc.az };
    for (double value : values) if (!std::isfinite(value)) return false;
    return true;
}

bool SimCarPhysicsStep(tCar *car, tSituation *situation)
{
    if (car->formula_vertical_failed) return false;
    if (SimDeltaTime == 0.0) return true;
    // Roll back the entire car and its public views, not just its vertical
    // coordinates. Predictor inputs, brake state, forces and tire state are
    // part of the transaction as well.
    const tCar before = *car;
    const tCarElt public_before = *car->carElt;
    if (!std::isfinite(SimDeltaTime) || SimDeltaTime < 0.0
        || !SimCarFiniteDynamics(car->DynGCg) || !SimCarFiniteDynamics(car->DynGC))
        car->formula_vertical_failed = 1;
    if (!car->formula_vertical_failed)
    {
        SimCarUpdateWheelPos(car);
        SimBrakeSystemUpdate(car);
        SimAeroUpdate(car, situation);
        for (int axle = 0; axle < 2; ++axle) SimWingUpdate(car, axle, situation);
        SimCarVerticalCoupledUpdate(car, situation);
    }
    if (!car->formula_vertical_failed)
    {
        SimTransmissionUpdate(car);
        SimWheelUpdateRotation(car);
        SimCarUpdate(car, situation);
    }
    bool valid = !car->formula_vertical_failed && TireVerticalStateIsFinite(car->formula_vertical_state)
        && SimCarFiniteDynamics(car->DynGCg) && SimCarFiniteDynamics(car->DynGC);
    for (const auto& wheel : car->wheel)
        valid = valid && std::isfinite(wheel.forces.x) && std::isfinite(wheel.forces.y)
            && std::isfinite(wheel.forces.z) && std::isfinite(wheel.spinVel)
            && std::isfinite(wheel.relPos.z) && std::isfinite(wheel.susp.force);
    if (!valid)
    {
        *car->carElt = public_before;
        *car = before;
        if (!SimCarFiniteDynamics(car->DynGCg)) car->DynGCg = public_before.pub.DynGCg;
        if (!SimCarFiniteDynamics(car->DynGC)) car->DynGC = public_before.pub.DynGC;
        car->formula_vertical_failed = 1;
        GfLogError("Vehicle physics step rejected; complete last valid car state preserved (car=%d)\n",
            car->carElt->index);
    }
    return valid;
}

void
SimCarUpdate2(tCar *car, tSituation *situation)
{
    car->formula_diagnostic_elapsed += SimDeltaTime;
    if (car->formula_diagnostic_telemetry
        && car->formula_diagnostic_elapsed >= car->formula_diagnostic_interval)
    {
        car->formula_diagnostic_elapsed = 0.0;
        GfLogInfo("Formula dynamics: time=%.6f car=%d speed=%.6f roll=%.6f roll_rate=%.6f pitch=%.6f lateral_acceleration=%.6f steering=%.6f throttle=%.6f brake=%.6f gear=%d engine_speed=%.6f engine_torque=%.6f engine_power=%.6f body_drag=%.6f front_wing_drag=%.6f rear_wing_drag=%.6f front_downforce=%.6f rear_downforce=%.6f density=%.6f damage=%d\n",
                  situation->currentTime, car->carElt->index, car->DynGC.vel.x, car->DynGCg.pos.ax, car->DynGCg.vel.ax, car->DynGCg.pos.ay,
                  car->DynGC.acc.y, car->steer.steer, car->ctrl->accelCmd, car->ctrl->brakeCmd,
                  car->carElt->_gear, car->engine.rads, car->engine.Tq, car->engine.rads * car->engine.Tq,
                  car->aero.drag, car->wing[0].forces.x, car->wing[1].forces.x,
                  car->aero.lift[0] + car->wing[0].forces.z, car->aero.lift[1] + car->wing[1].forces.z,
                  SimAirDensity, car->dammage);
        GfLogInfo("Formula power balance: time=%.6f clutch_transfer=%.6f gear_ratio=%.6f floor_drag=%.6f floor_downforce=%.6f\n",
                  situation->currentTime, car->transmission.clutch.transferValue, car->transmission.curOverallRatio,
                  car->aero.floor_drag, car->aero.floor_downforce);
        for (int wheel_index = 0; wheel_index < 4; ++wheel_index)
        {
            const tWheel& wheel = car->wheel[wheel_index];
            GfLogInfo("Formula wheel: car=%d wheel=%d state=%d mesh_contact=%d load=%.6f travel=%.6f bar_force=%.6f steer=%.6f ground_height=%.6f normal_x=%.6f normal_y=%.6f normal_z=%.6f temperature=%.6f pressure=%.6f camber=%.6f wear=%.6f wheel_speed=%.6f hysteresis_energy=%.6f lateral_energy=%.6f longitudinal_energy=%.6f convection_energy=%.6f additional_cooling_energy=%.6f heat_capacity=%.6f elapsed=%.6f\n",
                      car->carElt->index, wheel_index, wheel.state, wheel.ground_mesh_contact, wheel.forces.z,
                      wheel.susp.x, wheel.axleFz, wheel.steer, wheel.zRoad,
                      wheel.ground_mesh_normal.x, wheel.ground_mesh_normal.y, wheel.ground_mesh_normal.z,
                      wheel.Ttire, wheel.currentPressure, wheel.camber, wheel.currentWear, wheel.spinVel * wheel.radius,
                      wheel.hysteresis_energy, wheel.lateral_heating_energy, wheel.longitudinal_heating_energy,
                      wheel.convection_energy, wheel.additional_cooling_energy, wheel.thermal_capacity, SimDeltaTime);
            GfLogInfo("Formula wheel power: wheel=%d drive_torque=%.6f brake_torque=%.6f rolling_resistance=%.6f\n",
                      wheel_index, wheel.driveTq, wheel.brake.Tq, wheel.rollRes);
            const TireWheelModelState& tire = wheel.formula_tire_model;
            double ambient_pressure = car->formula_tire_configuration.pressure.ambient_absolute_pascals;
            GfLogInfo("Formula coupled force: car=%d wheel=%d force_x=%.6f force_y=%.6f spin=%.6f commanded_spin=%.6f reaction_torque=%.6f brake_torque=%.6f slip_power=%.6f utilization=%.6f\n",
                car->carElt->index, wheel_index, wheel.forces.x, wheel.forces.y,
                wheel.spinVel, wheel.in.spinVel, wheel.spinTq, wheel.brake.Tq,
                tire.force.slip_power, tire.force.combined_utilization);
            if (car->formula_vertical_solver_initialized)
            {
                const double extension =
                    car->formula_vertical_state.suspension_extension_meters[wheel_index];
                const double extension_velocity =
                    car->formula_vertical_state.suspension_extension_velocity_meters_per_second[wheel_index];
                const double chassis_velocity =
                    car->formula_vertical_state.body_velocity[0]
                    - car->wheel[wheel_index].staticPos.x
                        * car->formula_vertical_state.body_velocity[2]
                    + car->wheel[wheel_index].staticPos.y
                        * car->formula_vertical_state.body_velocity[1];
                const double gap = car->wheel[wheel_index].pos.z
                    - car->wheel[wheel_index].zRoad - extension;
                const double gap_velocity = chassis_velocity - extension_velocity
                    - car->formula_vertical_ground_velocity[wheel_index];
                GfLogInfo("Formula vertical wheel: car=%d wheel=%d H=%.9f G=%.9f C=%.9f L=%.9f Ldot=%.9f gap=%.9f gapdot=%.9f N=%.6f S=%.6f contact=%d residual=%.9f iterations=%d active=%d energy=%.6f\n",
                    car->carElt->index, wheel_index, car->wheel[wheel_index].pos.z,
                    car->wheel[wheel_index].zRoad,
                    car->wheel[wheel_index].pos.z + car->wheel[wheel_index].radius - extension,
                    extension, extension_velocity, gap, gap_velocity,
                    car->wheel[wheel_index].forces.z, car->wheel[wheel_index].susp.force,
                    car->wheel[wheel_index].formula_tire_model_grounded,
                    car->formula_vertical_residual,
                    car->formula_vertical_newton_iterations,
                    car->formula_vertical_active_constraint_count,
                    car->formula_vertical_energy_joules);
            }
            GfLogInfo("Formula coupled tire: car=%d wheel=%d enabled=%d empirical_deformation=%.6f effective_radius=%.6f contact_patch=%.6f coverage=%.6f utilization=%.6f demand=%.6f lateral_capacity=%.6f longitudinal_capacity=%.6f slip_angle=%.6f slip_ratio=%.6f aligning_torque=%.6f tread_inner=%.6f tread_center=%.6f tread_outer=%.6f carcass=%.6f gas=%.6f pressure_abs=%.6f pressure_gauge=%.6f wear_inner=%.6f wear_center=%.6f wear_outer=%.6f wear_grip=%.6f slip_power=%.6f thermal_residual=%.9f pressure_clamps=%d temperature_clamps=%d contact_clamps=%d thermal_updates=%d\n",
                      car->carElt->index, wheel_index, car->formula_tire_model_enabled,
                      tire.contact.empirical_deformation,
                      tire.contact.effective_radius, tire.contact.contact_patch,
                      tire.contact.contact_fraction, tire.force.combined_utilization,
                      tire.force.combined_demand, tire.force.lateral_capacity,
                      tire.force.longitudinal_capacity, tire.force.slip_angle_rad,
                      tire.force.slip_ratio, tire.force.aligning_torque,
                      tire.thermal.tread_inner_kelvin, tire.thermal.tread_center_kelvin,
                      tire.thermal.tread_outer_kelvin, tire.thermal.carcass_kelvin,
                      tire.thermal.gas_kelvin, tire.thermal.pressure_absolute_pascals,
                      tire.thermal.pressure_absolute_pascals - ambient_pressure,
                      tire.wear.inner_wear_fraction, tire.wear.center_wear_fraction,
                      tire.wear.outer_wear_fraction, tire.wear.wear_grip_scale,
                      tire.force.slip_power, tire.thermal.last_balance_residual_joules,
                      (int)tire.thermal.pressure_clamp_count, (int)tire.thermal.temperature_clamp_count,
                      (int)tire.contact.clamp_count, (int)tire.thermal.update_count);
        }
    }
    if ((SimTelemetry == car->carElt->index)
        || (car->ctrl->telemetryMode > 0))
        SimTelemetryOut(car);
}
