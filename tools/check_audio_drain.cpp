// Drain-logic test for src/interfaces/ofp_audio_presentation.h.
// Self-contained: only the interfaces header, no third-party deps.
// Build (MSVC): cl /nologo /EHsc /std:c++17 /I src/interfaces /Feout.exe tools/check_audio_drain.cpp
// Run: asserts overrun/session/corrupt-slot behavior of OfpAudioDrainSteps.
#include <cassert>
#include <cstdio>
#include <initializer_list>

#include "ofp_audio_presentation.h"

static OfpAudioStepV1 make_step(unsigned int write_index, unsigned int session,
                                unsigned long long sequence)
{
    OfpAudioStepV1 step{};
    step.schema_version = OFP_AUDIO_STEP_VERSION;
    step.struct_size = (unsigned int)sizeof(OfpAudioStepV1);
    step.car_index = 0;
    step.session_generation = session;
    step.sequence = sequence;
    step.rpm = 8000.0;
    (void)write_index;
    return step;
}

void verifyPhysicsFrameRates()
{
    for (unsigned int framesPerSecond : {60u, 30u, 20u, 10u})
    {
        OfpAudioStepV1 ring[OFP_AUDIO_STEP_DEPTH]{};
        OfpAudioStepV1 output[OFP_AUDIO_STEP_DEPTH]{};
        OfpAudioRingState reader{0, 42, 0, true};
        unsigned int write = 0;
        unsigned long long expectedSequence = 1;
        for (unsigned int frame = 0; frame < framesPerSecond; ++frame)
        {
            const unsigned int target = (frame + 1) * 500 / framesPerSecond;
            while (write < target)
            {
                auto step = make_step(write, 42, write + 1);
                step.dt_s = 0.002f;
                step.is_rev_limited = ((write + 1) % 7 == 0);
                ring[write % OFP_AUDIO_STEP_DEPTH] = step;
                ++write;
            }
            const int count = OfpAudioDrainSteps(ring, OFP_AUDIO_STEP_DEPTH,
                write, 42, reader, output, OFP_AUDIO_STEP_DEPTH);
            for (int index = 0; index < count; ++index)
            {
                assert(output[index].sequence == expectedSequence);
                assert(output[index].is_rev_limited == (expectedSequence % 7 == 0));
                ++expectedSequence;
            }
        }
        assert(expectedSequence == 501);
        assert(reader.dropped == 0);
    }
}

int main()
{
    const unsigned int testDepth = 8;
    verifyPhysicsFrameRates();
    OfpAudioStepV1 ring[testDepth];
    for (unsigned int i = 0; i < testDepth; ++i)
        ring[i] = OfpAudioStepV1{};
    OfpAudioStepV1 out[16];
    OfpAudioRingState state{ 0, 0, 0, false };

    // Null guards.
    assert(OfpAudioDrainSteps(nullptr, testDepth, 0, 1, state, out, 16) == 0);
    assert(OfpAudioDrainSteps(ring, 0, 0, 1, state, out, 16) == 0);
    assert(OfpAudioDrainSteps(ring, testDepth, 0, 1, state, nullptr, 16) == 0);
    assert(OfpAudioDrainSteps(ring, testDepth, 0, 1, state, out, 0) == 0);

    // First call binds (no drops counted) and yields nothing.
    unsigned int write = 0;
    assert(OfpAudioDrainSteps(ring, testDepth, write, 7, state, out, 16) == 0);
    assert(state.bound && state.session == 7 && state.dropped == 0);

    // Publish 3 steps, drain all in order.
    for (unsigned long long s = 1; s <= 3; ++s)
        ring[write++ % testDepth] = make_step(write, 7, s);
    assert(OfpAudioDrainSteps(ring, testDepth, write, 7, state, out, 16) == 3);
    assert(out[0].sequence == 1 && out[2].sequence == 3);
    assert(state.dropped == 0);

    // Overrun: 10 more steps with depth 8 -> oldest 2 discarded and counted.
    for (unsigned long long s = 4; s <= 13; ++s)
        ring[write++ % testDepth] = make_step(write, 7, s);
    assert(OfpAudioDrainSteps(ring, testDepth, write, 7, state, out, 16) == 8);
    assert(out[0].sequence == 6 && out[7].sequence == 13);
    assert(state.dropped == 2);

    // Session bump rebaselines without counting drops.
    assert(OfpAudioDrainSteps(ring, testDepth, write, 8, state, out, 16) == 0);
    assert(state.session == 8 && state.dropped == 2);
    ring[write++ % testDepth] = make_step(write, 8, 14);
    assert(OfpAudioDrainSteps(ring, testDepth, write, 8, state, out, 16) == 1);
    assert(out[0].sequence == 14);

    // Corrupt slot never pins the reader: counted and skipped.
    ring[write % testDepth] = OfpAudioStepV1{}; // schema 0
    ++write;
    ring[write++ % testDepth] = make_step(write, 8, 15);
    assert(OfpAudioDrainSteps(ring, testDepth, write, 8, state, out, 16) == 1);
    assert(out[0].sequence == 15);
    assert(state.dropped == 3);

    // Partial drains: capacity smaller than available advances the reader
    // across calls without loss or duplication.
    OfpAudioRingState partial{ 0, 0, 0, false };
    unsigned int write2 = 0;
    for (unsigned long long s = 1; s <= 5; ++s)
        ring[write2++ % testDepth] = make_step(write2, 9, s);
    assert(OfpAudioDrainSteps(ring, testDepth, write2, 9, partial, out, 16) == 0); // bind
    for (unsigned long long s = 6; s <= 10; ++s)
        ring[write2++ % testDepth] = make_step(write2, 9, s);
    assert(OfpAudioDrainSteps(ring, testDepth, write2, 9, partial, out, 2) == 2);
    assert(out[0].sequence == 6 && out[1].sequence == 7);
    assert(OfpAudioDrainSteps(ring, testDepth, write2, 9, partial, out, 16) == 3);
    assert(out[0].sequence == 8 && out[2].sequence == 10);
    assert(partial.dropped == 0);

    // Exact boundary: available == depth drains whole without overrun.
    OfpAudioRingState edge{ 0, 0, 0, false };
    unsigned int write3 = 0;
    for (unsigned long long s = 1; s <= 8; ++s)
        ring[write3++ % testDepth] = make_step(write3, 10, s);
    assert(OfpAudioDrainSteps(ring, testDepth, write3, 10, edge, out, 16) == 0); // bind
    for (unsigned long long s = 9; s <= 16; ++s)
        ring[write3++ % testDepth] = make_step(write3, 10, s);
    assert(OfpAudioDrainSteps(ring, testDepth, write3, 10, edge, out, 16) == 8);
    assert(out[0].sequence == 9 && out[7].sequence == 16);
    assert(edge.dropped == 0);
    // One more than depth is the minimal overrun: oldest discarded, counted.
    for (unsigned long long s = 17; s <= 25; ++s)
        ring[write3++ % testDepth] = make_step(write3, 10, s);
    assert(OfpAudioDrainSteps(ring, testDepth, write3, 10, edge, out, 16) == 8);
    assert(out[0].sequence == 18 && out[7].sequence == 25);
    assert(edge.dropped == 1);

    std::printf("OFP_DRAIN_TEST_OK depth=%u size=%u\n", testDepth,
                (unsigned)sizeof(OfpAudioStepV1));
    return 0;
}

