/***************************************************************************

    file                 : OsgScenery.cpp
    created              : Mon Aug 21 20:13:56 CEST 2012
    copyright            : (C) 2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/Camera>

#include "OsgMain.h"
#include "OsgScenery.h"
#include "OsgLoader.h"

#include <glfeatures.h>	//gluXXX
#include <robottools.h>	//RtXXX()
#include <portability.h>

namespace osggraph {

double SDScenery::grWrldX = 0.0;
double SDScenery::grWrldY = 0.0;
double SDScenery::grWrldZ = 0.0;
double SDScenery::grWrldMaxSize = 0.0;

SDScenery::SDScenery(void) :
    m_background(NULL),
    m_pit(NULL),
    m_tracklights(NULL),
    _scenery(NULL)
{
    _max_visibility = 0;
    _nb_cloudlayer = 0;
    _DynamicSkyDome = 0;
    _SkyDomeDistance = 0;
    _SkyDomeDistThresh = 12000;

    _bgsky =  false;
    _speedWay = false;
    _speedWayLong = false;
}

SDScenery::~SDScenery(void)
{
    delete	m_background;
    delete  m_pit;
    delete  m_tracklights;

    if(_scenery != NULL)
    {
        _scenery->removeChildren(0, _scenery->getNumChildren());
        _scenery = NULL;
    }
}

int SDScenery::LoadScene(const tTrack *track)
{
    void		*hndl = grTrackHandle;
    const char	*acname;
    char 		buf[256];

    GfLogDebug("Initialisation class SDScenery\n");

    m_background = new SDBackground;
    m_pit = new SDPit;
    m_tracklights = new SDTrackLights;
    _scenery = new osg::Group;

    // Load graphics options.
    LoadGraphicsOptions();

    if(grHandle == NULL)
    {
        grHandle = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
    }//if grHandle

    /* Determine the world limits */
    grWrldX = (int)(track->max.x - track->min.x + 1);
    grWrldY = (int)(track->max.y - track->min.y + 1);
    grWrldZ = (int)(track->max.z - track->min.z + 1);
    grWrldMaxSize = (int)(MAX(MAX(grWrldX, grWrldY), grWrldZ));

    if (strcmp(track->category, "speedway") == 0)
    {
        _speedWay = true;
        if (strcmp(track->subcategory, "long") == 0)
            _speedWayLong = true;
        else
            _speedWayLong = false;
    }
    else
        _speedWay = false;

    GfLogDebug("SpeedWay = %d - SubCategory = %d\n", _speedWay, _speedWayLong);

    acname = GfParmGetStr(hndl, TRK_SECT_GRAPH, TRK_ATT_3DDESC, "track.ac");

    GfLogDebug("ACname = %s\n", acname);

    if (strlen(acname) == 0)
    {
        GfLogError("No specified track 3D model file\n");
    }

    _bgsky = strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_DYNAMICSKYDOME, GR_ATT_DYNAMICSKYDOME_DISABLED), GR_ATT_DYNAMICSKYDOME_ENABLED) == 0;
    if (_bgsky)
    {
        snprintf(buf, 256, "tracks/%s/%s/", track->category, track->internalname);
        m_background->build(grWrldX, grWrldY, grWrldZ, buf);
        GfLogDebug("Background loaded\n");
    }

    snprintf(buf, 256, "tracks/%s/%s/", track->category, track->internalname);

    std::string ext = osgDB::getFileExtension(acname);

    if (ext == "acc")
    {
        GfLogDebug("Load 3D Model Scene ACC\n");
        if (!LoadTrack(buf, acname))
        {
            GfLogError("LoadTrack %s%s failed\n", buf, acname);
            return -1;
        }
    }
    else
    {
        std::string localdir = GfLocalDir(), datadir = GfDataDir();
        osgDB::FilePathList pathList = osgDB::Registry::instance()->getDataFilePathList();
        pathList.push_back(localdir + buf);
        pathList.push_back(datadir + buf);
        pathList.push_back(datadir + "data/objects/");
        pathList.push_back(datadir + "data/textures/");
        osgDB::Registry::instance()->setDataFilePathList(pathList);
        osg::ref_ptr<osg::Node> pTrack = osgDB::readNodeFile(acname);

        if (ext =="ac")
        {
            osg::ref_ptr<osg::MatrixTransform> rot = new osg::MatrixTransform;
            osg::Matrix mat( 1.0f,  0.0f, 0.0f, 0.0f,
                0.0f,  0.0f, 1.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f,  0.0f, 0.0f, 1.0f);
            rot->setMatrix(mat);
            rot->addChild(pTrack);
            _scenery->addChild(rot.get());
        }
        else
        {
            _scenery->addChild(pTrack.get());
        }
    }

    m_pit->build(track);
    _scenery->addChild(m_pit->getPit());

    m_tracklights->build(track);
    //_scenery->addChild(m_tracklights->getTrackLight());

    osgDB::Registry::instance()->setDataFilePathList( osgDB::FilePathList() );
    osgDB::Registry::instance()->clearObjectCache();
    return 0;
}

void SDScenery::LoadSkyOptions()
{
    // Sky dome / background.
    _SkyDomeDistance = (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE, 0, 0) + 0.5);
    if (_SkyDomeDistance > 0 && _SkyDomeDistance < _SkyDomeDistThresh)
        _SkyDomeDistance = _SkyDomeDistThresh; // If user enabled it (>0), must be at least the threshold.

    _DynamicSkyDome = _SkyDomeDistance > 0 && strcmp(GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_DYNAMICSKYDOME, GR_ATT_DYNAMICSKYDOME_DISABLED),
                                                     GR_ATT_DYNAMICSKYDOME_ENABLED) == 0;

    GfLogDebug("Graphic options : Sky dome : distance = %u m, dynamic = %s\n",
              _SkyDomeDistance, _DynamicSkyDome ? "true" : "false");

    _max_visibility = (unsigned)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_VISIBILITY, 0, 0));
}

void SDScenery::LoadGraphicsOptions()
{
    if (!grHandle)
    {
        grHandle = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_REREAD);
    }//if grHandle

    LoadSkyOptions();
}

void SDScenery::ShutdownScene(void)
{
    //delete loader;
    _scenery->removeChildren(0, _scenery->getNumChildren());
    _scenery = NULL;
}

bool SDScenery::LoadTrack(const std::string &dir, const std::string &file)
{
    std::string localdir = GfLocalDir();
    osgLoader loader;
    loader.AddSearchPath(localdir + dir);
    loader.AddSearchPath(dir);
    loader.AddSearchPath("data/textures/");

    std::string path = dir + file;
    osg::Node *pTrack;
    osgLoader::loadedModels loaded;

    if ((pTrack = loader.Load3dFile(loaded, GfLocalDir() + path))
        || (pTrack = loader.Load3dFile(loaded, path)))
    {
        pTrack->getOrCreateStateSet()->setRenderBinDetails(TRACKBIN,"RenderBin");
        _scenery->addChild(pTrack);
#if 0
        std::string Tpath = GfLocalDir();
        Tpath = Tpath+"/track.osg";
        osgDB::writeNodeFile( *pTrack, Tpath);
#endif
    }
    else
        return false;

    return true;
}

void SDScenery::reposition(double X, double Y, double Z)
{
    m_background->reposition(X, Y, getWorldZ() / 2);
}

void SDScenery::update_tracklights(double currentTime, double totTime, int raceType)
{
    m_tracklights->update(currentTime, totTime, raceType);
}

} // namespace osggraph
