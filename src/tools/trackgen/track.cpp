/***************************************************************************

    file         : track.cpp
    created      : Sun Dec 24 12:14:18 CET 2000
    copyright    : (C) 2000 by Eric Espie
    email        : eric.espie@torcs.org

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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <cmath>

#include <robottools.h>
#include <tgfclient.h>
#include <track.h>

#include "ac3d.h"
#include "util.h"

#include "trackgen.h"

struct tTexElt
{
    char *name;
    char *namebump;
    char *nameraceline;
    int mipmap; // Not yet used.
    unsigned int texid;
    tTexElt *next;
};

struct tDispElt
{
    int start;
    int nb;
    Ac3d::Surface::SURF surfType;
    char *name;
    int id;
    tTexElt *texture;
    tDispElt *next;
};

struct tGroup
{
    int nb;
    tDispElt *dispList;
};

static tGroup *Groups;
static int ActiveGroups;
static int GroupNb;

#define LMAX TrackStep

static float *trackvertices;
static float *tracktexcoord;
static float *tracktexcoord2;
static unsigned int *trackindices;

static tdble TrackStep = DEFAULT_TRACK_STEP;

#ifndef sqr
#define sqr(x) ((x) * (x))
#endif

static void initPits(tTrack *theTrack, void *TrackHandle, tTrackPitInfo *pits)
{
    tTrackSeg *curMainSeg;
    tTrackSeg *curPitSeg = nullptr;
    tTrackSeg *pitBuildingsStart = nullptr;
    const char *segName;
    tTrkLocPos curPos;
    int i;
    bool found = false;
    bool changeSeg = false;
    tdble offset = 0;
    tdble toStart = 0;
    static char path2[256];

    sprintf(path2, "%s/%s", TRK_SECT_MAIN, TRK_SECT_PITS);

    // In TR_PIT_NO_BUILDING, the pit positions are located exactly the same,
    // as in the TR_PIT_ON_TRACK_SIDE. It differs only later, showing/not showing
    // the buildings.
    switch (pits->type)
    {
    case TR_PIT_ON_TRACK_SIDE:
    case TR_PIT_NO_BUILDING:
        pits->driversPitsNb = MIN(pits->nPitSeg, pits->nMaxPits);
        pits->driversPits = (tTrackOwnPit *)calloc(pits->driversPitsNb, sizeof(tTrackOwnPit));

        curPos.type = TR_TOMIDDLE;
        // TR_LPOS_MAIN; //NB: TR_LPOS_MAIN not handled by RtTrackLocal2Global!

        segName = GfParmGetStr(TrackHandle, path2, TRK_ATT_BUILDINGS_START, nullptr);
        if (segName != 0)
        {
            pitBuildingsStart = theTrack->seg;
            found = false;
            for (i = 0; i <= theTrack->nseg; i++)
            {
                if (!strcmp(segName, pitBuildingsStart->name))
                {
                    found = true;
                }
                else if (found)
                {
                    pitBuildingsStart = pitBuildingsStart->next;
                    break;
                }
                pitBuildingsStart = pitBuildingsStart->prev;
            }

            if (!found)
            {
                pitBuildingsStart = nullptr;
            }
        }

        if (pitBuildingsStart == nullptr)
        {
            if (pits->pitStart == nullptr)
            {
                GfLogError("Can't find pit start\n");
                exit(1);
            }
            pitBuildingsStart = pits->pitStart;
        }

        curMainSeg = pitBuildingsStart->prev;

        changeSeg = true;
        offset = 0;
        toStart = 0;
        i = 0;
        while (i < pits->driversPitsNb)
        {
            if (changeSeg)
            {
                changeSeg = false;
                offset = 0;
                curMainSeg = curMainSeg->next;

                switch (pits->side)
                {
                case TR_RGT:
                    curPitSeg = curMainSeg->rside;
                    if (curPitSeg->rside)
                    {
                        offset = curPitSeg->width;
                        curPitSeg = curPitSeg->rside;
                    }
                    break;

                case TR_LFT:
                    curPitSeg = curMainSeg->lside;
                    if (curPitSeg->lside)
                    {
                        offset = curPitSeg->width;
                        curPitSeg = curPitSeg->lside;
                    }
                    break;
                }

                curPos.seg = curMainSeg;
                if (toStart >= curMainSeg->length)
                {
                    toStart -= curMainSeg->length;
                    changeSeg = true;
                    continue;
                }
            } // if changeSeg

            /* Not the real position but the start and border one
             * instead of center */
            // RtTrackLocal2Global expects toStart as a length in meters for straight,
            // and as an angle in radian for curves
            // TODO(kilo)
            // Proper handling of this should enable non-linear pitlanes.
            // Postponed after 2.0
#if 0
                tdble pitCenter = toStart + pits->len / 2.0;
                switch(curMainSeg->type) {
                    case TR_STR:
                        pits->driversPits[i].pos.toStart = pitCenter;
                        break;

                    case TR_LFT:
                    case TR_RGT:
                        pits->driversPits[i].pos.toStart = pitCenter / curMainSeg->radius;
                        break;
                }
#endif
            // TODO(kilo) get rid of following line when above feature is ready
            curPos.toStart = toStart;

            switch (pits->side)
            {
            case TR_RGT:
                curPos.toRight = -offset - RtTrackGetWidth(curPitSeg, toStart);
                curPos.toLeft = curMainSeg->width - curPos.toRight;
                curPos.toMiddle = -curMainSeg->width / 2.0 + curPos.toRight;
                break;

            case TR_LFT:
                curPos.toLeft = -offset - RtTrackGetWidth(curPitSeg, toStart);
                curPos.toRight = curMainSeg->width - curPos.toLeft;
                curPos.toMiddle = curMainSeg->width / 2.0 - curPos.toLeft;
                break;
            }

            memcpy(&(pits->driversPits[i].pos), &curPos, sizeof(curPos));
            toStart += pits->len;
            if (toStart >= curMainSeg->length)
            {
                toStart -= curMainSeg->length;
                changeSeg = true;
            }

            i++;

        } // while i
        break;

    case TR_PIT_ON_SEPARATE_PATH:
        break;

    case TR_PIT_NONE:
        break;
    }
}

void SETPOINT(const tTexElt *curTexElt, int &nbvert, float t1, float t2, float x, float y, float z)
{
    if (*(curTexElt->name) != 0)
    {
        tracktexcoord[2 * nbvert] = t1;
        tracktexcoord[2 * nbvert + 1] = t2;
        trackvertices[3 * nbvert] = x;
        trackvertices[3 * nbvert + 1] = y;
        trackvertices[3 * nbvert + 2] = z;
        trackindices[nbvert] = nbvert;
        ++nbvert;
    }
}

void SETPOINT2(const tTexElt *curTexElt, int &nbvert, float u, float v, float u2, float v2, float x, float y, float z)
{
    if (*(curTexElt->name) != 0)
    {
        tracktexcoord[2 * nbvert] = u;
        tracktexcoord[2 * nbvert + 1] = v;
        tracktexcoord2[2 * nbvert] = u2;
        tracktexcoord2[2 * nbvert + 1] = v2;
        trackvertices[3 * nbvert] = x;
        trackvertices[3 * nbvert + 1] = y;
        trackvertices[3 * nbvert + 2] = z;
        trackindices[nbvert] = nbvert;
        ++nbvert;
    }
}

/** Bug fix:
    If width of border decreased, additional vertices are needed to avoid missing surfaces
    Implementation:
    Compares old end vertices and new start vertices, set startNeeded if one is different
**/
int InitScene(tTrack *Track, void *TrackHandle, bool bump, bool raceline, bool bridge, bool acc)
{
    int i;
    tTrackSeg *seg;
    tTrackSeg *lastSeg = nullptr;
    tTrackSeg *mseg;
    int nbvert;
    tdble width;
    tdble anz, ts = 0;
    tdble radiusr, radiusl;
    tdble step;
    tTrkLocPos trkpos;
    tdble x, y, z;
    tdble x2, y2, z2;
    tdble x3, y3, z3;
    bool startNeeded;

    tdble xprev = 0;
    tdble yprev = 0;

    tDispElt *aDispElt = nullptr;
    unsigned int prevTexId;
    unsigned int curTexId = 0;
    int curTexType = 0;
    int curTexLink = 0;
    tdble curTexOffset = 0;
    tdble curTexSeg;
    tdble curTexSize = 0;
    tdble curTexSize2 = 0;
    tdble curHeight;
    Ac3d::Surface::SURF prevSurfType = Ac3d::Surface::PolygonSingleSidedSmooth;
    Ac3d::Surface::SURF curSurfType = Ac3d::Surface::PolygonSingleSidedSmooth; // shaded single sided polygon
    tTexElt *texList = nullptr;
    tTexElt *curTexElt = nullptr;
    tTrackBarrier *curBarrier;
    tdble texLen;
    tdble texLen2;
    tdble texStep;
    tdble texStep2;
    tdble texMaxT = 0;
    tdble texMaxT2 = 0;
    tTrackPitInfo *pits;
    static int GenTexId = 1;
    tdble runninglentgh;

    tdble tmWidth = Track->graphic.turnMarksInfo.width;
    tdble tmHeight = Track->graphic.turnMarksInfo.height;
    tdble tmVSpace = Track->graphic.turnMarksInfo.vSpace;
    tdble tmHSpace = Track->graphic.turnMarksInfo.hSpace;
    char buf[256];
    bool hasBorder;
    tDispElt *theCurDispElt = nullptr;
    char sname[256];
    //	float		dmax = 0;

#define LG_STEP_MAX 50.0

    printf("++++++++++++ Track ++++++++++++\n");
    printf("name      = %s\n", Track->name);
    printf("authors   = %s\n", Track->authors);
    printf("filename  = %s\n", Track->filename);
    printf("nseg      = %d\n", Track->nseg);
    printf("version   = %d\n", Track->version);
    printf("length    = %f\n", Track->length);
    printf("width     = %f\n", Track->width);
    printf("pits segs = %d\n", Track->pits.nPitSeg);
    printf("pits      = %d\n", Track->pits.nMaxPits);
    printf("XSize     = %f\n", Track->max.x);
    printf("YSize     = %f\n", Track->max.y);
    printf("ZSize     = %f\n", Track->max.z);

    tdble delatx = Track->seg->next->vertex[TR_SL].x - Track->seg->vertex[TR_EL].x;
    tdble delaty = Track->seg->next->vertex[TR_SL].y - Track->seg->vertex[TR_EL].y;
    tdble delatz = Track->seg->next->vertex[TR_SL].z - Track->seg->vertex[TR_EL].z;
    tdble delata = Track->seg->next->angle[TR_ZS] - Track->seg->angle[TR_ZE];
    NORM_PI_PI(delata);

    printf("Delta X   = %f\n", delatx);
    printf("Delta Y   = %f\n", delaty);
    printf("Delta Z   = %f\n", delatz);
    printf("Delta Ang = %f (%f)\n", delata, RAD2DEG(delata));

    /*     if ((fabs(delatx) > 1.0) || (fabs(delaty) > 1.0) || (fabs(delatz) > 1.0)) { */
    /* 	exit(0); */
    /*     } */

    Groups = (tGroup *)calloc(Track->nseg, sizeof(tGroup));
    ActiveGroups = 0;
    GroupNb = Track->nseg;

    width = Track->width;
    // wi2 = width / 2.0; // Never used.

    double rlWidthScale = 1.0;
    double rlWidthScale2 = 1.0;
    double rlOffset = 0.0;
    double rlOffset2 = 0.0;

    if (raceline)
    {
        double SideDistExt = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_RLEXT, nullptr, 2.0);
        double SideDistInt = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_RLINT, nullptr, 2.0);
        if (acc)
        {
            rlWidthScale2 = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_RLWIDTHSCALE, nullptr, 1.0);
            rlOffset2 = (1.0 - 1.0 / rlWidthScale2) / 2.0;
        }
        else
        {
            rlWidthScale = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_RLWIDTHSCALE, nullptr, 1.0);
            rlOffset = (1.0 - 1.0 / rlWidthScale) / 2.0;
        }
        generateRaceLine(Track, SideDistExt, SideDistInt);
    }

    trkpos.type = TR_LPOS_MAIN;

#define SETTEXTURE(texname, texnamebump, texnameraceline, _mipmap)                                                  \
    do                                                                                                              \
    {                                                                                                               \
        int found = 0;                                                                                              \
        curTexElt = texList;                                                                                        \
        if (curTexElt == nullptr)                                                                                   \
        {                                                                                                           \
            curTexElt = (tTexElt *)calloc(1, sizeof(tTexElt));                                                      \
            curTexElt->next = curTexElt;                                                                            \
            texList = curTexElt;                                                                                    \
            curTexElt->name = strdup(texname);                                                                      \
            curTexElt->namebump = strdup(texnamebump);                                                              \
            curTexElt->nameraceline = strdup(texnameraceline);                                                      \
            curTexElt->mipmap = _mipmap;                                                                            \
            curTexElt->texid = GenTexId++;                                                                          \
        }                                                                                                           \
        else                                                                                                        \
        {                                                                                                           \
            do                                                                                                      \
            {                                                                                                       \
                curTexElt = curTexElt->next;                                                                        \
                if (strcmp(texname, curTexElt->name) == 0)                                                          \
                {                                                                                                   \
                    found = 1;                                                                                      \
                    break;                                                                                          \
                }                                                                                                   \
            } while (curTexElt != texList);                                                                         \
            if (!found)                                                                                             \
            {                                                                                                       \
                curTexElt = (tTexElt *)calloc(1, sizeof(tTexElt));                                                  \
                curTexElt->next = texList->next;                                                                    \
                texList->next = curTexElt;                                                                          \
                texList = curTexElt;                                                                                \
                curTexElt->name = strdup(texname);                                                                  \
                curTexElt->namebump = strdup(texnamebump);                                                          \
                curTexElt->nameraceline = strdup(texnameraceline);                                                  \
                curTexElt->mipmap = _mipmap;                                                                        \
                curTexElt->texid = GenTexId++;                                                                      \
            }                                                                                                       \
        }                                                                                                           \
        curTexId = curTexElt->texid;                                                                                \
    } while (0)

#define NBELTLIST 1

#define NEWDISPLIST(texchange, _name, _id)                                                                          \
    do                                                                                                              \
    {                                                                                                               \
        if (!bump || (*(curTexElt->namebump) != 0))                                                                 \
        {                                                                                                           \
            if (theCurDispElt != nullptr)                                                                           \
            {                                                                                                       \
                startNeeded = texchange;                                                                            \
                if (theCurDispElt->start != nbvert)                                                                 \
                {                                                                                                   \
                    theCurDispElt->nb = nbvert - theCurDispElt->start;                                              \
                    theCurDispElt = aDispElt = (tDispElt *)malloc(sizeof(tDispElt));                                \
                    aDispElt->start = nbvert;                                                                       \
                    aDispElt->nb = 0;                                                                               \
                    aDispElt->name = strdup(_name);                                                                 \
                    aDispElt->id = _id;                                                                             \
                    aDispElt->texture = curTexElt;                                                                  \
                    aDispElt->surfType = curSurfType;                                                               \
                    if (Groups[_id].nb == 0)                                                                        \
                    {                                                                                               \
                        ActiveGroups++;                                                                             \
                        aDispElt->next = aDispElt;                                                                  \
                        Groups[_id].dispList = aDispElt;                                                            \
                    }                                                                                               \
                    else                                                                                            \
                    {                                                                                               \
                        aDispElt->next = Groups[_id].dispList->next;                                                \
                        Groups[_id].dispList->next = aDispElt;                                                      \
                        Groups[_id].dispList = aDispElt;                                                            \
                    }                                                                                               \
                    Groups[_id].nb++;                                                                               \
                }                                                                                                   \
                else                                                                                                \
                {                                                                                                   \
                    aDispElt->texture = curTexElt;                                                                  \
                    aDispElt->surfType = curSurfType;                                                               \
                }                                                                                                   \
            }                                                                                                       \
            else                                                                                                    \
            {                                                                                                       \
                theCurDispElt = aDispElt = (tDispElt *)malloc(sizeof(tDispElt));                                    \
                aDispElt->start = nbvert;                                                                           \
                aDispElt->nb = 0;                                                                                   \
                aDispElt->name = strdup(_name);                                                                     \
                aDispElt->id = _id;                                                                                 \
                aDispElt->texture = curTexElt;                                                                      \
                aDispElt->surfType = curSurfType;                                                                   \
                aDispElt->next = aDispElt;                                                                          \
                Groups[_id].dispList = aDispElt;                                                                    \
                Groups[_id].nb++;                                                                                   \
                ActiveGroups++;                                                                                     \
            }                                                                                                       \
            /* fprintf(stderr, "%s %d (%s)\n", _name, _id, curTexElt->name); */                                     \
        }                                                                                                           \
    } while (0)

#define CHECKDISPLIST(mat, name, id, off)                                                                           \
    do                                                                                                              \
    {                                                                                                               \
        const char *texname;                                                                                        \
        const char *texnamebump;                                                                                    \
        const char *texnameraceline;                                                                                \
        int mipmap;                                                                                                 \
        static char path_[256];                                                                                     \
        if (Track->version < 4)                                                                                     \
        {                                                                                                           \
            sprintf(path_, "%s/%s/%s", TRK_SECT_SURFACES, TRK_LST_SURF, mat);                                       \
        }                                                                                                           \
        else                                                                                                        \
        {                                                                                                           \
            sprintf(path_, "%s/%s", TRK_SECT_SURFACES, mat);                                                        \
        }                                                                                                           \
        texnamebump = GfParmGetStr(TrackHandle, path_, TRK_ATT_BUMPNAME, "");                                       \
        texnameraceline = GfParmGetStr(TrackHandle, path_, TRK_ATT_RACELINENAME, "raceline.png");                   \
        texname = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXTURE, "tr-asphalt.png");                              \
        mipmap = (int)GfParmGetNum(TrackHandle, path_, TRK_ATT_TEXMIPMAP, nullptr, 0);                              \
        SETTEXTURE(texname, texnamebump, texnameraceline, mipmap);                                                  \
        if ((curTexId != prevTexId) || (curSurfType != prevSurfType) || startNeeded)                                \
        {                                                                                                           \
            const char *textype;                                                                                    \
            if (bump)                                                                                               \
            {                                                                                                       \
                curTexType = 1;                                                                                     \
                curTexLink = 1;                                                                                     \
                curTexOffset = -off;                                                                                \
                curTexSize = GfParmGetNum(TrackHandle, path_, TRK_ATT_BUMPSIZE, nullptr, 20.0);                     \
            }                                                                                                       \
            else                                                                                                    \
            {                                                                                                       \
                textype = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXTYPE, "continuous");                          \
                if (strcmp(textype, "continuous") == 0)                                                             \
                    curTexType = 1;                                                                                 \
                else                                                                                                \
                    curTexType = 0;                                                                                 \
                textype = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXLINK, TRK_VAL_YES);                           \
                if (strcmp(textype, TRK_VAL_YES) == 0)                                                              \
                    curTexLink = 1;                                                                                 \
                else                                                                                                \
                    curTexLink = 0;                                                                                 \
                textype = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXSTARTBOUNDARY, TRK_VAL_NO);                   \
                if (strcmp(textype, TRK_VAL_YES) == 0)                                                              \
                    curTexOffset = -off;                                                                            \
                else                                                                                                \
                    curTexOffset = 0;                                                                               \
                curTexSize = GfParmGetNum(TrackHandle, path_, TRK_ATT_TEXSIZE, nullptr, 20.0);                      \
            }                                                                                                       \
            prevTexId = curTexId;                                                                                   \
            prevSurfType = curSurfType;                                                                             \
            NEWDISPLIST(true, name, id);                                                                            \
        }                                                                                                           \
    } while (0)

#define CHECKDISPLIST2(texture, mipmap, name, id)                                                                   \
    do                                                                                                              \
    {                                                                                                               \
        char texname[300];                                                                                          \
        sprintf(texname, "%s.png", texture);                                                                        \
        SETTEXTURE(texname, "", "", mipmap);                                                                        \
        if (curTexId != prevTexId)                                                                                  \
        {                                                                                                           \
            prevTexId = curTexId;                                                                                   \
            NEWDISPLIST(true, name, id);                                                                            \
        }                                                                                                           \
    } while (0)

#define CHECKDISPLIST3(texture, mipmap, name, id)                                                                   \
    do                                                                                                              \
    {                                                                                                               \
        SETTEXTURE(texture, "", "", mipmap);                                                                        \
        if (curTexId != prevTexId)                                                                                  \
        {                                                                                                           \
            prevTexId = curTexId;                                                                                   \
            NEWDISPLIST(true, name, id);                                                                            \
        }                                                                                                           \
    } while (0)

#define CLOSEDISPLIST()                                                                                             \
    do                                                                                                              \
    {                                                                                                               \
        theCurDispElt->nb = nbvert - theCurDispElt->start;                                                          \
    } while (0)

    /* Count the number of vertice to allocate */
    nbvert = 0;
    for (i = 0, seg = Track->seg->next; i < Track->nseg; i++, seg = seg->next)
    {
        nbvert++;
        switch (seg->type)
        {
        case TR_STR:
            nbvert += (int)(seg->length / LMAX);
            break;
        case TR_LFT:
            nbvert += (int)(seg->arc * (seg->radiusr) / LMAX);
            break;
        case TR_RGT:
            nbvert += (int)(seg->arc * (seg->radiusl) / LMAX);
            break;
        }
    }

    nbvert++;
    nbvert *= 30;
    nbvert += 58; /* start bridge */
    nbvert += 12 + 10 * Track->pits.driversPitsNb;
    // TODO: Investigate if this is a feasible solution, nbvert was perviously += 1000.
    nbvert += 10000; /* safety margin */
    printf("=== Indices Array Size   = %d\n", nbvert);
    printf("=== Vertex Array Size    = %d\n", nbvert * 3);
    printf("=== Tex Coord Array Size = %d\n", nbvert * 2);
    trackindices = (unsigned int *)malloc(sizeof(unsigned int) * nbvert);
    trackvertices = (float *)malloc(sizeof(GLfloat) * (nbvert * 3));
    tracktexcoord = (float *)malloc(sizeof(GLfloat) * (nbvert * 2));
    if (raceline && acc)
        tracktexcoord2 = (float *)malloc(sizeof(GLfloat) * (nbvert * 2));

    nbvert = 0;

    /* Main track */
    prevTexId = 0;
    texLen = 0;
    startNeeded = true;
    runninglentgh = 0;
    for (i = 0, seg = Track->seg->next; i < Track->nseg; i++, seg = seg->next)
    {
        CHECKDISPLIST(seg->surface->material, "tkMn", i, seg->lgfromstart);
        if (!curTexLink)
        {
            curTexSeg = 0;
        }
        else
        {
            curTexSeg = seg->lgfromstart;
        }
        curTexSeg += curTexOffset;

        if (raceline)
        {
            // Required if some smaller than width tiled texture is used.
            if (acc)
                curTexSize2 = seg->width;
            else
                curTexSize = seg->width;
        }

        texLen = curTexSeg / curTexSize;
        texLen2 = curTexSeg / curTexSize2;
        if (startNeeded || (runninglentgh > LG_STEP_MAX))
        {
            NEWDISPLIST(false, "tkMn", i);
            runninglentgh = 0;
            ts = 0;
            if (raceline)
            {
                // Required if some smaller than width tiled texture is used.
                if (acc)
                {
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));
                    texMaxT2 = 1.0;
                }
                else
                    texMaxT = 1.0;
            }
            else
            {
                // Normal case
                texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));
            }

            const double rlto = getTexureOffset(seg->lgfromstart) / rlWidthScale;
            const double rlto2 = getTexureOffset(seg->lgfromstart) / rlWidthScale2;

            if (raceline && acc)
            {
                SETPOINT2(curTexElt, nbvert, texLen, texMaxT, texLen2, texMaxT2 - rlOffset2 + rlto2, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                SETPOINT2(curTexElt, nbvert, texLen, 0.0, texLen2, 0.0 + rlOffset2 + rlto2, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
            }
            else
            {
                SETPOINT(curTexElt, nbvert, texLen, texMaxT - rlOffset + rlto, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                SETPOINT(curTexElt, nbvert, texLen, 0.0 + rlOffset + rlto, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
            }
        }

        switch (seg->type)
        {
        case TR_STR:
            ts = LMAX;
            texStep = LMAX / curTexSize;
            texStep2 = LMAX / curTexSize2;
            trkpos.seg = seg;
            while (ts < seg->length)
            {
                texLen += texStep;
                texLen2 += texStep2;
                trkpos.toStart = ts;

                trkpos.toRight = width;
                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);

                const double rlto = getTexureOffset(seg->lgfromstart + ts) / rlWidthScale;
                const double rlto2 = getTexureOffset(seg->lgfromstart + ts) / rlWidthScale2;

                if (raceline && acc)
                {
                    SETPOINT2(curTexElt, nbvert, texLen, texMaxT, texLen2, texMaxT2 - rlOffset2 + rlto2, x, y, RtTrackHeightL(&trkpos));
                }
                else
                {
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT - rlOffset + rlto, x, y, RtTrackHeightL(&trkpos));
                }

                trkpos.toRight = 0;
                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);

                if (raceline && acc)
                {
                    SETPOINT2(curTexElt, nbvert, texLen, 0.0, texLen2, 0.0 + rlOffset2 + rlto2, x, y, RtTrackHeightL(&trkpos));
                }
                else
                {
                    SETPOINT(curTexElt, nbvert, texLen, 0.0 + rlOffset + rlto, x, y, RtTrackHeightL(&trkpos));
                }

                ts += LMAX;
            }
            ts = seg->length;
            break;
        case TR_LFT:
            step = LMAX / (seg->radiusr);
            texStep = step * seg->radius / curTexSize;
            texStep2 = step * seg->radius / curTexSize2;
            anz = seg->angle[TR_ZS] + step;
            ts = step;
            radiusr = seg->radiusr;
            radiusl = seg->radiusl;
            trkpos.seg = seg;
            while (anz < seg->angle[TR_ZE])
            {
                texLen += texStep;
                texLen2 += texStep2;
                trkpos.toStart = ts;

                const double rlto = getTexureOffset(seg->lgfromstart + ts * seg->radius) / rlWidthScale;
                const double rlto2 = getTexureOffset(seg->lgfromstart + ts * seg->radius) / rlWidthScale2;
                /* left */
                trkpos.toRight = width;
                if (raceline && acc)
                {
                    SETPOINT2(curTexElt, nbvert, texLen, texMaxT, texLen2, texMaxT2 - rlOffset2 + rlto2, seg->center.x + radiusl * sin(anz), seg->center.y - radiusl * cos(anz), RtTrackHeightL(&trkpos));
                }
                else
                {
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT - rlOffset + rlto, seg->center.x + radiusl * sin(anz), seg->center.y - radiusl * cos(anz), RtTrackHeightL(&trkpos));
                }

                /* right */
                trkpos.toRight = 0;
                if (raceline && acc)
                {
                    SETPOINT2(curTexElt, nbvert, texLen, 0.0, texLen2, 0.0 + rlOffset2 + rlto2, seg->center.x + radiusr * sin(anz), seg->center.y - radiusr * cos(anz), RtTrackHeightL(&trkpos));
                }
                else
                {
                    SETPOINT(curTexElt, nbvert, texLen, 0.0 + rlOffset + rlto, seg->center.x + radiusr * sin(anz), seg->center.y - radiusr * cos(anz), RtTrackHeightL(&trkpos));
                }

                ts += step;
                anz += step;
            }
            ts = seg->arc;
            break;
        case TR_RGT:
            step = LMAX / (seg->radiusl);
            texStep = step * seg->radius / curTexSize;
            texStep2 = step * seg->radius / curTexSize2;
            anz = seg->angle[TR_ZS] - step;
            ts = step;
            radiusr = seg->radiusr;
            radiusl = seg->radiusl;
            trkpos.seg = seg;
            while (anz > seg->angle[TR_ZE])
            {
                texLen += texStep;
                texLen2 += texStep2;
                trkpos.toStart = ts;

                const double rlto = getTexureOffset(seg->lgfromstart + ts * seg->radius) / rlWidthScale;
                const double rlto2 = getTexureOffset(seg->lgfromstart + ts * seg->radius) / rlWidthScale2;
                /* left */
                trkpos.toRight = width;
                if (raceline && acc)
                {
                    SETPOINT2(curTexElt, nbvert, texLen, texMaxT, texLen2, texMaxT2 - rlOffset2 + rlto2, seg->center.x - radiusl * sin(anz), seg->center.y + radiusl * cos(anz), RtTrackHeightL(&trkpos));
                }
                else
                {
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT - rlOffset + rlto, seg->center.x - radiusl * sin(anz), seg->center.y + radiusl * cos(anz), RtTrackHeightL(&trkpos));
                }

                /* right */
                trkpos.toRight = 0;
                if (raceline && acc)
                {
                    SETPOINT2(curTexElt, nbvert, texLen, 0, texLen2, 0 + rlOffset2 + rlto2, seg->center.x - radiusr * sin(anz), seg->center.y + radiusr * cos(anz), RtTrackHeightL(&trkpos));
                }
                else
                {
                    SETPOINT(curTexElt, nbvert, texLen, 0 + rlOffset + rlto, seg->center.x - radiusr * sin(anz), seg->center.y + radiusr * cos(anz), RtTrackHeightL(&trkpos));
                }

                ts += step;
                anz -= step;
            }
            ts = seg->arc;
            break;
        }
        texLen = (curTexSeg + seg->length) / curTexSize;
        texLen2 = (curTexSeg + seg->length) / curTexSize2;

        const double rlto = getTexureOffset(seg->lgfromstart + seg->length) / rlWidthScale;
        const double rlto2 = getTexureOffset(seg->lgfromstart + seg->length) / rlWidthScale2;
        if (raceline && acc)
        {
            SETPOINT2(curTexElt, nbvert, texLen, texMaxT, texLen2, texMaxT2 - rlOffset2 + rlto2, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
            SETPOINT2(curTexElt, nbvert, texLen, 0, texLen2, 0 + rlOffset2 + rlto2, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
        }
        else
        {
            SETPOINT(curTexElt, nbvert, texLen, texMaxT - rlOffset + rlto, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
            SETPOINT(curTexElt, nbvert, texLen, 0 + rlOffset + rlto, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
        }

        startNeeded = false;
        runninglentgh += seg->length;
    }

    if (raceline && !acc)
    {
        CLOSEDISPLIST();
        printf("=== Indices really used = %d\n", nbvert);
        return 0;
    }

    /* Right Border */
    lastSeg = nullptr;
    for (int j = 0; j < 3; j++)
    {
        prevTexId = 0;
        texLen = 0;
        startNeeded = true;
        runninglentgh = 0;
        sprintf(sname, "t%dRB", j);
        for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next)
        {
            if ((mseg->rside != nullptr) && (mseg->rside->type2 == TR_RBORDER))
            {
                seg = mseg->rside;
                if ((lastSeg && lastSeg->style != seg->style) || mseg->prev->rside->width != seg->width || mseg->prev->rside->height != seg->height)
                    startNeeded = true;
                CHECKDISPLIST(seg->surface->material, sname, i, mseg->lgfromstart);
                if (!curTexLink)
                {
                    curTexSeg = 0;
                }
                else
                {
                    curTexSeg = mseg->lgfromstart;
                }
                curTexSeg += curTexOffset;
                texLen = curTexSeg / curTexSize;
                if (startNeeded || (runninglentgh > LG_STEP_MAX))
                {
                    NEWDISPLIST(false, sname, i);
                    runninglentgh = 0;
                    ts = 0;

                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                    switch (seg->style)
                    {
                    case TR_PLAN:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                        }
                        break;
                    case TR_CURB:
                        switch (j)
                        {
                        case 0:
                            if (!mseg->prev->rside || (mseg->prev->rside->type2 != TR_RBORDER) || (mseg->prev->rside->style != TR_CURB) ||
                                (mseg->prev->rside->style == TR_CURB && (mseg->prev->rside->width != seg->width || mseg->prev->rside->height != seg->height)))
                            {
                                SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z - 0.1);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                            }
                            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height);
                            break;
                        case 1:
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height);
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                            break;
                        case 2:
                            break;
                        }
                        break;
                    case TR_WALL:
                        switch (j)
                        {
                        case 0:
                            SETPOINT(curTexElt, nbvert, -texLen, 0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            SETPOINT(curTexElt, nbvert, -texLen, .33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height);
                            break;
                        case 1:
                            SETPOINT(curTexElt, nbvert, -texLen, 0.33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height);
                            break;
                        case 2:
                            if (!mseg->prev->rside || (mseg->prev->rside->type2 != TR_RBORDER) || (mseg->prev->rside->style != TR_WALL) ||
                                (mseg->prev->rside->style == TR_WALL && (mseg->prev->rside->height < seg->height || mseg->prev->rside->width < seg->width)))
                            {
                                SETPOINT(curTexElt, nbvert, -texLen - seg->width / curTexSize, 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height);
                                SETPOINT(curTexElt, nbvert, -texLen - seg->width / curTexSize, 1.00, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            }
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                            break;
                        }
                        break;
                    case TR_FENCE:
                    case TR_FENCE1:
                    case TR_FENCE2:
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // not supported
                        break;
                    }
                }

                switch (seg->type)
                {
                case TR_STR:
                    ts = LMAX;
                    texStep = LMAX / curTexSize;
                    texLen += texStep;
                    trkpos.seg = seg;
                    while (ts < seg->length)
                    {
                        trkpos.toStart = ts;
                        width = RtTrackGetWidth(seg, ts);
                        texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                        switch (seg->style)
                        {
                        case TR_PLAN:
                            if (j == 0)
                            {
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                            }
                            break;
                        case TR_CURB:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                break;
                            case 2:
                                break;
                            }
                            break;
                        case TR_WALL:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 2:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            }
                            break;
                        case TR_FENCE:
                        case TR_FENCE1:
                        case TR_FENCE2:
                        case TR_PITBUILDING:
                        case TR_NO_BARRIER:
                            // not supported
                            break;
                        }

                        ts += LMAX;
                        texLen += texStep;
                    }
                    ts = seg->length;
                    break;
                case TR_LFT:
                    step = LMAX / (mseg->radiusr);
                    texStep = step * mseg->radius / curTexSize;
                    anz = seg->angle[TR_ZS] + step;
                    ts = step;
                    texLen += texStep;
                    radiusr = seg->radiusr;
                    radiusl = seg->radiusl;
                    trkpos.seg = seg;
                    while (anz < seg->angle[TR_ZE])
                    {
                        trkpos.toStart = ts;
                        width = RtTrackGetWidth(seg, ts);
                        texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                        switch (seg->style)
                        {
                        case TR_PLAN:
                            if (j == 0)
                            {
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                            }
                            break;
                        case TR_CURB:
                            switch (j)
                            {
                            case 0:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                break;
                            case 2:
                                break;
                            }
                            break;
                        case TR_WALL:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 2:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            }
                            break;
                        case TR_FENCE:
                        case TR_FENCE1:
                        case TR_FENCE2:
                        case TR_PITBUILDING:
                        case TR_NO_BARRIER:
                            // not supported
                            break;
                        }

                        ts += step;
                        texLen += texStep;
                        anz += step;
                    }
                    ts = seg->arc;
                    break;
                case TR_RGT:
                    step = LMAX / (mseg->radiusl);
                    texStep = step * mseg->radius / curTexSize;
                    anz = seg->angle[TR_ZS] - step;
                    ts = step;
                    texLen += texStep;
                    radiusr = seg->radiusr;
                    radiusl = seg->radiusl;
                    trkpos.seg = seg;
                    while (anz > seg->angle[TR_ZE])
                    {
                        trkpos.toStart = ts;
                        width = RtTrackGetWidth(seg, ts);
                        texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                        switch (seg->style)
                        {
                        case TR_PLAN:
                            if (j == 0)
                            {
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                            }
                            break;
                        case TR_CURB:
                            switch (j)
                            {
                            case 0:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                break;
                            case 2:
                                break;
                            }
                            break;
                        case TR_WALL:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 2:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            }
                            break;
                        case TR_FENCE:
                        case TR_FENCE1:
                        case TR_FENCE2:
                        case TR_PITBUILDING:
                        case TR_NO_BARRIER:
                            // not supported
                            break;
                        }

                        ts += step;
                        texLen += texStep;
                        anz -= step;
                    }
                    ts = seg->arc;
                    break;
                }
                texLen = (curTexSeg + mseg->length) / curTexSize;

                width = RtTrackGetWidth(seg, ts);
                texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                switch (seg->style)
                {
                case TR_PLAN:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                    }
                    break;
                case TR_CURB:
                    switch (j)
                    {
                    case 0:
                        SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height);
                        if (mseg->next->rside && ((mseg->next->rside->type2 != TR_RBORDER) || (mseg->next->rside->style != TR_CURB) ||
                            (mseg->next->rside->style == TR_CURB && (mseg->next->rside->width != seg->width || mseg->next->rside->height != seg->height))))
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z - 0.1);
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                        }
                        break;
                    case 1:
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height);
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                        break;
                    case 2:
                        break;
                    }
                    break;
                case TR_WALL:
                    switch (j)
                    {
                    case 0:
                        SETPOINT(curTexElt, nbvert, -texLen, 0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                        SETPOINT(curTexElt, nbvert, -texLen, 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height);
                        break;
                    case 1:
                        SETPOINT(curTexElt, nbvert, -texLen, 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height);
                        SETPOINT(curTexElt, nbvert, -texLen, 0.66, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height);
                        break;
                    case 2:
                        SETPOINT(curTexElt, nbvert, -texLen, 0.66, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height);
                        SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                        if (mseg->next->rside && ((mseg->next->rside->type2 != TR_RBORDER) || (mseg->next->rside->style != TR_WALL) ||
                            (mseg->next->rside->style == TR_WALL && (mseg->next->rside->height < seg->height || mseg->next->rside->width < seg->width))))
                        {
                            SETPOINT(curTexElt, nbvert, -texLen + seg->width / curTexSize, 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height);
                            SETPOINT(curTexElt, nbvert, -texLen + seg->width / curTexSize, 1.00, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                        }
                        break;
                    }
                    break;
                case TR_FENCE:
                case TR_FENCE1:
                case TR_FENCE2:
                case TR_PITBUILDING:
                case TR_NO_BARRIER:
                    // not supported
                    break;
                }

                startNeeded = false;
                runninglentgh += seg->length;
                lastSeg = seg;
            }
            else
            {
                startNeeded = true;
                lastSeg = nullptr;
            }
        }
    }

    /* Right Side */
    prevTexId = 0;
    texLen = 0;
    startNeeded = true;
    runninglentgh = 0;
    hasBorder = false;
    lastSeg = nullptr;
    for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next)
    {
        if ((mseg->rside != nullptr) && ((mseg->rside->type2 == TR_RSIDE) || (mseg->rside->rside != nullptr)))
        {
            seg = mseg->rside;
            if (seg->rside != nullptr)
            {
                seg = seg->rside;
                if (!hasBorder)
                {
                    startNeeded = true;
                    hasBorder = true;
                }
            }
            else
            {
                if (hasBorder)
                {
                    startNeeded = true;
                    hasBorder = false;
                }
            }

            if (!startNeeded)
            {
                tdble d0 = Distance(lastSeg->vertex[TR_EL].x, lastSeg->vertex[TR_EL].y, lastSeg->vertex[TR_EL].z, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                tdble d1 = Distance(lastSeg->vertex[TR_ER].x, lastSeg->vertex[TR_ER].y, lastSeg->vertex[TR_ER].z, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                if ((d0 > 0.01) || (d1 > 0.01))
                    startNeeded = true;
            }

            CHECKDISPLIST(seg->surface->material, "tkRS", i, mseg->lgfromstart);
            if (!curTexLink)
            {
                curTexSeg = 0;
            }
            else
            {
                curTexSeg = mseg->lgfromstart;
            }
            curTexSeg += curTexOffset;
            texLen = curTexSeg / curTexSize;
            if (startNeeded || (runninglentgh > LG_STEP_MAX))
            {
                NEWDISPLIST(false, "tkRS", i);
                runninglentgh = 0;
                ts = 0;

                width = RtTrackGetWidth(seg, ts);
                texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
            }

            switch (seg->type)
            {
            case TR_STR:
                ts = LMAX;
                texStep = LMAX / curTexSize;
                texLen += texStep;
                trkpos.seg = seg;
                while (ts < seg->length)
                {
                    trkpos.toStart = ts;

                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                    trkpos.toRight = width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));

                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));

                    ts += LMAX;
                    texLen += texStep;
                }
                ts = seg->length;
                break;
            case TR_LFT:
                step = LMAX / (mseg->radiusr);
                texStep = step * mseg->radius / curTexSize;
                anz = seg->angle[TR_ZS] + step;
                ts = step;
                texLen += texStep;
                radiusr = seg->radiusr;
                radiusl = seg->radiusl;
                trkpos.seg = seg;
                while (anz < seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                    /* left */
                    trkpos.toRight = width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));

                    /* right */
                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));

                    ts += step;
                    texLen += texStep;
                    anz += step;
                }
                ts = seg->arc;
                break;
            case TR_RGT:
                step = LMAX / (mseg->radiusl);
                texStep = step * mseg->radius / curTexSize;
                anz = seg->angle[TR_ZS] - step;
                ts = step;
                texLen += texStep;
                radiusr = seg->radiusr;
                radiusl = seg->radiusl;
                trkpos.seg = seg;
                while (anz > seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                    /* left */
                    trkpos.toRight = width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));

                    /* right */
                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));

                    ts += step;
                    texLen += texStep;
                    anz -= step;
                }
                ts = seg->arc;
                break;
            }
            texLen = (curTexSeg + mseg->length) / curTexSize;

            width = RtTrackGetWidth(seg, ts);
            texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);

            startNeeded = false;
            runninglentgh += seg->length;
            lastSeg = seg;
        }
        else
        {
            startNeeded = true;
        }
    }

    /* Left Border */
    lastSeg = nullptr;
    for (int j = 0; j < 3; j++)
    {
        prevTexId = 0;
        texLen = 0;
        startNeeded = true;
        runninglentgh = 0;
        sprintf(sname, "t%dLB", j);
        for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next)
        {
            if ((mseg->lside != nullptr) && (mseg->lside->type2 == TR_LBORDER))
            {
                seg = mseg->lside;
                if ((lastSeg && lastSeg->style != seg->style) || mseg->prev->lside->width != seg->width || mseg->prev->lside->height != seg->height)
                    startNeeded = true;
                CHECKDISPLIST(seg->surface->material, sname, i, mseg->lgfromstart);
                if (!curTexLink)
                {
                    curTexSeg = 0;
                }
                else
                {
                    curTexSeg = mseg->lgfromstart;
                }
                curTexSeg += curTexOffset;
                texLen = curTexSeg / curTexSize;
                if (startNeeded || (runninglentgh > LG_STEP_MAX))
                {
                    NEWDISPLIST(false, sname, i);
                    runninglentgh = 0;
                    ts = 0;
                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));
                    switch (seg->style)
                    {
                    case TR_PLAN:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                        }
                        break;
                    case TR_CURB:
                        switch (j)
                        {
                        case 0:
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height);
                            break;
                        case 1:
                            if (!mseg->prev->lside || (mseg->prev->lside->type2 != TR_LBORDER) || (mseg->prev->lside->style != TR_CURB) ||
                                (mseg->prev->lside->style == TR_CURB && (mseg->prev->lside->width != seg->width || mseg->prev->lside->height != seg->height)))
                            {
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                                SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z - 0.1);
                            }
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                            break;
                        case 2:
                            break;
                        }
                        break;
                    case TR_WALL:
                       switch (j)
                        {
                        case 0:
                            if (!mseg->prev->lside || (mseg->prev->lside->type2 != TR_LBORDER) || (mseg->prev->lside->style != TR_WALL) ||
                                (mseg->prev->lside->style == TR_WALL && (mseg->prev->lside->height < seg->height || mseg->prev->lside->width < seg->width)))
                            {
                                SETPOINT(curTexElt, nbvert, texLen - seg->width / curTexSize, 1.00, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                                SETPOINT(curTexElt, nbvert, texLen - seg->width / curTexSize, 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height);
                            }
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height);
                            break;
                        case 1:
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height);
                            break;
                        case 2:
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                            break;
                        }
                        break;
                    case TR_FENCE:
                    case TR_FENCE1:
                    case TR_FENCE2:
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // not supported
                        break;
                    }
                }

                switch (seg->type)
                {
                case TR_STR:
                    ts = LMAX;
                    texStep = LMAX / curTexSize;
                    texLen += texStep;
                    trkpos.seg = seg;
                    while (ts < seg->length)
                    {
                        trkpos.toStart = ts;
                        width = RtTrackGetWidth(seg, ts);
                        texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                        switch (seg->style)
                        {
                        case TR_PLAN:
                            if (j == 0)
                            {
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                            }
                            break;
                        case TR_CURB:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            case 2:
                                break;
                            }
                            break;
                        case TR_WALL:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 2:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            }
                            break;
                        case TR_FENCE:
                        case TR_FENCE1:
                        case TR_FENCE2:
                        case TR_PITBUILDING:
                        case TR_NO_BARRIER:
                            // not supported
                            break;
                        }

                        ts += LMAX;
                        texLen += texStep;
                    }
                    ts = seg->length;
                    break;
                case TR_LFT:
                    step = LMAX / (mseg->radiusr);
                    texStep = step * mseg->radius / curTexSize;
                    anz = seg->angle[TR_ZS] + step;
                    ts = step;
                    texLen += texStep;
                    radiusr = seg->radiusr;
                    radiusl = seg->radiusl;
                    trkpos.seg = seg;
                    while (anz < seg->angle[TR_ZE])
                    {
                        trkpos.toStart = ts;
                        width = RtTrackGetWidth(seg, ts);
                        texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                        switch (seg->style)
                        {
                        case TR_PLAN:
                            if (j == 0)
                            {
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                            }
                            break;
                        case TR_CURB:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            case 2:
                                break;
                            }
                            break;
                        case TR_WALL:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 2:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            }
                            break;
                        case TR_FENCE:
                        case TR_FENCE1:
                        case TR_FENCE2:
                        case TR_PITBUILDING:
                        case TR_NO_BARRIER:
                            // not supported
                            break;
                        }

                        ts += step;
                        texLen += texStep;
                        anz += step;
                    }
                    ts = seg->arc;
                    break;
                case TR_RGT:
                    step = LMAX / (mseg->radiusl);
                    texStep = step * mseg->radius / curTexSize;
                    anz = seg->angle[TR_ZS] - step;
                    ts = step;
                    texLen += texStep;
                    radiusr = seg->radiusr;
                    radiusl = seg->radiusl;
                    trkpos.seg = seg;
                    while (anz > seg->angle[TR_ZE])
                    {
                        trkpos.toStart = ts;
                        width = RtTrackGetWidth(seg, ts);
                        texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                        switch (seg->style)
                        {
                        case TR_PLAN:
                            if (j == 0)
                            {
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                            }
                            break;
                        case TR_CURB:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            case 2:
                                break;
                            }
                            break;
                        case TR_WALL:
                            switch (j)
                            {
                            case 0:
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, RtTrackHeightL(&trkpos));
                                SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 1:
                                /* left */
                                trkpos.toRight = width;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                /* right */
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                break;
                            case 2:
                                trkpos.toRight = 0;
                                RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                                SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height);
                                SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, RtTrackHeightL(&trkpos));
                                break;
                            }
                            break;
                        case TR_FENCE:
                        case TR_FENCE1:
                        case TR_FENCE2:
                        case TR_PITBUILDING:
                        case TR_NO_BARRIER:
                            // not supported
                            break;
                        }

                        ts += step;
                        texLen += texStep;
                        anz -= step;
                    }
                    ts = seg->arc;
                    break;
                }
                texLen = (curTexSeg + mseg->length) / curTexSize;

                width = RtTrackGetWidth(seg, ts);
                texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                switch (seg->style)
                {
                case TR_PLAN:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                        SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                    }
                    break;
                case TR_CURB:
                    switch (j)
                    {
                    case 0:
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height);
                        break;
                    case 1:
                        SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                        if (mseg->next->lside && ((mseg->next->lside->type2 != TR_LBORDER) || (mseg->next->lside->style != TR_CURB) ||
                            (mseg->next->lside->style == TR_CURB && (mseg->next->lside->height != seg->height || mseg->next->lside->width != seg->width))))
                        {
                            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z - 0.1);
                        }
                        break;
                    case 2:
                        break;
                    }
                    break;
                case TR_WALL:
                    switch (j)
                    {
                    case 0:
                        SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                        SETPOINT(curTexElt, nbvert, texLen, 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height);
                        if (mseg->next->lside && ((mseg->next->lside->type2 != TR_LBORDER) || (mseg->next->lside->style != TR_WALL) ||
                            (mseg->next->lside->style == TR_WALL && (mseg->next->lside->height < seg->height || mseg->next->lside->width < seg->width))))
                        {
                            SETPOINT(curTexElt, nbvert, texLen + seg->width / curTexSize, 1.00, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                            SETPOINT(curTexElt, nbvert, texLen + seg->width / curTexSize, 0.66, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height);
                        }
                        break;
                    case 1:
                        SETPOINT(curTexElt, nbvert, texLen, 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height);
                        break;
                    case 2:
                        SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                        break;
                    }
                    break;
                case TR_FENCE:
                case TR_FENCE1:
                case TR_FENCE2:
                case TR_PITBUILDING:
                case TR_NO_BARRIER:
                    // not supported
                    break;
                }

                startNeeded = false;
                runninglentgh += seg->length;
                lastSeg = seg;
            }
            else
            {
                startNeeded = true;
                lastSeg = nullptr;
            }
        }
    }

    /* Left Side */
    prevTexId = 0;
    texLen = 0;
    startNeeded = true;
    runninglentgh = 0;
    hasBorder = false;
    lastSeg = nullptr;
    for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next)
    {
        if ((mseg->lside != nullptr) && ((mseg->lside->type2 == TR_LSIDE) || (mseg->lside->lside != nullptr)))
        {
            seg = mseg->lside;
            if (seg->lside)
            {
                seg = seg->lside;
                if (!hasBorder)
                {
                    startNeeded = true;
                    hasBorder = true;
                }
            }
            else
            {
                if (hasBorder)
                {
                    startNeeded = true;
                    hasBorder = false;
                }
            }

            if (!startNeeded)
            {
                tdble d0 = Distance(lastSeg->vertex[TR_EL].x, lastSeg->vertex[TR_EL].y, lastSeg->vertex[TR_EL].z, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                tdble d1 = Distance(lastSeg->vertex[TR_ER].x, lastSeg->vertex[TR_ER].y, lastSeg->vertex[TR_ER].z, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                if ((d0 > 0.01) || (d1 > 0.01))
                    startNeeded = true;
            }

            CHECKDISPLIST(seg->surface->material, "tkLS", i, mseg->lgfromstart);
            if (!curTexLink)
            {
                curTexSeg = 0;
            }
            else
            {
                curTexSeg = mseg->lgfromstart;
            }
            curTexSeg += curTexOffset;
            texLen = curTexSeg / curTexSize;
            if (startNeeded || (runninglentgh > LG_STEP_MAX))
            {
                NEWDISPLIST(false, "tkLS", i);
                runninglentgh = 0;

                ts = 0;
                width = RtTrackGetWidth(seg, ts);
                texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));
                SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
            }

            switch (seg->type)
            {
            case TR_STR:
                ts = LMAX;
                texStep = LMAX / curTexSize;
                texLen += texStep;
                trkpos.seg = seg;
                while (ts < seg->length)
                {
                    trkpos.toStart = ts;

                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                    trkpos.toRight = width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));

                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));

                    ts += LMAX;
                    texLen += texStep;
                }
                ts = seg->length;
                break;
            case TR_LFT:
                step = LMAX / (mseg->radiusr);
                texStep = step * mseg->radius / curTexSize;
                anz = seg->angle[TR_ZS] + step;
                ts = step;
                texLen += texStep;
                radiusr = seg->radiusr;
                radiusl = seg->radiusl;
                trkpos.seg = seg;
                while (anz < seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                    /* left */
                    trkpos.toRight = width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));

                    /* right */
                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));

                    ts += step;
                    texLen += texStep;
                    anz += step;
                }
                ts = seg->arc;
                break;
            case TR_RGT:
                step = LMAX / (mseg->radiusl);
                texStep = step * mseg->radius / curTexSize;
                anz = seg->angle[TR_ZS] - step;
                ts = step;
                texLen += texStep;
                radiusr = seg->radiusr;
                radiusl = seg->radiusl;
                trkpos.seg = seg;
                while (anz > seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    width = RtTrackGetWidth(seg, ts);
                    texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

                    /* left */
                    trkpos.toRight = width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos));

                    /* right */
                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 0, x, y, RtTrackHeightL(&trkpos));

                    ts += step;
                    texLen += texStep;
                    anz -= step;
                }
                ts = seg->arc;
                break;
            }
            texLen = (curTexSeg + mseg->length) / curTexSize;

            width = RtTrackGetWidth(seg, ts);
            texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

            SETPOINT(curTexElt, nbvert, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
            SETPOINT(curTexElt, nbvert, texLen, 0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);

            startNeeded = false;
            runninglentgh += seg->length;
            lastSeg = seg;
        }
        else
        {
            startNeeded = true;
        }
    }

    /* Right Barrier */
    tTrackBarrier *lastBarrier = Track->seg->next->prev->barrier[0];
    curSurfType = Ac3d::Surface::PolygonSingleSidedSmooth;
    for (int j = 0; j < 3; j++)
    {
        prevTexId = 0;
        texLen = 0;
        startNeeded = true;
        runninglentgh = 0;
        sprintf(sname, "B%dRt", j);
        for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next)
        {
            // Find last side segment
            seg = mseg;
            while (seg->rside)
                seg = seg->rside;

            curBarrier = mseg->barrier[0];

            if (seg->raceInfo & TR_PITBUILD || curBarrier->style == TR_NO_BARRIER)
            {
                startNeeded = true;
                runninglentgh = 0;
                lastBarrier = curBarrier;
                xprev = 0;
                yprev = 0;
                continue;
            }

            bool startPolygonNeeded = false;

            if (lastBarrier)
            {
                if (lastBarrier->style != curBarrier->style || lastBarrier->width != curBarrier->width || lastBarrier->height != curBarrier->height)
                    startNeeded = true;
                else if (lastBarrier->style == curBarrier->style)
                {
                    tTrackSeg *prevSide = mseg->prev;
                    while (prevSide->rside)
                        prevSide = prevSide->rside;
                    startPolygonNeeded = std::fabs(seg->vertex[TR_SR].x - prevSide->vertex[TR_ER].x) > 0.001 || std::fabs(seg->vertex[TR_SR].y - prevSide->vertex[TR_ER].y) > 0.001;
                    if (startPolygonNeeded)
                        startNeeded = true;
                }
            }

            curSurfType = curBarrier->style == TR_FENCE ? Ac3d::Surface::PolygonDoubleSidedSmooth : Ac3d::Surface::PolygonSingleSidedSmooth;
            CHECKDISPLIST(curBarrier->surface->material, sname, i, 0);

            if (!curTexLink)
            {
                texLen = 0; // edited
            }

            // fence only has one side (j = 0)
            if (curBarrier->style == TR_FENCE && j != 0)
            {
                xprev = seg->vertex[TR_SR].x; // edited
                yprev = seg->vertex[TR_SR].y; // edited
                lastBarrier = curBarrier;
                continue;
            }

            trkpos.seg = seg;
            if (startNeeded || (runninglentgh > LG_STEP_MAX))
            {
                NEWDISPLIST(false, sname, i);
                if (curTexType == 0)
                    texLen = 0;
                runninglentgh = 0;
                xprev = seg->vertex[TR_SR].x; // edited
                yprev = seg->vertex[TR_SR].y; // edited

                switch (curBarrier->style)
                {
                case TR_FENCE:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                        SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                    }
                    break;
                case TR_FENCE1:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                        SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                    }
                    else if (j == 2)
                    {
                        SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                    }
                    break;
                case TR_FENCE2:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                        SETPOINT(curTexElt, nbvert, -texLen, 0.5, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                    }
                    else if (j == 2)
                    {
                        SETPOINT(curTexElt, nbvert, -texLen, 0.5, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                    }
                    break;
                case TR_WALL:
                    switch (j)
                    {
                    case 0:
                        SETPOINT(curTexElt, nbvert, -texLen, 0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                        SETPOINT(curTexElt, nbvert, -texLen, 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                        break;
                    case 1:
                        trkpos.toStart = 0;
                        trkpos.toRight = -curBarrier->width;
                        RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                        SETPOINT(curTexElt, nbvert, -texLen, 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, seg->vertex[TR_SR].z + curBarrier->height);
                        break;
                    case 2:
                        trkpos.toStart = 0;
                        trkpos.toRight = -curBarrier->width;
                        RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                        if ((mseg->prev->barrier[0]->style != TR_WALL) || (mseg->prev->barrier[0]->height != curBarrier->height) || (mseg->prev->barrier[0]->width != curBarrier->width) || startPolygonNeeded)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen - curBarrier->width / curTexSize, 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen - curBarrier->width / curTexSize, 1.00, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z);
                        }
                        SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, seg->vertex[TR_SR].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, seg->vertex[TR_SR].z);
                        break;
                    }
                    break;
                case TR_CURB:
                case TR_PLAN:
                    // not supported
                    break;
                case TR_PITBUILDING:
                case TR_NO_BARRIER:
                    // nothing to do
                    break;
                }
            }
            switch (seg->type)
            {
            case TR_STR:
                ts = LMAX;

                trkpos.seg = seg;
                while (ts < seg->length)
                {
                    trkpos.toStart = ts;
                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    curHeight = RtTrackHeightL(&trkpos);
                    texStep = sqrt(pow((y - yprev), 2.0f) + pow((x - xprev), 2.0f)) / curTexSize; // new
                    texLen += texStep;                                                            // new
                    xprev = x;                                                                    // new
                    yprev = y;                                                                    // new
                    switch (curBarrier->style)
                    {
                    case TR_FENCE:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_FENCE1:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        break;
                    case TR_FENCE2:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.5, x, y, curHeight + curBarrier->height);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.5, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight);
                        }
                        break;
                    case TR_WALL:
                        switch (j)
                        {
                        case 0:
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, curHeight + curBarrier->height);
                            break;
                        case 1:
                            SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, curHeight + curBarrier->height);
                            trkpos.toRight = -curBarrier->width;
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, curHeight + curBarrier->height);
                            break;
                        case 2:
                            trkpos.toRight = -curBarrier->width;
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight);
                            break;
                        }
                        break;
                    case TR_CURB:
                    case TR_PLAN:
                        // not supported
                        break;
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // nothing to do
                        break;
                    }

                    ts += LMAX;
                }
                ts = seg->length;
                break;
            case TR_LFT:
                step = LMAX / (mseg->radiusr);
                // texStep = step * mseg->radius / curTexSize; //edited
                anz = seg->angle[TR_ZS] + step;
                ts = step;
                // texLen += texStep;
                radiusr = seg->radiusr;
                trkpos.seg = seg;
                while (anz < seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);

                    curHeight = RtTrackHeightL(&trkpos);
                    texStep = sqrt(pow((y - yprev), 2.0f) + pow((x - xprev), 2.0f)) / curTexSize; // new
                    texLen += texStep;                                                            // new
                    xprev = x;                                                                    // new
                    yprev = y;                                                                    // new
                    switch (curBarrier->style)
                    {
                    case TR_FENCE:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_FENCE1:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        break;
                    case TR_FENCE2:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.5, x, y, curHeight + curBarrier->height);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.5, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight);
                        }
                        break;
                    case TR_WALL:
                        switch (j)
                        {
                        case 0:
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, curHeight + curBarrier->height);
                            break;
                        case 1:
                            SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, curHeight + curBarrier->height);
                            trkpos.toRight = -curBarrier->width;
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, curHeight + curBarrier->height);
                            break;
                        case 2:
                            trkpos.toRight = -curBarrier->width;
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight);
                            break;
                        }
                        break;
                    case TR_CURB:
                    case TR_PLAN:
                        // not supported
                        break;
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // nothing to do
                        break;
                    }

                    ts += step;
                    // removed
                    anz += step;
                }
                ts = seg->arc;
                break;
            case TR_RGT:
                step = LMAX / (mseg->radiusl);
                // removed
                anz = seg->angle[TR_ZS] - step;
                ts = step;
                // removed
                radiusr = seg->radiusr;
                trkpos.seg = seg;
                while (anz > seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    trkpos.toRight = 0;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);

                    curHeight = RtTrackHeightL(&trkpos);
                    texStep = sqrt(pow((y - yprev), 2.0f) + pow((x - xprev), 2.0f)) / curTexSize; // new
                    texLen += texStep;                                                            // new
                    xprev = x;                                                                    // new
                    yprev = y;                                                                    // new
                    switch (curBarrier->style)
                    {
                    case TR_FENCE:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_FENCE1:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        break;
                    case TR_FENCE2:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.5, x, y, curHeight + curBarrier->height);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.5, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight);
                        }
                        break;
                    case TR_WALL:
                        switch (j)
                        {
                        case 0:
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, curHeight + curBarrier->height);
                            break;
                        case 1:
                            SETPOINT(curTexElt, nbvert, -texLen, 0.33, x, y, curHeight + curBarrier->height);
                            trkpos.toRight = -curBarrier->width;
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, curHeight + curBarrier->height);
                            break;
                        case 2:
                            trkpos.toRight = -curBarrier->width;
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight);
                            break;
                        }
                        break;
                    case TR_CURB:
                    case TR_PLAN:
                        // not supported
                        break;
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // nothing to do
                        break;
                    }
                    ts += step;
                    anz -= step;
                }
                ts = seg->arc;
                break;
            }
            texStep = sqrt(pow((seg->vertex[TR_ER].y - yprev), 2.0f) + pow((seg->vertex[TR_ER].x - xprev), 2.0f)) / curTexSize; // new
            texLen += texStep;                                                                                                  // edited
            xprev = seg->vertex[TR_ER].x;                                                                                       // new
            yprev = seg->vertex[TR_ER].y;                                                                                       // new
            switch (curBarrier->style)
            {
            case TR_FENCE:
                if (j == 0)
                {
                    SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                    SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                }
                break;
            case TR_FENCE1:
                if (j == 0)
                {
                    SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                    SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                }
                else if (j == 2)
                {
                    SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                }
                break;
            case TR_FENCE2:
                if (j == 0)
                {
                    SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                    SETPOINT(curTexElt, nbvert, -texLen, 0.5, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                }
                else if (j == 2)
                {
                    SETPOINT(curTexElt, nbvert, -texLen, 0.5, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                }
                break;
            case TR_WALL:
                switch (j)
                {
                case 0:
                    SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                    SETPOINT(curTexElt, nbvert, -texLen, 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                    break;
                case 1:
                    SETPOINT(curTexElt, nbvert, -texLen, 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                    trkpos.toStart = ts;
                    trkpos.toRight = -curBarrier->width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, seg->vertex[TR_ER].z + curBarrier->height);
                    break;
                case 2:
                    trkpos.toStart = ts;
                    trkpos.toRight = -curBarrier->width;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, -texLen, 0.66, x, y, seg->vertex[TR_ER].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, -texLen, 1.00, x, y, seg->vertex[TR_ER].z);
                    {
                        tTrackSeg *nextSide = mseg->next;
                        while (nextSide->rside)
                            nextSide = nextSide->rside;
                        const bool endPolygonNeeded = std::fabs(xprev - nextSide->vertex[TR_SR].x) > 0.001 || std::fabs(yprev - nextSide->vertex[TR_SR].y) > 0.001;
                        if ((mseg->next->barrier[0]->style != TR_WALL) || (mseg->next->barrier[0]->height != curBarrier->height) || (mseg->next->barrier[0]->width != curBarrier->width) || endPolygonNeeded)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen + curBarrier->width / curTexSize, 0.66, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, -texLen + curBarrier->width / curTexSize, 1.00, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z);
                        }
                    }
                    break;
                }
                break;
            case TR_CURB:
            case TR_PLAN:
                // not supported
                break;
            case TR_PITBUILDING:
            case TR_NO_BARRIER:
                // nothing to do
                break;
            }
            startNeeded = false;
            runninglentgh += seg->length;
            lastBarrier = curBarrier;
        }
    }

    /* Left Barrier */
    lastBarrier = Track->seg->next->prev->barrier[1];
    curSurfType = Ac3d::Surface::PolygonSingleSidedSmooth;
    for (int j = 0; j < 3; j++)
    {
        prevTexId = 0;
        texLen = 0;
        startNeeded = true;
        runninglentgh = 0;
        sprintf(sname, "B%dLt", j);
        for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next)
        {
            // Find last side segment
            seg = mseg;
            while (seg->lside)
                seg = seg->lside;

            curBarrier = mseg->barrier[1];

            if (seg->raceInfo & TR_PITBUILD || curBarrier->style == TR_NO_BARRIER)
            {
                runninglentgh = 0;
                startNeeded = true;
                xprev = 0.0;
                yprev = 0.0;

                continue;
            }

            bool startPolygonNeeded = false;

            if (lastBarrier)
            {
                if (lastBarrier->style != curBarrier->style || lastBarrier->width != curBarrier->width || lastBarrier->height != curBarrier->height)
                    startNeeded = true;
                else if (lastBarrier->style == curBarrier->style)
                {
                    tTrackSeg *prevSide = mseg->prev;
                    while (prevSide->lside)
                        prevSide = prevSide->lside;
                    startPolygonNeeded = std::fabs(seg->vertex[TR_SL].x - prevSide->vertex[TR_EL].x) > 0.001 || std::fabs(seg->vertex[TR_SL].y - prevSide->vertex[TR_EL].y) > 0.001;
                    if (startPolygonNeeded)
                        startNeeded = true;
                }
            }

            curSurfType = curBarrier->style == TR_FENCE ? Ac3d::Surface::PolygonDoubleSidedSmooth : Ac3d::Surface::PolygonSingleSidedSmooth;
            CHECKDISPLIST(curBarrier->surface->material, sname, i, 0);

            if (!curTexLink)
            {
                texLen = 0;
            }

            // fence only has one side (j = 0)
            if (curBarrier->style == TR_FENCE && j != 0)
            {
                xprev = seg->vertex[TR_SR].x; // edited
                yprev = seg->vertex[TR_SR].y; // edited
                continue;
            }

            trkpos.seg = seg;
            if (startNeeded || (runninglentgh > LG_STEP_MAX))
            {
                NEWDISPLIST(false, sname, i);
                runninglentgh = 0;
                if (curTexType == 0)
                    texLen = 0;
                xprev = seg->vertex[TR_SR].x; // edited
                yprev = seg->vertex[TR_SR].y; // edited

                switch (curBarrier->style)
                {
                case TR_FENCE:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                    }
                    break;
                case TR_FENCE1:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                    }
                    else if (j == 2)
                    {
                        SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                        SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                    }
                    break;
                case TR_FENCE2:
                    if (j == 0)
                    {
                        SETPOINT(curTexElt, nbvert, texLen, 0.5, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                    }
                    else if (j == 2)
                    {
                        SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                        SETPOINT(curTexElt, nbvert, texLen, 0.5, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                    }
                    break;
                case TR_WALL:
                    switch (j)
                    {
                    case 0:
                        trkpos.toStart = 0;
                        trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, 0);
                        trkpos.seg = seg;
                        RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                        if ((mseg->prev->barrier[1]->style != TR_WALL) || (mseg->prev->barrier[1]->height != curBarrier->height) || (mseg->prev->barrier[1]->width != curBarrier->width) || startPolygonNeeded)
                        {
                            SETPOINT(curTexElt, nbvert, texLen - curBarrier->width / curTexSize, 1.00, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                            SETPOINT(curTexElt, nbvert, texLen - curBarrier->width / curTexSize, 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                        }
                        SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, seg->vertex[TR_SL].z);
                        SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, seg->vertex[TR_SL].z + curBarrier->height);
                        break;
                    case 1:
                        trkpos.toStart = 0;
                        trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, 0);
                        trkpos.seg = seg;
                        RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                        SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, seg->vertex[TR_SL].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                        break;
                    case 2:
                        SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height);
                        SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z);
                        break;
                    }
                    break;
                case TR_CURB:
                case TR_PLAN:
                    // not supported
                    break;
                case TR_PITBUILDING:
                case TR_NO_BARRIER:
                    // nothing to do
                    break;
                }
            }

            switch (seg->type)
            {
            case TR_STR:
                ts = LMAX;

                trkpos.seg = seg;
                while (ts < seg->length)
                {
                    trkpos.toStart = ts;
                    trkpos.toRight = RtTrackGetWidth(seg, ts);
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    curHeight = RtTrackHeightL(&trkpos);
                    texStep = sqrt(pow((y - yprev), 2.0f) + pow((x - xprev), 2.0f)) / curTexSize; // new
                    texLen += texStep;                                                            // new
                    xprev = x;                                                                    // new
                    yprev = y;                                                                    // new
                    switch (curBarrier->style)
                    {
                    case TR_FENCE:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        break;
                    case TR_FENCE1:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_FENCE2:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 0.5, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, texLen, 0.5, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_WALL:
                        switch (j)
                        {
                        case 0:
                            trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, curHeight + curBarrier->height);
                            break;
                        case 1:
                            trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, curHeight + curBarrier->height);
                            trkpos.toRight = RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, curHeight + curBarrier->height);
                            break;
                        case 2:
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0, x, y, curHeight);
                            break;
                        }
                        break;
                    case TR_CURB:
                    case TR_PLAN:
                        // not supported
                        break;
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // nothing to do
                        break;
                    }

                    ts += LMAX;
                }
                ts = seg->length;
                break;
            case TR_LFT:
                step = LMAX / (mseg->radiusr);

                anz = seg->angle[TR_ZS] + step;
                ts = step;

                radiusl = seg->radiusl;
                trkpos.seg = seg;
                while (anz < seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    trkpos.toRight = RtTrackGetWidth(seg, ts);
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    curHeight = RtTrackHeightL(&trkpos);
                    texStep = sqrt(pow((y - yprev), 2.0f) + pow((x - xprev), 2.0f)) / curTexSize; // new
                    texLen += texStep;                                                            // new
                    xprev = x;                                                                    // new
                    yprev = y;                                                                    // new

                    switch (curBarrier->style)
                    {
                    case TR_FENCE:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        break;
                    case TR_FENCE1:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_FENCE2:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 0.5, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, texLen, 0.5, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_WALL:
                        switch (j)
                        {
                        case 0:
                            trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, curHeight + curBarrier->height);
                            break;
                        case 1:
                            trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, curHeight + curBarrier->height);
                            trkpos.toRight = RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, curHeight + curBarrier->height);
                            break;
                        case 2:
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0, x, y, curHeight);
                            break;
                        }
                        break;
                    case TR_CURB:
                    case TR_PLAN:
                        // not supported
                        break;
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // nothing to do
                        break;
                    }

                    ts += step;

                    anz += step;
                }
                ts = seg->arc;
                break;
            case TR_RGT:
                step = LMAX / (mseg->radiusl);

                anz = seg->angle[TR_ZS] - step;
                ts = step;

                radiusl = seg->radiusl;
                trkpos.seg = seg;
                while (anz > seg->angle[TR_ZE])
                {
                    trkpos.toStart = ts;
                    trkpos.toRight = RtTrackGetWidth(seg, ts);
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    curHeight = RtTrackHeightL(&trkpos);
                    texStep = sqrt(pow((y - yprev), 2.0f) + pow((x - xprev), 2.0f)) / curTexSize; // new
                    texLen += texStep;                                                            // new
                    xprev = x;                                                                    // new
                    yprev = y;                                                                    // new
                    switch (curBarrier->style)
                    {
                    case TR_FENCE:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        break;
                    case TR_FENCE1:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, -texLen, 0.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, -texLen, 1.0, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_FENCE2:
                        if (j == 0)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 0.5, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0.0, x, y, curHeight);
                        }
                        else if (j == 2)
                        {
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, texLen, 0.5, x, y, curHeight + curBarrier->height);
                        }
                        break;
                    case TR_WALL:
                        switch (j)
                        {
                        case 0:
                            trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, curHeight);
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, curHeight + curBarrier->height);
                            break;
                        case 1:
                            trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, curHeight + curBarrier->height);
                            trkpos.toRight = RtTrackGetWidth(seg, ts);
                            RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, curHeight + curBarrier->height);
                            break;
                        case 2:
                            SETPOINT(curTexElt, nbvert, texLen, 0.33, x, y, curHeight + curBarrier->height);
                            SETPOINT(curTexElt, nbvert, texLen, 0, x, y, curHeight);
                            break;
                        }
                        break;
                    case TR_CURB:
                    case TR_PLAN:
                        // not supported
                        break;
                    case TR_PITBUILDING:
                    case TR_NO_BARRIER:
                        // nothing to do
                        break;
                    }

                    ts += step;

                    anz -= step;
                }
                ts = seg->arc;
                break;
            }
            texStep = sqrt(pow((seg->vertex[TR_ER].y - yprev), 2.0f) + pow((seg->vertex[TR_ER].x - xprev), 2.0f)) / curTexSize; // new
            texLen += texStep;                                                                                                  // edited
            xprev = seg->vertex[TR_ER].x;                                                                                       // new
            yprev = seg->vertex[TR_ER].y;

            switch (curBarrier->style)
            {
            case TR_FENCE:
                if (j == 0)
                {
                    SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                }
                break;
            case TR_FENCE1:
                if (j == 0)
                {
                    SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                }
                else if (j == 2)
                {
                    SETPOINT(curTexElt, nbvert, -texLen, 0.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                    SETPOINT(curTexElt, nbvert, -texLen, 1.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                }
                break;
            case TR_FENCE2:
                if (j == 0)
                {
                    SETPOINT(curTexElt, nbvert, texLen, 0.5, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                }
                else if (j == 2)
                {
                    SETPOINT(curTexElt, nbvert, texLen, 1.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                    SETPOINT(curTexElt, nbvert, texLen, 0.5, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                }
                break;
            case TR_WALL:
                switch (j)
                {
                case 0:
                    trkpos.toStart = ts;
                    trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 1.0, x, y, seg->vertex[TR_EL].z);
                    SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, seg->vertex[TR_EL].z + curBarrier->height);
                    {
                        tTrackSeg *nextSide = mseg->next;
                        while (nextSide->lside)
                            nextSide = nextSide->lside;
                        const bool endPolygonNeeded = std::fabs(seg->vertex[TR_EL].x - nextSide->vertex[TR_SL].x) > 0.001 || std::fabs(seg->vertex[TR_EL].y - nextSide->vertex[TR_SL].y) > 0.001;
                        if ((mseg->next->barrier[1]->style != TR_WALL) || (mseg->next->barrier[1]->height != curBarrier->height) || (mseg->next->barrier[1]->width != curBarrier->width) || endPolygonNeeded)
                        {
                            SETPOINT(curTexElt, nbvert, texLen + curBarrier->width / curTexSize, 1.00, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                            SETPOINT(curTexElt, nbvert, texLen + curBarrier->width / curTexSize, 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                        }
                    }
                    break;
                case 1:
                    trkpos.toStart = ts;
                    trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    SETPOINT(curTexElt, nbvert, texLen, 0.66, x, y, seg->vertex[TR_EL].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                    break;
                case 2:
                    SETPOINT(curTexElt, nbvert, texLen, 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height);
                    SETPOINT(curTexElt, nbvert, texLen, 0.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z);
                    break;
                }
                break;
            case TR_CURB:
            case TR_PLAN:
                // not supported
                break;
            case TR_PITBUILDING:
            case TR_NO_BARRIER:
                // nothing to do
                break;
            }

            startNeeded = false;
            runninglentgh += seg->length;
            lastBarrier = curBarrier;
        }
    }

    if (!bump)
    {
        curSurfType = Ac3d::Surface::PolygonSingleSidedSmooth;

        /* Turn Marks */
        for (i = 0, seg = Track->seg->next; i < Track->nseg; i++, seg = seg->next)
        {
            if (seg->ext)
            {
                t3Dd normvec;
                int nbMarks = seg->ext->nbMarks;
                int *marks = seg->ext->marks;
                int j, k;

                for (j = 0; j < nbMarks; j++)
                {
                    /* find the segment */
                    tdble lgfs = seg->lgfromstart - (tdble)marks[j];
                    if (lgfs < 0)
                    {
                        lgfs += Track->length;
                    }
                    for (k = 0, mseg = Track->seg->next; k < Track->nseg; k++, mseg = mseg->next)
                    {
                        if ((lgfs >= mseg->lgfromstart) && (lgfs < (mseg->lgfromstart + mseg->length)))
                        {
                            break;
                        }
                    }

                    if (seg->type == TR_RGT)
                    {
                        sprintf(buf, "turn%dR", marks[j]);
                        trkpos.toRight = Track->width + tmHSpace + tmWidth;
                    }
                    else
                    {
                        sprintf(buf, "turn%dL", marks[j]);
                        trkpos.toRight = -tmHSpace;
                    }
                    trkpos.toStart = lgfs - mseg->lgfromstart;
                    if (mseg->type != TR_STR)
                    {
                        trkpos.toStart = trkpos.toStart / mseg->radius;
                    }
                    trkpos.seg = mseg;
                    RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
                    z = tmVSpace + RtTrackHeightL(&trkpos);
                    if (seg->type == TR_LFT)
                    {
                        RtTrackSideNormalG(mseg, x, y, TR_RGT, &normvec);
                        normvec.x = -normvec.x;
                        normvec.y = -normvec.y;
                    }
                    else
                    {
                        RtTrackSideNormalG(mseg, x, y, TR_LFT, &normvec);
                    }
                    CHECKDISPLIST2(buf, 0, "TuMk", mseg->id);

                    SETPOINT(curTexElt, nbvert, 0.0, 0.0, x, y, z);
                    SETPOINT(curTexElt, nbvert, 1.0, 0.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z);
                    SETPOINT(curTexElt, nbvert, 0.0, 1.0, x, y, z + tmHeight);
                    SETPOINT(curTexElt, nbvert, 1.0, 1.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z + tmHeight);

                    CHECKDISPLIST2("back-sign", 0, "TuMk", mseg->id);

                    SETPOINT(curTexElt, nbvert, 0.0, 0.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z);
                    SETPOINT(curTexElt, nbvert, 1.0, 0.0, x, y, z);
                    SETPOINT(curTexElt, nbvert, 0.0, 1.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z + tmHeight);
                    SETPOINT(curTexElt, nbvert, 1.0, 1.0, x, y, z + tmHeight);

                    printf("(%f, %f, %f), (%f, %f, %f)\n", x, y, z, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z + tmHeight);
                }
            }
        }

        /* Start Bridge */
        if (bridge)
        {
            curSurfType = Ac3d::Surface::PolygonSingleSidedSmooth;
            CHECKDISPLIST2("pylon1", 4, "S0Bg", 0);
#define BR_HEIGHT_1 8.0
#define BR_HEIGHT_2 6.0
#define BR_WIDTH_0 2.0
#define BR_WIDTH_1 2.0
            mseg = Track->seg->next;
            if (mseg->rside)
            {
                seg = mseg->rside;
                if (seg->rside)
                {
                    seg = seg->rside;
                }
            }
            else
            {
                seg = mseg;
            }

            x = seg->vertex[TR_SR].x;
            y = seg->vertex[TR_SR].y - 0.1;
            z = seg->vertex[TR_SR].z;

            SETPOINT(curTexElt, nbvert, 0, 0, x, y, z);
            SETPOINT(curTexElt, nbvert, 0, 1, x, y, z + BR_HEIGHT_2);

            x += BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 1, 0, x, y, z);
            SETPOINT(curTexElt, nbvert, 1, 1, x, y, z + BR_HEIGHT_2);

            y -= BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 2, 0, x, y, z);
            SETPOINT(curTexElt, nbvert, 2, 1, x, y, z + BR_HEIGHT_2);

            x -= BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 3, 0, x, y, z);
            SETPOINT(curTexElt, nbvert, 3, 1, x, y, z + BR_HEIGHT_2);

            y += BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 4, 0, x, y, z);
            SETPOINT(curTexElt, nbvert, 4, 1, x, y, z + BR_HEIGHT_2);

            NEWDISPLIST(false, "S1Bg", 0);

            if (mseg->lside)
            {
                seg = mseg->lside;
                if (seg->lside)
                {
                    seg = seg->lside;
                }
            }
            else
            {
                seg = mseg;
            }
            x2 = seg->vertex[TR_SL].x;
            y2 = seg->vertex[TR_SL].y + 0.1;
            z2 = seg->vertex[TR_SL].z;

            SETPOINT(curTexElt, nbvert, 0, 1, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 0, 0, x2, y2, z2);

            x2 += BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 1, 1, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 1, 0, x2, y2, z2);

            y2 += BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 2, 1, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 2, 0, x2, y2, z2);

            x2 -= BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 3, 1, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 3, 0, x2, y2, z2);

            y2 -= BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 4, 1, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 4, 0, x2, y2, z2);

            CHECKDISPLIST2("pylon2", 4, "S2Bg", 0);

            SETPOINT(curTexElt, nbvert, 0, 1, x, y, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 0, 0, x, y, z + BR_HEIGHT_2);

            y -= BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 1, 1, x, y, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 1, 0, x, y, z + BR_HEIGHT_2);

            x += BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 2, 1, x, y, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 2, 0, x, y, z + BR_HEIGHT_2);

            y += BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 3, 1, x, y, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 3, 0, x, y, z + BR_HEIGHT_2);

            x -= BR_WIDTH_0;

            NEWDISPLIST(false, "S3Bg", 0);

            SETPOINT(curTexElt, nbvert, 3, 1, x + BR_WIDTH_0, y, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 3, 0, x, y, z + BR_HEIGHT_1);

            y -= BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 4, 1, x + BR_WIDTH_0, y, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 4, 0, x, y, z + BR_HEIGHT_1);

            y += BR_WIDTH_1; /* back to origin */

            NEWDISPLIST(false, "S4Bg", 0);

            y2 += BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 0, 1, x2 + BR_WIDTH_0, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 0, 0, x2, y2, z + BR_HEIGHT_1);

            y2 -= BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 1, 1, x2 + BR_WIDTH_0, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 1, 0, x2, y2, z + BR_HEIGHT_1);

            x2 += BR_WIDTH_0;

            NEWDISPLIST(false, "S5Bg", 0);

            SETPOINT(curTexElt, nbvert, 1, 1, x2, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 1, 0, x2, y2, z + BR_HEIGHT_2);

            y2 += BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 2, 1, x2, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 2, 0, x2, y2, z + BR_HEIGHT_2);

            x2 -= BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 3, 1, x2, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 3, 0, x2, y2, z + BR_HEIGHT_2);

            y2 -= BR_WIDTH_1;

            SETPOINT(curTexElt, nbvert, 4, 1, x2, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 4, 0, x2, y2, z + BR_HEIGHT_2);

            /* Middle on the bridge */
            CHECKDISPLIST2("pylon3", 4, "S6Bg", 2);

            SETPOINT(curTexElt, nbvert, 0, 0, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 1, 0, x, y, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 0, 0.25, x2, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 1, 0.25, x, y, z + BR_HEIGHT_1);

            x += BR_WIDTH_0;
            x2 += BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 0, 0.5, x2, y2, z + BR_HEIGHT_1);
            SETPOINT(curTexElt, nbvert, 1, 0.5, x, y, z + BR_HEIGHT_1);

            SETPOINT(curTexElt, nbvert, 0, 0.75, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 1, 0.75, x, y, z + BR_HEIGHT_2);

            x -= BR_WIDTH_0;
            x2 -= BR_WIDTH_0;

            SETPOINT(curTexElt, nbvert, 0, 1, x2, y2, z + BR_HEIGHT_2);
            SETPOINT(curTexElt, nbvert, 1, 1, x, y, z + BR_HEIGHT_2);
        }

        /* draw the pits */
#define PIT_HEIGHT 5.0
#define PIT_DEEP 10.0
#define PIT_TOP 0.2

        pits = &(Track->pits);
        initPits(Track, TrackHandle, pits);
        curSurfType = Ac3d::Surface::PolygonDoubleSidedSmooth; // this must be 2 sided

        switch (pits->type)
        {
        case TR_PIT_ON_TRACK_SIDE: {
            static int uid = 1;
            t3Dd normvec;

            startNeeded = true;
            sprintf(sname, "P%dts", uid++);
            CHECKDISPLIST3("concrete2.png", 4, sname, pits->driversPits[0].pos.seg->id);

            RtTrackLocal2Global(&(pits->driversPits[0].pos), &x, &y, pits->driversPits[0].pos.type);
            RtTrackSideNormalG(pits->driversPits[0].pos.seg, x, y, pits->side, &normvec);
            z2 = RtTrackHeightG(pits->driversPits[0].pos.seg, x, y);

            x2 = x + PIT_TOP * normvec.x;
            y2 = y + PIT_TOP * normvec.y;
            SETPOINT(curTexElt, nbvert, 1.0 - PIT_TOP, PIT_HEIGHT - PIT_TOP, x2, y2, z2 + PIT_HEIGHT - PIT_TOP);
            SETPOINT(curTexElt, nbvert, 1.0 - PIT_TOP, PIT_HEIGHT, x2, y2, z2 + PIT_HEIGHT);

            x2 = x;
            y2 = y;
            SETPOINT(curTexElt, nbvert, 1.0, PIT_HEIGHT - PIT_TOP, x2, y2, z2 + PIT_HEIGHT - PIT_TOP);

            x2 = x - PIT_DEEP * normvec.x;
            y2 = y - PIT_DEEP * normvec.y;
            SETPOINT(curTexElt, nbvert, 1.0 + PIT_DEEP, PIT_HEIGHT, x2, y2, z2 + PIT_HEIGHT);

            x2 = x;
            y2 = y;
            SETPOINT(curTexElt, nbvert, 1.0, 0, x2, y2, z2);

            x2 = x - PIT_DEEP * normvec.x;
            y2 = y - PIT_DEEP * normvec.y;
            SETPOINT(curTexElt, nbvert, 1.0 + PIT_DEEP, 0, x2, y2, z2);

            for (i = 0; i < pits->driversPitsNb; i++)
            {
                startNeeded = true;
                sprintf(sname, "P%dts", uid++);
                CHECKDISPLIST3("concrete.png", 4, sname, pits->driversPits[i].pos.seg->id);

                RtTrackLocal2Global(&(pits->driversPits[i].pos), &x, &y, pits->driversPits[i].pos.type);
                RtTrackSideNormalG(pits->driversPits[i].pos.seg, x, y, pits->side, &normvec);
                x2 = x;
                y2 = y;
                z2 = RtTrackHeightG(pits->driversPits[i].pos.seg, x2, y2);

                if (pits->side == TR_RGT)
                {
                    x3 = x + pits->len * normvec.y;
                    y3 = y - pits->len * normvec.x;
                }
                else
                {
                    x3 = x - pits->len * normvec.y;
                    y3 = y + pits->len * normvec.x;
                }

                z3 = RtTrackHeightG(pits->driversPits[i].pos.seg, x3, y3);
                SETPOINT(curTexElt, nbvert, pits->len, 0, x2, y2, z2 + PIT_HEIGHT - PIT_TOP);
                SETPOINT(curTexElt, nbvert, 0, 0, x3, y3, z3 + PIT_HEIGHT - PIT_TOP);

                tdble dx = PIT_TOP * normvec.x;
                tdble dy = PIT_TOP * normvec.y;
                SETPOINT(curTexElt, nbvert, pits->len, PIT_TOP, x2 + dx, y2 + dy, z2 + PIT_HEIGHT - PIT_TOP);
                SETPOINT(curTexElt, nbvert, 0, PIT_TOP, x3 + dx, y3 + dy, z3 + PIT_HEIGHT - PIT_TOP);
                SETPOINT(curTexElt, nbvert, pits->len, 2 * PIT_TOP, x2 + dx, y2 + dy, z2 + PIT_HEIGHT);
                SETPOINT(curTexElt, nbvert, 0, 2 * PIT_TOP, x3 + dx, y3 + dy, z3 + PIT_HEIGHT);

                dx = -PIT_DEEP * normvec.x;
                dy = -PIT_DEEP * normvec.y;

                SETPOINT(curTexElt, nbvert, pits->len, 2 * PIT_TOP + PIT_DEEP, x2 + dx, y2 + dy, z2 + PIT_HEIGHT);
                SETPOINT(curTexElt, nbvert, 0, 2 * PIT_TOP + PIT_DEEP, x3 + dx, y3 + dy, z3 + PIT_HEIGHT);
                SETPOINT(curTexElt, nbvert, pits->len, 2 * PIT_TOP + PIT_DEEP + PIT_HEIGHT, x2 + dx, y2 + dy, z2);
                SETPOINT(curTexElt, nbvert, 0, 2 * PIT_TOP + PIT_DEEP + PIT_HEIGHT, x3 + dx, y3 + dy, z3);
            } // for i

            startNeeded = true;
            i--;
            sprintf(sname, "P%dts", uid++);
            CHECKDISPLIST3("concrete2.png", 4, sname, pits->driversPits[i].pos.seg->id);

            RtTrackLocal2Global(&(pits->driversPits[i].pos), &x, &y, pits->driversPits[i].pos.type);
            RtTrackSideNormalG(pits->driversPits[i].pos.seg, x, y, pits->side, &normvec);

            if (pits->side == TR_RGT)
            {
                x += pits->len * normvec.y;
                y -= pits->len * normvec.x;
            }
            else
            {
                x -= pits->len * normvec.y;
                y += pits->len * normvec.x;
            }

            z2 = RtTrackHeightG(pits->driversPits[i].pos.seg, x, y);
            x2 = x + PIT_TOP * normvec.x;
            y2 = y + PIT_TOP * normvec.y;
            SETPOINT(curTexElt, nbvert, 1.0 - PIT_TOP, PIT_HEIGHT, x2, y2, z2 + PIT_HEIGHT);
            SETPOINT(curTexElt, nbvert, 1.0 - PIT_TOP, PIT_HEIGHT - PIT_TOP, x2, y2, z2 + PIT_HEIGHT - PIT_TOP);

            x2 = x - PIT_DEEP * normvec.x;
            y2 = y - PIT_DEEP * normvec.y;
            SETPOINT(curTexElt, nbvert, 1.0 + PIT_DEEP, PIT_HEIGHT, x2, y2, z2 + PIT_HEIGHT);

            x2 = x;
            y2 = y;
            SETPOINT(curTexElt, nbvert, 1.0, PIT_HEIGHT - PIT_TOP, x2, y2, z2 + PIT_HEIGHT - PIT_TOP);

            x2 = x - PIT_DEEP * normvec.x;
            y2 = y - PIT_DEEP * normvec.y;
            SETPOINT(curTexElt, nbvert, 1.0 + PIT_DEEP, 0, x2, y2, z2);

            x2 = x;
            y2 = y;
            SETPOINT(curTexElt, nbvert, 1.0, 0, x2, y2, z2);
            break; // TR_PIT_ON_TRACK_SIDE
        }
        case TR_PIT_NO_BUILDING:
            break;

        case TR_PIT_ON_SEPARATE_PATH:
            break;

        case TR_PIT_NONE:
            break;

        } // switch pits->type
    }
    CLOSEDISPLIST();
    printf("=== Indices really used = %d\n", nbvert);

    return 0;
} // InitScene

static void saveObject2(Ac3d &ac3d, tDispElt *aDispElt, char *name, bool bump, bool raceline, bool acc)
{
    int nb = aDispElt->nb;
    int start = aDispElt->start;
    char *texture = aDispElt->texture->name;
    char *texture2 = aDispElt->texture->nameraceline;
    const Ac3d::Surface::SURF surfType = aDispElt->surfType;

    Ac3d::Object object;
    object.type = "poly";
    object.name = name;
    object.textures.emplace_back(texture);
    if (!(!acc || (!raceline || strncmp(name, "tkMn", 4))))
    {
        object.textures.emplace_back("empty_texture_no_mapping");
        object.textures.emplace_back(texture2);
    }

    for (int i = 0; i < nb; i++)
    {
        const int index = 3 * (start + i);
        object.vertices.emplace_back(trackvertices[index], trackvertices[index + 2], -trackvertices[index + 1]);
    }

    Ac3d::Surface   surface;
    surface.surf = surfType;
    surface.mat = 0;
    if (object.textures.size() == 1)
    {
        surface.refs.emplace_back(0, tracktexcoord[2 * (start + 0)], tracktexcoord[2 * (start + 0) + 1]);
        surface.refs.emplace_back(1, tracktexcoord[2 * (start + 1)], tracktexcoord[2 * (start + 1) + 1]);
        surface.refs.emplace_back(2, tracktexcoord[2 * (start + 2)], tracktexcoord[2 * (start + 2) + 1]);
    }
    else if (object.textures.size() == 3)
    {
        surface.refs.emplace_back(0, tracktexcoord[2 * (start + 0)], tracktexcoord[2 * (start + 0) + 1], 0, 0, tracktexcoord2[2 * (start + 0)], tracktexcoord2[2 * (start + 0) + 1]);
        surface.refs.emplace_back(1, tracktexcoord[2 * (start + 1)], tracktexcoord[2 * (start + 1) + 1], 0, 0, tracktexcoord2[2 * (start + 1)], tracktexcoord2[2 * (start + 1) + 1]);
        surface.refs.emplace_back(2, tracktexcoord[2 * (start + 2)], tracktexcoord[2 * (start + 2) + 1], 0, 0, tracktexcoord2[2 * (start + 2)], tracktexcoord2[2 * (start + 2) + 1]);
    }

    object.surfaces.push_back(surface);

    /* triangle strip conversion to triangles */
    for (int i = 2; i < nb - 1; i++)
    {
        surface.surf = surfType;
        surface.mat = 0;
        surface.refs.clear();

        if ((i % 2) == 0)
        {
            int index = i;
            if (object.textures.size() == 1)
            {
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
                index = i - 1;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
                index = i + 1;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
            }
            else if (object.textures.size() == 3)
            {
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1], 0, 0, tracktexcoord2[2 * (start + index)], tracktexcoord2[2 * (start + index) + 1]);
                index = i - 1;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1], 0, 0, tracktexcoord2[2 * (start + index)], tracktexcoord2[2 * (start + index) + 1]);
                index = i + 1;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1], 0, 0, tracktexcoord2[2 * (start + index)], tracktexcoord2[2 * (start + index) + 1]);
            }
        }
        else
        {
            int index = i - 1;
            if (object.textures.size() == 1)
            {
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
                index = i;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
                index = i + 1;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
            }
            else if (object.textures.size() == 3)
            {
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1], 0, 0, tracktexcoord2[2 * (start + index)], tracktexcoord2[2 * (start + index) + 1]);
                index = i;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1], 0, 0, tracktexcoord2[2 * (start + index)], tracktexcoord2[2 * (start + index) + 1]);
                index = i + 1;
                surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1], 0, 0, tracktexcoord2[2 * (start + index)], tracktexcoord2[2 * (start + index) + 1]);
            }
        }

        object.surfaces.push_back(surface);
    }

    ac3d.addObject(object);
    ac3d.stack.pop();
}

static void saveObject(Ac3d &ac3d, int nb, int start, char *texture, char *name, Ac3d::Surface::SURF surfType)
{
    Ac3d::Object object;
    object.type = "poly";
    object.name = name;
    object.textures.emplace_back(texture);

    for (int i = 0; i < nb; i++)
    {
        const int index = 3 * (start + i);
        object.vertices.emplace_back(trackvertices[index], trackvertices[index + 2], -trackvertices[index + 1]);
    }

    Ac3d::Surface   surface;
    surface.surf = surfType;
    surface.mat = 0;
    surface.refs.emplace_back(0, tracktexcoord[2 * start], tracktexcoord[2 * start + 1]);
    surface.refs.emplace_back(1, tracktexcoord[2 * (start + 1)], tracktexcoord[2 * (start + 1) + 1]);
    surface.refs.emplace_back(2, tracktexcoord[2 * (start + 2)], tracktexcoord[2 * (start + 2) + 1]);

    object.surfaces.push_back(surface);

    /* triangle strip conversion to triangles */
    for (int i = 2; i < nb - 1; i++)
    {
        surface.surf = surfType;
        surface.mat = 0;
        surface.refs.clear();

        if ((i % 2) == 0)
        {
            int index = i;
            surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
            index = i - 1;
            surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
            index = i + 1;
            surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
        }
        else
        {
            int index = i - 1;
            surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
            index = i;
            surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
            index = i + 1;
            surface.refs.emplace_back(index, tracktexcoord[2 * (start + index)], tracktexcoord[2 * (start + index) + 1]);
        }

        object.surfaces.push_back(surface);
    }

    ac3d.addObject(object);
    ac3d.stack.pop();
}

static void SaveMainTrack(Ac3d &ac3d, bool bump, bool raceline, bool acc)
{
    tDispElt *aDispElt;
    char buf[256];
    int i;

    for (i = 0; i < GroupNb; i++)
    {
        if (Groups[i].nb != 0)
        {
            aDispElt = Groups[i].dispList;
            sprintf(buf, "TKMN%d", i);

            Ac3d::Object object;
            object.type = "group";
            object.name = buf;
            ac3d.addObject(object);

            do
            {
                aDispElt = aDispElt->next;
                if (aDispElt->nb != 0)
                {
                    sprintf(buf, "%s%d", aDispElt->name, aDispElt->id);

                    // skip bad display lists
                    if (aDispElt->nb > 0 && aDispElt->nb < 3)
                    {
                        GfLogWarning("Bug: Display list %s has %d vertices!\n", buf, aDispElt->nb);
                        continue;
                    }

                    if (bump)
                    {
                        saveObject(ac3d, aDispElt->nb, aDispElt->start, aDispElt->texture->namebump, buf, aDispElt->surfType);
                    }
                    else if (raceline && !acc)
                    {
                        saveObject(ac3d, aDispElt->nb, aDispElt->start, aDispElt->texture->nameraceline, buf, aDispElt->surfType);
                    }
                    else if (raceline && acc)
                    {
                        saveObject2(ac3d, aDispElt, buf, bump, raceline, acc);
                    }
                    else
                    {
                        saveObject(ac3d, aDispElt->nb, aDispElt->start, aDispElt->texture->name, buf, aDispElt->surfType);
                    }
                }
            } while (aDispElt != Groups[i].dispList);

            ac3d.stack.pop();
        }
    }
}

/** Calculate track parameters and exit without any file creation
    It is for information only, mainly for use from TrackEditor.
    @param	Track	track structure
    @param	TrackHandle	handle on the track description
    @return	none
*/
void CalculateTrack(tTrack *Track, void *TrackHandle, bool bump, bool raceline, bool bridge, bool acc)
{
    TrackStep = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_TSTEP, nullptr, TrackStep);
    printf("Track step: %.2f ", TrackStep);

    InitScene(Track, TrackHandle, bump, raceline, bridge, acc);

    printf("Calculation finished\n");
}

/** Generate the track AC3D file(s).
    @param	Track	track structure
    @param	TrackHandle	handle on the track description
    @param	outFile	output file name for track only
    @param	AllFd	fd of the merged file
    @return	none
*/
void GenerateTrack(tTrack *Track, void *TrackHandle, const std::string &outFile, Ac3d &allAc3d, bool all, bool bump, bool raceline, bool bridge, bool acc)
{
    TrackStep = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_TSTEP, nullptr, TrackStep);
    printf("Track step: %.2f ", TrackStep);

    InitScene(Track, TrackHandle, bump, raceline, bridge, acc);

    if (!outFile.empty())
    {
        Ac3d    ac3d;
        ac3d.addDefaultMaterial();
        Ac3d::Object object;
        object.type = "group";
        object.name = "track";
        ac3d.addObject(object);
        SaveMainTrack(ac3d, bump, raceline, acc);
        ac3d.removeBadTriangles();
        ac3d.removeEmptyObjects();
        ac3d.writeFile(outFile, false);
    }

    if (all)
    {
        Ac3d::Object object;
        object.type = "group";
        object.name = "track";
        allAc3d.addObject(object);
        SaveMainTrack(allAc3d, bump, raceline, acc);
        allAc3d.stack.pop();
    }
}
