#ifndef __SDL2MusicPlayer_h__
#define __SDL2MusicPlayer_h__

/***************************************************************************

    file                 : SDL2MusicPlayer.h
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

#include <SDL_mixer.h>

class SDL2MusicPlayer
{
	public:
		SDL2MusicPlayer(char* oggFilePath);
		virtual ~SDL2MusicPlayer();

		virtual void start();
		virtual void stop();
		virtual void pause();
		virtual void resume();
		virtual void rewind();
		virtual void setvolume(float volume);
		virtual float getvolume();
		virtual void fadeout();
		virtual void fadein();

	protected:
		virtual bool startPlayback();
		virtual bool isPlaying();
		virtual void doFade();

		Mix_Music* music;

		typedef enum { NONE, FADEIN, FADEOUT } eFadeState;

		eFadeState fadestate;

		float maxVolume;
		bool ready;
		bool started;
};
#endif // __SDL2MusicPlayer_h__
