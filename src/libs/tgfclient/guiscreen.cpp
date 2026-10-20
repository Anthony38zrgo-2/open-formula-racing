/***************************************************************************
                           screen.cpp -- screen init
                             -------------------
    created              : Fri Aug 13 22:29:56 CEST 1999
    copyright            : (C) 1999, 2004 by Eric Espie, Bernhard Wymann
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
    Screen management.
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @ingroup	screen
*/

#include <portability.h>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <sstream>
#include <algorithm>

#include <SDL.h>
#include <SDL_video.h>

#include "tgfclient.h"
#include "gui.h"

#include "glfeatures.h"

// The resizable properties.
static bool GfScrResizable = false;

// The resizable functions.
bool GfScrInitSDL2();
bool GfScrGetResizable();
void GfScrSetFullscreen(bool bFullScreen = true);
void gfScrShutdown();
void gfScrBaseOpenGLSetup();
void gfScrOpenGlFeatures();
bool GfScrCreateMenuWindow();
bool GfScrValidateWindowPosition(int X, int Y);
void GfScrInitialWindowedPosition();
void gfScrSaveWindowState();
bool gfScrAAOpenGLSetup();
void gfScrDisableResizable();
bool GfscrAllowMultiFullScreens();
SDL_Rect GetMultiFullScreenBounds();


// The screen properties.
static int GfScrWidth;
static int GfScrHeight;
static int GfViewWidth;
static int GfViewHeight;
static int GfScrCenX;
static int GfScrCenY;
static SDL_GLContext GLContext = NULL;

static int GfScrNumDisplays = 0;
static int GfScrStartDisplayId = 0;

// The screen surface.
static SDL_Surface *PScreenSurface = NULL;

/* Default list of screen color depths (bits per pixel, alpha included) in case
   something went wrong during hardware / driver capabilities detection */
static int ADefScreenColorDepths[] = { 16, 24, 32 };
static const int NDefScreenColorDepths =
    sizeof(ADefScreenColorDepths) / sizeof(ADefScreenColorDepths[0]);

/* Default list of screen sizes ("resolutions") in case
   something went wrong during hardware / driver capabilities detection */
static tScreenSize ADefScreenSizes[] =
{
    {  320,  200 },
    {  320,  240 },
    {  400,  300 },
    {  416,  312 },
    {  480,  360 },
    {  512,  384 },
    {  576,  384 },
    {  576,  432 },
    {  640,  384 },
    {  640,  400 },
    {  640,  480 },
    {  640,  512 },
    {  700,  525 },
    {  720,  450 },
    {  800,  512 },
    {  800,  600 },
    {  832,  624 },
    {  840,  525 },
    {  896,  672 },
    {  928,  696 },
    {  960,  600 },
    {  960,  720 },
    { 1024,  600 },
    { 1024,  768 },
    { 1152,  768 },
    { 1152,  864 },
    { 1280,  600 },
    { 1280,  720 },
    { 1280,  768 },
    { 1280,  800 },
    { 1280,  960 },
    { 1280, 1024 },
    { 1366,  768 },
    { 1400, 1050 },
    { 1440,  900 },
    { 1600,  900 },
    { 1600, 1024 },
    { 1680, 1050 },
    { 1792, 1344 },
    { 1800, 1440 },
    { 1920, 1080 },
    { 1920, 1200 },
    { 2560, 1080 },
    { 2560, 1440 },
    { 3840, 2160 }
};
static const int NDefScreenSizes =
    sizeof(ADefScreenSizes) / sizeof(ADefScreenSizes[0]);

/** Get a list of (common?) window sizes (pixels)
    @ingroup	screen
    @return	ScreenSizeVector of sizes
 */
ScreenSizeVector GfScrGetWindowSizes()
{
    ScreenSizeVector vecSizes;
    for(int i = 0; i < NDefScreenSizes; i++)
    {
        vecSizes.push_back(ADefScreenSizes[i]);
    }
    ScreenSizeVector custSizes = GfScrGetCustomWindowSizes();

    for (unsigned i = 0; i < custSizes.size(); i++)
    {
       vecSizes.push_back(custSizes[i]);
    }
    return vecSizes;
}

/** Get any custom screen / window sizes (pixels) from screen.xml
    @ingroup	screen
    @param	none
    @return	ScreenSizeVector of custom sizes
 */
ScreenSizeVector GfScrGetCustomWindowSizes()
{
   ScreenSizeVector vecSizes;

   void* hparmScreen =
      GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

   if (GfParmExistsSection(hparmScreen, GFSCR_SECT_WIN_MODES))
   {
      tScreenSize last;
      last.width = 0;
      last.height = 0;

      GfParmListSeekFirst(hparmScreen, GFSCR_SECT_WIN_MODES);
      do
      {
         last.width = GfParmGetCurNum(hparmScreen, GFSCR_SECT_WIN_MODES, GFSCR_ATT_WIN_X, NULL, 0);
         last.height = GfParmGetCurNum(hparmScreen, GFSCR_SECT_WIN_MODES, GFSCR_ATT_WIN_Y, NULL, 0);

         if ((last.height != 0) && (last.width != 0))
            vecSizes.push_back(last);
      }
      while (GfParmListSeekNext(hparmScreen, GFSCR_SECT_WIN_MODES) == 0);
   }

   GfParmReleaseHandle(hparmScreen);

   return vecSizes;
}

/** Get the supported screen / window sizes (pixels) for the current display mode.
    @ingroup	screen
    @param	nDisplayIndex	Index of the display from which to get the sizes
    @return	ScreenSizeVector of detected supported sizes
 */
ScreenSizeVector GfScrGetSupportedSizes(int nDisplayIndex)
{
    /* Build list of available screen sizes */
    int numModes;
    SDL_DisplayMode mode;
    SDL_Rect bounds;
    ScreenSizeVector vecSizes;
    tScreenSize last;
    last.width = 0;
    last.height = 0;
    last.refresh_rate = 0;

    bounds.w = 0;
    bounds.h = 0;

    // make sure nDisplayIndex is valid (less than Number of displays)
    if(nDisplayIndex < GfScrGetAttachedDisplays())
    {
        if(SDL_GetDesktopDisplayMode(nDisplayIndex, &mode) == 0)
        {
            bounds.w = mode.w;
            bounds.h = mode.h;
            GfLogInfo("Display %d : %d x %d x %d @ %d hz\n", nDisplayIndex + 1, mode.w,mode.h,SDL_BITSPERPIXEL(mode.format),mode.refresh_rate);
        }
        else
        {
            GfLogError("Could not get the Display mode for Display %d \n", nDisplayIndex + 1);
            bounds.w = 0;
            bounds.h = 0;
        }

        numModes = SDL_GetNumDisplayModes(nDisplayIndex);
        GfLogInfo("Display %d : modes available %d\n", nDisplayIndex + 1, numModes);

        for(int i = 0; i < numModes; i++)
        {
            if(SDL_GetDisplayMode(nDisplayIndex, i, &mode) == 0)
            {
                // Don't allow duplicate entries
                if((mode.w != last.width) || (mode.h != last.height))
                {
                    GfLogDebug("  %d x %d x %d @ %d hz\n",mode.w,mode.h,SDL_BITSPERPIXEL(mode.format),mode.refresh_rate);
                    last.width = mode.w;
                    last.height = mode.h;
                    last.refresh_rate = mode.refresh_rate;
                    vecSizes.push_back(last);
                }
            }
        }
        // Reverse the order for combobox GUI
        std::reverse(vecSizes.begin(), vecSizes.end());
    }
    else
    {
        GfLogError("Invalid Display index passed to GfScrGetSupportedSizes()\n");
    }

    if(vecSizes.empty())
    {
        GfLogInfo("No supported sizes for Display .\n");

        // Desperation stick the Display Bounds into the vector
        last.width = bounds.w;
        last.height = bounds.h;
        last.refresh_rate = mode.refresh_rate;
        vecSizes.push_back(last);
    }

    return vecSizes;
}

/** Get the screen dimensions
    @ingroup	screen
    @param	nDisplayIndex	Index of the display from which to get the size
    @return	tScreenSize containing the current size
 */
tScreenSize GfScrGetCurrentDisplaySize(int nDisplayIndex)
{
    tScreenSize size;

    size.width = 0;
    size.height = 0;
    size.refresh_rate = 0;

    SDL_DisplayMode mode;
    if(SDL_GetCurrentDisplayMode(nDisplayIndex, &mode) == 0)
    {
        size.width = mode.w;
        size.height = mode.h;
        size.refresh_rate = mode.refresh_rate;
    }
    return size;
}

/** Get the default / fallback screen / window color depths (bits per pixels, alpha included).
    @ingroup	screen
    @param	pnColorDepths	Address of number of default sizes (output).
    @return	Array of detected supported sizes (static data, never free).
 */
int* GfScrGetDefaultColorDepths(int* pnColorDepths)
{
    *pnColorDepths = NDefScreenColorDepths;

    return ADefScreenColorDepths;
}

/** Get the supported color depths as supported by the underlying hardware/driver.
    @ingroup	screen
    @param	pnDepths	Address of number of detected color depths (output)
    @return	Array of detected supported color depths (allocated on the heap, must use free at the end)
 */
int* GfScrGetSupportedColorDepths(int* pnDepths)
{
    // Need to completely re-write this function
    *pnDepths = NDefScreenColorDepths;
    return ADefScreenColorDepths;
}

static void gfScrReshapeViewport(int width, int height)
{
    GfViewWidth = GfScrWidth = width;
    GfViewHeight = GfScrHeight = height;
    GfScrCenX = width / 2;
    GfScrCenY = height / 2;

    glViewport((width-GfViewWidth)/2, (height-GfViewHeight)/2, GfViewWidth,  GfViewHeight);
    glMatrixMode(GL_PROJECTION );
    glLoadIdentity();
    glOrtho(0.0, 640.0, 0.0, 480.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void gfuiInitialWindowedPosition(int displayId, SDL_Window* window )
{
    int top = 0, left = 0, bottom = 0, right = 0, x = 0, y = 0;
    SDL_Rect rect;
    SDL_GetDisplayBounds(displayId, &rect);

    SDL_GetWindowPosition(window, &x, &y);

    SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right);

    if(y < rect.y + top)
        y = rect.y + top;
    if(x < rect.x)
        x = rect.x;

    SDL_SetWindowPosition(window, x, y);
}

SDL_Surface* gfScrCreateWindow(int nWinWidth, int nWinHeight, int nTotalDepth,int bfVideoMode)
{
    if(GfuiWindow)
    {
        SDL_DestroyWindow(GfuiWindow);
        GfuiWindow = NULL;
    }
    if(PScreenSurface)
    {
        SDL_FreeSurface(PScreenSurface);
        PScreenSurface = NULL;
    }
    // Set window/icon captions
    std::ostringstream ossCaption;
    ossCaption << GfuiApp().name() << ' ' << GfuiApp().version();

    GfuiWindow = SDL_CreateWindow(ossCaption.str().c_str(),
        SDL_WINDOWPOS_CENTERED_DISPLAY(GfScrStartDisplayId), SDL_WINDOWPOS_CENTERED_DISPLAY(GfScrStartDisplayId),
        nWinWidth, nWinHeight, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);


#if !defined(__APPLE__)
    // Set window icon (MUST be a 32x32 icon for Windows, and with black pixels as alpha ones,
    // as BMP doesn't support transparency).
    std::ostringstream ossIconFilename;
    ossIconFilename << GfDataDir() << "data/icons/icon.bmp";
    SDL_Surface* surfIcon = SDL_LoadBMP(ossIconFilename.str().c_str());
    if (surfIcon)
    {
        SDL_SetColorKey(surfIcon, SDL_TRUE, SDL_MapRGB(surfIcon->format, 0, 0, 0));
        SDL_SetWindowIcon(GfuiWindow, surfIcon);
        SDL_FreeSurface(surfIcon);
    }
#endif
    /* Create OpenGL context */
    if (!(GLContext = SDL_GL_CreateContext(GfuiWindow)))
    {
        GfLogError("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return nullptr;
    }

    // If specified, try best possible settings.
    PScreenSurface = SDL_CreateRGBSurface(0, nWinWidth, nWinHeight, nTotalDepth,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        0x00FF0000, 0x0000FF00, 0x000000FF,
#else
        0x000000FF, 0x0000FF00, 0x00FF0000,
#endif
        0x00000000);

    if (bfVideoMode & SDL_WINDOW_FULLSCREEN)
    {
        SDL_Rect bounds;

        /* Work around SDL2 bug */
        if (SDL_GetDisplayBounds(GfScrStartDisplayId, &bounds) == 0) {
            if (bounds.w == nWinWidth && bounds.h == nWinHeight)
                SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
            else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);
        } else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);
    }
    return PScreenSurface;
}

int GfScrGetAttachedDisplays()
{
    int nDisplays = SDL_GetNumVideoDisplays();
    return nDisplays;
}

bool GfScrInitSDL2(int nWinWidth, int nWinHeight, int nFullScreen)
{
    // Prepare video mode.
    int bfVideoMode = SDL_WINDOW_OPENGL;

    // Initialize SDL video subsystem (and exit if not supported).
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        GfLogError("Couldn't initialize SDL audio/video sub-system (%s)\n", SDL_GetError());
        return false;
    }

#ifdef __APPLE__
    // Version d'OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
    // Version d'OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif

#if ((SDL_MAJOR_VERSION >= 2) && (SDL_PATCHLEVEL >= 5))
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    GfScrNumDisplays = GfScrGetAttachedDisplays();

    // Get selected frame buffer specs from config file
    // 1) Load the config file
    void* hparmScreen =
        GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    // 2) Check / update test state of any 'in-test' specs.
    if (GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS))
    {
        // Remove the 'in-test' specs if the test failed (we are still in the 'in progress'
        // test state because the game crashed during the test).
        if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
                                     GFSCR_VAL_INPROGRESS)) == GFSCR_VAL_INPROGRESS)
        {
            GfLogInfo("Reverting to last validated screen specs, as last test failed.\n");
            GfParmRemoveSection(hparmScreen, GFSCR_SECT_INTESTPROPS);
        }

        // If the test has not yet been done, mark it as 'in progress'
        else
        {
            GfLogInfo("Testing new screen specs : let's see what's happening ...\n");
            GfParmSetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
                         GFSCR_VAL_INPROGRESS);
        }

        // Write the config file to disk (in case the forthcoming test makes the game crash,
        // or in order the Options / Display menu shows the actual current settings).
        GfParmWriteFile(NULL, hparmScreen, "Screen");
    }

    // 3) Select the 'in-test' specs if present, otherwise the 'validated' ones.
    const char* pszScrPropSec =
        GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS)
        ? GFSCR_SECT_INTESTPROPS : GFSCR_SECT_VALIDPROPS;

    // 4) Get/Read the specs.
    if (nWinWidth < 0)
        nWinWidth =
            (int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_X, (char*)NULL, 800);
    if (nWinHeight < 0)
        nWinHeight =
            (int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_Y, (char*)NULL, 600);
    int nTotalDepth =
        (int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_BPP, (char*)NULL, 32);
    GfScrStartDisplayId =
        (int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_STARTUPDISPLAY, (char*)NULL, 0);
    if(GfScrStartDisplayId >= GfScrNumDisplays)
    {
        GfScrStartDisplayId = 0;
    }

    double maxRefreshRate = GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_MAXREFRESH, NULL, 50);

    GfuiApp().eventLoop().setMaxRefreshRate(maxRefreshRate);

    bool bFullScreen;
    if (nFullScreen < 0)
        bFullScreen =
            std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_FSCR, GFSCR_VAL_NO))
            == GFSCR_VAL_YES;
    else
        bFullScreen = nFullScreen ? true : false;

    if(bFullScreen)
        bfVideoMode |= SDL_WINDOW_FULLSCREEN;

//* TODO : move and re-implement these?
    bool bAlphaChannel =
        std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_ALPHACHANNEL,
        GFSCR_VAL_YES))
        == GFSCR_VAL_YES;

    bool bBumpMap =
        std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_BUMPMAPPING,
                                 GFSCR_VAL_NO))
        == GFSCR_VAL_YES;

    int nAniFilt =
        (int)GfParmGetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_ANISOTROPICFILTERING, (char*)NULL, 0);

    bool bStereo =
        std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_STEREOVISION,
                                 GFSCR_VAL_NO))
        == GFSCR_VAL_YES;
    bool bTryBestVInitMode =
        std::string(GfParmGetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_VINIT,
                                 GFSCR_VAL_VINIT_BEST))
        == GFSCR_VAL_VINIT_BEST;


    // TODO ?
    // Add new values to the config OpenGL Major and Minor
    // and setup GL Major/Minor before window creation
    // SDL_GL_SetSwapInterval(1) for for vsync (may have to go AFTER window creation)

        if (bTryBestVInitMode)
    {
        GfLogInfo("Trying 'best possible mode' for video initialization.\n");

        // Detect best supported features for the specified frame buffer specs.
        // Warning: Restarts the game if the frame buffer specs changed since last call.
        // If specified and possible, setup the best possible settings.
        if (GfglFeatures::self().checkBestSupport(nWinWidth, nWinHeight, nTotalDepth,
                                                  bAlphaChannel, bFullScreen, bBumpMap, bStereo,nAniFilt,hparmScreen))
        {
            // Load Open GL user settings from the config file.
            GfglFeatures::self().loadSelection();

            // Setup the video mode parameters.
            const int nColorDepth =
                GfglFeatures::self().getSelected(GfglFeatures::ColorDepth);
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE, nColorDepth/3);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, nColorDepth/3);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, nColorDepth/3);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, nColorDepth);

            const int nAlphaDepth =
                GfglFeatures::self().getSelected(GfglFeatures::AlphaDepth);
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, nAlphaDepth);

            const int nDoubleBuffer =
                GfglFeatures::self().isSelected(GfglFeatures::DoubleBuffer) ? 1 : 0;
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, nDoubleBuffer);

            const int nMultiSampling =
                GfglFeatures::self().isSelected(GfglFeatures::MultiSampling) ? 1 : 0;
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, nMultiSampling);
            if (nMultiSampling)
            {
                const int nMaxMultiSamples =
                    GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples);
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, nMaxMultiSamples);
            }

            const int nStereoVision =
                GfglFeatures::self().isSelected(GfglFeatures::StereoVision) ? 1 : 0;
            SDL_GL_SetAttribute(SDL_GL_STEREO, nStereoVision);

            // Try the video mode with these parameters : should always work
            // (unless you downgraded you hardware / OS and didn't clear your config file).
            PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
        }

        // If best mode not supported, or test actually failed,
        // revert to a supported mode (restart the game).
        if (!PScreenSurface)
        {
            GfLogWarning("Failed to setup best supported video mode "
                         "whereas previously detected !\n");
            GfLogWarning("Tip: You should remove your %s%s file and restart,\n",
                         GfLocalDir(), GFSCR_CONF_FILE);
            GfLogWarning("     if something changed in your OS"
                         " or video hardware/driver configuration.\n");

            // If testing new screen specs, remember that the test failed
            // in order to revert to the previous validated specs on restart.
            if (std::string(pszScrPropSec) == GFSCR_SECT_INTESTPROPS)
            {
                GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_TESTSTATE,
                             GFSCR_VAL_FAILED);
            }

            // Force compatible video init. mode if not testing a new video mode.
            else
            {
                GfLogWarning("Falling back to a more compatible default mode ...\n");
                GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_VINIT,
                             GFSCR_VAL_VINIT_COMPATIBLE);
            }
            GfParmWriteFile(NULL, hparmScreen, "Screen");
            GfParmReleaseHandle(hparmScreen);

            // And restart the game.
            GfuiApp().restart(); // Never returns.
        }
    }

    // Video initialization with generic compatible settings.
    if (!PScreenSurface)
    {
        GfLogInfo("Trying 'default compatible' mode for video initialization.\n");

        // cancel StereoVision
        SDL_GL_SetAttribute(SDL_GL_STEREO, 0);

        //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

        PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
        if (!PScreenSurface)
            GfLogTrace("Can't get a %s%dx%dx%d compatible video mode\n",
                       bFullScreen ? "full-screen " : "", nWinWidth, nWinHeight, nTotalDepth);
    }

    // Failed : Try and remove the full-screen requirement if present ...
    if (!PScreenSurface && bFullScreen)
    {
        bfVideoMode &= ~SDL_WINDOW_FULLSCREEN;
        PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
        if (!PScreenSurface)
            GfLogTrace("Can't get a non-full-screen %dx%dx%d compatible video mode\n",
                       nWinWidth, nWinHeight, nTotalDepth);

        // Update screen specs.
        GfParmSetStr(hparmScreen, pszScrPropSec, GFSCR_ATT_FSCR, GFSCR_VAL_NO);
        GfParmWriteFile(NULL, hparmScreen, "Screen");
    }

    // Failed : Try with a lower fallback size  : should be supported everywhere ...
    if (!PScreenSurface)
    {
        nWinWidth = ADefScreenSizes[0].width;
        nWinHeight = ADefScreenSizes[0].height;
        PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
        if (!PScreenSurface)
            GfLogTrace("Can't get a %dx%dx%d compatible video mode\n",
                       nWinWidth, nWinHeight, nTotalDepth);

        // Update screen specs.
        GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_X, 0, (tdble)nWinWidth);
        GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_WIN_Y, 0, (tdble)nWinHeight);
        GfParmWriteFile(NULL, hparmScreen, "Screen");
    }

    // Failed : Try with a lower fallback color depth : should be supported everywhere ...
    if (!PScreenSurface)
    {
        nTotalDepth = ADefScreenColorDepths[0];
        PScreenSurface = gfScrCreateWindow(nWinWidth, nWinHeight, nTotalDepth,bfVideoMode);
        if (!PScreenSurface)
            GfLogTrace("Can't get a %dx%dx%d compatible video mode\n",
                       nWinWidth, nWinHeight, nTotalDepth);

        // Update screen specs.
        GfParmSetNum(hparmScreen, pszScrPropSec, GFSCR_ATT_BPP, 0, (tdble)nTotalDepth);
        GfParmWriteFile(NULL, hparmScreen, "Screen");
    }

    // Close the config file.
    GfParmReleaseHandle(hparmScreen);

    // Failed : No way ... no more ideas !
    if (!PScreenSurface)
    {
        GfLogError("Unable to get any compatible video mode"
                   " (fallback resolution / color depth not supported) : giving up !\n\n");
        return false;
    }

    // If we get here, that's because we succeeded in getting a valid video mode :-)

    // If 'compatible mode' selected, detect only standard Open GL features
    // and load OpenGL settings from the config file.
    if (!bTryBestVInitMode)
    {
        GfglFeatures::self().detectStandardSupport();
        GfglFeatures::self().dumpSupport();
        GfglFeatures::self().loadSelection();
        if (GfglFeatures::self().isSupported(GfglFeatures::MultiSampling))
        {
            bool MultiSamplingWasSelected =
                GfglFeatures::self().isSelected(GfglFeatures::MultiSampling);
            int MultiSamplingSamples =
                GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples);
            if(MultiSamplingWasSelected)
            {
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MultiSamplingSamples);
            }
            else
            {
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
            }
        }
    }

    // Save view geometry and screen center.
    GfViewWidth = nWinWidth;
    GfViewHeight = nWinHeight;
    GfScrCenX = nWinWidth / 2;
    GfScrCenY = nWinHeight / 2;

    // Report about selected SDL video mode.
    GfLogInfo("Selected SDL video mode :\n");
    GfLogInfo("  Full screen : %s\n", (bfVideoMode & SDL_WINDOW_FULLSCREEN) ? "Yes" : "No");
    GfLogInfo("  Size        : %dx%d\n", nWinWidth, nWinHeight);
    GfLogInfo("  Color depth : %d bits\n", nTotalDepth);
    GfLogInfo("  Max. refresh rate : %f\n", maxRefreshRate);

    // Report about underlying hardware (needs a running frame buffer).
    GfglFeatures::self().dumpHardwareInfo();

    if(GfuiWindow)
    {
        SDL_SetWindowPosition(GfuiWindow, SDL_WINDOWPOS_CENTERED_DISPLAY(GfScrStartDisplayId), SDL_WINDOWPOS_CENTERED_DISPLAY(GfScrStartDisplayId));
        SDL_ShowWindow(GfuiWindow);
        SDL_RestoreWindow(GfuiWindow);
    }

    // Position Window if not full screen
    if (!(bfVideoMode & SDL_WINDOW_FULLSCREEN))
    {
        gfuiInitialWindowedPosition(GfScrStartDisplayId, GfuiWindow);
    }


    // Initialize the Open GL viewport.
    gfScrReshapeViewport(nWinWidth, nWinHeight);

    // Setup the event loop about the new display.
    GfuiApp().eventLoop().setReshapeCB(gfScrReshapeViewport);
    GfuiApp().eventLoop().postRedisplay();
    return true;
}
bool GfScrInit(int nWinWidth, int nWinHeight, int nFullScreen)
{
    GfScrResizable = GfScrGetResizable();
    if (GfScrResizable)
        return GfScrInitSDL2();
    else
        return GfScrInitSDL2(nWinWidth,nWinHeight,nFullScreen);
}

/** Shutdown the screen
    @ingroup	screen
    @return	none
*/
void GfScrShutdown(void)
{
    if (GfScrResizable)
    {
        return gfScrShutdown();
    }

    GfLogTrace("Shutting down screen.\n");

    SDL_GL_MakeCurrent(GfuiWindow,GLContext);
    SDL_GL_DeleteContext(GLContext);
    GLContext = NULL;
    SDL_DestroyWindow(GfuiWindow);
    GfuiWindow = NULL;
    if(PScreenSurface)
    {
        SDL_FreeSurface(PScreenSurface);
        PScreenSurface = NULL;
    }

    // Shutdown SDL video sub-system.
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // If there's an 'in-test' screen properties section in the config file,
    // * if the test state is 'to do', do nothing (will be taken care of in next GfScrInit),
    // * if the test state is 'in progress', validate the new screen properties,
    // * if the test state is 'failed', revert to the validated screen properties.
    void* hparmScreen = GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD);

    if (GfParmExistsSection(hparmScreen, GFSCR_SECT_INTESTPROPS))
    {
        if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
                                     GFSCR_VAL_INPROGRESS)) == GFSCR_VAL_INPROGRESS)
        {
            GfLogInfo("Validating new screen specs (test was successful).\n");

            // Copy the 'in test' props to the 'validated' ones.
            GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_WIN_X, 0,
                         GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_X, 0, 800));
            GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_WIN_Y, 0,
                         GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_Y, 0, 600));
            GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_BPP, 0,
                         GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_BPP, 0, 32));
            GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_STARTUPDISPLAY, 0,
                         GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_STARTUPDISPLAY, 0, 0));
            GfParmSetStr(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_VDETECT,
                         GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_VDETECT, GFSCR_VAL_VDETECT_AUTO));
            GfParmSetNum(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_MAXREFRESH, 0,
                         GfParmGetNum(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_MAXREFRESH, 0, 50));
            const  char* pszVInitMode =
                GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_VINIT,
                             GFSCR_VAL_VINIT_COMPATIBLE);
            GfParmSetStr(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_VINIT, pszVInitMode);
            GfParmSetStr(hparmScreen, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_FSCR,
                         GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_FSCR, GFSCR_VAL_NO));
            // Store OpenGL settings if best video init mode selected
            // (because loadSelection can changed them).
            if (std::string(pszVInitMode) == GFSCR_VAL_VINIT_BEST)
                GfglFeatures::self().storeSelection(hparmScreen);
        }
        else if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
                                          GFSCR_VAL_INPROGRESS)) == GFSCR_VAL_FAILED)
        {
            GfLogInfo("Canceling new screen specs, back to old ones (test failed).\n");
        }


        if (std::string(GfParmGetStr(hparmScreen, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE,
                                     GFSCR_VAL_INPROGRESS)) != GFSCR_VAL_TODO)
        {
            // Remove the 'in-test' section.
            GfParmRemoveSection(hparmScreen, GFSCR_SECT_INTESTPROPS);

            // Write the screen config file to disk.
            GfParmWriteFile(NULL, hparmScreen, "Screen");
        }
        else
        {
            GfLogInfo("New screen specs will be tested when restarting.\n");
        }
    }

    // Release screen config params file.
    GfParmReleaseHandle(hparmScreen);
}


/** Get the screen and viewport sizes.
    @ingroup	screen
    @param	scrw	address of screen with
    @param	scrh	address of screen height
    @param	vieww	address of viewport with
    @param	viewh	address of viewport height
    @return	none
 */
void GfScrGetSize(int *scrW, int *scrH, int *viewW, int *viewH)
{
    *scrW = GfScrWidth;
    *scrH = GfScrHeight;
    *viewW = GfViewWidth;
    *viewH = GfViewHeight;
}

bool GfScrToggleFullScreen()
{
    Uint32 flags = SDL_GetWindowFlags(GfuiWindow);

    if ((flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        SDL_SetWindowFullscreen(GfuiWindow, 0);
        return false;
    } else {
        SDL_Rect bounds;

        /* Work around SDL2 bug */
        if (SDL_GetDisplayBounds(GfScrStartDisplayId, &bounds) == 0) {
            if (bounds.w == GfScrWidth && bounds.h == GfScrHeight)
                SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
            else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);
        } else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);

        return true;
    }
}

/** Capture screen pixels into an RGB buffer (caller must free the here-allocated buffer).
    @ingroup	screen
    @param	scrw	address of screen with
    @param	scrh	address of screen height
    @return	none
 */
unsigned char* GfScrCapture(int* viewW, int *viewH)
{
    unsigned char *img;
    int sW, sH;

    GfScrGetSize(&sW, &sH, viewW, viewH);
    img = (unsigned char*)malloc((*viewW) * (*viewH) * 3);
    if (img)
    {
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_FRONT);
        glReadPixels((sW-(*viewW))/2, (sH-(*viewH))/2, *viewW, *viewH,
                     GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)img);
    }

    return img;
}

/** Capture screen pixels into a PNG file
    @ingroup	screen
    @param	filename	filename of the png file
    @return	0 Ok
        <br>-1 Error
 */
int GfScrCaptureAsPNG(const char *filename)
{
    int viewW, viewH;

    // Capture screen to an RGB image (in memory) (and measure elapsed time).
    const double dCaptureBeginTime = GfTimeClock();

    unsigned char* img = GfScrCapture(&viewW, &viewH);

    const double dCaptureEndTime = GfTimeClock();

    const double dCaptureDuration = dCaptureEndTime - dCaptureBeginTime;

    // Write RGB image to the PNG file (and measure elapsed time).
    const int nStatus = GfTexWriteImageToPNG(img, filename, viewW, viewH);

    const double dFileWriteDuration = GfTimeClock() - dCaptureEndTime;

    // Free the image buffer.
    if (img)
        free(img);

    if (!nStatus)
        GfLogTrace("Captured screen to %s (capture=%.3f s, PNG=%.3f s)\n",
                   filename, dCaptureDuration, dFileWriteDuration);
    else
        GfLogError("Failed to capture screen to %s\n", filename);

    return nStatus;
}

SDL_Window* GfScrGetMainWindow()
{
    return GfuiWindow;
}

// ===========
bool GfScrUsingResizableWindow()
{
    return GfScrResizable;
}

bool GfScrGetResizable()
{
     GfScrResizable = false;
    // Read from screen.xml
    void* hparmScreen = GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    const char *resizable;
    resizable = GfParmGetStr(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_RESIZABLE, GFSCR_VAL_NO);
    if (strcmp(resizable, GFSCR_VAL_YES) == 0)
    {
        GfScrResizable = true;
    }
    // Close the config file.
    GfParmReleaseHandle(hparmScreen);

    return GfScrResizable;
}

bool gfScrAAOpenGLSetup()
{
    bool bSupported = false;
    // TODO read this from config
    int samplesWanted = 8;

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);

    SDL_Window* testWindow = SDL_CreateWindow("AA test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if(testWindow)
    {
        SDL_GLContext context = 0;
        context = SDL_GL_CreateContext(testWindow);
        if(context)
        {
            int buffers = -1;
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
            if(buffers)
            {
                int samples = -1;
                glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
                if(samples > 0)
                {
                    bSupported = true;
                    if(samples > samplesWanted)
                        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samplesWanted);
                    else
                        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
                }
            }
            SDL_GL_DeleteContext(context);
            context = NULL;
        }
        else
        {
            GfLogTrace("Unable to create an OpenGL AA test context: SDL Error: %s\n", SDL_GetError());
        }
        SDL_DestroyWindow(testWindow);
        testWindow = NULL;
    }
    else
    {
        GfLogTrace("Unable to create an OpenGL AA test window: SDL Error: %s\n", SDL_GetError());
    }

    if(bSupported == false)
    {
        GfLogTrace("Disabling Anti-aliasing\n");
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }


    return bSupported;
}

void gfScrBaseOpenGLSetup()
{

    // Will need for multi-window support
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // Setup Anti-aliasing (if available)
    gfScrAAOpenGLSetup();

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    //SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

#if ((SDL_MAJOR_VERSION >= 2) && (SDL_PATCHLEVEL >= 5))
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
}

bool GfScrCreateMenuWindow()
{
    SDL_Rect bounds;
    SDL_GetDisplayBounds(GfScrStartDisplayId, &bounds);

    float percentage = 0.90;

    // Set window/icon captions
    std::ostringstream ossCaption;
    ossCaption << GfuiApp().name() << " " << GfuiApp().version();

    GfuiWindow = SDL_CreateWindow(ossCaption.str().c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        bounds.w * percentage,
        bounds.h * percentage,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!GfuiWindow)
    {
        GfLogError("Unable to create an OpenGL window: SDL Error: %s\n", SDL_GetError());
        return false;
    }

#if !defined(__APPLE__)
    // Set window icon (MUST be a 32x32 icon for Windows, and with black pixels as alpha ones,
    // as BMP doesn't support transparency).
    std::ostringstream ossIconFilename;
    ossIconFilename << GfDataDir() << "data/icons/icon.bmp";
    SDL_Surface* surfIcon = SDL_LoadBMP(ossIconFilename.str().c_str());
    if (surfIcon)
    {
        SDL_SetColorKey(surfIcon, SDL_TRUE, SDL_MapRGB(surfIcon->format, 0, 0, 0));
        SDL_SetWindowIcon(GfuiWindow, surfIcon);
        SDL_FreeSurface(surfIcon);
    }
#endif
    // Create OpenGL context
    GLContext = SDL_GL_CreateContext(GfuiWindow);

    int doublebuffer = -1;
    int shared = -1;
    int samples = -1;

    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &doublebuffer);
    SDL_GL_GetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, &shared);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);

    if ((!GLContext) || (!doublebuffer)) // || (!shared))
    {
        GfLogError("Unable to create an OpenGL context: SDL Error: %s\n", SDL_GetError());
        GfLogError("\t GLContext = %p\n",GLContext);
        GfLogError("\t doublebuffer = %d\n",doublebuffer);
        GfLogError("\t shared = %d\n",shared);
        GfLogError("\t samples = %d\n",samples);
        return false;
    }
    else
    {
        SDL_GL_MakeCurrent(GfuiWindow,GLContext);
    }
    return true;
}

void gfScrOpenGlFeatures()
{
    GfglFeatures::self().detectStandardSupport();
    GfglFeatures::self().dumpSupport();

    if (SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic"))
        GfglFeatures::self().select(GfglFeatures::AnisotropicFiltering, 2);
    else
        GfglFeatures::self().select(GfglFeatures::AnisotropicFiltering, GfglFeatures::InvalidInt);

    if (SDL_GL_ExtensionSupported("GL_ARB_multitexture"))
    {
        int nValue = 0;
        GfglFeatures::self().select(GfglFeatures::MultiTexturing, true);
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &nValue);
        GfglFeatures::self().select(GfglFeatures::MultiTexturingUnits, nValue);
    }
    else
    {
        GfglFeatures::self().select(GfglFeatures::MultiTexturing, false);
        GfglFeatures::self().select(GfglFeatures::MultiTexturingUnits, 1);
    }
    if(SDL_GL_ExtensionSupported("GL_ARB_texture_compression"))
    {
        int nValue = 0;
        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, &nValue);
        if(nValue)
            GfglFeatures::self().select(GfglFeatures::TextureCompression, true);
    }
    else
    {
        GfglFeatures::self().select(GfglFeatures::TextureCompression, false);
    }
    int maxTex = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    if (maxTex > 16384) // Max in-game supported value (must be consistent with openglconfig.cpp)
        maxTex = 16384;
    GfglFeatures::self().select(GfglFeatures::TextureMaxSize, maxTex);
}

bool GfScrValidateWindowPosition(int X, int Y)
{
    SDL_Rect rect;
    int numDisplays = SDL_GetNumVideoDisplays();

    for(int i = 0;i < numDisplays;i++)
    {
        if(SDL_GetDisplayBounds(i,&rect) == 0)
        {
            if(X >= rect.x && X < rect.x + rect.w && Y >= rect.y && Y < rect.y + rect.h)
                return true;
        }
    }

    return false;
}

void GfScrInitialWindowedPosition()
{
    int x = SDL_WINDOWPOS_UNDEFINED;
    int y = SDL_WINDOWPOS_UNDEFINED;
    int w = 800;
    int h = 600;
    int full = 0;
    int max = 0;

    void* hparmScreen =
        GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    if (GfParmExistsSection(hparmScreen, GFSCR_SECT_WINDOWPROPS))
    {

         x = (int)GfParmGetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_LEFT, (char*)NULL, x);
         y = (int)GfParmGetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_TOP, (char*)NULL, y);
         w = (int)GfParmGetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_X, (char*)NULL, w);
         h = (int)GfParmGetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_Y, (char*)NULL, h);
         full = (int)GfParmGetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_FULLSCREEN, (char*)NULL, full);
         max = (int)GfParmGetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_MAXIMIZED, (char*)NULL, max);
    }
    GfParmReleaseHandle(hparmScreen);

    // Make sure these are valid
    if(GfScrValidateWindowPosition(x, y) == false)
    {
        SDL_Rect rect;
        if(SDL_GetDisplayBounds(0,&rect) == 0)
        {
            w = rect.w * 0.9;
            h = rect.h * 0.9;
        }
        x = SDL_WINDOWPOS_UNDEFINED;
        y = SDL_WINDOWPOS_UNDEFINED;
    }

    SDL_SetWindowPosition(GfuiWindow, x, y);
    SDL_SetWindowSize(GfuiWindow, w, h);

    if(max)
    {
        SDL_MaximizeWindow(GfuiWindow);
    }

    if(full == 1)
    {
        SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else if(full == 2)
    {
        GfScrToggleMultiFullScreens(NULL);
    }
}

bool GfScrInitSDL2()
{
    GfLogTrace("Initializing resizable screen.\n");
    // Initialize SDL video subsystem (and exit if not supported).
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        GfLogError("Couldn't initialize SDL audio/video sub-system (%s)\n", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GfuiApp().name().c_str(),
            "SDL2 initialization failed.\nPlease verify that all prerequistes are installed.\n", NULL);
        return false;
    }

#ifdef __APPLE__
    // Version d'OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
    // Version d'OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif

    // Get system info

    // Read saved info

    // Setup OpenGL
    gfScrBaseOpenGLSetup();

    // Create Window
    if(GfScrCreateMenuWindow())
    {
        // Only needed for ssggraph
        gfScrOpenGlFeatures();

        GfScrInitialWindowedPosition();

        SDL_ShowWindow(GfuiWindow);

        // Initialize the Open GL viewport.
        SDL_GetWindowSize(GfuiWindow, &GfScrWidth, &GfScrHeight);
        gfScrReshapeViewport(GfScrWidth, GfScrHeight);

        // Setup the event loop about the new display.
        GfuiApp().eventLoop().setReshapeCB(gfScrReshapeViewport);
        GfuiApp().eventLoop().postRedisplay();

        double maxRefreshRate = 50;
        void* hparmScreen =
            GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

        if (hparmScreen)
        {
            maxRefreshRate = GfParmGetNum(hparmScreen,
                GFSCR_SECT_VALIDPROPS, GFSCR_ATT_MAXREFRESH, NULL, maxRefreshRate);
            GfParmReleaseHandle(hparmScreen);
        }

        GfuiApp().eventLoop().setMaxRefreshRate(maxRefreshRate);

        return true;
    }
    else
    {
        gfScrDisableResizable();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GfuiApp().name().c_str(),
            "Unable to create a resizable openGL window.\nThe Display Mode has been reset.", NULL);
        GfuiApp().restart();
    }
    return false;
}

void GfScrSetFullscreen(bool bFullScreen /* = true */)
{
    if(bFullScreen == false)
    {
        SDL_SetWindowFullscreen(GfuiWindow, 0);
    }
    else
    {
        SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void GfScrToggleFullScreen(void* unused)
{
    Uint32 flags = SDL_GetWindowFlags(GfuiWindow);

    if ((flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
    {
        GfScrSetFullscreen(false);
    }
    else
    {
        if (flags & SDL_WINDOW_BORDERLESS) // we are MultiFullScreen
        {
            GfScrToggleMultiFullScreens(NULL);
        }
        GfScrSetFullscreen(true);
    }
    return;
}

SDL_Rect GetMultiFullScreenBounds()
{
    SDL_Rect bounds;
    SDL_Rect maxBounds;
    int nDisplays = SDL_GetNumVideoDisplays();
    for(int i = 0;i < nDisplays;i++)
    {
        if(SDL_GetDisplayBounds(i, &bounds) == 0)
        {
            if(i == 0)
            {
                maxBounds = bounds;
            }
            else
            {
                if(bounds.x < maxBounds.x)
                    maxBounds.x = bounds.x;

                maxBounds.w += bounds.w;
            }
        }
    }
    return maxBounds;
}

bool GfscrAllowMultiFullScreens()
{
    bool bRet = false;
    SDL_Rect bounds;

    int nDisplays = SDL_GetNumVideoDisplays();
    if(nDisplays > 1)
    {
        int height = 0;
        int top = 0;
        for(int i = 0;i < nDisplays;i++)
        {
            if(SDL_GetDisplayBounds(i, &bounds) == 0)
            {
                if(i == 0)
                {
                    top = bounds.y;
                    height = bounds.h;
                    bRet = true;
                    continue;
                }
                if((bounds.h != height) || (bounds.y != top))
                {
                    bRet = false;
                    break;
                }
            }
            else
            {
                bRet = false;
                break;
            }
        }
    }

    return bRet;
}

void GfScrToggleMultiFullScreens(void* unused)
{
    static int restoreX = 0;
    static int restoreY = 0;
    static int restoreW = 800;
    static int restoreH = 600;
    Uint32 flags = SDL_GetWindowFlags(GfuiWindow);

    if (flags & SDL_WINDOW_BORDERLESS) // we are MultiFullScreen
    {
        SDL_SetWindowBordered(GfuiWindow, SDL_TRUE);
        SDL_SetWindowPosition(GfuiWindow, restoreX, restoreY);
        SDL_SetWindowSize(GfuiWindow, restoreW, restoreH);
    }
    else if(GfscrAllowMultiFullScreens()) // NOT in Full-multiscreen
    {
        if ((flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
        {
            GfScrSetFullscreen(false);
        }

        SDL_GetWindowPosition(GfuiWindow, &restoreX, &restoreY);
        SDL_GetWindowSize(GfuiWindow, &restoreW, &restoreH);

        SDL_SetWindowBordered(GfuiWindow, SDL_FALSE);
        SDL_Rect bounds = GetMultiFullScreenBounds();

        if(!SDL_RectEmpty(&bounds))
        {
            SDL_SetWindowPosition(GfuiWindow,bounds.x, bounds.y);
            SDL_SetWindowSize(GfuiWindow, bounds.w, bounds.h);
        }
        else
        {
            GfLogError("GetMultiFullScreenBounds() returned an empty rectangle.\n");
        }
    }
    return;
}

void gfScrSaveWindowState()
{
    GfLogTrace("Saving resizable window state.\n");

    // TODO this has some problems on Linux
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int full = 0;
    int max = 0;

    int dispIndex = SDL_GetWindowDisplayIndex(GfuiWindow);

    Uint32 flags = SDL_GetWindowFlags(GfuiWindow);
    if (flags & SDL_WINDOW_BORDERLESS) // we are MultiFullScreen
    {
        full = 2;
        GfScrToggleMultiFullScreens(NULL);
    }
    if ((flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
    {
        full = 1;
        SDL_SetWindowFullscreen(GfuiWindow, 0);
    }

    flags = SDL_GetWindowFlags(GfuiWindow);
    if (flags & SDL_WINDOW_MAXIMIZED)
    {
        max = 1;
        SDL_RestoreWindow(GfuiWindow);
    }
    else if (flags & SDL_WINDOW_MINIMIZED)
    {
        SDL_RestoreWindow(GfuiWindow);
    }
    SDL_GetWindowPosition(GfuiWindow, &x, &y);
    SDL_GetWindowSize(GfuiWindow, &w, &h);

    void* hparmScreen =
        GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    if (GfParmExistsSection(hparmScreen, GFSCR_SECT_WINDOWPROPS))
    {
        GfParmSetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_STARTUPDISPLAY, (char*)NULL, dispIndex);
        GfParmSetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_FULLSCREEN, (char*)NULL, full);
        GfParmSetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_MAXIMIZED, (char*)NULL, max);
        GfParmSetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_LEFT, (char*)NULL, x);
        GfParmSetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_TOP, (char*)NULL, y);
        GfParmSetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_X, (char*)NULL, w);
        GfParmSetNum(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_WIN_Y, (char*)NULL, h);
    }

    // Write and release screen config params file.
    GfParmWriteFile(NULL, hparmScreen, "Screen");
    GfParmReleaseHandle(hparmScreen);
}

void gfScrShutdown()
{
    GfLogTrace("Shutting down resizable screen.\n");

    SDL_GL_MakeCurrent(GfuiWindow,GLContext);

    // save the window state and position
    gfScrSaveWindowState();

    SDL_GL_DeleteContext(GLContext);
    GLContext = NULL;
    SDL_DestroyWindow(GfuiWindow);
    GfuiWindow = NULL;

    // Shutdown SDL video sub-system.
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

void gfScrDisableResizable()
{
    void* hparmScreen =
        GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    if(hparmScreen)
    {
        if (GfParmExistsSection(hparmScreen, GFSCR_SECT_WINDOWPROPS))
        {
            GfParmSetStr(hparmScreen, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_RESIZABLE, GFSCR_VAL_NO);
        }

        // Write and release screen config params file.
        GfParmWriteFile(NULL, hparmScreen, "Screen");
        GfParmReleaseHandle(hparmScreen);
    }
}
