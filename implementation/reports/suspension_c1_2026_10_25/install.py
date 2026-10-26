"""Install the C1 suspension build into the local game distribution.

Reversible, file-only: never stops or drives a running game session. Every replaced
file is backed up and a rollback.ps1 is written next to the backup.
"""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
DIST = ROOT / "distribution"
CAR = "share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/"
PROFILE = ROOT / "rust/data/vehicles/williams-fw19-hockenheim"
TARGETS = {
    "lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll":
        ROOT / "rust/target/release/vehicle_physics_engine.dll",
    CAR + "suspension.json": PROFILE / "suspension.json",
    CAR + "suspension_filters.json": PROFILE / "suspension_filters.json",
    CAR + "vehicle.json": PROFILE / "vehicle.json",
    CAR + "AGENTS.md": PROFILE / "AGENTS.md",
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    validation = json.loads((OUT / "after_dll_validation.json").read_text(encoding="utf-8"))
    assert validation["passed"], "the candidate must pass verify_dll before installation"
    candidate = TARGETS["lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll"]
    assert sha(candidate) == validation["sha256"], "validated DLL differs from the built DLL"
    assert sha(PROFILE / "suspension.json") == validation["source_profile_sha256"]

    running = subprocess.run(
        ["powershell", "-NoProfile", "-Command",
         "@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count"],
        capture_output=True, text=True, check=True,
    ).stdout.strip()
    assert running == "0", "game is open; close it before installing"

    manifest_path = DIST / "distribution_manifest.json"
    metadata = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = {entry["path"]: entry for entry in metadata["files"]}

    backup = Path.home() / "open-formula-backups" / ("2026-10-25-install-c1-" + uuid.uuid4().hex[:8])
    backup.mkdir(parents=True)
    previous: dict[str, str | None] = {}
    for rel in list(TARGETS) + ["distribution_manifest.json"]:
        src = DIST / rel
        dst = backup / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        if src.exists():
            shutil.copy2(src, dst)
            previous[rel] = sha(src)
        else:
            previous[rel] = None

    installed: dict[str, dict] = {}
    try:
        for rel, src in TARGETS.items():
            target = DIST / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            staged = target.with_name(target.name + ".pending")
            shutil.copy2(src, staged)
            staged.replace(target)
            digest = sha(target)
            assert digest == sha(src)
            entry = entries.get(rel)
            if entry is None:
                entry = {"path": rel}
                metadata["files"].append(entry)
                entries[rel] = entry
            entry.update(size=target.stat().st_size, sha256=digest)
            installed[rel] = {"sha256": digest, "size": target.stat().st_size}
        metadata["files"].sort(key=lambda entry: entry["path"])
        manifest_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    except Exception:
        for rel, digest in previous.items():
            if digest is None:
                (DIST / rel).unlink(missing_ok=True)
            else:
                shutil.copy2(backup / rel, DIST / rel)
        raise

    lines = ["$ErrorActionPreference = 'Stop'",
             "if (@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count) { throw 'Close the game first' }"]
    for rel, digest in previous.items():
        if digest is None:
            lines.append(f"Remove-Item -LiteralPath '{DIST / rel}' -ErrorAction SilentlyContinue")
        else:
            lines.append(f"Copy-Item -LiteralPath '{backup / rel}' -Destination '{DIST / rel}' -Force")
    (backup / "rollback.ps1").write_text("\n".join(lines) + "\n", encoding="utf-8")

    receipt = {
        "installed": installed,
        "previous_sha256": previous,
        "backup": str(backup),
        "rollback": str(backup / "rollback.ps1"),
        "game_launcher": str(DIST / "OpenFormula.exe"),
        "profile_car_directory": str(DIST / CAR),
        "ride_height_target_m": {"front": 0.035014, "rear": 0.039991},
        "notes": "C1 suspension: damper knee blend, cubic bump stop + dedicated damper, "
                 "compliant hard stop, ARB damper, single road-height route with slew limit; "
                 "all 1 ms filters at 0.0.",
    }
    (OUT / "installation_c1.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in receipt.items() if k != "previous_sha256"}, indent=2))


if __name__ == "__main__":
    main()
