#define SDL_MAIN_HANDLED
#include "formula_garage_music.h"
#include <tgf.h>
#include <cstdio>
#include <cmath>
#include <atomic>
#include <algorithm>

namespace {
int checks=0, failures=0;
void check(bool pass,const char* name) { ++checks; if(!pass) { ++failures; std::printf("FAIL %s\n",name); } }
std::atomic<long long> energy{0};
void postMix(void*,Uint8* stream,int bytes) {
    const auto* pcm=reinterpret_cast<Sint16*>(stream);
    long long sum=0; for(int i=0;i<bytes/2;++i) sum+=std::abs((int)pcm[i]);
    energy.fetch_add(sum,std::memory_order_relaxed);
}
}
int main(int argc,char** argv)
{
    if(argc!=2) return 2;
    SDL_SetMainReady(); GfInit(false);
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    if(SDL_Init(SDL_INIT_AUDIO)!=0 || Mix_OpenAudio(44100,AUDIO_S16SYS,2,256)!=0) return 3;
    Mix_SetPostMix(postMix,nullptr);
    FormulaGarageMusic music;
    check(!music.playing(),"PIT starts silent");
    check(music.start(argv[1],40),"original MP3 decodes and starts");
    const int channel=music.playbackChannel();
    check(channel>=0 && Mix_FadingChannel(channel)==MIX_FADING_IN,"entry starts with fade-in");
    Mix_Chunk* chunk=Mix_GetChunk(channel);
    check(chunk && chunk->alen>40000000,"full original track decoded");
    check(Mix_VolumeChunk(chunk,-1)==51,"uses music volume, not SFX volume");
    SDL_Delay(250); const int rising=Mix_Volume(channel,-1);
    check(rising>0 && rising<MIX_MAX_VOLUME,"entry gain rises progressively");
    SDL_Delay(650);
    check(Mix_FadingChannel(channel)==MIX_NO_FADING && Mix_Volume(channel,-1)==MIX_MAX_VOLUME,"entry reaches steady level");
    const long long audible=energy.load(); SDL_Delay(350);
    check(energy.load()>audible,"track supplies actual decoded PCM");
    for(int i=0;i<5;++i) music.update(40);
    check(music.playbackChannel()==channel && Mix_FadingChannel(channel)==MIX_NO_FADING,"tabs and redraws do not restart music");
    music.stop();
    check(music.playing() && Mix_FadingChannel(channel)==MIX_FADING_OUT,"exit fades instead of halting");
    SDL_Delay(250); const int falling=Mix_Volume(channel,-1);
    check(falling>0 && falling<MIX_MAX_VOLUME,"exit gain falls progressively");
    music.stop(); SDL_Delay(650);
    check(!music.playing(),"repeated stop does not extend fade into driving");
    const long long silent=energy.load(); SDL_Delay(120);
    check(energy.load()==silent,"no music PCM after exit fade");
    check(music.start(argv[1],40),"re-entry starts again");
    SDL_Delay(300); music.stop(); SDL_Delay(100);
    check(music.start(argv[1],40),"rapid re-entry accepted during outgoing fade");
    check(Mix_FadingChannel(music.playbackChannel())==MIX_FADING_OUT,"rapid re-entry preserves outgoing envelope");
    SDL_Delay(850); music.update(40);
    check(Mix_FadingChannel(music.playbackChannel())==MIX_FADING_IN,"incoming fade starts after outgoing reaches silence");
    music.update(0);
    check(Mix_VolumeChunk(chunk,-1)==0,"global music mute respected");
    music.update(NAN);
    check(Mix_VolumeChunk(chunk,-1)==0,"nonfinite volume cannot become loud");
    music.shutdown();
    check(!music.playing(),"session teardown releases playback");
    int rate,channels; Uint16 format;
    check(Mix_QuerySpec(&rate,&format,&channels)!=0,"shared mixer remains open");
    music.shutdown(); check(!music.playing(),"shutdown is idempotent");
    FormulaGarageMusic invalid;
    check(!invalid.start("missing-garage-audio.mp3",40),"missing asset fails safely");
    check(Mix_QuerySpec(&rate,&format,&channels)!=0,"failed load preserves shared mixer");
    Mix_SetPostMix(nullptr,nullptr); Mix_CloseAudio(); SDL_Quit();
    std::printf("checks=%d failures=%d\n",checks,failures);
    return failures?1:0;
}
