"""Offline ABI + actual Melbourne spawn smoke; never launches/overwrites a game log."""
import argparse
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import re
import sqlite3
import time

ROOT=Path(__file__).resolve().parents[3]
parser=argparse.ArgumentParser()
parser.add_argument('--dll',type=Path,default=ROOT/'rust/target/release/vehicle_physics_engine.dll')
args=parser.parse_args()
source=(ROOT/'rust/vehicle-physics-engine/src/open_formula_ffi.rs').read_text(encoding='utf-8')
types={'f64':C.c_double,'i32':C.c_int32,'u32':C.c_uint32,'u64':C.c_uint64}
counts={'OFP_WHEELS':4,'OFP_FLOOR_PROBES':2,'OFP_MAX_CONTACTS':32}
for name in ['OfpVec3','OfpVehicleSpecs','OfpControlInput','OfpBodyContact','OfpWheelState','OfpSnapshot']:
    body=re.search(r'pub struct '+name+r' \{(.*?)\n\}',source,re.S).group(1)
    fields=[]
    for field,kind in re.findall(r'^    pub (\w+): ([^,\n]+),',body,re.M):
        if kind.startswith('['):
            item,count=kind[1:-1].split('; ')
            kind=types[item]*(counts[count] if count in counts else int(count))
        else: kind=types[kind]
        fields.append((field,kind))
    types[name]=type(name,(C.Structure,),{'_fields_':fields})
V,I,S=(types[k] for k in ['OfpVec3','OfpControlInput','OfpSnapshot'])
dll=C.CDLL(str(args.dll.resolve()))
dll.ofp_abi_signature.restype=C.c_uint64
signature=0xcbf29ce484222325
layout=re.search(r'fn ofp_abi_signature\(\).*?\n\}',source,re.S).group()
for operation,struct,field in re.findall(r'std::mem::(size_of|offset_of)(?:::)?(?:<)?!?\(?<?(Ofp\w+)(?:, (\w+))?',layout):
    value=C.sizeof(types[struct]) if operation=='size_of' else getattr(types[struct],field).offset
    signature=((signature^value)*0x100000001b3)&((1<<64)-1)
assert dll.ofp_abi_signature()==signature
dll.ofp_create.argtypes=[C.c_char_p,V,C.c_double,C.c_double];dll.ofp_create.restype=C.c_void_p
dll.ofp_destroy.argtypes=[C.c_void_p]
dll.ofp_load_track_ground_mesh.argtypes=[C.c_char_p]
dll.ofp_step.argtypes=[C.c_void_p,C.POINTER(I),C.c_void_p,C.c_uint32,C.c_double]
dll.ofp_snapshot.argtypes=[C.c_void_p,C.POINTER(S)]
dll.ofp_last_error.argtypes=[C.c_void_p,C.c_void_p,C.c_uint32]
dll.ofp_apply_setup.argtypes=[C.c_void_p,C.c_char_p]
profile=ROOT/'distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/vehicle.json'
mesh=ROOT/'distribution/share/games/speed-dreams-2/tracks/circuit/melbourne-nineteen-ninety-seven/melbourne_ground_collisions.txt'
assert dll.ofp_load_track_ground_mesh(str(mesh).encode())>0
with sqlite3.connect((ROOT/'distribution/user-settings/open_formula.db').as_uri()+'?mode=ro',uri=True) as db:
    setups=[(name,json.loads(values)) for name,values in db.execute('select name,values_json from car_setup')]
results=[]
for name,setup in [('base',None),*setups]:
    handle=dll.ofp_create(str(profile).encode(),V(-1007.681,1.871,-829.024),-0.005,0.)
    assert handle,name
    try:
        if setup: assert dll.ofp_apply_setup(handle,json.dumps(setup).encode())==0
        control=I(clutch=1.,gear_request=0)
        start=time.perf_counter()
        for step in range(240):
            status=dll.ofp_step(handle,C.byref(control),None,0,1./240.)
            if status:
                error=C.create_string_buffer(4096);dll.ofp_last_error(handle,error,len(error))
                raise AssertionError((name,step,error.value.decode()))
        duration=time.perf_counter()-start
        snapshot=S();assert dll.ofp_snapshot(handle,C.byref(snapshot))==0
        assert snapshot.rejected_steps==0 and abs(snapshot.sim_time_s-1.)<1e-10
        assert all(math.isfinite(w.normal_force_n) and w.normal_force_n>=0 for w in snapshot.wheels)
        assert sum(w.grounded for w in snapshot.wheels)==4,(name,'spawn did not settle on four wheels')
        results.append({'setup':name,'accepted_steps':240,'simulated_seconds':snapshot.sim_time_s,
                        'wall_seconds':duration,'normal_loads_n':[w.normal_force_n for w in snapshot.wheels],
                        'mean_wall_ms_per_step':duration/240*1000})
    finally: dll.ofp_destroy(handle)
receipt={'dll':str(args.dll),'sha256':hashlib.sha256(args.dll.read_bytes()).hexdigest(),
         'abi_signature':hex(signature),'actual_Melbourne_spawn':results,'manual_T3':'pending_user'}
(Path(__file__).resolve().parent/'smoke.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
print(json.dumps(receipt,indent=2))
