#!/usr/bin/env python3
"""Puerta offline completa del audio V10 (Etapas 1-5).

Corre, en orden, las verificaciones que no necesitan el juego compilado:
suite del wrapper + paridad, suite del motor (con sus 16 fallos esperados
pineados: rutas DSP/GF509/v8 desactivadas por diseno), sonda fisica v6,
afinacion 3500, helper de bancos (self-test), orden ABI y ambas DLL por
ctypes. Cada paso informa PASS/FAIL; el primero que falla detiene la puerta
con exit 1.

Usage: python tools/check_audio_gates.py
"""

import pathlib
import re
import subprocess
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
RUST_DIR = REPO_ROOT / "rust"

# Fallos esperados y pineados de la suite del motor: requieren f90_audio_dsp.dll,
# bancos GF509/v10_f2002_sampled/v8 o rutas donante, desactivados por diseno.
EXPECTED_ENGINE_FAILURES = frozenset([
    "dsp_runtime::tests::abi_and_build_source_gate_satisfied",
    "dsp_runtime::tests::disabled_when_master_gain_zero",
    "dsp_runtime::tests::impulse_timing_matches_event_offsets_both_banks_boundary",
    "dsp_runtime::tests::no_cross_block_loss_across_continuous_blocks",
    "mixer::tests::cpp_callback_has_zero_rust_allocations_and_frees",
    "mixer::tests::cpp_layer_enabled_augments_master",
    "mixer::tests::cpp_layer_gain_zero_leaves_body_invariant",
    "mixer::tests::gf509_render_has_zero_rust_allocations",
    "mixer::tests::gf509_replaces_only_continuous_source_and_one_shots_remain",
    "mixer::tests::legacy_telemetry_route_forwards_zero_dt_while_timed_interpolates",
    "mixer::tests::reset_stops_one_shots_and_clears_gf509_state",
    "mixer::tests::sampled_bank_enables_sample_only_engine_mode",
    "mixer::tests::v10_geometry_tuning_applies_and_invalid_geometry_is_rejected",
    "mixer::tests::v10_layer_tuning_applies_and_rejects_unknown_branch",
    "recorded_engine_automation::tests::invalid_control_points_and_external_paths_are_rejected",
    "recorded_engine_automation::tests::rendering_recorded_automation_does_not_allocate",
])


def engine_suite():
    """Corre la suite del motor y exige: verde salvo exactamente los 16 pineados."""
    print("=== gate: engine suite (16 pinned failures) ===")
    completed = subprocess.run(
        ["cargo", "test", "--offline", "-p", "vehicle_audio_engine", "--lib"],
        cwd=str(RUST_DIR), capture_output=True, text=True, timeout=600)
    output = completed.stdout + completed.stderr
    failed = set(re.findall(r"^test (\S+) \.\.\. FAILED", output, re.MULTILINE))
    if failed != EXPECTED_ENGINE_FAILURES:
        print(f"FAIL: engine suite: unexpected failure set: {sorted(failed - EXPECTED_ENGINE_FAILURES)}")
        print(f"FAIL: engine suite: missing pinned failures: {sorted(EXPECTED_ENGINE_FAILURES - failed)}")
        return 1
    passed = re.search(r"(\d+) passed", output)
    print(f"PASS: engine suite ({passed.group(1) if passed else '?'} passed, 16 pinned)")
    return 0

GATES = [
    ("wrapper suite + parity", ["cargo", "test", "--offline", "-p", "open_formula_audio"], RUST_DIR),
    ("v6 probe", ["cargo", "run", "--offline", "-p", "vehicle_physics_engine",
                  "--example", "ofp_audio_v6_probe"], RUST_DIR),
    ("idle 3500 tuning", [sys.executable, "tools/check_idle_3500.py"], REPO_ROOT),
    ("williams profile coherence", [sys.executable, "tools/check_williams_profile.py"], REPO_ROOT),
    ("replay manifest pins", [sys.executable, "tools/check_replay_manifest.py"], REPO_ROOT),
    ("ABI order", [sys.executable, "tools/check_audio_abi_order.py"], REPO_ROOT),
    ("release DLLs fresh", ["cargo", "build", "--offline", "--release",
                            "-p", "open_formula_audio", "-p", "vehicle_physics_engine"], RUST_DIR),
    ("audio DLL boundary", [sys.executable, "tools/check_audio_dll.py"], REPO_ROOT),
    ("physics DLL", [sys.executable, "tools/check_physics_dll.py"], REPO_ROOT),
    ("C++ syntax + drain", [sys.executable, "tools/check_cxx_audio.py"], REPO_ROOT),
    ("OpenAL transport regression", [sys.executable, "tools/check_audio_stream.py"], REPO_ROOT),
]


def banks_self_test():
    """Self-test + copia completa de bancos contra el staging del repo o el
    checkout donante.

    Portable: primero `sounds/` del repo (staging local ignorado por git);
    si no existe, el checkout donante de esta máquina de desarrollo; si no hay
    ninguno, FAIL con mensaje (los bancos son requisito, no opcional).
    Tras el self-test, copia completa a un temporal y valida el receipt
    (34 archivos + hash de inventario): ejerce la ruta de copia, no solo pines.
    """
    import json
    import tempfile
    candidates = [
        REPO_ROOT / "sounds",
        pathlib.Path("D:/Formula90s/game/sounds"),
    ]
    source = next((c for c in candidates if (c / "banks/v10-v2-bank/manifest.json").is_file()), None)
    if source is None:
        print("FAIL: banks self-test: no bank source (repo sounds/ nor donor checkout)")
        return 1
    print(f"=== gate: banks self-test ({source}) ===")
    try:
        subprocess.run(
            [sys.executable, "tools/prepare_audio_banks.py", "--source", str(source),
             "--dest", tempfile.mkdtemp(), "--self-test"],
            cwd=str(REPO_ROOT), check=True,
            capture_output=True, text=True, timeout=600)
    except subprocess.CalledProcessError as error:
        print(f"FAIL: banks self-test\n{error.stdout[-2000:]}\n{error.stderr[-2000:]}")
        return 1
    except subprocess.TimeoutExpired:
        print("FAIL (timeout): banks self-test")
        return 1
    print("PASS: banks self-test")
    print("=== gate: banks full copy + receipt ===")
    dest = pathlib.Path(tempfile.mkdtemp(prefix="ofp-banks-gate-"))
    try:
        subprocess.run(
            [sys.executable, "tools/prepare_audio_banks.py", "--source", str(source),
             "--dest", str(dest)],
            cwd=str(REPO_ROOT), check=True,
            capture_output=True, text=True, timeout=600)
        receipt = json.loads((dest / "audio_install_receipt.json").read_text(encoding="utf-8"))
        assert len(receipt["files"]) == 34, receipt["files"]
        assert receipt["v10_wavs"] == 18 and receipt["commons_wavs"] == 12
        assert len(receipt["source_inventory_sha256"]) == 64
    except subprocess.CalledProcessError as error:
        print(f"FAIL: banks full copy\n{error.stdout[-2000:]}\n{error.stderr[-2000:]}")
        return 1
    except (AssertionError, KeyError, ValueError) as error:
        print(f"FAIL: banks receipt invalid: {error}")
        return 1
    except subprocess.TimeoutExpired:
        print("FAIL (timeout): banks full copy")
        return 1
    finally:
        import shutil
        shutil.rmtree(dest, ignore_errors=True)
    print("PASS: banks full copy + receipt")
    print("=== gate: banks negative (missing/corrupt) ===")
    import shutil as _shutil
    neg = pathlib.Path(tempfile.mkdtemp(prefix="ofp-banks-neg-"))
    try:
        # Solo manifests + mixer (sin WAVs): debe fallar por faltantes.
        for name in ("banks/v10-v2-bank/manifest.json", "banks/commons/bank_manifest.json",
                     "sound_mixer_config.json"):
            target = neg / "src" / name
            target.parent.mkdir(parents=True, exist_ok=True)
            _shutil.copyfile(source / name, target)
        missing = subprocess.run(
            [sys.executable, "tools/prepare_audio_banks.py", "--source", str(neg / "src"),
             "--dest", str(neg / "dst")],
            cwd=str(REPO_ROOT), capture_output=True, text=True, timeout=600)
        if missing.returncode == 0:
            print("FAIL: banks negative: missing assets accepted")
            return 1
        # Manifest mutado: debe fallar en el pin.
        with open(neg / "src" / "banks/v10-v2-bank/manifest.json", "r+b") as handle:
            handle.seek(100)
            handle.write(b"X")
        corrupt = subprocess.run(
            [sys.executable, "tools/prepare_audio_banks.py", "--source", str(neg / "src"),
             "--dest", str(neg / "dst2")],
            cwd=str(REPO_ROOT), capture_output=True, text=True, timeout=600)
        if corrupt.returncode == 0:
            print("FAIL: banks negative: corrupt manifest accepted")
            return 1
    except subprocess.TimeoutExpired:
        print("FAIL (timeout): banks negative")
        return 1
    finally:
        _shutil.rmtree(neg, ignore_errors=True)
    print("PASS: banks negative (missing/corrupt)")
    return 0


def main():
    for name, command, cwd in GATES:
        print(f"=== gate: {name} ===")
        try:
            subprocess.run(command, cwd=str(cwd), check=True,
                           capture_output=True, text=True, timeout=600)
        except subprocess.CalledProcessError as error:
            print(f"FAIL: {name}\n{error.stdout[-2000:]}\n{error.stderr[-2000:]}")
            return 1
        except subprocess.TimeoutExpired:
            print(f"FAIL (timeout): {name}")
            return 1
        print(f"PASS: {name}")
    if banks_self_test() != 0:
        return 1
    if engine_suite() != 0:
        return 1
    print("AUDIO_GATES_OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
