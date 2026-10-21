/***************************************************************************

    file                 : aero.cpp
    created              : Sun Mar 19 00:04:50 CET 2000
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



#include "sim.h"

tdble rho = SimAirDensity; /* air density, prepare for variable environment */

// Christos plausibility check ...
tdble Max_Cl_given_Cd (tdble Cd)
{
    // if Cd = 1, then all air hitting the surface is stopped.
    // In any case, horizontal speed of air particles is given by
    tdble ux = 1 - Cd;

    // We assume no energy lost and thus can calculate the maximum
    // possible vertical speed imparted to the paticles
    tdble uy = sqrt(1 - ux*ux);

    // So now Cl is just the imparted speed
    return uy;
}

tdble Max_SCl_given_Cd (tdble Cd, tdble A)
{
    tdble Cl = Max_Cl_given_Cd (Cd);
    return A * Cl * rho / 2.0f;
}

tdble MaximumLiftGivenDrag (tdble drag, tdble A)
{
    // We know the drag, C/2 \rho A.
    // We must calculate the drag coefficient
    tdble Cd = (drag / A) * 2.0f / rho;
    return Max_SCl_given_Cd (Cd, A);
}
// ... Christos plausibility check

void
SimAeroConfig(tCar *car)
{
    void *hdle = car->params;
    tdble Cx, FrntArea;
    // New style parameters:
    // To be able to check the total of clift to be in the defined range
    // min <= 2 * (car->aero.Clift[0] + car->aero.Clift[1]) <= max
    // with the default procedures of SD we define Clift with
    // the two new style parameters CliftTotal and CliftBias.
    // If CliftTotal and/or CliftBias are not defined, the corresponding
    // value is calculated from the old style parameters
    tdble CliftTotal;
    tdble CliftBias;

    float aero_factor = car->options->aero_factor;

    Cx       = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_CX, (char*)NULL, 0.4f);
    FrntArea = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_FRNTAREA, (char*)NULL, 2.5f);
    car->aero.whole_vehicle_reference = strcmp(GfParmGetStr(hdle, SECT_AERODYNAMICS, "drag coefficient scope", "body"), "whole vehicle reference") == 0;
    car->aero.reference_drag_area = Cx * FrntArea;
    car->aero.reference_wing_drag_area = GfParmGetNum(hdle, SECT_AERODYNAMICS, "reference wing drag area", NULL, 0.0);
    car->aero.floor_lift_area = GfParmGetNum(hdle, "Formula Underfloor", "lift area", NULL, 0.0);
    car->aero.floor_induced_drag_ratio = GfParmGetNum(hdle, "Formula Underfloor", "induced drag ratio", NULL, 0.13);
    car->aero.floor_choke_height = GfParmGetNum(hdle, "Formula Underfloor", "choke height", NULL, 0.014);
    car->aero.floor_optimal_height = GfParmGetNum(hdle, "Formula Underfloor", "optimal height", NULL, 0.052);
    car->aero.floor_high_height = GfParmGetNum(hdle, "Formula Underfloor", "high height", NULL, 0.165);
    car->aero.floor_front_clearance = GfParmGetNum(hdle, "Formula Underfloor", "front clearance", NULL, 0.03);
    car->aero.floor_rear_clearance = GfParmGetNum(hdle, "Formula Underfloor", "rear clearance", NULL, 0.045);
    car->aero.floor_optimal_rake = GfParmGetNum(hdle, "Formula Underfloor", "optimal rake", "deg", 0.6);
    car->aero.floor_rake_window = GfParmGetNum(hdle, "Formula Underfloor", "rake window", "deg", 3.25);
    car->aero.floor_seal_roll = GfParmGetNum(hdle, "Formula Underfloor", "seal roll", NULL, 0.15);
    car->aero.floor_position.x = GfParmGetNum(hdle, "Formula Underfloor", "xpos", NULL, 0.0);
    car->aero.floor_position.z = GfParmGetNum(hdle, "Formula Underfloor", "zpos", NULL, 0.0);
    car->aero.floor_downforce = car->aero.floor_drag = 0.0;
    car->aero.soft_maximum_load_ratio = GfParmGetNum(hdle, "Formula Underfloor", "soft maximum load ratio", NULL, 0.0);
    car->aero.hard_maximum_load_ratio = GfParmGetNum(hdle, "Formula Underfloor", "hard maximum load ratio", NULL, 0.0);
    car->aero.hard_maximum_downforce = GfParmGetNum(hdle, "Formula Underfloor", "hard maximum downforce", NULL, 0.0);
    if (!(car->aero.floor_choke_height > 0.0 && car->aero.floor_optimal_height > car->aero.floor_choke_height &&
          car->aero.floor_high_height > car->aero.floor_optimal_height && car->aero.floor_rake_window > 0.0 &&
          car->aero.floor_seal_roll > 0.0))
    {
        GfLogError("Invalid Formula underfloor height or angular range; underfloor disabled\n");
        car->aero.floor_lift_area = 0.0;
    }

    // Try to get the old style parameters
    car->aero.Clift[0] = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_FCL, (char*)NULL, 0.0f);
    car->aero.Clift[1] = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_RCL, (char*)NULL, 0.0f);

    // Calculate CliftTotal from old style parameters
    CliftTotal = 2 * (car->aero.Clift[0] + car->aero.Clift[1]);
    // Use calculated value if no new style definition found
    CliftTotal = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_CL, (char*)NULL, CliftTotal);

    // Calculate CliftBias from old style parameters
    CliftBias = CliftTotal != 0.0f ? 2 * car->aero.Clift[0] / CliftTotal : 0.5f;
    // Use calculated value if no new style definition found
    CliftBias = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_CLBIAS, (char*)NULL, CliftBias);

    // Calculate front/rear Clift based on new style parameters
    car->aero.Clift[0] = 0.5f * CliftBias * CliftTotal;
    car->aero.Clift[1] = 0.5f * (CliftTotal - 2 * car->aero.Clift[0]);

    car->aero.Clift[0] *= aero_factor; // 4.0f;
    car->aero.Clift[1] *= aero_factor; // 4.0f;

    car->aero.CdBody = 0.645f * Cx * FrntArea;
    if (car->aero.whole_vehicle_reference)
        car->aero.CdBody = 0.5 * SimAirDensity * MAX(0.0, car->aero.reference_drag_area - car->aero.reference_wing_drag_area);
    car->aero.Cd = car->aero.CdBody;

    float max_lift = MaximumLiftGivenDrag (0.5f * rho * Cx * FrntArea, FrntArea);
    float current_lift = 2.0f * (car->aero.Clift[0] + car->aero.Clift[1]);

    if (current_lift > max_lift)
    {
        if (car->features & FEAT_LIMITEDGROUNDEFFECT)
        {
            fprintf (stderr, "\n\nError: car %s, driver %s: lift coefficients (%f, %f), generate a lift of %f, while maximum theoretical value is %f -> CLift reduced\n\n",
                 car->carElt->_carName,
                 car->carElt->_name,
                 car->aero.Clift[0],
                 car->aero.Clift[1],
                 current_lift,
                 max_lift);
            car->aero.Clift[0] *= max_lift/current_lift;
            car->aero.Clift[1] *= max_lift/current_lift;
        }
        else
        {
/*
            fprintf (stderr, "\n\nWarning: car %s, driver %s: lift coefficients (%f, %f), generate a lift of %f, while maximum theoretical value is %f\n\n",
                 car->carElt->_carName,
                 car->carElt->_name,
                 car->aero.Clift[0],
                 car->aero.Clift[1],
                 current_lift,
                 max_lift);
*/
        }
    }
    else
    {
/*
        fprintf (stderr, "\n\nInfo: car %s, driver %s: lift coefficients (%f, %f), generate a lift of %f, while maximum theoretical value is %f\n\n",
                 car->carElt->_carName,
                 car->carElt->_name,
                 car->aero.Clift[0],
                 car->aero.Clift[1],
                 current_lift,
                 max_lift);
*/
    }
}


void
SimAeroUpdate(tCar *car, tSituation *s)
{
    tdble	hm;
    int		i;
    tCar	*otherCar;
    tdble	x, y;
    tdble	yaw, otherYaw, airSpeed, tmpas, spdang, tmpsdpang, dyaw;
    tdble	dragK = 1.0;

    x = car->DynGCg.pos.x;
    y = car->DynGCg.pos.y;
    yaw = car->DynGCg.pos.az;
    airSpeed = car->DynGC.vel.x;
    spdang = atan2(car->DynGCg.vel.y, car->DynGCg.vel.x);

    if (airSpeed > 10.0)
    {
        for (i = 0; i < s->_ncars; i++)
        {
            if (i == car->carElt->index)
            {
                continue;
            }

            otherCar = &(SimCarTable[i]);
            otherYaw = otherCar->DynGCg.pos.az;
            tmpsdpang = spdang - atan2(y - otherCar->DynGCg.pos.y, x - otherCar->DynGCg.pos.x);
            FLOAT_NORM_PI_PI(tmpsdpang);
            dyaw = yaw - otherYaw;
            FLOAT_NORM_PI_PI(dyaw);

            if ((otherCar->DynGC.vel.x > 10.0) &&
                (fabs(dyaw) < 0.1396))
            {
                if (fabs(tmpsdpang) > 2.9671)
                {	    /* 10 degrees */
                    /* behind another car */
                    tmpas = (tdble) (1.0 - exp(- 2.0 * DIST(x, y, otherCar->DynGCg.pos.x, otherCar->DynGCg.pos.y) /
                                      (otherCar->aero.Cd * otherCar->DynGC.vel.x)));
                    if (tmpas < dragK)
                    {
                        dragK = tmpas;
                    }
                }
                else if (fabs(tmpsdpang) < 0.1396)
                {	    /* 8 degrees */
                    /* before another car [not sure how much the drag should be reduced in this case. In no case it should be lowered more than 50% I think. - Christos] */
                    tmpas = (tdble) (1.0 - 0.5f * exp(- 8.0 * DIST(x, y, otherCar->DynGCg.pos.x, otherCar->DynGCg.pos.y) / (car->aero.Cd * car->DynGC.vel.x)));

                    if (tmpas < dragK)
                    {
                        dragK = tmpas;
                    }
                }
            }
        }
    }

    car->airSpeed2 = airSpeed * airSpeed;
    tdble v2 = car->airSpeed2;

    // simulate ground effect drop off caused by non-frontal airflow (diffusor stops working etc.)

    // Never used : remove ?
    //tdble speed = sqrt(car->DynGC.vel.x*car->DynGC.vel.x + car->DynGC.vel.y*car->DynGC.vel.y);
    //tdble cosa = 1.0f;

    if (car->options->aero_damage)
        car->aero.drag = (tdble) (-SIGN(car->DynGC.vel.x) * car->aero.CdBody * v2 * (1.0f + (tdble)car->dammage / 10000.0f) * dragK * dragK);
    else
        car->aero.drag = (tdble) (-SIGN(car->DynGC.vel.x) * car->aero.CdBody * v2 * dragK * dragK);

    hm = 1.5f * (car->wheel[0].rideHeight + car->wheel[1].rideHeight + car->wheel[2].rideHeight + car->wheel[3].rideHeight);
    hm = hm*hm;
    hm = hm*hm;
    hm = 2 * exp(-3.0f*hm);
    car->aero.lift[0] = - car->aero.Clift[0] * v2 * hm;
    car->aero.lift[1] = - car->aero.Clift[1] * v2 * hm;
    if (car->aero.whole_vehicle_reference)
    {
        car->aero.CdBody = 0.5 * SimAirDensity * MAX(0.0, car->aero.reference_drag_area - car->aero.reference_wing_drag_area);
        car->aero.drag = -SIGN(airSpeed) * car->aero.CdBody * v2 * dragK * dragK *
                         (car->options->aero_damage ? 1.0 + car->dammage / 10000.0 : 1.0);
    }
}

static const char *WingSect[2] = {SECT_FRNTWING, SECT_REARWING};

static double formulaSmoothStep(double value)
{
    value = MAX(0.0, MIN(1.0, value));
    return value * value * (3.0 - 2.0 * value);
}

static double formulaFloorHeightEfficiency(double height, const tAero& aerodynamics)
{
    if (height <= 0.0)
        return 0.0;
    if (height < aerodynamics.floor_choke_height)
        return 0.2 * formulaSmoothStep(height / aerodynamics.floor_choke_height);
    if (height < aerodynamics.floor_optimal_height)
        return 0.2 + 0.8 * formulaSmoothStep((height - aerodynamics.floor_choke_height) /
            (aerodynamics.floor_optimal_height - aerodynamics.floor_choke_height));
    return 1.0 - formulaSmoothStep((height - aerodynamics.floor_optimal_height) /
        (aerodynamics.floor_high_height - aerodynamics.floor_optimal_height));
}

static void updateFormulaUnderfloor(tCar* car)
{
    tAero& aerodynamics = car->aero;
    if (car->formula_vertical_authority_diagnostic
        && car->formula_vertical_authority_every_step)
    {
        GfLogInfo("Formula vertical authority aero read: time=%.6f ride=%.9f,%.9f,%.9f,%.9f packers=%.9f,%.9f,%.9f,%.9f dyn_pos_z=%.9f state_z=%.9f\n",
            car->formula_vertical_time,
            car->wheel[0].rideHeight, car->wheel[1].rideHeight,
            car->wheel[2].rideHeight, car->wheel[3].rideHeight,
            car->wheel[0].susp.spring.packers, car->wheel[1].susp.spring.packers,
            car->wheel[2].susp.spring.packers, car->wheel[3].susp.spring.packers,
            car->DynGCg.pos.z, car->formula_vertical_state.body_position[0]);
    }
    aerodynamics.floor_downforce = aerodynamics.floor_drag = 0.0;
    if (aerodynamics.floor_lift_area <= 0.0 || car->DynGC.vel.x <= 0.0)
        return;
    const double front_height = 0.5 * (car->wheel[0].rideHeight + car->wheel[1].rideHeight) - 0.05 + aerodynamics.floor_front_clearance;
    const double rear_height = 0.5 * (car->wheel[2].rideHeight + car->wheel[3].rideHeight) - 0.05 + aerodynamics.floor_rear_clearance;
    const double ground_roll = atan2((car->wheel[1].rideHeight + car->wheel[3].rideHeight -
                                     car->wheel[0].rideHeight - car->wheel[2].rideHeight) * 0.5, MAX(0.1, fabs(car->wheeltrack)));
    const double rake_degrees = atan2(rear_height - front_height, MAX(0.1, car->wheelbase)) * 180.0 / PI;
    const double height_factor = sqrt(formulaFloorHeightEfficiency(front_height, aerodynamics) *
                                      formulaFloorHeightEfficiency(rear_height, aerodynamics));
    const double rake_factor = 1.0 - formulaSmoothStep(fabs(rake_degrees - aerodynamics.floor_optimal_rake) / aerodynamics.floor_rake_window);
    const double seal_factor = 1.0 - formulaSmoothStep(fabs(ground_roll) / aerodynamics.floor_seal_roll);
    const double forward_speed = MAX(0.0, car->DynGC.vel.x);
    const double yaw_factor = pow(forward_speed / MAX(0.001, hypot(forward_speed, car->DynGC.vel.y)), 1.2);
    aerodynamics.floor_downforce = -0.5 * SimAirDensity * car->airSpeed2 * aerodynamics.floor_lift_area *
        height_factor * rake_factor * seal_factor * yaw_factor * car->options->aero_factor;
    aerodynamics.floor_drag = -fabs(aerodynamics.floor_downforce) * aerodynamics.floor_induced_drag_ratio;
    const double raw_downforce = -(car->wing[0].forces.z + car->wing[1].forces.z + aerodynamics.floor_downforce);
    const double vehicle_weight = (car->mass + car->fuel * car->fuelMass) * G;
    const double hard_limit = MIN(aerodynamics.hard_maximum_downforce, aerodynamics.hard_maximum_load_ratio * vehicle_weight);
    const double soft_limit = MIN(hard_limit, aerodynamics.soft_maximum_load_ratio * vehicle_weight);
    if (hard_limit > 0.0 && soft_limit >= 0.0 && raw_downforce > soft_limit)
    {
        const double limit_span = hard_limit - soft_limit;
        const double limited_downforce = limit_span > 0.0 ?
            soft_limit + limit_span * (1.0 - exp(-(raw_downforce - soft_limit) / limit_span)) : hard_limit;
        const double limit_factor = MIN(1.0, limited_downforce / raw_downforce);
        aerodynamics.floor_downforce *= limit_factor;
        aerodynamics.floor_drag *= limit_factor;
        for (int wing_index = 0; wing_index < 2; ++wing_index)
        {
            car->wing[wing_index].forces.z *= limit_factor;
            car->wing[wing_index].forces.x *= limit_factor;
        }
    }
}

tdble F(tWing* wing)
{
    return 1 - exp( pow(-(wing->a / wing->b),wing->c));
}

tdble CliftFromAoA(tWing* wing)
{
    tdble angle = (tdble) (wing->angle * 180/PI);
    //fprintf(stderr,"wing->angle: %g rad = angle: %g deg\n",wing->angle,angle);

    wing->Kz_org = 4.0f * wing->Kx;

    if (angle <= wing->AoAatMax)
    {
        wing->a = wing->f * (angle + wing->AoAOffset);
        //fprintf(stderr,"a: %g\n",wing->a);
        double s = sin(wing->a/180.0*PI);
        //fprintf(stderr,"s: %g\n",s);
        return (tdble)(s * s * (wing->CliftMax + wing->d) - wing->d);
    }
    else
    {
        wing->a = (angle - wing->AoAatMax - 90.0f);
        //fprintf(stderr,"a: %g F(a): %g\n",wing->a,F(wing));
        return (tdble)((wing->CliftMax - F(wing) * (wing->CliftMax - wing->CliftAsymp)) * wing->Kx);
    }
}

void
SimWingConfig(tCar *car, int index)
{
    void   *hdle = car->params;
    tWing  *wing = &(car->wing[index]);
    tdble area;
    tCarSetupItem *setupAngle = &(car->carElt->setup.wingAngle[index]);

    area              = GfParmGetNum(hdle, WingSect[index], PRM_WINGAREA, (char*)NULL, 0);
    setupAngle->desired_value = setupAngle->min = setupAngle->max = 0.0;
    GfParmGetNumWithLimits(hdle, WingSect[index], PRM_WINGANGLE, (char*)NULL, &(setupAngle->desired_value), &(setupAngle->min), &(setupAngle->max));
    setupAngle->changed = true;
    setupAngle->stepsize = (tdble) DEG2RAD(0.1);
    wing->staticPos.x = GfParmGetNum(hdle, WingSect[index], PRM_XPOS, (char*)NULL, 0);
    wing->staticPos.z = GfParmGetNum(hdle, WingSect[index], PRM_ZPOS, (char*)NULL, 0);
    wing->staticPos.y = 0.0;

//>>> simuv4
    const char * w = GfParmGetStr(hdle, WingSect[index], PRM_WINGTYPE, "FLAT");

    wing->WingType = 0; // Default if nothing is contained in the wing section
    wing->polar_point_count = 0;
    wing->polar_area = area;

    if (area == 0)
      wing->WingType = -1;
    else if (strncmp(w,"FLAT",4) == 0)
      wing->WingType = 0;
    else if (strncmp(w,"PROFILE",7) == 0)
      wing->WingType = 1;
    else if (strncmp(w,"THIN",4) == 0)
      wing->WingType = 2;
    else if (strcmp(w, "TABULATED") == 0)
    {
        wing->WingType = 3;
        for (int point_index = 0; point_index < 16; ++point_index)
        {
            char section_path[256];
            snprintf(section_path, sizeof(section_path), "%s/Polar/%d", WingSect[index], point_index + 1);
            const tdble angle = GfParmGetNum(hdle, section_path, "angle", "deg", -1000.0);
            if (angle == -1000.0)
                break;
            if (point_index > 0 && angle <= wing->polar_angles[point_index - 1])
                break;
            wing->polar_angles[point_index] = angle;
            wing->polar_drag_coefficients[point_index] = MAX(0.0, GfParmGetNum(hdle, section_path, "drag coefficient", NULL, 0.0));
            wing->polar_lift_coefficients[point_index] = MAX(0.0, GfParmGetNum(hdle, section_path, "downforce coefficient", NULL, 0.0));
            ++wing->polar_point_count;
        }
        if (wing->polar_point_count < 2)
        {
            GfLogError("Tabulated wing requires at least two ascending polar points\n");
            wing->WingType = -1;
        }
    }
    // ...

    if (wing->WingType == 1)
    {
        //fprintf(stderr,"index: %d\n",index);
        //fprintf(stderr,"WingType: %d\n",wing->WingType);

        /* [deg] Angle of Attack at the maximum of coefficient of lift */
        wing->AoAatMax = GfParmGetNum(hdle, WingSect[index], PRM_AOAATMAX, (char*) "deg", 90);
        //fprintf(stderr,"AoAatMax: %g\n",wing->AoAatMax);

        /* [deg] Angle of Attack at coefficient of lift = 0 (-30 < AoAatZero < 0) */
        wing->AoAatZero = GfParmGetNum(hdle, WingSect[index], PRM_AOAATZERO, (char*) "deg", 0);
        //fprintf(stderr,"AoAatZero: %g\n",wing->AoAatZero);
        wing->AoAatZRad = (tdble) (wing->AoAatZero/180*PI);

        /* [deg] Offset for Angle of Attack */
        wing->AoAOffset = GfParmGetNum(hdle, WingSect[index], PRM_AOAOFFSET, (char*) "deg", 0);
        //fprintf(stderr,"AoAOffset: %g\n",wing->AoAOffset);

        /* Maximum of coefficient of lift (0 < CliftMax < 4) */
        wing->CliftMax = GfParmGetNum(hdle, WingSect[index], PRM_CLMAX, (char*)NULL, 4);
        //fprintf(stderr,"CliftMax: %g\n",wing->CliftMax);

        /* Coefficient of lift at Angle of Attack = 0 */
        wing->CliftZero = GfParmGetNum(hdle, WingSect[index], PRM_CLATZERO, (char*)NULL, 0);
        //fprintf(stderr,"CliftZero: %g\n",wing->CliftZero);

        /* Asymptotic coefficient of lift at large Angle of Attack */
        wing->CliftAsymp = GfParmGetNum(hdle, WingSect[index], PRM_CLASYMP, (char*)NULL, wing->CliftMax);
        //fprintf(stderr,"CliftAsymp: %g\n",wing->CliftAsymp);

        /* Delay of decreasing */
        wing->b = GfParmGetNum(hdle, WingSect[index], PRM_DELAYDECREASE, (char*)NULL, 20);
        //fprintf(stderr,"b: %g\n",wing->b);

        /* Curvature of start of decreasing */
        wing->c = GfParmGetNum(hdle, WingSect[index], PRM_CURVEDECREASE, (char*)NULL, 2);
        //fprintf(stderr,"c: %g\n",wing->c);

        /* Scale factor for angle */
        wing->f = (tdble) (90.0 / (wing->AoAatMax + wing->AoAOffset));
        //fprintf(stderr,"f: %g\n",wing->f);
        double phi = wing->f * (wing->AoAOffset);
        //fprintf(stderr,"phi: %g deg\n",phi);
        phi *= PI / 180;
        //fprintf(stderr,"phi: %g rad\n",phi);
        double sinphi = sin(phi);
        //fprintf(stderr,"sinphi: %g\n",sinphi);
        double sinphi2 = sinphi * sinphi;

        /* Scale at AoA = 0 */
        wing->d = (tdble) (1.8f * (sinphi2 * wing->CliftMax - wing->CliftZero));
        //fprintf(stderr,"d: %g\n",wing->d);
    }
    else if (wing->WingType == 2)
    {
        wing->AoAatZero = GfParmGetNum(hdle, WingSect[index], PRM_AOAATZERO, (char*)NULL, 0);
            wing->AoAatZero = MAX(MIN(wing->AoAatZero, 0), (tdble) -PI_6);
        wing->AoStall = GfParmGetNum(hdle, WingSect[index], PRM_ANGLEOFSTALL, (char*)NULL, (tdble)(PI_6*0.5));
            wing->AoStall = MAX(MIN(wing->AoStall, (tdble) PI_4), 0.017453293f);
        wing->Stallw = GfParmGetNum(hdle, WingSect[index], PRM_STALLWIDTH, (char*)NULL, 0.034906585f);
            wing->Stallw = MAX(MIN(wing->Stallw, wing->AoStall), 0.017453293f);
        wing->AR = GfParmGetNum(hdle, WingSect[index], PRM_ASPECTRATIO, (char*)NULL, 0);
    }

    wing->Kx = -rho * area;

    if (wing->WingType == 0)
    {
        wing->Kz = car->options->aero_factor * wing->Kx;
    }
    else if (wing->WingType == 1)
    {
        wing->Kz = CliftFromAoA(wing) * car->options->aero_factor * wing->Kx;
    }
    else if (wing->WingType == 2)
    {
        if (wing->AR > 0.001)
            wing->Kz1 = (tdble) (2 * PI * wing->AR / (wing->AR + 2));
        else
            wing->Kz1 = (tdble)(2 * PI);

        wing->Kx = (tdble) (0.5 * rho * area);
        wing->Kz = car->options->aero_factor * wing->Kx;
        wing->Kz2 = 1.05f;
        wing->Kz3 = 0.05f;
        wing->Kx1 = 0.6f;
        wing->Kx2 = 0.006f;
        wing->Kx3 = 1.0f;
        wing->Kx4 = 0.9f;
    }
}

void
SimWingReConfig(tCar *car, int index)
{/* called by SimCarReConfig() in car.cpp */
    tWing  *wing = &(car->wing[index]);
    tCarSetupItem *angle = &(car->carElt->setup.wingAngle[index]);

    if (angle->changed)
    {
        wing->angle = MIN(angle->max,MAX(angle->min,angle->desired_value));
        angle->value = wing->angle;

        if (wing->WingType == 0)
        {
            if (index==1)
            {
                car->aero.Cd = car->aero.CdBody - wing->Kx*sin(wing->angle);
            }
        }
        else if (wing->WingType == 1)
        {
            tWing  *otherwing = &(car->wing[1-index]);
            car->aero.Cd = (tdble)(car->aero.CdBody - wing->Kx*sin(wing->angle - wing->AoAatZRad) - otherwing->Kx*sin(otherwing->angle - otherwing->AoAatZRad));
        }

        angle->changed = false;
    }
}

void
SimWingUpdate(tCar *car, int index, tSituation* s)
{
    tWing  *wing = &(car->wing[index]);

    /* return with 0 if no wing present */
    if (wing->WingType == -1)
    {
        wing->forces.x = wing->forces.z = 0.0f;
        return;
    }

    if (index == 1)
    {
        // Check wing angle controller
        if (car->ctrl->wingControlMode == 2)
            // Update wing angle
            wing->angle = car->ctrl->wingRearCmd;
        car->aero.Cd = car->aero.CdBody - wing->Kx*sin(wing->angle);
    }
    else
        // Check wing angle controller
        if (car->ctrl->wingControlMode == 2)
            // Update wing angle
            wing->angle = car->ctrl->wingFrontCmd;

    tdble vt2 = car->airSpeed2;
    // compute angle of attack
    tdble aoa = atan2(car->DynGC.vel.z, car->DynGC.vel.x) + car->DynGCg.pos.ay;

    aoa += wing->angle;
    if (wing->WingType == 3)
    {
        const tdble attack_degrees = aoa * 180.0 / PI;
        int upper_point = 1;
        while (upper_point < wing->polar_point_count - 1 && attack_degrees > wing->polar_angles[upper_point])
            ++upper_point;
        const int lower_point = upper_point - 1;
        const tdble interpolation = MAX(0.0, MIN(1.0, (attack_degrees - wing->polar_angles[lower_point]) /
                                                (wing->polar_angles[upper_point] - wing->polar_angles[lower_point])));
        tdble drag_coefficient = wing->polar_drag_coefficients[lower_point] + interpolation *
            (wing->polar_drag_coefficients[upper_point] - wing->polar_drag_coefficients[lower_point]);
        tdble lift_coefficient = wing->polar_lift_coefficients[lower_point] + interpolation *
            (wing->polar_lift_coefficients[upper_point] - wing->polar_lift_coefficients[lower_point]);
        const double distance_outside_polar = MAX(wing->polar_angles[0] - attack_degrees,
            attack_degrees - wing->polar_angles[wing->polar_point_count - 1]);
        if (distance_outside_polar > 0.0)
        {
            const double stalled_fraction = formulaSmoothStep(distance_outside_polar / 35.0);
            if (attack_degrees < wing->polar_angles[0])
            {
                const double lift_slope = MAX(0.0, (wing->polar_lift_coefficients[1] - wing->polar_lift_coefficients[0]) /
                    (wing->polar_angles[1] - wing->polar_angles[0]));
                lift_coefficient = MAX(0.0, lift_coefficient - lift_slope * distance_outside_polar);
            }
            lift_coefficient *= 1.0 - stalled_fraction;
            drag_coefficient += stalled_fraction * (1.2 - drag_coefficient);
        }
        const tdble dynamic_pressure_area = 0.5 * SimAirDensity * vt2 * wing->polar_area;
        wing->forces.x = -SIGN(car->DynGC.vel.x) * dynamic_pressure_area * drag_coefficient *
            (car->options->aero_damage ? 1.0 + car->dammage / 10000.0 : 1.0);
        wing->forces.z = car->DynGC.vel.x > 0.0 && fabs(aoa) < PI_2 ?
            -dynamic_pressure_area * lift_coefficient * car->options->aero_factor : 0.0;
        if (index == 1)
        {
            updateFormulaUnderfloor(car);
            car->aero.Cd = car->aero.CdBody + (vt2 > 0.0 ?
                (fabs(car->wing[0].forces.x) + fabs(wing->forces.x) + fabs(car->aero.floor_drag)) / vt2 : 0.0);
        }
        return;
    }

    if (wing->WingType == 2) //thin wing works for every direction
    {
        tdble x;
        while (aoa > PI) aoa -= (tdble) (2 * PI);
        while (aoa < -PI) aoa += (tdble) (2 * PI);
        /* first calculate coefficients */
        if (aoa > PI_2)
        {
        if (aoa > PI - wing->AoStall) wing->forces.x = (tdble) (wing->Kx1 * (PI - aoa) * (PI - aoa) + wing->Kx2);
        else wing->forces.x = (tdble) (wing->Kx3 - wing->Kx4 * cos(2*aoa));
        if (aoa > PI - wing->AoStall + wing->Stallw)
            {x = (tdble)0.0;}
        else
        {
            x = (tdble) (aoa - PI + wing->AoStall - wing->Stallw);
            x = (tdble) (x * x / (x * x + wing->Stallw * wing->Stallw));
        }
        wing->forces.z = (tdble) (-(1-x) * wing->Kz1 * (aoa - PI + wing->AoAatZero) - x * (wing->Kz2 * sin(2*aoa) + wing->Kz3));
        }
        else if (aoa > 0)
        {
        if (aoa < wing->AoStall)
            wing->forces.x = wing->Kx1 * aoa * aoa + wing->Kx2;
        else
            wing->forces.x = wing->Kx3 - wing->Kx4 * cos(2*aoa);

        if (aoa < wing->AoStall - wing->Stallw)
            {x = (tdble)0.0;}
        else
        {
            x = aoa - wing->AoStall + wing->Stallw;
            x = x * x / (x * x + wing->Stallw * wing->Stallw);
        }

        wing->forces.z = -(1-x) * wing->Kz1 * (aoa - wing->AoAatZero) - x * (wing->Kz2 * sin(2*aoa) + wing->Kz3);
        }
        else if (aoa > -PI_2)
        {
        if (aoa > -wing->AoStall)
            wing->forces.x = wing->Kx1 * aoa * aoa + wing->Kx2;
        else
            wing->forces.x = wing->Kx3 - wing->Kx4 * cos(2*aoa);

        if (aoa > -wing->AoStall + wing->Stallw)
            {x = (tdble)0.0;}
        else
        {
            x = aoa + wing->AoStall - wing->Stallw;
            x = x * x / (x * x + wing->Stallw * wing->Stallw);
        }

        wing->forces.z = -(1-x) * wing->Kz1 * (aoa - wing->AoAatZero) - x * (wing->Kz2 * sin(2*aoa) - wing->Kz3);
        }
        else
        {
        if (aoa < wing->AoStall - PI)
            wing->forces.x = (tdble) (wing->Kx1 * (PI + aoa) * (PI + aoa) + wing->Kx2);
        else
            wing->forces.x = wing->Kx3 - wing->Kx4 * cos(2*aoa);

        if (aoa < wing->AoStall - wing->Stallw - PI)
            {x = (tdble)0.0;}
        else
        {
            x = (tdble) (aoa - wing->AoStall + wing->Stallw + PI);
            x = x * x / (x * x + wing->Stallw * wing->Stallw);
        }

        wing->forces.z = (tdble) (-(1-x) * wing->Kz1 * (aoa + wing->AoAatZero + PI) - x * (wing->Kz2 * sin(2*aoa) - wing->Kz3));
        }

        /* add induced drag */
        if (wing->AR > 0.001)
        {
        if (wing->forces.x > 0.0)
            wing->forces.x += (tdble) (wing->forces.z * wing->forces.z / (wing->AR * 2.8274)); //0.9*PI
        else
            wing->forces.x -= (tdble) (wing->forces.z * wing->forces.z / (wing->AR * 2.8274));
        }

        /* then multiply with 0.5*rho*area and the square of velocity */
        wing->forces.x *= (tdble)(- car->DynGC.vel.x * fabs(car->DynGC.vel.x) * wing->Kx * (1.0f + (tdble)car->dammage / 10000.0));
        wing->forces.z *= wing->Kx * vt2;
    }
    else if (car->DynGC.vel.x > 0.0f)
    {
        if (wing->WingType == 0)
        {
            // the sinus of the angle of attack
            tdble sinaoa = sin(aoa);

            // make drag always negative and have a minimal angle of attack
            wing->forces.x = (tdble) (wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0) * MAX(fabs(sinaoa), 0.02));

            // If angle of attack is too large, no downforce, only drag
            if (fabs(aoa) > PI_2)
            {
                wing->forces.z = 0.0;
            }
            else
            {
                // 0 deg -> 30 deg as it was in simuV2.1
                if (fabs(aoa) < PI_6)
                {
                    sinaoa = sin(aoa);
                }
                else // 30 deg -> 90 deg smoothly reduced downforce
                {
                    sinaoa = (tdble) (0.25f * (1.0f - ((aoa-PI_3)/PI_6)*((aoa-PI_3)/PI_6)*((aoa-PI_3)/PI_6)));
                }
                wing->forces.z = (tdble) MIN(0.0,wing->Kz * vt2 * sinaoa);
            }
        }
        else if (wing->WingType == 1)
        {
            wing->forces.x = (tdble) (wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0) * MAX(fabs(sin(aoa - wing->AoAatZRad)), 0.02));
            wing->forces.z = (tdble) MIN(0.0,wing->Kx* vt2 * CliftFromAoA(wing));
            // fprintf(stderr,"%d fz: %g (%g)\n",index,wing->forces.z,CliftFromAoA(wing));
        }
    }
    else
        wing->forces.x = wing->forces.z = 0.0f;
}
