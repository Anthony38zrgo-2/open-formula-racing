/***************************************************************************

    file                 : util.cpp
    created              : Wed May 29 22:20:24 CEST 2002
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

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <cmath>
#include <iostream>
#include <fstream>

#include <plib/ul.h>
#include <tgfclient.h>
#include <track.h>
#include <robottools.h>
#include <portability.h>
#include "ac3d.h"
#include "trackgen.h"
#include "util.h"

tdble Distance(tdble x0, tdble y0, tdble z0, tdble x1, tdble y1, tdble z1)
{
    return sqrt((x0-x1)*(x0-x1)+(y0-y1)*(y0-y1)+(z0-z1)*(z0-z1));
}

bool
GetFilename(const char *filename, const char *filepaths, char *buf, size_t size)
{
    bool found = false;

    if (filepaths)
    {
        const char *c1 = filepaths;
        const char *c2 = c1;

        while ((!found) && (c2 != nullptr))
        {
            c2 = strchr(c1, ';');

            if (c2 == nullptr)
            {
                snprintf(buf, size, "%s/%s", c1, filename);
            }
            else
            {
                size_t lg = c2 - c1;
                strncpy(buf, c1, lg);
                buf[lg] = '/';
                strcpy(buf + lg + 1, filename);
                c1 = c2 + 1;
            }

            if (ulFileExists(buf))
            {
                found = true;
            }
        }
    }
    else
    {
        strcpy(buf, filename);
        if (ulFileExists(buf))
        {
            found = true;
        }
    }

    if (!found)
    {
        printf("File %s not found\n", filename);
        printf("File Path was %s\n", filepaths);
    }

    return found;
}

/*
 * calculates an angle that aligns with the closest track segment. Angle
* is determined so that the x axis is aligned parallel to track,
*  with positive y axis pointing away from track
*  Finished
*/
float
getTrackAngle(tTrack *Track, void *TrackHandle, float x, float y)
{
    float angle=0, dst=100000, shDst=100000;
    int 		i;
    tTrackSeg 		*seg = Track->seg;
    tTrkLocPos		curPos, closePos, objPos;
    float x2, y2, x3, y3;
    for(i = 0; i < Track->nseg; i++)
    {
        seg=seg->next;
        curPos.seg=seg;
        curPos.toMiddle=0;

        switch (seg->type)
        {
        case TR_STR:
            for (float toStart=0; toStart<seg->length; toStart+=(seg->length/5.0))
            {
                curPos.toStart=toStart;
                RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOMIDDLE);
                dst=Distance(x,y,0,x2,y2,0);
                //printf("distance: %g\n",dst);

                if ( dst < shDst )
                {
                    shDst=dst;
                    closePos=curPos;
                }
            }

            break;
        case TR_RGT:
            for (float toStart=0; toStart<seg->arc; toStart+=(seg->arc/5.0))
            {
                curPos.toStart=toStart;
                RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOMIDDLE);
                dst=Distance(x,y,0,x2,y2,0);
                //printf("distance: %g\n",dst);

                if ( dst < shDst )
                {
                    shDst=dst;
                    closePos=curPos;
                }
            }

            break;
        case TR_LFT:
            for (float toStart=0; toStart<seg->arc; toStart+=(seg->arc/10.0))
            {
                curPos.toStart=toStart;
                RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOMIDDLE);
                dst=Distance(x,y,0,x2,y2,0);
                //printf("distance: %g\n",dst);

                if ( dst < shDst )
                {
                    shDst=dst;
                    closePos=curPos;
                }
            }

            break;
        }
    }

    printf("shortest distance: %g\n",shDst);
    curPos=closePos;
    seg=curPos.seg;
    printf("found at segment: %s\n",seg->name);
    RtTrackLocal2Global(&curPos, &x3, &y3, TR_TOMIDDLE);
    curPos.toStart+=0.001;
    RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOMIDDLE);
    angle=90.0-atan2f(x2-x3,y2-y3)*180/PI;
    printf("calculated angle: %g\n",angle);
    RtTrackGlobal2Local(seg, x, y, &objPos, 0);
    if ( objPos.toMiddle < 0 ) {angle-=180.0;}
    return angle;
}

/*
 * same thing for border alignment, just one difference: a new position
 * x/y/z is returned to keep it aligned with the track in distance & height
 * need additional input in xml object definition:
 *   <attnum...
 *
 */
float
getBorderAngle(tTrack *Track, void *TrackHandle, float x, float y, float distance, float *xRet, float *yRet, float *zRet )
{
    float angle=0, dst=100000, shDst=100000;
    tTrackSeg 		*seg = Track->seg;
    tTrkLocPos		curPos, closePos, objPos;
    float x2, y2, x3, y3;
    for(int i = 0; i < Track->nseg; i++)
    {
        seg=seg->next;
        curPos.seg=seg;
        curPos.toMiddle=0;

        switch (seg->type)
        {
        case TR_STR:
            for (float toStart=0; toStart<seg->length; toStart+=(seg->length/5.0))
            {
                curPos.toStart=toStart;
                RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOMIDDLE);
                dst=Distance(x,y,0,x2,y2,0);
                //printf("distance: %g\n",dst);
                if ( dst < shDst ){
                    shDst=dst;
                    closePos=curPos;
                }
            }

            break;
        case TR_RGT:
            for (float toStart=0; toStart<seg->arc; toStart+=(seg->arc/5.0))
            {
                curPos.toStart=toStart;
                RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOMIDDLE);
                dst=Distance(x,y,0,x2,y2,0);
                //printf("distance: %g\n",dst);

                if ( dst < shDst )
                {
                    shDst=dst;
                    closePos=curPos;
                }
            }
            break;

        case TR_LFT:
            for (float toStart=0; toStart<seg->arc; toStart+=(seg->arc/10.0))
            {
                curPos.toStart=toStart;
                RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOMIDDLE);
                dst=Distance(x,y,0,x2,y2,0);
                //printf("distance: %g\n",dst);
                if ( dst < shDst )
                {
                    shDst=dst;
                    closePos=curPos;
                }
            }

            break;
        }
    }
    //check if left or right

    curPos=closePos;
    seg=curPos.seg;
    //check if left or right
    RtTrackGlobal2Local(seg, x, y, &objPos, 1);

    if ( objPos.toMiddle < 0 )
        // object on right side
    {
        curPos.toRight = 0;
        objPos.toRight = -distance;
        RtTrackLocal2Global(&curPos, &x3, &y3, TR_TORIGHT);
        curPos.toStart+=0.001;
        RtTrackLocal2Global(&curPos, &x2, &y2, TR_TORIGHT);
        angle=-90.0-atan2f(x2-x3,y2-y3)*180/PI;
        RtTrackLocal2Global(&objPos, xRet, yRet, TR_TORIGHT);
    }
    else
        // left side
    {	curPos.toLeft = 0;
        objPos.toLeft = -distance;
        RtTrackLocal2Global(&curPos, &x3, &y3, TR_TOLEFT);
        curPos.toStart+=0.001;
        RtTrackLocal2Global(&curPos, &x2, &y2, TR_TOLEFT);
        angle=90.0-atan2f(x2-x3,y2-y3)*180/PI;
        RtTrackLocal2Global(&objPos, xRet, yRet, TR_TOLEFT);
    }

    *zRet=RtTrackHeightG(seg, *xRet, *yRet);
    printf("tried to align to border: x: %g y: %g z: %g angle: %g \n", *xRet, *yRet, *zRet, angle);
    //return values

    return angle;
}

bool loadPngTexture(const char *fname, ssgTextureInfo *info)
{
    GLubyte *tex;
    int w, h;

    tex = (GLubyte *)GfTexReadImageFromPNG(fname, 2.0, &w, &h, 0, 0);
    if (!tex) {
        return false;
    }

    if (info) {
        info->width = w;
        info->height = h;
        info->depth = 4;
        info->alpha = true;
    }

    return true;
}

bool loadJpegTexture(const char *fname, ssgTextureInfo *info)
{
    GLubyte *tex;
    int w, h;

    tex = (GLubyte *)GfTexReadImageFromJPEG(fname, 2.0, &w, &h, 0, 0);
    if (!tex) {
        return false;
    }

    if (info) {
        info->width = w;
        info->height = h;
        info->depth = 4;
        info->alpha = true;
    }

    return true;
}

std::ostream &operator << (std::ostream &out, const t3Dd &v)
{
    out << v.x << ", " << v.y << ", " << v.z;

    return out;
}

std::ostream &operator << (std::ostream &out, tSegType type)
{
    out << (type == TR_RGT ? "TR_RGT" :
            type == TR_LFT ? "TR_LFT" :
            type == TR_STR ? "TR_STR" :
            std::to_string(type));

    return out;
}

std::ostream &operator << (std::ostream &out, tSegType2 type2)
{
    out << (type2 == TR_MAIN ? "TR_MAIN" :
            type2 == TR_LSIDE ? "TR_LSIDE" :
            type2 == TR_RSIDE ? "TR_RSIDE" :
            type2 == TR_LBORDER ? "TR_LBORDER" :
            type2 == TR_RBORDER ? "TR_RBORDER" :
            std::to_string(type2));

    return out;
}

std::ostream &operator << (std::ostream &out, tSegStyle style)
{
    out << (style == TR_PLAN ? "TR_PLAN" :
            style == TR_CURB ? "TR_CURB" :
            style == TR_WALL ? "TR_WALL" :
            style == TR_FENCE ? "TR_FENCE" :
            style == TR_PITBUILDING ? "TR_PITBUILDING" :
            style == TR_NO_BARRIER ? "TR_NO_BARRIER" :
            std::to_string(style));

    return out;
}

std::string dumpRaceInfo(unsigned int raceInfo)
{
    std::string info;
    if (raceInfo  == 0)
        info += "TR_NORMAL ";
    if (raceInfo & TR_LAST)
        info += "TR_LAST ";
    if (raceInfo & TR_START)
        info += "TR_START ";
    if (raceInfo & TR_PITLANE)
        info += "TR_PITLANE ";
    if (raceInfo & TR_SPEEDLIMIT)
        info += "TR_SPEEDLIMIT ";
    if (raceInfo & TR_PITENTRY)
        info += "TR_PITENTRY ";
    if (raceInfo & TR_PITEXIT)
        info += "TR_PITEXIT ";
    if (raceInfo & TR_PIT)
        info += "TR_PIT ";
    if (raceInfo & TR_PITSTART)
        info += "TR_PITSTART ";
    if (raceInfo & TR_PITEND)
        info += "TR_PITEND ";
    if (raceInfo & TR_PITBUILD)
        info += "TR_PITBUILD";
    return info;
}

void dumpSeg(std::ofstream &ofs,const tTrackSeg *seg, const std::string & indent = "")
{
//    ofs << indent << std::hex << seg << std::dec << std::endl;
    ofs << indent << "name          " << (seg->name ? seg->name : "") << std::endl;
    ofs << indent << "id            " << seg->id << std::endl;
    ofs << indent << "type          " << seg->type << std::endl;
    ofs << indent << "type2         " << seg->type2  << std::endl;
    ofs << indent << "style         " << seg->style << std::endl;
    ofs << indent << "length        " << seg->length << std::endl;
    ofs << indent << "width         " << seg->width << std::endl;
    ofs << indent << "startWidth    " << seg->startWidth << std::endl;
    ofs << indent << "endWidth      " << seg->endWidth << std::endl;
    ofs << indent << "lgfromstart   " << seg->lgfromstart << std::endl;
    ofs << indent << "radius        " << seg->radius << std::endl;
    ofs << indent << "radiusr       " << seg->radiusr << std::endl;
    ofs << indent << "radiusl       " << seg->radiusl << std::endl;
    ofs << indent << "arc           " << seg->arc << std::endl;
    ofs << indent << "center        " << seg->center << std::endl;
    ofs << indent << "vertex[0]     " << seg->vertex[0] << std::endl;
    ofs << indent << "vertex[1]     " << seg->vertex[1] << std::endl;
    ofs << indent << "vertex[2]     " << seg->vertex[2] << std::endl;
    ofs << indent << "vertex[3]     " << seg->vertex[3] << std::endl;
    ofs << indent << "angle         ";
    for (size_t i = 0; i < 7; ++i)
        ofs << seg->angle[i] << " ";
    ofs << std::endl;
    ofs << indent << "sin           " << seg->sin << std::endl;
    ofs << indent << "cos           " << seg->cos << std::endl;
    ofs << indent << "Kzl           " << seg->Kzl << std::endl;
    ofs << indent << "Kzw           " << seg->Kzw << std::endl;
    ofs << indent << "Kyl           " << seg->Kyl << std::endl;
    ofs << indent << "rgtSideNormal " << seg->rgtSideNormal << std::endl;
    ofs << indent << "envIndex      " << seg->envIndex << std::endl;
    ofs << indent << "height        " << seg->height << std::endl;
    ofs << indent << "raceinfo      0x" << std::hex << seg->raceInfo << std::dec << " " << dumpRaceInfo(seg->raceInfo) << std::endl;
    ofs << indent << "DoVfactor     " << seg->DoVfactor << std::endl;
    if (seg->ext)
    {
        ofs << indent << "nbMarks       " << seg->ext->nbMarks << std::endl;
        ofs << indent << "marks         ";
        for (int i = 0; i < seg->ext->nbMarks; ++i)
            ofs << seg->ext->marks[i] << " ";
        ofs << std::endl;
    }
    if (seg->surface)
    {
        ofs << indent << "surface" << std::endl;
        ofs << indent << "    material  " << (seg->surface->material ? seg->surface->material : "none") << std::endl;
    }
//    if (seg->next)
//        ofs << indent << "next         " << std::hex << seg->next << std::dec << std::endl;
//    if (seg->prev)
//        ofs << indent << "prev         " << std::hex << seg->prev << std::dec << std::endl;
    if (seg->barrier[TR_SIDE_RGT])
    {
        ofs << indent << "barrier[TR_SIDE_RGT]  " << std::endl;
        ofs << indent << "    style     " << seg->barrier[TR_SIDE_RGT]->style << std::endl;
        ofs << indent << "    width     " << seg->barrier[TR_SIDE_RGT]->width << std::endl;
        ofs << indent << "    height    " << seg->barrier[TR_SIDE_RGT]->height << std::endl;
        ofs << indent << "    surface   " << (seg->barrier[TR_SIDE_RGT]->surface && seg->barrier[TR_SIDE_RGT]->surface->material ? seg->barrier[TR_SIDE_RGT]->surface->material : "") << std::endl;
    }
    if (seg->barrier[TR_SIDE_LFT])
    {
        ofs << indent << "barrier[TR_SIDE_LFT]  " << std::endl;
        ofs << indent << "    style     " << seg->barrier[TR_SIDE_LFT]->style << std::endl;
        ofs << indent << "    width     " << seg->barrier[TR_SIDE_LFT]->width << std::endl;
        ofs << indent << "    height    " << seg->barrier[TR_SIDE_LFT]->height << std::endl;
        ofs << indent << "    surface   " << (seg->barrier[TR_SIDE_LFT]->surface && seg->barrier[TR_SIDE_LFT]->surface->material ? seg->barrier[TR_SIDE_LFT]->surface->material : "") << std::endl;
    }
    tTrackSeg *s = seg->rside;
    std::string i = indent + "    ";
    if (s)
    {
        ofs << indent << "--------------- right ----------------" << std::endl;
        dumpSeg(ofs, s, i);
    }
    s = seg->lside;
    i = indent + "    ";
    if (s)
    {
        ofs << indent << "--------------- left -----------------" << std::endl;
        dumpSeg(ofs, s, i);
    }
}

void dumpTrackSegs(tTrack *track)
{
    tTrackSeg *seg;
    int i;
    std::string ofname = track->filename;
    ofname.resize(ofname.size() - 3);
    ofname.append("dump");
    std::ofstream ofs;
    ofs.open(ofname.c_str());
    if (ofs)
    {
        for (i = 0, seg = track->seg->next; i < track->nseg; i++, seg = seg->next)
        {
            ofs << "-------------------------------------------" << std::endl;
            dumpSeg(ofs, seg);
        }
        ofs.close();
    }
}
