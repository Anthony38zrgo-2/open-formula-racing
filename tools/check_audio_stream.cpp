#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <string>
#include <cstring>
#include <raceman.h>
#include <tgf.h>
extern "C" {
#include <AL/alc.h>
}

#define private public
#include "ofp_v10_stream.h"
#undef private
#include "ofp_audio_loader.h"

static void publishStep(tCarElt& car, std::uint64_t sequence)
{
    OfpAudioStepV1 step{};
    step.schema_version = OFP_AUDIO_STEP_VERSION;
    step.struct_size = sizeof(step);
    step.car_index = 0;
    step.session_generation = car.audio_session;
    step.sequence = sequence;
    step.sim_time_s = sequence * 0.002;
    step.dt_s = 0.002f;
    step.rpm = 3500;
    step.max_torque_nm = 390;
    step.final_drive = 4.65;
    car.audio_steps[car.audio_step_write++ % OFP_AUDIO_STEP_DEPTH] = step;
}

int main(int argumentCount, char** arguments)
{
    assert(argumentCount == 3);
    ALCdevice* device = alcOpenDevice(nullptr);
    assert(device);
    ALCcontext* context = alcCreateContext(device, nullptr);
    assert(context && alcMakeContextCurrent(context));
    {
        OfpAudioLoader loader;
        assert(loader.load(arguments[1]));
        auto car = std::unique_ptr<tCarElt>(new tCarElt{});
        std::strcpy(car->_carName, "williams-nineteen-ninety-seven");
        car->audio_session = 1;
        publishStep(*car, 1);
        OfpV10Stream stream(loader, 0);
        assert(stream.tryActivate(car.get(), arguments[2]));
        const float camera[3] = {0, 0, 0};
        stream.update(car.get(), camera, camera, true);
        ALint queued = 0;
        alGetSourcei(stream.m_al_source, AL_BUFFERS_QUEUED, &queued);
        assert(queued == OfpV10Stream::kBuffers);
        assert(stream.freeBufferCount == 0);
        assert(stream.renderCalls() == OfpV10Stream::kBuffers);
        alSourceStop(stream.m_al_source);
        publishStep(*car, 2);
        stream.update(car.get(), camera, camera, true);
        alGetSourcei(stream.m_al_source, AL_BUFFERS_QUEUED, &queued);
        assert(queued == OfpV10Stream::kBuffers);
        assert(stream.renderCalls() == 2 * OfpV10Stream::kBuffers);
        assert(stream.freeBufferCount == 0);

        car->audio_session = 2;
        car->audio_step_write = 0;
        for (std::uint64_t sequence = 1; sequence <= 25; ++sequence)
            publishStep(*car, sequence);
        stream.update(car.get(), camera, camera, true);
        assert(stream.isActive());
        assert(stream.droppedSteps() == 0);
        OfpAudioDiagnosticsV1 diagnostics{};
        diagnostics.schema_version = OFP_AUDIO_ABI_VERSION;
        diagnostics.struct_size = sizeof(diagnostics);
        assert(loader.diagnostics(stream.m_handle, &diagnostics) == OFP_AUDIO_OK);
        assert(diagnostics.last_sequence == 25);
        assert(diagnostics.dropped_packets == 0);
        publishStep(*car, 26);
        stream.update(car.get(), camera, camera, true);
        assert(loader.diagnostics(stream.m_handle, &diagnostics) == OFP_AUDIO_OK);
        assert(diagnostics.last_sequence == 26);
        for (std::uint64_t sequence = 27; sequence <= 91; ++sequence)
            publishStep(*car, sequence);
        stream.update(car.get(), camera, camera, true);
        assert(stream.isActive());
        assert(stream.droppedSteps() == 1);
        assert(loader.diagnostics(stream.m_handle, &diagnostics) == OFP_AUDIO_OK);
        assert(diagnostics.last_sequence == 91);
        assert(diagnostics.dropped_packets == 0);
        stream.deactivate();

        ALuint sources[1024]{};
        int count = 0;
        while (count < 1024)
        {
            alGenSources(1, &sources[count]);
            if (alGetError() != AL_NO_ERROR)
                break;
            ++count;
        }
        assert(count > 0);
        assert(!stream.tryActivate(car.get(), arguments[2]));
        assert(!stream.isActive());
        alDeleteSources(count, sources);
        assert(alGetError() == AL_NO_ERROR);
    }
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
    std::puts("OFP_STREAM_TEST_OK priming recycling session low_fps source_exhaustion");
}
