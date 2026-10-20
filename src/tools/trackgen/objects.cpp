/***************************************************************************

    file                 : objects.cpp
    created              : Fri May 24 20:09:20 CEST 2002
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


#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <cmath>

#include <tgfclient.h>
#include <track.h>
#include <portability.h>

#include "ac3d.h"
#include "util.h"
#include "objects.h"
#include "trackgen.h"
#include "robottools.h"

namespace
{

float    GroupSize = 0;
float    XGroupOffset = 0;
float    YGroupOffset = 0;
int      XGroupNb = 0;
int      ObjUniqId = 0;

struct objdef
{
    bool            random = false;
    bool            trackOriented = false;
    bool            terrainOriented = false;
    bool            borderOriented = false;
    unsigned int    color = 0;
    Ac3d            ac3d;
    tdble           deltaHeight = 0;
    tdble           deltaVert = 0;
    float           distance = 0;
    bool            scaleRandom = false;
    bool            scaleFixed = false;
    tdble           scale = 0;
    tdble           scaleMin = 0;
    tdble           scaleMax = 0;
    std::string     fileName;
    std::string     name;
    bool            useMaterial = false;
};

std::vector<objdef> objects;

tdble   trackOffsetX = 0;
tdble   trackOffsetY = 0;

void
InitObjects(tTrack *track, void *TrackHandle)
{
    tTrkLocPos pos;
    pos.seg = track->seg;
    pos.type = TR_LPOS_MAIN;
    pos.toStart = 0;
    pos.toRight = 0;
    pos.toMiddle = 0;
    pos.toLeft = 0;
    RtTrackLocal2Global(&pos, &trackOffsetX, &trackOffsetY, TR_TOMIDDLE);

    ObjUniqId = 0;

    srand((unsigned int)GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_SEED, nullptr, 1));

    std::string inputPath(track->filename);
    inputPath.resize(inputPath.find_last_of('/'));

    const std::string modelPath(inputPath + ";" + GfDataDir() + "data/objects");

    const int objnb = GfParmGetEltNb(TrackHandle, TRK_SECT_OBJECTS);
    GfParmListSeekFirst(TrackHandle, TRK_SECT_OBJECTS);

    objects.resize(objnb);

    for (int i = 0; i < objnb; i++)
    {
        objdef	*curObj = &objects[objnb - 1 - i]; // iterate backwards to match old behaviour
        curObj->color = (unsigned int)GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_COLOR, nullptr, 0);
        const char *objName = GfParmGetCurStr(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_OBJECT, nullptr);

        if (!objName)
        {
            GfError("Missing %s in section %s/%s", TRK_ATT_OBJECT, TRK_SECT_OBJECTS, GfParmListGetCurEltName(TrackHandle, TRK_SECT_OBJECTS));
            exit(1);
        }

        curObj->name = GfParmListGetCurEltName(TrackHandle, TRK_SECT_OBJECTS);

        char filename[1024];
        if (!GetFilename(objName, modelPath.c_str(), filename, sizeof(filename)))
        {
            GfError("Couldn't find object %s in %s\n", objName, modelPath.c_str());
            GfParmListSeekNext(TrackHandle, TRK_SECT_OBJECTS);
            continue;
        }
        curObj->fileName = filename;

        try
        {
            curObj->ac3d.readFile(curObj->fileName);
            curObj->ac3d.flattenGeometry();
            curObj->ac3d.generateTriangles();   // convert quads to triangles
            curObj->ac3d.removeBadTriangles();
            curObj->ac3d.removeEmptyObjects();
            curObj->ac3d.flipAxes(true);        // convert to track coordinate system
        }
        catch (const Ac3d::Exception &e)
        {
            GfError("Reading object %s: %s\n", curObj->fileName.c_str(), e.what());
            GfParmListSeekNext(TrackHandle, TRK_SECT_OBJECTS);
            continue;
        }

        const std::string scaleType = GfParmGetCurStr(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_SCALE_TYPE, "");
        if (scaleType == "random")
        {
            curObj->scaleFixed = false;
            curObj->scaleRandom = true;
            curObj->scaleMin = GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_SCALE_MIN, nullptr, 0.5);
            curObj->scaleMax = GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_SCALE_MAX, nullptr, 2.0);
        }
        else if (scaleType == "fixed")
        {
            curObj->scaleFixed = true;
            curObj->scaleRandom = false;
            curObj->scale = GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_SCALE, nullptr, 1.0);
        }
        else
        {
            curObj->scaleFixed = false;
            curObj->scaleRandom = false;
        }

        if (strcmp(GfParmGetCurStr(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_ORIENTATION_TYPE, ""), "random") == 0)
        {
            curObj->deltaHeight = GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_DH, nullptr, 0);
            curObj->deltaVert = GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_DV, nullptr, 5.0);
            curObj->random = true;
        }
        else
        {
            curObj->random = false;
            Ac3d::Matrix m1;
            m1.makeRotation(GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_ORIENTATION, "deg", 0), 0.0, 0.0);
            curObj->ac3d.transform(m1);
        }

        if (strcmp(GfParmGetCurStr(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_ORIENTATION_TYPE, ""), "track") == 0)
        {
            curObj->trackOriented = true;
        }
        else
        {
            curObj->trackOriented = false;
        }

        if (strcmp(GfParmGetCurStr(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_ORIENTATION_TYPE, ""), "terrain") == 0)
        {
            curObj->terrainOriented = true;
        }
        else
        {
            curObj->terrainOriented = false;
        }

        if (strcmp(GfParmGetCurStr(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_ORIENTATION_TYPE, ""), "border") == 0)
        {
            curObj->borderOriented = true;
            curObj->distance = GfParmGetCurNum(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_BORDER_DISTANCE, nullptr, 1.0);
        }
        else
        {
            curObj->borderOriented = false;
        }

        if (strcmp(GfParmGetCurStr(TrackHandle, TRK_SECT_OBJECTS, TRK_ATT_USE_MATERIAL, ""), "yes") == 0)
        {
            curObj->useMaterial = true;
        }
        else
        {
            curObj->useMaterial = false;
        }

        GfParmListSeekNext(TrackHandle, TRK_SECT_OBJECTS);
    }
}

void
AddObject(tTrack *track, void *trackHandle, const Ac3d::Object *terrainRoot, const Ac3d::Object *trackRoot, Ac3d &objectsRoot, unsigned int clr, tdble x, tdble y, bool multipleMaterials, bool individual)
{
    for (auto &curObj : objects)
    {
        if (clr == curObj.color)
        {
            Ac3d::Matrix    m;
            tdble           dv = 0;
            tdble           angle = 0;
            double          z = 0;
            tdble           orientation = 0;
            double          height = 0;
            std::string     name;
            Ac3d::V3d       normal;

            if (individual)
            {
                orientation = GfParmGetCurNum(trackHandle, TRK_SECT_TERRAIN_OBJECTS, TRK_ATT_ORIENTATION, "deg", 0);
                height = GfParmGetCurNum(trackHandle, TRK_SECT_TERRAIN_OBJECTS, TRK_ATT_HEIGHT, "m", 0);
                name = GfParmListGetCurEltName(trackHandle, TRK_SECT_TERRAIN_OBJECTS);
            }
            else
                name = curObj.name;

            Ac3d obj(curObj.ac3d);

            if (curObj.random)
            {
                /* random rotations */
                /* 		sgMakeCoordMat4 (m, 0.0, 0.0, curObj.deltaHeight * rand() / (RAND_MAX + 1.0), 0.0, 0.0, 0.0); */
                /* 		ApplyTransform (m, obj); */
                dv = curObj.deltaVert;
                angle = 360.0 * rand() / (RAND_MAX + 1.0);
            }

            if (curObj.scaleRandom)
            {
                const tdble random = ((tdble)rand()) / (tdble)RAND_MAX;

                if (curObj.scaleMin > curObj.scaleMax)
                    std::swap(curObj.scaleMin, curObj.scaleMax);

                const tdble scale = (random * (curObj.scaleMax - curObj.scaleMin)) + curObj.scaleMin;

                m.makeScale(scale);
                obj.transform(m);
            }
            else if (curObj.scaleFixed)
            {
                m.makeScale(curObj.scale);
                obj.transform(m);
            }

            if (curObj.terrainOriented)
            {
                angle = terrainRoot->getTerrainAngle(x, y);
            }

            if (curObj.trackOriented)
            {
                /* NEW: calculate angle for track-aligned orientation */
                angle = getTrackAngle(track, trackHandle, x, y);
            }

            if (curObj.borderOriented)
            {
                /* NEW: calculate angle for border-aligned orientation */
                float xNeu, yNeu, zNeu;
                angle = getBorderAngle(track, trackHandle, x, y, curObj.distance, &xNeu, &yNeu, &zNeu);
                // do something else with x and y
                x = xNeu;
                y = yNeu;
                z = zNeu;
                printf("tried to align to border: x: %g y: %g z: %g angle: %g \n", x, y, z, angle);
            }
            else
            {
                z = terrainRoot->getTerrainHeight(x, y);
                if (z == -1000000.0f)
                {
                    z = trackRoot->getTerrainHeight(x, y);

                    if (z == -1000000.0f)
                    {
                        if (individual)
                        {
                            printf("WARNING: failed to find elevation for individual object: %s x: %g y: %g (track x: %g track y: %g)\n",
                                name.c_str(), x, y, x - trackOffsetX, y - trackOffsetY);
                        }
                        else
                        {
                            printf("WARNING: failed to find elevation for object map object: %s x: %g y: %g (track x: %g track y: %g)\n",
                                name.c_str(), x, y, x - trackOffsetX, y - trackOffsetY);
                        }
                        return;
                    }
                }
            }

            if (individual)
                printf("placing individual object: %s x: %g y: %g z: %g \n", name.c_str(), x, y, z);
            else
                printf("placing object map object: %s x: %g y: %g z: %g \n", name.c_str(), x, y, z);
            m.makeRotation(angle + orientation, dv / 2.0 - dv * rand() / (RAND_MAX + 1.0), dv / 2.0 - dv * rand() / (RAND_MAX + 1.0));
            obj.transform(m);

            m.makeLocation(x, y, z + height);
            obj.transform(m);
            obj.splitBySURF();
            obj.splitByMaterial();
            obj.splitByUV();

            objectsRoot.merge(obj, multipleMaterials || curObj.useMaterial);

            return;
        }
    }
}

bool
saveACInner(Ac3d::Object *ent, Ac3d &ac3d)
{
    /* WARNING - RECURSIVE! */

    if (ent->type != "poly")
    {
        ent->name = "objg" + std::to_string(ObjUniqId++);

        for (auto & kid : ent->kids)
        {
            if (!saveACInner(&kid, ac3d))
            {
                return false;
            }
        }

        return true;
    }

    ent->name = "obj" + std::to_string(ObjUniqId++);;

    return true;
}

/* insert one leaf in group */
void
InsertInGroup(Ac3d::Object *leaf, Ac3d::Object *GroupRoot, std::vector<Ac3d::Object *> &Groups)
{
    Ac3d::BoundingSphere boundingSphere = leaf->getBoundingSphere();
    const int grIdx = (int)((boundingSphere.center[0] - XGroupOffset) / GroupSize) +
        XGroupNb * (int)((boundingSphere.center[1] - YGroupOffset) / GroupSize);

    if (Groups[grIdx] == nullptr)
    {
        GroupRoot->kids.push_back(Ac3d::Object("group", ""));
        Groups[grIdx] = &GroupRoot->kids.back();
    }

    Groups[grIdx]->kids.push_back(*leaf);
}

/* insert leaves in groups */
void
InsertInner(Ac3d::Object *ent, Ac3d::Object *GroupRoot, std::vector<Ac3d::Object *> &Groups)
{
    /* WARNING - RECURSIVE! */

    if (ent->type != "poly")
    {
        for (auto & kid : ent->kids)
            InsertInner(&kid, GroupRoot, Groups);

        return;
    }

    InsertInGroup(ent, GroupRoot, Groups);
}

void
Group(tTrack *track, void *TrackHandle, Ac3d::Object *Root, Ac3d::Object *GroupRoot, std::vector<Ac3d::Object *> &Groups)
{
    const tdble Margin = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_BMARGIN, nullptr, DEFAULT_BORDER_MARGIN);
    GroupSize = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_GRPSZ, nullptr, DEFAULT_GROUP_SIZE);
    XGroupOffset = track->min.x - Margin;
    YGroupOffset = track->min.y - Margin;

    XGroupNb = (int)((track->max.x + Margin - (track->min.x - Margin)) / GroupSize) + 1;

    const int GroupNb = XGroupNb * ((int)((track->max.y + Margin - (track->min.y - Margin)) / GroupSize) + 1);

    Groups.resize(GroupNb);

    for (size_t i = 0; i < Groups.size(); ++i)
        Groups[i] = nullptr;

    InsertInner(Root, GroupRoot, Groups);

    int used = 0;
    for (size_t i = 0; i < Groups.size(); i++)
    {
        if (Groups[i] != nullptr)
            used++;
    }
    printf("%d groups : %d used\n", GroupNb, used);
}

} // end anonymous namespace

void
GenerateObjects(tTrack *track, void *TrackHandle, void *CfgHandle, Ac3d &allAc3d, bool all, const std::string &terrainFile, const std::string &trackFile, const std::string &outputFile, bool multipleMaterials, bool acc)
{
    std::string inputPath(track->filename);
    inputPath.resize(inputPath.find_last_of('/'));

    Ac3d::Object *TerrainRoot;
    Ac3d Terrain;
    if (!terrainFile.empty())
    {
        try
        {
            Terrain.readFile(terrainFile);
            Terrain.flipAxes(true);       // convert to track coordinate system
        }
        catch (const Ac3d::Exception &e)
        {
            GfOut("Reading terrain file %s: %s\n", terrainFile.c_str(), e.what());
            exit(1);
        }
        TerrainRoot = &Terrain.root;
    }
    else
    {
        Terrain.addObject(allAc3d.root.kids.front());
        Terrain.flipAxes(true);       // convert to track coordinate system
        TerrainRoot = &Terrain.root;
    }

    Ac3d::Object *TrackRoot;
    Ac3d Track;
    if (!trackFile.empty())
    {
        try
        {
            Track.readFile(trackFile);
            Track.flipAxes(true);       // convert to track coordinate system
        }
        catch (const Ac3d::Exception &e)
        {
            GfOut("Reading track file %s: %s\n", trackFile.c_str(), e.what());
            exit(1);
        }
        TrackRoot = &Track.root;
    }
    else
    {
        Track.addObject(allAc3d.root.kids.back());
        Track.flipAxes(true);       // convert to track coordinate system
        TrackRoot = &Track.root;
    }

    InitObjects(track, TrackHandle);

    const tdble Margin = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_BMARGIN, nullptr, DEFAULT_BORDER_MARGIN);

    const tdble xmin = track->min.x - Margin;
    const tdble xmax = track->max.x + Margin;
    const tdble ymin = track->min.y - Margin;
    const tdble ymax = track->max.y + Margin;

    int index = 0;

    if (GfParmGetEltNb(TrackHandle, TRK_SECT_TERRAIN_OBJMAP) != 0)
    {
        GfParmListSeekFirst(TrackHandle, TRK_SECT_TERRAIN_OBJMAP);

        do
        {
            Ac3d ObjectsRoot;

            index++;
            const char *map = GfParmGetCurStr(TrackHandle, TRK_SECT_TERRAIN_OBJMAP, TRK_ATT_OBJMAP, "");

            const std::string imageFile(inputPath + "/" + map);
            printf("Processing object map %s\n", imageFile.c_str());
            int	width, height;
            unsigned char *MapImage = GfTexReadImageFromPNG(imageFile.c_str(), 2.2, &width, &height, 0, 0, false);

            if (!MapImage)
            {
                return;
            }

            const tdble kX = (xmax - xmin) / width;
            const tdble dX = xmin;
            const tdble kY = (ymax - ymin) / height;
            const tdble dY = ymin;

            for (int j = 0; j < height; j++)
            {
                for (int i = 0; i < width; i++)
                {
                    const unsigned int clr = (MapImage[4 * (i + width * j)] << 16) + (MapImage[4 * (i + width * j) + 1] << 8) + MapImage[4 * (i + width * j) + 2];

                    if (clr)
                    {
                        printf("found color: 0x%X x: %d y: %d\n", clr, i, j);
                        AddObject(track, TrackHandle, TerrainRoot, TrackRoot, ObjectsRoot, clr, i * kX + dX, j * kY + dY, multipleMaterials, false);
                    }
                }
            }

            free(MapImage);

            Ac3d GroupRoot;
            if (ObjectsRoot.materials.empty())
                GroupRoot.addDefaultMaterial();
            else
                GroupRoot.materials = ObjectsRoot.materials;
            Ac3d::Object object("group", "");
            GroupRoot.addObject(object);
            std::vector<Ac3d::Object *> Groups;

            Group(track, TrackHandle, &ObjectsRoot.root, &GroupRoot.root.kids.front(), Groups);

            const char *extName = GfParmGetStr(CfgHandle, "Files", "object", "obj");
            const std::string objectFile(outputFile + "-" + extName + "-" + std::to_string(index) + ".ac");
            saveACInner(&GroupRoot.root.kids.front(), GroupRoot);
            GroupRoot.flipAxes(false); // convert to track coordinate system

            if (!acc)
                GroupRoot.writeFile(objectFile, false);

            if (all)
            {
                allAc3d.merge(GroupRoot, multipleMaterials);
            }
        } while (!GfParmListSeekNext(TrackHandle, TRK_SECT_TERRAIN_OBJMAP));
    }

    if (GfParmGetEltNb(TrackHandle, TRK_SECT_TERRAIN_OBJECTS) != 0)
    {
        GfParmListSeekFirst(TrackHandle, TRK_SECT_TERRAIN_OBJECTS);

        Ac3d ObjectsRoot;

        index++;

        do
        {
            const tdble x = GfParmGetCurNum(TrackHandle, TRK_SECT_TERRAIN_OBJECTS, TRK_ATT_X, "m", 0);
            const tdble y = GfParmGetCurNum(TrackHandle, TRK_SECT_TERRAIN_OBJECTS, TRK_ATT_Y, "m", 0);
            const unsigned int color = (unsigned int)GfParmGetCurNum(TrackHandle, TRK_SECT_TERRAIN_OBJECTS, TRK_ATT_COLOR, nullptr, 0);

            printf("found color: 0x%X x: %f y: %f\n", color, x, y);
            AddObject(track, TrackHandle, TerrainRoot, TrackRoot, ObjectsRoot, color, x + trackOffsetX, y + trackOffsetY, multipleMaterials, true);
        } while (!GfParmListSeekNext(TrackHandle, TRK_SECT_TERRAIN_OBJECTS));

        Ac3d GroupRoot;
        if (ObjectsRoot.materials.empty())
            GroupRoot.addDefaultMaterial();
        else
            GroupRoot.materials = ObjectsRoot.materials;
        Ac3d::Object object("group", "");
        GroupRoot.addObject(object);
        std::vector<Ac3d::Object *> Groups;

        Group(track, TrackHandle, &ObjectsRoot.root, &GroupRoot.root.kids.front(), Groups);

        const char *extName = GfParmGetStr(CfgHandle, "Files", "object", "obj");
        const std::string objectFile(outputFile + "-" + extName + "-" + std::to_string(index) + ".ac");
        saveACInner(&GroupRoot.root.kids.front(), GroupRoot);
        GroupRoot.flipAxes(false); // convert to track coordinate system

        if (!acc)
            GroupRoot.writeFile(objectFile, false);

        if (all)
        {
            allAc3d.merge(GroupRoot, multipleMaterials);
        }
    }
}
