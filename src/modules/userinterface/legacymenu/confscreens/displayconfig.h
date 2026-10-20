/***************************************************************************

    file                 : displayconfig.h
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

#ifndef _DISPLAYCONFIG_H_
#define _DISPLAYCONFIG_H_

#include <tgfclient.h>
#include <gui.h>

#include "confscreens.h"



class DisplayMenu : public GfuiMenuScreen
{
public:

	DisplayMenu();

	bool initialize(void* pPreviousMenu);

	enum EDisplayMode { eFullScreen = 0, eWindowed = 1, eResizable = 2, nDisplayModes };
	enum EDisplayType { eNone = 0, e4by3, e16by9, e21by9, nDisplayTypes };
	enum ESpanSplit { eDisabled = 0, eEnabled = 1, nSpanSplits };

	void setDisplayMode(EDisplayMode eMode);
	void setScreenSizeIndex(int nIndex);
	void setMonitorType(EDisplayType eType);
	void setArcRatio(float ratio);
	void setMenuDisplay(int nIndex);
	void setMaxRefreshRateIndex(int nIndex);
	void storeSettings() const;
	void loadSettings();

	void storeGraphicSettings() const;
	void loadGraphicSettings();

	void storeWindowSettings() const;

	void updateControls();

protected:

	void resetColorDepths();
	void resetScreenSizes();

	bool restartNeeded();

	// Control callback functions (must be static).
	static void onActivate(void *pDisplayMenu);
	static void onChangeScreenSize(tComboBoxInfo *pInfo);
	static void onChangeDisplayMode(tComboBoxInfo *pInfo);
	static void onChangeMonitorType(tComboBoxInfo *pInfo);
	static void onChangeSpansplit(tComboBoxInfo *pInfo);
	static void onChangeBezelComp(void *pDisplayMenu);
	static void onChangeScreenDist(void *pDisplayMenu);
	static void onChangeArcRatio(void *pDisplayMenu);
	static void onChangeMenuDisplay(tComboBoxInfo *pInfo);
	static void onChangeMaxRefreshRate(tComboBoxInfo *pInfo);

	static void onAccept(void *pDisplayMenu);
	static void onCancel(void *pDisplayMenu);

private:

	// Holds screen size choices for the current display mode.
	ScreenSizeVector _vScreenSizes;

	//! Currently selected display mode.
	EDisplayMode _eDisplayMode;
	EDisplayMode _eOriginalDisplayMode;

	//! Currently selected screen size.
	int _nScreenWidth;
	int _nScreenHeight;
	int _nMenuDisplay;
	int _nOriginalScreenWidth;
	int _nOriginalScreenHeight;
	int _nOriginalMenuDisplay;

	int _nAttachedDisplays;

	EDisplayType _eDisplayType;
	float	_fArcRatio;
	float	_fBezelComp;
	float	_fScreenDist;

	int	_nSpansplitIndex;

	int	_nBezelCompID;
	int	_nScreenDistId;
	int	_nArcRatioID;

	//! Currently selected max. refresh rate (Hz).
	int	_nMaxRefreshRate;
	int _nOriginalMaxRefreshRate;
};

extern void* DisplayMenuInit(void* pPreviousMenu);
extern void DisplayMenuRelease(void);

#endif //_DISPLAYCONFIG_H_
