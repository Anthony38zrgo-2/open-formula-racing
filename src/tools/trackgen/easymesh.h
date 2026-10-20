/***************************************************************************

    file                 : easymesh.h
    created              : Sun Feb 25 22:50:07 /etc/localtime 2001
    copyright            : Bojan NICENO & Eric Espie
    email                : niceno@univ.trieste.it Eric.Espie@torcs.org

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

#ifndef _EASYMESH_H_
#define _EASYMESH_H_

void GenerateTerrain(tTrack *track, void *TrackHandle, const std::string &outfile, Ac3d &allAc3d, bool all, int noElevation, bool useBorder, bool acc);
int getPointCount();
void addPoint(double x, double y, double z, double F, int mark);
void addSegment(int n0, int n1, int mark);

#endif /* _EASYMESH_H_ */
