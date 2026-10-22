#!/usr/bin/env python3
"""Puerta de coherencia perfil Williams <-> stream C++ (Etapa 5).

Extrae las constantes de `OfpV10Stream::tryActivate` y las compara con
`rust/data/vehicles/williams-fw19-hockenheim/{audio,powertrain}.json`.
La fuente de verdad es el codigo: si alguien retoca el stream sin tocar el
perfil (o al reves), la puerta falla. Sale distinto de cero ante desvio.

Usage: python tools/check_williams_profile.py
"""

import json
import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
STREAM = REPO_ROOT / "src" / "modules" / "sound" / "snddefault" / "ofp_v10_stream.cpp"
AUDIO_JSON = REPO_ROOT / "rust" / "data" / "vehicles" / "williams-fw19-hockenheim" / "audio.json"
POWERTRAIN_JSON = REPO_ROOT / "rust" / "data" / "vehicles" / "williams-fw19-hockenheim" / "powertrain.json"

# Atributo del perfil -> patron del literal en el stream.
GAINS = {
    "engine_gain": r"params\.engine_gain = ([\d.]+)f;",
    "gearbox_gain": r"params\.gearbox_gain = ([\d.]+)f;",
    "gearbox_whine_gain": r"params\.gearbox_whine_gain = ([\d.]+)f;",
    "gearbox_whine_tone_gain": r"params\.gearbox_whine_tone_gain = ([\d.]+)f;",
    "gearbox_whine_noise_gain": r"params\.gearbox_whine_noise_gain = ([\d.]+)f;",
    "backfire_gain": r"params\.backfire_gain = ([\d.]+)f;",
    "limiter_gain": r"params\.limiter_gain = ([\d.]+)f;",
    "coast_gain": r"params\.coast_gain = ([\d.]+)f;",
}

failures = []


def check(name, condition, detail=""):
    print(("OK   " if condition else "FAIL ") + name + (" " + str(detail) if detail and not condition else ""))
    if not condition:
        failures.append(name)


def main():
    code = STREAM.read_text(encoding="utf-8")
    profile = json.loads(AUDIO_JSON.read_text(encoding="utf-8"))["grand_prix_sampler"]
    powertrain = json.loads(POWERTRAIN_JSON.read_text(encoding="utf-8"))["powertrain"]
    for attribute, pattern in GAINS.items():
        match = re.search(pattern, code)
        if not match:
            check(f"gain {attribute} present", False, "literal no encontrado")
            continue
        check(f"gain {attribute} == {profile[attribute]}",
              abs(float(match.group(1)) - float(profile[attribute])) < 1e-9,
              match.group(1))
    idle = re.search(r"kWilliamsIdleRpm = ([\d.]+)", code)
    maximum = re.search(r"kWilliamsMaxRpm = ([\d.]+)", code)
    check("idle 3500", idle and float(idle.group(1)) == float(powertrain["idle_rpm"]))
    check("max 17500", maximum and float(maximum.group(1)) == float(powertrain["max_rpm"]))
    final = re.search(r"kWilliamsGearboxWhineFinalDrive = ([\d.]+)f", code)
    check("final drive", final and abs(float(final.group(1)) - float(powertrain["final_drive"])) < 1e-9)
    print("WILLIAMS_PROFILE_OK" if not failures else f"FAILURES: {len(failures)}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
