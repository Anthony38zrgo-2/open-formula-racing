/***************************************************************************

    file                 : grsound.cpp
    created              : Thu Aug 17 23:57:10 CEST 2000
    copyright            : (C) 2000-2003 by Eric Espie, Christos Dimitrakakis
    email                : torcs@free.fr, dimitrak@idiap.ch

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under1 the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "grsound.h"

#include <sound.h>

#include <cstddef>
#include <string>
#include <vector>

#include "OpenalSoundInterface.h"
#if !defined(USE_MACPORTS)
#include "PlibSoundInterface.h"
#endif
#include "CarSoundData.h"
#include "ofp_audio_loader.h"
#include "ofp_v10_stream.h"

static int soundInitialized = 0;

static SoundInterface* sound_interface = NULL;
static CarSoundData** car_sound_data = NULL;

/* Etapa 4 V10 state (OpenAL path only; the legacy path is untouched). */
static OfpAudioLoader* v10_loader = NULL;
static std::vector<OfpV10Stream*> v10_streams;
static std::string v10_assets_root;
static bool v10_muted = false;

namespace
{
    bool v10FileExists(const std::string& path)
    {
        return GfFileExists(path.c_str());
    }

    std::string v10ResolveAssetsRoot()
    {
        const std::string candidates[] = {
            std::string(GfDataDir()) + "sounds",
            std::string(GfLocalDir()) + "data/sounds"
        };
        for (std::size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i)
        {
            if (v10FileExists(candidates[i] + "/banks/commons/bank_manifest.json")
                && v10FileExists(candidates[i] + "/banks/v10-v2-bank/manifest.json")
                && v10FileExists(candidates[i] + "/sound_mixer_config.json"))
                return candidates[i];
        }
        return std::string();
    }

    int v10ActiveCount()
    {
        int count = 0;
        for (std::size_t i = 0; i < v10_streams.size(); ++i)
        {
            if (v10_streams[i] && v10_streams[i]->isActive())
                ++count;
        }
        return count;
    }

    /* Idempotent teardown: `grInitSound` may run again for a new race without
       an intervening shutdown, so a previous backend must never leak handles,
       streams or OpenAL sources into the next session. */
    void v10Teardown()
    {
        for (std::size_t i = 0; i < v10_streams.size(); ++i)
            delete v10_streams[i];
        v10_streams.clear();
        delete v10_loader;
        v10_loader = NULL;
    }
}

enum SoundMode {DISABLED, OPENAL_MODE, PLIB_MODE};

static enum SoundMode sound_mode = OPENAL_MODE;

static const char *soundDisabledStr = SND_ATT_SOUND_STATE_DISABLED;
static const char *soundOpenALStr = SND_ATT_SOUND_STATE_OPENAL;
static const char *soundPlibStr = SND_ATT_SOUND_STATE_PLIB;

void grInitSound(tSituation* s, int ncars)
{
	char buf[256];

	// Check if we want sound (sound.xml).
	void *paramHandle = GfParmReadFileLocal(SND_PARAM_FILE, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	const char *optionName = GfParmGetStr(paramHandle, SND_SCT_SOUND, SND_ATT_SOUND_STATE, soundOpenALStr);
	float global_volume = GfParmGetNum(paramHandle, SND_SCT_SOUND, SND_ATT_SOUND_VOLUME, "%", 100.0f);
	if (!strcmp(optionName, soundDisabledStr)) {
		sound_mode = DISABLED;
	} else if (!strcmp(optionName, soundOpenALStr)) {
		sound_mode = OPENAL_MODE;
	} else if (!strcmp(optionName, soundPlibStr)) {
		sound_mode = PLIB_MODE;
	}
	GfLogInfo("Initializing sound engine (%s)\n", optionName);
	GfParmReleaseHandle(paramHandle);
	paramHandle = NULL;

	switch (sound_mode) {
	case OPENAL_MODE:
		try
		{
			sound_interface = new OpenalSoundInterface (44100, 32);
		}
		catch(...)
		{
			GfLogError("Unable to create OpenAL device, disabling sound \n");
			sound_mode = DISABLED;
			return;
		}
		break;
	case PLIB_MODE:
#if !defined(USE_MACPORTS)
		sound_interface = new PlibSoundInterface(44100, 32);
		break;
#endif
	case DISABLED:
		return;
	default:
		GfLogError("Unknown sound mode %d (%s)\n", sound_mode, optionName);
		exit(-1);
	}

	sound_interface->setGlobalGain(global_volume/100.0f);

	car_sound_data = new CarSoundData* [ncars];

	int i;
	for (i = 0; i<ncars; i++) {
		void* handle = s->cars[i]->_carHandle;
		tCarElt	*car = s->cars[i];
		const char* param;
		char filename[512];

		// ENGINE PARAMS
		tdble rpm_scale;
		param = GfParmGetStr(handle, "Sound", "engine sample", "engine-1.wav");
		rpm_scale = GfParmGetNum(handle, "Sound", "rpm scale", NULL, 1.0);
		sprintf(filename, "cars/models/%s/%.*s",
			car->_carName,
			(int)(sizeof(filename) - strlen(car->_carName) - strlen("cars/models//")),
			param);
		std::string localname = std::string(GfLocalDir()) + filename;
		if (!GfFileExists(localname.c_str()) && !GfFileExists(filename))
		{
 			sprintf(filename, "data/sound/%.*s",
				(int)(sizeof(filename) - strlen(car->_carName) - strlen("data/sound/")),
				param);
		}

		car_sound_data[car->index] = new CarSoundData (car->index, sound_interface);
		Sound* engine_sound = sound_interface->addSample(filename, ACTIVE_VOLUME | ACTIVE_PITCH | ACTIVE_LP_FILTER, true, false);
		car_sound_data[i]->setEngineSound (engine_sound, rpm_scale);

		// TURBO PARAMS
		float default_turbo_rpm = 100.0f;//0.5f*car->_enginerpmMaxTq;
		bool turbo_on;
		param = GfParmGetStr(handle, SECT_ENGINE, PRM_TURBO, "false");
		if (!strcmp(param, "true")) {
			turbo_on = true;
		} else {
			if (strcmp(param, "false")) {
				fprintf (stderr, "expected true or false, found %s\n", param);
			}
			turbo_on = false;
		}

		float turbo_rpm = GfParmGetNum(handle, SECT_ENGINE, PRM_TURBO_RPM, NULL, default_turbo_rpm);
		float turbo_lag = GfParmGetNum(handle, SECT_ENGINE, PRM_TURBO_LAG, NULL, 1.0f);
		car_sound_data[i]->setTurboParameters (turbo_on, turbo_rpm, turbo_lag);
	}

	sound_interface->setSkidSound("data/sound/skid_tyres.wav");
	sound_interface->setScrubSound("data/sound/tire_scrub.wav");
	sound_interface->setRoadRideSound("data/sound/road-ride.wav");
	sound_interface->setGrassRideSound("data/sound/grass_ride.wav");
	sound_interface->setCurbRideSound("data/sound/curb_ride.wav");
	sound_interface->setDirtRideSound("data/sound/dirt_ride.wav");
	sound_interface->setDirtSkidSound("data/sound/dirt_skid.wav");
	sound_interface->setMetalSkidSound("data/sound/skid_metal.wav");
	sound_interface->setAxleSound("data/sound/axle.wav");
	sound_interface->setTurboSound("data/sound/turbo1.wav");
	sound_interface->setBackfireLoopSound("data/sound/backfire_loop.wav");
	sound_interface->setSnowRideSound("data/sound/snow_ride.wav");

	for (i = 0; i < NB_CRASH_SOUND; i++) {
		sprintf(buf, "data/sound/crash%d.wav", i+1);
		sound_interface->setCrashSound(buf, i);
	}

	sound_interface->setBangSound("data/sound/boom.wav");
	sound_interface->setBottomCrashSound("data/sound/bottom_crash.wav");
	sound_interface->setBackfireSound("data/sound/backfire.wav");
	sound_interface->setGearChangeSound("data/sound/gear_change1.wav");

	sound_interface->setNCars(ncars);
	soundInitialized = 1;

	// Must happen after all static non-shared have been allocated.
	if (sound_mode == OPENAL_MODE)
	{
		v10_assets_root = v10ResolveAssetsRoot();
		static_cast<OpenalSoundInterface*>(sound_interface)->reserveStreamingSources(
			v10_assets_root.empty() ? 0 : (ncars < OfpV10Stream::kMaxStreams ? ncars : OfpV10Stream::kMaxStreams));
	}
	sound_interface->initSharedSourcePool();

	/* Etapa 4 V10: Williams-only backend on the OpenAL path. Every failure
	   below is fail-closed with one logged cause; the legacy mix keeps
	   running untouched (explicit reversion path, never a silent overlay).
	   The teardown first makes re-init safe: a new race must not inherit
	   handles, streams or sources from the previous one. */
	v10_muted = false;
	v10Teardown();
	if (sound_mode == OPENAL_MODE)
	{
		v10_assets_root = v10ResolveAssetsRoot();
		if (v10_assets_root.empty())
		{
			GfLogInfo("Open Formula audio: no staged banks found, V10 disabled\n");
		}
		else
		{
#ifdef _WIN32
			const std::string dll = std::string(GfLibDir())
				+ "modules/sound/open_formula_audio.dll";
#else
			const std::string dll;
#endif
			v10_loader = new OfpAudioLoader();
			if (!v10_loader->load(dll))
			{
				GfLogError("Open Formula audio: %s\n", v10_loader->lastError().c_str());
				delete v10_loader;
				v10_loader = NULL;
			}
			else
			{
				/* Slots are keyed by car index, like `car_sound_data` and the
				   engine priorities: indices are dense 0..ncars-1 throughout
				   this codebase, so slot and car agree here and in refresh. */
				v10_streams.assign((std::size_t)ncars, NULL);
				for (int i = 0; i < ncars; ++i)
				{
					/* Log id follows the car, not the slot (see refresh). */
					const int id = (s && i < s->_ncars && s->cars[i]) ? s->cars[i]->index : i;
					v10_streams[(std::size_t)i] = new OfpV10Stream(*v10_loader, id);
				}
			}
		}
	}
}


void
grShutdownSound()
{
    GfLogInfo("Shutting down sound engine\n");

    /* Etapa 4 V10 first (null-safe): a mode flip to DISABLED between races
       must still release the previous backend. */
    v10Teardown();

    if (sound_mode == DISABLED) {
		return;
    }

    if (!soundInitialized) {
		return;
    }
    soundInitialized = 0;

    delete sound_interface;

#if !defined(USE_MACPORTS)
    if (__slPendingError) {
		GfLogError("Plib: The following error was ignored: %s\n", __slPendingError);
		__slPendingError = 0; // ARG!!! ugly ugly bad thing... but should not occur anymore now
    }
#endif
}



void
grRefreshSound(tSituation *s, Camera* camera)
{
	if (sound_mode == DISABLED) {
		return;
	}

	// TODO: Fix for a lot of cars.
	// I guess in this implementation we can change the Update() call to have _ncars = 1?

	// TODO: Just consider cars near the camera, doing computations just for them?

	if (camera) {
		sndVec3 *p_camera = camera->Posv;
		sndVec3* u_camera = camera->Speedv;
		sndVec3 c_camera;
		sndVec3* a_camera = camera->Upv;
		int i;
		for (i = 0; i<3; i++) {
			c_camera[i] = (*camera->Centerv)[i] - (*camera->Posv)[i];
		}

		//sgNormaliseVec3 (c_camera);

		const float camera_vel[3] = {
			(float)(*u_camera)[0], (float)(*u_camera)[1], (float)(*u_camera)[2]
		};
		const bool v10_path = (sound_mode == OPENAL_MODE) && (v10_loader != NULL);
		for (i = 0; i < s->_ncars; i++) {
			tCarElt	*car = s->cars[i];
			bool v10 = false;
			/* Streams are keyed by car index like everything else here
			   (`car_sound_data`, engine priorities), never by loop position. */
			const int id = car ? car->index : -1;
			OfpV10Stream* stream = (v10_path && id >= 0 && id < (int)v10_streams.size())
				? v10_streams[id]
				: NULL;
			if (stream)
			{
				/* Lazy, capped activation: at most kMaxStreams mixer-owned
				   cars; the rest keep the legacy path. */
				if (stream->isActive())
				{
					v10 = true;
				}
				else if (v10ActiveCount() < OfpV10Stream::kMaxStreams)
				{
					v10 = stream->tryActivate(car, v10_assets_root);
				}
				car_sound_data[id]->setV10Active(v10);
				static_cast<OpenalSoundInterface*>(sound_interface)->setV10Active(id, v10);
			}
			car_sound_data[car->index]->setListenerPosition(*p_camera);
			car_sound_data[car->index]->update(car);
			if (v10 && stream)
			{
				const float camera_pos[3] = {
					(float)(*p_camera)[0], (float)(*p_camera)[1], (float)(*p_camera)[2]
				};
				stream->update(car, camera_pos, camera_vel, !v10_muted);
			}
		}

		sound_interface->update (car_sound_data, s->_ncars,
					 *p_camera, *u_camera, c_camera, *a_camera);
	}

}

float grGetVolume()
{
    if (sound_mode == DISABLED) {
        return 0.0f;
    }

    if (!soundInitialized) {
        return 0.0f;
    }
    return sound_interface->getVolume();
}

void grSetVolume(float vol)
{
    if (sound_mode == DISABLED) {
        return;
    }

    if (!soundInitialized) {
        return;
    }
    sound_interface->setVolume(vol);
}

void grMuteSound(bool bOn)
{
    if (sound_mode == DISABLED) {
		return;
    }

    if (!soundInitialized) {
		return;
    }

    /* Etapa 4 V10: global volume/mute apply once through the listener gain
       (owned by the OpenAL backend) plus source pause here; the mixer PCM is
       never scaled per source, so there is no double gain. */
    v10_muted = bOn;
    for (std::size_t i = 0; i < v10_streams.size(); ++i)
    {
        if (v10_streams[i])
            v10_streams[i]->setPaused(bOn);
    }

	sound_interface->mute(bOn);
}
