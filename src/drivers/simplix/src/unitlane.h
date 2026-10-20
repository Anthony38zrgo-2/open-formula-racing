//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
// unitlane.h
//--------------------------------------------------------------------------*
// A robot for Speed Dreams-Version	2.X	simuV4
//--------------------------------------------------------------------------*
// Lane
// Fahrspur
//
// File			:	unitlane.h
// Created		: 2007.11.17
// Last	changed	: 2014.11.29
// Copyright	: © 2007-2014 Wolf-Dieter Beelitz
// eMail		:	wdbee@users.sourceforge.net
// Version		: 4.05.000
//--------------------------------------------------------------------------*
// Teile diese Unit	basieren auf diversen Header-Dateien von TORCS
//
//	  Copyright:	(C)	2000 by	Eric Espie
//	  eMail	  : torcs@free.fr
//
// dem erweiterten Robot-Tutorial bt
//
//	  Copyright:	(C)	2002-2004 Bernhard Wymann
//	  eMail	  : berniw@bluewin.ch
//
// dem Roboter mouse_2006
//
//	  Copyright:	(C)	2006-2007 Tim Foden
//
// dem Roboter wdbee_2007
//
//	  Copyright:	(C)	2006-2007 Wolf-Dieter Beelitz
//	  eMail	  : wdbee@users.sourceforge.net
//
// und dem Roboter delphin
//
//	  Copyright:	(C)	2006-2007 Wolf-Dieter Beelitz
//	  eMail	  : wdbee@users.sourceforge.net
//
//--------------------------------------------------------------------------*
// Das Programm	wurde unter	Windows	XP entwickelt und getestet.
// Fehler sind nicht bekannt, dennoch gilt:
// Wer die Dateien verwendet erkennt an, dass für Fehler, Schäden,
// Folgefehler oder	Folgeschäden keine Haftung übernommen wird.
//
// Im übrigen gilt für die Nutzung und/oder	Weitergabe die
// GNU GPL (General	Public License)
// Version 2 oder nach eigener Wahl	eine spätere Version.
//--------------------------------------------------------------------------*
// This	program	is free	software; you can redistribute it and/or modify
// it under	the	terms of the GNU General Public	License	as published by
// the Free	Software Foundation; either	version	2 of the License, or
// (at your	option)	any	later version.
//--------------------------------------------------------------------------*
#ifndef	_UNITLANE_H_
#define	_UNITLANE_H_

#include "unitglobal.h"
#include "unittrack.h"
#include "unitcarparam.h"
#include "unitfixcarparam.h"
#include "unitcubicspline.h"
#include <vector>

//==========================================================================*
// Class TLane
//--------------------------------------------------------------------------*
class TLane
{
  public:
	struct	TPathPt
	{
		TPathPt(const TSection &sec, float left, float right) :
			DistFromStart(sec.DistFromStart),
			ToRight(sec.ToRight),
			Offset(0),
			Center(sec.Center),
			Point(CalcPt()),
			Crv(0),
			CrvZ(0),
			NextCrv(0),
			WToL(left),
			WToR(right),
			WPitToL(sec.PitWidthToLeft),
			WPitToR(sec.PitWidthToRight),
			Fix(false),
			MaxSpeed(10),
			AccSpd(10),
			Speed(10),
			FlyHeight(0)
		{}

		TPathPt(const TSection &sec) :
			TPathPt(sec, 0, 0)
		{}

		float DistFromStart;
		TVec3d ToRight;
		float	Offset;							   //	Offset from	centre point
		TVec3d Center;						   //	Lane specific center
		TVec3d Point;							   //	Actual point (same as CalcPt())
		float	Crv;	 							 // Curvature in xy
		float	CrvZ;								 // Curvature in z	direction... e.g. bumps
		float	NextCrv;						   //	Cuvature comming next
		float	WToL;							   // Lane specfic width to	left
		float	WToR;							   // Lane specfic width to	right
		float	WPitToL;						   //	Lane specfic width to left
		float	WPitToR;						   //	Lane specfic width to right
		bool Fix;
		float MaxSpeed;						   // Max speed	through	this point
		float AccSpd;						   //	Speed through this point, with modelled	accel
		float Speed;							   //	Speed through this point (braking only)
		float FlyHeight;						   // Predicted height	of car above track (flying)

		double Dist()	const {return DistFromStart;}
		double WtoL()	const {return WToL;}
		double WtoR()	const {return WToR;}
		const	TVec3d&	Pt() const {return Center;}
		const	TVec3d&	Norm() const {return ToRight;}
		TVec3d CalcPt() const	{return	Center + ToRight *	Offset;}
	};

  public:
	static	const int TA_N = 10;				  // Nbr of	points
	double	TA_X[TA_N];							  // X-coordinates
	double	TA_Y[TA_N];							  // Y-coordinates
	double	TA_S[TA_N];							  // Directions

	std::vector<TPathPt> oPathPoints;			  // Points	in this	lane

	TLane(const TDriver &driver);

	virtual bool ContainsPos
	  (double TrackPos) const;
	virtual bool GetLanePoint
	  (double TrackPos, TLanePoint& LanePoint)	const;

	void Initialise
	  (TTrackDescription* pTrack,
	  const TFixCarParam& FixCarParam,
	  const TCarParam&	CarParam,
	  double MaxLeft =	FLT_MAX,
	  double MaxRight = FLT_MAX);
	void SmmothLane();

	const TPathPt&	PathPoints(int Index) const;
	const std::vector<TLane::TPathPt> &PathPoints() const;

	void Dump();
	void SmoothSpeeds();
	void CalcCurvaturesXY
	  (int	Start, int Step	= 1);
	void CalcCurvaturesZ
	  (int	Start, int Step	= 1);
	void CalcMaxSpeeds
	  (int	Start, int Len,	int	Step = 1);
	void PropagateBreaking
	  (int	Start, int Len,	int	Step = 1);
	void PropagatePitBreaking
	  (int	Start, int Len,	float PitStopPos, float	ScaleMu);
	void PropagateAcceleration
	  (int	Start, int Len,	int	Step = 1);

	void CalcCurvaturesXY
	  (int	Step = 1);
	void CalcCurvaturesZ
	  (int	Step = 1);
	void CalcMaxSpeeds
	  (int	Step = 1);
	void PropagateBreaking
	  (int	Step = 1);
	void PropagatePitBreaking
	  (int	Start, float PitStopPos, float ScaleMu);
	void PropagateAcceleration
	  (int	Step = 1);
	void CalcFwdAbsCrv
	  (int	Range, int Step	= 1);

	double	CalcEstimatedTime
	  (int	Start, int Len)	const;
	double	CalcEstimatedLapTime() const;
	double	CalcTrackRollangle(double TrackPos);
	double	CalcTrackTurnangle(int P, int Q);


  protected:
	TTrackDescription*	oTrack;					  // TORCS track data
	TFixCarParam oFixCarParam;					  // Copy of car params
	TCarParam oCarParam;						  //	Copy of	car	params
	TCubicSpline oTurnScale;					  // Scale of	turns
	bool UseBrakeLimit;
};
//==========================================================================*
#endif // _UNITLANE_H_
//--------------------------------------------------------------------------*
// end of file unitlane.h
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
