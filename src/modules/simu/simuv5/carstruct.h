/***************************************************************************

    file                 : carstruct.h
    created              : Sun Mar 19 00:06:07 CET 2000
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

#ifndef _CAR__H_
#define _CAR__H_

#include <plib/sg.h>
#include <SOLID/solid.h>

#include "aero.h"
#include "susp.h"
#include "brake.h"
#include "axle.h"
#include "steer.h"
#include "wheel.h"
#include "transmission.h"
#include "engine.h"
#include "simulationOptions.h"
#include "tire_model_configuration.h"
#include "tire_vertical_solver.h"

typedef struct
{
    /* driver's interface */
    tCarCtrl	*ctrl;
    void	*params;
    tCarElt	*carElt;

    tCarCtrl	preCtrl;
    bool formula_diagnostic_telemetry;
    double formula_diagnostic_elapsed;
    double formula_diagnostic_interval;
    TireModelConfiguration formula_tire_configuration;
    int formula_tire_model_enabled;
    int formula_vertical_solver_initialized;
    TireVerticalSolverState formula_vertical_state;
    double formula_vertical_previous_ground_height[4];
    double formula_vertical_ground_velocity[4];
    double formula_vertical_energy_joules;
    double formula_vertical_residual;
    int formula_vertical_newton_iterations;
    int formula_vertical_active_constraint_count;
    int formula_vertical_diagnostic_every_step;
    int formula_diagnostic_controls_enabled;
    int formula_vertical_external_moment_source;
    double formula_vertical_substep_seconds;
    double formula_vertical_time;
    double formula_diagnostic_bump_amplitude_meters;
    double formula_diagnostic_bump_frequency_hertz;
    double formula_diagnostic_bump_duration_seconds;
    double formula_diagnostic_slope;
    double formula_vertical_start_x;

    /* components */
    tAxle		axle[2];
    tWheel		wheel[4];
    tSteer		steer;
    tBrakeSyst	brkSyst;
    tAero		aero;
    tWing		wing[2];
    tTransmission	transmission;	/* includes clutch, gearbox and driveshaft */
    tEngine		engine;

    /* static */
    t3Dd	dimension;	/* car's mesures */
    tdble	mass;		/* mass with pilot (without fuel) */
    tdble	Minv;		/* 1 / mass with pilot (without fuel) */
    tdble	tank;		/* fuel tank capa */
    t3Dd	statGC;		/* static pos of GC */
    t3Dd	Iinv;		/* inverse of inertial moment along the car's 3 axis */

    /* dynamic */
    tdble	fuel;		/* current fuel load */
    tdble	fuelMass;		/* current fuel mass */
    tdble	fuel_consumption; /* average fuel consumption */
    tdble	fuel_prev;	/* average fuel consumption */
    tdble	fuel_time;	/* average fuel consumption */
    tDynPt	DynGC;		/* GC local data except position */
    tDynPt	DynGCg;		/* GC global data */
    tPosd	VelColl;	/* resulting velocity after collision */
    tDynPt	preDynGC;	/* previous one */
    tTrkLocPos	trkPos;		/* current track position */
    tdble	airSpeed2;	/* current air speed (squared) for aerodynamic forces */

    /* internals */
    tdble	Cosz;
    tdble	Sinz;
    tDynPt	corner[4];	/* x,y,z for static relative pos, ax,ay,az for dyn. world coord */
    int		collision;
    t3Dd	normal;
    t3Dd	collpos;
    tdble	wheelbase;
    tdble	wheeltrack;
    sgMat4	posMat;
    DtShapeRef	shape;		/* for collision */
    int		blocked;		// Flag to show if the car has had already a collision in the same timestep.
    int		dammage;

    /* enabling features */
    int     features;
    tdble   TCL_BrakeScale;
    tdble   TCL_SlipScale;
    tdble   TCL_AccelScale;
    tdble   ABS_SlipScale;
    tdble   ABS_BrakeScale;

    tDynPt	restPos;	/* target rest position after the car is broken */

    int		collisionAware;
    SimulationOptions* options;
} tCar;

#if 0

#define CHECK_VAR(_var_, _msg_) do {						\
    if (isnan(_var_) || isinf(_var_)) {						\
    printf("%s = %f  in %s line %d\n", _msg_, _var_, __FILE__, __LINE__);	\
        assert (0);								\
    exit(0);								\
    }										\
} while (0)

#define DUMP_CAR(_car_) do {					\
    printf("DynGC.acc.x  = %f\n", (_car_)->DynGC.acc.x);	\
    printf("DynGC.acc.y  = %f\n", (_car_)->DynGC.acc.y);	\
    printf("DynGC.acc.y  = %f\n", (_car_)->DynGC.acc.y);	\
    printf("DynGC.vel.x  = %f\n", (_car_)->DynGC.vel.x);	\
    printf("DynGC.vel.y  = %f\n", (_car_)->DynGC.vel.y);	\
    printf("DynGCg.pos.x = %f\n", (_car_)->DynGCg.pos.x);	\
    printf("DynGCg.pos.y = %f\n", (_car_)->DynGCg.pos.y);	\
    printf("DynGCg.acc.x = %f\n", (_car_)->DynGCg.acc.x);	\
    printf("DynGCg.acc.y = %f\n", (_car_)->DynGCg.acc.y);	\
    printf("DynGCg.vel.x = %f\n", (_car_)->DynGCg.vel.x);	\
    printf("DynGCg.vel.y = %f\n", (_car_)->DynGCg.vel.y);	\
    printf("DynGCg.pos.x = %f\n", (_car_)->DynGCg.pos.x);	\
    printf("DynGCg.pos.y = %f\n", (_car_)->DynGCg.pos.y);	\
    printf("aero.drag    = %f\n", (_car_)->aero.drag);	\
} while (0)


#define CHECK(_car_) do {									\
    if (isnan((_car_)->DynGC.acc.x) || isinf((_car_)->DynGC.acc.x) ||				\
    isnan((_car_)->DynGC.acc.y) || isinf((_car_)->DynGC.acc.y) ||				\
    isnan((_car_)->DynGC.vel.x) || isinf((_car_)->DynGC.vel.x) ||				\
    isnan((_car_)->DynGC.vel.y) || isinf((_car_)->DynGC.vel.y) ||				\
    isnan((_car_)->DynGC.acc.x) || isinf((_car_)->DynGC.acc.x) ||				\
    isnan((_car_)->DynGCg.acc.y) || isinf((_car_)->DynGCg.acc.y) ||				\
    isnan((_car_)->DynGCg.vel.x) || isinf((_car_)->DynGCg.vel.x) ||				\
    isnan((_car_)->DynGCg.vel.y) || isinf((_car_)->DynGCg.vel.y) ||				\
    isnan((_car_)->DynGCg.pos.x) || isinf((_car_)->DynGCg.pos.x) ||				\
    isnan((_car_)->DynGCg.pos.y) || isinf((_car_)->DynGCg.pos.y) ||				\
    isnan((_car_)->aero.drag) || isinf((_car_)->aero.drag)) {				\
    printf("Problem for %s in %s line %d\n", (_car_)->carElt->_name, __FILE__, __LINE__);	\
    DUMP_CAR(_car_);									\
    assert (0);										\
    /* GfScrShutdown(); */									\
    exit(0);										\
    }												\
} while (0)

#else

#define CHECK_VAR(_var_, _msg_)
#define CHECK(_car_)

#endif

#endif /* _CAR__H_ */
