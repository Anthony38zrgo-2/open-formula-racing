/***************************************************************************

    file                 : elevation.h
    created              : Mon May 20 23:11:14 CEST 2002
    copyright            : (C) 2001 by Eric Espie
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

#ifndef _ELEVATION_H_
#define _ELEVATION_H_

extern void LoadElevation(tTrack *track, void *TrackHandle, const std::string &imgFile);
extern tdble GetElevation(tdble x, tdble y, tdble z);
extern void SaveElevation(tTrack *track, void *TrackHandle, const std::string &imgFile, const std::string &meshFile, int disp, int heightSteps);


#endif /* _ELEVATION_H_ */
