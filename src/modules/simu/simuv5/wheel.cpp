/***************************************************************************

    file                 : wheel.cpp
    created              : Sun Mar 19 00:09:06 CET 2000
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

#include <tgf.h>
#include "sim.h"
#include "track_ground_mesh.h"

static const char *WheelSect[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
static const char *SuspSect[4] = {SECT_FRNTRGTSUSP, SECT_FRNTLFTSUSP, SECT_REARRGTSUSP, SECT_REARLFTSUSP};
static const char *BrkSect[4] = {SECT_FRNTRGTBRAKE, SECT_FRNTLFTBRAKE, SECT_REARRGTBRAKE, SECT_REARLFTBRAKE};

static bool tyredeg_enabled(const tCar *car)
{
    return car->options->tyre_temperature && car->features & FEAT_TIRETEMPDEG;
}

void SimWheelConfig(tCar *car, int index)
{
    void *hdle = car->params;
    char		path[256];
    tCarElt *carElt = car->carElt;
    tWheel *wheel = &(car->wheel[index]);
    tdble rimdiam, tirewidth, tireratio, tireheight, patchLen;
    tdble Ca, RFactor, EFactor;
    tCarSetupItem *setupToe = &(car->carElt->setup.toe[index]);
    tCarSetupItem *setupCamber = &(car->carElt->setup.camber[index]);
    tCarSetupItem *setupPressure = &(car->carElt->setup.tirePressure[index]);
    tCarSetupItem *setupOpLoad = &(car->carElt->setup.tireOpLoad[index]);
    tCarSetupItem *setupCompound = &(car->carElt->setup.tireCompound);

    /* Note: ride height is already read in SimAxleConfig() */

    setupToe->desired_value = setupToe->min = setupToe->max = 0.0f;
    GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_TOE, (char*)NULL, &(setupToe->desired_value), &(setupToe->min), &(setupToe->max));
    setupToe->changed = true;
    setupToe->stepsize = (tdble) DEG2RAD(0.1);

    setupCamber->desired_value = setupCamber->min = setupCamber->max = 0.0f;
    GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_CAMBER, (char*)NULL, &(setupCamber->desired_value), &(setupCamber->min), &(setupCamber->max));
    setupCamber->changed = true;
    setupCamber->stepsize = (tdble) DEG2RAD(0.1);

    setupPressure->desired_value = setupPressure->min = setupPressure->max = 138000;
    GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_PRESSURE, (char*)NULL, &(setupPressure->desired_value), &(setupPressure->min), &(setupPressure->max));
    setupPressure->changed = true;
    setupPressure->stepsize = 10000;
    GfLogInfo(" # Car pressure tire simu = %.2f\n", setupPressure->desired_value);

    setupOpLoad->desired_value = setupOpLoad->min = setupOpLoad->max = wheel->weight0 * 1.2f;
    GfParmGetNumWithLimits(hdle, WheelSect[index], PRM_OPLOAD, (char*)NULL, &(setupOpLoad->desired_value), &(setupOpLoad->min), &(setupOpLoad->max));
    setupOpLoad->changed = true;
    setupOpLoad->stepsize = 100;

    setupCompound->desired_value = setupCompound->min = setupCompound->max = 5;
    GfParmGetNumWithLimits(hdle, SECT_TIRESET, PRM_COMPOUNDS_SET, (char*)NULL, &(setupCompound->desired_value), &(setupCompound->min), &(setupCompound->max));
    setupCompound->changed = true;
    setupCompound->stepsize = 1;

    // Additional parameters for the tire wear model
    wheel->treadThinkness	= GfParmGetNum(hdle, WheelSect[index], PRM_TREADTHICKNESS, (char*)NULL, 0.005f);		// default 5 [mm]
    tdble rimmass = GfParmGetNum(hdle, WheelSect[index], PRM_RIMMASS, (char*)NULL, 7.0f);							// default 7 [kg]
    wheel->hysteresisFactor = GfParmGetNum(hdle, WheelSect[index], PRM_HYSTERESIS, (char*)NULL, 1.0f);				// default 1.0 [-]
	wheel->coolingFactor = GfParmGetNum(hdle, WheelSect[index], PRM_TIRECOOLING, (char*)NULL, 0.0f);				// default 0.0 [-] maintain compatibility with older cars
	wheel->latHeatFactor = GfParmGetNum(hdle, WheelSect[index], PRM_LATMUHEATING, (char*)NULL, 0.0f);				// default 0.0 [-]
	wheel->longHeatFactor = GfParmGetNum(hdle, WheelSect[index], PRM_LONGMUHEATING, (char*)NULL, 0.0f);				// default 0.0 [-]
	wheel->tireSpeedCoolFactor = GfParmGetNum(hdle, WheelSect[index], PRM_TIRESPDCOOLING, (char*)NULL, 0.0f);		// default 0.0 [-] but recommend 0.5-1.25 for most cars
    wheel->corrected_thermal_load = GfParmGetNum(hdle, "Formula Controls", "correct thermal camber units", NULL, 0.0) > 0.0;
	wheel->tireTreadDrainFactor = GfParmGetNum(hdle, WheelSect[index], PRM_TREADDRAINSPD, (char*)NULL, 0.0);		// default 0
    wheel->wearFactor = GfParmGetNum(hdle, WheelSect[index], PRM_WEAR, (char*)NULL, 1.0f);

	if (car->features & FEAT_COMPOUNDS)
	{
		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_SOFT);
		wheel->hysteresisFactorC[1] = GfParmGetNum(hdle, path, PRM_HYSTERESIS, (char*)NULL, wheel->hysteresisFactor);
		wheel->coolingFactorC[1] = GfParmGetNum(hdle, path, PRM_TIRECOOLING, (char*)NULL, wheel->coolingFactor);
		wheel->latHeatFactorC[1] = GfParmGetNum(hdle, path, PRM_LATMUHEATING, (char*)NULL, wheel->latHeatFactor);
		wheel->longHeatFactorC[1] = GfParmGetNum(hdle, path, PRM_LONGMUHEATING, (char*)NULL, wheel->longHeatFactor);
		wheel->tireSpeedCoolFactorC[1] = GfParmGetNum(hdle, path, PRM_TIRESPDCOOLING, (char*)NULL, wheel->tireSpeedCoolFactor);
		wheel->tireTreadDrainFactorC[1] = GfParmGetNum(hdle, path, PRM_TREADDRAINSPD, (char*)NULL, wheel->tireTreadDrainFactor);
		wheel->wearFactorC[1] = GfParmGetNum(hdle, path, PRM_WEAR, (char*)NULL, wheel->wearFactor);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_MEDIUM);
		wheel->hysteresisFactorC[2] = GfParmGetNum(hdle, path, PRM_HYSTERESIS, (char*)NULL, wheel->hysteresisFactor);
		wheel->coolingFactorC[2] = GfParmGetNum(hdle, path, PRM_TIRECOOLING, (char*)NULL, wheel->coolingFactor);
		wheel->latHeatFactorC[2] = GfParmGetNum(hdle, path, PRM_LATMUHEATING, (char*)NULL, wheel->latHeatFactor);
		wheel->longHeatFactorC[2] = GfParmGetNum(hdle, path, PRM_LONGMUHEATING, (char*)NULL, wheel->longHeatFactor);
		wheel->tireSpeedCoolFactorC[2] = GfParmGetNum(hdle, path, PRM_TIRESPDCOOLING, (char*)NULL, wheel->tireSpeedCoolFactor);
		wheel->tireTreadDrainFactorC[2] = GfParmGetNum(hdle, path, PRM_TREADDRAINSPD, (char*)NULL, wheel->tireTreadDrainFactor);
		wheel->wearFactorC[2] = GfParmGetNum(hdle, path, PRM_WEAR, (char*)NULL, wheel->wearFactor);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_HARD);
		wheel->hysteresisFactorC[3] = GfParmGetNum(hdle, path, PRM_HYSTERESIS, (char*)NULL, wheel->hysteresisFactor);
		wheel->coolingFactorC[3] = GfParmGetNum(hdle, path, PRM_TIRECOOLING, (char*)NULL, wheel->coolingFactor);
		wheel->latHeatFactorC[3] = GfParmGetNum(hdle, path, PRM_LATMUHEATING, (char*)NULL, wheel->latHeatFactor);
		wheel->longHeatFactorC[3] = GfParmGetNum(hdle, path, PRM_LONGMUHEATING, (char*)NULL, wheel->longHeatFactor);
		wheel->tireSpeedCoolFactorC[3] = GfParmGetNum(hdle, path, PRM_TIRESPDCOOLING, (char*)NULL, wheel->tireSpeedCoolFactor);
		wheel->tireTreadDrainFactorC[3] = GfParmGetNum(hdle, path, PRM_TREADDRAINSPD, (char*)NULL, wheel->tireTreadDrainFactor);
		wheel->wearFactorC[3] = GfParmGetNum(hdle, path, PRM_WEAR, (char*)NULL, wheel->wearFactor);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_WET);
		wheel->hysteresisFactorC[4] = GfParmGetNum(hdle, path, PRM_HYSTERESIS, (char*)NULL, wheel->hysteresisFactor);
		wheel->coolingFactorC[4] = GfParmGetNum(hdle, path, PRM_TIRECOOLING, (char*)NULL, wheel->coolingFactor);
		wheel->latHeatFactorC[4] = GfParmGetNum(hdle, path, PRM_LATMUHEATING, (char*)NULL, wheel->latHeatFactor);
		wheel->longHeatFactorC[4] = GfParmGetNum(hdle, path, PRM_LONGMUHEATING, (char*)NULL, wheel->longHeatFactor);
		wheel->tireSpeedCoolFactorC[4] = GfParmGetNum(hdle, path, PRM_TIRESPDCOOLING, (char*)NULL, wheel->tireSpeedCoolFactor);
		wheel->tireTreadDrainFactorC[4] = GfParmGetNum(hdle, path, PRM_TREADDRAINSPD, (char*)NULL, 1.5f);
		wheel->wearFactorC[4] = GfParmGetNum(hdle, path, PRM_WEAR, (char*)NULL, wheel->wearFactor);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_EXTREM_WET);
		wheel->hysteresisFactorC[5] = GfParmGetNum(hdle, path, PRM_HYSTERESIS, (char*)NULL, wheel->hysteresisFactor);
		wheel->coolingFactorC[5] = GfParmGetNum(hdle, path, PRM_TIRECOOLING, (char*)NULL, wheel->coolingFactor);
		wheel->latHeatFactorC[5] = GfParmGetNum(hdle, path, PRM_LATMUHEATING, (char*)NULL, wheel->latHeatFactor);
		wheel->longHeatFactorC[5] = GfParmGetNum(hdle, path, PRM_LONGMUHEATING, (char*)NULL, wheel->longHeatFactor);
		wheel->tireSpeedCoolFactorC[5] = GfParmGetNum(hdle, path, PRM_TIRESPDCOOLING, (char*)NULL, wheel->tireSpeedCoolFactor);
		wheel->tireTreadDrainFactorC[5] = GfParmGetNum(hdle, path, PRM_TREADDRAINSPD, (char*)NULL, 3.0f);
		wheel->wearFactorC[5] = GfParmGetNum(hdle, path, PRM_WEAR, (char*)NULL, wheel->wearFactor);

		/*if (SimRain < 1)
		{
			wheel->hysteresisFactorC[4] *= 1.5;
			wheel->hysteresisFactorC[5] *= 2.0;
			wheel->wearFactorC[4] *= 3.5;
			wheel->wearFactorC[5] *= 3.5;
			GfLogInfo("# Simu wear factor compound with no rain wet = %.4f - extreme wet = %.4f\n",
								  wheel->wearFactorC[4], wheel->wearFactorC[5]);
		}*/
	}

    rimdiam               = GfParmGetNum(hdle, WheelSect[index], PRM_RIMDIAM, (char*)NULL, 0.33f);
    tirewidth             = GfParmGetNum(hdle, WheelSect[index], PRM_TIREWIDTH, (char*)NULL, 0.145f);
    tireheight            = GfParmGetNum(hdle, WheelSect[index], PRM_TIREHEIGHT, (char*)NULL, -1.0f);
    tireratio             = GfParmGetNum(hdle, WheelSect[index], PRM_TIRERATIO, (char*)NULL, 0.75f);
    wheel->mu             = GfParmGetNum(hdle, WheelSect[index], PRM_MU, (char*)NULL, 1.0f);
	wheel->muWet = GfParmGetNum(hdle, WheelSect[index], PRM_MUWET, (char*)NULL, 1.0f);

	if (car->features & FEAT_COMPOUNDS && car->options->compounds)
	{
		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_SOFT);
		wheel->muC[1] = GfParmGetNum(hdle, path, PRM_MU, (char*)NULL, wheel->mu);
		wheel->muWetC[1] = GfParmGetNum(hdle, path, PRM_MUWET, (char*)NULL, wheel->muC[1]);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_MEDIUM);
		wheel->muC[2] = GfParmGetNum(hdle, path, PRM_MU, (char*)NULL, wheel->mu);
		wheel->muWetC[2] = GfParmGetNum(hdle, path, PRM_MUWET, (char*)NULL, wheel->muC[2]);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_HARD);
		wheel->muC[3] = GfParmGetNum(hdle, path, PRM_MU, (char*)NULL, wheel->mu);
		wheel->muWetC[3] = GfParmGetNum(hdle, path, PRM_MUWET, (char*)NULL, wheel->muC[3]);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_WET);
		wheel->muC[4] = GfParmGetNum(hdle, path, PRM_MU, (char*)NULL, wheel->mu);
		wheel->muWetC[4] = GfParmGetNum(hdle, path, PRM_MUWET, (char*)NULL, wheel->muC[4] * 1.05);

		sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_EXTREM_WET);
		wheel->muC[5] = GfParmGetNum(hdle, path, PRM_MU, (char*)NULL, wheel->mu);
		wheel->muWetC[5] = GfParmGetNum(hdle, path, PRM_MUWET, (char*)NULL, wheel->muC[5] * 1.05);
		GfLogInfo("# Simu MU compound soft = %.3f - medium = %.3f - hard = %.3f - wet = %.3f - extreme wet = %.3f\n",
			wheel->muC[1], wheel->muC[2], wheel->muC[3], wheel->muC[4], wheel->muC[5]);
		GfLogInfo("# Simu MU WET compound soft = %.3f - medium = %.3f - hard = %.3f - wet = %.3f - extreme wet = %.3f\n",
			wheel->muWetC[1], wheel->muWetC[2], wheel->muWetC[3], wheel->muWetC[4], wheel->muWetC[5]);

		/*if(SimRain < 1)
		{
			wheel->muC[4] -= 0.2;
			wheel->muC[5] -= 0.3;
			GfLogInfo("# Simu MU compound with no rain wet = %.3f - extreme wet = %.3f\n",
					  wheel->muC[4], wheel->muC[5]);
		}*/
	}

    wheel->I              = GfParmGetNum(hdle, WheelSect[index], PRM_INERTIA, (char*)NULL, 1.5f);
    //BUG: the next line should go after SimBrakeConfig to have an effect
    wheel->I += wheel->brake.I; // add brake inertia
    wheel->staticPos.y    = GfParmGetNum(hdle, WheelSect[index], PRM_YPOS, (char*)NULL, 0.0f);
    Ca                    = GfParmGetNum(hdle, WheelSect[index], PRM_CA, (char*)NULL, 30.0f);
    RFactor               = GfParmGetNum(hdle, WheelSect[index], PRM_RFACTOR, (char*)NULL, 0.8f);
    EFactor               = GfParmGetNum(hdle, WheelSect[index], PRM_EFACTOR, (char*)NULL, 0.7f);
    wheel->lfMax          = GfParmGetNum(hdle, WheelSect[index], PRM_LOADFMAX, (char*)NULL, 1.6f);
    wheel->lfMin          = GfParmGetNum(hdle, WheelSect[index], PRM_LOADFMIN, (char*)NULL, 0.8f);
    wheel->AlignTqFactor  = GfParmGetNum(hdle, WheelSect[index], PRM_ALIGNTQFACTOR, (char*)NULL, 0.6f);
	wheel->LatMuFactor    = GfParmGetNum(hdle, WheelSect[index], PRM_LATMUFACTOR, (char*)NULL, 1.0f);
	wheel->LongMuFactor   = GfParmGetNum(hdle, WheelSect[index], PRM_LONGMUFACTOR, (char*)NULL, 1.0f);
    wheel->mass           = GfParmGetNum(hdle, WheelSect[index], PRM_MASS, (char*)NULL, 20.0f);

    wheel->lfMin = MIN(0.9f, wheel->lfMin);
    wheel->lfMax = MAX(1.1f, wheel->lfMax);

    if (wheel->AlignTqFactor < 0.1f )
    {
        wheel->AlignTqFactor = 0.1f;
    }

    // Absolute pressure of cold tire.
    wheel->pressure = MIN(setupPressure->max, MAX(setupPressure->min, setupPressure->desired_value));
    wheel->currentPressure	= wheel->pressure; //MIN(setupPressure->max, MAX(setupPressure->min, setupPressure->desired_value));

    if (car->features & FEAT_COMPOUNDS && car->options->compounds)
    {
        wheel->tireSet = MIN(setupCompound->max, MAX(setupCompound->min, setupCompound->desired_value));
        wheel->mu = wheel->muC[wheel->tireSet];
        wheel->hysteresisFactor = wheel->hysteresisFactorC[wheel->tireSet];
        wheel->wearFactor = wheel->wearFactorC[wheel->tireSet];
    }

    RFactor = MIN(1.0f, RFactor);
    RFactor = MAX(0.1f, RFactor);
    EFactor = MIN(1.0f, EFactor);

    if (tireheight > 0.0)
        wheel->radius = rimdiam / 2.0f + tireheight;
    else
        wheel->radius = rimdiam / 2.0f + tirewidth * tireratio;

    patchLen = wheel->weight0 / (tirewidth * wheel->pressure);
    wheel->tireSpringRate = wheel->weight0 / (wheel->radius * (1.0f - cos(asin(patchLen / (2.0f * wheel->radius)))));
    wheel->relPos.x = wheel->staticPos.x = car->axle[index/2].xpos;
    wheel->relPos.y = wheel->staticPos.y;
    /* BUG? susp.spring.x0 is still 0 here, maybe move after SimSuspReConfig in SimWheelReConfig? */
    wheel->relPos.z = wheel->radius - wheel->susp.spring.x0;
    wheel->relPos.ay = wheel->relPos.az = 0.0f;
    wheel->steer = 0.0f;

    /* temperature and degradation */
    wheel->Tinit = GfParmGetNum(hdle, WheelSect[index], PRM_INITTEMP, (char*)NULL, Tair);
    wheel->treadDepth = 1.0;
    wheel->Topt = GfParmGetNum(hdle, WheelSect[index], PRM_OPTTEMP, (char*)NULL, 350.0f);

    if ((car->features & FEAT_COMPOUNDS) && car->options->compounds)
    {
        sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_SOFT);
        wheel->TinitC[1] = GfParmGetNum(hdle, path, PRM_INITTEMP, (char*)NULL, wheel->Tinit);
        wheel->ToptC[1] = GfParmGetNum(hdle, path, PRM_OPTTEMP, (char*)NULL, wheel->Topt);

        sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_MEDIUM);
        wheel->TinitC[2] = GfParmGetNum(hdle, path, PRM_INITTEMP, (char*)NULL, wheel->Tinit);
        wheel->ToptC[2] = GfParmGetNum(hdle, path, PRM_OPTTEMP, (char*)NULL, wheel->Topt);

        sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_HARD);
        wheel->TinitC[3] = GfParmGetNum(hdle, path, PRM_INITTEMP, (char*)NULL, wheel->Tinit);
        wheel->ToptC[3] = GfParmGetNum(hdle, path, PRM_OPTTEMP, (char*)NULL, wheel->Topt);

        sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_WET);
        wheel->TinitC[4] = GfParmGetNum(hdle, path, PRM_INITTEMP, (char*)NULL, wheel->Tinit);
        wheel->ToptC[4] = GfParmGetNum(hdle, path, PRM_OPTTEMP, (char*)NULL, wheel->Topt);

        sprintf(path, "%s/%s/%s", WheelSect[index], SECT_COMPOUNDS, SECT_EXTREM_WET);
        wheel->TinitC[5] = GfParmGetNum(hdle, path, PRM_INITTEMP, (char*)NULL, wheel->Tinit);
        wheel->ToptC[5] = GfParmGetNum(hdle, path, PRM_OPTTEMP, (char*)NULL, wheel->Topt);
        GfLogInfo("# Simu Optimal temperature compound soft = %.3f - medium = %.3f - hard = %.3f - wet = %.3f - extreme wet = %.3f\n",
                  wheel->ToptC[1] -273.15, wheel->ToptC[2] - 273.15, wheel->ToptC[3] - 273.15, wheel->ToptC[4] - 273.15, wheel->ToptC[5] - 273.15);

        wheel->Tinit = wheel->TinitC[wheel->tireSet];
        wheel->Topt = wheel->ToptC[wheel->tireSet];
    }

    if (tyredeg_enabled(car))
    {
        wheel->Ttire = wheel->Tinit;
    }
    else
    {
        wheel->Ttire = wheel->Topt;
    }

    //wheel->heatingm -=SimRain;
    const tdble rubberDensity = 930.0f;	// Density of Rubber (NR) in [kg/m^3].
    wheel->treadMass = (2.0f*wheel->radius - wheel->treadThinkness)*PI*tirewidth*wheel->treadThinkness*rubberDensity;
    wheel->baseMass = wheel->mass - wheel->treadMass - rimmass;

    if (wheel->baseMass < 0.0f)
    {
        wheel->baseMass = 3.0f;
        GfError("Wheel mass minus tire tread mass minus rim mass is smaller than 0.0kg, setting it to 3.0 kg");
    }

    // Surface area for convection model
    tdble innerRadius = rimdiam / 2.0f;
    tdble tireSideArea = PI*(wheel->radius*wheel->radius - innerRadius*innerRadius);
    wheel->tireConvectionSurface = 2.0f*(PI*tirewidth*wheel->radius + tireSideArea);

    // Mass of gas in the tire m=P*V/(R*T)
    tdble temperature = Tair;		// Kelvin
    tdble volume = tireSideArea*tirewidth;		// meter*meter*meter
    tdble nitrogenR = 296.8f;					// Joule/(kg*Kelvin), N2

    wheel->tireGasMass = (wheel->pressure * volume) / (nitrogenR * temperature);	// kg

    /* components */
    SimSuspConfig(car, hdle, SuspSect[index], &(wheel->susp), index);
    SimBrakeConfig(hdle, BrkSect[index], &(wheel->brake));

    carElt->_rimRadius(index) = rimdiam / 2.0f;

    if (tireheight > 0.0)
        carElt->_tireHeight(index) = tireheight;
    else
        carElt->_tireHeight(index) = tirewidth * tireratio;

    carElt->_tireWidth(index) = tirewidth;
    carElt->_brakeDiskRadius(index) = wheel->brake.radius;
    carElt->_wheelRadius(index) = wheel->radius;

    /* initialize carElt values even if tire temperature and wear feature is not enabled */
    carElt->_tyreT_opt(index) = wheel->Topt;
    carElt->_tyreT_in(index) = wheel->Ttire;
    carElt->_tyreT_mid(index) = wheel->Ttire;
    carElt->_tyreT_out(index) = wheel->Ttire;
    carElt->_tyreCondition(index) = 1.0;
    carElt->_tyreTreadDepth(index) = wheel->treadDepth;
    carElt->_tyreCurrentPressure(index) = wheel->pressure;
    carElt->_tyreCompound(index) = wheel->tireSet;

    wheel->mfC = (tdble)(2.0 - asin(RFactor) * 2.0 / PI);
    wheel->mfE = EFactor;

	// increase stiffness factor when temperature and pressure increases
	// but only if temperature feature is enabled.
	if (car->features & FEAT_TIRETEMPDEG)
	{
		//wheel->mfB = ((Ca*0.50) + (wheel->currentPressure * 0.0001)) / wheel->mfC;

		wheel->mfB = Ca / wheel->mfC;
	}
	else
	{
		wheel->mfB = Ca / wheel->mfC;
	}

    wheel->lfK = log((1.0f - wheel->lfMin) / (wheel->lfMax - wheel->lfMin));

    wheel->feedBack.I += wheel->I;
    wheel->feedBack.spinVel = 0.0f;
    wheel->feedBack.Tq = 0.0f;
    wheel->feedBack.brkTq = 0.0f;
    wheel->torques.x = wheel->torques.y = wheel->torques.z = 0.0f;

    /* calculate optimal slip value */
    tdble s, Bx, low, high;
    int i;
    //wheel->mfC * atan(Bx * (1.0f - wheel->mfE) + wheel->mfE * atan(Bx)) == PI/2
    low = 0.0;
    high = wheel->mfB;

    if (wheel->mfC * atan(high * (1.0f - wheel->mfE) + wheel->mfE * atan(high)) < PI_2)
    {
        /* tire parameters are unphysical*/
        s = 1.0;
        GfLogWarning("Tire magic curve parameters are unphysical!");
    }
    else
    {
        for (i = 0; i < 32; i++)
        {
            Bx = (tdble)(0.5 * (low + high));
            if (wheel->mfC * atan(Bx * (1.0f - wheel->mfE) + wheel->mfE * atan(Bx)) < PI_2)
            {
                low = Bx;
            }
            else
            {
                high = Bx;
            }
        }

        s = (tdble)(0.5 * (low + high) / wheel->mfB);
    }

    car->carElt->_wheelSlipOpt(index) = s;
    GfLogInfo("SimuV4 MU = %.3f - Topt = %.2f - Tinit = %.2f - wear = %.2f\n",
              wheel->mu, wheel->Topt - 273.15, wheel->Tinit -273.15, wheel->wearFactor);
}

void SimWheelReConfig(tCar *car, int index)
{/* called by SimCarReConfig in car.cpp */
    tCarElt *carElt = car->carElt;
    tWheel *wheel = &(car->wheel[index]);
    tdble x0;

    tCarSetupItem *setupRideHeight = &(car->carElt->setup.rideHeight[index]);
    tCarSetupItem *setupToe = &(car->carElt->setup.toe[index]);
    tCarSetupItem *setupCamber = &(car->carElt->setup.camber[index]);
    tCarSetupItem *setupPressure = &(car->carElt->setup.tirePressure[index]);
    tCarSetupItem *setupOpLoad = &(car->carElt->setup.tireOpLoad[index]);
    tCarSetupItem *setupCompound = &(car->carElt->setup.tireCompound);
    tdble patchLen;

    if (setupToe->changed)
    {
        wheel->staticPos.az = MIN(setupToe->max, MAX(setupToe->min, setupToe->desired_value));
        setupToe->value = wheel->staticPos.az;
        setupToe->changed = false;
    }

    if (setupCamber->changed)
    {
        wheel->staticPos.ax = MIN(setupCamber->max, MAX(setupCamber->min, setupCamber->desired_value));
        if (index % 2)
        {
            wheel->relPos.ax = -wheel->staticPos.ax;
        }
        else
        {
            wheel->relPos.ax = wheel->staticPos.ax;
        }

        wheel->cosax = cos(wheel->relPos.ax);
        wheel->sinax = sin(wheel->relPos.ax);
        setupCamber->value = wheel->staticPos.ax;
        setupCamber -> changed = false;
    }

    if ( setupPressure->changed ||car->carElt->setup.FRWeightRep.changed )
    {
        wheel->pressure = MIN(setupPressure->max, MAX(setupPressure->min, setupPressure->desired_value));
        patchLen = wheel->weight0 / (carElt->_tireWidth(index) * wheel->pressure);
        wheel->tireSpringRate = wheel->weight0 / (wheel->radius * (1.0f - cos(asin(patchLen / (2.0f * wheel->radius)))));
        setupPressure->value = wheel->pressure;
        setupPressure->changed = false;
        GfLogInfo(" # Car simu setup Pressure simuReWheelReConfig = %.2f\n", wheel->pressure);
        SimTireModelResetWheel(car, index, false);
    }

    if (setupOpLoad->changed)
    {
        wheel->opLoad = MIN(setupOpLoad->max, MAX(setupOpLoad->min, setupOpLoad->desired_value));
        setupOpLoad->value = wheel->opLoad;
        setupOpLoad->changed = false;
    }

    if (car->features & FEAT_COMPOUNDS && car->options->compounds)
    {
        wheel->tireSet = MIN(setupCompound->max, MAX(setupCompound->min, setupCompound->desired_value));
        GfLogInfo("# Tireset value = %d\n", wheel->tireSet);
        setupCompound->value = wheel->tireSet;
        setupCompound->changed = false;
        wheel->mu = wheel->muC[wheel->tireSet];
		wheel->muWet = wheel->muWetC[wheel->tireSet];
		wheel->Tinit = wheel->TinitC[wheel->tireSet];
		wheel->Topt = wheel->ToptC[wheel->tireSet];
		wheel->hysteresisFactor = wheel->hysteresisFactorC[wheel->tireSet];
		wheel->coolingFactor = wheel->coolingFactorC[wheel->tireSet];
		wheel->latHeatFactor = wheel->latHeatFactorC[wheel->tireSet];
		wheel->longHeatFactor = wheel->longHeatFactorC[wheel->tireSet];
		wheel->tireSpeedCoolFactor = wheel->tireSpeedCoolFactorC[wheel->tireSet];
		wheel->tireTreadDrainFactor = wheel->tireTreadDrainFactorC[wheel->tireSet];
        wheel->wearFactor = wheel->wearFactorC[wheel->tireSet];
        GfLogInfo("# SimuV4 tire compound changed mu = %.3f - hysteresis = %.2f - wear factor = %.7f\n", wheel->mu, wheel->hysteresisFactor,
                  wheel->wearFactor);
        SimTireModelResetWheel(car, index, true);
    }

    x0 = setupRideHeight->value;
    SimSuspReConfig(car, &(wheel->susp), index, wheel->weight0, x0);
    GfLogInfo("SimuV4 MU = %.3f - Topt = %.2f - Tinit = %.2f - wear = %.2f\n",
              wheel->mu, wheel->Topt - 273.15, wheel->Tinit - 273.15, wheel->wearFactor);
}

void SimWheelDamage(tWheel* wheel, tdble dmg)
{
	// total wheel damage
	tdble wheelDamage = 0.0;
	double impactDamage = 0;

	impactDamage = (dmg * 0.025 * SimDeltaTime);

	if (wheelDamage < 1.0)
	{
		wheelDamage += impactDamage;
		//wheel->bent_damage_x -= impactDamage;
		//wheel->bent_damage_z -= impactDamage;
		wheel->rotational_damage_x -= wheelDamage;
		wheel->rotational_damage_z -= impactDamage;
	}
	else
	{
		wheelDamage = 1.0;
	}
	//printf ("Bent: %f -> %f\n", exp(dmg), impactDamage);
}

double GetWheelDamage(tWheel* wheel)
{
	return wheel->rotational_damage_x;
}

void SimWheelUpdateRoad(tCar *car, int index)
{
    tWheel *wheel = &(car->wheel[index]);
    RtTrackGlobal2Local(car->trkPos.seg, wheel->pos.x, wheel->pos.y, &(wheel->trkPos),
        TR_LPOS_SEGMENT);
    wheel->zRoad = RtTrackHeightL(&(wheel->trkPos));
    wheel->ground_friction = wheel->trkPos.seg->surface->kFriction;
    wheel->ground_rolling_resistance = wheel->trkPos.seg->surface->kRollRes;
    TrackGroundSample ground_sample;
    wheel->ground_mesh_contact = simulation_track_ground_mesh.sample(wheel->pos.x,
        wheel->pos.y, wheel->pos.z, ground_sample);
    if (wheel->ground_mesh_contact)
    {
        wheel->zRoad = ground_sample.height;
        wheel->ground_mesh_normal.x = ground_sample.normal[0];
        wheel->ground_mesh_normal.y = ground_sample.normal[1];
        wheel->ground_mesh_normal.z = ground_sample.normal[2];
        wheel->ground_friction = ground_sample.friction;
        wheel->ground_rolling_resistance = ground_sample.rolling_resistance;
    }
}



static void SimWheelUpdateDriverAssists(tCar *car, int index)
{
    tWheel *wheel = &(car->wheel[index]);
    tdble sx = wheel->sx;

    if (car->features & FEAT_TCLINSIMU)
    {
        tEngine *engine = &(car->engine);
        if (sx < -car->TCL_SlipScale)
        {
            wheel->brake.TCL = -sx;
            engine->TCL = (tdble) MIN(car->TCL_AccelScale * wheel->brake.TCL, engine->TCL);
        }
    }

    if (car->features & FEAT_ABSINSIMU)
    {
        if (sx > car->ABS_SlipScale)
        {
            wheel->brake.ABS = (tdble) MAX(0.0, MIN(1.0, 1 - car->ABS_BrakeScale * sx));
        }
        else
        {
            wheel->brake.ABS = 1.0f;
        }
    }
}

void SimWheelUpdateDriverAssistsForCoupled(tCar *car, int index)
{
    SimWheelUpdateDriverAssists(car, index);
}



void SimWheelUpdateRotation(tCar *car)
{
    int i;
    tWheel *wheel;
    tdble deltan;
    tdble cosaz, sinaz;

    tdble maxslip = 0.0;

    for (i = 0; i < 4; i++)
    {
        wheel = &(car->wheel[i]);
        /*calculate gyroscopic forces*/
        cosaz = cos(wheel->relPos.az);
        sinaz = sin(wheel->relPos.az);

        if( (i == 0) || (i == 1) )
        {
            wheel->torques.y = wheel->torques.x * sinaz;
            wheel->torques.x = wheel->torques.x * cosaz;
        }
        else
        {
            wheel->torques.x = wheel->torques.y =0.0;
        }

        deltan = -(wheel->in.spinVel - wheel->prespinVel) * wheel->I / SimDeltaTime;
        wheel->torques.x -= deltan * wheel->cosax *sinaz;
        wheel->torques.y += deltan * wheel->cosax *cosaz;
        wheel->torques.z = deltan * wheel->sinax;
        /*update rotation*/
        wheel->spinVel = wheel->in.spinVel;

        wheel->prespinVel = wheel->spinVel;

        wheel->relPos.ay += wheel->spinVel * SimDeltaTime;
        FLOAT_NORM_PI_PI(wheel->relPos.ay);
        car->carElt->_wheelSpinVel(i) = wheel->spinVel;

        // Option TCL ...
        if (car->features & FEAT_TCLINSIMU)
        {
            if (maxslip < wheel->brake.TCL)
                maxslip = wheel->brake.TCL;
        }
        // ... Option TCL
    }

    // Option TCL ...
    if (maxslip > 0.0)
    {
        for (i = 0; i < 4; i++)
        {
            wheel = &(car->wheel[i]);
            if (wheel->brake.TCL != maxslip)
                wheel->brake.TCL = 0.0;
        }
    }
    // ... Option TCL
}

void SimUpdateFreeWheels(tCar *car, int axlenb)
{
    int i;
    tWheel *wheel;
    tdble BrTq;		// brake torque
    tdble ndot;		// rotation acceleration
    tdble I;

    for (i = axlenb * 2; i < axlenb * 2 + 2; i++)
    {
        wheel = &(car->wheel[i]);

        I = wheel->I + car->axle[axlenb].I / 2.0f;

        ndot = SimDeltaTime * wheel->spinTq / I;
        wheel->spinVel -= ndot;

        BrTq = (tdble)(- SIGN(wheel->spinVel) * wheel->brake.Tq);
        ndot = SimDeltaTime * BrTq / I;

        if (fabs(ndot) > fabs(wheel->spinVel))
        {
            ndot = -wheel->spinVel;
        }

        wheel->spinVel += ndot;
        wheel->in.spinVel = wheel->spinVel;
    }
}
