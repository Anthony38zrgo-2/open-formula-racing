"""Install only the tested physics DLL; preserve profile and user setup bytes."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import uuid
import os

root=Path(__file__).resolve().parents[3]
out=Path(__file__).resolve().parent
distribution=root/'distribution'
relative='lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll'
target=distribution/relative
candidate=root/'rust/target/release/vehicle_physics_engine.dll'
manifest=distribution/'distribution_manifest.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
smoke=json.loads((out/'smoke.json').read_text(encoding='utf-8'))
validation=json.loads((out/'validation.json').read_text(encoding='utf-8'))
assert validation['total_passed']==37 and validation['failed']==0
assert json.loads((out/'storage_validation/summary.json').read_text(encoding='utf-8'))['storage_checks']=='passed'
assert smoke['sha256']==sha(candidate),'candidate differs from the actual-track smoke'
check=subprocess.run(['powershell','-NoProfile','-Command',
    '@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count'],capture_output=True,text=True,check=True)
assert check.stdout.strip()=='0','game is running; do not overwrite its DLL'
protected=[distribution/'user-settings/open_formula.db',
           *(distribution/'share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim').glob('*.json')]
before={str(p):sha(p) for p in protected}
backup=Path.home()/'open-formula-backups'/('2026-10-23-suspension-coupling-'+uuid.uuid4().hex[:8])
backup.mkdir(parents=True)
shutil.copy2(target,backup/'vehicle_physics_engine.dll')
shutil.copy2(manifest,backup/'distribution_manifest.json')
old_hash=sha(target); new_hash=sha(candidate)
metadata=json.loads(manifest.read_text(encoding='utf-8'))
entry=next(row for row in metadata['files'] if row['path']==relative)
entry.update(sha256=new_hash,size=candidate.stat().st_size)
temp=target.with_suffix('.dll.pending')
shutil.copy2(candidate,temp)
os.replace(temp,target)
try:
    manifest.write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    assert sha(target)==new_hash
    assert all(sha(Path(p))==digest for p,digest in before.items()),'protected files changed'
except Exception:
    shutil.copy2(backup/'vehicle_physics_engine.dll',target)
    shutil.copy2(backup/'distribution_manifest.json',manifest)
    raise
rollback=f'''$ErrorActionPreference = 'Stop'
if (@(Get-Process -Name speed-dreams-2,OpenFormula -ErrorAction SilentlyContinue).Count) {{ throw 'Close the game first' }}
if ((Get-FileHash -LiteralPath '{target}').Hash -ne '{new_hash}') {{ throw 'DLL changed since installation; review before rollback' }}
Copy-Item -LiteralPath '{backup / 'vehicle_physics_engine.dll'}' -Destination '{target}' -Force
Copy-Item -LiteralPath '{backup / 'distribution_manifest.json'}' -Destination '{manifest}' -Force
Get-FileHash -LiteralPath '{target}'
'''
(backup/'rollback.ps1').write_text(rollback,encoding='utf-8')
sources=[root/'rust/vehicle-physics-engine/src'/name for name in
         ['simulation.rs','suspension.rs','simplified_axle_integration.rs','simplified_body_coupling.rs','open_formula.rs','wheel_mechanics.rs','vehicle_mass_properties.rs','lib.rs']]
receipt={'installed_dll':str(target),'installed_sha256':new_hash,'previous_sha256':old_hash,
         'backup':str(backup),'rollback':str(backup/'rollback.ps1'),'protected_sha256':before,
         'source_sha256':{str(p.relative_to(root)):sha(p) for p in sources},
         'abi_signature':smoke['abi_signature'],'manual_T3_acceptance':'pending_user',
         'scope':'physics DLL only; host ABI unchanged; audio/graphics DLLs unchanged'}
(out/'installation.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in receipt.items() if k not in ['protected_sha256','source_sha256']},indent=2))
