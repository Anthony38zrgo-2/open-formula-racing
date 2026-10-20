/***************************************************************************
                  driverselect.cpp -- drivers interactive selection
                             -------------------
    created              : Mon Aug 16 20:40:44 CEST 1999
    copyright            : (C) 1999 by Eric Espie, 2009 Jean-Philippe Meuret,
						   2024 Xavier Del Campo Romero
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
    		Driver selection menu.
    @ingroup	racemantools
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
*/


#include <algorithm>
#include <cerrno>
#include <cstdlib>

#include <tgfclient.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robot.h>

#include <cars.h>
#include <drivers.h>
#include <racemanagers.h>
#include <race.h>

#include "racescreens.h"
#include "garagemenu.h"


// Uncomment to re-activate focus managment (what for ?)
//#define FOCUS on

// Handle for the screen.
static void* ScrHandle;

// Information about the current race type.
static tRmDriverSelect* MenuData;

// GUI control Ids.
static int		CandidatesLabelId;
static int		CompetitorsLabelId;
static int		CompetitorsScrollListId, CandidatesScrollListId;
static int		CandidateCarId;
static int		SelectButtonId, DeselectButtonId;
static int		RemoveAllButtonId, SelectRandomButtonId, ShuffleButtonId;
static int		MoveUpButtonId, MoveDownButtonId;
static int		CarCategoryEditId;
static int		DriverTypeEditId;
static int		SkinEditId;
static int		SkinLeftButtonId, SkinRightButtonId;
static int		CarImageId;
#ifdef FOCUS
static int		FocusedDriverLabelId;
#endif
static int		CurrentDriverTypeLabelId;
static int		CurrentDriverCarLabelId;
static int		CurrentDriverCarCategoryLabelId;
static int		NumberofDriversToGenId;

static int      NextButtonId;
static int      GenerateButtonId;
static int      DeleteButtonId;
static int      ChangeCarButtonId;

// The car selection menu.
static RmGarageMenu GarageMenu;

// Car categories
static const char* AnyCarCategory = "--- All car categories ---";
static std::vector<std::string> VecCarCategoryIds; // Category folder/file names
static std::vector<std::string> VecCarCategoryNames; // Category real/displayed names
static size_t CurCarCategoryIndex = 0;

// Driver types
static const char* AnyDriverType = "--- All driver types ---";
static std::vector<std::string> VecDriverTypes;
static size_t CurDriverTypeIndex = 0;

// Car models
static const char* AnyCarModel = "--- All car models ---";
static std::vector<std::string> VecCarModels;
static size_t CurCarModelIdx = 0;

// Skin names, targets and associated preview files for the currently selected driver.
static std::vector<GfDriverSkin> VecCurDriverPossSkins;
static size_t CurSkinIndex = 0;

// The current driver
// (the last one the user clicked on, shown as highligthed in one of the scroll-lists).
GfDriver* PCurrentDriver;

// Local functions.
static void rmdsFilterCandidatesScrollList(const std::string& strCarCatId,
										   const std::string& strType,
										   const std::string& strCarModel);
static void rmdsClickOnDriver(void * /* dummy */);

static void
rmdsUpdateLabels()
{
	int ncm = GfuiScrollListGetNumberOfElements(ScrHandle, CompetitorsScrollListId),
		nca = GfuiScrollListGetNumberOfElements(ScrHandle, CandidatesScrollListId);
	char cmlabel[sizeof "COMPETITORS (18446744073709551615)"],
		calabel[sizeof "CANDIDATES (18446744073709551615)"];

	snprintf(cmlabel, sizeof cmlabel, "COMPETITORS (%d)", ncm);
	snprintf(calabel, sizeof calabel, "CANDIDATES (%d)", nca);
	GfuiLabelSetText(ScrHandle, CompetitorsLabelId, cmlabel);
	GfuiLabelSetText(ScrHandle, CandidatesLabelId, calabel);
}

static bool
rmdsIsAnyCompetitorHighlighted()
{
	GfDriver *pDriver;

    const char* name =
		GfuiScrollListGetSelectedElement(ScrHandle, CompetitorsScrollListId, (void**)&pDriver);

	return name != 0;
}

static void
rmdsHighlightDriver(const GfDriver* pDriver)
{
	if (!pDriver)
		return;

	// Search first in the competitors scroll-list.
	GfDriver* pCompetitor;
	int index = 0;
	while (GfuiScrollListGetElement(ScrHandle, CompetitorsScrollListId, index, (void**)&pCompetitor))
	{
		if (pCompetitor == pDriver)
		{
			//GfLogDebug("Highlighting competitor #%d '%s'\n",
			//		   curDrvIndex, pCompetitor->getName().c_str());
			GfuiScrollListSetSelectedElement(ScrHandle, CompetitorsScrollListId, index);
			return;
		}
		index++;
	}

	// Then in the candidates scroll-list.
	GfDriver* pCandidate;
	index = 0;
	while (GfuiScrollListGetElement(ScrHandle, CandidatesScrollListId, index, (void**)&pCandidate))
	{
		if (pCandidate == pDriver)
		{
			//GfLogDebug("Highlighting candidate #%d '%s'\n", curDrvIndex, name);
			GfuiScrollListSetSelectedElement(ScrHandle, CandidatesScrollListId, index);
			return;
		}
		index++;
	}
}

static void
rmdsReloadCompetitorsScrollList()
{
	GfuiScrollListClear(ScrHandle, CompetitorsScrollListId);

	// For each competitor in the race :
	std::vector<GfDriver*> vecCompetitors = MenuData->pRace->getCompetitors();
	std::vector<GfDriver*>::iterator itComp;
	for (itComp = vecCompetitors.begin(); itComp != vecCompetitors.end(); ++itComp)
		// Add its name to the Competitors scroll list.
		GfuiScrollListInsertElement(ScrHandle, CompetitorsScrollListId, (*itComp)->getName().c_str(),
									MenuData->pRace->getCompetitorsCount(), (void*)(*itComp));

	// Disable selection when max nb of competitors reached or no more candidates.
	const bool bAcceptsMore = MenuData->pRace->acceptsMoreCompetitors();
	const int nCandidates =
		GfuiScrollListGetNumberOfElements(ScrHandle, CandidatesScrollListId);
	GfuiEnable(ScrHandle, SelectButtonId,
			   nCandidates > 0 && bAcceptsMore ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, SelectRandomButtonId,
			   nCandidates > 0 && bAcceptsMore ? GFUI_ENABLE : GFUI_DISABLE);
	rmdsUpdateLabels();
}

static bool
rmdsCanGenerateDriverType()
{
	const std::string &driver = VecDriverTypes[CurDriverTypeIndex];
	static const char *exc[] =
	{
		"human", "networkhuman"
	};

	for (size_t i = 0; i < sizeof exc / sizeof *exc; i++)
		if (driver == exc[i])
			return false;

	return true;
}

static void
rmdsUpdateGenerate()
{
	bool driver_enable = rmdsCanGenerateDriverType(),
		car_enable = VecCarCategoryNames[CurCarCategoryIndex] != AnyCarCategory;
	int enable = driver_enable && car_enable ? GFUI_ENABLE : GFUI_DISABLE;

	GfuiEnable(ScrHandle, GenerateButtonId, enable);
}

static void
rmdsRefreshCarModels()
{
	const std::string &sel = VecCarCategoryIds[CurCarCategoryIndex],
		&catId = sel == AnyCarCategory ? "" : sel;
	std::vector<GfCar *> cars = GfCars::self()->getCarsInCategory(catId);

	VecCarModels.clear();
	VecCarModels.push_back(AnyCarModel);
	GfuiComboboxClear(ScrHandle, CandidateCarId);
	GfuiComboboxAddText(ScrHandle, CandidateCarId, AnyCarModel);

	for (const GfCar *c : cars) {
		VecCarModels.push_back(c->getId());
		GfuiComboboxAddText(ScrHandle, CandidateCarId, c->getName().c_str());
	}

	GfuiComboboxSetSelectedIndex(ScrHandle, CandidateCarId, CurCarModelIdx);
}

// Screen activation call-back.
static void
rmdsActivate(void * /* not used */)
{
	GfLogTrace("Entering Driver Select menu\n");

	// Fill-in the competitors scroll-list.
	rmdsReloadCompetitorsScrollList();

	// Initialize the currently highlighted competitor (and scroll the list if needed to show him)
	// (the 1st human driver, or else of the 1st driver).
	PCurrentDriver = 0;
	std::vector<GfDriver*> vecCompetitors = MenuData->pRace->getCompetitors();
	std::vector<GfDriver*>::iterator itComp;
	for (itComp = vecCompetitors.begin(); itComp != vecCompetitors.end(); ++itComp)
	{
		if ((*itComp)->isHuman())
		{
			PCurrentDriver = *itComp;
			break;
		}
	}
	if (!PCurrentDriver && !vecCompetitors.empty())
	{
		itComp = vecCompetitors.begin();
		PCurrentDriver = *itComp;
	}

	if (PCurrentDriver)
		GfuiScrollListShowElement(ScrHandle, CompetitorsScrollListId,
								  itComp - vecCompetitors.begin());

	// Update GUI : current driver info, car preview.
	rmdsHighlightDriver(PCurrentDriver);
	rmdsClickOnDriver(0);

    // Initialize the driver type filter criteria to "any driver" if possible.
	// or else the first available type.
	const std::vector<std::string>::const_iterator itDrvTyp =
		std::find(VecDriverTypes.begin(), VecDriverTypes.end(), AnyDriverType);
	if (itDrvTyp == VecDriverTypes.end())
		CurDriverTypeIndex = 0;
	else
		CurDriverTypeIndex = itDrvTyp - VecDriverTypes.begin();

	// Initialize the car category filter criteria :
	// use the one of the current driver if any, or else "any category" if possible,
	// or else the first available category.
	const std::string strCarCatId =
		PCurrentDriver ? PCurrentDriver->getCar()->getCategoryId() : AnyCarCategory;
	const std::vector<std::string>::const_iterator itCarCat =
		std::find(VecCarCategoryIds.begin(), VecCarCategoryIds.end(), strCarCatId);
	if (itCarCat == VecCarCategoryIds.end())
		CurCarCategoryIndex = 0;
	else
		CurCarCategoryIndex = itCarCat - VecCarCategoryIds.begin();

	// Initialize the car model filter criteria :
	// use the one of the current driver if any, or else "any category" if possible,
	// or else the first available model.
	const std::string strCarModelId =
		PCurrentDriver ? PCurrentDriver->getCar()->getName() : AnyCarModel;
	const std::vector<std::string>::const_iterator itCarModel =
		std::find(VecCarModels.begin(), VecCarModels.end(), strCarModelId);
	if (itCarModel == VecCarModels.end())
		CurCarModelIdx = 0;
	else
		CurCarModelIdx = itCarModel - VecCarModels.begin();

	// Update GUI (candidate list, filter criteria).
	rmdsRefreshCarModels();
    GfuiLabelSetText(ScrHandle, DriverTypeEditId, VecDriverTypes[CurDriverTypeIndex].c_str());
    GfuiLabelSetText(ScrHandle, CarCategoryEditId, VecCarCategoryNames[CurCarCategoryIndex].c_str());
    rmdsFilterCandidatesScrollList(VecCarCategoryIds[CurCarCategoryIndex],
								   VecDriverTypes[CurDriverTypeIndex],
								   VecCarModels[CurCarModelIdx]);
	rmdsUpdateGenerate();
	rmdsUpdateLabels();
}

static void
rmdsCleanup(void)
{
    VecCarCategoryIds.clear();
    VecCarCategoryNames.clear();
    VecDriverTypes.clear();
    VecCurDriverPossSkins.clear();
}

// Screen de-activation call-back.
static void
rmdsDeactivate(void *nextScreenHdle)
{
    rmdsCleanup();
    GfuiScreenRelease(ScrHandle);

    if (nextScreenHdle)
		GfuiScreenActivate(nextScreenHdle);
}

static void
rmdsChangeCarCategory(int dir)
{
	CurCarCategoryIndex =
		(CurCarCategoryIndex + VecCarCategoryIds.size() + dir) % VecCarCategoryIds.size();

    GfuiLabelSetText(ScrHandle, CarCategoryEditId, VecCarCategoryNames[CurCarCategoryIndex].c_str());

    rmdsFilterCandidatesScrollList(VecCarCategoryIds[CurCarCategoryIndex],
								   VecDriverTypes[CurDriverTypeIndex],
								   VecCarModels[CurCarModelIdx]);

	if (rmdsIsAnyCompetitorHighlighted())
		GfuiEnable(ScrHandle, ChangeCarButtonId, GFUI_ENABLE);

	rmdsUpdateGenerate();
	CurCarModelIdx = 0;
	rmdsRefreshCarModels();
	rmdsUpdateLabels();
}

static void
rmdsChangeCarCategoryLeft(void *)
{
	rmdsChangeCarCategory(-1);
}

static void
rmdsChangeCarCategoryRight(void *)
{
	rmdsChangeCarCategory(1);
}

static void
rmdsChangeDriverType(int dir)
{
 	CurDriverTypeIndex =
		(CurDriverTypeIndex + VecDriverTypes.size() + dir) % VecDriverTypes.size();

    GfuiLabelSetText(ScrHandle, DriverTypeEditId, VecDriverTypes[CurDriverTypeIndex].c_str());

    rmdsFilterCandidatesScrollList(VecCarCategoryIds[CurCarCategoryIndex],
								   VecDriverTypes[CurDriverTypeIndex],
								   VecCarModels[CurCarModelIdx]);

	if (rmdsIsAnyCompetitorHighlighted())
		GfuiEnable(ScrHandle, ChangeCarButtonId, GFUI_ENABLE);

	rmdsUpdateGenerate();
	CurCarModelIdx = 0;
	rmdsRefreshCarModels();
	rmdsUpdateLabels();
}

static void
rmdsChangeDriverTypeLeft(void *)
{
	rmdsChangeDriverType(-1);
}

static void
rmdsChangeDriverTypeRight(void *)
{
	rmdsChangeDriverType(1);
}

static void
rmdsChangeSkin(int dir)
{
	if (VecCurDriverPossSkins.empty())
	{
		GfuiLabelSetText(ScrHandle, SkinEditId, "no choice");
		GfuiStaticImageSet(ScrHandle, CarImageId, "data/img/nocarpreview.png");
		return;
	}

	// Update skin combo-box.
 	CurSkinIndex = (CurSkinIndex + VecCurDriverPossSkins.size()
					+ dir) % VecCurDriverPossSkins.size();

	const GfDriverSkin& curSkin = VecCurDriverPossSkins[CurSkinIndex];
	std::string strCurSkinDispName =
		curSkin.getName().empty() ? "standard " : curSkin.getName();
	//#736: display skin name starting with capital letter
	strCurSkinDispName[0] = toupper(strCurSkinDispName[0]);
	GfuiLabelSetText(ScrHandle, SkinEditId, strCurSkinDispName.c_str());

	const std::string &filename = curSkin.getCarPreviewFileName();
	std::string localname = std::string(GfLocalDir()) + filename;

	// Load associated preview image (or "no preview" panel if none available).
	if (GfFileExists(localname.c_str()))
		GfuiStaticImageSet(ScrHandle, CarImageId, localname.c_str());
	else if (GfFileExists(filename.c_str()))
		GfuiStaticImageSet(ScrHandle, CarImageId, filename.c_str());
	else
		GfuiStaticImageSet(ScrHandle, CarImageId, "data/img/nocarpreview.png");

	// Update highlighted driver skin.
	if (PCurrentDriver)
		PCurrentDriver->setSkin(curSkin); // TODO: Can we do this for the whole game session (not only this race if it ever starts) ?
}

static void
rmdsChangeSkinLeft(void *)
{
	rmdsChangeSkin(-1);
}

static void
rmdsChangeSkinRight(void *)
{
	rmdsChangeSkin(1);
}

static void
rmdsNextMenu(void * /* dummy */)
{
	// Finally, go back to the caller menu.
    rmdsDeactivate(MenuData->nextScreen);
}

static void
rmdsPreviousMenu(void *screen)
{
	// Go back to the caller menu without any change to the starting grid.
	rmdsDeactivate(screen);
}

static void
rmdsGarageMenu(void *pPreviousMenu)
{
	if (PCurrentDriver)
	{
		GarageMenu.setPreviousMenuHandle(pPreviousMenu);
		GarageMenu.runMenu(MenuData->pRace, PCurrentDriver);
	}
}

static void
rmdsMoveCompetitor(int dir)
{
	if (PCurrentDriver)
	{
		// Move the competitor in the scroll-list.
		GfuiScrollListMoveSelectedElement(ScrHandle, CompetitorsScrollListId, dir);

		// Move the competitor in the race.
		MenuData->pRace->moveCompetitor(PCurrentDriver, dir);

		// Disable moveUp/Down according to the position of the selected competitor.
		const unsigned nCompetitors = MenuData->pRace->getCompetitorsCount();
		const int nSelCompInd =
			GfuiScrollListGetSelectedElementIndex(ScrHandle, CompetitorsScrollListId);
		GfuiEnable(ScrHandle, MoveUpButtonId,
				   nSelCompInd > 0 ? GFUI_ENABLE : GFUI_DISABLE);
		GfuiEnable(ScrHandle, MoveDownButtonId,
				   nSelCompInd < (int)nCompetitors -1 ? GFUI_ENABLE : GFUI_DISABLE);
	}
}

static void
rmdsMoveCompetitorUp(void *)
{
	rmdsMoveCompetitor(-1);
}

static void
rmdsMoveCompetitorDown(void *)
{
	rmdsMoveCompetitor(1);
}

static void
rmdsClickOnDriver(void * /* dummy */)
{
	// Determine which list and which driver were clicked on.
	GfDriver* pDriver = 0;
    if (GfuiScrollListGetSelectedElement(ScrHandle, CompetitorsScrollListId, (void**)&pDriver))
	{
		// A driver from the Competitors scroll-list.
		GfuiEnable(ScrHandle, SelectButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, DeselectButtonId, GFUI_ENABLE);
		GfuiEnable(ScrHandle, DeleteButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, ChangeCarButtonId, GFUI_ENABLE);
		GfuiVisibilitySet(ScrHandle, SkinEditId, GFUI_VISIBLE);
	}
    else if (GfuiScrollListGetSelectedElement(ScrHandle, CandidatesScrollListId, (void**)&pDriver))
	{
		int enable = pDriver->isHuman() ? GFUI_DISABLE : GFUI_ENABLE;

		// A driver from the Candidates scroll-list.
		GfuiEnable(ScrHandle, SelectButtonId,
				   MenuData->pRace->acceptsMoreCompetitors() ? GFUI_ENABLE : GFUI_DISABLE);
		GfuiEnable(ScrHandle, DeselectButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, ChangeCarButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, DeleteButtonId, enable);
		GfuiVisibilitySet(ScrHandle, SkinEditId, GFUI_VISIBLE);
	}
    else
	{
		// No driver from any scroll-list.
		GfuiEnable(ScrHandle, SelectButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, DeselectButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, ChangeCarButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, DeleteButtonId, GFUI_DISABLE);
		GfuiVisibilitySet(ScrHandle, SkinEditId, GFUI_INVISIBLE);
		GfuiStaticImageSet(ScrHandle, CarImageId, "data/img/nocarpreview.png");
	}

    // Update selected driver (if any) displayed infos.
    if (pDriver)
	{
		//GfLogDebug("rmdsClickOnDriver: '%s'\n", pDriver->getName().c_str());

		// The selected driver is the new current one.
		PCurrentDriver = pDriver;

		// Update current driver info (but don't show car information if Career mode,
		// no choice, and we don't know yet which car actually)
		GfuiLabelSetText(ScrHandle, CurrentDriverTypeLabelId, pDriver->getType().c_str());
		const GfCar* pCar = pDriver->getCar();
		GfuiLabelSetText(ScrHandle, CurrentDriverCarLabelId, pCar->getName().c_str());
		GfuiLabelSetText(ScrHandle, CurrentDriverCarCategoryLabelId, pCar->getCategoryId().c_str());

		// Get really available skins (the user may have changed some file somewhere
		// since last time we got here for this driver).
		VecCurDriverPossSkins = pDriver->getPossibleSkins();

		// Determine the index of the currently selected skin for this driver.
		CurSkinIndex = 0;
		std::vector<GfDriverSkin>::iterator itSkin =
			GfDriver::findSkin(VecCurDriverPossSkins, pDriver->getSkin().getName());
		if (itSkin != VecCurDriverPossSkins.end())
			CurSkinIndex = itSkin - VecCurDriverPossSkins.begin();

		const int skinButtonsEnabled =
			VecCurDriverPossSkins.size() > 1 ? GFUI_ENABLE : GFUI_DISABLE;
		GfuiEnable(ScrHandle, SkinRightButtonId, skinButtonsEnabled);
		GfuiEnable(ScrHandle, SkinLeftButtonId, skinButtonsEnabled);

		// Update driver skin and show it in the GUI.
		rmdsChangeSkin(0);
    }

	// Disable random selection if max competitors reached or no more candidates.
	const bool bAcceptsMore = MenuData->pRace->acceptsMoreCompetitors();
	const int nCandidates =
		GfuiScrollListGetNumberOfElements(ScrHandle, CandidatesScrollListId);
	GfuiEnable(ScrHandle, SelectRandomButtonId,
			   nCandidates > 0 && bAcceptsMore ? GFUI_ENABLE : GFUI_DISABLE);

	// Disable removeAll if no more competitors.
	const unsigned nCompetitors = MenuData->pRace->getCompetitorsCount();
	GfuiEnable(ScrHandle, RemoveAllButtonId, nCompetitors > 0 ? GFUI_ENABLE : GFUI_DISABLE);

	// Disable shuffle if at most 1 competitor
	GfuiEnable(ScrHandle, ShuffleButtonId, nCompetitors > 1 ? GFUI_ENABLE : GFUI_DISABLE);

	// Disable moveUp/Down according to the position of the selected competitor.
	const int nSelCompInd =
		GfuiScrollListGetSelectedElementIndex(ScrHandle, CompetitorsScrollListId);
	GfuiEnable(ScrHandle, MoveUpButtonId,
			   nSelCompInd > 0 ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, MoveDownButtonId,
			   nSelCompInd >= 0 && nSelCompInd < (int)nCompetitors - 1 ? GFUI_ENABLE : GFUI_DISABLE);

// TODO - need to determine if this IS a network game and if it is Server OR Spectator
//      - allow 0 Competitors
#ifndef CLIENT_SERVER
    // Don't allow user to Accept 0 drivers, this would cause a crash.
    GfuiEnable(ScrHandle, NextButtonId, nCompetitors > 0 ? GFUI_ENABLE : GFUI_DISABLE);
#endif
}

static void
rmdsSelectDeselectDriver(void * /* dummy */ )
{
    const char* name;
    int	src, dst;
    GfDriver *pDriver = 0;
    bool bSelect;

    // If the selected driver is in the Candidate scroll-list,
    // and if the max number of selected drivers has not been reached,
    // remove the driver from the Candidate scroll-list,
	// and add him to the Competitors scroll-list and to the race competitors.
    bSelect = false;
    name = 0;
    if (MenuData->pRace->acceptsMoreCompetitors())
	{
		src = CandidatesScrollListId;
		name = GfuiScrollListExtractSelectedElement(ScrHandle, src, (void**)&pDriver);
		if (name)
		{
			dst = CompetitorsScrollListId;
			GfuiScrollListInsertElement(ScrHandle, dst, name, GfDrivers::self()->getCount(), (void*)pDriver);
			// Change human car to an accepted category if this not the case, and if possible.
			if (pDriver->isHuman()
				&& !MenuData->pRace->acceptsCarCategory(pDriver->getCar()->getCategoryId()))
			{
				const std::vector<std::string>& vecAccCatIds =
					MenuData->pRace->getAcceptedCarCategoryIds();
				if (vecAccCatIds.size() > 0)
				{
					const GfCar* pNewCar = GfCars::self()->getCarsInCategory(vecAccCatIds[0])[0];
					if (pNewCar)
					{
						const GfCar* pOldCar = pDriver->getCar();
						pDriver->setCar(pNewCar);
						GfLogTrace("Changing %s car to %s (%s category was not accepted)\n",
								   pDriver->getName().c_str(), pNewCar->getName().c_str(),
								   pOldCar->getName().c_str());
					}
				}
			}

			MenuData->pRace->appendCompetitor(pDriver); // => Now selected.
		}
    }

    // Otherwise, if the selected driver is in the Competitors scroll-list,
    // remove the driver from the Competitors scroll-list and from the race competitors,
	// and add him to the Candidate scroll-list
    // (if it matches the Candidate scroll-list filtering criteria,
	//  see rmdsFilterCandidatesScrollList for how humans enjoy a different filtering).
    if (!name)
	{
		bSelect = true;
		src = CompetitorsScrollListId;
		name = GfuiScrollListExtractSelectedElement(ScrHandle, src, (void**)&pDriver);
		if (!name)
			return; // Should never happen.

		const std::string strCarCatIdFilter =
			(pDriver->isHuman() || VecCarCategoryIds[CurCarCategoryIndex] == AnyCarCategory
			 ? "" : VecCarCategoryIds[CurCarCategoryIndex]);
		const std::string strTypeFilter =
			(VecDriverTypes[CurDriverTypeIndex] == AnyDriverType
			 ? "" : VecDriverTypes[CurDriverTypeIndex]);
		if (pDriver->matchesTypeAndCategory(strTypeFilter, strCarCatIdFilter))
		{
			dst = CandidatesScrollListId;
			GfuiScrollListInsertElement(ScrHandle, dst, name,
										pDriver->isHuman() ? 0 : GfDrivers::self()->getCount(), (void*)pDriver);
		}
		MenuData->pRace->removeCompetitor(pDriver); // => No more selected.

		// If no more competitor, select the 1st candidate if any.
		if (MenuData->pRace->getCompetitorsCount() == 0)
		{
			GfuiScrollListSetSelectedElement(ScrHandle, CandidatesScrollListId, 0);
			(void)GfuiScrollListGetSelectedElement(ScrHandle, CandidatesScrollListId, (void**)&pDriver);
		}
    }

    // Focused driver management (inhibited for the moment : what is it useful for ?)
	const GfDriver* pFocDriver = MenuData->pRace->getFocusedCompetitor();
    if (bSelect)
	{
		if (MenuData->pRace->isCompetitorFocused(pDriver))
		{
			/* the focused element was deselected : select a new one */
			name = GfuiScrollListGetElement(ScrHandle, CompetitorsScrollListId, 0, (void**)&pDriver);
			if (name)
			{
				MenuData->pRace->setFocusedCompetitor(pDriver);
#ifdef FOCUS
				GfuiLabelSetText(ScrHandle, FocusedDriverLabelId, pDriver->getName.c_str());
#endif
			}
			else
			{
				MenuData->pRace->setFocusedCompetitor(0);
#ifdef FOCUS
				GfuiLabelSetText(ScrHandle, FocusedDriverLabelId, "");
#endif
			}
		}
    }
	else
	{
		if (pDriver && (!pFocDriver || pDriver->isHuman()))
		{
			MenuData->pRace->setFocusedCompetitor(pDriver);
#ifdef FOCUS
			GfuiLabelSetText(ScrHandle, FocusedDriverLabelId, pDriver->getName().c_str());
#endif
		}
    }

    // Update selected driver displayed info.
    rmdsClickOnDriver(0);
	rmdsUpdateLabels();

	// For a smart display refresh, when automatically called multiple time.
    GfuiDisplay();
}

static void
rmdsRemoveAllCompetitors(void * /* dummy */ )
{
	// Take care that no candidate is selected (see why in rmdsSelectDeselectDriver).
	GfuiScrollListClearSelection(ScrHandle, CandidatesScrollListId);

	// Deselect the first competitors until there's none left.
	int nCompetitors;
	while ((nCompetitors = GfuiScrollListGetNumberOfElements(ScrHandle, CompetitorsScrollListId)) > 0)
	{
		// Select the firstd element of the Competitors scroll-list.
		GfuiScrollListSetSelectedElement(ScrHandle, CompetitorsScrollListId, 0);

		// Do the normal deselection job (just as if the user had manually done it).
		rmdsSelectDeselectDriver(0);
	}

	rmdsUpdateLabels();
}

static void
rmdsSelectRandomCandidates(void * /* dummy */ )
{
	// Max number of randomly selected candidates.
	static const unsigned nRandomCompetitors = 5;

	// Take care that no competitor is selected (see why in rmdsSelectDeselectDriver).
	GfuiScrollListClearSelection(ScrHandle, CompetitorsScrollListId);

	// Select as many random candidates as possible.
	unsigned nCount = 1;
	int nCandidates;
	while (nCount <= nRandomCompetitors
		   && MenuData->pRace->acceptsMoreCompetitors()
		   && (nCandidates = GfuiScrollListGetNumberOfElements(ScrHandle, CandidatesScrollListId)) > 0)
	{
		// Pick-up a random candidate from the candidate scroll-list.
		const unsigned nPickedCandInd = rand() % nCandidates;

		// Make it the selected element of the Candidate scroll-list.
		GfuiScrollListSetSelectedElement(ScrHandle, CandidatesScrollListId, nPickedCandInd);

		// Do the normal selection job (just as if the user had manually done it).
		rmdsSelectDeselectDriver(0);

		// Next random candidate.
		nCount++;
	}

	rmdsUpdateLabels();
}

static void
rmdsShuffleCompetitors(void * /* dummy */ )
{
	// Shuffle the race competitor list and reload the scroll-list.
	MenuData->pRace->shuffleCompetitors();
	rmdsReloadCompetitorsScrollList();

    // Re-highlight the previously highlighted driver if any
	// (in case it was in the competitors list).
	rmdsHighlightDriver(PCurrentDriver);
}

static void
rmdsChangeCarModel(tComboBoxInfo *info)
{
	CurCarModelIdx = info->nPos;

	const std::string &catId = VecCarCategoryIds[CurCarCategoryIndex],
		&driver = VecDriverTypes[CurDriverTypeIndex],
		&model = VecCarModels[CurCarModelIdx];

	rmdsFilterCandidatesScrollList(catId, driver, model);
	// Re-highlight the previously highlighted driver if any
	// (in case it was in the competitors list).
	rmdsHighlightDriver(PCurrentDriver);
}

static void
rmdsAddKeys(void)
{
    // Add standard keyboard shortcuts.
    GfuiMenuDefaultKeysAdd(ScrHandle);

    GfuiAddKey(ScrHandle, GFUIK_ESCAPE, "Previous menu", MenuData->prevScreen, rmdsPreviousMenu, NULL);
    GfuiAddKey(ScrHandle, GFUIK_RETURN, "Next menu", NULL, rmdsNextMenu, NULL);
    GfuiAddKey(ScrHandle, '-', "Move Up", NULL, rmdsMoveCompetitorUp, NULL);
    GfuiAddKey(ScrHandle, '+', "Move Down", NULL, rmdsMoveCompetitorDown, NULL);
    GfuiAddKey(ScrHandle, ' ', "Select/Deselect", NULL, rmdsSelectDeselectDriver, NULL);
#ifdef FOCUS
    GfuiAddKey(ScrHandle, 'f', "Set Focus", NULL, rmdsSetFocus, NULL);
#endif
}

static void
rmdsRefreshLists()
{
	// rmdsActivate() would reassign the indexes.
	size_t drv_i = CurDriverTypeIndex, cat_i = CurCarCategoryIndex,
		model_i = CurCarModelIdx;

	GfuiScrollListClear(ScrHandle, CandidatesScrollListId);
	GfuiScrollListClear(ScrHandle, CompetitorsScrollListId);
	MenuData->pRace->removeAllCompetitors();
	rmdsRemoveAllCompetitors(nullptr);
	MenuData->pRace->load(MenuData->pRace->getManager(), true);
	rmdsActivate(nullptr);
	CurDriverTypeIndex = drv_i;
	CurCarCategoryIndex = cat_i;
	CurCarModelIdx = model_i;

	const std::string &category = VecCarCategoryNames[CurCarCategoryIndex],
		&catId = VecCarCategoryIds[CurCarCategoryIndex],
		&driver = VecDriverTypes[CurDriverTypeIndex],
		&model = VecCarModels[CurCarModelIdx];

	GfuiLabelSetText(ScrHandle, DriverTypeEditId, driver.c_str());
	GfuiLabelSetText(ScrHandle, CarCategoryEditId, category.c_str());
	rmdsRefreshCarModels();
    rmdsFilterCandidatesScrollList(catId, driver, model);
	rmdsReloadCompetitorsScrollList();
	rmdsUpdateGenerate();
}

static void
rmdsGenerate(void *)
{
	const std::string &catId = VecCarCategoryIds[CurCarCategoryIndex],
		&driver = VecDriverTypes[CurDriverTypeIndex],
		&seldriver = driver == AnyDriverType ? "" : driver,
		&model = VecCarModels[CurCarModelIdx],
		&selcar = model == AnyCarModel ? "" : model;
	GfDrivers *drivers = GfDrivers::self();

	MenuData->pRace->store();

	const char *numstr = GfuiComboboxGetText(ScrHandle, NumberofDriversToGenId);

	if (!numstr)
	{
		GfLogError("Failed to extract number of drivers to generate\n");
		return;
	}

	unsigned long num;
	char *end;

	errno = 0;
	num = ::strtoul(numstr, &end, 10);

	if (errno || *end)
	{
		GfLogError("Invalid number of drivers to generate: %s\n", numstr);
		return;
	}

	for (unsigned long i = 0; i < num; i++)
		if (drivers->gen(seldriver, catId, selcar))
		{
			const char *d = seldriver.empty() ? "random" : seldriver.c_str();

			GfLogError("Failed to generate driver %lu with driver type "
				"\"%s\" and category \"%s\"\n", i, d, catId.c_str());
			return;
		}

	if (drivers->reload())
	{
		GfLogError("Failed to reload drivers\n");
		return;
	}

	rmdsRefreshLists();
}

static void
rmdsDelete(void *)
{
	void *dv;

	if (!GfuiScrollListGetSelectedElement(ScrHandle, CandidatesScrollListId, &dv))
		return;

	const GfDriver *d = static_cast<GfDriver *>(dv);
	const std::string &name = d->getName();
	GfDrivers *drivers = GfDrivers::self();

	MenuData->pRace->store();

	if (drivers->del(d->getModuleName(), name))
		GfLogError("Failed to delete driver: %s\n", name.c_str());
	else if (drivers->reload())
		GfLogError("Failed to reload drivers\n");

	rmdsRefreshLists();
}

static void
rmdsSetupNumDriverGen(int id)
{
	GfuiComboboxClear(ScrHandle, id);

	for (int i = 1; i <= 10; i++) {
		char s[sizeof "00"];

		snprintf(s, sizeof s, "%d", i);
		GfuiComboboxAddText(ScrHandle, id, s);
	}
}

/** Interactive Drivers list selection
    @param	vs	Pointer on tRmDriverSelect structure (casted to void*)
    @warning	The race manager params are modified but not saved.
 */
void
RmDriversSelect(void *vs)
{
    // Initialize drivers selection
    MenuData = (tRmDriverSelect*)vs;

    // Create screen, background image and title
    ScrHandle = GfuiScreenCreate((float*)NULL, NULL, rmdsActivate, NULL, (tfuiCallback)NULL, 1);

    void *menuDescHdle = GfuiMenuLoad("driverselectmenu.xml");
    GfuiMenuCreateStaticControls(ScrHandle, menuDescHdle);

    CandidatesLabelId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "candidateslabel");
    CompetitorsLabelId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "competitorslabel");
    CompetitorsScrollListId = GfuiMenuCreateScrollListControl(ScrHandle, menuDescHdle, "competitorsscrolllist", NULL, rmdsClickOnDriver);
    CandidatesScrollListId = GfuiMenuCreateScrollListControl(ScrHandle, menuDescHdle, "candidatesscrolllist", NULL, rmdsClickOnDriver);

    // Car category filtering "combobox" (left arrow, label, right arrow)
	const int nCatPrevButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "carcategoryleftarrow",
							NULL, rmdsChangeCarCategoryLeft);
	const int nCatNextButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "carcategoryrightarrow",
							NULL, rmdsChangeCarCategoryRight);
    CarCategoryEditId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "carcategorytext");

    // Driver type filtering "combobox" (left arrow, label, right arrow)
	const int nDrvTypPrevButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "drivertypeleftarrow",
							NULL, rmdsChangeDriverTypeLeft);
	const int nDrvTypNextButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "drivertyperightarrow",
							NULL, rmdsChangeDriverTypeRight);
    DriverTypeEditId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "drivertypetext");

    // Scroll-lists manipulation buttons
	MoveUpButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "moveupbutton", NULL, rmdsMoveCompetitorUp);
	MoveDownButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "movedownbutton", NULL, rmdsMoveCompetitorDown);

    SelectButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "selectbutton", 0, rmdsSelectDeselectDriver);
    DeselectButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "deselectbutton", 0, rmdsSelectDeselectDriver);
    RemoveAllButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "removeallbutton", 0, rmdsRemoveAllCompetitors);
    SelectRandomButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "selectrandombutton", 0, rmdsSelectRandomCandidates);
    ShuffleButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "shufflebutton", 0, rmdsShuffleCompetitors);

	CandidateCarId =
		GfuiMenuCreateComboboxControl(ScrHandle, menuDescHdle, "cartypecb", 0, rmdsChangeCarModel);

	NumberofDriversToGenId =
		GfuiMenuCreateComboboxControl(ScrHandle, menuDescHdle, "numdrivergen", 0, NULL);

	rmdsSetupNumDriverGen(NumberofDriversToGenId);

    // Skin selection "combobox" (left arrow, label, right arrow)
    SkinLeftButtonId = GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "skinleftarrow", NULL, rmdsChangeSkinLeft);
    SkinRightButtonId = GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "skinrightarrow", NULL, rmdsChangeSkinRight);
    SkinEditId = GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "skintext");
	GfuiEnable(ScrHandle, SkinRightButtonId, GFUI_DISABLE);
	GfuiEnable(ScrHandle, SkinLeftButtonId, GFUI_DISABLE);

    // Car preview image
    CarImageId = GfuiMenuCreateStaticImageControl(ScrHandle, menuDescHdle, "carpreviewimage");
    GfuiStaticImageSet(ScrHandle, CarImageId, "data/img/nocarpreview.png");

	// Initialize the car category Ids and names for the driver filter system.
	for (unsigned nCatInd = 0; nCatInd < GfCars::self()->getCategoryIds().size(); nCatInd++)
	{
		// Keep only accepted categories.
		if (MenuData->pRace->acceptsCarCategory(GfCars::self()->getCategoryIds()[nCatInd]))
		{
			VecCarCategoryIds.push_back(GfCars::self()->getCategoryIds()[nCatInd]);
			VecCarCategoryNames.push_back(GfCars::self()->getCategoryNames()[nCatInd]);
			//GfLogDebug("Accepted cat : %s\n", GfCars::self()->getCategoryIds()[nCatInd].c_str());
		}
	}

	if (VecCarCategoryIds.size() > 1)
	{
		VecCarCategoryIds.push_back(AnyCarCategory);
		VecCarCategoryNames.push_back(AnyCarCategory);
	}
	else
	{
		GfuiEnable(ScrHandle, nCatPrevButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, nCatNextButtonId, GFUI_DISABLE);
	}

	// Initialize the driver types lists for the driver filter system.
	std::vector<std::string>::const_iterator itDrvType = GfDrivers::self()->getTypes().begin();
	while (itDrvType != GfDrivers::self()->getTypes().end())
	{
		// Keep only accepted types.
		if (MenuData->pRace->acceptsDriverType(*itDrvType))
		{
			VecDriverTypes.push_back(*itDrvType);
			//GfLogDebug("Accepted type : %s\n", itDrvType->c_str());
		}
		++itDrvType;
	}
	if (VecDriverTypes.size() > 1)
		VecDriverTypes.push_back(AnyDriverType);
	else
	{
		GfuiEnable(ScrHandle, nDrvTypPrevButtonId, GFUI_DISABLE);
		GfuiEnable(ScrHandle, nDrvTypNextButtonId, GFUI_DISABLE);
	}

    // Current Driver Info
    CurrentDriverTypeLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "currentdrivertypelabel");
    CurrentDriverCarCategoryLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "currentdrivercarcategorylabel");
    CurrentDriverCarLabelId =
		GfuiMenuCreateLabelControl(ScrHandle, menuDescHdle, "currentdrivercarlabel");

    // Next, Back and Change Car buttons
    NextButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "nextmenubutton", NULL, rmdsNextMenu);
	GenerateButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "generatebutton", NULL, rmdsGenerate);
	DeleteButtonId =
		GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "deletebutton", NULL, rmdsDelete);
    GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "backbutton",
								MenuData->prevScreen, rmdsPreviousMenu);
    ChangeCarButtonId = GfuiMenuCreateButtonControl(ScrHandle, menuDescHdle, "carselectbutton",
											ScrHandle, rmdsGarageMenu);
	GfuiEnable(ScrHandle, ChangeCarButtonId, GFUI_DISABLE);

    GfParmReleaseHandle(menuDescHdle);

    // Keyboard shortcuts
    GfuiMenuDefaultKeysAdd(ScrHandle);
    rmdsAddKeys();

	// Display the menu.
    GfuiScreenActivate(ScrHandle);
}


static void
rmdsFilterCandidatesScrollList(const std::string& strCarCatId,
	const std::string& strType, const std::string& strCarModel)
{
	// Empty the candidates scroll-list
    GfuiScrollListClear(ScrHandle, CandidatesScrollListId);

    // Fill it with drivers that match the filter criteria,
	// are accepted by the race and are not already among competitors.
	// Note: We must check this race acceptance because strCarCatId and strType may be empty,
	//       and in these cases getDriversWithTypeAndCategory(...) filters less.
	// a) Retrieve the list of competitors.
	const std::vector<GfDriver*>& vecCompetitors = MenuData->pRace->getCompetitors();

	// b) Tweak the filtering criteria :
	//    - use "" for AnyCarCategory,
	//    - use "" for AnyDriverType,
	//    - when strType is "*human", set strCarCatId to AnyCarCategory
	//      (because humans can change their car for the race, and so they have
	//       to be in the candidate list even if their current car is not in the right category).
	const std::string strCarCatIdFilter =
		(strCarCatId == AnyCarCategory || strType.find("human") != strType.npos)
		? "" : strCarCatId;
	const std::string strTypeFilter = strType == AnyDriverType ? "" : strType;

	// c) Retrieve the list of drivers matching with the criteria.
	const std::vector<GfDriver*> vecCandidates =
		GfDrivers::self()->getDriversWithTypeAndCategory(strTypeFilter, strCarCatIdFilter);

	//GfLogDebug("rmdsFilterCandidatesScrollList('%s' => '%s', '%s' => '%s')\n",
	//		   strCarCatId.c_str(), strCarCatIdFilter.c_str(),
	//		   strType.c_str(), strTypeFilter.c_str());

	// d) Keep only drivers accepted by the race and not already among competitors
	//    (but don't reject humans with the wrong car category : they must be able to change it).
	std::vector<GfDriver*>::const_iterator itCandidate;
	for (itCandidate = vecCandidates.begin(); itCandidate != vecCandidates.end(); ++itCandidate)
	{
		if (std::find(vecCompetitors.begin(), vecCompetitors.end(), *itCandidate)
			== vecCompetitors.end()
			&& MenuData->pRace->acceptsDriverType((*itCandidate)->getType())
			&& (strCarModel == AnyCarModel
				|| (*itCandidate)->getCar()->getId() == strCarModel)
			&& ((*itCandidate)->isHuman()
				|| MenuData->pRace->acceptsCarCategory((*itCandidate)->getCar()->getCategoryId())))
			GfuiScrollListInsertElement(ScrHandle, CandidatesScrollListId,
										(*itCandidate)->getName().c_str(), // Item string
										itCandidate - vecCandidates.begin() + 1, // Insert. index
										(void*)(*itCandidate));
    }

    // Show first element of the list if any.
    GfuiScrollListShowElement(ScrHandle, CandidatesScrollListId, 0);

	// Select it if the Competitors scroll list has no element selected.
	if (GfuiScrollListGetSelectedElementIndex(ScrHandle, CompetitorsScrollListId) < 0)
		GfuiScrollListSetSelectedElement(ScrHandle, CandidatesScrollListId, 0);

	// Disable selection if max nb of competitors reached or no more candidates.
	const int nCandidates =
		GfuiScrollListGetNumberOfElements(ScrHandle, CandidatesScrollListId);
	const bool bAcceptsMore = MenuData->pRace->acceptsMoreCompetitors();
	GfuiEnable(ScrHandle, SelectButtonId,
			   nCandidates > 0 && bAcceptsMore ? GFUI_ENABLE : GFUI_DISABLE);
	GfuiEnable(ScrHandle, SelectRandomButtonId,
			   nCandidates > 0 && bAcceptsMore ? GFUI_ENABLE : GFUI_DISABLE);
	rmdsUpdateLabels();
}
