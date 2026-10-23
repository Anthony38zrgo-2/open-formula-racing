"""Install the validated suspension candidate with reversible file replacements."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
DIST = ROOT / "distribution"
CAR = "share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/"
TARGETS = {
    "lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll": ROOT / "rust/target/release/vehicle_physics_engine.dll",
    CAR + "suspension.json": ROOT / "rust/data/vehicles/williams-fw19-hockenheim/suspension.json",
    CAR + "profile_provenance.json": ROOT / "rust/data/vehicles/williams-fw19-hockenheim/profile_provenance.json",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    previous_receipt = json.loads((OUT / "installation.json").read_text(encoding="utf-8")) if (OUT / "installation.json").exists() else None
    validation = json.loads((OUT / "validation.json").read_text(encoding="utf-8"))
    assert validation["passed"]
    dll = json.loads((OUT / "dll_validation.json").read_text(encoding="utf-8"))
    assert dll["passed"] and sha(next(iter(TARGETS.values()))) == dll["sha256"]
    assert sha(TARGETS[CAR + "suspension.json"]) == dll["source_profile_sha256"]
    # Installation is file-only: never stop or drive a running user session.
    running = subprocess.run(["powershell", "-NoProfile", "-Command",
                              "@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count"],
                             capture_output=True, text=True, check=True).stdout.strip()
    assert running == "0", "game is open; candidate prepared, installation deferred"
    protected = [DIST / "user-settings/open_formula.db", DIST / "user-settings/config/rust_physics.xml"]
    protected += [p for p in (DIST / CAR).glob("*.json") if p.name not in ("suspension.json", "profile_provenance.json")]
    before = {str(p): sha(p) for p in protected if p.exists()}
    manifest = DIST / "distribution_manifest.json"
    metadata = json.loads(manifest.read_text(encoding="utf-8"))
    backup = Path.home() / "open-formula-backups" / ("2026-10-23-install-35-40-" + uuid.uuid4().hex[:8])
    backup.mkdir(parents=True)
    previous = {}
    for rel in list(TARGETS) + ["distribution_manifest.json"]:
        src = DIST / rel
        dst = backup / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        previous[rel] = sha(src)
    installed = {}
    try:
        for rel, src in TARGETS.items():
            entry = next((e for e in metadata["files"] if e["path"] == rel), None)
            assert entry is not None, f"missing manifest entry: {rel}"
            target = DIST / rel
            staged = target.with_name(target.name + ".pending")
            shutil.copy2(src, staged)
            os.replace(staged, target)
            digest = sha(target)
            assert digest == sha(src)
            entry.update(sha256=digest, size=target.stat().st_size)
            installed[rel] = {"sha256": digest, "size": target.stat().st_size}
        manifest.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        assert all(sha(Path(p)) == h for p, h in before.items()), "protected user/config file changed"
    except Exception:
        for rel in previous:
            shutil.copy2(backup / rel, DIST / rel)
        raise
    rollback = ["$ErrorActionPreference = 'Stop'",
                "if (@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count) { throw 'Close the game first' }"]
    for rel in previous:
        rollback.append(f"Copy-Item -LiteralPath '{backup / rel}' -Destination '{DIST / rel}' -Force")
    (backup / "rollback.ps1").write_text("\n".join(rollback) + "\n", encoding="utf-8")
    baseline_rollback = (previous_receipt.get("baseline_rollback",previous_receipt["rollback"]) if previous_receipt else str(backup / "rollback.ps1"))
    receipt = {"installed": installed, "previous_sha256": previous, "backup": str(backup),
               "rollback": str(backup / "rollback.ps1"), "baseline_rollback": baseline_rollback, "protected_sha256": before,
               "ride_height_target_m": {"front": .035, "rear": .040},
               "bump_stop_working_length_m": {"front": .005, "rear": .005},
               "human_driving_acceptance": "pending"}
    (OUT / "installation.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in receipt.items() if k != "protected_sha256"}, indent=2))


if __name__ == "__main__":
    main()
