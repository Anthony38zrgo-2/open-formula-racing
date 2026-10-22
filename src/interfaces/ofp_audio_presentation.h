/*
 * ofp_audio_presentation.h -- Audio presentation contract (Etapa 3/4).
 *
 * Published by `rustphysics` on every accepted physics step into the
 * per-car ring in `tCarElt`, consumed by `snddefault` through
 * `open_formula_audio::ofp_audio_push_step`.
 *
 * Rules (GOAL Etapa 3):
 * - Raw mechanical state only: no acoustic decision lives in the physics
 *   integrator. Delivered load, derivatives and shift/blip/recovery phases are
 *   derived inside `open_formula_audio` (ported `audio_telemetry.rs` adapter).
 * - One packet per accepted step carries version/size, car + session
 *   generation, sequence, simulation time and the real step duration. Cut
 *   windows and gear transitions ride on this clock, never on the last
 *   graphic frame.
 * - A bounded per-car ring preserves order and counts losses. Stale or
 *   over-session data is discarded in a controlled way (no stuck limiter).
 * - Never overload `formulaHud`; never reach into private module structures
 *   from `snddefault`; unit conversion m/s -> km/h happens exactly once, in
 *   the audio wrapper.
 *
 * Threading: the race loop is single-threaded (physics `updateCar` and sound
 * `refresh` interleave), so plain counters suffice. If either side ever moves
 * to its own thread, the ring indices must become `std::atomic`.
 */

#ifndef OFP_AUDIO_PRESENTATION_H
#define OFP_AUDIO_PRESENTATION_H

#include <cstddef>
#include <cstdint>

#include "ofp_audio_abi.h"

/* 128 ms at 500 Hz, including 25 steps per 20 FPS refresh. */
#define OFP_AUDIO_STEP_DEPTH 64

/* Physics surface codes (`surface_code` in open_formula.rs). */
#define OFP_SURFACE_ROAD 0
#define OFP_SURFACE_CURB 1
#define OFP_SURFACE_DIRT 2
#define OFP_SURFACE_GRASS 3
#define OFP_SURFACE_GRAVEL 4
#define OFP_SURFACE_SAND 5
#define OFP_SURFACE_WALL 6
#define OFP_SURFACE_METAL 7

/* `OfpAudioStepV1` is defined once in `ofp_audio_abi.h` (ABI v1, 552 bytes,
   locked by the Rust layout test). Everything below operates on it. */
static_assert(sizeof(OfpAudioStepV1) == 552,
              "OfpAudioStepV1 must stay 552 bytes (Rust ABI v1 layout test)");

/*
 * Explicit surface translation (physics -> audio bed code):
 *   Road -> asphalt(0); Grass -> grass(1); Dirt/Gravel/Sand -> sand(2);
 *   Curb -> rumble(3, piano/kerb); Wall/Metal -> asphalt(0, no bed).
 */
inline std::uint32_t OfpAudioTranslateSurface(std::int32_t physics_surface)
{
    switch (physics_surface)
    {
        case OFP_SURFACE_GRASS: return 1u;
        case OFP_SURFACE_DIRT:
        case OFP_SURFACE_GRAVEL:
        case OFP_SURFACE_SAND: return 2u;
        case OFP_SURFACE_CURB: return 3u;
        default: return 0u;
    }
}

/*
 * Driven-wheel output-shaft frequency (Hz), following the donor contract:
 * mean angular velocity of the driven wheels x final drive / 2PI.
 * Front axle (indices 0,1) counts when front_torque_split > 0;
 * rear axle (indices 2,3) counts when split < 1. Wheel order must stay
 * FL, FR, RL, RR on both sides of the ABI.
 */
inline double OfpAudioOutputShaftHz(const double wheel_spin_rad_s[OFP_AUDIO_STEP_WHEELS],
                                    double final_drive, double front_torque_split)
{
    double sum = 0.0;
    double count = 0.0;
    for (int i = 0; i < OFP_AUDIO_STEP_WHEELS; ++i)
    {
        const bool driven = (i < 2) ? (front_torque_split > 0.0)
                                    : (front_torque_split < 1.0);
        if (driven)
        {
            sum += wheel_spin_rad_s[i];
            count += 1.0;
        }
    }
    if (count <= 0.0 || !(final_drive > 0.0))
        return 0.0;
    return sum / count * final_drive / 6.28318530717958647692;
}

/*
 * Reader state for one car, owned by the consumer (`snddefault`).
 * `read` tracks the producer `write` counter; `session` the generation.
 */
struct OfpAudioRingState
{
    std::uint32_t read;
    std::uint32_t session;
    std::uint64_t dropped;
    bool bound;
};

/*
 * Drain new ring entries into `out` (up to `capacity`). Validates
 * version/size per entry, handles session changes (rebaseline, no drops
 * counted) and overruns (oldest discarded, counted). Returns entries copied.
 * Never fabricates a step; stale data is impossible by construction because
 * `read` only moves forward.
 */
inline int OfpAudioDrainSteps(const OfpAudioStepV1* ring, std::uint32_t depth,
                              std::uint32_t write_counter, std::uint32_t session,
                              OfpAudioRingState& state,
                              OfpAudioStepV1* out, int capacity)
{
    if (!ring || depth == 0 || !out || capacity <= 0)
        return 0;
    if (!state.bound || session != state.session)
    {
        state.read = write_counter;
        state.session = session;
        state.bound = true;
        return 0;
    }
    std::uint32_t available = write_counter - state.read;
    if (available == 0)
        return 0;
    if (available > depth)
    {
        state.dropped += (std::uint64_t)(available - depth);
        state.read = write_counter - depth;
        available = depth;
    }
    int count = 0;
    const int want = (available < (std::uint32_t)capacity) ? (int)available : capacity;
    for (int i = 0; i < want; ++i)
    {
        const OfpAudioStepV1& step = ring[(state.read + (std::uint32_t)i) % depth];
        if (step.schema_version != OFP_AUDIO_STEP_VERSION)
            continue;
        if (step.struct_size != (std::uint32_t)sizeof(OfpAudioStepV1))
            continue;
        if (step.session_generation != session)
            continue;
        out[count++] = step;
    }
    /* Consumed entries leave the ring even when individually invalid; the
       gap is counted so a corrupt slot can never pin the reader. */
    state.dropped += (std::uint64_t)(want - count);
    state.read += (std::uint32_t)want;
    return count;
}

#endif /* OFP_AUDIO_PRESENTATION_H */
