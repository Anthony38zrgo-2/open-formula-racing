/***************************************************************************

    file                 : objects.h
    created              : Fri May 24 20:10:22 CEST 2002
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

#ifndef _OBJECTS_H_
#define _OBJECTS_H_

extern void GenerateObjects(tTrack *track, void *TrackHandle, void *CfgHandle, Ac3d &allAc3d, bool all, const std::string &terrainFile, const std::string &trackFile, const std::string &outputFile, bool multipleMaterials, bool acc);

#endif /* _OBJECTS_H_ */
