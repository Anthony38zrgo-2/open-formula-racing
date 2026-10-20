/***************************************************************************

    file                 : relief.cpp
    created              : Tue Mar  6 23:15:19 CET 2001
    copyright            : (C) 2000 by Eric Espie
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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <plib/ssg.h>

#include <tgfclient.h>
#include <track.h>

#include "ac3d.h"
#include "easymesh.h"
#include "trackgen.h"
#include "relief.h"

static std::vector<ssgBranch*> InteriorList;
static std::vector<ssgBranch*> ExteriorList;

static tdble GridStep = DEFAULT_BORDER_STEP;

static ssgEntity *Root = nullptr;

/*
 * Read the lines from AC3D file
 * separate between interior and exterior lines
 */
static ssgBranch *hookNode(char *s)
{
    ssgBranch *branch = new ssgBranch();

    if (strncmp(s, "interior", 8) == 0)
    {
        InteriorList.push_back(branch);
    }
    else if (strncmp(s, "exterior", 8) == 0)
    {
        ExteriorList.push_back(branch);
    }
    else
    {
        printf("Relief: unsupported type %s\n", s);
    }
    return branch;
}

/*
  Load a simple database
*/
void LoadRelief(tTrack *track, void *TrackHandle, const std::string &reliefFile)
{
    GridStep = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_BSTEP, nullptr, GridStep);

    ssgLoaderOptions options;

    options.setCreateBranchCallback(hookNode);

    ssgSetCurrentOptions(&options);

    printf("\nLoading relief file %s\n", reliefFile.c_str());

    Root = ssgLoadAC(reliefFile.c_str());

    printf("Relief: %zu interior, %zu exterior\n", InteriorList.size(), ExteriorList.size());
}

static void countRec(ssgEntity *e, int *nb_vert, int *nb_seg)
{
    if (e->isAKindOf(_SSG_TYPE_BRANCH))
    {
        ssgBranch *br = dynamic_cast<ssgBranch *>(e);

        for (int i = 0; i < br->getNumKids(); i++)
        {
            countRec(br->getKid(i), nb_vert, nb_seg);
        }
    }
    else if (e->isAKindOf(_SSG_TYPE_VTXTABLE))
    {
        ssgVtxTable *vt = dynamic_cast<ssgVtxTable *>(e);

        *nb_vert += vt->getNumVertices();
        *nb_seg += vt->getNumLines();
    }
}

void CountRelief(bool interior, int *nb_vert, int *nb_seg)
{
    *nb_vert = *nb_seg = 0;

    if (Root == nullptr)
        return;

    std::vector<ssgBranch*>& lines = interior ? InteriorList : ExteriorList;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        ssgBranch *br = lines[i]->getParent(0);
        ssgBranch *br2 = new ssgBranch();

        br2->addKid(br);
        ssgFlatten(br);
        lines[i] = br2;

        countRec(dynamic_cast<ssgEntity *>(lines[i]), nb_vert, nb_seg);
    }
}

static void genRec(ssgEntity *e, bool interior)
{
    if (e->isAKindOf(_SSG_TYPE_BRANCH))
    {
        ssgBranch *br = dynamic_cast<ssgBranch *>(e);

        for (int i = 0; i < br->getNumKids(); i++)
        {
            genRec(br->getKid(i), interior);
        }
    }
    else if (e->isAKindOf(_SSG_TYPE_VTXTABLE))
    {
        ssgVtxTable *vt = dynamic_cast<ssgVtxTable *>(e);

        int nv = vt->getNumVertices();
        int nl = vt->getNumLines();
        int sv = getPointCount();
        GLenum pt = vt->getPrimitiveType();

        printf("Relief: %s %s %d vertices %d lines\n", interior ? "interior" : "exterior",
               pt == GL_LINE_LOOP ? "line loop" : pt == GL_LINE_STRIP ? "line strip" : "unsupported type",
               nv, nl);

        for (int i = 0; i < nv; i++)
        {
            float *vtx = vt->getVertex(i);

            addPoint(vtx[0], vtx[1], vtx[2], GridStep, 100000);
        }

        for (int i = 0; i < nl; i++)
        {
            short vv0, vv1;

            vt->getLine(i, &vv0, &vv1);
            addSegment(vv0 + sv, vv1 + sv, 100000);
        }
    }
}

void GenRelief(bool interior)
{
    if (Root == nullptr)
        return;

    std::vector<ssgBranch*>& lines = interior ? InteriorList : ExteriorList;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        genRec(dynamic_cast<ssgEntity *>(lines[i]), interior);
    }
}
