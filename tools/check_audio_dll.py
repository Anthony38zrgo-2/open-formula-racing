"""Etapa 4/6 (offline): la DLL real open_formula_audio.dll por ctypes.

Ejercita el artefacto cdylib exacto que cargara snddefault (no el rlib de
los tests): version/firma/procedencia, creacion Williams, paquetes
temporizados, render estereo/mono, diagnosticos, consultas de strings y rutas
de error (version mala, null, bloque sobredimensionado). Todo con asserts;
sale distinto de cero ante cualquier desvio.
"""

import ctypes
import math
import pathlib
import struct
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
DLL = REPO / "rust" / "target" / "release" / "open_formula_audio.dll"
SOUNDS = REPO / "sounds"

failures = []


def check(name, condition, detail=""):
    print(("OK   " if condition else "FAIL ") + name + (" " + str(detail) if detail and not condition else ""))
    if not condition:
        failures.append(name)


def main():
    dll = ctypes.CDLL(str(DLL))
    dll.ofp_audio_abi_version.restype = ctypes.c_uint32
    dll.ofp_audio_sample_rate.restype = ctypes.c_uint32
    dll.ofp_audio_max_block_frames.restype = ctypes.c_uint32
    dll.ofp_audio_layout_signature.restype = ctypes.c_char_p
    dll.ofp_audio_provenance.restype = ctypes.c_char_p
    check("abi_version == 1", dll.ofp_audio_abi_version() == 1)
    check("sample_rate == 44100", dll.ofp_audio_sample_rate() == 44100)
    check("max_block == 4096", dll.ofp_audio_max_block_frames() == 4096)
    sig = dll.ofp_audio_layout_signature().decode()
    check("layout_signature", sig == "OfpAudioCreateParamsV1:136;OfpAudioPacketV1:224;OfpAudioDiagnosticsV1:112;OFP_AUDIO_ABI_VERSION=1", sig)
    prov = dll.ofp_audio_provenance().decode()
    check("provenance mentions OFP-V10-3500", "OFP-V10-3500" in prov, prov)

    class CreateParams(ctypes.Structure):
        _fields_ = [
            ("schema_version", ctypes.c_uint32), ("struct_size", ctypes.c_uint32),
            ("sample_rate", ctypes.c_uint32), ("forward_gears", ctypes.c_int32),
            ("idle_rpm", ctypes.c_double), ("max_rpm", ctypes.c_double),
            ("engine_gain", ctypes.c_float), ("gearbox_gain", ctypes.c_float),
            ("gearbox_whine_gain", ctypes.c_float), ("gearbox_whine_tone_gain", ctypes.c_float),
            ("gearbox_whine_noise_gain", ctypes.c_float), ("backfire_gain", ctypes.c_float),
            ("limiter_gain", ctypes.c_float), ("coast_gain", ctypes.c_float),
            ("coast_gain_enabled", ctypes.c_uint32),
            ("gearbox_whine_final_drive", ctypes.c_float), ("reverse_ratio", ctypes.c_float),
            ("gear_teeth", ctypes.c_float), ("final_teeth", ctypes.c_float),
            ("gear_ratios", ctypes.c_float * 8),
            ("fallback_to_legacy", ctypes.c_uint32), ("reserved", ctypes.c_uint32 * 4),
        ]

    assert ctypes.sizeof(CreateParams) == 136, ctypes.sizeof(CreateParams)
    dll.ofp_audio_create.argtypes = [ctypes.c_char_p, ctypes.POINTER(CreateParams)]
    dll.ofp_audio_create.restype = ctypes.c_void_p
    dll.ofp_audio_destroy.argtypes = [ctypes.c_void_p]
    dll.ofp_audio_v10_active.argtypes = [ctypes.c_void_p]
    dll.ofp_audio_v10_active.restype = ctypes.c_uint32

    params = CreateParams(
        1, 136, 44100, 6, 3500.0, 17500.0,
        1.0, 0.561329504, 0.137826425, 0.12, 0.025, 1.0, 0.3, 0.55, 1,
        4.65, 3.0, 21.0, 46.0, (ctypes.c_float * 8)(3.41, 2.64, 2.112, 1.727, 1.441, 1.232, 0.0, 0.0),
        0, (ctypes.c_uint32 * 4)(0, 0, 0, 0),
    )
    root = str(SOUNDS).encode()
    handle = dll.ofp_audio_create(root, ctypes.byref(params))
    check("create Williams non-null", bool(handle))
    if not handle:
        buf = ctypes.create_string_buffer(256)
        dll.ofp_audio_last_create_error.argtypes = [ctypes.c_char_p, ctypes.c_uint32]
        dll.ofp_audio_last_create_error.restype = ctypes.c_uint32
        dll.ofp_audio_last_create_error(buf, 256)
        print("create error:", buf.value.decode(errors="replace"))
        return 1
    check("v10_active == 1", dll.ofp_audio_v10_active(handle) == 1)

    # Bad-version creation must fail closed.
    bad = CreateParams()
    ctypes.memmove(ctypes.byref(bad), ctypes.byref(params), 136)
    bad.schema_version = 999
    check("bad version fails closed", not dll.ofp_audio_create(root, ctypes.byref(bad)))

    class Packet(ctypes.Structure):
        _fields_ = [
            ("schema_version", ctypes.c_uint32), ("struct_size", ctypes.c_uint32),
            ("car_index", ctypes.c_int32), ("session_generation", ctypes.c_uint32),
            ("sequence", ctypes.c_uint64), ("sim_time_s", ctypes.c_double),
            ("dt_s", ctypes.c_float), ("_pad0", ctypes.c_uint32),
            ("rpm", ctypes.c_double), ("throttle", ctypes.c_float),
            ("normalized_engine_load", ctypes.c_float), ("normalized_engine_torque", ctypes.c_float),
            ("rpm_derivative", ctypes.c_float), ("throttle_derivative", ctypes.c_float),
            ("speed_kph", ctypes.c_double), ("slip", ctypes.c_float),
            ("gear", ctypes.c_int32), ("torque_sign", ctypes.c_int32), ("shift_phase", ctypes.c_int32),
            ("clutch_engagement", ctypes.c_float), ("tc_cut_ratio", ctypes.c_float),
            ("rev_limiter_active", ctypes.c_uint32),
            ("output_shaft_hz", ctypes.c_double), ("has_output_shaft", ctypes.c_uint32),
            ("target_gear", ctypes.c_int32),
            ("scrape_active", ctypes.c_uint32), ("scrape_intensity", ctypes.c_float),
            ("scrape_speed_m_s", ctypes.c_float), ("scrape_onset", ctypes.c_float),
            ("tire_slip_ratio", ctypes.c_float * 4), ("tire_slip_angle", ctypes.c_float * 4),
            ("tire_contact", ctypes.c_float * 4), ("tire_load_n", ctypes.c_float * 4),
            ("tire_speed_kph", ctypes.c_float), ("surface_code", ctypes.c_uint32),
            ("reserved", ctypes.c_uint32),
        ]

    assert ctypes.sizeof(Packet) == 224, ctypes.sizeof(Packet)
    dll.ofp_audio_push_packet.argtypes = [ctypes.c_void_p, ctypes.POINTER(Packet)]
    dll.ofp_audio_push_packet.restype = ctypes.c_uint32

    def packet(seq):
        return Packet(
            1, 224, 0, 7, seq, seq * 0.01, 0.01, 0,
            9000.0, 0.8, 0.8, 0.6, 0.0, 0.0, 180.0, 0.02, 4, 1, 0,
            1.0, 0.0, 0, 80.0, 1, 4, 0, 0.0, 0.0, 0.0,
            (ctypes.c_float * 4)(0, 0, 0, 0), (ctypes.c_float * 4)(0, 0, 0, 0),
            (ctypes.c_float * 4)(1, 1, 1, 1), (ctypes.c_float * 4)(3500, 3500, 3500, 3500),
            180.0, 0, 0,
        )

    for seq in (1, 2, 3):
        check(f"push seq {seq} accepted", dll.ofp_audio_push_packet(handle, ctypes.byref(packet(seq))) == 0)
    check("stale rejected", dll.ofp_audio_push_packet(handle, ctypes.byref(packet(2))) == 7)
    check("null handle rejected", dll.ofp_audio_push_packet(None, ctypes.byref(packet(9))) == 1)
    nan_throttle = packet(4)
    nan_throttle.throttle = float("nan")
    check("NaN throttle rejected", dll.ofp_audio_push_packet(handle, ctypes.byref(nan_throttle)) == 9)
    check("binding intact after NaN", dll.ofp_audio_push_packet(handle, ctypes.byref(packet(4))) == 0)

    dll.ofp_audio_render.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float), ctypes.c_uint32]
    dll.ofp_audio_render.restype = ctypes.c_uint32
    dll.ofp_audio_render_mono.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_uint32]
    dll.ofp_audio_render_mono.restype = ctypes.c_uint32
    left = (ctypes.c_float * 1024)()
    right = (ctypes.c_float * 1024)()
    mono = (ctypes.c_float * 1024)()
    # Twin handles at identical state (same deterministic seed + packets),
    # exactly like the Rust twin test: stereo on one, mono on the other.
    other = dll.ofp_audio_create(root, ctypes.byref(params))
    check("twin handle non-null", bool(other))
    for seq in (1, 2, 3):
        dll.ofp_audio_push_packet(other, ctypes.byref(packet(seq)))
    check("render 1024 stereo", dll.ofp_audio_render(handle, left, right, 1024) == 1024)
    check("render overflow yields 0", dll.ofp_audio_render(handle, left, right, 5000) == 0)
    check("render null yields 0", dll.ofp_audio_render(None, left, right, 1024) == 0)
    check("render_mono 1024", dll.ofp_audio_render_mono(other, mono, 1024) == 1024)
    peak = max(max(abs(left[i]), abs(right[i])) for i in range(1024))
    check("finite", all(math.isfinite(left[i]) and math.isfinite(right[i]) for i in range(1024)))
    check("audible", peak > 0.001, peak)
    check("no clipping", peak < 1.0, peak)
    worst = max(abs(mono[i] - (left[i] + right[i]) * 0.5) for i in range(1024))
    check("mono is exact downmix", worst <= 1e-6, worst)

    class Diagnostics(ctypes.Structure):
        _fields_ = [
            ("schema_version", ctypes.c_uint32), ("struct_size", ctypes.c_uint32),
            ("sample_rate", ctypes.c_uint32), ("v10_active", ctypes.c_uint32),
            ("continuous_source", ctypes.c_uint32),
            ("rendered_rpm", ctypes.c_float), ("gate", ctypes.c_float), ("load_gain", ctypes.c_float),
            ("active_zones", ctypes.c_uint32), ("active_voices", ctypes.c_uint32),
            ("accepted_events", ctypes.c_uint64), ("suppressed_events", ctypes.c_uint64),
            ("dropped_packets", ctypes.c_uint64), ("last_sequence", ctypes.c_uint64),
            ("last_error_code", ctypes.c_uint32), ("reserved", ctypes.c_uint32),
            ("final_peak", ctypes.c_float), ("final_rms", ctypes.c_float),
            ("effective_min_rpm", ctypes.c_double), ("bank_min_rpm", ctypes.c_double),
            ("bank_max_rpm", ctypes.c_double),
        ]

    assert ctypes.sizeof(Diagnostics) == 112, ctypes.sizeof(Diagnostics)
    dll.ofp_audio_diagnostics.argtypes = [ctypes.c_void_p, ctypes.POINTER(Diagnostics)]
    dll.ofp_audio_diagnostics.restype = ctypes.c_uint32
    diag = Diagnostics(1, 112)
    check("diagnostics ok", dll.ofp_audio_diagnostics(handle, ctypes.byref(diag)) == 0)
    check("diag v10 active", diag.v10_active == 1)
    check("diag last_sequence == 4", diag.last_sequence == 4, diag.last_sequence)
    check("diag dropped == 1 (stale)", diag.dropped_packets == 1, diag.dropped_packets)
    check("diag effective min 3500", diag.effective_min_rpm == 3500.0, diag.effective_min_rpm)

    for name in ("ofp_audio_bank_id", "ofp_audio_bank_sha256", "ofp_audio_mixer_hash"):
        fn = getattr(dll, name)
        fn.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint32]
        fn.restype = ctypes.c_uint32
        buf = ctypes.create_string_buffer(128)
        check(f"{name} non-empty", fn(handle, buf, 128) > 0, buf.value.decode(errors="replace"))

    dll.ofp_audio_reset.argtypes = [ctypes.c_void_p]
    dll.ofp_audio_reset.restype = ctypes.c_uint32
    check("reset ok", dll.ofp_audio_reset(handle) == 0)
    dll.ofp_audio_destroy(handle)
    dll.ofp_audio_destroy(other)
    print("DLL_BOUNDARY_OK" if not failures else "FAILURES: %d" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
