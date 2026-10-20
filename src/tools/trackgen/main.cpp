/***************************************************************************

    file                 : trackgen.cpp
    created              : Sat Dec 23 09:27:43 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr

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
#include <cstdio>
#include <cctype>

#include <sstream>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _WIN32
#include <getopt.h>
#endif

#ifdef _WIN32
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tgf.hpp>
#include <portability.h>
#include <itrackloader.h>

#include "ac3d.h"
#include "easymesh.h"
#include "objects.h"
#include "elevation.h"
#include "trackgen.h"
#include "util.h"


class Application : public GfApplication
{
    //! Command line options.
    int HeightSteps;
    bool Bump;
    bool Raceline;
    bool UseBorder;
    std::string TrackName;
    std::string TrackCategory;
    bool TrackOnly;
    bool JustCalculate;
    bool MergeAll;
    bool MergeTerrain;
    int DoSaveElevation;
    bool Bridge;
    bool Acc;
    bool DumpTrack;
    bool MultipleMaterials;
    std::string TrackXMLFilePath;   // Full path to track XML input file to read from (mainly for unittests)
    std::string TrackACFilePath;    // Full path to track AC output file to write into (mainly for unittests)

    double x_min = DBL_MAX;
    double y_min = DBL_MAX;
    double x_max = DBL_MIN;
    double y_max = DBL_MIN;

public:

    //! Constructor.
    Application();

    //! Initialization.
    virtual void initialize(bool bLoggingEnabled, int argc = 0, char **argv = 0);

    //! Parse the command line options.
    bool parseOptions();

    //! Generate the track.
    int generate();
};

//! Constructor.
Application::Application()
: GfApplication("TrackGen", "1.7.0.11", "Terrain generator for tracks")
, HeightSteps(30)
, Bump(false)
, Raceline(false)
, UseBorder(true)
, TrackOnly(true)
, JustCalculate(false)
, MergeAll(true)
, MergeTerrain(true)
, DoSaveElevation(-1)
, Bridge(true)
, Acc(false)
, DumpTrack(false)
, MultipleMaterials(false)
{
}

void Application::initialize(bool bLoggingEnabled, int argc, char **argv)
{
    // Base initialization first.
    GfApplication::initialize(bLoggingEnabled, argc, argv);

    // Specific options.
    registerOption("c", "category", /* bHasValue = */ true);
    registerOption("n", "name", /* bHasValue = */ true);
    registerOption("b", "bump", /* bHasValue = */ false);
    registerOption("r", "raceline", /* bHasValue = */ false);
    registerOption("B", "noborder", /* bHasValue = */ false);
    registerOption("a", "all", /* bHasValue = */ false);
    registerOption("z", "calc", /* bHasValue = */ false);
    registerOption("s", "split", /* bHasValue = */ false);
    registerOption("S", "splitall", /* bHasValue = */ false);
    registerOption("E", "saveelev", /* bHasValue = */ true);
    registerOption("H", "height4", /* bHasValue = */ true);
    registerOption("nb", "nobridge", /* bHasValue = */ false);
    registerOption("dt", "dumptrack", /* bHasValue = */ false);
    registerOption("i", "inpath", /* bHasValue = */ true);
    registerOption("o", "outpath", /* bHasValue = */ true);
    registerOption("m", "materials", /* bHasValue = */ false);
    registerOption("A", "acc", /* bHasValue = */ false);

    // Help on specific options.
    addOptionsHelpSyntaxLine("-c|--category <cat> -n|--name <name> [-b|--bump] [-r|--raceline] [-B|--noborder] [-nb|--nobridge] [-m|--materials]");
    addOptionsHelpSyntaxLine("[-a|--all] [-z|--calc] [-s|--split] [-S|--splitall] [-A|--acc]");
    addOptionsHelpSyntaxLine("[-E|--saveelev <#ef> [-H|--height4 <#hs>]] [-dt|--dumptrack]");
    addOptionsHelpSyntaxLine("[-i xml_path] [-o ac_path]");

    addOptionsHelpExplainLine("<cat>    : track category (road, speedway, dirt...)");
    addOptionsHelpExplainLine("<name>   : track name");
    addOptionsHelpExplainLine("bump     : draw bump track");
    addOptionsHelpExplainLine("raceline : draw raceline track\n");
    addOptionsHelpExplainLine("noborder : don't use terrain border "
                              "(relief supplied int clockwise, ext CC)");
    addOptionsHelpExplainLine("all      : draw all (default is track only)");
    addOptionsHelpExplainLine("calc     : only calculate track parameters and exit");
    addOptionsHelpExplainLine("split    : split the track and the terrain");
    addOptionsHelpExplainLine("splitall : split all");
    addOptionsHelpExplainLine("<#ef>    : # of the elevation file to save");
    addOptionsHelpExplainLine("  0: all elevation files");
    addOptionsHelpExplainLine("  1: elevation file of terrain + track");
    addOptionsHelpExplainLine("  2: elevation file of terrain with track white");
    addOptionsHelpExplainLine("  3: track only");
    addOptionsHelpExplainLine("  4: track elevations with height steps");
    addOptionsHelpExplainLine("<#hs> : nb of height steps for 4th elevation file [30]");
    addOptionsHelpExplainLine("materials : mulitple materials");
    addOptionsHelpExplainLine("dumptrack : dump track segments to file name.dump");
}

// Parse the command line options.
bool Application::parseOptions()
{
    // Parse command line for registered options, and interpret standard ones.
    if (!GfApplication::parseOptions())
        return false;

    std::list<Option>::const_iterator itOpt;
    for (itOpt = _lstOptions.begin(); itOpt != _lstOptions.end(); ++itOpt)
    {
        // Not found in the command line => ignore / default value.
        if (!itOpt->bFound)
            continue;

        if (itOpt->strLongName == "all")
        {
            TrackOnly = false;
        }
        else if (itOpt->strLongName == "calc")
        {
            JustCalculate = true;
        }
        else if (itOpt->strLongName == "bump")
        {
            Bump = true;
        }
        else if (itOpt->strLongName == "raceline")
        {
            Raceline = true;
        }
        else if (itOpt->strLongName == "acc")
        {
            Acc = true;
            TrackOnly = false;
        }
        else if (itOpt->strLongName == "split")
        {
            MergeAll = false;
            MergeTerrain = true;
        }
        else if (itOpt->strLongName == "splitall")
        {
            MergeAll = false;
            MergeTerrain = false;
        }
        else if (itOpt->strLongName == "noborder")
        {
            UseBorder = false;
        }
        else if (itOpt->strLongName == "name")
        {
            TrackName = itOpt->strValue;
        }
        else if (itOpt->strLongName == "saveelev")
        {
            DoSaveElevation = strtol(itOpt->strValue.c_str(), nullptr, 0);
            TrackOnly = false;
        }
        else if (itOpt->strLongName == "category")
        {
            TrackCategory = itOpt->strValue;
        }
        else if (itOpt->strLongName == "height4")
        {
            HeightSteps = strtol(itOpt->strValue.c_str(), nullptr, 0);
        }
        else if (itOpt->strLongName == "nobridge")
        {
            Bridge = false;
        }
        else if (itOpt->strLongName == "dumptrack")
        {
            DumpTrack = true;
        }
        else if (itOpt->strLongName == "inpath")
        {
            TrackXMLFilePath = itOpt->strValue;
        }
        else if (itOpt->strLongName == "outpath")
        {
            TrackACFilePath = itOpt->strValue;
        }
        else if (itOpt->strLongName == "materials")
        {
            MultipleMaterials = true;
        }
    }

    if (TrackName.empty() || TrackCategory.empty())
    {
        printUsage("No track name or category specified");
        return false;
    }

    return true;
}

int Application::generate()
{
    int ret = EXIT_FAILURE;
    void *CfgHandle = nullptr, *TrackHandle = nullptr;
    tTrack *Track = nullptr;
    std::string useObjectMaterials;
    ITrackLoader* PiTrackLoader = nullptr;
    char trackdef[1024];
    char buf2[1024];
    std::string OutputFileName, OutTrackName, OutMeshName, TiledFile;
    Ac3d allAc3d;
    const char *extName;
    bool all = false;
    std::ostringstream ossModLibName;
    GfModule *pmodTrkLoader = nullptr;

    ssgAddTextureFormat(".rgb", ssgLoadSGI);
    ssgAddTextureFormat(".rgba", ssgLoadSGI);
    ssgAddTextureFormat(".int", ssgLoadSGI);
    ssgAddTextureFormat(".inta", ssgLoadSGI);
    ssgAddTextureFormat(".bw", ssgLoadSGI);
    ssgAddTextureFormat(".png", loadPngTexture);
    ssgAddTextureFormat(".jpg", loadJpegTexture);

    // Get the trackgen paramaters.
    CfgHandle = GfParmReadFile(CFG_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    if (!CfgHandle) {
        GfLogError("Cannot find %s\n", CFG_FILE);
        goto end;
    }

    // Load and initialize the track loader module.
    GfLogInfo("Loading Track Loader ...\n");
    ossModLibName << GfLibDir() << "modules/track/" << "trackv1" << DLLEXT;
    pmodTrkLoader = GfModule::load(ossModLibName.str());
    if (!pmodTrkLoader) {
        GfLogError("Cannot find %s\n", ossModLibName.str().c_str());
        goto end;
    }

    // Check that it implements ITrackLoader.
    PiTrackLoader = pmodTrkLoader->getInterface<ITrackLoader>();
    if (!PiTrackLoader) {
        GfLogError("Cannot find ITrackLoader interface\n");
        goto end;
    }

    // This is the track definition.
    if (TrackXMLFilePath.empty())
        snprintf(trackdef, sizeof(trackdef), "tracks/%s/%s/%s.xml", TrackCategory.c_str(), TrackName.c_str(), TrackName.c_str());
    else
        snprintf(trackdef, sizeof(trackdef), "%s/%s.xml", TrackXMLFilePath.c_str(), TrackName.c_str());

    TrackHandle = GfParmReadFile(trackdef, GFPARM_RMODE_STD);
    if (!TrackHandle) {
        GfLogError("Cannot find %s\n", trackdef);
        goto end;
    }

    // Build the track structure with graphic extensions.
    Track = PiTrackLoader->load(trackdef, true);
    if (!Track) {
        GfLogError("Failed to load track from %s\n", trackdef);
        goto end;
    }

    if (JustCalculate) {
        CalculateTrack(Track, TrackHandle, Bump, Raceline, Bridge, Acc);
        ret = EXIT_SUCCESS;
        goto end;
    }

    // dump the track segments to a file for debugging
    if (DumpTrack)
        dumpTrackSegs(Track);

    // Get the output file radix.
    if (TrackACFilePath.empty())
        snprintf(buf2, sizeof(buf2), "tracks/%s/%s/%s", Track->category, Track->internalname, Track->internalname);
    else
        snprintf(buf2, sizeof(buf2), "%s/%s", TrackACFilePath.c_str(), Track->internalname);

    OutputFileName = buf2;

    // Number of groups for the complete track.
    allAc3d.addDefaultMaterial();

    if (TrackOnly) {
        // Track.
        if (!Bump && !Raceline)
            all = true;
    } else if (MergeAll) {
        // track + terrain + objects.
        all = true;
    }

    // Main Track.
    if (Bump) {
        extName = "trk-bump";
    } else if (Raceline) {
        extName = "trk-raceline";
    } else {
        extName = "trk";
    }

    if (!Acc)
        snprintf(buf2, sizeof(buf2), "%s-%s.ac", OutputFileName.c_str(), extName);
    else
        buf2[0] = 0;

    OutTrackName = buf2;

    GenerateTrack(Track, TrackHandle, OutTrackName, allAc3d, all, Bump, Raceline, Bridge, Acc);

    if (TrackOnly) {
        ret = EXIT_SUCCESS;
        goto end;
    }

    // Terrain.
    if (MergeTerrain && !MergeAll) {
        /* terrain + objects  */
        all = true;
    }

    if (!Acc)
        OutMeshName = OutputFileName + "-msh.ac";

    GenerateTerrain(Track, TrackHandle, OutMeshName, allAc3d, all, DoSaveElevation, UseBorder, Acc);

    // add or set tiled texture uv coordinates for acc files with a tiled texture
    TiledFile = GfParmGetStr(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_TILED, "");
    if (Acc && !TiledFile.empty())
    {
        std::vector<Ac3d::Object *> polys;

        allAc3d.getPolys(polys);

        for (auto *poly : polys)
        {
            for (const auto &vertex : poly->vertices)
            {
                if (vertex[0] > x_max)
                    x_max = vertex[0];
                else if (vertex[0] < x_min)
                    x_min = vertex[0];

                // flip vertices
                if (-vertex[2] > y_max)
                    y_max = vertex[2];
                else if (-vertex[2] < y_min)
                    y_min = vertex[2];
            }
        }

        for (auto *poly : polys)
        {
            if (poly->textures.size() == 1)
                poly->textures.emplace_back(TiledFile, "tiled");
            else if (poly->textures.size() > 2)
            {
                poly->textures[1].name = TiledFile;
                poly->textures[1].type = "tiled";
            }

            std::vector<Ac3d::V2d> uvs(poly->vertices.size());

            for (size_t i = 0; i < poly->vertices.size(); i++)
            {
                uvs[i][0] = (poly->vertices[i][0] - x_min) / (x_max - x_min);
                uvs[i][1] = (poly->vertices[i][1] - y_min) / (y_max - y_min);
            }

            for (auto &surf : poly->surfaces)
            {
                for (auto &ref : surf.refs)
                {
                    if (ref.coords.empty())
                        ref.coords.emplace_back(0, 0);
                    if (ref.coords.size() == 1)
                        ref.coords.emplace_back(uvs[ref.index][0], uvs[ref.index][1]);
                    else
                    {
                        ref.coords[1][0] = uvs[ref.index][0];
                        ref.coords[1][1] = uvs[ref.index][1];
                    }
                }
            }
        }
    }

    if (!Acc && DoSaveElevation != -1) {
        if (all) {
            allAc3d.removeBadTriangles();
            allAc3d.removeEmptyObjects();
            allAc3d.writeFile(OutputFileName + ".ac", false);
        }
        switch (DoSaveElevation) {
            case 0:
            case 1:
                SaveElevation(Track, TrackHandle, OutputFileName + "-elv.png", OutputFileName + ".ac", 1, HeightSteps);
                if (DoSaveElevation) {
                    break;
                }
                SD_FALLTHROUGH // [[fallthrough]]
            case 2:
                SaveElevation(Track, TrackHandle, OutputFileName + "-elv2.png", OutMeshName, 1, HeightSteps);
                if (DoSaveElevation) {
                    break;
                }
                SD_FALLTHROUGH // [[fallthrough]]
            case 3:
                SaveElevation(Track, TrackHandle, OutputFileName + "-elv3.png", OutMeshName, 0, HeightSteps);
                if (DoSaveElevation) {
                    break;
                }
                SD_FALLTHROUGH // [[fallthrough]]
            case 4:
                SaveElevation(Track, TrackHandle, OutputFileName + "-elv4.png", OutTrackName, 2, HeightSteps);
                break;
        }

        ret = EXIT_SUCCESS;
        goto end;
    }

    // check if we should use the object's materials
    useObjectMaterials = GfParmGetStr(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_USE_OBJ_MATERIALS, "no");

    if (useObjectMaterials == "yes")
        MultipleMaterials = true;

    GenerateObjects(Track, TrackHandle, CfgHandle, allAc3d, all, OutMeshName, OutTrackName, OutputFileName, MultipleMaterials, Acc);

    allAc3d.removeBadTriangles();
    allAc3d.removeEmptyObjects();

    if (!Acc)
        allAc3d.writeFile(OutputFileName + ".ac", false);
    else
    {
        allAc3d.generateNormals();
        allAc3d.writeFile(OutputFileName + ".acc", false);
    }

    ret = EXIT_SUCCESS;

end:
    if (TrackHandle)
        GfParmReleaseHandle(TrackHandle);

    if (CfgHandle)
        GfParmReleaseHandle(CfgHandle);

    if (PiTrackLoader)
        PiTrackLoader->unload();

    if (pmodTrkLoader)
        GfModule::unload(pmodTrkLoader);

    return ret;
}


int main(int argc, char **argv)
{
    // Create and initialize the application
    Application app;
    app.initialize(/*bLoggingEnabled=*/true, argc, argv);

    // Parse the command line options
    if (!app.parseOptions())
        return EXIT_FAILURE;

    // If "data dir" specified in any way, cd to it.
    if (chdir(GfDataDir()))
    {
        GfLogError("Could not start %s : failed to cd to the datadir '%s' (%s)\n",
                   app.name().c_str(), GfDataDir(), strerror(errno));
        return EXIT_FAILURE;
    }

    // Do the requested job.
    return app.generate();
}
