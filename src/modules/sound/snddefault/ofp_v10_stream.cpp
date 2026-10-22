/***************************************************************************
    file                 : ofp_v10_stream.cpp
    created              : Etapa 4 audio V10 (Open Formula)
 ***************************************************************************/

#include "ofp_v10_stream.h"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

#if defined(__APPLE__) && !defined(USE_MACPORTS)
#include "al.h"
#include "alc.h"
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <tgf.h>

#include "ofp_audio_loader.h"
#include "sound_defines.h"

/* Williams profile constants (Etapa 4 first activation). They must match the
   physics profile (`idle_rpm` 3500, `max_rpm` 17500); the mixer validates the
   range against the bank at creation and fails closed otherwise. */
static const double kWilliamsIdleRpm = 3500.0;
static const double kWilliamsMaxRpm = 17500.0;
static const float kWilliamsGearboxWhineFinalDrive = 4.65f;

static bool containsWilliams(const char* name)
{
    if (!name || !*name)
        return false;
    char lowered[256];
    std::size_t i = 0;
    for (; name[i] && i + 1 < sizeof(lowered); ++i)
        lowered[i] = (char)std::tolower((unsigned char)name[i]);
    lowered[i] = '\0';
    return std::strstr(lowered, "williams") != nullptr;
}

OfpV10Stream::OfpV10Stream(OfpAudioLoader& loader, int car_index)
    : m_loader(loader)
    , m_car_index(car_index)
    , m_handle(nullptr)
    , m_paused(false)
    , m_cockpit_mode(true)
    , m_al_ready(false)
    , m_activate_cooldown(0)
    , m_al_error_logged(false)
    , m_create_logged(false)
    , m_al_source(0)
    , freeBufferCount(0)
    , m_source_started(false)
    , m_underruns(0)
    , m_render_calls(0)
    , m_last_logged_dropped(0)
    , m_last_logged_underruns(0)
{
    m_ring_state.read = 0;
    m_ring_state.session = 0;
    m_ring_state.dropped = 0;
    m_ring_state.bound = false;
    std::memset(m_al_buffers, 0, sizeof(m_al_buffers));
    std::memset(m_scratch_l, 0, sizeof(m_scratch_l));
    std::memset(m_scratch_r, 0, sizeof(m_scratch_r));
    std::memset(m_scratch_mono, 0, sizeof(m_scratch_mono));
    std::memset(m_s16, 0, sizeof(m_s16));
}

OfpV10Stream::~OfpV10Stream()
{
    deactivate();
}

bool OfpV10Stream::tryActivate(tCarElt* car, const std::string& assets_root)
{
    if (m_handle)
        return true;
    if (!m_loader.isLoaded() || !car)
        return false;
    /* First activation gate: Williams cars only. Every other car keeps the
       existing audio until it has a compatible profile and telemetry. */
    if (!containsWilliams(car->_carName))
        return false;
    /* Physics gate: no published step, no backend (e.g. a non-rustphysics
       session leaves the ring zeroed, so schema never validates). */
    if (car->audio_step_write == 0)
        return false;
    const OfpAudioStepV1& first =
        car->audio_steps[(car->audio_step_write - 1) % OFP_AUDIO_STEP_DEPTH];
    if (first.schema_version != OFP_AUDIO_STEP_VERSION)
        return false;
    if (first.struct_size != (std::uint32_t)sizeof(OfpAudioStepV1))
        return false;
    /* Backoff after an expensive failure (decoding ~19 MB only to fail every
       refresh is not acceptable); the cheap gates above still run always. */
    if (m_activate_cooldown > 0)
    {
        --m_activate_cooldown;
        return false;
    }

    struct OfpAudioCreateParamsV1 params;
    std::memset(&params, 0, sizeof(params));
    params.schema_version = OFP_AUDIO_ABI_VERSION;
    params.struct_size = (std::uint32_t)sizeof(params);
    params.sample_rate = OFP_AUDIO_SAMPLE_RATE;
    params.forward_gears = 6;
    params.idle_rpm = kWilliamsIdleRpm;
    params.max_rpm = kWilliamsMaxRpm;
    /* Gains from the Williams Hockenheim `grand_prix_sampler` profile. */
    params.engine_gain = 1.0f;
    params.gearbox_gain = 0.561329504f;
    params.gearbox_whine_gain = 0.137826425f;
    params.gearbox_whine_tone_gain = 0.12f;
    params.gearbox_whine_noise_gain = 0.025f;
    params.backfire_gain = 1.0f;
    params.limiter_gain = 0.3f;
    params.coast_gain = 0.55f;
    params.coast_gain_enabled = 1;
    params.gearbox_whine_final_drive = kWilliamsGearboxWhineFinalDrive;
    params.reverse_ratio = 3.0f;
    params.gear_teeth = 21.0f;
    params.final_teeth = 46.0f;
    /* Gear ratios are informational here: the output-shaft frequency always
       comes from the physics packet, so these are only a fallback. */
    for (int i = 0; i < 8; ++i)
        params.gear_ratios[i] = 0.0f;
    if (car->_gearNb > 1 && car->_gearNb <= 9)
    {
        params.forward_gears = car->_gearNb - 1;
        for (int i = 0; i < params.forward_gears && i < 8; ++i)
        {
            /* _gearRatio holds MAX_GEARS entries including final drive; stay
               strictly inside even against an unexpected offset. */
            const int ratio_index = i + car->_gearOffset + 1;
            params.gear_ratios[i] = (ratio_index >= 0 && ratio_index < MAX_GEARS)
                ? (float)car->_gearRatio[ratio_index]
                : 0.0f;
        }
    }
    params.fallback_to_legacy = 0; /* fail closed: never a silent legacy mix over the V10 */
    params.reserved[0] = params.reserved[1] = params.reserved[2] = params.reserved[3] = 0;

    OfpAudioHandle handle = m_loader.create(assets_root.c_str(), &params);
    if (!handle)
    {
        char error[256] = { 0 };
        m_loader.last_create_error(error, sizeof(error));
        /* Fail-closed is a steady state (missing banks/DLL), not a per-frame
           event: one log per stream lifetime is enough. */
        if (!m_create_logged)
        {
            m_create_logged = true;
            GfLogError("Open Formula audio: V10 unavailable for car %d: %s\n",
                       m_car_index, error);
        }
        m_activate_cooldown = 300;
        return false;
    }
    if (!m_loader.v10_active(handle))
    {
        /* Must never announce the V10 as active nor keep a legacy mix over it. */
        m_loader.destroy(handle);
        GfLogError("Open Formula audio: V10 init reported inactive for car %d\n", m_car_index);
        m_activate_cooldown = 300;
        return false;
    }
    m_handle = handle;
    if (!ensureAlSource(true))
    {
        m_loader.destroy(m_handle);
        m_handle = nullptr;
        m_activate_cooldown = 300;
        return false;
    }
    m_ring_state.read = car->audio_step_write - 1;
    m_ring_state.session = car->audio_session;
    m_ring_state.dropped = 0;
    m_ring_state.bound = true;
    logStatus("activated");
    return true;
}

void OfpV10Stream::deactivate()
{
    releaseAlSource();
    if (m_handle)
    {
        m_loader.destroy(m_handle);
        m_handle = nullptr;
    }
    m_ring_state.bound = false;
    m_source_started = false;
}

void OfpV10Stream::setPaused(bool paused)
{
    m_paused = paused;
}

bool OfpV10Stream::ensureAlSource(bool cockpit)
{
    if (m_al_ready && cockpit == m_cockpit_mode)
        return true;
    releaseAlSource();

    alGetError();
    alGenSources(1, &m_al_source);
    if (alGetError() != AL_NO_ERROR)
    {
        if (!m_al_error_logged)
        {
            m_al_error_logged = true;
            GfLogError("Open Formula audio: car %d has no OpenAL source left\n", m_car_index);
        }
        return false;
    }
    for (int i = 0; i < kBuffers; ++i)
    {
        alGenBuffers(1, &m_al_buffers[i]);
        if (alGetError() != AL_NO_ERROR || !alIsBuffer(m_al_buffers[i]))
        {
            if (!m_al_error_logged)
            {
                m_al_error_logged = true;
                GfLogError("Open Formula audio: car %d has no OpenAL buffers left\n", m_car_index);
            }
            releaseAlSource();
            return false;
        }
    }
    /* Same distance model as the legacy sounds (inverse distance, ref 5 m,
       rolloff 0.5): a single attenuation path, never doubled. */
    alSourcef(m_al_source, AL_REFERENCE_DISTANCE, 5.0f);
    alSourcef(m_al_source, AL_ROLLOFF_FACTOR, 0.5f);
    alSourcef(m_al_source, AL_MAX_DISTANCE, 10000.0f);
    alSourcef(m_al_source, AL_GAIN, 1.0f);
    alSourcef(m_al_source, AL_PITCH, 1.0f);
    if (cockpit)
    {
        /* Stereo buffers are not spatialised by OpenAL 1.1: run relative. */
        alSourcei(m_al_source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(m_al_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSource3f(m_al_source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        alSourcei(m_al_source, AL_SOURCE_RELATIVE, AL_FALSE);
    }
    m_cockpit_mode = cockpit;
    for (int i = 0; i < kBuffers; ++i)
        freeBuffers[i] = m_al_buffers[i];
    freeBufferCount = kBuffers;
    m_al_ready = true;
    m_source_started = false;
    m_al_error_logged = false;
    return true;
}

void OfpV10Stream::releaseAlSource()
{
    if (m_al_source && alIsSource(m_al_source))
    {
        alSourceStop(m_al_source);
        ALint queued = 0;
        alGetSourcei(m_al_source, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0)
        {
            ALuint buffer = 0;
            alSourceUnqueueBuffers(m_al_source, 1, &buffer);
            if (alGetError() != AL_NO_ERROR)
                break;
        }
        alDeleteSources(1, &m_al_source);
        alGetError();
    }
    m_al_source = 0;
    for (int i = 0; i < kBuffers; ++i)
    {
        if (m_al_buffers[i] && alIsBuffer(m_al_buffers[i]))
            alDeleteBuffers(1, &m_al_buffers[i]);
        m_al_buffers[i] = 0;
    }
    alGetError();
    m_al_ready = false;
    freeBufferCount = 0;
    m_source_started = false;
}

float OfpV10Stream::dopplerFactor(const float src_pos[3], const float src_vel[3],
                                  const float lis_pos[3], const float lis_vel[3]) const
{
    /* Single application of the legacy radial formula
       (audited against `SoundSource::update` in Sound.cpp; OpenAL Doppler
       stays disabled and the Rust pitch path stays untouched). */
    float p[3];
    float dist2 = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        p[i] = src_pos[i] - lis_pos[i];
        dist2 += p[i] * p[i];
    }
    const float dist = 0.01f + std::sqrt(dist2);
    float u_rel = 0.0f, u_rel_src = 0.0f, u_rel_lis = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        const float dir = p[i] / dist;
        const float u = src_vel[i] - lis_vel[i];
        u_rel += u * dir;
        u_rel_src += src_vel[i] * dir;
        u_rel_lis += lis_vel[i] * dir;
    }
    if (std::fabs(u_rel) >= 0.9f * (float)SPEED_OF_SOUND)
        return 1.0f;
    return (float)(((double)SPEED_OF_SOUND - (double)u_rel_src)
                 / ((double)SPEED_OF_SOUND - (double)u_rel_lis));
}

bool OfpV10Stream::recycleProcessedBuffers()
{
    alGetError();
    ALint processed = 0;
    alGetSourcei(m_al_source, AL_BUFFERS_PROCESSED, &processed);
    if (alGetError() != AL_NO_ERROR)
        return false;
    while (processed-- > 0 && freeBufferCount < kBuffers)
    {
        ALuint buffer = 0;
        alSourceUnqueueBuffers(m_al_source, 1, &buffer);
        if (alGetError() != AL_NO_ERROR)
            return false;
        freeBuffers[freeBufferCount++] = buffer;
    }
    return true;
}

bool OfpV10Stream::queueBlock(const float* left, const float* right, int frames, bool mono)
{
    if (freeBufferCount == 0)
        return false;
    ALuint buffer = freeBuffers[freeBufferCount - 1];
    const int channels = mono ? 1 : 2;
    for (int i = 0; i < frames; ++i)
    {
        float l = left[i];
        float r = mono ? left[i] : right[i];
        if (l > 1.0f) l = 1.0f; else if (l < -1.0f) l = -1.0f;
        if (r > 1.0f) r = 1.0f; else if (r < -1.0f) r = -1.0f;
        m_s16[i * channels] = (short)(l * 32767.0f);
        if (!mono)
            m_s16[i * channels + 1] = (short)(r * 32767.0f);
    }
    alBufferData(buffer, mono ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
                 m_s16, (ALsizei)(frames * channels * sizeof(short)), 44100);
    if (alGetError() != AL_NO_ERROR)
        return false;
    alSourceQueueBuffers(m_al_source, 1, &buffer);
    if (alGetError() != AL_NO_ERROR)
        return false;
    --freeBufferCount;
    return true;
}

void OfpV10Stream::logStatus(const char* what)
{
    char bank[128] = { 0 };
    char mixer[128] = { 0 };
    m_loader.bank_id(m_handle, bank, sizeof(bank));
    m_loader.mixer_hash(m_handle, mixer, sizeof(mixer));
    GfLogInfo("Open Formula audio: car %d %s (bank %s, mixer %s)\n",
              m_car_index, what, bank, mixer);
}

void OfpV10Stream::update(tCarElt* car, const float camera_pos[3], const float camera_vel[3],
                          bool audible)
{
    if (!m_handle || !car)
        return;
    const bool sessionChanged = car->audio_session != m_ring_state.session;
    const std::uint32_t pendingSteps = car->audio_step_write - m_ring_state.read;
    const bool telemetryOverrun = !sessionChanged && pendingSteps > OFP_AUDIO_STEP_DEPTH;
    if (sessionChanged || telemetryOverrun)
    {
        releaseAlSource();
        const std::uint32_t code = m_loader.reset_session(m_handle, car->audio_session);
        if (code != OFP_AUDIO_OK)
        {
            GfLogError("Open Formula audio: car %d session reset failed (%u)\n", m_car_index, code);
            deactivate();
            return;
        }
        m_ring_state.session = car->audio_session;
        if (sessionChanged)
            m_ring_state.read = 0;
        else
        {
            m_ring_state.dropped += pendingSteps - OFP_AUDIO_STEP_DEPTH;
            m_ring_state.read = car->audio_step_write - OFP_AUDIO_STEP_DEPTH;
        }
        m_ring_state.bound = true;
    }
    if ((car->_state & RM_CAR_STATE_NO_SIMU) || !audible || m_paused)
    {
        /* Freeze: stop the source and render nothing while muted/paused/
           disabled. Smoothing glides on resume; the ring overrun is counted. */
        if (m_al_ready && m_source_started)
        {
            alSourceStop(m_al_source);
            alGetError();
            m_source_started = false;
        }
        return;
    }

    float car_pos[3] = {
        (float)car->pub.DynGCg.pos.x,
        (float)car->pub.DynGCg.pos.y,
        (float)car->pub.DynGCg.pos.z
    };
    float car_vel[3] = {
        (float)car->pub.DynGCg.vel.x,
        (float)car->pub.DynGCg.vel.y,
        (float)car->pub.DynGCg.vel.z
    };
    float dx = car_pos[0] - camera_pos[0];
    float dy = car_pos[1] - camera_pos[1];
    float dz = car_pos[2] - camera_pos[2];
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const bool cockpit = distance < kCockpitDistanceM;
    /* Source first, drain after: if no OpenAL source is left, the steps stay
       ringed for the next tick instead of being consumed into nothing. A
       cockpit/exterior switch recreates the source and drops its queued PCM
       (≤93 ms gap); the mixer keeps advancing and smoothing glides over it. */
    if (!ensureAlSource(cockpit))
        return;

    /* Camera path is independent from mechanical events, but shares the tick:
       it only feeds listener distance (Rust LOD input; synth stays off). */
    m_loader.set_listener_distance(m_handle, distance);

    /* Drain new steps in order; drops are counted, never fabricated.
       Rising-edge impact detection lives in the wrapper (`push_step`), the
       single owner of that decision, so this loop stays a dumb pipe. */
    const int drained = OfpAudioDrainSteps(car->audio_steps, OFP_AUDIO_STEP_DEPTH,
                                           car->audio_step_write, car->audio_session,
                                           m_ring_state, m_batch, OFP_AUDIO_STEP_DEPTH);
    for (int i = 0; i < drained; ++i)
    {
        const std::uint32_t code = m_loader.push_step(m_handle, &m_batch[i]);
        if (code != OFP_AUDIO_OK)
        {
            GfLogError("Open Formula audio: car %d step rejected (%u)\n", m_car_index, code);
            deactivate();
            return;
        }
    }
    if (m_ring_state.dropped != m_last_logged_dropped
        || m_underruns != m_last_logged_underruns)
    {
        m_last_logged_dropped = m_ring_state.dropped;
        m_last_logged_underruns = m_underruns;
        /* Real depth/latency evidence for Etapa 6: drops (ring overrun) and
           underruns (starved queue) with the render-call count. */
        GfLogInfo("Open Formula audio: car %d steps dropped=%llu underruns=%llu renders=%llu\n",
                  m_car_index, (unsigned long long)m_ring_state.dropped,
                  (unsigned long long)m_underruns, (unsigned long long)m_render_calls);
    }

    /* Starvation check before refilling: a previously started source with no
       queued buffer underran while the feeding thread was stalled. */
    ALint queued = 0;
    if (!recycleProcessedBuffers())
        return;
    alGetSourcei(m_al_source, AL_BUFFERS_QUEUED, &queued);
    if (m_source_started && queued == 0)
    {
        ++m_underruns;
        m_source_started = false;
    }

    /* Top-up: four buffers cover a 50 ms refresh at 20 FPS. */
    int blocks = 0;
    while (queued < kBuffers && freeBufferCount > 0 && blocks < kBuffers)
    {
        std::uint32_t written = 0;
        if (cockpit)
        {
            written = m_loader.render(m_handle, m_scratch_l, m_scratch_r, kFrames);
            if (written != (std::uint32_t)kFrames)
                break;
            if (!queueBlock(m_scratch_l, m_scratch_r, kFrames, false))
                break;
        }
        else
        {
            written = m_loader.render_mono(m_handle, m_scratch_mono, kFrames);
            if (written != (std::uint32_t)kFrames)
                break;
            if (!queueBlock(m_scratch_mono, m_scratch_mono, kFrames, true))
                break;
            alSource3f(m_al_source, AL_POSITION, car_pos[0], car_pos[1], car_pos[2]);
            alSource3f(m_al_source, AL_VELOCITY, car_vel[0], car_vel[1], car_vel[2]);
            /* Pitch base is 1 (the sampler resolves it); Doppler exactly once. */
            alSourcef(m_al_source, AL_PITCH, dopplerFactor(car_pos, car_vel, camera_pos, camera_vel));
        }
        ++m_render_calls;
        ++blocks;
        alGetSourcei(m_al_source, AL_BUFFERS_QUEUED, &queued);
    }

    ALint state = 0;
    alGetSourcei(m_al_source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING && queued > 0)
    {
        alSourcePlay(m_al_source);
        if (alGetError() == AL_NO_ERROR)
            m_source_started = true;
    }
}
