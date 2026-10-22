/*
 * ofp_audio_abi.h -- C ABI of the Open Formula audio library (ABI v1).
 *
 * Mirrors `rust/open-formula-audio/src/lib.rs`. Every structure is fixed-size,
 * `#[repr(C)]` on the Rust side and must stay in exact field order. The library
 * is loaded at run time with LoadLibrary, so snddefault never links a Rust
 * static library or a C++ runtime across the boundary.
 *
 * Owner rule: a single thread touches a mutable handle. No exception crosses
 * the ABI (all panics are caught). Creation from an explicit assets root and
 * explicit profile; V10 failure with fallback==0 fails closed (null).
 */

#ifndef OFP_AUDIO_ABI_H
#define OFP_AUDIO_ABI_H

#include <stddef.h>
#include <stdint.h>

#define OFP_AUDIO_ABI_VERSION 1u
#define OFP_AUDIO_OK 0u
#define OFP_AUDIO_SAMPLE_RATE 44100u
#define OFP_AUDIO_MAX_BLOCK_FRAMES 4096u
#define OFP_AUDIO_WHEELS 4
#define OFP_AUDIO_GEAR_RATIOS 8

#define OFP_AUDIO_SURFACE_ASPHALT 0u
#define OFP_AUDIO_SURFACE_GRASS 1u
#define OFP_AUDIO_SURFACE_SAND 2u
#define OFP_AUDIO_SURFACE_RUMBLE 3u

#define OFP_AUDIO_STEP_VERSION 1u
#define OFP_AUDIO_STEP_WHEELS 4
#define OFP_AUDIO_STEP_MAX_CONTACTS 32

struct OfpAudioStepV1
{
    uint32_t schema_version;
    uint32_t struct_size;
    int32_t car_index;
    uint32_t session_generation;
    uint64_t sequence;
    double sim_time_s;
    float dt_s;
    uint32_t _pad0;
    double rpm;
    float throttle;
    double speed_m_s;
    int32_t gear;
    int32_t target_gear;
    double engine_torque_nm;
    double clutch_torque_nm;
    double max_torque_nm;
    double clutch_engagement;
    double tc_cut_ratio_smoothed;
    uint32_t is_rev_limited;
    double shift_timer_s;
    double wheel_spin_rad_s[OFP_AUDIO_STEP_WHEELS];
    double final_drive;
    double front_torque_split;
    float slip_ratio[OFP_AUDIO_STEP_WHEELS];
    float slip_angle_rad[OFP_AUDIO_STEP_WHEELS];
    float contact_fraction[OFP_AUDIO_STEP_WHEELS];
    float normal_force_n[OFP_AUDIO_STEP_WHEELS];
    int32_t surface[OFP_AUDIO_STEP_WHEELS];
    uint32_t floor_valid_mask;
    double floor_impulse_ns[2];
    uint32_t body_contact_count;
    double body_contact_impulse_ns[OFP_AUDIO_STEP_MAX_CONTACTS];
    float scrape_speed_m_s;
    uint32_t reserved;
};

#define OFP_AUDIO_OK 0u
#define OFP_AUDIO_ERR_NULL 1u
#define OFP_AUDIO_ERR_VERSION 2u
#define OFP_AUDIO_ERR_SIZE 3u
#define OFP_AUDIO_ERR_UTF8 4u
#define OFP_AUDIO_ERR_NOT_FOUND 5u
#define OFP_AUDIO_ERR_V10_LOAD 6u
#define OFP_AUDIO_ERR_STALE 7u
#define OFP_AUDIO_ERR_SESSION 8u
#define OFP_AUDIO_ERR_RANGE 9u
#define OFP_AUDIO_ERR_PANIC 10u

struct OfpAudioCreateParamsV1
{
    uint32_t schema_version;
    uint32_t struct_size;
    uint32_t sample_rate;
    int32_t forward_gears;
    double idle_rpm;
    double max_rpm;
    float engine_gain;
    float gearbox_gain;
    float gearbox_whine_gain;
    float gearbox_whine_tone_gain;
    float gearbox_whine_noise_gain;
    float backfire_gain;
    float limiter_gain;
    float coast_gain;
    uint32_t coast_gain_enabled;
    float gearbox_whine_final_drive;
    float reverse_ratio;
    float gear_teeth;
    float final_teeth;
    float gear_ratios[OFP_AUDIO_GEAR_RATIOS];
    uint32_t fallback_to_legacy;
    uint32_t reserved[4];
};

struct OfpAudioPacketV1
{
    uint32_t schema_version;
    uint32_t struct_size;
    int32_t car_index;
    uint32_t session_generation;
    uint64_t sequence;
    double sim_time_s;
    float dt_s;
    uint32_t _pad0;
    double rpm;
    float throttle;
    float normalized_engine_load;
    float normalized_engine_torque;
    float rpm_derivative;
    float throttle_derivative;
    double speed_kph;
    float slip;
    int32_t gear;
    int32_t torque_sign;
    int32_t shift_phase;
    float clutch_engagement;
    float tc_cut_ratio;
    uint32_t rev_limiter_active;
    double output_shaft_hz;
    uint32_t has_output_shaft;
    int32_t target_gear;
    uint32_t scrape_active;
    float scrape_intensity;
    float scrape_speed_m_s;
    float scrape_onset;
    float tire_slip_ratio[OFP_AUDIO_WHEELS];
    float tire_slip_angle[OFP_AUDIO_WHEELS];
    float tire_contact[OFP_AUDIO_WHEELS];
    float tire_load_n[OFP_AUDIO_WHEELS];
    float tire_speed_kph;
    uint32_t surface_code;
    uint32_t reserved;
};

struct OfpAudioDiagnosticsV1
{
    uint32_t schema_version;
    uint32_t struct_size;
    uint32_t sample_rate;
    uint32_t v10_active;
    uint32_t continuous_source;
    float rendered_rpm;
    float gate;
    float load_gain;
    uint32_t active_zones;
    uint32_t active_voices;
    uint64_t accepted_events;
    uint64_t suppressed_events;
    uint64_t dropped_packets;
    uint64_t last_sequence;
    uint32_t last_error_code;
    uint32_t reserved;
    float final_peak;
    float final_rms;
    double effective_min_rpm;
    double bank_min_rpm;
    double bank_max_rpm;
};

typedef void* OfpAudioHandle;

#ifdef __cplusplus
/* Sizes locked by the Rust layout tests (`cargo test -p open_formula_audio`);
   any drift breaks compilation here instead of silently reinterpreting memory. */
static_assert(sizeof(struct OfpAudioCreateParamsV1) == 136, "CreateParamsV1 must stay 136 bytes");
static_assert(sizeof(struct OfpAudioPacketV1) == 224, "PacketV1 must stay 224 bytes");
static_assert(sizeof(struct OfpAudioDiagnosticsV1) == 112, "DiagnosticsV1 must stay 112 bytes");
static_assert(sizeof(struct OfpAudioStepV1) == 552, "StepV1 must stay 552 bytes");
extern "C" {
#endif

uint32_t ofp_audio_abi_version(void);
uint32_t ofp_audio_sample_rate(void);
uint32_t ofp_audio_max_block_frames(void);
const char* ofp_audio_layout_signature(void);
const char* ofp_audio_provenance(void);
uint32_t ofp_audio_last_create_error(char* buf, uint32_t buf_len);

OfpAudioHandle ofp_audio_create(const char* assets_root, const struct OfpAudioCreateParamsV1* params);
void ofp_audio_destroy(OfpAudioHandle handle);
uint32_t ofp_audio_v10_active(OfpAudioHandle handle);
uint32_t ofp_audio_push_packet(OfpAudioHandle handle, const struct OfpAudioPacketV1* packet);
uint32_t ofp_audio_push_step(OfpAudioHandle handle, const struct OfpAudioStepV1* step);
uint32_t ofp_audio_trigger(OfpAudioHandle handle, int32_t code);
void ofp_audio_set_listener_distance(OfpAudioHandle handle, float distance_m);
uint32_t ofp_audio_render(OfpAudioHandle handle, float* out_l, float* out_r, uint32_t n);
uint32_t ofp_audio_render_mono(OfpAudioHandle handle, float* out, uint32_t n);
uint32_t ofp_audio_diagnostics(OfpAudioHandle handle, struct OfpAudioDiagnosticsV1* out);
uint32_t ofp_audio_bank_id(OfpAudioHandle handle, char* buf, uint32_t buf_len);
uint32_t ofp_audio_bank_sha256(OfpAudioHandle handle, char* buf, uint32_t buf_len);
uint32_t ofp_audio_mixer_hash(OfpAudioHandle handle, char* buf, uint32_t buf_len);
uint32_t ofp_audio_last_error(OfpAudioHandle handle, char* buf, uint32_t buf_len);
uint32_t ofp_audio_reset(OfpAudioHandle handle);
uint32_t ofp_audio_reset_session(OfpAudioHandle handle, uint32_t new_generation);

#ifdef __cplusplus
}
#endif

#endif /* OFP_AUDIO_ABI_H */
