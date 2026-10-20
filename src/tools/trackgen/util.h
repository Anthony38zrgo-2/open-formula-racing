/***************************************************************************

    file                 : util.h
    created              : Wed May 29 22:20:44 CEST 2002
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <plib/ssg.h>

extern bool GetFilename(const char *filename, const char *filepaths, char *buf, size_t size);
extern float getTrackAngle(tTrack *Track, void *TrackHandle, float x, float y);
extern float getBorderAngle(tTrack *Track, void *TrackHandle, float x, float y, float distance, float *xRet, float *yRet, float *zRet);
extern tdble Distance(tdble x0, tdble y0, tdble z0, tdble x1, tdble y1, tdble z1);
extern bool loadPngTexture(const char *fname, ssgTextureInfo *info);
extern bool loadJpegTexture(const char *fname, ssgTextureInfo *info);
extern void dumpTrackSegs(tTrack *track);

#endif /* _UTIL_H_ */
