#include <cassert>
#include <cstdio>
#include <tgf.h>
#include "OpenalSoundInterface.h"

int main()
{
    GfInit(false);
    for (int reservation : {0, 4})
    {
        OpenalSoundInterface backend(44100, 32);
        backend.reserveStreamingSources(reservation);
        backend.initSharedSourcePool();
        ALuint sources[4]{};
        for (int index = 0; index < reservation; ++index)
        {
            alGenSources(1, &sources[index]);
            assert(alGetError() == AL_NO_ERROR);
        }
        ALuint extra = 0;
        alGenSources(1, &extra);
        assert(alGetError() != AL_NO_ERROR);
        if (reservation)
            alDeleteSources(reservation, sources);
        assert(alGetError() == AL_NO_ERROR);
    }
    std::puts("OFP_SOURCE_POOL_TEST_OK legacy_budget four_reserved_streams");
}
