#!/usr/bin/env python3
"""Prepare the shared V10 + commons audio banks for an Open Formula install.

Copies ONLY manifest-declared assets (no .import, no AGENTS.md, no remastering):
  <source>/banks/v10-v2-bank/{manifest.json + 18 declared WAVs + source_inventory.json}
  <source>/banks/commons/{bank_manifest.json + 12 declared WAVs}
  <source>/sound_mixer_config.json
into:
  <dest>/banks/v10-v2-bank/
  <dest>/banks/commons/
  <dest>/sound_mixer_config.json

Every copied file is verified against the SHA-256 declared by its manifest, and
both manifests plus the mixer config are verified against the pinned hashes in
`implementation/audio_v10_provenance.json`. Any missing or mismatched file is a
hard failure (non-zero exit). A receipt with asset/code hashes is written to
`<dest>/audio_install_receipt.json`.

Typical use (banks come from an explicit operator-provided checkout):
  python tools/prepare_audio_banks.py \
      --source D:/Formula90s/game/sounds \
      --dest distribution/share/games/speed-dreams-2/data/sounds
"""

import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
import wave

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
PROVENANCE = REPO_ROOT / "implementation" / "audio_v10_provenance.json"

V10_MANIFEST = "banks/v10-v2-bank/manifest.json"
COMMONS_MANIFEST = "banks/commons/bank_manifest.json"
MIXER_CONFIG = "sound_mixer_config.json"

# Pinned manifest/config hashes (mirrors the provenance record; the helper
# refuses manifests that differ from the approved bank revision).
PINNED = {
    V10_MANIFEST: "6cbf798f9535e83144660d25b5c42089af19f4dcbc29e23a7212cbee38f80b8a",
    COMMONS_MANIFEST: "1ee39041141b70a14abd862b556122255d3ba9e9b71b5c9b3d9098ff7454c0fe",
    MIXER_CONFIG: "9346307e8235da4166592fb88dee147003ae5c64b15517c19bc35cc56d93e966",
}


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fail(message):
    print(f"prepare_audio_banks: error: {message}", file=sys.stderr)
    return 1


def check_wav_format(path):
    """The V10 bank must stay integer PCM mono 44100/16 (commons: 44100 mono)."""
    try:
        with wave.open(str(path), "rb") as sample:
            channels = sample.getnchannels()
            width = sample.getsampwidth()
            rate = sample.getframerate()
            frames = sample.getnframes()
    except wave.Error as error:
        return f"{path.name}: not a readable WAV ({error})"
    if channels != 1:
        return f"{path.name}: expected mono, got {channels} channels"
    if rate != 44100:
        return f"{path.name}: expected 44100 Hz, got {rate}"
    if width != 2:
        return f"{path.name}: expected 16-bit samples, got {width * 8}-bit"
    if frames == 0:
        return f"{path.name}: empty sample"
    return None


def v10_declared_files(manifest):
    files = {}
    for loop in manifest.get("loops", []) + manifest.get("coast_loops", []):
        files[loop["derived_filename"]] = (loop["derived_sha256"], loop.get("derived_frames"))
    for event in manifest.get("events", []):
        files[event["derived_filename"]] = (event["derived_sha256"], event.get("derived_frames"))
    whine = manifest.get("transmission_whine")
    if whine:
        files[whine["derived_filename"]] = (whine["derived_sha256"], whine.get("derived_frames"))
    return files


def commons_declared_files(manifest):
    return {entry["file"]: (entry["sha256"], None) for entry in manifest.get("files", [])}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, help="sounds root holding banks/ + mixer config")
    parser.add_argument("--dest", required=True, help="destination sounds root to populate")
    parser.add_argument("--receipt", default=None, help="receipt path (default: <dest>/audio_install_receipt.json)")
    parser.add_argument("--self-test", action="store_true", help="verify helper logic against --source only (no copy)")
    arguments = parser.parse_args(argv)

    source = pathlib.Path(arguments.source)
    dest = pathlib.Path(arguments.dest)

    # 1. Manifests + mixer config exist and match the pinned bank revision.
    try:
        manifests = {}
        for name in (V10_MANIFEST, COMMONS_MANIFEST, MIXER_CONFIG):
            path = source / name
            if not path.is_file():
                return fail(f"source file missing: {path}")
            digest = sha256_file(path)
            if digest != PINNED[name]:
                return fail(f"{name} differs from the approved revision (got {digest})")
            if name.endswith(".json"):
                manifests[name] = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        return fail(f"cannot read source manifests: {error}")

    v10_files = v10_declared_files(manifests[V10_MANIFEST])
    commons_files = commons_declared_files(manifests[COMMONS_MANIFEST])
    if len(v10_files) != 18:
        return fail(f"V10 bank declares {len(v10_files)} WAVs, expected 18 (not the 13-asset bank)")
    if len(commons_files) != 12:
        return fail(f"commons bank declares {len(commons_files)} WAVs, expected 12")

    if arguments.self_test:
        print(f"self-test: manifests pinned OK ({len(v10_files)} V10 + {len(commons_files)} commons declared)")
        return 0

    # 2. Copy declared assets only, verifying bytes and WAV format/frames.
    copied = []
    failures = []

    def copy_declared(bank_dir, declared, check_frames):
        source_dir = source / bank_dir
        dest_dir = dest / bank_dir
        dest_dir.mkdir(parents=True, exist_ok=True)
        for filename, (expected_sha, expected_frames) in sorted(declared.items()):
            source_file = source_dir / filename
            if not source_file.is_file():
                failures.append(f"missing declared asset: {bank_dir}/{filename}")
                continue
            digest = sha256_file(source_file)
            if digest != expected_sha:
                failures.append(f"hash mismatch: {bank_dir}/{filename} (got {digest})")
                continue
            problem = check_wav_format(source_file)
            if problem:
                failures.append(problem)
                continue
            if check_frames and expected_frames is not None:
                with wave.open(str(source_file), "rb") as sample:
                    if sample.getnframes() != expected_frames:
                        failures.append(
                            f"frame mismatch: {bank_dir}/{filename} "
                            f"(got {sample.getnframes()}, manifest {expected_frames})"
                        )
                        continue
            dest_file = dest_dir / filename
            dest_file.write_bytes(source_file.read_bytes())
            copied.append(f"{bank_dir}/{filename}")

    copy_declared("banks/v10-v2-bank", v10_files, check_frames=True)
    copy_declared("banks/commons", commons_files, check_frames=False)

    # 3. Preserve manifests + mixer config (already hash-verified above).
    for name in (V10_MANIFEST, COMMONS_MANIFEST, MIXER_CONFIG):
        dest_file = dest / name
        dest_file.parent.mkdir(parents=True, exist_ok=True)
        dest_file.write_bytes((source / name).read_bytes())
        copied.append(name)

    # 3b. Preserve source_inventory.json + attributions: verified against the
    # hash the (pinned) V10 manifest itself declares for it.
    inventory_name = "banks/v10-v2-bank/source_inventory.json"
    inventory_source = source / inventory_name
    if not inventory_source.is_file():
        failures.append(f"missing provenance asset: {inventory_name}")
    else:
        declared_inventory = manifests[V10_MANIFEST].get("source_inventory_sha256")
        if sha256_file(inventory_source) != declared_inventory:
            failures.append(f"hash mismatch: {inventory_name}")
        else:
            dest_inventory = dest / inventory_name
            dest_inventory.write_bytes(inventory_source.read_bytes())
            copied.append(inventory_name)

    if failures:
        for failure in failures:
            print(f"prepare_audio_banks: error: {failure}", file=sys.stderr)
        return 1

    # 4. Receipt with asset + code hashes (no bank bytes in git).
    try:
        code_head = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=str(REPO_ROOT),
            capture_output=True, text=True, check=True,
        ).stdout.strip()
    except (subprocess.SubprocessError, OSError):
        code_head = "unknown"
    receipt = {
        "tool": "tools/prepare_audio_banks.py",
        "variant": "OFP-V10-3500",
        "source": str(source),
        "dest": str(dest),
        "v10_wavs": len(v10_files),
        "commons_wavs": len(commons_files),
        "manifest_hashes": {name: PINNED[name] for name in PINNED},
        "source_inventory_sha256": manifests[V10_MANIFEST].get("source_inventory_sha256"),
        "code_head": code_head,
        "files": sorted(copied),
    }
    receipt_path = pathlib.Path(arguments.receipt) if arguments.receipt else dest / "audio_install_receipt.json"
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    total_bytes = sum((dest / name).stat().st_size for name in copied)
    print(f"prepare_audio_banks: {len(copied)} files verified, {total_bytes} bytes -> {dest}")
    print(f"prepare_audio_banks: receipt {receipt_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
