#!/usr/bin/env python3
"""Puerta del manifiesto del replay offline (Etapa 2/6).

Exige que `implementation/reports/audio_etapa2/replay_manifest.json` declare
la fuente seleccionada y la mezcla efectiva con los hashes fijados, 16
segmentos audibles y sin clipping. Sale distinto de cero ante cualquier desvio.

Usage: python tools/check_replay_manifest.py
"""

import json
import pathlib
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
MANIFEST = REPO_ROOT / "implementation" / "reports" / "audio_etapa2" / "replay_manifest.json"

PINNED_MIXER = "9346307e8235da4166592fb88dee147003ae5c64b15517c19bc35cc56d93e966"


def main():
    try:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        print(f"FAIL: cannot read replay manifest: {error}")
        return 1
    failures = []

    def check(name, condition, detail=""):
        print(("OK   " if condition else "FAIL ") + name + (" " + str(detail) if detail and not condition else ""))
        if not condition:
            failures.append(name)

    check("bank_id is v10_v2_engine_bank", manifest.get("bank_id") == "v10_v2_engine_bank",
          manifest.get("bank_id"))
    check("mixer hash pinned", manifest.get("mixer_config_hash") == PINNED_MIXER,
          manifest.get("mixer_config_hash"))
    check("v10_active", manifest.get("v10_active") is True)
    segments = manifest.get("segments", [])
    check("16 segments", len(segments) == 16, len(segments))
    check("all audible, none clipping",
          all(0.001 < s.get("peak", 0) < 1.0 for s in segments))
    check("no drops", all(s.get("dropped_packets", -1) == 0 for s in segments))
    print("REPLAY_MANIFEST_OK" if not failures else f"FAILURES: {len(failures)}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
