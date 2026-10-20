/***************************************************************************

    file        : OsgScreens.cpp
    created     : Sun Jan 13 22:11:03 CEST 2013
    copyright   : (C) 2013 by Xavier Bertaux
    email       : bertauxx@yahoo.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tgfclient.h>

#include <stdexcept>

#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/ViewerEventHandlers>
#include <osg/GraphicsContext>
#include <osg/ValueObject>
#include <osg/FrontFace>
#include <osgGA/EventQueue>

#include "OsgScreens.h"
#if SDL_MAJOR_VERSION >= 2
//#include "OsgGraphicsWindow.h"
#endif

#include "OsgDebugHUD.h"
#include "OsgReflectionMapping.h"
#include "OsgMain.h"
#include "OsgCar.h"
#include "OsgHUD.h"
#include <raceman.h>        //tSituation

namespace osggraph {

SDScreens::SDScreens() :
    root(NULL),
    prerenderRoot(NULL),
    m_gw(NULL),
    m_NbActiveScreens(0),
    m_NbArrangeScreens(0),
    m_SpanSplit(false),
    m_CurrentScreenIndex(0)
{
     debugHUD = new SDDebugHUD();
}

extern SDHUD hud;

class CameraDrawnCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator()(const osg::Camera& cam) const
    {
        getCars()->updateShadingParameters(cam.getViewMatrix());
    }
};

void SDScreens::Init(int x,int y, int width, int height, osg::ref_ptr<osg::Node> m_sceneroot, osg::Vec3f fogcolor)
{
    //intialising main screen

    viewer = new osgViewer::Viewer;
    osgViewer::StatsHandler *statsHandler = new osgViewer::StatsHandler();
    statsHandler->setKeyEventTogglesOnScreenStats('?');
    viewer->addEventHandler(statsHandler);
    viewer->setLightingMode( osg::View::NO_LIGHT );
    viewer->setThreadingModel( osgViewer::Viewer::CullThreadPerCameraDrawThreadPerContext );
#if 1 //SDL_MAJOR_VERSION < 2
    m_gw = viewer->setUpViewerAsEmbeddedInWindow(0, 0, width, height);
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw = m_gw;
    osg::ref_ptr<osg::Camera> Camera = viewer->getCamera();
    //Camera->setGraphicsContext(gw);
    //Camera->setViewport(new osg::Viewport(0, 0, width, height));
    Camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    Camera->setPreDrawCallback(new CameraDrawnCallback);
    Camera->setClearColor(osg::Vec4f(0.3f,0.3f,0.4f,1.0f));
#else
    SDL_Window* GfuiWindow = GfScrGetMainWindow();
    //viewer->setThreadingModel(osgViewer::Viewer::CullThreadPerCameraDrawThreadPerContext);
    //viewer->setThreadingModel(osgViewer::Viewer::CullDrawThreadPerContext);
    //viewer->setThreadingModel(osgViewer::Viewer::DrawThreadPerContext);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    SDL_GetWindowPosition(GfuiWindow, &traits->x, &traits->y);
    SDL_GetWindowSize(GfuiWindow, &traits->width, &traits->height);
    traits->windowName = SDL_GetWindowTitle(GfuiWindow);
    traits->windowDecoration = !(SDL_GetWindowFlags(GfuiWindow)&SDL_WINDOW_BORDERLESS);
    traits->screenNum = SDL_GetWindowDisplayIndex(GfuiWindow);
    traits->red = 8;
    traits->green = 8;
    traits->blue = 8;
    traits->alpha = 0; // set to 0 to stop ScreenCaptureHandler reading the alpha channel
    traits->depth = 24;
    traits->stencil = 8;
    traits->vsync = true;
    traits->doubleBuffer = true;
    traits->inheritedWindowData = new OSGUtil::OsgGraphicsWindowSDL2::WindowData(GfuiWindow);

    osg::ref_ptr<OSGUtil::OsgGraphicsWindowSDL2> gw = new OSGUtil::OsgGraphicsWindowSDL2(traits.get());
    viewer->getCamera()->setGraphicsContext(gw);
    viewer->getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
    viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    viewer->getCamera()->setPreDrawCallback(new CameraDrawnCallback);

    if(!gw->valid())
        throw std::runtime_error("Failed to create GraphicsContext");
#endif

    osg::ref_ptr<osg::Camera> mirrorCam = new osg::Camera;
    mirrorCam->setGraphicsContext(gw);
    mirrorCam->setClearColor(osg::Vec4f(0.3f,0.3f,0.4f,1.0f));
    mirrorCam->setClearMask( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    mirrorCam->setReferenceFrame( osg::Camera::ABSOLUTE_RF );

    SDView * view = new SDView((int)Screens.size(), viewer->getCamera(),0,0, width, height, mirrorCam.get());
    Screens.push_back(view);

    /* Set the scene graph root node for traversal by the viewer */
    root = new osg::Group;
    viewer->setSceneData(root.get());
#if 1
    prerenderRoot = new osg::Group;
    root->addChild(prerenderRoot);
    //root->addChild(reflectionMapping->getCamerasRoot());
#endif
    root->addChild(m_sceneroot.get());
    root->addChild(mirrorCam);
    mirrorCam->addChild(m_sceneroot.get());

    root->getOrCreateStateSet()->setMode( GL_CULL_FACE, osg::StateAttribute::ON );

    //create the hud and its own camera and add it to the viewer
    hud.CreateHUD(height, width);
    hud.camera->setGraphicsContext(gw);
    hud.camera->setViewport(0,0, width, height);
    viewer->addSlave(hud.camera, false);


    //viewer->setSceneData(root.get());
    viewer->realize();
}

void SDScreens::InitCars(tSituation *s)
{
    const char *pszSpanSplit;
    int grNbSuggestedScreens = 0;

    for (int i = 0; i < s->_ncars; i++)
    {
        tCarElt *elt = s->cars[i];

        //  Pre-assign each human driver (if any) to a different screen
        // (set him as the "current driver" for this screen).
        if (grNbSuggestedScreens < SD_NB_MAX_SCREEN
                && elt->_driverType == RM_DRV_HUMAN && !elt->_networkPlayer)
        {
            Screens[0]->setCurrentCar(elt);
            GfLogTrace("Screen #%d : Assigned to %s\n", 0, elt->_name);
            grNbSuggestedScreens++;
        }
    }

    /* Check whether view should be spanned across vertical splits */
    pszSpanSplit = GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);
    m_SpanSplit = strcmp(pszSpanSplit, GR_VAL_YES) ? 0 : 1;

    if (m_SpanSplit == 0 && grNbSuggestedScreens > 1)
    {
        // Mulitplayer, so ignore the stored number of screens
        m_NbActiveScreens = grNbSuggestedScreens;
        m_NbArrangeScreens = 0;
    } else
    {
        // Load the real number of active screens and the arrangment.
        m_NbActiveScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_NB_SCREENS, NULL, 1.0);
        m_NbArrangeScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_ARR_SCREENS, NULL, 0.0);
    }

    // Initialize the cameras for all the screens.
    for (unsigned i=0; i<Screens.size();i++)
    {
        Screens[i]->Init(s);
    }
}

void SDScreens::update(tSituation * s, SDFrameInfo* fi,osg::Vec4f(colorfog))
{
    if(GfScrUsingResizableWindow())
    {
        int width = 0;
        int height = 0;
        int grWinx = 0;
        int grWiny = 0;
        GfScrGetSize(&grWinx, &grWiny, &width, &height);
        m_gw->resized(grWinx,grWiny,width,height);
        m_gw->setClearColor(colorfog);
    }
    for (unsigned i=0; i< Screens.size(); i++)
    {
        Screens[i]->update(s, fi);
    }

    SDCars * cars = getCars();
    tCarElt * c = this->getActiveView()->getCurrentCar();

    this->debugHUD->setTexture(cars->getCar(c)->getReflectionMap()->getReflectionMap());

    if (!viewer->done())
        viewer->frame();
}

void SDScreens::changeCamera(long p)
{
    this->getActiveView()->getCameras()->nextCamera(p);

    // For SpanSplit ensure screens change together
    if (m_SpanSplit && getActiveView()->getViewOffset() )
    {
        int camList,camNum;

        getActiveView()->getCameras()->getIntSelectedListAndCamera(&camList,&camNum);

        for (int i=0; i < m_NbActiveScreens; i++)
            if (Screens[i]->getViewOffset() )
                Screens[i]->getCameras()->selectCamera(camList,camNum);

    }
}

int prevCamList=0;
int prevCamNum=0;
bool usingRearCam = false;
void SDScreens::changeCameraTemporaryOn()
{
	//if we are already on rear cam do nothing
	if(usingRearCam) return;

	int newCamList = 0;
	int newCamNum = 0;
	GfLogInfo("Switching camera\n");

	//remember the currently selected cameras
	int camList,camNum;
	getActiveView()->getCameras()->getIntSelectedListAndCamera(&camList,&camNum);
	GfLogInfo("Previous cam was %i %i \n",camList,camNum);

	//temporary switch to this the rear view one
	newCamList = 0; //(see osgcamera.cpp for reference)
	newCamNum = 5;
	prevCamList = camList;
	prevCamNum = camNum;

    this->getActiveView()->getCameras()->selectCamera(newCamList,newCamNum);
    usingRearCam = true;
}

void SDScreens::changeCameraTemporaryOff()
{
	//if we are already not on rear cam do nothing
	if(!usingRearCam) return;
    this->getActiveView()->getCameras()->selectCamera(prevCamList,prevCamNum);
    usingRearCam = false;
}


void SDScreens::toggleHUD()
{
    hud.ToggleHUD();
}

void SDScreens::toggleHUDwidget(const std::string &widget)
{
    hud.ToggleHUDwidget(widget);
}

void SDScreens::toggleHUDwidgets(const std::string & widgets)
{
    hud.ToggleHUDwidgets(widgets);
}

void SDScreens::toggleHUDdriverinput()
{
    hud.ToggleHUDdriverinput();
}

void SDScreens::toggleHUDeditmode()
{
    hud.ToggleHUDeditmode();
}

void SDScreens::toggleStats()
{
    viewer->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KeySymbol('?'));
}

void SDScreens::registerViewDependantPreRenderNode(osg::ref_ptr<osg::Node> node)
{
    //TODO : multi-screen support of this feature
    prerenderRoot->addChild(node);
}


SDScreens::~SDScreens()
{
    root->removeChildren(0, root->getNumChildren());
    root = NULL;

    for (unsigned i=0;i< Screens.size();i++)
    {
        delete Screens[i];
    }

    delete debugHUD;
}

} // namespace osggraph
