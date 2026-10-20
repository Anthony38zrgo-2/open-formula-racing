/***************************************************************************

    file                 : aero.h
    created              : Sun Mar 19 00:04:59 CET 2000
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

#ifndef _AERO_H_
#define _AERO_H_

typedef struct
{
    /* dynamic */
    tdble	drag;		/* drag force along car x axis */
    tdble	lift[2];	/* front & rear lift force along car z axis */

    /* static */
    //tdble	SCx2;       replaced by CdBody as initial Cd
    tdble	Clift[2];	/* front & rear lift due to body not wings */
    tdble	Cd;		    /* for aspiration, value updated depending on wing angles */
    tdble	CdBody;	    /* for aspiration, value without wings, for variable wing angles */
    bool whole_vehicle_reference;
    tdble reference_drag_area;
    tdble reference_wing_drag_area;
    tdble floor_lift_area;
    tdble floor_induced_drag_ratio;
    tdble floor_choke_height;
    tdble floor_optimal_height;
    tdble floor_high_height;
    tdble floor_front_clearance;
    tdble floor_rear_clearance;
    tdble floor_optimal_rake;
    tdble floor_rake_window;
    tdble floor_seal_roll;
    tdble floor_downforce;
    tdble floor_drag;
    tdble soft_maximum_load_ratio;
    tdble hard_maximum_load_ratio;
    tdble hard_maximum_downforce;
    t3Dd floor_position;
} tAero;


typedef struct
{
    /* dynamic */
    t3Dd	forces;
    tdble	Kx;
    tdble	Kz;
    tdble	Kz_org;
    tdble	angle;

    /* static */
    t3Dd	staticPos;

//>>> simuV4
    /* static for wings with const angle */
    tdble	AoAatMax;	/* [deg] Angle of Attack at the maximum of coefficient of lift */
    tdble	AoAatZero;	/* [deg] Angle of Attack at coefficient of lift = 0 (-30 < AoAatZero < 0) */
    tdble	AoAatZRad;	/* [rad] Angle of Attack at coefficient of lift = 0 (-30 < AoAatZero < 0) */
    tdble	AoAOffset;	/* [deg] Offset for Angle of Attack */

    tdble	CliftMax;	/* Maximum of coefficient of lift (0 < CliftMax < 4) */
    tdble	CliftZero;	/* Coefficient of lift at Angle of Attack = 0 */
    tdble	CliftAsymp;	/* Asymptotic coefficient of lift at large Angle of Attack */
    tdble	a;			/* [deg] Scaled angle at decreasing */
    tdble	b;			/* Delay of decreasing */
    tdble	c;			/* Curvature of start of decreasing */
    tdble	d;			/* Scale at AoA = 0 */
    tdble	f;			/* Scale factor */

    /* parameters for THIN wing model */
    tdble   AoStall;    /* angle of stall =15 deg (1 < AoStall < 45 in degrees) */
    tdble   Stallw;     /* stall width =2 deg, (1 < Stallw < AoStall) */
    tdble   AR;         /* effective aspect ratio of wing, 0 means infinite, must be positive */
    tdble   Kx1, Kx2, Kx3, Kx4;
    tdble   Kz1, Kz2, Kz3;

    int		WingType;	/* -1=None, 0=FLAT, 1=PROFILE, 2=THIN... */
    int polar_point_count;
    tdble polar_angles[16];
    tdble polar_drag_coefficients[16];
    tdble polar_lift_coefficients[16];
    tdble polar_area;
//<<< simuV4
} tWing;

#endif /* _AERO_H_  */
