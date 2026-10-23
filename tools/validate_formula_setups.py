"""Exercise production SQLite storage with descriptors from the real Rust vehicle."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sqlite3
import uuid

parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, default=Path('C:/Users/bill/open-formula-build/engine'))
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
args.output.mkdir(parents=True, exist_ok=True)
profile = root/'rust/data/vehicles/williams-fw19-hockenheim/vehicle.json'
profile_hash = hashlib.sha256(profile.read_bytes()).hexdigest()
library = ctypes.CDLL(str(root/'rust/target/release/vehicle_physics_engine.dll'))
class Vec3(ctypes.Structure):
    _fields_ = [('x', ctypes.c_double), ('y', ctypes.c_double), ('z', ctypes.c_double)]
library.ofp_create.argtypes = [ctypes.c_char_p, Vec3, ctypes.c_double, ctypes.c_double]
library.ofp_create.restype = ctypes.c_void_p
library.ofp_destroy.argtypes = [ctypes.c_void_p]
for name in ['ofp_setup_fields', 'ofp_setup_document']:
    getattr(library,name).argtypes = [ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint32]
library.ofp_apply_setup.argtypes = [ctypes.c_void_p,ctypes.c_char_p]
handle = library.ofp_create(str(profile).encode(),Vec3(0,1,0),0,0)
assert handle, 'Rust vehicle creation failed'
def document(name):
    function = getattr(library,name)
    size = function(handle,None,0)
    assert size>0
    buffer = ctypes.create_string_buffer(size+1)
    function(handle,buffer,len(buffer))
    return buffer.value.decode()
try:
    initial = document('ofp_setup_document')
    descriptors = document('ofp_setup_fields')
    (args.output/'profile_document.json').write_text(initial)
    (args.output/'fields.tsv').write_text(descriptors)
    environment = os.environ.copy()
    environment['PATH'] = str(root/'distribution/bin')+os.pathsep+environment['PATH']
    database_path = args.output / ('validation-'+uuid.uuid4().hex+'.db')
    result = subprocess.run([str(args.build/'src/modules/userinterface/legacymenu/validate_formula_setup_database.exe'),
        str((args.output/'profile_document.json').resolve()),str((args.output/'fields.tsv').resolve()),
        str(database_path.resolve())],env=environment,capture_output=True,text=True)
    (args.output/'storage_validation.txt').write_text(result.stdout+result.stderr)
    print(result.stdout,end='')
    if result.returncode:
        print(result.stderr,end='')
        raise SystemExit(result.returncode)
    with sqlite3.connect(database_path) as connection:
        assert connection.execute('PRAGMA integrity_check').fetchone()[0] == 'ok'
        assert connection.execute('PRAGMA foreign_key_check').fetchall() == []
        assert connection.execute('SELECT count(*) FROM circuit').fetchone()[0] == 1
        assert connection.execute('SELECT count(*) FROM team').fetchone()[0] == 1
        values = json.loads(connection.execute('SELECT values_json FROM car_setup').fetchone()[0])
    assert library.ofp_apply_setup(handle,json.dumps(values).encode())==0, 'Saved setup rejected by Rust'
    values['aero.model.front_wing.incidence_deg'] = 12.5
    values['brakes.front_brake_bias'] = .615
    assert library.ofp_apply_setup(handle,json.dumps(values).encode())==0, 'Edited setup rejected by Rust'
    resolved = json.loads(document('ofp_setup_document'))['document']
    def flatten(value,prefix=''):
        if isinstance(value,dict):
            return {path:leaf for key,item in value.items() for path,leaf in flatten(item,prefix+'.'+key if prefix else key).items()}
        return {prefix:value}
    original = flatten(json.loads(initial)['document'])
    actual = flatten(resolved)
    assert all(actual[key]==value for key,value in original.items() if key not in values), 'Fixed configuration changed'
    before = document('ofp_setup_document')
    assert library.ofp_apply_setup(handle,b'{"tires.front.radius": 5}')!=0
    assert document('ofp_setup_document')==before, 'Rejected transaction mutated the car'
    assert hashlib.sha256(profile.read_bytes()).hexdigest()==profile_hash, 'Source profile changed'
    summary = dict(storage='sqlite',storage_checks='passed',database=str(database_path),editable_fields=len(values),rust_apply='passed',rust_edit='passed',
                   fixed_values_unchanged=True,rejected_transaction_unchanged=True,source_sha256=profile_hash)
    (args.output/'summary.json').write_text(json.dumps(summary,indent=2))
    print('RUST_SETUP_APPLY_EDIT_FIXED_PROFILE=PASS')
finally:
    library.ofp_destroy(handle)
