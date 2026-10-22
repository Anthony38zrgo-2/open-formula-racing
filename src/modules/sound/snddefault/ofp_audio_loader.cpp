/***************************************************************************
    file                 : ofp_audio_loader.cpp
    created              : Etapa 4 audio V10 (Open Formula)
 ***************************************************************************/

#include "ofp_audio_loader.h"

#include <cstddef>
#include <cstdio>
#include <cstring>

#include <tgf.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Must match `ofp_audio_layout_signature()` in rust/open-formula-audio. */
static const char* kExpectedLayoutSignature =
    "OfpAudioCreateParamsV1:136;"
    "OfpAudioPacketV1:224;"
    "OfpAudioDiagnosticsV1:112;"
    "OFP_AUDIO_ABI_VERSION=1";

OfpAudioLoader::OfpAudioLoader()
    : abi_version(nullptr)
    , sample_rate(nullptr)
    , max_block_frames(nullptr)
    , layout_signature(nullptr)
    , provenance(nullptr)
    , last_create_error(nullptr)
    , create(nullptr)
    , destroy(nullptr)
    , v10_active(nullptr)
    , push_packet(nullptr)
    , push_step(nullptr)
    , trigger(nullptr)
    , set_listener_distance(nullptr)
    , render(nullptr)
    , render_mono(nullptr)
    , diagnostics(nullptr)
    , bank_id(nullptr)
    , bank_sha256(nullptr)
    , mixer_hash(nullptr)
    , last_error(nullptr)
    , reset(nullptr)
    , reset_session(nullptr)
    , m_module(nullptr)
{
}

OfpAudioLoader::~OfpAudioLoader()
{
    unload();
}

bool OfpAudioLoader::load(const std::string& dll_path)
{
    unload();
    m_path = dll_path;

#ifdef _WIN32
    HMODULE module = ::LoadLibraryA(dll_path.c_str());
    if (!module)
    {
        char buf[32];
        (void)std::snprintf(buf, sizeof(buf), "%lu", (unsigned long)::GetLastError());
        m_last_error = "LoadLibrary failed for " + dll_path + " (error " + buf + ")";
        return false;
    }
    m_module = reinterpret_cast<void*>(module);

    /* Same table pattern as `rustphysics` (`rust_library.cpp`): the DLL
       symbol names differ from the member names, so each row pairs the
       string with its slot explicitly. */
    struct Symbol
    {
        const char* symbol;
        void** slot;
    };
    const Symbol symbols[] = {
        { "ofp_audio_abi_version", reinterpret_cast<void**>(&abi_version) },
        { "ofp_audio_sample_rate", reinterpret_cast<void**>(&sample_rate) },
        { "ofp_audio_max_block_frames", reinterpret_cast<void**>(&max_block_frames) },
        { "ofp_audio_layout_signature", reinterpret_cast<void**>(&layout_signature) },
        { "ofp_audio_provenance", reinterpret_cast<void**>(&provenance) },
        { "ofp_audio_last_create_error", reinterpret_cast<void**>(&last_create_error) },
        { "ofp_audio_create", reinterpret_cast<void**>(&create) },
        { "ofp_audio_destroy", reinterpret_cast<void**>(&destroy) },
        { "ofp_audio_v10_active", reinterpret_cast<void**>(&v10_active) },
        { "ofp_audio_push_packet", reinterpret_cast<void**>(&push_packet) },
        { "ofp_audio_push_step", reinterpret_cast<void**>(&push_step) },
        { "ofp_audio_trigger", reinterpret_cast<void**>(&trigger) },
        { "ofp_audio_set_listener_distance", reinterpret_cast<void**>(&set_listener_distance) },
        { "ofp_audio_render", reinterpret_cast<void**>(&render) },
        { "ofp_audio_render_mono", reinterpret_cast<void**>(&render_mono) },
        { "ofp_audio_diagnostics", reinterpret_cast<void**>(&diagnostics) },
        { "ofp_audio_bank_id", reinterpret_cast<void**>(&bank_id) },
        { "ofp_audio_bank_sha256", reinterpret_cast<void**>(&bank_sha256) },
        { "ofp_audio_mixer_hash", reinterpret_cast<void**>(&mixer_hash) },
        { "ofp_audio_last_error", reinterpret_cast<void**>(&last_error) },
        { "ofp_audio_reset", reinterpret_cast<void**>(&reset) },
        { "ofp_audio_reset_session", reinterpret_cast<void**>(&reset_session) },
    };
    for (size_t i = 0; i < sizeof(symbols) / sizeof(symbols[0]); ++i)
    {
        *symbols[i].slot = reinterpret_cast<void*>(::GetProcAddress(module, symbols[i].symbol));
        if (!*symbols[i].slot)
        {
            m_last_error = std::string("missing audio symbol: ") + symbols[i].symbol;
            unload();
            return false;
        }
    }

    if (abi_version() != OFP_AUDIO_ABI_VERSION)
    {
        char buf[128];
        (void)std::snprintf(buf, sizeof(buf), "audio ABI mismatch: library reports %u, host expects %u",
                            (unsigned)abi_version(), (unsigned)OFP_AUDIO_ABI_VERSION);
        m_last_error = buf;
        unload();
        return false;
    }
    const char* signature = layout_signature();
    if (!signature || std::strcmp(signature, kExpectedLayoutSignature) != 0)
    {
        m_last_error = "audio ABI layout mismatch: library and host were built "
                       "from different structure definitions";
        unload();
        return false;
    }
    if (sample_rate() != OFP_AUDIO_SAMPLE_RATE)
    {
        m_last_error = "audio sample-rate mismatch: library does not render at 44100 Hz";
        unload();
        return false;
    }
    GfLogInfo("Open Formula audio: loaded %s (%s)\n", dll_path.c_str(), provenance());
    return true;
#else
    /* Non-Windows builds have no deployment target yet (Etapa 5 verifies the
       load on Windows/UCRT64); fail closed instead of half-resolving. */
    (void)dll_path;
    m_last_error = "audio DLL loading is only supported on Windows in this build";
    return false;
#endif
}

void OfpAudioLoader::unload()
{
    if (m_module)
    {
#ifdef _WIN32
        ::FreeLibrary(reinterpret_cast<HMODULE>(m_module));
#endif
        m_module = nullptr;
    }
    abi_version = nullptr;
    sample_rate = nullptr;
    max_block_frames = nullptr;
    layout_signature = nullptr;
    provenance = nullptr;
    last_create_error = nullptr;
    create = nullptr;
    destroy = nullptr;
    v10_active = nullptr;
    push_packet = nullptr;
    push_step = nullptr;
    trigger = nullptr;
    set_listener_distance = nullptr;
    render = nullptr;
    render_mono = nullptr;
    diagnostics = nullptr;
    bank_id = nullptr;
    bank_sha256 = nullptr;
    mixer_hash = nullptr;
    last_error = nullptr;
    reset = nullptr;
    reset_session = nullptr;
}
