"""Install the verified DLL, preserving user settings and an external rollback."""
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path
from uuid import uuid4

root = Path(__file__).resolve().parents[3]
out = Path(__file__).resolve().parent
distribution = root / 'distribution'
relative = 'lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll'
target = distribution / relative
candidate = root / 'rust/target/release/vehicle_physics_engine.dll'
manifest_path = distribution / 'distribution_manifest.json'
database = distribution / 'user-settings/open_formula.db'
profiles = sorted((distribution / 'share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim').glob('*.json'))
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
result = json.loads((out / 'verification/verification.json').read_text())
assert result['dlls']['installed']['sha256'] == sha(target), 'reference changed since verification'
assert result['dlls']['fixed']['sha256'] == sha(candidate), 'candidate changed since verification'
assert result['results']['regression_design_builds_agree']['identical_numeric_trajectory']
assert result['results']['replayed_saved_regraje']['identical_numeric_trajectory']
controls = [v for v in result['results'].values() if 'control' in v]
assert len(controls) == 10
assert all(v['matches_the_equivalent_physical_element'] and not v['identical_numeric_trajectory'] for v in controls)
check = subprocess.run(['powershell', '-NoProfile', '-Command',
    "@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count"],
    capture_output=True, text=True, check=True)
assert check.stdout.strip() == '0', 'a game session is running; do not replace its DLL'
backup = Path.home() / 'open-formula-backups' / ('2026-10-22-suspension-' + uuid4().hex[:8])
backup.mkdir(parents=True)
shutil.copy2(target, backup / 'vehicle_physics_engine.dll')
shutil.copy2(manifest_path, backup / 'distribution_manifest.json')
protected = {str(p): sha(p) for p in [database, *profiles]}
old_hash, new_hash = sha(target), sha(candidate)
manifest = json.loads(manifest_path.read_text())
entries = [entry for entry in manifest['files'] if entry['path'] == relative]
assert len(entries) == 1
entries[0].update(size=candidate.stat().st_size, sha256=new_hash)
temporary = target.with_suffix('.dll.pending')
shutil.copy2(candidate, temporary)
os.replace(temporary, target)
try:
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    assert sha(target) == new_hash
    assert all(sha(Path(p)) == digest for p, digest in protected.items()), 'protected user files changed'
except Exception:
    shutil.copy2(backup / 'vehicle_physics_engine.dll', target)
    shutil.copy2(backup / 'distribution_manifest.json', manifest_path)
    raise
rollback = f'''# Close Open Formula before restoring. User settings and profiles are not changed.
$running = @(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {{ throw 'Close Open Formula before rollback' }}
$currentHash = (Get-FileHash -LiteralPath '{target}' -Algorithm SHA256).Hash
if ($currentHash -ne '{new_hash}') {{ throw 'Installed DLL changed; review before rollback' }}
Copy-Item -LiteralPath '{backup / 'vehicle_physics_engine.dll'}' -Destination '{target}' -Force
Copy-Item -LiteralPath '{backup / 'distribution_manifest.json'}' -Destination '{manifest_path}' -Force
Get-FileHash -LiteralPath '{target}' -Algorithm SHA256
'''
(backup / 'rollback.ps1').write_text(rollback)
receipt = {'backup_directory': str(backup), 'old_sha256': old_hash, 'installed_sha256': new_hash,
           'installed_path': str(target), 'rollback_script': str(backup / 'rollback.ps1'),
           'protected_sha256': protected, 'profiles_and_database_unchanged': True,
           'manual_T3_acceptance': 'pending_user'}
(out / 'installation.json').write_text(json.dumps(receipt, indent=2))
print(json.dumps({k:v for k,v in receipt.items() if k != 'protected_sha256'}, indent=2))
