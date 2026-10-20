/***************************************************************************

    file                 : trackgen.h
    created              : Sun Dec 24 16:00:03 CET 2000
    copyright            : (C) 2000 by Eric Espié
    email                : Eric.Espie@torcs.org

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file

    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
*/

#ifndef _TRACKGEN_H_
#define _TRACKGEN_H_

#define CLOCKWISE	0
#define ANTICLOCKWISE	1

#define DEFAULT_TRACK_STEP      10.0
#define DEFAULT_BORDER_MARGIN   100.0
#define DEFAULT_BORDER_STEP     10.0
#define DEFAULT_BORDER_HEIGHT   0.0
#define DEFAULT_GROUP_SIZE      100.0
#define DEFAULT_SURFACE         "grass"

#define CFG_FILE	"tools/trackgen/trackgen.xml"


extern void GenerateTrack(tTrack * Track, void *TrackHandle, const std::string &outFile, Ac3d &allAc3d, bool all, bool bump, bool raceline, bool bridge, bool acc);
extern void CalculateTrack(tTrack * Track, void *TrackHandle, bool bump, bool raceline, bool bridge, bool acc);
extern void generateRaceLine(tTrack* pTrack, const double SideDistExt, const double SideDistInt);
extern double getTexureOffset(double length);

#endif /* _TRACKGEN_H_ */
