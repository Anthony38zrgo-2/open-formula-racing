/***************************************************************************

	file                 : guimenusfx.cpp
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

#include "guimenusfx.h"

#include <tgf.h>
#include "tgfclient.h"
#include <portability.h>
#include <sound.h>
#include <SDL_mixer.h>


static bool mixerInitialized = false;

const char *sfxDisabledStr = SND_VAL_MENUSFX_STATE_DISABLED;

bool sfxenabled = true;
int sfxVolume = MIX_MAX_VOLUME;
Mix_Chunk* MenuSfx[2];
const char* filename[2] =  {"data/menu/menu-click.ogg",
							"data/menu/menu-rollover.ogg"};
int numSfx = 2;

bool isSfxEnabled();
void loadMenuSfx();
void unloadMenuSfx();
bool initMixer();
void readSfxConfig();

bool isSfxEnabled()
{
	return sfxenabled;
}

void loadMenuSfx()
{
	for (int i = 0; i < numSfx; i++)
	{
		MenuSfx[i] = Mix_LoadWAV(filename[i]);
		if (MenuSfx[i] == NULL)
			GfLogError("Mix_LoadWAV() failed %s \n", Mix_GetError());
	}

	for (int i = 0; i < numSfx; i++)
	{
		if (MenuSfx[i])
			Mix_VolumeChunk(MenuSfx[i], sfxVolume);
	}
}

void unloadMenuSfx()
{
	for (int i = 0; i < numSfx; i++)
	{
		Mix_FreeChunk(MenuSfx[i]);
		MenuSfx[i] = NULL;
	}
}

bool initMixer()
{
	if (!mixerInitialized)
	{
		if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) >= 0 )
		{
			mixerInitialized = true;
		}
		else
		{
			GfLogError("Mix_OpenAudio() failed %s \n", Mix_GetError());
		}
	}
	return mixerInitialized;
}

void readSfxConfig()
{
	GfLogInfo("Reading Menu SFX config\n");
	void *paramHandle = GfParmReadFileLocal(SND_PARAM_FILE, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	const char *sfxEnabledstr = GfParmGetStr(paramHandle, SND_SCT_MENUSFX, SND_ATT_MENUSFX_STATE, sfxDisabledStr);

	float sfx_volume = GfParmGetNum(paramHandle, SND_SCT_MENUSFX, SND_ATT_MENUSFX_VOLUME, "%", 100.0f);

	if (sfx_volume>100.0f)
		sfx_volume = 100.0f;
	else if (sfx_volume < 0.0f)
		sfx_volume = 0.0f;

	sfxVolume = (int)(sfx_volume * MIX_MAX_VOLUME/100.0f);

	if (0 == strcmp(sfxEnabledstr, SND_VAL_MENUSFX_STATE_ENABLED))
		sfxenabled = true;
	else
		sfxenabled = false;


	GfParmReleaseHandle(paramHandle);
	paramHandle = NULL;
}

void gfuiInitMenuSfx()
{
	for (int i = 0; i < numSfx; i++)
	{
		MenuSfx[i] = NULL;
	}

	readSfxConfig();
	if (isSfxEnabled())
	{
		GfLogInfo("(Re-)Initializing Menu SFX \n");
		if(initMixer())
		{
			loadMenuSfx();
		}
	}
	else
	{
		GfLogInfo("Menu SFX is disabled \n");
	}
}

void gfuiShutdownMenuSfx()
{
	if (mixerInitialized)
	{
		unloadMenuSfx();
		mixerInitialized = false;
	}
}

void setMenuSfxVolume(float vol /* 100.0f */)
{
	if (vol>100.0f)
		vol = 100.0f;
	else if (vol < 0.0f)
		vol = 0.0f;

	sfxVolume = (int)(vol * MIX_MAX_VOLUME/100.0f);

	if (isSfxEnabled())
	{
		for(int i = 0; i < numSfx; i++)
		{
			if (MenuSfx[i])
				Mix_VolumeChunk(MenuSfx[i], sfxVolume);
		}
	}
	GfLogInfo("Menu SFX volume set to %.2f\n", vol);
}

float getMenuSfxVolume()
{
	float divisor = MIX_MAX_VOLUME/100.0f;
	return (sfxVolume / divisor);
}

void enableMenuSfx(bool enable /* true */)
{
	if (isSfxEnabled())
	{
		if(enable == false)
			gfuiShutdownMenuSfx();
	}
	else
	{
		 if(enable == true)
			 gfuiInitMenuSfx();
	}
	sfxenabled = enable;
}

void playMenuSfx(int sfxIndex)
{
	if (isSfxEnabled())
	{
		Mix_PlayChannel(-1, MenuSfx[sfxIndex], 0);
	}
}
