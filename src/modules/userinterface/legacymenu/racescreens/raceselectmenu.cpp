/***************************************************************************

	file        : raceselectmenu.cpp
	created     : Sat Nov 16 09:36:29 CET 2002
	copyright   : (C) 2002 by Eric Espie
	email       : eric.espie@torcs.org

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
			Race selection menu
	@author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
*/

#include <map>
#include <algorithm>
#include <list>
#include <string>
#include <stdexcept>

#include <tgfclient.h>
#include <guimenu.h>

#include <raceman.h>

#include <racemanagers.h>
#include <race.h>

#include "legacymenu.h"
#include "racescreens.h"


// The Race Select menu.
void *RmRaceSelectMenuHandle = NULL;


static std::map<std::string, int> rmMapSubTypeComboIds;
static std::list<std::string> rmRaceTypes;


/* Called when the menu is activated */
static void
rmOnActivate(void * /* dummy */)
{
	GfLogTrace("Entering Race Mode Select menu\n");

	LmRaceEngine().reset();
}

/* Exit from Race engine */
static void
rmOnRaceSelectShutdown(void *prevMenu)
{
	rmRaceTypes.clear();

	GfuiScreenRelease(RmRaceSelectMenuHandle);
	RmRaceSelectMenuHandle = NULL;
	GfuiScreenActivate(prevMenu);

	LmRaceEngine().cleanup();

	LegacyMenu::self().shutdownGraphics(/*bUnloadModule=*/true);
}


static void
rmOnSelectRaceMan(void *pvRaceManTypeIndex)
{
	// Get the race managers with the given type
	const std::vector<std::string>& vecRaceManTypes = GfRaceManagers::self()->getTypes();
	std::string *strRaceManType = static_cast<std::string *>(pvRaceManTypeIndex);
	bool found = false;

	for (std::vector<std::string>::const_iterator it = vecRaceManTypes.begin();
		it != vecRaceManTypes.end(); it++)
	{
		if (*it == *strRaceManType)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		GfLogError("Race type %s not found on GfRaceManagers\n",
			strRaceManType->c_str());
		return;
	}

	const std::vector<GfRaceManager*> vecRaceMans =
		GfRaceManagers::self()->getRaceManagersWithType(*strRaceManType);

	// If more than 1, get the one with the currently selected sub-type.
	GfRaceManager* pSelRaceMan = 0;
	if (vecRaceMans.size() > 1)
	{
		const int nSubTypeComboId = rmMapSubTypeComboIds[*strRaceManType];
		const char* pszSelSubType = GfuiComboboxGetText(RmRaceSelectMenuHandle, nSubTypeComboId);
		std::vector<GfRaceManager*>::const_iterator itRaceMan;
		for (itRaceMan = vecRaceMans.begin(); itRaceMan != vecRaceMans.end(); ++itRaceMan)
		{
			if ((*itRaceMan)->getSubType() == pszSelSubType)
			{
				// Start configuring it.
				pSelRaceMan = *itRaceMan;
				break;
			}
		}
	}

	// If only 1, no choice.
	else if (vecRaceMans.size() == 1)
	{
		pSelRaceMan = vecRaceMans.back();
	}

	if (pSelRaceMan)
	{
		// Give the selected race manager to the race engine.
		LmRaceEngine().selectRaceman(pSelRaceMan);

		// Start the race configuration menus sequence.
		LmRaceEngine().configureRace(/* bInteractive */ true);
	}
	else
	{
		GfLogError("No such race manager (type '%s')\n", strRaceManType->c_str());
	}
}

static bool
rmSupported(const std::string &type)
{
	if (LmRaceEngine().supportsHumanDrivers())
		return type != "OptimizationMT";

	return type == "Practice" || type == "OptimizationMT";
}

static std::string
rmRemoveSpaces(const std::string &s)
{
	std::string ret;

	for (auto c : s)
		if (c != ' ')
			ret += c;

	return ret;
}

static int
rmCreateRaceSubtype(void *h, const char *name)
{
	const std::vector<std::string> &types = GfRaceManagers::self()->getTypes();

	for (const auto &type : types)
	{
		std::string btn = rmRemoveSpaces(type) + "Combo";

		if (!rmSupported(type) || btn != name)
			continue;

		int id = -1;
		bool first = true;
		std::vector<GfRaceManager*> managers =
			GfRaceManagers::self()->getRaceManagersWithType(type);

		for (auto m : managers)
		{
			const std::string &subtype = m->getSubType();

			if (subtype.empty())
				continue;
			else if (id < 0
				&& (id = GfuiMenuCreateComboboxControl(RmRaceSelectMenuHandle,
					h, name, nullptr, nullptr)) < 0)
			{
				GfLogError("GfuiMenuCreateComboboxControl %s failed\n", name);
				return -1;
			}

			int enable = first ? GFUI_DISABLE : GFUI_ENABLE;

			GfuiEnable(RmRaceSelectMenuHandle, id, enable);

			if (first)
				first = false;

			GfuiComboboxAddText(RmRaceSelectMenuHandle, id, subtype.c_str());
		}

		rmMapSubTypeComboIds[type] = id;
		break;
	}

	return 0;
}

static int
rmCreateRaceType(void *h, const char *name)
{
	const std::vector<std::string> &types = GfRaceManagers::self()->getTypes();

	for (const auto &type : types)
	{
		std::string btn = rmRemoveSpaces(type) + "Button";

		if (!rmSupported(type) || btn != name)
			continue;

		rmRaceTypes.push_back(type);

		if (GfuiMenuCreateButtonControl(RmRaceSelectMenuHandle, h,
			name, &rmRaceTypes.back(), rmOnSelectRaceMan) < 0)
		{
			GfLogError("GfuiMenuCreateButtonControl %s failed\n", name);
			return -1;
		}

		break;
	}

	return 0;
}

static bool
rmEndsWith(const char *s, const char *suffix)
{
	const char *p = strstr(s, suffix);

	return p && !*(p + strlen(suffix));
}

/* Initialize the single player menu */
void *
RmRaceSelectInit(void *prevMenu)
{
	// Create screen, load menu XML descriptor and create static controls.
	RmRaceSelectMenuHandle = GfuiScreenCreate((float*)NULL,
											NULL, rmOnActivate,
											NULL, (tfuiCallback)NULL,
											1);
#if defined(CLIENT_SERVER)
	void *h = GfuiMenuLoad("csraceselectmenu.xml");
#else
	void *h = GfuiMenuLoad("raceselectmenu.xml");
#endif

	GfuiMenuCreateStaticControls(RmRaceSelectMenuHandle, h);

	const char *section = GFMNU_SECT_DYNAMIC_CONTROLS;
	int n = GfParmGetEltNb(h, section);

	if (n < 0)
		std::runtime_error("GfParmGetEltNb failed");
	else if (GfParmListSeekFirst(h, section))
		std::runtime_error("GfParmListSeekFirst failed");

	for (int i = 0; i < n; i++)
	{
		const char *name = GfParmListGetCurEltName(h, section);
		int res;

		if (!name)
			std::runtime_error("GfParmListGetCurEltName failed");
		else if (rmEndsWith(name, "Button") && rmCreateRaceType(h, name))
			std::runtime_error("rmCreateRaceType failed");
		else if (rmEndsWith(name, "Combo") && rmCreateRaceSubtype(h, name))
			std::runtime_error("rmCreateRaceSubtype failed");
		else if ((res = GfParmListSeekNext(h, section)) < 0)
			std::runtime_error("GfParmListSeekNext failed");
		else if (res > 0)
			break;
	}

	// Create Back button
	GfuiMenuCreateButtonControl(RmRaceSelectMenuHandle, h, "BackButton",
								prevMenu, rmOnRaceSelectShutdown);

	// Close menu XML descriptor.
	GfParmReleaseHandle(h);

	// Register keyboard shortcuts.
	GfuiMenuDefaultKeysAdd(RmRaceSelectMenuHandle);
	GfuiAddKey(RmRaceSelectMenuHandle, GFUIK_ESCAPE, "Back To Main Menu",
			   prevMenu, rmOnRaceSelectShutdown, NULL);

	// Give the race engine the menu to come back to.
	LmRaceEngine().initializeState(RmRaceSelectMenuHandle);

	return RmRaceSelectMenuHandle;
}
