/***************************************************************************

    file                 : displayconfig.cpp
    created              : October 2010
    copyright            : (C) 2010 Jean-Philippe Meuret
    web                  : speed-dreams.sourceforge.net

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Display configuration menu */

#include <portability.h>

#include <sstream>
#include <tgfclient.h>
#include <graphic.h>

#include "legacymenu.h"
#include "displayconfig.h"


// Some consts.
static const char* ADisplayModes[DisplayMenu::nDisplayModes] = { "Full-screen", "Windowed", "Resizable" };
static const char* MonitorTypes[DisplayMenu::nDisplayTypes] = { "none", "4:3", "16:9", "21:9" };
static const char* SpansplitValues[] = { GR_VAL_NO, GR_VAL_YES };
static const int NbSpansplitValues = sizeof(SpansplitValues) / sizeof(SpansplitValues[0]);
static const int AMaxRefreshRates[] = {30, 40, 50, 60, 72, 75, 85, 100, 120, 144, 150, 200, 240, 360, 480, 1000 };
static const int NMaxRefreshRates = sizeof(AMaxRefreshRates) / sizeof(AMaxRefreshRates[0]);


// Call-backs ================================================================
void DisplayMenu::onActivate(void *pDisplayMenu)
{
	// Get the DisplayMenu instance.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pDisplayMenu);

	// Load settings from screen.xml file.
	pMenu->loadSettings();

	// Load some settings from graph.xml
	pMenu->loadGraphicSettings();

	if(GfScrUsingResizableWindow())
	{
		pMenu->_eOriginalDisplayMode = pMenu->_eDisplayMode = eResizable;
	}

	// Initialize GUI from loaded values.
	pMenu->updateControls();
}

void DisplayMenu::onChangeDisplayMode(tComboBoxInfo *pInfo)
{
 	// Get the DisplayMenu instance from call-back user data.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pInfo->userData);

	pMenu->setDisplayMode((EDisplayMode)pInfo->nPos);
}

void DisplayMenu::onChangeScreenSize(tComboBoxInfo *pInfo)
{
 	// Get the DisplayMenu instance from call-back user data.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pInfo->userData);

	pMenu->setScreenSizeIndex(pInfo->nPos);
}

void DisplayMenu::onChangeMonitorType(tComboBoxInfo *pInfo)
{
 	// Get the DisplayMenu instance from call-back user data.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pInfo->userData);

	pMenu->setMonitorType((EDisplayType)pInfo->nPos);
}

void DisplayMenu::onChangeSpansplit(tComboBoxInfo *pInfo)
{
 	// Get the DisplayMenu instance from call-back user data.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pInfo->userData);

	pMenu->_nSpansplitIndex = pInfo->nPos;
}

void DisplayMenu::onChangeBezelComp(void *pDisplayMenu)
{
    DisplayMenu* pMenu = static_cast<DisplayMenu*>(pDisplayMenu);
    char* val = GfuiEditboxGetString(pMenu->getMenuHandle(), pMenu->_nBezelCompID);
    sscanf(val, "%g", &pMenu->_fBezelComp);
    if (pMenu->_fBezelComp > 150.0f)
        pMenu->_fBezelComp = 150.0f;
    else if (pMenu->_fBezelComp < 50.0f)
        pMenu->_fBezelComp = 50.0f;

    char buf[32];
    sprintf(buf, "%g", pMenu->_fBezelComp);
    GfuiEditboxSetString(pMenu->getMenuHandle(), pMenu->_nBezelCompID, buf);
}

void DisplayMenu::onChangeScreenDist(void *pDisplayMenu)
{
    DisplayMenu* pMenu = static_cast<DisplayMenu*>(pDisplayMenu);
    char* val = GfuiEditboxGetString(pMenu->getMenuHandle(), pMenu->_nScreenDistId);
    sscanf(val, "%g", &pMenu->_fScreenDist);
    if (pMenu->_fScreenDist > 25.0f)
        pMenu->_fScreenDist = 25.0f;
    else if (pMenu->_fScreenDist < 0.1f)
        pMenu->_fScreenDist = 0.1f;

    char buf[32];
    sprintf(buf, "%g", pMenu->_fScreenDist);
    GfuiEditboxSetString(pMenu->getMenuHandle(), pMenu->_nScreenDistId, buf);
}

void DisplayMenu::onChangeArcRatio(void *pDisplayMenu)
{
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pDisplayMenu);

	char* val = GfuiEditboxGetString(pMenu->getMenuHandle(), pMenu->_nArcRatioID);
	sscanf(val, "%g", &pMenu->_fArcRatio);
	if (pMenu->_fArcRatio > 2.0f)
		pMenu->_fArcRatio = 2.0f;
	else if (pMenu->_fArcRatio < 0.0f)
		pMenu->_fArcRatio = 0.0f;

	char buf[32];
	sprintf(buf, "%g", pMenu->_fArcRatio);
	GfuiEditboxSetString(pMenu->getMenuHandle(), pMenu->_nArcRatioID, buf);
}

void DisplayMenu::onChangeMenuDisplay(tComboBoxInfo *pInfo)
{
	// Get the DisplayMenu instance from call-back user data.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pInfo->userData);

	pMenu->setMenuDisplay(pInfo->nPos);
}

void DisplayMenu::onChangeMaxRefreshRate(tComboBoxInfo *pInfo)
{
 	// Get the DisplayMenu instance from call-back user data.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pInfo->userData);

	pMenu->setMaxRefreshRateIndex(pInfo->nPos);
}

// Re-init screen to take new graphical settings into account (implies process restart).
void DisplayMenu::onAccept(void *pDisplayMenu)
{
	// Get the DisplayMenu instance from call-back user data.
	DisplayMenu* pMenu = static_cast<DisplayMenu*>(pDisplayMenu);

    // Force current control to loose focus (if one had it) and update associated variable.
    GfuiUnSelectCurrent();

	pMenu->storeWindowSettings();

	// Save some settings to graph.xml
	pMenu->storeGraphicSettings();

	if (pMenu->_eDisplayMode != eResizable)
	{
		// Save some settings to screen.xml
		pMenu->storeSettings();

	}

	if(pMenu->restartNeeded())
	{
		// Shutdown the user interface.
		LegacyMenu::self().shutdown();

		// Restart the game.
		GfuiApp().restart();
	}
	GfuiApp().eventLoop().setMaxRefreshRate(pMenu->_nMaxRefreshRate);
	GfuiScreenActivate(pMenu->getPreviousMenuHandle());
}

bool DisplayMenu::restartNeeded()
{
	bool needRestart = ((_eDisplayMode != _eOriginalDisplayMode)
						|| (_nScreenWidth !=_nOriginalScreenWidth)
						|| (_nScreenHeight != _nOriginalScreenHeight)
						|| (_nOriginalMenuDisplay != _nMenuDisplay))
						|| (_nOriginalMaxRefreshRate != _nMaxRefreshRate);

	if(GfScrUsingResizableWindow() && (_eDisplayMode == eResizable))
		needRestart = false;

	return needRestart;
}

void DisplayMenu::onCancel(void *pDisplayMenu)
{
	// Get the DisplayMenu instance from call-back user data.
	const DisplayMenu* pMenu = static_cast<DisplayMenu*>(pDisplayMenu);

	GfuiUnSelectCurrent();

	// Back to previous screen.
	GfuiScreenActivate(pMenu->getPreviousMenuHandle());
}

void DisplayMenu::updateControls()
{
	char buf[32];
	int enable = GFUI_ENABLE;

	int nControlId = getDynamicControlId("DisplayModeCombo");
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, _eDisplayMode);

	if(_eDisplayMode == eResizable)
	{
		enable = GFUI_DISABLE;
	}
	nControlId = getDynamicControlId("MonitorTypeCombo");
	GfuiEnable(getMenuHandle(),nControlId,enable);

	nControlId = getDynamicControlId("StartupDisplayCombo");
	GfuiEnable(getMenuHandle(),nControlId,enable);

	nControlId = getDynamicControlId("ScreenSizeCombo");
	GfuiEnable(getMenuHandle(),nControlId,enable);


	resetScreenSizes();

	nControlId = getDynamicControlId("MonitorTypeCombo");
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, _eDisplayType);

	nControlId = getDynamicControlId("SpanSplitsCombo");
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, _nSpansplitIndex);

	nControlId = getDynamicControlId("StartupDisplayCombo");
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, _nMenuDisplay);

	sprintf(buf, "%g", _fBezelComp);
	GfuiEditboxSetString(getMenuHandle(), _nBezelCompID, buf);

	sprintf(buf, "%g", _fScreenDist);
	GfuiEditboxSetString(getMenuHandle(), _nScreenDistId, buf);

	sprintf(buf, "%g", _fArcRatio);
	GfuiEditboxSetString(getMenuHandle(), _nArcRatioID, buf);

	nControlId = getDynamicControlId("MaxRefreshRateCombo");
	int nMaxRefRateIndex = 0; // Defaults to None.
	for (int nMaxRefRateInd = 0; nMaxRefRateInd < NMaxRefreshRates; nMaxRefRateInd++)
		if (_nMaxRefreshRate <= AMaxRefreshRates[nMaxRefRateInd])
		{
			nMaxRefRateIndex = nMaxRefRateInd;
			break;
		}
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nControlId, nMaxRefRateIndex);
}

void DisplayMenu::loadSettings()
{
	// Open screen config params file.
	void* hScrConfParams =
		GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	// Select the screen properties to edit : the 'in-test' ones if present,
	// otherwise the 'validated' ones.
	const char* pszScrPropSec =
		GfParmExistsSection(hScrConfParams, GFSCR_SECT_INTESTPROPS)
		? GFSCR_SECT_INTESTPROPS : GFSCR_SECT_VALIDPROPS;

	// Display mode : Full-screen or Windowed.
	const char *pszFullScreen =
		GfParmGetStr(hScrConfParams, pszScrPropSec, GFSCR_ATT_FSCR, GFSCR_VAL_NO);
	_eOriginalDisplayMode = _eDisplayMode = strcmp(pszFullScreen, GFSCR_VAL_YES) ? eWindowed : eFullScreen;

	// Screen / window size.
	_nOriginalScreenWidth =_nScreenWidth = (int)GfParmGetNum(hScrConfParams, pszScrPropSec, GFSCR_ATT_WIN_X, NULL, 800);
	_nOriginalScreenHeight = _nScreenHeight = (int)GfParmGetNum(hScrConfParams, pszScrPropSec, GFSCR_ATT_WIN_Y, NULL, 600);


	_nOriginalMenuDisplay = _nMenuDisplay = (int)GfParmGetNum(hScrConfParams, pszScrPropSec, GFSCR_ATT_STARTUPDISPLAY, NULL, 0);
	if(_nMenuDisplay >= _nAttachedDisplays)
	{
		_nMenuDisplay = 0;
	}


	// Max. refresh rate (Hz).
	_nMaxRefreshRate = _nOriginalMaxRefreshRate =
		(int)GfParmGetNum(hScrConfParams, pszScrPropSec, GFSCR_ATT_MAXREFRESH, NULL, 50);

	// Release screen config params file.
	GfParmReleaseHandle(hScrConfParams);
}

// Save graphical settings to XML file.
void DisplayMenu::storeSettings() const
{
	// Open screen config params file.
	void* hScrConfParams =
		GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	// Write new screen properties to the 'in-test' section, with 'to do' test state
	// (will become 'validated' after a succesfull restart, once we are sure they are OK :
	//  see guiscreen::GfScrShutdown).
	GfParmSetStr(hScrConfParams, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_TESTSTATE, GFSCR_VAL_TODO);

	GfParmSetNum(hScrConfParams, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_X, (char*)NULL, _nScreenWidth);
	GfParmSetNum(hScrConfParams, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_WIN_Y, (char*)NULL, _nScreenHeight);
	GfParmSetNum(hScrConfParams, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_STARTUPDISPLAY, (char*)NULL, _nMenuDisplay);
	GfParmSetNum(hScrConfParams, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_MAXREFRESH, (char*)NULL, _nMaxRefreshRate);

	const char* pszDisplMode =
		(_eDisplayMode == eFullScreen) ? GFSCR_VAL_YES : GFSCR_VAL_NO;
	GfParmSetStr(hScrConfParams, GFSCR_SECT_INTESTPROPS, GFSCR_ATT_FSCR, pszDisplMode);

	// Write and release screen config params file.
	GfParmWriteFile(NULL, hScrConfParams, "Screen");
	GfParmReleaseHandle(hScrConfParams);
}

void DisplayMenu::loadGraphicSettings()
{
	void* grHandle =
		GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	// Monitor Type : 4:3, 16:9 or 21:9
	const char *pszMonitorType =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_MONITOR, GR_VAL_MONITOR_NONE);

	for (int i = 0; i < nDisplayTypes; i++)
	{
		if (!strcmp(pszMonitorType, MonitorTypes[i]))
		{
			_eDisplayType = (EDisplayType)i;
			break;
		}
	}

	// Span Split Screens
	const char *pszSpanSplit =
		GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, GR_VAL_NO);

	_nSpansplitIndex = 0;
	for (int i = 0; i < NbSpansplitValues; i++)
	{
		if (!strcmp(pszSpanSplit, SpansplitValues[i]))
		{
			_nSpansplitIndex = i;
			break;
		}
	}

	// Bezel Compensation
	_fBezelComp = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_BEZELCOMP, "%", 110.0f);
	if (_fBezelComp > 150.0f) {
		_fBezelComp = 150.0f;
	}
	else if (_fBezelComp < 50.0f) {
		_fBezelComp = 50.0f;
	}

	_fScreenDist = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SCREENDIST, NULL, 1.0f);
	if (_fScreenDist > 5.0f)
	{
		_fScreenDist = 5.0f;
	}
	else if (_fScreenDist < 0.0f)
	{
		_fScreenDist = 0.0f;
	}

	_fArcRatio = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_ARCRATIO, NULL, 1.0f);
	if (_fArcRatio > 2.0f)
	{
		_fArcRatio = 2.0f;
	}
	else if (_fArcRatio < 0.0f)
	{
		_fArcRatio = 0.0f;
	}

	//sprintf(buf, "%g", _fArcRatio);
	//GfuiEditboxSetString(PDisplayMenu->getMenuHandle(), sArcRatioID, buf);
	//GfuiEditboxSetString(ScrHandle, ArcRatioId, buf);

	// Release screen config params file.
	GfParmReleaseHandle(grHandle);
}

// Save graphical settings to XML file.
void DisplayMenu::storeGraphicSettings() const
{
	// Open graph config params file.
    void* grHandle = GfParmReadFileLocal(GR_PARAM_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

    GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_MONITOR, MonitorTypes[_eDisplayType]);
    GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_SPANSPLIT, SpansplitValues[_nSpansplitIndex]);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_BEZELCOMP, "%", _fBezelComp);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SCREENDIST, NULL, _fScreenDist);
    GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_ARCRATIO, NULL, _fArcRatio);

    GfParmWriteFile(NULL, grHandle, "graph");

    GfParmReleaseHandle(grHandle);

}

void DisplayMenu::storeWindowSettings() const
{
	// Open screen config params file.
	void* hScrConfParams = GfParmReadFileLocal(GFSCR_CONF_FILE, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	GfParmSetNum(hScrConfParams, GFSCR_SECT_VALIDPROPS, GFSCR_ATT_MAXREFRESH, NULL, _nMaxRefreshRate);

	if(_eDisplayMode == eResizable)
		GfParmSetStr(hScrConfParams, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_RESIZABLE, GFSCR_VAL_YES);
	else
		GfParmSetStr(hScrConfParams, GFSCR_SECT_WINDOWPROPS, GFSCR_ATT_RESIZABLE, GFSCR_VAL_NO);

	// Write and release screen config params file.
	GfParmWriteFile(NULL, hScrConfParams, "Screen");
	GfParmReleaseHandle(hScrConfParams);
}

void DisplayMenu::setDisplayMode(EDisplayMode eMode)
{
	if (_eDisplayMode != eMode)
	{
		_eDisplayMode = eMode;

		updateControls();
	}
}

void DisplayMenu::resetScreenSizes()
{
	if (_eDisplayMode == eResizable)
	{
		return;
	}

	// Either show the sizes supported by the current display (Full screen
	// or the Default/Custom sizes
	if (_eDisplayMode == eFullScreen)
	{
		_vScreenSizes = GfScrGetSupportedSizes(_nMenuDisplay);
		tScreenSize _currSize = GfScrGetCurrentDisplaySize( _nMenuDisplay);
		_nScreenWidth = _currSize.width;
		_nScreenHeight = _currSize.height;
		_nMaxRefreshRate = _currSize.refresh_rate;
	}
	else
	{
		_vScreenSizes = GfScrGetWindowSizes();
		int vw = 0;
		int vh = 0;
		GfScrGetSize(&_nScreenWidth, &_nScreenHeight, &vw, &vh);
	}


	// Update combo-box with new possible sizes.
	const int nComboId = getDynamicControlId("ScreenSizeCombo");
	GfuiComboboxClear(getMenuHandle(), nComboId);
	std::ostringstream ossSize;
	for (unsigned int nSizeIndex = 0; nSizeIndex < _vScreenSizes.size(); nSizeIndex++)
	{
		ossSize.str("");
		ossSize << _vScreenSizes[nSizeIndex].width << " x " << _vScreenSizes[nSizeIndex].height;
		GfuiComboboxAddText(getMenuHandle(), nComboId, ossSize.str().c_str());
	}

	// Try and find the closest screen size to the current choice in the new list.
	// 1) Is there an exact match ?
	int nScreenSizeIndex = -1;
	for (unsigned int nSizeInd = 0; nSizeInd < _vScreenSizes.size(); nSizeInd++)
		if (_nScreenWidth == _vScreenSizes[nSizeInd].width
			&& _nScreenHeight == _vScreenSizes[nSizeInd].height)
		{
			nScreenSizeIndex = nSizeInd;
			break;
		}

	// 2) Is there an approximate match ?
	if (nScreenSizeIndex < 0)
		for (unsigned int nSizeInd = 0; nSizeInd < _vScreenSizes.size(); nSizeInd++)
			if (_nScreenWidth <= _vScreenSizes[nSizeInd].width
				&& _nScreenHeight <= _vScreenSizes[nSizeInd].height)
			{
				nScreenSizeIndex = nSizeInd;
				break;
			}

	// 3) Not found : the closest is the biggest.
	if (nScreenSizeIndex < 0)
		nScreenSizeIndex = _vScreenSizes.size() - 1;

	// 4) Store new screen size.
	_nScreenWidth = _vScreenSizes[nScreenSizeIndex].width;
	_nScreenHeight = _vScreenSizes[nScreenSizeIndex].height;

	// Select the found one in the combo-box.
	GfuiComboboxSetSelectedIndex(getMenuHandle(), nComboId, nScreenSizeIndex);

	{
		const int nComboId = getDynamicControlId("MaxRefreshRateCombo");

		if (nComboId != -1)
			for (size_t i = 0; i < NMaxRefreshRates; i++)
			{
				if (AMaxRefreshRates[i] == _nMaxRefreshRate)
				{
					GfuiComboboxSetSelectedIndex(getMenuHandle(), nComboId, i);
					break;
				}
			}
	}
}

void DisplayMenu::setScreenSizeIndex(int nIndex)
{
	_nScreenWidth = _vScreenSizes[nIndex].width;
	_nScreenHeight = _vScreenSizes[nIndex].height;
}

void DisplayMenu::setMonitorType(EDisplayType eType)
{
	if (_eDisplayType != eType)
	{
		_eDisplayType = eType;
	}
}

void DisplayMenu::setArcRatio(float ratio)
{
    printf("DisplayMenu::setArcRatio() \n");
    printf("this = %p \n", this);

    _fArcRatio = ratio;
    if (_fArcRatio > 2.0f)
    {
        _fArcRatio = 2.0f;
    }
    else if (_fArcRatio < 0.0f)
    {
        _fArcRatio = 0.0f;
    }
}

void DisplayMenu::setMenuDisplay(int nIndex)
{
	if (_nMenuDisplay != nIndex)
	{
		_nMenuDisplay = nIndex;

		resetScreenSizes();
	}
}

void DisplayMenu::setMaxRefreshRateIndex(int nIndex)
{
	_nMaxRefreshRate = AMaxRefreshRates[nIndex];
}

DisplayMenu::DisplayMenu()
: GfuiMenuScreen("displayconfigmenu.xml")
{
	_eDisplayMode = eWindowed;
	_nScreenWidth = 800;
	_nScreenHeight = 600;
	_nMenuDisplay = 0;
	_nAttachedDisplays = 0;
	_eDisplayType = eNone;
	_fArcRatio = 1.0f;
	_fBezelComp = 110.0f;
	_fScreenDist = 1.0f;
	_nOriginalScreenWidth = 800;
	_nOriginalScreenHeight = 600;
	_nOriginalMenuDisplay = 0;
	_eOriginalDisplayMode = eWindowed;
	_nMaxRefreshRate = 50;
}

bool DisplayMenu::initialize(void *pPreviousMenu)
{
	// Save the menu to return to.
	setPreviousMenuHandle(pPreviousMenu);

	// Create the menu and all its controls.
	createMenu(NULL, this, onActivate, NULL, (tfuiCallback)NULL, 1);

    openXMLDescriptor();

	const int nDisplayModeComboId =
		createComboboxControl("DisplayModeCombo", this, onChangeDisplayMode);

	createComboboxControl("ScreenSizeCombo", this, onChangeScreenSize);

	const int nDisplayTypeComboId =
		createComboboxControl("MonitorTypeCombo", this, onChangeMonitorType);

	const int nMenuDisplayComboId =
		createComboboxControl("StartupDisplayCombo", this, onChangeMenuDisplay);

	_nAttachedDisplays = GfScrGetAttachedDisplays();
	for (int i = 0; i < _nAttachedDisplays; i++)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "Display %d", i + 1);
		GfuiComboboxAddText(getMenuHandle(), nMenuDisplayComboId, buf);
	}
	if(_nAttachedDisplays < 2)
	{
		GfuiEnable(getMenuHandle(),nMenuDisplayComboId,GFUI_DISABLE);
	}

	// Load constant value lists in combo-boxes.
	// 1) Color depths combo : not constant, as depends on selected video detection mode.

	// 2) Display modes combo.
	for (int nDispModeInd = 0; nDispModeInd < nDisplayModes; nDispModeInd++)
		GfuiComboboxAddText(getMenuHandle(), nDisplayModeComboId, ADisplayModes[nDispModeInd]);

	// 3) Monitor type combo.
	for (int nDisplayTypeInd = 0; nDisplayTypeInd < nDisplayTypes; nDisplayTypeInd++)
		GfuiComboboxAddText(getMenuHandle(), nDisplayTypeComboId, MonitorTypes[nDisplayTypeInd]);

	const int nMaxRefRateComboId =
		createComboboxControl("MaxRefreshRateCombo", this, onChangeMaxRefreshRate);

	// 6) Max refresh rate combo.
	std::ostringstream ossMaxRefRate;
	for (int nRefRateInd = 0; nRefRateInd < NMaxRefreshRates; nRefRateInd++)
	{
		ossMaxRefRate.str("");
		if (AMaxRefreshRates[nRefRateInd] != 0)
			ossMaxRefRate << AMaxRefreshRates[nRefRateInd];
		else
			ossMaxRefRate << "None";
		GfuiComboboxAddText(getMenuHandle(), nMaxRefRateComboId, ossMaxRefRate.str().c_str());
	}

	const int nSpanSplitsComboId =
		createComboboxControl("SpanSplitsCombo", this, onChangeSpansplit);

	for (int index = 0; index < NbSpansplitValues; index++)
		GfuiComboboxAddText(getMenuHandle(), nSpanSplitsComboId, SpansplitValues[index]);

	_nBezelCompID = createEditControl("bezelcompedit", this, NULL, onChangeBezelComp);
	_nScreenDistId = createEditControl("screendistedit", this, NULL, onChangeScreenDist);
	_nArcRatioID = createEditControl("arcratioedit", this, NULL, onChangeArcRatio);

	createButtonControl("ApplyButton", this, onAccept);
	createButtonControl("CancelButton", this, onCancel);

	// Keyboard shortcuts.
	addDefaultShortcuts();
	addShortcut(GFUIK_RETURN, "Apply", this, onAccept, 0);
	addShortcut(GFUIK_ESCAPE, "Cancel", this, onCancel, 0);
	// TODO Keyboard shortcuts: Add support for shortcuts in GfuiCombobox ?
	//addShortcut(GFUIK_LEFT, "Previous Resolution", this, onChangeScreenSize, 0);
	//addShortcut(GFUIK_RIGHT, "Next Resolution", this, onChangeScreenSize, 0);

	createStaticControls();
	closeXMLDescriptor();

	return true;
}

// The unique DisplayMenu instance.
static DisplayMenu *PDisplayMenu = 0;

/** Create and activate the display options menu screen.
    @ingroup	screen
    @param	precMenu	previous menu to return to
*/
void* DisplayMenuInit(void *pPreviousMenu)
{
	if (!PDisplayMenu)
	{
		PDisplayMenu = new DisplayMenu;

		PDisplayMenu->initialize(pPreviousMenu);
	}

	return PDisplayMenu->getMenuHandle();
}

/** Relase the display options menu screen.
    @ingroup	screen
*/
void DisplayMenuRelease(void)
{
	delete PDisplayMenu;
	PDisplayMenu = NULL;
}
