/***************************************************************************

    file                 : SDL2MusicPlayer.cpp
    created              : Sat June 5 2021
    copyright            : (C) 2021
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

#include <cstdio>
#include <tgf.h>
#include "sdl2musicplayer.h"

SDL2MusicPlayer::SDL2MusicPlayer(char* oggFilePath):
    music(NULL),
    fadestate(FADEIN),
    maxVolume(1.0),
    ready(false),
    started(false)

{
    if(oggFilePath)
    {
        music =  Mix_LoadMUS(oggFilePath);
    }
}

SDL2MusicPlayer::~SDL2MusicPlayer()
{
    stop();

    if(music)
    {
        Mix_FreeMusic( music );
        music = NULL;
    }
}

void SDL2MusicPlayer::stop()
{
    Mix_HaltMusic();

    if (!ready)
    {
        return;
    }

    ready = false;
}

bool SDL2MusicPlayer::isPlaying()
{
    return started;
}

void SDL2MusicPlayer::start()
{
    if(music)
    {
        Mix_FadeInMusic(music, -1, 1000);
        started = true;
    }
}

void SDL2MusicPlayer::pause()
{
        //Mix_PauseMusic();
        Mix_FadeOutMusic(1000);
}

void SDL2MusicPlayer::resume()
{
    if((Mix_PlayingMusic() == 0) || (!started))
    {
        start();
    }
    else
    {
        Mix_ResumeMusic();
    }
}
void SDL2MusicPlayer::rewind()
{
    //_stream->rewind();
}

bool SDL2MusicPlayer::startPlayback()
{
    if(isPlaying())
    {
        return true;
    }


    return true;
}

void SDL2MusicPlayer::fadeout()
{
    fadestate = FADEOUT;
}

void SDL2MusicPlayer::fadein()
{
    fadestate = FADEIN;
}

void SDL2MusicPlayer::setvolume(float volume)
{
    maxVolume = volume;
    Mix_VolumeMusic(int(maxVolume * 100));
}

float SDL2MusicPlayer::getvolume()
{
    return maxVolume;
}

void SDL2MusicPlayer::doFade()
{

}
