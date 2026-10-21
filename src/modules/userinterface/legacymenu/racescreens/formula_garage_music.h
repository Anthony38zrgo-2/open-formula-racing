#ifndef OPEN_FORMULA_GARAGE_MUSIC_H
#define OPEN_FORMULA_GARAGE_MUSIC_H
#include <SDL.h>
#include <SDL_mixer.h>
#include <string>

// Separate chunk channel: menu music transitions cannot abruptly halt its fade.
class FormulaGarageMusic
{
public:
    bool start(const std::string& path, float volumePercent);
    void update(float volumePercent);
    void stop();
    void shutdown();
    int playbackChannel() const { return channel; }
    bool playing() const;
private:
    Mix_Chunk* track = nullptr;
    int channel = -1;
    bool requested = false;
    bool ownsMixer = false;
    bool ownsAudioSubsystem = false;
};
#endif
