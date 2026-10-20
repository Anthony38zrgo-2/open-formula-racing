#ifndef __guimenusfx_h__
#define __guimenusfx_h__

/***************************************************************************

    file                 : guimenusfx.h
    created              : Mon March 28 2022
    copyright            : (C) 2022
    email                :

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


// DLL exported symbols declarator for Windows.
#ifdef _WIN32
# ifdef TGFCLIENT_DLL
#  define TGFCLIENT_API __declspec(dllexport)
# else
#  define TGFCLIENT_API __declspec(dllimport)
# endif
#else
# define TGFCLIENT_API
#endif

TGFCLIENT_API void gfuiInitMenuSfx();
TGFCLIENT_API void gfuiShutdownMenuSfx();
TGFCLIENT_API void enableMenuSfx(bool enable = true);
TGFCLIENT_API void setMenuSfxVolume(float volume = 100.0f);
TGFCLIENT_API float getMenuSfxVolume();
TGFCLIENT_API void playMenuSfx(int sfxIndex);

#define SFX_CLICK 0
#define SFX_FOCUS 1

#endif //__guimenusfx_h__
