/***************************************************************************

    file                 : raceloadingmenu.cpp
    created              : Sun Feb 25 00:34:46 2001
    copyright            : (C) 2000 by Eric Espie
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
    		The menu for when the race is running
    @ingroup	racemantools
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
*/

#include <algorithm>
#include <cctype>
#include <string>

#include <tgfclient.h>

#include "racescreens.h"
#include "loading_presentation.h"

static void* loadingScreen = 0;
static LoadingPresentation raceLoadingPresentation;
static std::string latestLoadingMessage;
static float loadingProgress = 0.12f;

static void DisplayRaceLoadingScreen()
{
    raceLoadingPresentation.draw(loadingProgress, latestLoadingMessage.c_str());
}

void RmLoadingScreenStart(const char*, const char*)
{
    if (loadingScreen && GfuiScreenIsActive(loadingScreen))
        return;
    if (loadingScreen)
        RmLoadingScreenShutdown();

    loadingScreen = GfuiScreenCreate(0, 0, 0, 0, 0, 0);
    raceLoadingPresentation.load();
    latestLoadingMessage = "Preparando la carrera";
    loadingProgress = 0.12f;
    GfuiScreenActivate(loadingScreen);
    GfuiApp().eventLoop().setRedisplayCB(DisplayRaceLoadingScreen);
    DisplayRaceLoadingScreen();
    GfuiApp().eventLoop().forceRedisplay();
}

void RmLoadingScreenShutdown()
{
    if (!loadingScreen)
        return;
    if (GfuiScreenIsActive(loadingScreen))
    {
        loadingProgress = 1.0f;
        DisplayRaceLoadingScreen();
        GfuiScreenDeactivate();
    }
    raceLoadingPresentation.release();
    GfuiScreenRelease(loadingScreen);
    loadingScreen = 0;
    latestLoadingMessage.clear();
}

void RmLoadingScreenSetText(const char* text)
{
    if (!text || !*text)
        return;
    GfLogTrace("%s\n", text);
    if (!loadingScreen)
        return;

    latestLoadingMessage = text;
    std::string normalizedMessage = latestLoadingMessage;
    std::transform(normalizedMessage.begin(), normalizedMessage.end(), normalizedMessage.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    float milestone = std::min(loadingProgress + 0.035f, 0.94f);
    if (normalizedMessage.find("loading physics") != std::string::npos)
        milestone = 0.32f;
    else if (normalizedMessage.find("loading graphics for all cars") != std::string::npos)
        milestone = 0.78f;
    else if (normalizedMessage.find("loading graphics for") != std::string::npos)
        milestone = 0.50f;
    else if (normalizedMessage.find("loading sound") != std::string::npos)
        milestone = 0.90f;
    else if (normalizedMessage.find("prestart") != std::string::npos)
        milestone = 0.97f;
    else if (normalizedMessage == "ready.")
        milestone = 1.0f;
    loadingProgress = std::max(loadingProgress, milestone);
    DisplayRaceLoadingScreen();
    GfuiApp().eventLoop().forceRedisplay();
}
