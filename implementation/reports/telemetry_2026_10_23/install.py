"""Install the verified telemetry build (physics DLL + host module) into the distribution.

Guards:
  * refuses to run while the game is open;
  * refuses to install anything that did not pass the offline verification;
  * hashes the user's setup database and every vehicle profile before and after;
  * backs up both DLLs and the manifest, and writes a rollback script.
"""
from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
DISTRIBUTION = ROOT / "distribution"
MODULES = DISTRIBUTION / "lib/games/speed-dreams-2/modules/simu"
MANIFEST = DISTRIBUTION / "distribution_manifest.json"
BUILD_MODULES = Path(r"C:\Users\bill\open-formula-build\engine\lib\games\speed-dreams-2\modules\simu")

TARGETS = [
    (
        "lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll",
        ROOT / "rust/target/release/vehicle_physics_engine.dll",
    ),
    (
        "lib/games/speed-dreams-2/modules/simu/rustphysics.dll",
        BUILD_MODULES / "rustphysics.dll",
    ),
]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    validation = json.loads((OUT / "telemetry_validation/telemetry_validation.json").read_text(encoding="utf-8"))
    assert validation["failed"] == 0, "verification did not pass; refusing to install"
    assert validation["abi_version"] == 7, "verified build is not ABI v7"
    for relative, candidate in TARGETS:
        assert candidate.exists(), f"missing build output: {candidate}"
        if relative.endswith("vehicle_physics_engine.dll"):
            assert sha(candidate) == validation["candidate_sha256"], (
                "the physics DLL changed after it was verified"
            )

    running = subprocess.run(
        ["powershell", "-NoProfile", "-Command",
         "@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count"],
        capture_output=True, text=True, check=True).stdout.strip()
    assert running == "0", "the game is running; close it before installing"

    protected = [DISTRIBUTION / "user-settings/open_formula.db"]
    protected += list((DISTRIBUTION / "share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim").glob("*.json"))
    protected += [DISTRIBUTION / "user-settings/config/rust_physics.xml"]
    before = {str(path): sha(path) for path in protected if path.exists()}

    backup = Path.home() / "open-formula-backups" / ("2026-10-23-telemetry-" + uuid.uuid4().hex[:8])
    backup.mkdir(parents=True)
    previous = {}
    for relative, candidate in TARGETS:
        target = DISTRIBUTION / relative
        assert target.exists(), f"unexpected distribution layout: {target} is missing"
        previous[relative] = sha(target)
        shutil.copy2(target, backup / Path(relative).name)
    shutil.copy2(MANIFEST, backup / "distribution_manifest.json")

    metadata = json.loads(MANIFEST.read_text(encoding="utf-8"))
    installed = {}
    staged: list[Path] = []
    try:
        for relative, candidate in TARGETS:
            target = DISTRIBUTION / relative
            entry = next((row for row in metadata["files"] if row["path"] == relative), None)
            assert entry is not None, f"manifest has no entry for {relative}"
            pending = target.with_suffix(".dll.pending")
            shutil.copy2(candidate, pending)
            staged.append(pending)
            os.replace(pending, target)
            installed[relative] = {"sha256": sha(target), "size": target.stat().st_size}
            entry.update(sha256=installed[relative]["sha256"], size=installed[relative]["size"])
        MANIFEST.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        for relative, digest in installed.items():
            assert sha(DISTRIBUTION / relative) == digest["sha256"]
        assert all(sha(Path(path)) == digest for path, digest in before.items()), (
            "a protected user file changed during installation"
        )
    except Exception:
        for relative, _ in TARGETS:
            target = DISTRIBUTION / relative
            shutil.copy2(backup / Path(relative).name, target)
        shutil.copy2(backup / "distribution_manifest.json", MANIFEST)
        raise

    rollback_lines = [
        "$ErrorActionPreference = 'Stop'",
        "if (@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count) { throw 'Close the game first' }",
    ]
    for relative, _ in TARGETS:
        rollback_lines.append(
            f"Copy-Item -LiteralPath '{backup / Path(relative).name}' "
            f"-Destination '{DISTRIBUTION / relative}' -Force"
        )
    rollback_lines.append(f"Copy-Item -LiteralPath '{backup / 'distribution_manifest.json'}' -Destination '{MANIFEST}' -Force")
    rollback_lines.append("Write-Output 'rolled back'")
    (backup / "rollback.ps1").write_text("\n".join(rollback_lines) + "\n", encoding="utf-8")

    sources = [
        "rust/vehicle-physics-engine/src/vertical_diagnostics.rs",
        "rust/vehicle-physics-engine/src/simplified_axle_integration.rs",
        "rust/vehicle-physics-engine/src/simulation.rs",
        "rust/vehicle-physics-engine/src/suspension.rs",
        "rust/vehicle-physics-engine/src/open_formula.rs",
        "rust/vehicle-physics-engine/src/open_formula_ffi.rs",
        "src/modules/simu/rustphysics/rustphysics.cpp",
        "src/modules/simu/rustphysics/rust_abi.h",
        "src/modules/simu/rustphysics/rust_library.h",
        "src/modules/simu/rustphysics/rust_library.cpp",
    ]
    receipt = {
        "installed": installed,
        "previous_sha256": previous,
        "backup": str(backup),
        "rollback": str(backup / "rollback.ps1"),
        "abi_version": validation["abi_version"],
        "abi_signature": validation["abi_signature"],
        "verification": {
            "passed": validation["passed"],
            "failed": validation["failed"],
            "report": str(OUT / "telemetry_validation/telemetry_validation.json"),
        },
        "protected_sha256": before,
        "source_sha256": {path: sha(ROOT / path) for path in sources},
        "configuration": {
            "summary_hz": "25 (diagnostic interval seconds = 0.04)",
            "trace": "enabled in user-settings/config/rust_physics.xml; requests 3 s pre / 1 s post; metadata reports overwritten history",
            "trace_directory": "user-settings/rustphysics/",
        },
        "manual_T3_acceptance": "pending_user",
    }
    (OUT / "installation.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in receipt.items() if k not in ("protected_sha256", "source_sha256")}, indent=2))


if __name__ == "__main__":
    main()
