/***************************************************************************

    file                 : splash.cpp
    created              : Sat Mar 18 23:49:03 CET 2000
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

#include <SDL.h>

#include <tgfclient.h>
#include <glfeatures.h>

#include "splash.h"
#include "loading_presentation.h"


static LoadingPresentation startupLoadingPresentation;

static bool splDisplaying = false;
static bool splTimedOut = false;
static bool splBackgroundWorkDone = false;
static bool (*splBackgroundWork)(void) = 0;
static bool (*splOnClosed)(void) = 0;


/*
 * Function
 *	splashClose
 *
 * Description
 *	Close the splash screen and start main menu
 *
 * Parameters
 *	None
 *
 * Return
 *	Nothing
 *
 * Remarks
 *
 */
static void splashClose()
{
    // Don't close if background work not completed.
    if (!splBackgroundWorkDone)
        return;

    // Completed ? OK, let's close.
    splDisplaying = false;
    startupLoadingPresentation.release();

    // And do what was specified.
    if (splOnClosed)
        splOnClosed();
}

/*
 * Function
 *	splashIdle
 *
 * Description
 *	Called by main loop when nothing to do :
 *  check if splash screen must be closed and close it if so.
 *
 * Parameters
 *
 *
 * Return
 *
 *
 * Remarks
 *
 */
static void splashIdle()
{
    // Do the specified work "in the background" if not already not.
    if (!splBackgroundWorkDone && splBackgroundWork)
    {
        splBackgroundWork();

        // And now it's done, remember it.
        splBackgroundWorkDone = true;
    }

    // Close if the splash screen delay is over.
    if (splTimedOut)
        splashClose();
}

/*
 * Function
 *	splashKey
 *
 * Description
 *
 *
 * Parameters
 *
 *
 * Return
 *
 *
 * Remarks
 *
 */
static void splashKey(int /* key */, int /* modifiers */, int /* x */, int /* y */)
{
    splashClose();
}

/*
 * Function
 *	splashTimer
 *
 * Description
 *	End of splash timer callback : can't close splash screen itself,
 *  as not run under the control of the thread that created it
 *
 * Parameters
 *	None
 *
 * Return
 *	None
 *
 * Remarks
 *
 */
static void splashTimer(int /* value */)
{
    // The splash screen delay is now over.
    if (splDisplaying)
        splTimedOut = true;
}


/*
 * Function
 *	splashDisplay
 *
 * Description
 *
 *
 * Parameters
 *
 *
 * Return
 *
 *
 * Remarks
 *
 */
static void splashDisplay(void)
{
    splDisplaying = true;
    startupLoadingPresentation.draw(splBackgroundWorkDone ? 0.10f : 0.04f,
        splBackgroundWorkDone ? "Carga inicial completada" : "Iniciando Open Formula");
}

static void splashMouse(int /* b */, int s, int /* x */, int /* y */)
{
    if (s == SDL_RELEASED)
        splashClose();
}


/*
 * Function
 *	SplashScreen
 *
 * Description
 *	Display the splash screen and do some specified work in the background (fnBackWork).
 *  Then close it and call the specified fnOnClosed function :
 *  * on mouse click, or keyboard hit or 7 second time-out, if interactive,
 *  * when fnBackWork returns, if not interactive.
 *
 * Parameters
 *	fnBackWork : function to call for background work (loading stuff, ...)
 *  fnOnClosed : function to call when closing the splash screen.
 *  bInteractive : if false,
 *
 * Return
 *	true on success, false if anything bad happened.
 *
 * Remarks
 *
 */
bool SplashScreen(bool (*fnBackWork)(void), bool (*fnOnClosed)(void), bool bInteractive)
{
    splBackgroundWork = fnBackWork;
    splOnClosed = fnOnClosed;
    splTimedOut = !bInteractive;

    startupLoadingPresentation.load();

    // Prevent fnOnClosed being called (by splashClose)
    // before fnBackWork (called by splashIdle) is done.
    splBackgroundWorkDone = false;

    // Setup event loop callbacks.
    GfuiApp().eventLoop().setRedisplayCB(splashDisplay);
    if (bInteractive)
    {
        GfuiApp().eventLoop().setKeyboardDownCB(splashKey);
        GfuiApp().eventLoop().setMouseButtonCB(splashMouse);
        GfuiApp().eventLoop().setTimerCB(2000, splashTimer);
    }
    GfuiApp().eventLoop().setRecomputeCB(splashIdle);
    splashDisplay();

    return true;
}
