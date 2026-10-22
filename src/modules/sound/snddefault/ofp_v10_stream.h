/***************************************************************************
    file                 : ofp_v10_stream.h
    created              : Etapa 4 audio V10 (Open Formula)

    One car's V10 backend: owns the Rust mixer handle plus one OpenAL source
    with four streaming buffers. All OpenAL calls happen on the refresh thread
    (the context owner); the context is never moved and Rust never opens a
    device. Rendered outside the simulation thread with preallocated buffers
    and bounded batches.

    Transport decision (recorded before default activation): synchronous
    top-up in `grRefreshSound`, keeping four buffers queued with at most four
    blocks per refresh. Four 1024-frame buffers at 44100 Hz hold ~93 ms max.
    A prolonged stall of the feeding thread still starves that queue; the
    starvation counter proves it. Move to a dedicated OpenAL owner thread only
    if Etapa 6 measures habitual underruns -- never grow the buffer
    indefinitely and never share one context concurrently without a contract.

    Cockpit (camera within 3 m of the car): stereo stream relative to the
    listener. Exterior/rivals: mono downmix from the mixer on a positional
    source (OpenAL 1.1 does not spatialise multichannel buffers; the downmix
    is verified sample-accurate, never claimed stereo-equivalent). The engine
    pitch is resolved by the sampler, so the PCM source runs at pitch base 1;
    exterior Doppler is applied exactly once with the legacy radial formula
    (audited against `SoundSource::update`; OpenAL Doppler stays disabled and
    the Rust synth/LOD path stays off, so there is no double pitch or double
    attenuation). Global volume/mute apply once through the listener gain and
    source pause, never as per-source scaling.

 ***************************************************************************/

#ifndef OFP_V10_STREAM_H
#define OFP_V10_STREAM_H

#include <cstdint>
#include <string>

#include <raceman.h>

#if defined(__APPLE__) && !defined(USE_MACPORTS)
#include "al.h"
#else
#include <AL/al.h>
#endif

#include "ofp_audio_abi.h"
#include "ofp_audio_presentation.h"

class OfpAudioLoader;

class OfpV10Stream
{
public:
    static const int kBuffers = 4;
    static const int kFrames = 1024;
    static const int kMaxStreams = 4;
    /* Cockpit threshold: camera within 3 m renders stereo relative. */
    static constexpr float kCockpitDistanceM = 3.0f;

    explicit OfpV10Stream(OfpAudioLoader& loader, int car_index);
    ~OfpV10Stream();

    OfpV10Stream(const OfpV10Stream&) = delete;
    OfpV10Stream& operator=(const OfpV10Stream&) = delete;

    /* Tries Williams activation: car-name gate plus first valid step from the
       physics ring, then mixer creation with the explicit profile. Returns
       true only with the V10 sampler confirmed active (fail-closed otherwise:
       no silent legacy mix over the V10). */
    bool tryActivate(tCarElt* car, const std::string& assets_root);
    bool isActive() const { return m_handle != nullptr; }
    void deactivate();

    /* One refresh tick (refresh thread only): drain new steps, push them in
       order, top-up the OpenAL queue, recycle processed buffers. `camera_pos`
       selects cockpit (stereo relative) vs exterior (mono positional).
       `audible` is false when muted/paused/disabled: the source stops and no
       PCM renders (the mixer state freezes; smoothing glides on resume). */
    void update(tCarElt* car, const float camera_pos[3], const float camera_vel[3],
                bool audible);

    void setPaused(bool paused);
    std::uint64_t underruns() const { return m_underruns; }
    std::uint64_t renderCalls() const { return m_render_calls; }
    std::uint64_t droppedSteps() const { return m_ring_state.dropped; }
    int carIndex() const { return m_car_index; }

private:
    bool ensureAlSource(bool cockpit);
    bool recycleProcessedBuffers();
    void releaseAlSource();
    float dopplerFactor(const float src_pos[3], const float src_vel[3],
                        const float lis_pos[3], const float lis_vel[3]) const;
    bool queueBlock(const float* left, const float* right, int frames, bool mono);
    void logStatus(const char* what);

    OfpAudioLoader& m_loader;
    int m_car_index;
    OfpAudioHandle m_handle;
    OfpAudioRingState m_ring_state;
    bool m_paused;
    bool m_cockpit_mode;
    bool m_al_ready;
    /* Activation backoff (in refreshes) and silencing of repeated AL errors:
       a persistent failure must neither re-decode 19 MB per frame nor flood
       the log. */
    int m_activate_cooldown;
    bool m_al_error_logged;
    bool m_create_logged;

    unsigned int m_al_source;
    unsigned int m_al_buffers[kBuffers];
    unsigned int freeBuffers[kBuffers];
    int freeBufferCount;
    bool m_source_started;

    /* Preallocated: never allocate on the render path. */
    float m_scratch_l[kFrames];
    float m_scratch_r[kFrames];
    float m_scratch_mono[kFrames];
    short m_s16[kFrames * 2];
    OfpAudioStepV1 m_batch[OFP_AUDIO_STEP_DEPTH];

    /* Impact edge detection lives in the wrapper (`push_step`), the single
       owner of that decision: a C++ copy here would fire every hit twice. */

    std::uint64_t m_underruns;
    std::uint64_t m_render_calls;
    std::uint64_t m_last_logged_dropped;
    std::uint64_t m_last_logged_underruns;
};

#endif /* OFP_V10_STREAM_H */
