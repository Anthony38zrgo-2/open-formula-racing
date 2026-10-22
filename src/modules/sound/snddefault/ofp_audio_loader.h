/***************************************************************************
    file                 : ofp_audio_loader.h
    created              : Etapa 4 audio V10 (Open Formula)

    Versioned run-time loader for `open_formula_audio` (ABI v1). Mirrors the
    `rustphysics` DLL loader pattern: LoadLibrary/GetProcAddress, ABI version
    plus layout-signature check, fail-closed with a single logged cause.
    The library never opens an OpenAL device; it only renders PCM.

 ***************************************************************************/

#ifndef OFP_AUDIO_LOADER_H
#define OFP_AUDIO_LOADER_H

#include <cstdint>
#include <string>

#include "ofp_audio_abi.h"

class OfpAudioLoader
{
public:
    OfpAudioLoader();
    ~OfpAudioLoader();

    OfpAudioLoader(const OfpAudioLoader&) = delete;
    OfpAudioLoader& operator=(const OfpAudioLoader&) = delete;

    /* Loads `dll_path` and verifies ABI v1 + layout signature.
       Returns false on any failure; `lastError()` holds the single cause.
       A V10 initialisation failure later is reported per handle, never here. */
    bool load(const std::string& dll_path);
    void unload();
    bool isLoaded() const { return m_module != nullptr; }
    const std::string& lastError() const { return m_last_error; }
    const std::string& path() const { return m_path; }

    /* Verified entry points (null unless `load` succeeded). */
    std::uint32_t (*abi_version)();
    std::uint32_t (*sample_rate)();
    std::uint32_t (*max_block_frames)();
    const char* (*layout_signature)();
    const char* (*provenance)();
    std::uint32_t (*last_create_error)(char* buf, std::uint32_t buf_len);
    OfpAudioHandle (*create)(const char* assets_root, const struct OfpAudioCreateParamsV1* params);
    void (*destroy)(OfpAudioHandle handle);
    std::uint32_t (*v10_active)(OfpAudioHandle handle);
    std::uint32_t (*push_packet)(OfpAudioHandle handle, const struct OfpAudioPacketV1* packet);
    std::uint32_t (*push_step)(OfpAudioHandle handle, const struct OfpAudioStepV1* step);
    std::uint32_t (*trigger)(OfpAudioHandle handle, std::int32_t code);
    void (*set_listener_distance)(OfpAudioHandle handle, float distance_m);
    std::uint32_t (*render)(OfpAudioHandle handle, float* out_l, float* out_r, std::uint32_t n);
    std::uint32_t (*render_mono)(OfpAudioHandle handle, float* out, std::uint32_t n);
    std::uint32_t (*diagnostics)(OfpAudioHandle handle, struct OfpAudioDiagnosticsV1* out);
    std::uint32_t (*bank_id)(OfpAudioHandle handle, char* buf, std::uint32_t buf_len);
    std::uint32_t (*bank_sha256)(OfpAudioHandle handle, char* buf, std::uint32_t buf_len);
    std::uint32_t (*mixer_hash)(OfpAudioHandle handle, char* buf, std::uint32_t buf_len);
    std::uint32_t (*last_error)(OfpAudioHandle handle, char* buf, std::uint32_t buf_len);
    std::uint32_t (*reset)(OfpAudioHandle handle);
    std::uint32_t (*reset_session)(OfpAudioHandle handle, std::uint32_t new_generation);

private:
    void* m_module;
    std::string m_path;
    std::string m_last_error;
};

#endif /* OFP_AUDIO_LOADER_H */
