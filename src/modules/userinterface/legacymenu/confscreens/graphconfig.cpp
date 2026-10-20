/***************************************************************************

    file                 : graphconfig.cpp
    created              : Sun Jun  9 16:30:25 CEST 2002
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

#include <portability.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <tgfclient.h>
#include <graphic.h>

#include "graphconfig.h"

static const int SkyDomeDistanceValues[] = {0, 12000, 20000, 40000, 80000};
static const int NbSkyDomeDistanceValues = sizeof(SkyDomeDistanceValues) / sizeof(SkyDomeDistanceValues[0]);

static const char* DynamicTimeOfDayValues[] =
	{ GR_ATT_DYNAMICSKYDOME_DISABLED, GR_ATT_DYNAMICSKYDOME_ENABLED };
static const int NbDynamicTimeOfDayValues = sizeof(DynamicTimeOfDayValues) / sizeof(DynamicTimeOfDayValues[0]);
static const int PrecipDensityValues[] = {0, 20, 40, 60, 80, 100};
static const int NbPrecipDensityValues = sizeof(PrecipDensityValues) / sizeof(PrecipDensityValues[0]);
static const int CloudLayersValues[] = {1, 2, 3};
static const int NbCloudLayersValues = sizeof(CloudLayersValues) / sizeof(CloudLayersValues[0]);
static const char* BackgroundLandscapeValues[] = { GR_ATT_BGSKY_DISABLED, GR_ATT_BGSKY_ENABLED };
static const int NbBackgroundLandscapeValues = sizeof(BackgroundLandscapeValues) / sizeof(BackgroundLandscapeValues[0]);

static const int VisibilityValues[] = { 4000, 6000, 8000, 10000, 12000 };
static const int NbVisibilityValues = sizeof(VisibilityValues) / sizeof(VisibilityValues[0]);

static void	*ScrHandle = NULL;

static int	FovEditId;
static int	SmokeEditId;
static int	SkidEditId;
static int	LodFactorEditId;
static int	SkyDomeDistanceLabelId;
static int	DynamicTimeOfDayLabelId, DynamicTimeOfDayLeftButtonId, DynamicTimeOfDayRightButtonId;
static int	PrecipDensityLabelId;
static int	CloudLayersLabelId, CloudLayersLeftButtonId, CloudLayersRightButtonId;
static int	BackgroundLandscapeLabelId, BackgroundLandscapeLeftButtonId, BackgroundLandscapeRightButtonId;
static int	VisibilityLabelId, VisibilityLeftButtonId, VisibilityRightButtonId;

static int	FovFactorValue = 100;
static int	SmokeValue = 300;
static int	SkidValue = 20;
static tdble	LodFactorValue = 1.0;
static int 	SkyDomeDistanceIndex = 0;
static int 	DynamicTimeOfDayIndex = 0;
static int 	PrecipDensityIndex = NbPrecipDensityValues - 1;
static int	CloudLayerIndex = 0;
static int	BackgroundLandscapeIndex = 0;
static int	VisibilityIndex = 0;

static char	buf[512];


// Options IO functions ===================================================================

static void
loadOptions()
{
    void* grHandle = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    FovFactorValue = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, "%", 100.0);

    SmokeValue = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SMOKENB, NULL, 300.0);

    SkidValue = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_MAXSTRIPBYWHEEL, NULL, 20.0);

    LodFactorValue = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_LODFACTOR, NULL, 1.0);

    SkyDomeDistanceIndex = 0; // Default value index, in case file value not found in list.
    const int nSkyDomeDist =
		(int)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE, NULL, 0) + 0.5);
    for (int i = 0; i < NbSkyDomeDistanceValues; i++)
    {
		if (nSkyDomeDist <= SkyDomeDistanceValues[i])
		{
			SkyDomeDistanceIndex = i;
			break;
		}
    }

	DynamicTimeOfDayIndex = 0; // Default value index, in case file value not found in list.
	const char* pszDynamicTimeOfDay =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_DYNAMICSKYDOME, GR_ATT_DYNAMICSKYDOME_DISABLED);
	for (int i = 0; i < NbDynamicTimeOfDayValues; i++)
	{
		if (!strcmp(pszDynamicTimeOfDay, DynamicTimeOfDayValues[i]))
		{
			DynamicTimeOfDayIndex = i;
			break;
		}
	}

	BackgroundLandscapeIndex = 0; // Default value index, in case file value not found in list.
	const char* pszBackgroundLandscape =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKY, GR_ATT_BGSKY_DISABLED);
	for (int i = 0; i < NbBackgroundLandscapeValues; i++)
	{
		if (!strcmp(pszBackgroundLandscape, BackgroundLandscapeValues[i]))
		{
			BackgroundLandscapeIndex = i;
			break;
		}
	}

	const int nCloudLayer =
		(int)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_CLOUDLAYER, NULL, 1) + 0.5f);
	for (int i = 0; i < NbCloudLayersValues; i++)
	{
		if (nCloudLayer <= CloudLayersValues[i])
		{
			CloudLayerIndex = i;
			break;
		}
	}

    PrecipDensityIndex = NbPrecipDensityValues - 1; // Default value index, in case file value not found in list.
    const int nPrecipDensity =
		(int)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_PRECIPDENSITY, "%", 100) + 0.5f);
    for (int i = 0; i < NbPrecipDensityValues; i++)
    {
		if (nPrecipDensity <= PrecipDensityValues[i])
		{
			PrecipDensityIndex = i;
			break;
		}
    }

	VisibilityIndex = NbVisibilityValues -1; // Default value index, in case file value not found in list.
	const int nVisibility =
		(int)(GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_VISIBILITY, NULL, 0)+ 0.5);
	for (int i = 0; i < NbVisibilityValues; i++)
	{
		if (nVisibility <= VisibilityValues[i])
		{
			VisibilityIndex = i;
			break;
		}
	}

    GfParmReleaseHandle(grHandle);
}

static void
saveOptions()
{
    // Force current edit to loose focus (if one has it) and update associated variable.
    GfuiUnSelectCurrent();

    void* grHandle = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, "%", FovFactorValue);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SMOKENB, NULL, SmokeValue);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_MAXSTRIPBYWHEEL, NULL, SkidValue);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_LODFACTOR, NULL, LodFactorValue);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SKYDOMEDISTANCE, NULL, SkyDomeDistanceValues[SkyDomeDistanceIndex]);
    GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_DYNAMICSKYDOME, DynamicTimeOfDayValues[DynamicTimeOfDayIndex]);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_PRECIPDENSITY, "%", PrecipDensityValues[PrecipDensityIndex]);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_CLOUDLAYER, NULL, CloudLayersValues[CloudLayerIndex]);
    GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_BGSKY, BackgroundLandscapeValues[BackgroundLandscapeIndex]);
	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_VISIBILITY, NULL, VisibilityValues[VisibilityIndex]);

    GfParmWriteFile(NULL, grHandle, "graph");

    GfParmReleaseHandle(grHandle);
}

// GUI callback functions ===================================================================


static void
onChangeVisibility(int delta)
{
    VisibilityIndex = (VisibilityIndex + NbVisibilityValues + delta) % NbVisibilityValues;
    snprintf(buf, sizeof(buf), "%d", VisibilityValues[VisibilityIndex]);
    GfuiLabelSetText(ScrHandle, VisibilityLabelId, buf);
}

static void
onVisibilityLeft(void *)
{
	onChangeVisibility(-1);
}

static void
onVisibilityRight(void *)
{
	onChangeVisibility(1);
}

static void
onChangeFov(int edit)
{
	if (edit)
	{
		// Get new value from the edit control
		char* val = GfuiEditboxGetString(ScrHandle, FovEditId);
		FovFactorValue = strtol(val, (char **)NULL, 0);
	}

	// Display current value
    snprintf(buf, sizeof(buf), "%d", FovFactorValue);
    GfuiEditboxSetString(ScrHandle, FovEditId, buf);
}

static void
onFovEdit(void *)
{
	onChangeFov(1);
}

static void
onChangeLodFactor(int edit)
{
	if (edit)
	{
		// Get new value from the edit control
		char* val = GfuiEditboxGetString(ScrHandle, LodFactorEditId);
		sscanf(val, "%g", &LodFactorValue);
	}

	// Display current value
    snprintf(buf, sizeof(buf), "%g", LodFactorValue);
    GfuiEditboxSetString(ScrHandle, LodFactorEditId, buf);
}

static void
onLodEdit(void *)
{
	onChangeLodFactor(1);
}

static void
onChangeSmoke(int edit)
{
	if (edit)
	{
		// Get new value from the edit control
		char* val = GfuiEditboxGetString(ScrHandle, SmokeEditId);
		SmokeValue = strtol(val, (char **)NULL, 0);
	}

	// Display current value
    snprintf(buf, sizeof(buf), "%d", SmokeValue);
    GfuiEditboxSetString(ScrHandle, SmokeEditId, buf);
}

static void
onSmokeEdit(void *)
{
	onChangeSmoke(1);
}

static void
onChangeSkid(int edit)
{
	if (edit)
	{
		// Get new value from the edit control
		char* val = GfuiEditboxGetString(ScrHandle, SkidEditId);
		SkidValue = strtol(val, (char **)NULL, 0);
	}

	// Display current value
    snprintf(buf, sizeof(buf), "%d", SkidValue);
    GfuiEditboxSetString(ScrHandle, SkidEditId, buf);
}

static void
onSkidEdit(void *)
{
	onChangeSkid(1);
}

static void
onChangeDynamicTimeOfDay(int delta)
{
    DynamicTimeOfDayIndex = (DynamicTimeOfDayIndex + NbDynamicTimeOfDayValues + delta) % NbDynamicTimeOfDayValues;
    GfuiLabelSetText(ScrHandle, DynamicTimeOfDayLabelId, DynamicTimeOfDayValues[DynamicTimeOfDayIndex]);
}

static void
onDynamicTimeOfDayLeft(void *)
{
	onChangeDynamicTimeOfDay(-1);
}

static void
onDynamicTimeOfDayRight(void *)
{
	onChangeDynamicTimeOfDay(1);
}

static void
onChangeBackgroundLandscape(int delta)
{
    BackgroundLandscapeIndex = (BackgroundLandscapeIndex + NbBackgroundLandscapeValues + delta) % NbBackgroundLandscapeValues;
    GfuiLabelSetText(ScrHandle, BackgroundLandscapeLabelId, BackgroundLandscapeValues[BackgroundLandscapeIndex]);
}

static void
onBackgroundLandscapeLeft(void *)
{
	onChangeBackgroundLandscape(-1);
}

static void
onBackgroundLandscapeRight(void *)
{
	onChangeBackgroundLandscape(1);
}

static void
onChangeCloudLayers(int delta)
{
    CloudLayerIndex = (CloudLayerIndex + NbCloudLayersValues + delta) % NbCloudLayersValues;
    snprintf(buf, sizeof(buf), "%d", CloudLayersValues[CloudLayerIndex]);
    GfuiLabelSetText(ScrHandle, CloudLayersLabelId, buf);
}

static void
onCloudLayerLeft(void *)
{
	onChangeCloudLayers(-1);
}

static void
onCloudLayerRight(void *)
{
	onChangeCloudLayers(1);
}

static void
onChangeSkyDomeDistance(int delta)
{
    SkyDomeDistanceIndex = (SkyDomeDistanceIndex + NbSkyDomeDistanceValues + delta) % NbSkyDomeDistanceValues;
    snprintf(buf, sizeof(buf), "%d", SkyDomeDistanceValues[SkyDomeDistanceIndex]);
    GfuiLabelSetText(ScrHandle, SkyDomeDistanceLabelId, buf);

	const bool bSkyDome = SkyDomeDistanceValues[SkyDomeDistanceIndex] != 0;

	// No changes of dynamic time of day if sky dome disabled
	GfuiEnable(ScrHandle, DynamicTimeOfDayLeftButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, DynamicTimeOfDayRightButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	if (bSkyDome)
		onChangeDynamicTimeOfDay(0); // Display real value.
	else
		GfuiLabelSetText(ScrHandle, DynamicTimeOfDayLabelId, GR_ATT_DYNAMICSKYDOME_DISABLED);


	// No changes of background landscape if sky dome disabled
	GfuiEnable(ScrHandle, BackgroundLandscapeLeftButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, BackgroundLandscapeRightButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	if (bSkyDome)
		onChangeBackgroundLandscape(0); // Display real value.
	else
		GfuiLabelSetText(ScrHandle, BackgroundLandscapeLabelId, GR_ATT_BGSKY_DISABLED);

	// No changes of nb of cloud layers if sky dome disabled
	GfuiEnable(ScrHandle, CloudLayersLeftButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, CloudLayersRightButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	if (bSkyDome)
		onChangeCloudLayers(0); // Display real value.
	else
		GfuiLabelSetText(ScrHandle, CloudLayersLabelId, "1");

	GfuiEnable(ScrHandle, VisibilityLeftButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, VisibilityRightButtonId, bSkyDome ? GFUI_ENABLE : GFUI_DISABLE);
	if (bSkyDome)
		onChangeVisibility(0); // Display Real value.
	else
		GfuiLabelSetText(ScrHandle, VisibilityLabelId, "4000");

	// No changes of FOV if sky dome enabled
	GfuiEnable(ScrHandle, FovEditId, bSkyDome ? GFUI_DISABLE : GFUI_ENABLE);
}

static void
onSkyDomeDistanceLeft(void *)
{
	onChangeSkyDomeDistance(-1);
}

static void
onSkyDomeDistanceRight(void *)
{
	onChangeSkyDomeDistance(1);
}

static void
onChangePrecipDensity(int delta)
{
    PrecipDensityIndex = (PrecipDensityIndex + NbPrecipDensityValues + delta) % NbPrecipDensityValues;
    snprintf(buf, sizeof(buf), "%d", PrecipDensityValues[PrecipDensityIndex]);
    GfuiLabelSetText(ScrHandle, PrecipDensityLabelId, buf);
}

static void
onPrecipDensityLeft(void *)
{
	onChangePrecipDensity(-1);
}

static void
onPrecipDensityRight(void *)
{
	onChangePrecipDensity(1);
}

static void
onActivate(void* /* dummy */)
{
    loadOptions();

	// Load GUI control values.
    onChangeFov(0);
	onChangeLodFactor(0);
	onChangeSmoke(0);
	onChangeSkid(0);
	onChangeSkyDomeDistance(0); // Also loads DynamicTimeOfDay,  BackgroundLandscape, CloudLayers
    onChangePrecipDensity(0);
	onChangeVisibility(0);
}

static void
onAccept(void* tgtScrHdle)
{
    saveOptions();

    GfuiScreenActivate(tgtScrHdle);
}

static void
onCancel(void* tgtScrHdle)
{
    GfuiScreenActivate(tgtScrHdle);
}


// Menu initialization ==================================================================

void*
GraphMenuInit(void* prevMenu)
{
    // Don't do it twice.
    if (ScrHandle)
		return ScrHandle;

    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);

    void* param = GfuiMenuLoad("graphicsconfigmenu.xml");

    GfuiMenuCreateStaticControls(ScrHandle, param);

    FovEditId =
		GfuiMenuCreateEditControl(ScrHandle, param, "fovedit", NULL, NULL, onFovEdit);
    SmokeEditId =
		GfuiMenuCreateEditControl(ScrHandle, param, "smokeedit", NULL, NULL, onSmokeEdit);
    SkidEditId =
		GfuiMenuCreateEditControl(ScrHandle, param, "skidedit", NULL, NULL, onSkidEdit);
    LodFactorEditId =
		GfuiMenuCreateEditControl(ScrHandle, param, "lodedit", NULL, NULL, onLodEdit);

    GfuiMenuCreateButtonControl(ScrHandle, param, "skydomedistleftarrow", NULL, onSkyDomeDistanceLeft);
    GfuiMenuCreateButtonControl(ScrHandle, param, "skydomedistrightarrow", NULL, onSkyDomeDistanceRight);
    SkyDomeDistanceLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, param, "skydomedistlabel");

    DynamicTimeOfDayLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "dynamicskydomeleftarrow", NULL, onDynamicTimeOfDayLeft);
    DynamicTimeOfDayRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "dynamicskydomerightarrow", NULL, onDynamicTimeOfDayRight);
    DynamicTimeOfDayLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, param, "dynamicskydomelabel");

    GfuiMenuCreateButtonControl(ScrHandle, param, "precipdensityleftarrow", NULL, onPrecipDensityLeft);
    GfuiMenuCreateButtonControl(ScrHandle, param, "precipdensityrightarrow", NULL, onPrecipDensityRight);
    PrecipDensityLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, param, "precipdensitylabel");

	CloudLayersLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "cloudlayernbleftarrow", NULL, onCloudLayerLeft);
	CloudLayersRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "cloudlayernbrightarrow", NULL, onCloudLayerRight);
    CloudLayersLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, param, "cloudlayerlabel");

	BackgroundLandscapeLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "bgskyleftarrow", NULL, onBackgroundLandscapeLeft);
    BackgroundLandscapeRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "bgskyrightarrow", NULL, onBackgroundLandscapeRight);
    BackgroundLandscapeLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, param, "bgskydomelabel");

	VisibilityLeftButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "visibilityleftarrow", NULL, onVisibilityLeft);
    VisibilityRightButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, param, "visibilityrightarrow", NULL, onVisibilityRight);
    VisibilityLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, param, "visibilitylabel");

    GfuiMenuCreateButtonControl(ScrHandle, param, "ApplyButton", prevMenu, onAccept);
    GfuiMenuCreateButtonControl(ScrHandle, param, "CancelButton", prevMenu, onCancel);

    GfParmReleaseHandle(param);

    // Add standard keyboard shortcuts.
    GfuiMenuDefaultKeysAdd(ScrHandle);
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Apply", prevMenu, onAccept, NULL);
    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Cancel", prevMenu, onCancel, NULL);

    return ScrHandle;
}
