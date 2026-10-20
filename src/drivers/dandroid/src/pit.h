/***************************************************************************

    file                 : pit.h
    created              : Thu Aug 31 01:21:49 UTC 2006
    copyright            : (C) 2006 Daniel Schellhammer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PIT_H_
#define _PIT_H_

#include "torcs_or_sd.h"
#include "globaldefinitions.h"

#ifdef DANDROID_TORCS
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#endif

#include "spline.h"

#define NPOINTS  7
#define DTPOINTS 4 // drive through

// The "DANDROID" logger instance.
extern GfLogger* PLogDANDROID;
#define LogDANDROID (*PLogDANDROID)

class Pit
{
public:
    Pit();
    ~Pit();
    void   init(PTrack t, PSituation s, PtCarElt c, int pitdamage, double pitentrymargin);
    void   setPitstop(bool pitstop);
    bool   getPitstop() { return pitstop; }
    void   setInPit(bool inpitl) { inpitlane = inpitl; }
    bool   getInPit() { return inpitlane; }
    double getPitOffset(double fromstart);
    bool   isBetween(double fromstart);
    bool   isPitlimit(double fromstart);
    bool   pitForPenalty();
    double getPitEntry() { return pitentry; }
    double getLimitEntry() { return limitentry; }
    double getLimitExit() { return limitexit; }
    double getNPitStart() { return p[1].x; }
    double getNPitLoc() { return p[3].x; }
    double getNPitEnd() { return p[5].x; }
    double toSplineCoord(double x);
    void   update(double fromstart);
    int    getRepair();
    int    getCompound();
    double getFuel();
    double getSpeedlimit();
    double getSpeedlimit(double fromstart);
    double getDist();
    double getSideDist();
    double tyreConditionFront();
    double tyreConditionRear();
    double tyreCondition();
    double tyreTreadDepthFront();
    double tyreTreadDepthRear();
    double tyreTreadDepth();
    double avgWearPerLap() { return avgwearperlap; }
    void   setTYC(bool tyc) { HASTYC = tyc; }
    void   setCPD(bool c) { HASCPD = c; }
    void   pitCommand();
public:
    PTrack         track;
    PtCarElt       car;
    PtCarElt       teamcar;
    tTrackOwnPit*  mypit;            /* pointer to my pit */
    tTrackPitInfo* pitinfo;          /* general pit info */
    SplinePoint    p[NPOINTS];       /* spline points */
    Spline         spline;           /* spline */
    SplinePoint    p2[DTPOINTS];
    Spline         spline2;
    int            penalty;
    bool           pitstop;          /* pitstop planned */
    bool           inpitlane;        /* we are still in the pit lane */
    double         mFromStart;
    double         pitentry;         /* distance to start line of the pit entry */
    double         pitexit;          /* distance to the start line of the pit exit */
    double         limitentry;       /* distance to start line of the pit entry */
    double         limitexit;        /* distance to the start line of the pit exit */
    double         speedlimit;       /* pit speed limit */
    bool           fuelchecked;      /* fuel statistics updated */
    double         lastfuel;         /* the fuel available when we cross the start lane */
    double         lastpitfuel;      /* amount refueled, special case when we refuel */
    double         maxfuelperlap;    /* the maximum amount of fuel we needed for a lap */
    double         totalfuel;        /* the total amount of fuel we needed for the race */
    int            fuellapscounted;  /* the total laps we counted, maybe we miss a lap */
    double         avgfuelperlap;    /* the average amount of fuel we needed for a lap */
    double         avgwearperlap;  /* the average amount wear tire by meter */
    double         lastwearperlap;
    double         maxwearperlap;    /* the maximum amount of wear we needed for a lap */
    int            PIT_DAMAGE;
    int            MAX_DAMAGE;
    int            MAX_DAMAGE_DIST;
    double         ENTRY_MARGIN;
    double         SPEED_LIMIT_MARGIN;
    double         MIN_WEAR;
    bool           HASTYC;
    bool           HASCPD;
    int            RAIN;
};

#endif // _PIT_H_
