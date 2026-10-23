"""Read-only diagnosis against the installed physics DLL; writes only this folder."""
import ctypes as C
import hashlib
import json
import re
import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
source = (ROOT / 'rust/vehicle-physics-engine/src/open_formula_ffi.rs').read_text()
types = {'f64': C.c_double, 'i32': C.c_int32, 'u32': C.c_uint32, 'u64': C.c_uint64}
counts = {'OFP_WHEELS': 4, 'OFP_FLOOR_PROBES': 2, 'OFP_MAX_CONTACTS': 32}
for name in ['OfpVec3', 'OfpVehicleSpecs', 'OfpControlInput', 'OfpBodyContact', 'OfpWheelState', 'OfpSnapshot']:
    body = re.search(r'pub struct ' + name + r' \{(.*?)\n\}', source, re.S).group(1)
    fields = []
    for field, kind in re.findall(r'^    pub (\w+): ([^,\n]+),', body, re.M):
        if kind.startswith('['):
            item, count = kind[1:-1].split('; ')
            kind = types[item] * (counts[count] if count in counts else int(count))
        else:
            kind = types[kind]
        fields.append((field, kind))
    types[name] = type(name, (C.Structure,), {'_fields_': fields})
V, I, S = (types[n] for n in ['OfpVec3', 'OfpControlInput', 'OfpSnapshot'])
dllpath = ROOT / 'distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll'
dll = C.CDLL(str(dllpath))
dll.ofp_abi_signature.restype = C.c_uint64
# Reproduce the exported layout signature before reading/writing ABI memory.
layout = re.search(r'fn ofp_abi_signature\(\).*?\n\}', source, re.S).group()
signature = 0xcbf29ce484222325
for operation, struct, field in re.findall(r'std::mem::(size_of|offset_of)(?:::)?(?:<)?!?\(?<?(Ofp\w+)(?:, (\w+))?', layout):
    value = C.sizeof(types[struct]) if operation == 'size_of' else getattr(types[struct], field).offset
    signature = ((signature ^ value) * 0x100000001b3) & ((1 << 64) - 1)
assert signature == dll.ofp_abi_signature(), (hex(signature), hex(dll.ofp_abi_signature()))
dll.ofp_create.argtypes = [C.c_char_p, V, C.c_double, C.c_double]
dll.ofp_create.restype = C.c_void_p
dll.ofp_destroy.argtypes = [C.c_void_p]
dll.ofp_snapshot.argtypes = [C.c_void_p, C.POINTER(S)]
dll.ofp_step.argtypes = [C.c_void_p, C.POINTER(I), C.c_void_p, C.c_uint32, C.c_double]
dll.ofp_apply_setup.argtypes = [C.c_void_p, C.c_char_p]
dll.ofp_setup_document.argtypes = [C.c_void_p, C.c_void_p, C.c_uint32]
dll.ofp_load_track_ground_mesh.argtypes = [C.c_char_p]
dll.ofp_build_provenance.restype = C.c_char_p
profile = ROOT / 'distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/vehicle.json'
with sqlite3.connect((ROOT / 'distribution/user-settings/open_formula.db').as_uri() + '?mode=ro', uri=True) as db:
    name, setup_text = db.execute('select name, values_json from car_setup where id=1').fetchone()
setup = json.loads(setup_text)
handle = dll.ofp_create(str(profile).encode(), V(0, 0.7, 0), 0, 20)
assert handle
size = dll.ofp_setup_document(handle, None, 0)
buffer = C.create_string_buffer(size + 1)
dll.ofp_setup_document(handle, buffer, len(buffer))
base = json.loads(buffer.value)['document']
dll.ofp_destroy(handle)

# A split-height road excites the axle's differential mode without human inputs.
# Ground document coordinates are Speed Dreams: X forward, Y left, Z up.
triangles = []
for y0, y1, z in [(-10, 0, 0), (0, 10, 0.03)]:
    a, b, c, d = (-30, y0, z), (30, y0, z), (30, y1, z), (-30, y1, z)
    for triangle in [(a, b, c), (a, c, d)]:
        triangles.append('1.0 0.01 ' + ' '.join(str(x) for point in triangle for x in point))
ground = OUT / 'split_height_ground.txt'
ground.write_text('FORMULA_GROUND_1\n4\n' + '\n'.join(triangles) + '\n')
assert dll.ofp_load_track_ground_mesh(str(ground).encode()) == 4

def setpath(document, path, value):
    parts = path.split('.')
    for part in parts[:-1]:
        document = document[part]
    document[parts[-1]] = value

def run(label, patch=None, physical_patch=None):
    path = profile
    if physical_patch:
        document = json.loads(json.dumps(base))
        for key, value in setup.items():
            setpath(document, key, value)
        for key, value in physical_patch.items():
            setpath(document, key, value)
        path = OUT / (label + '.json')
        path.write_text(json.dumps(document))
    handle = dll.ofp_create(str(path).encode(), V(0, 0.7, 0), 0, 20)
    assert handle, label
    try:
        if not physical_patch:
            values = dict(setup)
            values.update(patch or {})
            assert dll.ofp_apply_setup(handle, json.dumps(values).encode()) == 0
            size = dll.ofp_setup_document(handle, None, 0)
            readback = C.create_string_buffer(size + 1)
            dll.ofp_setup_document(handle, readback, len(readback))
            resolved = json.loads(readback.value)['document']
            for key, value in values.items():
                leaf = resolved
                for part in key.split('.'):
                    leaf = leaf[part]
                assert leaf == value, (label, key, leaf, value)
        trajectory = []
        control = I(clutch=1.0, gear_request=0)
        for step in range(480):
            assert dll.ofp_step(handle, C.byref(control), None, 0, 1 / 480) == 0, (label, step)
            snapshot = S()
            assert dll.ofp_snapshot(handle, C.byref(snapshot)) == 0
            row = [snapshot.position.x, snapshot.position.y, snapshot.position.z]
            for wheel in snapshot.wheels:
                row.extend([wheel.suspension_travel_m, wheel.suspension_velocity_m_s,
                            wheel.spring_force_n, wheel.damper_force_n, wheel.normal_force_n])
            trajectory.append(row)
        return trajectory
    finally:
        dll.ofp_destroy(handle)

reference = run('saved_setup')
experiments = {
    'front_rebound_0_2': ({'suspension.front.rebound_damp_multiplier': 0.2}, None),
    'front_rebound_3': ({'suspension.front.rebound_damp_multiplier': 3.0}, None),
    'front_damping_0_05': ({'suspension.front.damping_ratio': 0.05}, None),
    'front_bump_0_2': ({'suspension.front.bump_damp_multiplier': 0.2}, None),
    'front_stop_4': ({'suspension.front.bump_stop_multiplier': 4.0}, None),
    'garage_arb_0': ({'suspension.geometry_physical.front_arb.ratio': 0.0}, None),
    'garage_arb_2': ({'suspension.geometry_physical.front_arb.ratio': 2.0}, None),
    'physical_rebound_half': (None, {'suspension.geometry_physical.front.damper_rebound_Ns_per_m': 5737.5}),
    'actual_arb_0': (None, {'suspension.front.arb_ratio': 0.0}),
    'actual_arb_0_2': (None, {'suspension.front.arb_ratio': 0.2}),
}
results = {}
for label, (patch, physical_patch) in experiments.items():
    result = run(label, patch, physical_patch)
    results[label] = {'maximum_absolute_difference': max(abs(a - b) for ra, rb in zip(reference, result) for a, b in zip(ra, rb)),
                      'identical_numeric_trajectory': result == reference,
                      'maximum_front_right_compression_difference_m': max(abs(a[8] - b[8]) for a, b in zip(reference, result)),
                      'maximum_front_right_damper_force_difference_n': max(abs(a[11] - b[11]) for a, b in zip(reference, result)),
                      'maximum_front_right_normal_force_difference_n': max(abs(a[12] - b[12]) for a, b in zip(reference, result))}
    print(label, results[label], flush=True)

log = (ROOT / 'distribution/user-settings/runtime.log').read_text(errors='replace')
(OUT / 'runtime_excerpt.log').write_text('\n'.join(line for line in log.splitlines()
    if 'RUSTDIAG time=3' in line or 'Formula garage setup:' in line or 'Formula reglaje' in line))
samples = []
for line in log.splitlines():
    if 'RUSTDIAG time=' not in line:
        continue
    def scalar(key):
        return float(re.search(r'\b' + key + r'=([-0-9.]+)', line).group(1))
    def vector(key):
        return [float(x) for x in re.search(r'\b' + key + r'=\(([^)]+)\)', line).group(1).split(',')]
    samples.append({'time': scalar('time'), 'position_sd': vector('pos'), 'speed': scalar('speed'),
                    'yaw': scalar('yaw'), 'roll': scalar('roll'), 'normal_n': vector('N'),
                    'compression_m': vector('travel'), 'contact': vector('contact'),
                    'floor_impulse_ns': vector('floor_impulse')})
runs = []
current = []
for sample in samples:
    if sample['contact'][1] == 0 and sample['normal_n'][1] == 0 and sample['speed'] > 5:
        if current and sample['time'] < current[-1]['time']:
            runs.append(current)
            current = []
        current.append(sample)
    elif current:
        runs.append(current)
        current = []
if current:
    runs.append(current)
summary = {'dll_sha256': hashlib.sha256(dllpath.read_bytes()).hexdigest(),
           'dll_provenance': dll.ofp_build_provenance().decode(), 'abi_signature': hex(signature),
           'setup_name': name, 'saved_setup_values': setup,
           'experiment': '480 steps at 480 Hz, drop on split-height road, no steering, neutral, clutch disengaged',
           'experiments': results,
           'front_right_unloaded_sample_runs': runs,
           'limitations': ['Synthetic rig is not a replay of Melbourne T3.',
                           'Runtime log samples every approximately 0.25 s; contact between samples is unknown.',
                           'Current saved setup does not establish all historical live values.']}
(OUT / 'results.json').write_text(json.dumps(summary, indent=2))
print('FR unloaded sample runs:', [(r[0]['time'], r[-1]['time'], len(r)) for r in runs])
