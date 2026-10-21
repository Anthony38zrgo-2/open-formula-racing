#include "formula_garage_music.h"
#include <tgf.h>
#include <algorithm>
#include <cmath>

namespace { constexpr int fadeMilliseconds = 800; }
bool FormulaGarageMusic::playing() const
{
    return track && channel >= 0 && Mix_Playing(channel) && Mix_GetChunk(channel) == track;
}
bool FormulaGarageMusic::start(const std::string& path, float volume)
{
    if (!track)
    {
        int frequency, channels; Uint16 format;
        if (!Mix_QuerySpec(&frequency, &format, &channels))
        {
            ownsAudioSubsystem = !SDL_WasInit(SDL_INIT_AUDIO);
            if (ownsAudioSubsystem && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
            if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0)
            { shutdown(); return false; }
            ownsMixer = true;
        }
        track = Mix_LoadWAV(path.c_str());
        if (!track)
        {
            GfLogWarning("Garage music: cannot decode %s: %s\n", path.c_str(), Mix_GetError());
            shutdown(); return false;
        }
        GfLogInfo("Garage music: loaded %s, decoded %u bytes\n", path.c_str(), track->alen);
    }
    requested = true;
    update(volume);
    return playing();
}
void FormulaGarageMusic::update(float volume)
{
    if (!track) return;
    if (!std::isfinite(volume)) volume = 0;
    Mix_VolumeChunk(track, (int)std::round(std::max(0.f, std::min(100.f, volume)) * MIX_MAX_VOLUME / 100.f));
    if (!requested || playing()) return;
    // A quick exit/re-entry finishes the outgoing fade first. Never restart
    // or jump the gain of an in-progress fade on the same channel.
    channel = Mix_FadeInChannel(-1, track, -1, fadeMilliseconds);
    if (channel < 0)
    {
        const int count = Mix_AllocateChannels(-1);
        if (Mix_AllocateChannels(count + 1) > count)
            channel = Mix_FadeInChannel(count, track, -1, fadeMilliseconds);
    }
    if (channel < 0) requested = false;
}
void FormulaGarageMusic::stop()
{
    requested = false;
    if (playing() && Mix_FadingChannel(channel) != MIX_FADING_OUT)
        Mix_FadeOutChannel(channel, fadeMilliseconds);
}
void FormulaGarageMusic::shutdown()
{
    requested = false;
    if (playing())
    {
        // Only final session teardown waits; regular exits fade asynchronously
        // while PIT/driving continues. Avoid freeing a chunk still in use.
        Mix_FadeOutChannel(channel, 150);
        const Uint32 started = SDL_GetTicks();
        while (playing() && SDL_GetTicks() - started < 250) SDL_Delay(5);
        if (playing()) Mix_HaltChannel(channel);
    }
    if (track) Mix_FreeChunk(track);
    track = nullptr; channel = -1;
    if (ownsMixer) Mix_CloseAudio();
    if (ownsAudioSubsystem) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    ownsMixer = ownsAudioSubsystem = false;
}
