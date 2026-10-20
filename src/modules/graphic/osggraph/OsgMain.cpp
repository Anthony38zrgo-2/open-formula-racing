/***************************************************************************

    file                 : OsgMain.cpp
    created              : Thu Aug 17 23:23:49 CEST 2000
    copyright            : (C)2013 by Xavier Bertaux
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

#include <osgDB/Registry>
#include <osg/Group>
#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
#include <osgViewer/Viewer>
#include <osgUtil/Optimizer>

#include <glfeatures.h> // GfglFeatures
#include <robot.h>	//ROB_SECT_ARBITRARY
#include <graphic.h>

#include "OsgMain.h"
//#include "OsgOptions.h"
#include "OsgCar.h"
#include "OsgCarLight.h"
#include "OsgScenery.h"
#include "OsgRender.h"
#include "OsgMath.h"
#include "OsgScreens.h"
#include "OsgHUD.h"
#include "OsgParticles.h"

#include <raceman.h>	//tSituation

namespace osggraph {

//extern	osg::Timer m_timer;
//extern	osg::Timer_t m_start_tick;

//SDOptions *Options = 0;
SDCarLights *carLights = 0;
SDCars *cars = 0;
SDScenery *scenery = 0;
SDRender *render = 0;
SDScreens *screens = 0;
SDCamera *cam = 0;

int Rain, Clouds = 0;
tdble TimeOfDay = 0.0f;

SDHUD hud;

SDParticleSystemManager ParticleManager;


/*oid *getOptions()
{
    return Options;
}*/

SDScreens *getScreens()
{
    return screens;
}

SDRender *getRender()
{
    return render;
}

SDCars * getCars()
{
    return cars;
}

SDCarLights * getCarLights()
{
    return carLights;
}

SDScenery * getScenery()
{
    return scenery;
}

static osg::Timer m_timer;

int grMaxTextureUnits = 0;

tdble grMaxDammage = 10000.0;

void *grHandle = 0;
void *grTrackHandle = 0;

int m_Winx, m_Winy, m_Winw, m_Winh;

tdble grLodFactorValue = 1.0;

// Frame/FPS info.
static SDFrameInfo frameInfo;
static double fFPSPrevInstTime;   // Last "instant" FPS refresh time
static unsigned nFPSTotalSeconds; // Total duration since initView

// Mouse coords graphics backend to screen ratios.
static float fMouseRatioX, fMouseRatioY;

// Number of active screens.
int m_NbActiveScreens = 1;
int m_NbArrangeScreens = 0;

// Current screen index.
static int m_CurrentScreenIndex = 0;

static void SDPrevCar(void * /* dummy */)
{
    screens->getActiveView()->selectPrevCar();
}

static void SDNextCar(void * /* dummy */)
{
    screens->getActiveView()->selectNextCar();
}

static void SDSelectCamera(int cam)
{
    screens->changeCamera(cam);
}

static void SDDriverView(void *)
{
    SDSelectCamera(0);
}

static void SDCarView(void *)
{
    SDSelectCamera(1);
}

static void SDSideCarView(void *)
{
    SDSelectCamera(2);
}

static void SDUpCarView(void *)
{
    SDSelectCamera(3);
}

static void SDPerspCarView(void *)
{
    SDSelectCamera(4);
}

static void SDAllCircuitView(void *)
{
    SDSelectCamera(5);
}

static void SDTrackView(void *)
{
    SDSelectCamera(6);
}

static void SDTrackViewZoomed(void *)
{
    SDSelectCamera(7);
}

static void SDFollowCarZoomed(void *)
{
    SDSelectCamera(8);
}

static void SDTVDirectorView(void *)
{
    SDSelectCamera(9);
}

static void SDSelectCameraTemporaryOn(void *vp)//temporary select a different camera
{
    screens->changeCameraTemporaryOn();
}

static void SDSelectCameraTemporaryOff(void *vp)//temporary select a different camera
{
    screens->changeCameraTemporaryOff();
}

static void SDSetZoom(int zoom)
{
    screens->getActiveView()->getCameras()->getSelectedCamera()->setZoom(zoom);
}

static void SDSetZoomIn(void *vp)
{
    SDSetZoom(GR_ZOOM_IN);
}

static void SDSetZoomOut(void *vp)
{
    SDSetZoom(GR_ZOOM_OUT);
}

static void SDSetZoomMinimum(void *vp)
{
    SDSetZoom(GR_ZOOM_MIN);
}

static void SDSetZoomMaximum(void *vp)
{
    SDSetZoom(GR_ZOOM_MAX);
}

static void SDSetZoomDefault(void *vp)
{
    SDSetZoom(GR_ZOOM_DFLT);
}

void SDSwitchMirror(void *vp)
{
    screens->getActiveView()->switchMirror();
}

/*OSG hud widgets toggle*/
void SDToggleHUD(void *vp)
{
    screens->toggleHUD();
}

void SDToggleHUDwidget(void *vp)
{
    screens->toggleHUDwidget(static_cast<const char *>(vp));
}

void SDToggleHUDwidgets(void *vp)
{
    screens->toggleHUDwidgets(static_cast<const char *>(vp));
}

void SDToggleHUDdriverinput(void *vp)
{
    screens->toggleHUDdriverinput();
}

void SDToggleHUDeditmode(void *vp)
{
    screens->toggleHUDeditmode();
}

/*Driver position change*/
static void SDMoveSeatUpDown(float z)
{
    //if positive up if negative down
    float movement = 0.005 * z;
    tCarElt* curCar = screens->getActiveView()->getCurrentCar();
    curCar->_drvPos_z+=movement;
}

static void SDMoveSeatUp(void *)
{
    SDMoveSeatUpDown(1);
}

static void SDMoveSeatDown(void *)
{
    SDMoveSeatUpDown(-1);
}

static void SDMoveSeatLeftRight(float y)
{
    //if positive left if negative right
    float movement = 0.005 * y;
    tCarElt* curCar = screens->getActiveView()->getCurrentCar();
    curCar->_drvPos_y+=movement;
}

static void SDMoveSeatLeft(void *)
{
    SDMoveSeatLeftRight(1);
}

static void SDMoveSeatRight(void *)
{
    SDMoveSeatLeftRight(-1);
}

static void SDMoveSeatForwardBackward(float x)
{
    //if positive forward if negative backward
    float movement = 0.005 * x;
    tCarElt* curCar = screens->getActiveView()->getCurrentCar();
    curCar->_drvPos_x+=movement;
}

static void SDMoveSeatForward(void *)
{
    SDMoveSeatForwardBackward(1);
}

static void SDMoveSeatBackward(void *)
{
    SDMoveSeatForwardBackward(-1);
}

void SDToggleStats(void *vp)
{
    screens->toggleStats();
}


int initView(int x, int y, int width, int height, int /* flag */, void *screen)
{
    screens = new SDScreens();

    m_Winx = x;
    m_Winy = y;
    m_Winw = width;
    m_Winh = height;

    fMouseRatioX = width / 640.0;
    fMouseRatioY = height / 480.0;

    frameInfo.fInstFps = 0.0;
    frameInfo.fAvgFps = 0.0;
    frameInfo.fMinFps = 100000.0;
    frameInfo.fMaxFps = 0.0;
    frameInfo.nInstFrames = 0;
    frameInfo.nTotalFrames = 0;
    fFPSPrevInstTime = GfTimeClock();
    nFPSTotalSeconds = 0;

    screens->Init(x,y,width,height, render->getRoot(), render->getFogColor());

    GfuiAddKey(screen, GFUIK_END,      "Zoom Minimum", NULL,	SDSetZoomMinimum, NULL);
    GfuiAddKey(screen, GFUIK_HOME,     "Zoom Maximum", NULL,	SDSetZoomMaximum, NULL);
    GfuiAddKey(screen, '*',            "Zoom Default", NULL,	SDSetZoomDefault, NULL);

    GfuiAddKey( screen, GFUIK_PAGEUP,   "Select Previous Car", (void*)0, SDPrevCar, NULL);
    GfuiAddKey( screen, GFUIK_PAGEDOWN, "Select Next Car",     (void*)0, SDNextCar, NULL);

    GfuiAddKey(screen, GFUIK_F2,       "Driver Views",      NULL, SDDriverView, NULL);
    GfuiAddKey(screen, GFUIK_F3,       "Car Views",         NULL, SDCarView, NULL);
    GfuiAddKey(screen, GFUIK_F4,       "Side Car Views",    NULL, SDSideCarView, NULL);
    GfuiAddKey(screen, GFUIK_F5,       "Up Car View",       NULL, SDUpCarView, NULL);
    GfuiAddKey(screen, GFUIK_F6,       "Persp Car View",    NULL, SDPerspCarView, NULL);
    GfuiAddKey(screen, GFUIK_F7,       "All Circuit Views", NULL, SDAllCircuitView, NULL);
    GfuiAddKey(screen, GFUIK_F8,       "Track View",        NULL, SDTrackView, NULL);
    GfuiAddKey(screen, GFUIK_F9,       "Track View Zoomed", NULL, SDTrackViewZoomed, NULL);
    GfuiAddKey(screen, GFUIK_F10,      "Follow Car Zoomed", NULL, SDFollowCarZoomed, NULL);
    GfuiAddKey(screen, GFUIK_F11,      "TV Director View",  NULL, SDTVDirectorView, NULL);

    GfuiAddKey(screen, '?',            "Toggle OSG Stats",  (void *)0, SDToggleStats, NULL);

    GfuiAddKey(screen, '1',            "Toggle HUD laptimeWidget",     (void*)"laptimeWidget",     SDToggleHUDwidget, NULL);
    GfuiAddKey(screen, '2',            "Toggle HUD carinfoWidget",     (void*)"carinfoWidget,carstatusWidget", SDToggleHUDwidgets, NULL);
    GfuiAddKey(screen, '3',            "Toggle HUD boardWidget",       (void*)"boardWidget",       SDToggleHUDwidget, NULL);
    GfuiAddKey(screen, '4',            "Toggle HUD driverinputWidget", (void*)0, SDToggleHUDdriverinput, NULL);
    GfuiAddKey(screen, '5',            "Toggle HUD deltaWidget",       (void*)"deltaWidget",       SDToggleHUDwidget, NULL);
    GfuiAddKey(screen, '6',            "Toggle HUD dashitemsWidget",   (void*)"dashitemsWidget",   SDToggleHUDwidget, NULL);
    GfuiAddKey(screen, '7',            "Toggle HUD raceinfoWidget",    (void*)"racepositionWidget,racelapsWidget", SDToggleHUDwidgets, NULL);
    GfuiAddKey(screen, '8',            "Toggle HUD",                   (void*)0, SDToggleHUD, NULL);

    GfuiAddKey(screen, '1', GFUIM_CTRL, "Toggle HUD textFPSWidget",     (void*)"textFPSWidget",     SDToggleHUDwidget, NULL, GFUI_HELP_RIGHT);
    GfuiAddKey(screen, '2', GFUIM_CTRL, "Toggle HUD tiresWidget",       (void*)"tiresWidget",       SDToggleHUDwidget, NULL, GFUI_HELP_RIGHT);
    GfuiAddKey(screen, '3', GFUIM_CTRL, "Toggle HUD gforceWidget",      (void*)"gforceWidget",      SDToggleHUDwidget, NULL, GFUI_HELP_RIGHT);
    GfuiAddKey(screen, '4', GFUIM_CTRL, "Toggle HUD graphFPSWidget",    (void*)"graphFPSWidget",    SDToggleHUDwidget, NULL, GFUI_HELP_RIGHT);
    GfuiAddKey(screen, '5', GFUIM_CTRL, "Toggle HUD graphSpeedWidget",  (void*)"graphSpeedWidget",  SDToggleHUDwidget, NULL, GFUI_HELP_RIGHT);
    GfuiAddKey(screen, '6', GFUIM_CTRL, "Toggle HUD graphFFBWidget",    (void*)"graphFFBWidget",    SDToggleHUDwidget, NULL, GFUI_HELP_RIGHT);
    GfuiAddKey(screen, '7', GFUIM_CTRL, "Toggle HUD graphInputsWidget", (void*)"graphInputsWidget", SDToggleHUDwidget, NULL, GFUI_HELP_RIGHT);
    GfuiAddKey(screen, '8', GFUIM_CTRL, "Toggle HUD edit mode",         NULL,                   SDToggleHUDeditmode, NULL, GFUI_HELP_RIGHT);

    /*GfuiAddKey(screen, '5',            "Debug Info",        (void*)3, grSelectBoard, NULL);
    GfuiAddKey(screen, '4',            "G/Cmd Graph",       (void*)4, grSelectBoard, NULL);
    GfuiAddKey(screen, '3',            "Leaders Board",     (void*)2, grSelectBoard, NULL);
    GfuiAddKey(screen, '2',            "Driver Counters",   (void*)1, grSelectBoard, NULL);
    GfuiAddKey(screen, '1',            "Driver Board",      (void*)0, grSelectBoard, NULL);*/
    GfuiAddKey(screen, '9',            "Mirror",            (void*)0, SDSwitchMirror, NULL);


    //GfuiAddKey(screen, '0',            "Arcade Board",      (void*)5, grSelectBoard, NULL);*/
    GfuiAddKey(screen, '+', GFUIM_CTRL, "Zoom In",           NULL,	 SDSetZoomIn, NULL);
    GfuiAddKey(screen, '=', GFUIM_CTRL, "Zoom In",           NULL,	 SDSetZoomIn, NULL);
    GfuiAddKey(screen, '-', GFUIM_CTRL, "Zoom Out",          NULL, SDSetZoomOut, NULL);
    //GfuiAddKey(screen, '>',             "Zoom In",           (void*)GR_ZOOM_IN,	 SDSetZoom, NULL);
    //GfuiAddKey(screen, '<',             "Zoom Out",          (void*)GR_ZOOM_OUT, SDSetZoom, NULL);
    //GfuiAddKey(screen, '(',            "Split Screen",   (void*)SD_SPLIT_ADD, SDSplitScreen, NULL);
    //GfuiAddKey(screen, ')',            "UnSplit Screen", (void*)SD_SPLIT_REM, SDSplitScreen, NULL);
    //GfuiAddKey(screen, '_',            "Split Screen Arrangement", (void*)SD_SPLIT_ARR, SDSplitScreen, NULL);
    //GfuiAddKey(screen, GFUIK_TAB,      "Next (split) Screen", (void*)SD_NEXT_SCREEN, SDChangeScreen, NULL);
    /*GfuiAddKey(screen, 'm',            "Track Maps",          (void*)0, grSelectTrackMap, NULL);*/

    GfuiAddKey(screen, '+',        GFUIM_ALT,   "Move seat up",         NULL,  SDMoveSeatUp, NULL);
    GfuiAddKey(screen, '-',        GFUIM_ALT,   "Move seat down",       NULL, SDMoveSeatDown, NULL);
    GfuiAddKey(screen, GFUIK_LEFT, GFUIM_ALT,   "Move seat left",       NULL,  SDMoveSeatLeft, NULL);
    GfuiAddKey(screen, GFUIK_RIGHT,GFUIM_ALT,   "Move seat right",      NULL, SDMoveSeatRight, NULL);
    GfuiAddKey(screen, GFUIK_UP,   GFUIM_ALT,   "Move seat forward",    NULL,  SDMoveSeatForward, NULL);
    GfuiAddKey(screen, GFUIK_DOWN, GFUIM_ALT,   "Move seat backward",   NULL, SDMoveSeatBackward, NULL);

    if(GfScrUsingResizableWindow())
        GfuiAddKey(screen, GFUIK_RETURN, GFUIM_ALT, "Toggle Full-screen", (void*)0, GfScrToggleFullScreen, NULL);

    GfLogInfo("Current screen is #%d (out of %d)\n", m_CurrentScreenIndex, m_NbActiveScreens);

    return 0; // true;
}

void adaptScreenSize()
{
    // TODO need to resize 'screens'
    //GfScrGetSize(&grWinx, &grWiny, &grWinw, &grWinh);
}

int refresh(tSituation *s)
{
    static bool ranOnce;

    if (!ranOnce)
    {
        osgUtil::Optimizer optimizer;

        optimizer.optimize(render->getRoot());
        ranOnce = true;
    }

    if (frameInfo.nTotalFrames < 2)
    {
        // skip first 2 frames
        fFPSPrevInstTime = GfTimeClock();
        frameInfo.nTotalFrames++;
    }
    else
    {
        // Compute F/S indicators every second.
        frameInfo.nInstFrames++;
        frameInfo.nTotalFrames++;
        const double dCurTime = GfTimeClock();
        const double dDeltaTime = dCurTime - fFPSPrevInstTime;

        if (dDeltaTime > 1.0)
        {
            ++nFPSTotalSeconds;
            fFPSPrevInstTime = dCurTime;
            frameInfo.fInstFps = frameInfo.nInstFrames / dDeltaTime;
            frameInfo.nInstFrames = 0;
            frameInfo.fAvgFps = (double)(frameInfo.nTotalFrames - 2) / nFPSTotalSeconds;
            if (frameInfo.fInstFps > frameInfo.fMaxFps)
                frameInfo.fMaxFps = frameInfo.fInstFps;
            if (frameInfo.fInstFps < frameInfo.fMinFps)
                frameInfo.fMinFps = frameInfo.fInstFps;

            // Trace F/S every 5 seconds.
            if (nFPSTotalSeconds % 5 == 2)
                GfLogDebug("Frame rate (F/s) : Instant = %.1f (Average %.1f Minimum %.1f Maximum %.1f)\n",
                    frameInfo.fInstFps, frameInfo.fAvgFps, frameInfo.fMinFps, frameInfo.fMaxFps);
        }
    }
    adaptScreenSize();

    tCarElt* curCar = screens->getActiveView()->getCurrentCar();

    //switch to look back camera if requested by the driver
    if (curCar->_lookback == true){
        SDSelectCameraTemporaryOn((void*)0);
    }else{
        SDSelectCameraTemporaryOff((void*)0);
    }

    cam = screens->getActiveView()->getCameras()->getSelectedCamera();
    osg::Vec3d eye = cam->getCameraPosition();
    double X = eye[0];
    double Y = eye[1];
    double Z = eye[2];
    render->UpdateSky(s->currentTime, s->accelTime, X, Y);

    int drawDriver = cam->getDrawDriver();
    int drawCurrent = cam->getDrawCurrent();
    cars->updateCars(s, curCar, drawCurrent, drawDriver);

    scenery->reposition(X, Y, Z );
    scenery->update_tracklights(s->currentTime, s->_totTime, s->_raceType);

    screens->update(s, &frameInfo, osg::Vec4f(render->getFogColor(),1.0f));

    //refresh the hud
    hud.Refresh(s, &frameInfo, curCar, Clouds, Rain, TimeOfDay);

    //update particles
    ParticleManager.update(s);

    return 0;
}

void shutdownCars(void)
{
    if (cars)
    {
        cars->unLoad();
        delete cars;
        cars = NULL;
        GfLogInfo("Delete cars in OsgMain\n");
    }

    if (carLights)
    {
        delete carLights;
        carLights = NULL;
        GfLogInfo("Delete carLights in OsgMain\n");
    }

    //shutdown particles
    ParticleManager.shutdown();

    // Trace final mean F/s.
    if (nFPSTotalSeconds > 0)
        GfLogTrace("Average frame rate: %.2f F/s\n",
                   (double)frameInfo.nTotalFrames/((double)nFPSTotalSeconds + GfTimeClock() - fFPSPrevInstTime));
}

int initTrack(tTrack *track)
{
    // The inittrack does as well init the context, that is highly inconsistent, IMHO.
    // TODO: Find a solution to init the graphics first independent of objects.

    // Now, do the real track loading job.
    grTrackHandle = GfParmReadFileBoth(track->filename, GFPARM_RMODE_STD);
    if (!grTrackHandle) {
        GfLogError("GfParmReadFileBoth %s failed\n", track->filename);
        return -1;
    }

    //Options = new SDOptions;

    scenery = new SDScenery;
    render = new SDRender;
    //cam = new SDCamera;

    if (scenery->LoadScene(track))
        return -1;

    render->Init(track);

    Clouds = track->local.clouds;
    Rain = track->local.rain;
    TimeOfDay = track->local.timeofday;

    return 0;
}

int  initCars(tSituation *s)
{
    osgLoader::loadedModels loaded;

    GfLogInfo("InitCars\n");
    carLights = new SDCarLights;
    cars = new SDCars;
    carLights->loadStates();
    cars->loadCars(s, scenery->getSpeedWay(), scenery->getSpeedWayLong(), loaded);
    render->addCars(cars->getCarsNode(), carLights->getLightsRoot());
    GfLogInfo("All cars loaded\n");

    screens->InitCars(s);

    if (!grHandle)
    {
        grHandle = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    }

    //initialize particles
    ParticleManager.initialize(s);

    return 0;
}

void shutdownTrack(void)
{
    // Do the real track termination job.
    osgDB::Registry::instance()->clearObjectCache();

    if (grTrackHandle)
    {
        GfParmReleaseHandle(grTrackHandle);
        grTrackHandle = 0;
    }

    if (scenery)
    {
        scenery->ShutdownScene();
        delete scenery;
        scenery = NULL;
        GfLogInfo("Delete scenery in OsgMain\n");
    }
}

void shutdownView(void)
{
    GfLogInfo("Frame rate (F/s) : Average %.1f Minimum %.1f Maximum %.1f\n",
        frameInfo.fAvgFps, frameInfo.fMinFps, frameInfo.fMaxFps);

    if (screens)
    {
        delete screens;
        screens = NULL;
        GfLogInfo("Delete screens in OsgMain\n");
    }

    if (render)
    {
        delete render;
        render = NULL;
        GfLogInfo("Delete render in OsgMain\n");
    }
}

Camera * getCamera(void)
{
    return screens->getActiveView()->getCamera();
}

} // namespace osggraph
