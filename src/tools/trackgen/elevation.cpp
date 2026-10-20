/***************************************************************************

	file                 : elevation.cpp
	created              : Mon May 20 22:31:09 CEST 2002
	copyright            : (C) 2001 by Eric Espie
	email                : eric.espie@torcs.org

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

	@author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
 */


#include <portability.h>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cmath>

#include <tgfclient.h>
#include <track.h>

#include "ac3d.h"
#include "trackgen.h"
#include "util.h"
#include "elevation.h"

static unsigned char	*ElvImage;
static int	ElvOk = 0;
static tdble 	Margin = DEFAULT_BORDER_MARGIN;

static tdble	kX, kY, dX, dY;
static tdble	kZ, dZ;
static int	width, height;

#define MAX_CLR	255.0

void LoadElevation(tTrack *track, void *TrackHandle, const std::string &imgFile)
{
	tdble zmin, zmax;
	tdble xmin, xmax, ymin, ymax;

	ElvImage = GfTexReadImageFromPNG(imgFile.c_str(), 2.2, &width, &height, 0, 0, false);
	if (!ElvImage) {
		return;
	}

	printf("Loading Elevation Map %s\n", imgFile.c_str());

	Margin = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_BMARGIN, nullptr, Margin);

	xmin = track->min.x - Margin;
	xmax = track->max.x + Margin;
	ymin = track->min.y - Margin;
	ymax = track->max.y + Margin;

	kX = (tdble)(width - 1) / (xmax - xmin);
	dX = -xmin * kX;
	kY = (tdble)(height - 1) / (ymax - ymin);
	dY = -ymin * kY;
	ElvOk = 1;

	zmin = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_ALT_MIN, nullptr, track->min.z);
	zmax = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_ALT_MAX, nullptr, track->max.z);

	dZ = zmin;
	kZ = (zmax - dZ) / MAX_CLR;
}


tdble GetElevation(tdble x, tdble y, tdble z)
{
	if (ElvOk) {
		int iX = (int)(x * kX + dX);
		int iY = (int)(y * kY + dY);
		/* RGBA */
		int clr = ElvImage[4 * (iY * width + iX)];
		return (tdble)clr * kZ + dZ;
	}

	return z;
}


void SaveElevation(tTrack *track, void *TrackHandle, const std::string &imgFile, const std::string &meshFile, int disp, int heightSteps)
{
	float zmin, zmax;
	float xmin, xmax, ymin, ymax;
	float x, y, z;
	int clr;
	int i, j, k, l;
	int columns;
	static char	buf[1024];
	char *s;
	float heightStep;

	s = getenv("COLUMNS");
	if (s) {
		columns = strtol(getenv("COLUMNS"), nullptr, 0);
	} else {
		columns = 80;
	}

	Margin = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_BMARGIN, nullptr, Margin);

	xmin = track->min.x - Margin;
	xmax = track->max.x + Margin;
	ymin = track->min.y - Margin;
	ymax = track->max.y + Margin;

	width = 1024;
	height = (int)((ymax - ymin) * width / (xmax - xmin));

	printf("Generating Elevation Map %s (%d, %d)\n", imgFile.c_str(), width, height);
	kX = (xmax - xmin) / width;
	dX = xmin;
	kY = (ymax - ymin) / height;
	dY = ymin;

	zmin = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_ALT_MIN, nullptr, track->min.z);
	zmax = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_ALT_MAX, nullptr, track->max.z);

	heightStep = (float)(zmax - zmin) / (float)heightSteps;
	if (disp == 2) {
		printf("Height of steps = %f\n", heightStep);
	}

	kZ = MAX_CLR / (zmax - zmin);
	dZ = - zmin * MAX_CLR / (zmax - zmin);

	ElvImage = (unsigned char*)calloc(width * height, 3);
	if (!ElvImage) {
		return;
	}

	Ac3d root;

	try
	{
		root.readFile(meshFile);
		root.flipAxes(true);
	}
	catch (...)
	{
		printf("Could not load %s, ", meshFile.c_str());
		printf("please generate it with \"sd2-trackgen -c %s -n %s -a\"\n", track->category, track->internalname);
		return;
	}

	l = columns - 18;
	for (j = 0; j < height; j++) {
		s = buf;
		s += sprintf(buf, "%4d%% |", (j+1) * 100 / height);
		for (k = s - buf; k < s - buf + l; k++) {
			if ((k - (s - buf)) > (l * (j+1) / height)) {
				buf[k] = ' ';
			} else {
				buf[k] = '*';
			}
		}
		s += l;
		sprintf(s, "| row %4d", j+1);
		printf("\r%s", buf);
		fflush(stdout);
		for (i = 0; i < width; i++) {
			x = i * kX + dX;
			y = j * kY + dY;
			z = root.getTerrainHeight(x, y);
			if (z != -1000000.0f) {
				switch (disp) {
					case 0:
						clr = 0;
						break;
					case 1:
						clr = (int)(z * kZ + dZ);
						break;
					case 2:
						clr = (int)(floor((z + heightStep / 2.0) / heightStep) * heightStep * kZ + dZ);
						break;
					default:
						clr = 0;
						break;
				}
			} else {
				clr = (int)MAX_CLR;
			}

			ElvImage[3 * (i + width * j)]     = (unsigned char)clr;
			ElvImage[3 * (i + width * j) + 1] = (unsigned char)clr;
			ElvImage[3 * (i + width * j) + 2] = (unsigned char)clr;
		}
	}

	printf("\n");
	GfTexWriteImageToPNG(ElvImage, imgFile.c_str(), width, height);
}
