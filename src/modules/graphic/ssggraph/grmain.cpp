/***************************************************************************

    file                 : grmain.cpp
    created              : Thu Aug 17 23:23:49 CEST 2000
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

#include <portability.h>

#ifdef _WIN32
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <plib/ssg.h>
#include <glfeatures.h> // GfglFeatures
#include <robot.h>	//ROB_SECT_ARBITRARY
#include <graphic.h>

#include "grmain.h"
#include "grshadow.h"
#include "grskidmarks.h"
#include "grsmoke.h"
#include "grcar.h"
#include "grscreen.h"
#include "grscene.h"
#include "grloadac.h"
#include "grutil.h"
#include "grcarlight.h"
#include "grboard.h"
#include "grtracklight.h"
#include "grbackground.h"

 // TODO: Move this to glfeatures.
#ifdef _WIN32
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB = NULL;
PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = NULL;
#endif

namespace ssggraph {

int grMaxTextureUnits = 0;

tdble grMaxDammage = 10000.0;
int grNbCars = 0;

void *grHandle = 0;
void *grTrackHandle = 0;

int grWinx, grWiny, grWinw, grWinh;

tgrCarInfo *grCarInfo;
ssgContext grContext;
cGrScreen *grScreens[GR_NB_MAX_SCREEN];
tdble grLodFactorValue = 1.0;

// Frame/FPS info.
static cGrFrameInfo frameInfo;
static double fFPSPrevInstTime;   // Last "instant" FPS refresh time
static unsigned nFPSTotalSeconds; // Total duration since initView

// Mouse coords graphics backend to screen ratios.
static float fMouseRatioX, fMouseRatioY;

// Number of active screens.
int grNbActiveScreens = 1;
int grNbArrangeScreens = 0;
int grSpanSplit = 0;

// Current screen index.
static int nCurrentScreenIndex = 0;

static grssgLoaderOptions options(/*bDoMipMap*/true);


// Set up OpenGL features from user settings.
static void setupOpenGLFeatures(void)
{
    static bool bInitialized = false;

    // Don't do it twice.
    if (bInitialized)
        return;

    // Multi-texturing.
    grMaxTextureUnits = 1;
    if (GfglFeatures::self().isSelected(GfglFeatures::MultiTexturing))
    {
        // Use the selected number of texture units.
        grMaxTextureUnits = GfglFeatures::self().getSelected(GfglFeatures::MultiTexturingUnits);

#ifdef _WIN32
        // Retrieve the addresses of multi-texturing functions under Windows
        // They are not declared in gl.h or any other header ;
        // you can only get them through a call to wglGetProcAddress at run-time.
        glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
        glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
        glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)wglGetProcAddress("glClientActiveTextureARB");
        glMultiTexCoord2fvARB = (PFNGLMULTITEXCOORD2FVARBPROC)wglGetProcAddress("glMultiTexCoord2fvARB");
#endif
    }

    // Done once and for all.
    bInitialized = true;
}


static void
grAdaptScreenSize(void)
{
    int i;
    GfScrGetSize(&grWinx, &grWiny, &grWinw, &grWinh);
    grWinx = 0; grWiny = 0;
    switch (grNbActiveScreens)
    {
        default:
        case 1:
            // Hack to allow span-split to function OK
            if (grNbArrangeScreens > 1) grNbArrangeScreens = 0;

            // Always Full window.
            grScreens[0]->activate(grWinx, grWiny, grWinw, grWinh, 0.0);
            for (i=1; i < GR_NB_MAX_SCREEN; i++)
                grScreens[i]->deactivate();
            break;
        case 2:
            switch (grNbArrangeScreens) {
            default:
                grNbArrangeScreens = 0;
                SD_FALLTHROUGH // [[fallthrough]]
            case 0:
                // Top & Bottom half of the window
                grScreens[0]->activate(grWinx, grWiny + grWinh / 2, grWinw, grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx, grWiny,              grWinw, grWinh / 2, 0.0);
                break;
            case 1:
                // Left & Right half of the window
                grScreens[0]->activate(grWinx,              grWiny, grWinw / 2, grWinh, grSpanSplit * (-0.5 + 10));
                grScreens[1]->activate(grWinx + grWinw / 2, grWiny, grWinw / 2, grWinh, grSpanSplit * (0.5 + 10));
                break;
            case 2:
                // 33/66% Left/Right
                grScreens[0]->activate(grWinx,              grWiny, grWinw / 3,   grWinh, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 3, grWiny, grWinw * 2/3, grWinh, 0.0);
                break;
            case 3:
                // 66/33% Left/Right
                grScreens[0]->activate(grWinx,                grWiny, grWinw * 2/3, grWinh, 0.0);
                grScreens[1]->activate(grWinx + grWinw * 2/3, grWiny, grWinw / 3,   grWinh, 0.0);
                break;
            }

            for (i=2; i < GR_NB_MAX_SCREEN; i++)
                grScreens[i]->deactivate();
            break;
        case 3:
            switch (grNbArrangeScreens) {
            default:
                grNbArrangeScreens = 0;
                SD_FALLTHROUGH // [[fallthrough]]
            case 0:
                // Left/Right below wide
                grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw,     grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx,              grWiny,              grWinw / 2, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh / 2, 0.0);
                break;
            case 1:
                // All side by side
                grScreens[0]->activate(grWinx,                grWiny, grWinw / 3,   grWinh, grSpanSplit * (-1 + 10));
                grScreens[1]->activate(grWinx + grWinw / 3,   grWiny, grWinw / 3,   grWinh, grSpanSplit * (0.0 + 10));
                grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny, grWinw / 3,   grWinh, grSpanSplit * (1 + 10));
                break;
            case 2:
                // Left/Right above wide
                grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx,              grWiny,              grWinw,     grWinh / 2, 0.0);
                break;
            case 3:
                // 50/50% Left plus Top/Bottom on Right
                grScreens[0]->activate(grWinx,              grWiny,              grWinw / 2, grWinh, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh / 2, 0.0);
                break;
            case 5:
                // 50/50% Top/Bottom on Left plus Right
                grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh, 0.0);
                grScreens[2]->activate(grWinx,              grWiny,              grWinw / 2, grWinh / 2, 0.0);
                break;
            case 6:
                // 66/33% Left plus Top/Bottom on Right
                grScreens[0]->activate(grWinx,                grWiny,              grWinw * 2/3, grWinh, 0.0);
                grScreens[1]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 2, grWinw / 3,   grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3,   grWinh / 2, 0.0);
                break;
            case 7:
                // 33/66% Top/Bottom on Left plus Right
                grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 3,   grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 3, grWiny,              grWinw * 2/3, grWinh, 0.0);
                grScreens[2]->activate(grWinx,              grWiny,              grWinw / 3,   grWinh / 2, 0.0);
                break;
            }
            for (i=3; i < GR_NB_MAX_SCREEN; i++)
                grScreens[i]->deactivate();
            break;
        case 4:
            switch (grNbArrangeScreens) {
            case 8:
                // 3 side by side + 1 for rear view - only when spanning splits
                if (grSpanSplit) {
                    grScreens[0]->activate(grWinx,                grWiny, grWinw / 4,   grWinh, grSpanSplit * (-1 + 10));
                    grScreens[1]->activate(grWinx + grWinw / 4,   grWiny, grWinw / 4,   grWinh, grSpanSplit * (0.0 + 10));
                    grScreens[2]->activate(grWinx + grWinw * 2/4, grWiny, grWinw / 4,   grWinh, grSpanSplit * (1 + 10));
                    grScreens[3]->activate(grWinx + grWinw * 3/4, grWiny, grWinw / 4,   grWinh, 0.0);
                    break;
                }
                SD_FALLTHROUGH // [[fallthrough]]
            default:
                grNbArrangeScreens = 0;
                SD_FALLTHROUGH // [[fallthrough]]

            case 0:
                // Top/Bottom Left/Rigth Quarters
                grScreens[0]->activate(grWinx,              grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx,              grWiny,              grWinw / 2, grWinh / 2, 0.0);
                grScreens[3]->activate(grWinx + grWinw / 2, grWiny,              grWinw / 2, grWinh / 2, 0.0);
                break;
            case 1:
                // All side by side
                grScreens[0]->activate(grWinx,                grWiny, grWinw / 4,   grWinh, grSpanSplit * (-1.5 + 10));
                grScreens[1]->activate(grWinx + grWinw / 4,   grWiny, grWinw / 4,   grWinh, grSpanSplit * (-0.5 + 10));
                grScreens[2]->activate(grWinx + grWinw * 2/4, grWiny, grWinw / 4,   grWinh, grSpanSplit * (0.5 + 10));
                grScreens[3]->activate(grWinx + grWinw * 3/4, grWiny, grWinw / 4,   grWinh, grSpanSplit * (1.5 + 10));
                break;
            case 2:
                // Left/Middle/Right above wide
                grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 3,   grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
                grScreens[3]->activate(grWinx,                grWiny,              grWinw,     grWinh / 2, 0.0);
                break;
            case 3:
                // Left/Middle/Right below wide
                grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw,     grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx,                grWiny,              grWinw / 3, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx + grWinw / 3,   grWiny,              grWinw / 3, grWinh / 2, 0.0);
                grScreens[3]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3, grWinh / 2, 0.0);
                break;
            case 4:
                // 50/50% Left plus Top/Middle/Bottom on Right
                grScreens[0]->activate(grWinx,              grWiny,                grWinw / 2, grWinh, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 2, grWiny + grWinh * 2/3, grWinw / 2, grWinh / 3, 0.0);
                grScreens[2]->activate(grWinx + grWinw / 2, grWiny + grWinh / 3,   grWinw / 2, grWinh / 3, 0.0);
                grScreens[3]->activate(grWinx + grWinw / 2, grWiny,                grWinw / 2, grWinh / 3, 0.0);
                break;
            case 5:
                // 50/50% Top/Middle/Bottom on Left plus Right
                grScreens[0]->activate(grWinx,              grWiny + grWinh * 2/3, grWinw / 2, grWinh / 3, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 2, grWiny,                grWinw / 2, grWinh, 0.0);
                grScreens[2]->activate(grWinx,              grWiny + grWinh / 3  , grWinw / 2, grWinh / 3, 0.0);
                grScreens[3]->activate(grWinx,              grWiny,                grWinw / 2, grWinh / 3, 0.0);
                break;
            case 6:
                // 66/33% Left plus Top/Middle/Bottom on Right
                grScreens[0]->activate(grWinx,                grWiny,                grWinw * 2/3, grWinh, 0.0);
                grScreens[1]->activate(grWinx + grWinw * 2/3, grWiny + grWinh * 2/3, grWinw / 3,   grWinh / 3, 0.0);
                grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 3,   grWinw / 3,   grWinh / 3, 0.0);
                grScreens[3]->activate(grWinx + grWinw * 2/3, grWiny,                grWinw / 3,   grWinh / 3, 0.0);
                break;
            case 7:
                // 33/66% Top/Middle/Bottom on Left plus Right
                grScreens[0]->activate(grWinx,              grWiny + grWinh * 2/3, grWinw / 3,   grWinh / 3, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 3, grWiny,                grWinw * 2/3, grWinh, 0.0);
                grScreens[2]->activate(grWinx,              grWiny + grWinh / 3  , grWinw / 3,   grWinh / 3, 0.0);
                grScreens[3]->activate(grWinx,              grWiny,                grWinw / 3,   grWinh / 3, 0.0);
                break;
            }
            for (i=4; i < GR_NB_MAX_SCREEN; i++)
                grScreens[i]->deactivate();
            break;
        case 5:
            switch (grNbArrangeScreens) {
            default:
                grNbArrangeScreens = 0;
                SD_FALLTHROUGH // [[fallthrough]]
            case 0:
                // Top/Bottom Left/Middle/Rigth Matrix
                grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 2  , grWiny + grWinh / 2, grWinw / 2, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx,                grWiny,              grWinw / 3, grWinh / 2, 0.0);
                grScreens[3]->activate(grWinx + grWinw / 3,   grWiny,              grWinw / 3, grWinh / 2, 0.0);
                grScreens[4]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3, grWinh / 2, 0.0);
                break;
            case 1:
                // All side by side
                grScreens[0]->activate(grWinx,                grWiny, grWinw / 5,   grWinh, grSpanSplit * (-2.0 + 10));
                grScreens[1]->activate(grWinx + grWinw / 5,   grWiny, grWinw / 5,   grWinh, grSpanSplit * (-1.0 + 10));
                grScreens[2]->activate(grWinx + grWinw * 2/5, grWiny, grWinw / 5,   grWinh, grSpanSplit * (0.0 + 10));
                grScreens[3]->activate(grWinx + grWinw * 3/5, grWiny, grWinw / 5,   grWinh, grSpanSplit * (1.0 + 10));
                grScreens[4]->activate(grWinx + grWinw * 4/5, grWiny, grWinw / 5,   grWinh, grSpanSplit * (2.0 + 10));
                break;
            }
            for (i=5; i < GR_NB_MAX_SCREEN; i++)
                grScreens[i]->deactivate();
            break;
        case 6:
            switch (grNbArrangeScreens) {
            case 2:
                if (grSpanSplit) {
                    // five side by side + 1 for rear view - only when spanning splits
                    grScreens[0]->activate(grWinx,                grWiny, grWinw / 6,   grWinh, grSpanSplit * (-2.0 + 10));
                    grScreens[1]->activate(grWinx + grWinw / 6,   grWiny, grWinw / 6,   grWinh, grSpanSplit * (-1.0 + 10));
                    grScreens[2]->activate(grWinx + grWinw * 2/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (0.0 + 10));
                    grScreens[3]->activate(grWinx + grWinw * 3/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (1.0 + 10));
                    grScreens[4]->activate(grWinx + grWinw * 4/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (2.0 + 10));
                    grScreens[5]->activate(grWinx + grWinw * 5/6, grWiny, grWinw / 6,   grWinh, 0.0);
                    break;
                }
                SD_FALLTHROUGH // [[fallthrough]]
            default:
                grNbArrangeScreens = 0;
                SD_FALLTHROUGH // [[fallthrough]]
            case 0:
                // Top/Bottom Left/Middle/Rigth Matrix
                grScreens[0]->activate(grWinx,                grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
                grScreens[1]->activate(grWinx + grWinw / 3,   grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
                grScreens[2]->activate(grWinx + grWinw * 2/3, grWiny + grWinh / 2, grWinw / 3, grWinh / 2, 0.0);
                grScreens[3]->activate(grWinx,                grWiny,              grWinw / 3, grWinh / 2, 0.0);
                grScreens[4]->activate(grWinx + grWinw / 3,   grWiny,              grWinw / 3, grWinh / 2, 0.0);
                grScreens[5]->activate(grWinx + grWinw * 2/3, grWiny,              grWinw / 3, grWinh / 2, 0.0);
                break;
            case 1:
                // All side by side
                grScreens[0]->activate(grWinx,                grWiny, grWinw / 6,   grWinh, grSpanSplit * (-2.5 + 10));
                grScreens[1]->activate(grWinx + grWinw / 6,   grWiny, grWinw / 6,   grWinh, grSpanSplit * (-1.5 + 10));
                grScreens[2]->activate(grWinx + grWinw * 2/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (-0.5 + 10));
                grScreens[3]->activate(grWinx + grWinw * 3/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (0.5 + 10));
                grScreens[4]->activate(grWinx + grWinw * 4/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (1.5 + 10));
                grScreens[5]->activate(grWinx + grWinw * 5/6, grWiny, grWinw / 6,   grWinh, grSpanSplit * (2.5 + 10));
                break;
            }
            for (i=6; i < GR_NB_MAX_SCREEN; i++)
                grScreens[i]->deactivate();
            break;
        }
}

static void
grSplitScreenCmd(int p)
{
    // Change the number of active screens as specified.
    switch (p) {
        case GR_SPLIT_ADD:
            if (grNbActiveScreens < GR_NB_MAX_SCREEN)
                grNbActiveScreens++;
            if (grSpanSplit)
                grNbArrangeScreens=1;
            else
                grNbArrangeScreens=0;
            break;
        case GR_SPLIT_REM:
            if (grNbActiveScreens > 1)
                grNbActiveScreens--;
            if (grSpanSplit)
                grNbArrangeScreens=1;
            else
                grNbArrangeScreens=0;
            break;
        case GR_SPLIT_ARR:
            grNbArrangeScreens++;
    }

    // Ensure current screen index stays in the righ range.
    if (nCurrentScreenIndex >= grNbActiveScreens) {
        nCurrentScreenIndex = grNbActiveScreens - 1;
        GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);
    }

    // Save nb of active screens to user settings.
    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_NB_SCREENS, NULL, grNbActiveScreens);
    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_ARR_SCREENS, NULL, grNbArrangeScreens);
    GfParmWriteFile(NULL, grHandle, "Graph");
    grAdaptScreenSize();
}

static void
grSplitScreen(void *)
{
    grSplitScreenCmd(GR_SPLIT_ADD);
}

static void
grUnSplitScreen(void *)
{
    grSplitScreenCmd(GR_SPLIT_REM);
}

static void
grSplitScreenArr(void *)
{
    grSplitScreenCmd(GR_SPLIT_ARR);
}

static void
grChangeScreen(int p)
{
    switch (p) {
        case GR_NEXT_SCREEN:
            nCurrentScreenIndex = (nCurrentScreenIndex + 1) % grNbActiveScreens;
            break;
        case GR_PREV_SCREEN:
            nCurrentScreenIndex = (nCurrentScreenIndex - 1 + grNbActiveScreens) % grNbActiveScreens;
            break;
    }

    GfLogInfo("Changing current screen to #%d (out of %d)\n", nCurrentScreenIndex, grNbActiveScreens);

    GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);
    GfParmWriteFile(NULL, grHandle, "Graph");
}

static void
grNextScreen(void *vp)
{
    grChangeScreen(GR_NEXT_SCREEN);
}

cGrScreen *grGetCurrentScreen(void)
{
    return grScreens[nCurrentScreenIndex];
}

static void
grSetZoom(int zoom)
{
    grGetCurrentScreen()->setZoom(zoom);
}

static void
grSetMinZoom(void *)
{
    grSetZoom(GR_ZOOM_MIN);
}

static void
grSetMaxZoom(void *)
{
    grSetZoom(GR_ZOOM_MAX);
}

static void
grSetDefaultZoom(void *)
{
    grSetZoom(GR_ZOOM_DFLT);
}

static void
grZoomIn(void *)
{
    grSetZoom(GR_ZOOM_IN);
}

static void
grZoomOut(void *)
{
    grSetZoom(GR_ZOOM_OUT);
}

static void
grSelectCamera(int cam)
{
    grGetCurrentScreen()->selectCamera(cam);

    // For SpanSplit ensure screens change together
    if (grSpanSplit && grGetCurrentScreen()->getViewOffset() ) {
        int i, subcam;

    subcam = grGetCurrentScreen()->getNthCamera();

        for (i=0; i < grNbActiveScreens; i++)
            if (grScreens[i]->getViewOffset() )
                    grScreens[i]->selectNthCamera(cam, subcam);
    }
}

static void
gr1stPersonView(void *)
{
    grSelectCamera(0);
}

static void
gr3rdPersonView(void *)
{
    grSelectCamera(1);
}

static void
grSideCarView(void *)
{
    grSelectCamera(2);
}

static void
grUpCarView(void *)
{
    grSelectCamera(3);
}

static void
grPerspCarView(void *)
{
    grSelectCamera(4);
}

static void
grAllCircuitView(void *)
{
    grSelectCamera(5);
}

static void
grActionCamView(void *)
{
    grSelectCamera(6);
}

static void
grTVCamView(void *)
{
    grSelectCamera(7);
}

static void
grHelicopterView(void *)
{
    grSelectCamera(8);
}

static void
grTVDirectorView(void *)
{
    grSelectCamera(9);
}

cGrCamera * grGetCurCamera()
{
    return grGetCurrentScreen()->getCurCamera();
}

static void
grSelectBoard(int board)
{
    grGetCurrentScreen()->selectBoard(board);
}

static void
grDashboard(void *)
{
    grSelectBoard(6);
}

static void
grDebugInfo(void *)
{
    grSelectBoard(3);
}

static void
grGCmdGraph(void *)
{
    grSelectBoard(4);
}

static void
grLeadersBoard(void *)
{
    grSelectBoard(2);
}

static void
grDriverCounters(void *)
{
    grSelectBoard(1);
}

static void
grDriverBoard(void *)
{
    grSelectBoard(0);
}

static void
grDeltaBestLap(void *)
{
    grSelectBoard(7);
}

static void
grArcadeBoard(void *)
{
    grSelectBoard(5);
}

static void
grSelectTrackMap(void * /* vp */)
{
    grGetCurrentScreen()->selectTrackMap();
}

static void
grPrevCar(void * /* dummy */)
{
    // For SpanSplit ensure screens change together
    if (grSpanSplit && grGetCurrentScreen()->getViewOffset() ) {
        int i;
        tCarElt *car = grGetCurrentScreen()->getCurrentCar();

        for (i=0; i < grNbActiveScreens; i++)
            if (grScreens[i]->getViewOffset() ) {
                grScreens[i]->setCurrentCar(car);
                grScreens[i]->selectPrevCar();
            }
    } else
        grGetCurrentScreen()->selectPrevCar();
}

static void
grNextCar(void * /* dummy */)
{
    // For SpanSplit ensure screens change together
    if (grSpanSplit && grGetCurrentScreen()->getViewOffset() ) {
        int i;
        tCarElt *car = grGetCurrentScreen()->getCurrentCar();

        for (i=0; i < grNbActiveScreens; i++)
            if (grScreens[i]->getViewOffset() ) {
                grScreens[i]->setCurrentCar(car);
                grScreens[i]->selectNextCar();
            }
    } else
        grGetCurrentScreen()->selectNextCar();
}

static void
grSwitchMirror(void * /* dummy */)
{
    grGetCurrentScreen()->switchMirror();
}

int
initView(int x, int y, int width, int height, int /* flag */, void *screen)
{
    int i;

    grWinx = x;
    grWiny = y;
    grWinw = width;
    grWinh = height;

    fMouseRatioX = width / 640.0;
    fMouseRatioY = height / 480.0;

    frameInfo.fInstFps = 0.0;
    frameInfo.fAvgFps = 0.0;
    frameInfo.nInstFrames = 0;
    frameInfo.nTotalFrames = 0;
    fFPSPrevInstTime = GfTimeClock();
    nFPSTotalSeconds = 0;

    // Create the screens and initialize each board.
    for (i = 0; i < GR_NB_MAX_SCREEN; i++) {
        grScreens[i] = new cGrScreen(i);
        grScreens[i]->initBoard();
    }

    GfuiAddKey(screen, GFUIK_END,      "Zoom Minimum", NULL,	grSetMinZoom, NULL);
    GfuiAddKey(screen, GFUIK_HOME,     "Zoom Maximum", NULL,	grSetMaxZoom, NULL);
    GfuiAddKey(screen, '*',            "Zoom Default", NULL,	grSetDefaultZoom, NULL);

    GfuiAddKey(screen, GFUIK_PAGEUP,   "Select Previous Car", (void*)0, grPrevCar, NULL);
    GfuiAddKey(screen, GFUIK_PAGEDOWN, "Select Next Car",     (void*)0, grNextCar, NULL);

    GfuiAddKey(screen, GFUIK_F2,       "1st Person Views",  NULL, gr1stPersonView, NULL);
    GfuiAddKey(screen, GFUIK_F3,       "3rd Person Views",  NULL, gr3rdPersonView, NULL);
    GfuiAddKey(screen, GFUIK_F4,       "Side Car Views",    NULL, grSideCarView, NULL);
    GfuiAddKey(screen, GFUIK_F5,       "Up Car View",       NULL, grUpCarView, NULL);
    GfuiAddKey(screen, GFUIK_F6,       "Persp Car View",    NULL, grPerspCarView, NULL);
    GfuiAddKey(screen, GFUIK_F7,       "All Circuit Views", NULL, grAllCircuitView, NULL);
    GfuiAddKey(screen, GFUIK_F8,       "Action Cam Views",  NULL, grActionCamView, NULL);
    GfuiAddKey(screen, GFUIK_F9,       "TV Camera Views",   NULL, grTVCamView, NULL);
    GfuiAddKey(screen, GFUIK_F10,      "Helicopter Views",  NULL, grHelicopterView, NULL);
    GfuiAddKey(screen, GFUIK_F11,      "TV Director View",  NULL, grTVDirectorView, NULL);

    GfuiAddKey(screen, '6',            "Dashboard",         NULL, grDashboard, NULL);
    GfuiAddKey(screen, '5',            "Debug Info",        NULL, grDebugInfo, NULL);
    GfuiAddKey(screen, '4',            "G/Cmd Graph",       NULL, grGCmdGraph, NULL);
    GfuiAddKey(screen, '3',            "Leaders Board",     NULL, grLeadersBoard, NULL);
    GfuiAddKey(screen, '2',            "Driver Counters",   NULL, grDriverCounters, NULL);
    GfuiAddKey(screen, '1',            "Driver Board",      NULL, grDriverBoard, NULL);
    GfuiAddKey(screen, '7',            "Delta Best Lap",    NULL, grDeltaBestLap, NULL);
    GfuiAddKey(screen, '9',            "Mirror",            (void*)0, grSwitchMirror, NULL);
    GfuiAddKey(screen, '0',            "Arcade Board",      NULL, grArcadeBoard, NULL);
    GfuiAddKey(screen, '+', GFUIM_CTRL, "Zoom In",           NULL,	 grZoomIn, NULL);
    GfuiAddKey(screen, '=', GFUIM_CTRL, "Zoom In",           NULL,	 grZoomIn, NULL);
    GfuiAddKey(screen, '-', GFUIM_CTRL, "Zoom Out",          NULL, grZoomOut, NULL);
    //GfuiAddKey(screen, '>',             "Zoom In",           (void*)GR_ZOOM_IN,	 grSetZoom, NULL);
    //GfuiAddKey(screen, '<',             "Zoom Out",          (void*)GR_ZOOM_OUT, grSetZoom, NULL);
    GfuiAddKey(screen, '(',            "Split Screen",   NULL, grSplitScreen, NULL);
    GfuiAddKey(screen, ')',            "UnSplit Screen", NULL, grUnSplitScreen, NULL);
    GfuiAddKey(screen, '_',            "Split Screen Arrangement", NULL, grSplitScreenArr, NULL);
    GfuiAddKey(screen, GFUIK_TAB,      "Next (split) Screen", NULL, grNextScreen, NULL);
    GfuiAddKey(screen, 'm',            "Track Maps",          (void*)0, grSelectTrackMap, NULL);

    GfLogInfo("Current screen is #%d (out of %d)\n", nCurrentScreenIndex, grNbActiveScreens);

    grInitScene();

    grLodFactorValue = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_LODFACTOR, NULL, 1.0);

    return 0; // true;
}


int
refresh(tSituation *s)
{
    int	i;

    GfProfStartProfile("refresh");

    // Compute FPS indicators every second.
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
        frameInfo.fAvgFps = (double)frameInfo.nTotalFrames / nFPSTotalSeconds;
    }

    TRACE_GL("refresh: start");

    // Update sky if dynamic time enabled.
    grUpdateSky(s->currentTime, s->accelTime);

    GfProfStartProfile("grDrawBackground/glClear");
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GfProfStopProfile("grDrawBackground/glClear");

    grAdaptScreenSize();

    for (i = 0; i < grNbActiveScreens; i++)
    {
        grScreens[i]->update(s, &frameInfo);
    }

    grUpdateSmoke(s->currentTime);
    grTrackLightUpdate(s);

    GfProfStopProfile("refresh");

    return 0;
}

int
initCars(tSituation *s)
{
    char	idx[16];
    int		index;
    int		i;
    tCarElt 	*elt;
    void	*hdle;
    const char *pszSpanSplit;
    int grNbSuggestedScreens = 0;

    TRACE_GL("initCars: start");

    if (!grHandle)
    {
        grHandle = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
    }

    grInitCommonState();
    grInitCarlight(s->_ncars);
    grMaxDammage = (tdble)s->_maxDammage;
    grNbCars = s->_ncars;

    grCustomizePits();

    grCarInfo = (tgrCarInfo*)calloc(s->_ncars, sizeof(tgrCarInfo));

    for (i = 0; i < s->_ncars; i++) {
        elt = s->cars[i];
        /* Car pre-initialization */
        grPreInitCar(elt);
        /* Shadow init (Should be done before the cars for display order) */
        grInitShadow(elt);
        /* Skidmarks init */
        grInitSkidmarks(elt);
    }

    for (i = 0; i < s->_ncars; i++) {
    elt = s->cars[i];
    index = elt->index;
    hdle = elt->_paramsHandle;

    // WARNING: This index hack on the human robot for the Career mode
    //          does no more work with the new "welcome" module system
    //          (the "normal" index has no more the 10 limit) ... TO BE FIXED !!!!!!!
    if (elt->_driverType == RM_DRV_HUMAN && elt->_driverIndex > 10) {
        int clamp_driverIndex = elt->_driverIndex - 11 < 100 ? elt->_driverIndex - 11 : 99;
        snprintf(idx, sizeof(idx), "Robots/index/%d", clamp_driverIndex);
    }
    else
        snprintf(idx, sizeof(idx), "Robots/index/%d", elt->_driverIndex);

    grCarInfo[index].iconColor[0] = GfParmGetNum(hdle, idx, "red",   (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "red",   NULL, 0));
    grCarInfo[index].iconColor[1] = GfParmGetNum(hdle, idx, "green", (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "green", NULL, 0));
    grCarInfo[index].iconColor[2] = GfParmGetNum(hdle, idx, "blue",  (char*)NULL, GfParmGetNum(hdle, ROB_SECT_ARBITRARY, "blue",  NULL, 0));
    grCarInfo[index].iconColor[3] = 1.0;
    grInitCar(elt);

    // Pre-assign each human driver (if any) to a different screen
    // (set him as the "current driver" for this screen).
    if (grNbSuggestedScreens < GR_NB_MAX_SCREEN
        && elt->_driverType == RM_DRV_HUMAN && !elt->_networkPlayer)
    {
        grScreens[grNbSuggestedScreens]->setCurrentCar(elt);
        GfLogTrace("Screen #%d : Assigned to %s\n", grNbSuggestedScreens, elt->_name);
        grNbSuggestedScreens++;
    }
    }

    /* Check whether view should be spanned across vertical splits */
    pszSpanSplit = GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);
    grSpanSplit = strcmp(pszSpanSplit, GR_VAL_YES) ? 0 : 1;
    nCurrentScreenIndex = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);

    if (grSpanSplit == 0 && grNbSuggestedScreens > 1) {
        // Mulitplayer, so ignore the stored number of screens
        grNbActiveScreens = grNbSuggestedScreens;
        grNbArrangeScreens = 0;
    } else {
            // Load the real number of active screens and the arrangment.
        grNbActiveScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_NB_SCREENS, NULL, 1.0);
        grNbArrangeScreens = (int)GfParmGetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_ARR_SCREENS, NULL, 0.0);
    }

    // Ensure current screen index stays in the righ range.
    if (nCurrentScreenIndex >= grNbActiveScreens) {
        nCurrentScreenIndex = grNbActiveScreens - 1;
        GfParmSetNum(grHandle, GR_SCT_DISPMODE, GR_ATT_CUR_SCREEN, NULL, nCurrentScreenIndex);
    }

    // Initialize the cameras for all the screens.
    for (i = 0; i < GR_NB_MAX_SCREEN; i++) {
    grScreens[i]->initCams(s);
    }

    //Write back to file
    GfParmWriteFile(NULL, grHandle, "Graph");

    TRACE_GL("initCars: end");

    // Initialize other stuff.
    grInitSmoke(s->_ncars);
    grTrackLightInit();

    // Setup the screens (= OpenGL viewports) inside the physical game window.
    grAdaptScreenSize();

    return 0; // true;
}

void
shutdownCars(void)
{
    int i;

    GfOut("-- shutdownCars\n");

    if (grNbCars)
    {
        grShutdownBoardCar();
        grShutdownSkidmarks();
        grShutdownSmoke();
        grShutdownCarlight();
        grTrackLightShutdown();
        /* Delete ssg objects */
        CarsAnchor->removeAllKids();
        ShadowAnchor->removeAllKids();

        for (i = 0; i < grNbCars; i++)
        {
            ssgDeRefDelete(grCarInfo[i].envSelector);
            ssgDeRefDelete(grCarInfo[i].shadowBase);

            if (grCarInfo[i].driverSelectorinsg == false)
                delete grCarInfo[i].driverSelector;

            if (grCarInfo[i].rearwingSelectorinsg == false)
                delete grCarInfo[i].rearwingSelector;
        }

        PitsAnchor->removeAllKids();
        ThePits = 0;
        free(grCarInfo);
    }

    for (i = 0; i < GR_NB_MAX_SCREEN; i++)
    {
        grScreens[i]->setCurrentCar(NULL);
    }

    GfParmReleaseHandle(grHandle);
    grHandle = NULL;

    if (nFPSTotalSeconds > 0)
        GfLogTrace("Average frame rate: %.2f F/s\n",
                   (double)frameInfo.nTotalFrames/((double)nFPSTotalSeconds + GfTimeClock() - fFPSPrevInstTime));
}

int
initTrack(tTrack *track)
{
    // The inittrack does as well init the context, that is highly inconsistent, IMHO.
    // TODO: Find a solution to init the graphics first independent of objects.
    grContext.makeCurrent();

    setupOpenGLFeatures();

    grssgSetCurrentOptions(&options);

    // Now, do the real track loading job.
    grTrackHandle = GfParmReadFileBoth(track->filename, GFPARM_RMODE_STD);
    if (!grTrackHandle) {
        GfLogError("GfParmReadFileBoth %s failed\n", track->filename);
        return -1;
    }

    if (grNbActiveScreens > 0)
        return grLoadScene(track);

    return -1;
}


void
shutdownTrack(void)
{
    // Do the real track termination job.
    grShutdownScene();

    if (grTrackHandle)
    {
        GfParmReleaseHandle(grTrackHandle);
        grTrackHandle = 0;
    }

    // And then the context termination job (should not be there, see initTrack).
    options.endLoad();

    grShutdownState();
}

void
shutdownView(void)
{
    for (int i = 0; i < GR_NB_MAX_SCREEN; i++)
    {
        delete grScreens[i];
        grScreens[i] = 0;
    }
}

} // namespace ssggraph
