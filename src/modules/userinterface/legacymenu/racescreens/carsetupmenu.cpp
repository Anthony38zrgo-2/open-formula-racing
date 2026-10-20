/***************************************************************************

    file                 : carsetupmenu.cpp
    created              : April 2020
    copyright            : (C) 2020 Robert Reif
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


/*
This file deals with car setup
*/

#include <portability.h>
#include <translation.h>

#include <sstream>
#include <iomanip>

#include <race.h>
#include <car.h>
#include <cars.h>
#include <tracks.h>
#include <drivers.h>
#include <playerpref.h>

#include "carsetupmenu.h"

int currentFocusedIndex=0;

static const char *const sets[] =
{
    "NONE", "SOFT", "MEDIUM", "HARD", "WET", "EXTREME WET"
};

// callback functions

void CarSetupMenu::onActivateCallback(void *pMenu)
{
    // Get the CarSetupMenu instance.
    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu*>(pMenu);

    pCarSetupMenu->onActivate();
}

void CarSetupMenu::onAcceptCallback(void *pMenu)
{
    // Get the CarSetupMenu instance.
    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu*>(pMenu);

    pCarSetupMenu->onAccept();
}

void CarSetupMenu::onNextCallback(void *pMenu)
{
    // Get the CarSetupMenu instance from call-back user data.
    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu*>(pMenu);

    pCarSetupMenu->onNext();
}

void CarSetupMenu::onCancelCallback(void *pMenu)
{
    // Get the CarSetupMenu instance from call-back user data.
    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu*>(pMenu);

    pCarSetupMenu->onCancel();
}

void CarSetupMenu::onResetCallback(void *pMenu)
{
    // Get the CarSetupMenu instance from call-back user data.
    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu*>(pMenu);

    pCarSetupMenu->onReset();
}

void CarSetupMenu::onPreviousCallback(void *pMenu)
{
    // Get the CarSetupMenu instance from call-back user data.
    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu*>(pMenu);

    pCarSetupMenu->onPrevious();
}

void CarSetupMenu::onComboCallback(tComboBoxInfo *pInfo)
{
    // Get the CarSetupMenu instance from call-back user data.
    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu::ComboCallbackData *>(pInfo->userData)->menu;

    pCarSetupMenu->onCombo(pInfo);
}
void CarSetupMenu::onPlusCallback(void *pInfo)
{
    // Get the CarSetupMenu instance from call-back user data.
    ComboCallbackData* pInfoDer = static_cast<ComboCallbackData*>(pInfo);

    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu::ComboCallbackData *>(pInfoDer)->menu;
    int ctrlIndex = (pInfoDer)->index;
    pCarSetupMenu->onPlus(ctrlIndex);
}
void CarSetupMenu::onMinusCallback(void *pInfo)
{
    // Get the CarSetupMenu instance from call-back user data.
    ComboCallbackData* pInfoDer = static_cast<ComboCallbackData*>(pInfo);

    CarSetupMenu *pCarSetupMenu = static_cast<CarSetupMenu::ComboCallbackData *>(pInfoDer)->menu;
    int ctrlIndex = (pInfoDer)->index;
    pCarSetupMenu->onMinus(ctrlIndex);
}
/*
void CarSetupMenu::onIncDecButtonFocusCallback(void *pIndex)
{
    GfLogInfo("Focused Index %i \n", pIndex);
	currentFocusedIndex = (int)(long)pIndex;
}
*/

// member functions

void CarSetupMenu::onActivate()
{
    GfLogInfo("Entering Car Setup menu\n");

    // Load settings from XML file.
    loadSettings();

    // Initialize GUI from loaded values.
    updateControls();
}

void CarSetupMenu::onAccept()
{
    // Get the current page values.
    readCurrentPage();

    // Save all page values.
    storeSettings();

    // Switch back to garage menu.
    GfuiScreenActivate(getPreviousMenuHandle());
}

void CarSetupMenu::onCancel() const
{
    // Back to previous screen.
    GfuiScreenActivate(getPreviousMenuHandle());
}

void CarSetupMenu::onReset()
{
    // Reset all values on current page to their defaults.
    for (size_t index = 0; index < ITEMS_PER_PAGE; index++)
    {
        attribute &att = items[currentPage][index];
        if (att.type == "edit")
        {
            att.value = att.defaultValue;
        }
        else if (att.type == "combo")
        {
            att.strValue = att.defaultStrValue;
        }
    }

    // Update the GUI.
    updateControls();
}

void CarSetupMenu::onPrevious()
{
    // Check if first page.
    if (currentPage == 0)
       return;

    // Get the current page values.
    readCurrentPage();

    // Switch to previous page.
    currentPage--;

    // Update the GUI.
    updateControls();
}

void CarSetupMenu::onNext()
{
    // Check if last page.
    if (currentPage == (items.size() - 1))
       return;

    // Get the current page values.
    readCurrentPage();

    // Switch to next page.
    currentPage++;

    // update the GUI.
    updateControls();
}

void CarSetupMenu::onCombo(tComboBoxInfo *pInfo)
{
    ComboCallbackData *pData = static_cast<CarSetupMenu::ComboCallbackData *>(pInfo->userData);

    // Use currentPage and index in callback data to find item.
    attribute &att = items[currentPage][pData->index];

    att.strValue = pInfo->vecChoices[pInfo->nPos];

    if (att.section == "Tires Set")
        for (unsigned i = 0; i < sizeof sets / sizeof *sets; i++)
            if (sets[i] == att.strValue)
            {
                att.value = i;
                break;
            }
}

void CarSetupMenu::onPlus(int ctrlIndex)
{
	attribute &att = items[currentPage][ctrlIndex];
	if (att.type == "edit")
	{
		//increase the attribute value by it's precision, dont exceed max value
		att.value = att.value+att.precision;
		if (att.value > att.maxValue){
			att.value = att.maxValue;
		}
	}
    // Update the GUI.
    updateControls();
}

void CarSetupMenu::onMinus(int ctrlIndex)
{
	attribute &att = items[currentPage][ctrlIndex];
	if (att.type == "edit")
	{
		//decrease the attribute value by it's precision, dont exceed min value
		att.value = att.value-att.precision;
		if (att.value < att.minValue){
			att.value = att.minValue;
		}
	}
    // Update the GUI.
    updateControls();
}

void CarSetupMenu::readCurrentPage()
{
    for (size_t index = 0; index < ITEMS_PER_PAGE; index++)
    {
        attribute &att = items[currentPage][index];
        if (att.exists && att.type == "edit")
        {
            std::string strValue(GfuiEditboxGetString(getMenuHandle(), att.editId));
            std::istringstream issValue(strValue);
            issValue >> att.value;
        }
    }
}

void CarSetupMenu::updateControls()
{
    GfuiEnable(getMenuHandle(), getDynamicControlId("PreviousButton"), currentPage != 0 ? GFUI_ENABLE : GFUI_DISABLE);
    GfuiEnable(getMenuHandle(), getDynamicControlId("NextButton"), currentPage < (items.size() - 1) ? GFUI_ENABLE : GFUI_DISABLE);

    for (size_t index = 0; index < ITEMS_PER_PAGE; ++index)
    {
        attribute &att = items[currentPage][index];

        // Set label text.
        std::ostringstream ossLabel;
        if (!att.label.empty())
        {
            ossLabel << att.label;
            if (!att.units.empty())
                ossLabel << " (" << att.units << ")";
            ossLabel << ":";
        }

        GfuiLabelSetText(getMenuHandle(),
                         att.labelId,
                         ossLabel.str().c_str());

        ossLabel.str("");
        ossLabel.clear();

        // Set default label text.
        if (att.exists)
        {
            if (att.type == "edit")
            {
                // Check for missing min and max.
                if (att.minValue == att.maxValue)
                {
                    ossLabel << std::fixed << std::setprecision(att.precision)
                             << "Default: " << att.defaultValue;
                }
                else
                {
                    ossLabel << std::fixed << std::setprecision(att.precision)
                             << "Min: " << att.minValue
                             << "  Default: " << att.defaultValue
                             << "  Max: " << att.maxValue;
                }
            }
            else if (att.type == "combo")
            {
                ossLabel << "Default: " << att.defaultStrValue;
            }
        }
        GfuiLabelSetText(getMenuHandle(),
                         att.defaultLabelId,
                         ossLabel.str().c_str());

        // Update the edit or combo boxes.
        if (att.label.empty())
        {
            GfuiVisibilitySet(getMenuHandle(), att.editId, GFUI_INVISIBLE);
            GfuiVisibilitySet(getMenuHandle(), att.buttonPlusId, GFUI_INVISIBLE);
            GfuiVisibilitySet(getMenuHandle(), att.buttonMinusId, GFUI_INVISIBLE);
            GfuiVisibilitySet(getMenuHandle(), att.comboId, GFUI_INVISIBLE);
        }
        else
        {
            if (att.type == "edit")
            {
                GfuiVisibilitySet(getMenuHandle(), att.editId, GFUI_VISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.buttonPlusId, GFUI_VISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.buttonMinusId, GFUI_VISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.comboId, GFUI_INVISIBLE);
            }
            else if (att.type == "combo")
            {
                GfuiVisibilitySet(getMenuHandle(), att.editId, GFUI_INVISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.buttonPlusId, GFUI_INVISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.buttonMinusId, GFUI_INVISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.comboId, GFUI_VISIBLE);
            }
            else
            {
                GfuiVisibilitySet(getMenuHandle(), att.editId, GFUI_INVISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.buttonPlusId, GFUI_INVISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.buttonMinusId, GFUI_INVISIBLE);
                GfuiVisibilitySet(getMenuHandle(), att.comboId, GFUI_INVISIBLE);
            }

            if (att.exists)
            {
                if (att.type == "edit")
                {
                    std::ostringstream ossValue;
                    ossValue << std::fixed << std::setprecision(att.precision) << att.value;
                    GfuiEditboxSetString(getMenuHandle(), att.editId, ossValue.str().c_str());

                    if (att.minValue == att.maxValue)
                    {
                        GfuiEnable(getMenuHandle(), att.editId, GFUI_DISABLE);
                        GfuiEnable(getMenuHandle(), att.buttonPlusId, GFUI_DISABLE);
                        GfuiEnable(getMenuHandle(), att.buttonMinusId, GFUI_DISABLE);
                    }
                    else
                    {
                        GfuiEnable(getMenuHandle(), att.editId, GFUI_ENABLE);
                        GfuiEnable(getMenuHandle(), att.buttonPlusId, GFUI_ENABLE);
                        GfuiEnable(getMenuHandle(), att.buttonMinusId, GFUI_ENABLE);
                    }
                }
                else if (att.type == "combo")
                {
                    GfuiComboboxClear(getMenuHandle(), att.comboId);
                    size_t selected = 0;
                    for (size_t i = 0; i < att.in.size(); ++i)
                    {
                        GfuiComboboxAddText(getMenuHandle(), att.comboId, att.in[i].c_str());
                        if (att.in[i] == att.strValue)
                            selected = i;
                    }
                    GfuiComboboxSetSelectedIndex(getMenuHandle(), att.comboId, selected);
                }
            }
            else
            {
                if (att.type == "edit")
                {
                    GfuiEditboxSetString(getMenuHandle(), att.editId, "----");
                    GfuiEnable(getMenuHandle(), att.editId, GFUI_DISABLE);
					GfuiEnable(getMenuHandle(), att.buttonPlusId, GFUI_DISABLE);
					GfuiEnable(getMenuHandle(), att.buttonMinusId, GFUI_DISABLE);
                }
                else if (att.type == "combo")
                {
                    GfuiComboboxClear(getMenuHandle(), att.comboId);
                }
            }
        }
    }
}

void CarSetupMenu::loadSettings()
{
    GfuiLabelSetText(getMenuHandle(),
                     getDynamicControlId("CarNameLabel"),
                     getCar()->getName().c_str());

    // Open the XML file of the car.
    std::ostringstream ossCarFileName;
    std::string strCarId = getCar()->getId();
    ossCarFileName << "cars/models/" << strCarId << '/' << strCarId << PARAMEXT;
    void *hparmCar = GfParmReadFileBoth(ossCarFileName.str(), GFPARM_RMODE_STD);
    if (!hparmCar)
    {
        GfLogError("Car %s (file %s not %s)\n",
                   getCar()->getName().c_str(), ossCarFileName.str().c_str(),
                   GfFileExists(ossCarFileName.str().c_str()) ? "readable" : "found");
        return;
    }

    GfLogInfo("Opened car file: %s\n", ossCarFileName.str().c_str());

    // Open the XML file of the car setup.
    std::ostringstream ossCarSetupFileName;
    std::string strTrackId = getTrack()->getId();
    ossCarSetupFileName << GfLocalDir() << "drivers/human/cars/" << strCarId << '/' << strTrackId << PARAMEXT;
    void *hparmCarSetup = GfParmReadFile(ossCarSetupFileName.str(), GFPARM_RMODE_STD);
    if (!hparmCarSetup)
    {
        GfLogInfo("Car Setup: %s/%s (file %s not %s)\n",
                   getCar()->getName().c_str(), getTrack()->getName().c_str(), ossCarSetupFileName.str().c_str(),
                   GfFileExists(ossCarSetupFileName.str().c_str()) ? "readable" : "found");
    }
    else
        GfLogInfo("Opened car setup file: %s\n", ossCarSetupFileName.str().c_str());

    void *hparmItems = GfuiMenuLoad("carsetupmenuitems.xml");
    if (!hparmItems)
    {
        GfLogError("Car Setup Items (file %s not %s)\n",
                   "carsetupmenuitems.xml",
                   GfFileExists("carsetupmenuitems.xml") ? "readable" : "found");
    }
    else
    {
        GfLogInfo("Opened car setup menu items file: %s\n", "carsetupmenuitems.xml");

        std::vector<std::string> sections = GfParmListGetSectionNamesList(hparmItems);

        for (size_t i = 0; i < sections.size(); ++i)
        {
            std::string strSection = sections[i];

            GfLogDebug("section %zu: %s\n", i, strSection.c_str());

            size_t page = GfParmGetNum(hparmItems, strSection.c_str(), "page", "", 0);
            size_t index = GfParmGetNum(hparmItems, strSection.c_str(), "index", "", 0);

            if (page <= items.size())
                items.resize(page + 1);

            if (index >= ITEMS_PER_PAGE)
            {
                GfLogError("Invalid index %zu\n", index);
                continue;
            }

            attribute &att = items[page][index];
            std::string strIndex(std::to_string(index));

            att.labelId = getDynamicControlId(std::string("Label" + strIndex).c_str());
            att.editId = getDynamicControlId(std::string("Edit" + strIndex).c_str());
            att.comboId = getDynamicControlId(std::string("Combo" + strIndex).c_str());
            att.defaultLabelId = getDynamicControlId(std::string("DefaultLabel" + strIndex).c_str());
            att.buttonPlusId = getDynamicControlId(std::string("EditButtonPlus" + strIndex).c_str());
            att.buttonMinusId = getDynamicControlId(std::string("EditButtonMinus" + strIndex).c_str());
            att.type = GfParmGetStr(hparmItems, strSection.c_str(), "type", "");
            att.section = GfParmGetStr(hparmItems, strSection.c_str(), "section", "");
            att.param = GfParmGetStr(hparmItems, strSection.c_str(), "param", "");
            att.label = GfuiGetTranslatedText(hparmItems, strSection.c_str(), "label", "");

            if (att.type == "edit" || att.section == "Tires Set")
            {
                att.units = GfParmGetStr(hparmItems, strSection.c_str(), "unit", "");
                att.precision = GfParmGetNum(hparmItems, strSection.c_str(), "precision", "", 0);

                // Read values from car.
                att.exists = GfParmGetNumWithLimits(hparmCar, att.section.c_str(), att.param.c_str(), att.units.c_str(),
                                                    &att.defaultValue, &att.minValue, &att.maxValue) == 0;

                // Read value from car setup if avaliable.
                if (hparmCarSetup)
                    att.value = GfParmGetNum(hparmCarSetup, att.section.c_str(), att.param.c_str(),
                                             att.units.c_str(), att.defaultValue);
                else
                    att.value = att.defaultValue;

                GfLogDebug("section: \"%s\" param: \"%s\" units: \"%s\" label: \"%s\" page: %zu "
                           "index: %zu precision: %d labelId: %d editId: %d defaultLabelId: %d "
                           "exists: %d min: %f default %f max: %f value: %f\n",
                           att.section.c_str(), att.param.c_str(), att.units.c_str(),
                           att.label.c_str(), page, index, att.precision, att.labelId, att.editId,
                           att.defaultLabelId, att.exists, att.minValue, att.defaultValue, att.maxValue, att.value);

                if (att.section == "Tires Set")
                {
                    static const size_t sz = sizeof sets / sizeof *sets;

                    for (unsigned i = att.minValue; i <= att.maxValue
                        && i < sz; i++)
                        att.in.push_back(sets[i]);

                    unsigned defvalue = att.defaultValue, value = att.value;

                    if (value < sz)
                        att.strValue = sets[value];

                    if (att.defaultValue < sz)
                        att.defaultStrValue = sets[defvalue];
                }
            }
            else if (att.type == "combo")
            {
                att.defaultStrValue = GfParmGetStr(hparmCar, att.section.c_str(), att.param.c_str(), "");
                att.exists = !att.defaultStrValue.empty();
                att.in = GfParmGetStrIn(hparmCar, att.section.c_str(), att.param.c_str());

                if (hparmCarSetup)
                    att.strValue = GfParmGetStr(hparmCarSetup, att.section.c_str(), att.param.c_str(),
                                                att.defaultStrValue.c_str());
                else
                    att.strValue = att.defaultStrValue;

                GfLogDebug("section: \"%s\" param: \"%s\" label: \"%s\" page: %zu "
                           "index: %zu labelId: %d comboId: %d defaultLabelId: %d "
                           "exists: %d, in: %zu default: \"%s\" value: \"%s\"\n",
                           att.section.c_str(), att.param.c_str(), att.label.c_str(),
                           page, index, att.labelId, att.comboId, att.defaultLabelId,
                           att.exists, att.in.size(), att.defaultStrValue.c_str(),
                           att.strValue.c_str());
            }
        }

        // Save the control id for all items.
        for (size_t page = 0; page < items.size(); ++page)
        {
            for (size_t index = 0; index < ITEMS_PER_PAGE; ++index)
            {
                attribute &att = items[page][index];
                std::string strIndex(std::to_string(index));

                if (!att.labelId)
                    att.labelId = getDynamicControlId(std::string("Label" + strIndex).c_str());
                if (!att.editId)
                    att.editId = getDynamicControlId(std::string("Edit" + strIndex).c_str());
                if (!att.comboId)
                    att.comboId = getDynamicControlId(std::string("Combo" + strIndex).c_str());
                if (!att.defaultLabelId)
                    att.defaultLabelId = getDynamicControlId(std::string("DefaultLabel" + strIndex).c_str());
                if (!att.buttonPlusId)
                    att.buttonPlusId = getDynamicControlId(std::string("EditButtonPlus" + strIndex).c_str());
                if (!att.buttonMinusId)
                    att.buttonMinusId = getDynamicControlId(std::string("EditButtonMinus" + strIndex).c_str());
            }
        }

        // Close the XML file of the menu items.
        GfParmReleaseHandle(hparmItems);
    }

    // Close the XML file of the car.
    GfParmReleaseHandle(hparmCar);

    // Close the XML file of the car setup.
    if (hparmCarSetup)
        GfParmReleaseHandle(hparmCarSetup);
}

// Save car setup to XML file.
void CarSetupMenu::storeSettings()
{
    // Open the XML file of the car setup.
    std::ostringstream ossCarSetupFileName;
    std::string strCarId = getCar()->getId();
    std::string strTrackId = getTrack()->getId();
    ossCarSetupFileName << GfLocalDir() << "drivers/human/cars/" << strCarId << '/' << strTrackId << PARAMEXT;
    void *hparmCarSetup = GfParmReadFile(ossCarSetupFileName.str(), GFPARM_RMODE_STD);
    if (!hparmCarSetup)
    {
        // Create the car setup file directory if it doesn't exist.
        std::string strDir = ossCarSetupFileName.str();
        strDir.resize(strDir.find_last_of('/'));
        if (!GfDirExists(strDir.c_str()))
            GfDirCreate(strDir.c_str());

        hparmCarSetup = GfParmReadFile(ossCarSetupFileName.str(),
                                       GFPARM_RMODE_STD | GFPARM_RMODE_CREAT | GFPARM_TEMPLATE);
        if (!hparmCarSetup)
        {
            GfLogError("Car Setup %s/%s (file %s not %s)\n",
                       getCar()->getName().c_str(), getTrack()->getName().c_str(),
                       ossCarSetupFileName.str().c_str(),
                       GfFileExists(ossCarSetupFileName.str().c_str()) ? "readable" : "found");
            return;
        }

        GfLogInfo("Created car setup file: %s\n", ossCarSetupFileName.str().c_str());
    }
    else
    {
        // Remove existing content.
        GfParmClean(hparmCarSetup);

        GfLogInfo("Opened car setup file: %s\n", ossCarSetupFileName.str().c_str());
    }

    // Store all items.
    for (size_t page = 0; page < items.size(); ++page)
    {
        for (size_t index = 0; index < ITEMS_PER_PAGE; ++index)
        {
            attribute &att = items[page][index];
            // Only write items that exist and have been changed.
            if (att.exists)
            {
                if (att.type == "edit" || att.section == "Tires Set")
                {
                    if (att.value != att.defaultValue)
                    {
                        GfParmSetNum(hparmCarSetup, att.section.c_str(), att.param.c_str(),
                                     att.units.c_str(), att.value, att.minValue, att.maxValue);
                    }
                }
                else if (att.type == "combo")
                {
                    if (att.strValue != att.defaultStrValue)
                    {
                        GfParmSetStrAndIn(hparmCarSetup, att.section.c_str(), att.param.c_str(),
                                          att.strValue.c_str(), att.in);
                    }
                }
            }
        }
    }

    // Write the XML file of the car setup.
    GfParmWriteFile(NULL, hparmCarSetup, strCarId.c_str());

    // Close the XML file of the car setup.
    GfParmReleaseHandle(hparmCarSetup);
}

CarSetupMenu::CarSetupMenu()
: GfuiMenuScreen("carsetupmenu.xml")
, _pRace(nullptr)
, _pDriver(nullptr)
, currentPage(0)
{
    // Initialize combo callback user data.
    for (size_t i = 0; i < ITEMS_PER_PAGE; ++i)
    {
        comboCallbackData[i].menu = this;
        comboCallbackData[i].index = i;
    }
}

bool CarSetupMenu::initialize(void *pPrevMenu, const GfRace *pRace, const GfDriver *pDriver)
{
    _pRace = pRace;
    _pDriver = pDriver;
    setPreviousMenuHandle(pPrevMenu);

    GfLogDebug("Initializing Car Setup menu: \"%s\"\n", pDriver->getCar()->getName().c_str());

    createMenu(NULL, this, onActivateCallback, NULL, (tfuiCallback)NULL, 1);

    openXMLDescriptor();

    createStaticControls();

    createLabelControl("CarNameLabel");

    // Create items.
    for (size_t index = 0; index < ITEMS_PER_PAGE; ++index)
    {
        std::string strIndex(std::to_string(index));
        createLabelControl(std::string("Label" + strIndex).c_str());
        createEditControl(std::string("Edit" + strIndex).c_str(), this, NULL, NULL);

		//add plus and minus (increase/decrease) buttons for the edit
		createButtonControl(std::string("EditButtonMinus" + strIndex).c_str(), &comboCallbackData[index], onMinusCallback);
		createButtonControl(std::string("EditButtonPlus" + strIndex).c_str(), &comboCallbackData[index], onPlusCallback);

        createComboboxControl(std::string("Combo" + strIndex).c_str(), &comboCallbackData[index], onComboCallback);
        createLabelControl(std::string("DefaultLabel" + strIndex).c_str());
    }

    // Create buttons.
    createButtonControl("ApplyButton", this, onAcceptCallback);
    createButtonControl("CancelButton", this, onCancelCallback);
    createButtonControl("ResetButton", this, onResetCallback);
    createButtonControl("PreviousButton", this, onPreviousCallback);
    createButtonControl("NextButton", this, onNextCallback);

    closeXMLDescriptor();

    // Keyboard shortcuts.
    addDefaultShortcuts();
    addShortcut(GFUIK_ESCAPE, "Cancel", this, onCancelCallback, NULL);
    addShortcut(GFUIK_RETURN, "Accept", this, onAcceptCallback, NULL);

    return true;
}
