/***************************************************************************

    file        : openglconfig.cpp
    created     : Fri Jun 3 12:52:07 CET 2004
    copyright   : (C) 2005 Bernhard Wymann
    email       : berniw@bluewin.ch

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
             Open GL options menu
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <sstream>

#include <portability.h>
#include <tgfclient.h>
#include <glfeatures.h>
#include <raceman.h>

#include "legacymenu.h"
#include "openglconfig.h"


// Texture compression.
static const char *ATextureCompTexts[] =
    {GFSCR_ATT_TEXTURECOMPRESSION_DISABLED, GFSCR_ATT_TEXTURECOMPRESSION_ENABLED};
static const int NTextureComps =
    sizeof(ATextureCompTexts) / sizeof(ATextureCompTexts[0]);
static int NCurTextureCompIndex = 0;
static int TextureCompLabelId;
static int TextureCompLeftButtonId;
static int TextureCompRightButtonId;

// Bump Mapping.
static const char *ABumpMappingTexts[] =
    {GFSCR_ATT_BUMPMAPPING_DISABLED, GFSCR_ATT_BUMPMAPPING_ENABLED};
static const int NBumpMapping =
    sizeof(ABumpMappingTexts) / sizeof(ABumpMappingTexts[0]);
static int NCurBumpMappingIndex = 0;
static int BumpMappingLabelId;
static int BumpMappingLeftButtonId;
static int BumpMappingRightButtonId;

// Max texture size (WARNING: the order in the list is important, do not change).
static int AMaxTextureSizeTexts[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
static int NMaxTextureSizes = sizeof(AMaxTextureSizeTexts) / sizeof(AMaxTextureSizeTexts[0]);
static int NCurMaxTextureSizeIndex = 0;
static int MaxTextureSizeLabelId;

static const int NDefaultTextureSize = 64; // In case everything goes wrong.

// Multi-texturing.
static const char *AMultiTextureTexts[] =
    {GFSCR_ATT_MULTITEXTURING_DISABLED, GFSCR_ATT_MULTITEXTURING_ENABLED};
static const int NMultiTextures =
    sizeof(AMultiTextureTexts) / sizeof(AMultiTextureTexts[0]);
static int NCurMultiTextureIndex = 0;

static int MultiTextureLabelId;
static int MultiTextureLeftButtonId;
static int MultiTextureRightButtonId;

//Anisotropic-filtering
static const char *AAnisotropicFilteringTexts[] =
    {GFSCR_ATT_ANISOTROPICFILTERING_DISABLED, GFSCR_ATT_ANISOTROPICFILTERING_MEDIUM,GFSCR_ATT_ANISOTROPICFILTERING_HIGH};
static const int NAnisotropicFiltering =
    sizeof(AAnisotropicFilteringTexts) / sizeof(AAnisotropicFilteringTexts[0]);
static int NCurAnisotropicFilteringIndex = 0;

static int AnisotropicFilteringLabelId;
static int AnisotropicFilteringLeftButtonId;
static int AnisotropicFilteringRightButtonId;

/* list of available graphic engine */
static const int DefaultGraphicVersion = 1;
static const char *GraphicSchemeList[] = {RM_VAL_MOD_SSGRAPH, RM_VAL_MOD_OSGGRAPH};
static const char *GraphicDispNameList[] = 	{"ssggraph (old)", "OsgGraph (new)"};
static const int NbGraphicScheme = sizeof(GraphicSchemeList) / sizeof(GraphicSchemeList[0]);
static int CurGraphicScheme = DefaultGraphicVersion;


// Multi-sampling (initialized in OpenGLMenuInit).
static std::vector<std::string> VecMultiSampleTexts;
static int NMultiSamples = 0;
static int NCurMultiSampleIndex = 0;

static int MultiSampleLabelId;
static int MultiSampleLeftButtonId;
static int MultiSampleRightButtonId;

static int GraphicSchemeId;

// GUI screen handles.
static void	*ScrHandle = NULL;
static void	*PrevHandle = NULL;

static bool BMultiSamplingWasSelected = false;
static int BPrevMultiSamplingSamples = 0;

static void onAccept(void *)
{
    // Store current state of settings to the GL features layer.
    GfglFeatures::self().select(GfglFeatures::TextureCompression,
                                 strcmp(ATextureCompTexts[NCurTextureCompIndex],
                                        GFSCR_ATT_TEXTURECOMPRESSION_ENABLED) ? false : true);
    GfglFeatures::self().select(GfglFeatures::TextureMaxSize,
                                 AMaxTextureSizeTexts[NCurMaxTextureSizeIndex]);
    GfglFeatures::self().select(GfglFeatures::MultiTexturing,
                                 strcmp(AMultiTextureTexts[NCurMultiTextureIndex],
                                        GFSCR_ATT_MULTITEXTURING_ENABLED) ? false : true);
    GfglFeatures::self().select(GfglFeatures::MultiSampling,
                                VecMultiSampleTexts[NCurMultiSampleIndex]
                                != GFSCR_ATT_MULTISAMPLING_DISABLED);
    if (VecMultiSampleTexts[NCurMultiSampleIndex] != GFSCR_ATT_MULTISAMPLING_DISABLED)
        GfglFeatures::self().select(GfglFeatures::MultiSamplingSamples,
                                    (int)pow(2.0, (double)NCurMultiSampleIndex));

    GfglFeatures::self().select(GfglFeatures::BumpMapping, strcmp(ABumpMappingTexts[NCurBumpMappingIndex],
                                        GFSCR_ATT_BUMPMAPPING_ENABLED) ? false : true);


    GfglFeatures::self().select(GfglFeatures::AnisotropicFiltering, NCurAnisotropicFilteringIndex);

    // Store settings from the GL features layer to the screen.xml file.
    GfglFeatures::self().storeSelection();

    void *paramHandle = GfParmReadFileLocal(RACE_ENG_CFG, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
    std::string prev_engine = GfParmGetStr(paramHandle, RM_SECT_MODULES,
        RM_ATTR_MOD_GRAPHIC, GraphicSchemeList[0]);
    const char *engine = GraphicSchemeList[CurGraphicScheme];

    GfParmSetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_GRAPHIC, engine);
    GfParmWriteFile(NULL, paramHandle, "raceengine");
    GfParmReleaseHandle(paramHandle);

    // Return to previous screen.
    GfuiScreenActivate(PrevHandle);

    if(GfScrUsingResizableWindow() == false)
    {
        // But actually restart the game if the multi-sampling feature settings changed
        // (we can't change this without re-initializing the video mode).
        if (GfglFeatures::self().isSelected(GfglFeatures::MultiSampling) != BMultiSamplingWasSelected
            || GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples) != BPrevMultiSamplingSamples)
        {
            // Shutdown the user interface.
            LegacyMenu::self().shutdown();

            // Restart the game.
            GfuiApp().restart();
        }
    }

    if (engine != prev_engine)
        GfuiApp().restart();
}

// Toggle texture compression state enabled/disabled.
static void changeTextureCompressionState(int dir)
{
    NCurTextureCompIndex = (NCurTextureCompIndex + dir + NTextureComps) % NTextureComps;
    GfuiLabelSetText(ScrHandle, TextureCompLabelId, ATextureCompTexts[NCurTextureCompIndex]);
}

static void changeTextureCompressionLeft(void *)
{
    changeTextureCompressionState(-1);
}

static void changeTextureCompressionRight(void *)
{
    changeTextureCompressionState(1);
}

// Toggle multi-texturing state enabled/disabled.
static void changeMultiTextureState(int dir)
{
    NCurMultiTextureIndex = (NCurMultiTextureIndex + dir + NMultiTextures) % NMultiTextures;
    GfuiLabelSetText(ScrHandle, MultiTextureLabelId, AMultiTextureTexts[NCurMultiTextureIndex]);
}

static void changeMultiTextureLeft(void *)
{
    changeMultiTextureState(-1);
}

static void changeMultiTextureRight(void *)
{
    changeMultiTextureState(1);
}

// Toggle multi-sampling state enabled/disabled.
static void changeMultiSampleState(int dir)
{
    NCurMultiSampleIndex = (NCurMultiSampleIndex + dir + NMultiSamples) % NMultiSamples;
    GfuiLabelSetText(ScrHandle, MultiSampleLabelId,
                     VecMultiSampleTexts[NCurMultiSampleIndex].c_str());
}

static void changeMultiSampleLeft(void *)
{
    changeMultiSampleState(-1);
}

static void changeMultiSampleRight(void *)
{
    changeMultiSampleState(1);
}

// Scroll through texture sizes smaller or equal the system limit.
static void changeMaxTextureSizeState(int delta)
{
    char valuebuf[10];

    NCurMaxTextureSizeIndex += delta;
    if (NCurMaxTextureSizeIndex < 0)
    {
        NCurMaxTextureSizeIndex = NMaxTextureSizes - 1;
    } else if (NCurMaxTextureSizeIndex >= NMaxTextureSizes)
    {
        NCurMaxTextureSizeIndex= 0;
    }

    snprintf(valuebuf, sizeof(valuebuf), "%d", AMaxTextureSizeTexts[NCurMaxTextureSizeIndex]);
    GfuiLabelSetText(ScrHandle, MaxTextureSizeLabelId, valuebuf);
}

static void changeMaxTextureSizeLeft(void *)
{
    changeMaxTextureSizeState(-1);
}

static void changeMaxTextureSizeRight(void *)
{
    changeMaxTextureSizeState(1);
}

// Toggle bumpmapping state enabled/disabled.
static void changeBumpMappingState(int dir)
{
    NCurBumpMappingIndex = (NCurBumpMappingIndex + dir + NBumpMapping) % NBumpMapping;
    GfuiLabelSetText(ScrHandle, BumpMappingLabelId, ABumpMappingTexts[NCurBumpMappingIndex]);
}

static void changeBumpMappingLeft(void *)
{
    changeBumpMappingState(-1);
}

static void changeBumpMappingRight(void *)
{
    changeBumpMappingState(1);
}

// Toggle anisotropic filtering state enabled/disabled.
static void changeAnisotropicFilteringState(int dir)
{
    NCurAnisotropicFilteringIndex = (NCurAnisotropicFilteringIndex + dir + NAnisotropicFiltering) % NAnisotropicFiltering;
    GfuiLabelSetText(ScrHandle, AnisotropicFilteringLabelId, AAnisotropicFilteringTexts[NCurAnisotropicFilteringIndex]);
}

static void changeAnisotropicFilteringLeft(void *)
{
    changeAnisotropicFilteringState(-1);
}

static void changeAnisotropicFilteringRight(void *)
{
    changeAnisotropicFilteringState(1);
}

/* Change the graphc engine version (but only show really available modules) */
static void
onChangeGraphicVersion(int dir)
{
    char buf[1024];

    if (!dir)
        return;

    const int oldGraphicVersion = CurGraphicScheme;
    do
    {
        CurGraphicScheme = (CurGraphicScheme + NbGraphicScheme + dir) % NbGraphicScheme;

        snprintf(buf, sizeof(buf), "%smodules/graphic/%s%s", GfLibDir(), GraphicSchemeList[CurGraphicScheme], DLLEXT);
    }
    while (!GfFileExists(buf) && CurGraphicScheme != oldGraphicVersion);

    GfuiLabelSetText(ScrHandle, GraphicSchemeId, GraphicDispNameList[CurGraphicScheme]);
}

static void onChangeGraphicLeft(void *)
{
    onChangeGraphicVersion(-1);
}

static void onChangeGraphicRight(void *)
{
    onChangeGraphicVersion(1);
}

static void onActivate(void * /* dummy */)
{
    int i;
    char valuebuf[10];

    // Initialize current state and GUI from the GL features layer.
    // 1) Texture compression.
    if (GfglFeatures::self().isSupported(GfglFeatures::TextureCompression))
    {
        const char *pszTexComp =
            GfglFeatures::self().isSelected(GfglFeatures::TextureCompression)
            ? GFSCR_ATT_TEXTURECOMPRESSION_ENABLED : GFSCR_ATT_TEXTURECOMPRESSION_DISABLED;
        for (i = 0; i < NTextureComps; i++)
        {
            if (!strcmp(pszTexComp, ATextureCompTexts[i]))
            {
                NCurTextureCompIndex = i;
                break;
            }
        }

        GfuiLabelSetText(ScrHandle, TextureCompLabelId,
                         ATextureCompTexts[NCurTextureCompIndex]);
    }
    else
    {
        GfuiEnable(ScrHandle, TextureCompLeftButtonId, GFUI_DISABLE);
        GfuiEnable(ScrHandle, TextureCompRightButtonId, GFUI_DISABLE);
        GfuiLabelSetText(ScrHandle, TextureCompLabelId, "Not supported");
    }

    // 2) Max texture size.
    int sizelimit =
        GfglFeatures::self().getSupported(GfglFeatures::TextureMaxSize);
    int tsize =
        GfglFeatures::self().getSelected(GfglFeatures::TextureMaxSize);

    int maxsizenb = 0;
    for (i = 0; i < NMaxTextureSizes; i++)
    {
        if (AMaxTextureSizeTexts[i] <= sizelimit)
        {
            maxsizenb = i;
        } else {
            break;
        }
    }

    // Limit to available sizes.
    NMaxTextureSizes = maxsizenb + 1;

    bool found = false;
    for (i = 0; i < NMaxTextureSizes; i++)
    {
        if (AMaxTextureSizeTexts[i] == tsize)
        {
            NCurMaxTextureSizeIndex = i;
            found = true;
            break;
        }
    }

    if (!found)
    {
        // Should never come here if there is no bug in OpenGL.
        tsize = NDefaultTextureSize;
        for (i = 0; i < NMaxTextureSizes; i++)
        {
            if (AMaxTextureSizeTexts[i] == tsize)
            {
                NCurMaxTextureSizeIndex = i;
                break;
            }
        }
    }

    snprintf(valuebuf, sizeof(valuebuf), "%d", AMaxTextureSizeTexts[NCurMaxTextureSizeIndex]);
    GfuiLabelSetText(ScrHandle, MaxTextureSizeLabelId, valuebuf);

    // 3) Multi-texturing.
    if (GfglFeatures::self().isSupported(GfglFeatures::MultiTexturing))
    {
        const char *pszMultiTex =
            GfglFeatures::self().isSelected(GfglFeatures::MultiTexturing)
            ? GFSCR_ATT_MULTITEXTURING_ENABLED : GFSCR_ATT_MULTITEXTURING_DISABLED;
        for (i = 0; i < NMultiTextures; i++)
        {
            if (!strcmp(pszMultiTex, AMultiTextureTexts[i]))
            {
                NCurMultiTextureIndex = i;
                break;
            }
        }

        GfuiLabelSetText(ScrHandle, MultiTextureLabelId,
                         AMultiTextureTexts[NCurMultiTextureIndex]);
    }
    else
    {
        GfuiEnable(ScrHandle, MultiTextureLeftButtonId, GFUI_DISABLE);
        GfuiEnable(ScrHandle, MultiTextureRightButtonId, GFUI_DISABLE);
        GfuiLabelSetText(ScrHandle, MultiTextureLabelId, "Not supported");
    }

    // 4) Multi-sampling (anti-aliasing).
    if (GfglFeatures::self().isSupported(GfglFeatures::MultiSampling))
    {
        BMultiSamplingWasSelected =
            GfglFeatures::self().isSelected(GfglFeatures::MultiSampling);
        BPrevMultiSamplingSamples =
            GfglFeatures::self().getSelected(GfglFeatures::MultiSamplingSamples);

        if (!BMultiSamplingWasSelected)
            NCurMultiSampleIndex = 0;
        else
        {
            NCurMultiSampleIndex = 0;
            int nSamples = 1;
            while (nSamples < BPrevMultiSamplingSamples)
            {
                NCurMultiSampleIndex++;
                nSamples *= 2;
            }
        }

        GfuiLabelSetText(ScrHandle, MultiSampleLabelId,
                         VecMultiSampleTexts[NCurMultiSampleIndex].c_str());
    }
    else
    {
        GfuiEnable(ScrHandle, MultiSampleLeftButtonId, GFUI_DISABLE);
        GfuiEnable(ScrHandle, MultiSampleRightButtonId, GFUI_DISABLE);
        GfuiLabelSetText(ScrHandle, MultiSampleLabelId, "Not supported");
    }

    // Initialize current state and GUI from the GL features layer.
    // 6) Bump Mapping.
    if (GfglFeatures::self().isSupported(GfglFeatures::BumpMapping))
    {
        const char *pszBumpMapping =
            GfglFeatures::self().isSelected(GfglFeatures::BumpMapping)
            ? GFSCR_ATT_BUMPMAPPING_ENABLED : GFSCR_ATT_BUMPMAPPING_DISABLED;
        for (i = 0; i < NBumpMapping; i++)
        {
            if (!strcmp(pszBumpMapping, ABumpMappingTexts[i]))
            {
                NCurBumpMappingIndex = i;
                break;
            }
        }

        GfuiLabelSetText(ScrHandle, BumpMappingLabelId,
                         ABumpMappingTexts[NCurBumpMappingIndex]);
    }
    else
    {
        GfuiEnable(ScrHandle, BumpMappingLeftButtonId, GFUI_DISABLE);
        GfuiEnable(ScrHandle, BumpMappingRightButtonId, GFUI_DISABLE);
        GfuiLabelSetText(ScrHandle, BumpMappingLabelId, "Not supported");
    }

    // Initialize current state and GUI from the GL features layer.
    // 7) Anisotropic Filtering.
    int ani_sup= GfglFeatures::self().getSupported(GfglFeatures::AnisotropicFiltering);
    if (ani_sup!=GfglFeatures::InvalidInt)
    {
        NCurAnisotropicFilteringIndex =
                GfglFeatures::self().getSelected(GfglFeatures::AnisotropicFiltering);
        GfuiLabelSetText(ScrHandle, AnisotropicFilteringLabelId,
                         AAnisotropicFilteringTexts[NCurAnisotropicFilteringIndex]);
    }
    else
    {
        GfuiEnable(ScrHandle, AnisotropicFilteringLeftButtonId, GFUI_DISABLE);
        GfuiEnable(ScrHandle, AnisotropicFilteringRightButtonId, GFUI_DISABLE);
        GfuiLabelSetText(ScrHandle, AnisotropicFilteringLabelId, "Not supported");
    }

    // Initialize current Graphic Engine from the raceengine config file
    // ssggraph / OsgGraph
    void *paramHandle = GfParmReadFileLocal(RACE_ENG_CFG, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
    const char *graphicSchemeName = GfParmGetStr(paramHandle, RM_SECT_MODULES, RM_ATTR_MOD_GRAPHIC, GraphicSchemeList[1]);
    for (int i = 0; i < NbGraphicScheme; i++)
    {
        if (strcmp(graphicSchemeName, GraphicSchemeList[i]) == 0)
        {
            CurGraphicScheme = i;
            break;
        }
    }

    GfuiLabelSetText(ScrHandle, GraphicSchemeId, GraphicDispNameList[CurGraphicScheme]);
}

// OpenGL menu
void* OpenGLMenuInit(void *prevMenu)
{
    //const char *graphicSchemeName;
    // Has screen already been created?
    if (ScrHandle)
        return ScrHandle;

    PrevHandle = prevMenu;

    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);
    void *hparmMenu = GfuiMenuLoad("opengloptionsmenu.xml");
    GfuiMenuCreateStaticControls(ScrHandle, hparmMenu);

    //Initialize current Graphic Engine
    // ssggraph / OsgGraph
    // See onActivate()

    // Texture compression.
    TextureCompLeftButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "TextureCompressionLeftArrowButton", NULL,
                            changeTextureCompressionLeft);
    TextureCompRightButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "TextureCompressionRightArrowButton", NULL,
                            changeTextureCompressionRight);
    TextureCompLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"TextureCompressionLabel");

    // Texture sizing.
    GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"MaxTextureSizeLeftArrowButton", NULL,
                        changeMaxTextureSizeLeft);
    GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"MaxTextureSizeRightArrowButton", NULL,
                        changeMaxTextureSizeRight);
    MaxTextureSizeLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"MaxTextureSizeLabel");

    // Multi-texturing.
    MultiTextureLeftButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiTextureLeftArrowButton", NULL,
                            changeMultiTextureLeft);
    MultiTextureRightButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiTextureRightArrowButton", NULL,
                            changeMultiTextureRight);
    MultiTextureLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"MultiTextureLabel");

    // Multi-sampling.
    MultiSampleLeftButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiSampleLeftArrowButton", NULL,
                            changeMultiSampleLeft);
    MultiSampleRightButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "MultiSampleRightArrowButton", NULL,
                            changeMultiSampleRight);
    MultiSampleLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"MultiSampleLabel");

    // Bump Mapping.
    BumpMappingLeftButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "BumpMappingLeftArrowButton", NULL,
                            changeBumpMappingLeft);
    BumpMappingRightButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "BumpMappingRightArrowButton", NULL,
                            changeBumpMappingRight);
    BumpMappingLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"BumpMappingLabel");

    // Anisotropic Filtering.
    AnisotropicFilteringLeftButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "AnisotropicFilteringLeftArrowButton", NULL,
                            changeAnisotropicFilteringLeft);
    AnisotropicFilteringRightButtonId =
        GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "AnisotropicFilteringRightArrowButton", NULL,
                            changeAnisotropicFilteringRight);
    AnisotropicFilteringLabelId = GfuiMenuCreateLabelControl(ScrHandle,hparmMenu,"AnisotropicFilteringLabel");

    GraphicSchemeId = GfuiMenuCreateLabelControl(ScrHandle, hparmMenu, "graphiclabel");
#ifndef OFFICIAL_ONLY
    GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "graphicleftarrow", NULL, onChangeGraphicLeft);
    GfuiMenuCreateButtonControl(ScrHandle, hparmMenu, "graphicrightarrow", NULL, onChangeGraphicRight);
#endif

    GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"ApplyButton",NULL, onAccept);
    GfuiMenuCreateButtonControl(ScrHandle,hparmMenu,"CancelButton",prevMenu, GfuiScreenActivate);

    GfParmReleaseHandle(hparmMenu);

    GfuiMenuDefaultKeysAdd(ScrHandle);
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Apply", NULL, onAccept, NULL);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", prevMenu, GfuiScreenActivate, NULL);
    GfuiAddKey(ScrHandle, GFUIK_LEFT, "Decrease Texture Size Limit", NULL, changeMaxTextureSizeLeft, NULL);
    GfuiAddKey(ScrHandle, GFUIK_RIGHT, "Increase Texture Size Limit", NULL, changeMaxTextureSizeRight, NULL);
    GfuiAddKey(ScrHandle, ' ', "Toggle Texture Compression", NULL, changeTextureCompressionRight, NULL);

    // Initialize multi-sampling levels.
    NMultiSamples = 1;
    VecMultiSampleTexts.push_back(GFSCR_ATT_MULTISAMPLING_DISABLED);
    if (GfglFeatures::self().isSupported(GfglFeatures::MultiSampling)
        && GfglFeatures::self().getSupported(GfglFeatures::MultiSamplingSamples) > 1)
    {
        const int nMaxSamples =
            GfglFeatures::self().getSupported(GfglFeatures::MultiSamplingSamples);
        NMultiSamples += (int)(log((double)nMaxSamples) / log(2.0));
        std::ostringstream ossVal;
        for (int nVal = 2; nVal <= nMaxSamples; nVal *= 2)
        {
            ossVal.str("");
            ossVal << nVal << "x";
            VecMultiSampleTexts.push_back(ossVal.str());
        }
    }

    return ScrHandle;
}
