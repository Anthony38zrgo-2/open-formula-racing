"""Verification bank for the garage-setup authority fix.

Runs the rebuilt DLL (rust/target/release) next to the installed DLL (still the
pre-fix build 9c91648) on the same synthetic rig `diagnose.py` used, and answers four
questions with numbers:

1. Regression: does the design profile (no reglaje applied) produce a bit-identical
   trajectory on the fixed DLL as on the installed one?
2. Response: does every suspension control now change the trajectory?
3. Purity: does each control resolve to exactly the physical element the fix documents
   (proved by comparing against a profile that states that element directly)?
4. Idempotence: does replaying the same reglaje twice land on the same trajectory?

Writes only implementation/reports/melbourne_t3_extension/verification/.
Reads the installed SQLite regraje in mode=ro; never modifies profiles, the database
or the installed DLL.
"""
import ctypes as C
import argparse
import hashlib
import json
import re
import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument('--reference-dll', type=Path, default=ROOT / 'distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll')
parser.add_argument('--candidate-dll', type=Path, default=ROOT / 'rust/target/release/vehicle_physics_engine.dll')
args = parser.parse_args()
OUT = Path(__file__).resolve().parent / 'dll_verification'
OUT.mkdir(parents=True, exist_ok=True)

TYPES = {'f64': C.c_double, 'i32': C.c_int32, 'u32': C.c_uint32, 'u64': C.c_uint64}
COUNTS = {'OFP_WHEELS': 4, 'OFP_FLOOR_PROBES': 2, 'OFP_MAX_CONTACTS': 32}
FFI_SOURCE = (ROOT / 'rust/vehicle-physics-engine/src/open_formula_ffi.rs').read_text()


def build_structs():
    types = dict(TYPES)
    for name in ['OfpVec3', 'OfpVehicleSpecs', 'OfpControlInput', 'OfpBodyContact',
                 'OfpWheelState', 'OfpSnapshot']:
        body = re.search(r'pub struct ' + name + r' \{(.*?)\n\}', FFI_SOURCE, re.S).group(1)
        fields = []
        for field, kind in re.findall(r'^    pub (\w+): ([^,\n]+),', body, re.M):
            if kind.startswith('['):
                item, count = kind[1:-1].split('; ')
                kind = types[item] * (COUNTS[count] if count in COUNTS else int(count))
            else:
                kind = types[kind]
            fields.append((field, kind))
        types[name] = type(name, (C.Structure,), {'_fields_': fields})
    return types


TYPES_BUILDER = build_structs()
V, I, S = (TYPES_BUILDER[n] for n in ['OfpVec3', 'OfpControlInput', 'OfpSnapshot'])


def load(path):
    """Open one physics DLL and prove its ABI layout before touching its memory."""
    dll = C.CDLL(str(path))
    dll.ofp_abi_signature.restype = C.c_uint64
    layout = re.search(r'fn ofp_abi_signature\(\).*?\n\}', FFI_SOURCE, re.S).group()
    signature = 0xcbf29ce484222325
    for operation, struct, field in re.findall(
            r'std::mem::(size_of|offset_of)(?:::)?(?:<)?!?\(?<?(Ofp\w+)(?:, (\w+))?', layout):
        value = (C.sizeof(TYPES_BUILDER[struct]) if operation == 'size_of'
                 else getattr(TYPES_BUILDER[struct], field).offset)
        signature = ((signature ^ value) * 0x100000001b3) & ((1 << 64) - 1)
    assert signature == dll.ofp_abi_signature(), (hex(signature), hex(dll.ofp_abi_signature()))
    dll.ofp_create.argtypes = [C.c_char_p, V, C.c_double, C.c_double]
    dll.ofp_create.restype = C.c_void_p
    dll.ofp_destroy.argtypes = [C.c_void_p]
    dll.ofp_snapshot.argtypes = [C.c_void_p, C.POINTER(S)]
    dll.ofp_step.argtypes = [C.c_void_p, C.POINTER(I), C.c_void_p, C.c_uint32, C.c_double]
    dll.ofp_apply_setup.argtypes = [C.c_void_p, C.c_char_p]
    dll.ofp_reset_setup.argtypes = [C.c_void_p]
    dll.ofp_setup_document.argtypes = [C.c_void_p, C.c_void_p, C.c_uint32]
    dll.ofp_load_track_ground_mesh.argtypes = [C.c_char_p]
    dll.ofp_last_error.argtypes = [C.c_void_p, C.c_void_p, C.c_uint32]
    dll.ofp_build_provenance.restype = C.c_char_p
    return dll, hex(signature)


INSTALLED, installed_signature = load(args.reference_dll)
FIXED, fixed_signature = load(args.candidate_dll)
assert installed_signature == fixed_signature, 'the two builds must share one ABI layout'

PROFILE = ROOT / 'distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/vehicle.json'
GROUND = Path(__file__).resolve().parent / 'split_height_ground.txt'
assert GROUND.exists(), 'run diagnose.py first: this bank reuses its synthetic step'
with sqlite3.connect((ROOT / 'distribution/user-settings/open_formula.db').as_uri() + '?mode=ro',
                     uri=True) as db:
    row = db.execute('select name, values_json from car_setup where id=1').fetchone()
assert row, 'no regraje id=1 in the installed database'
SETUP_NAME, SETUP = row[0], json.loads(row[1])


def document_of(dll, profile_path=None):
    handle = dll.ofp_create(str(profile_path or PROFILE).encode(), V(0, 0.7, 0), 0, 20)
    assert handle
    try:
        size = dll.ofp_setup_document(handle, None, 0)
        buffer = C.create_string_buffer(size + 1)
        assert dll.ofp_setup_document(handle, buffer, len(buffer)) > 0
        return json.loads(buffer.value)
    finally:
        dll.ofp_destroy(handle)


BASE = document_of(FIXED)
assert BASE['base'] == document_of(INSTALLED)['base'], 'both DLLs must load one profile'


def setpath(document, path, value):
    parts = path.split('.')
    for part in parts[:-1]:
        document = document[part]
    document[parts[-1]] = value


def write_profile(label, overrides):
    """A profile document that states physical elements directly (no regraje)."""
    document = json.loads(json.dumps(BASE['document']))
    for key, value in overrides.items():
        setpath(document, key, value)
    path = OUT / (label + '.json')
    path.write_text(json.dumps(document))
    return path


def run(dll, label, patch=None, profile=None, replay=None, height=0.7):
    """One 1 s drop on the split-height step; returns 480 rows of 23 columns."""
    assert dll.ofp_load_track_ground_mesh(str(GROUND).encode()) == 4
    handle = dll.ofp_create(str(profile or PROFILE).encode(), V(0, height, 0), 0, 20)
    assert handle, label
    try:
        for values in [patch] + (replay or []):
            if values is None:
                continue
            status = dll.ofp_apply_setup(handle, json.dumps(values).encode())
            if status != 0:
                error = C.create_string_buffer(4096)
                dll.ofp_last_error(handle, error, len(error))
                raise AssertionError((label, error.value.decode()))
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


def compare(reference, result):
    """Wheels are ordered FL, FR, RL, RR in the ABI snapshot."""
    return {
        'maximum_absolute_difference': max(abs(a - b) for ra, rb in zip(reference, result)
                                           for a, b in zip(ra, rb)),
        'identical_numeric_trajectory': result == reference,
        'maximum_front_right_compression_difference_m': max(abs(a[8] - b[8])
                                                             for a, b in zip(reference, result)),
        'maximum_front_right_damper_force_difference_n': max(abs(a[11] - b[11])
                                                             for a, b in zip(reference, result)),
        'maximum_front_right_normal_force_difference_n': max(abs(a[12] - b[12])
                                                             for a, b in zip(reference, result)),
    }


def unloaded(reference, result):
    """Samples with the front right wheel carrying no load, on the step the bank drops."""
    return {'reference': sum(1 for row in reference if row[12] <= 0.0),
            'result': sum(1 for row in result if row[12] <= 0.0)}


results = {}
design = run(FIXED, 'design')
installed_design = run(INSTALLED, 'design_installed')

# 1. Regression: a car nobody touched must not change at all.
results['regression_design_builds_agree'] = compare(installed_design, design) | {
    'installed_dll': INSTALLED.ofp_build_provenance().decode(),
    'fixed_dll': FIXED.ofp_build_provenance().decode(),
}

# The design trims of the installed profile: the reference the fix translates against.
DESIGN = {
    'front': {'damping_ratio': 0.74, 'bump': 1.0, 'rebound': 1.12, 'stop': 2.6,
              'damper_bump': 6970.0, 'damper_rebound': 11475.0, 'arb': 1.00},
    'rear': {'damping_ratio': 0.71, 'bump': 0.9, 'rebound': 1.35, 'stop': 2.8,
             'damper_bump': 14000.0, 'damper_rebound': 22000.0, 'arb': 0.80},
}

# 2 and 3. Response and purity: each control against its own physical element.
CONTROLS = {
    'front_rebound': ('suspension.front.rebound_damp_multiplier', 2.24, 'front',
                      {'suspension.geometry_physical.front.damper_rebound_Ns_per_m': 22950.0}),
    'front_bump': ('suspension.front.bump_damp_multiplier', 2.0, 'front',
                   {'suspension.geometry_physical.front.damper_bump_Ns_per_m': 13940.0}),
    'front_damping': ('suspension.front.damping_ratio', 0.37, 'front',
                      {'suspension.geometry_physical.front.damper_bump_Ns_per_m': 3485.0,
                       'suspension.geometry_physical.front.damper_rebound_Ns_per_m': 5737.5}),
    'front_stop': ('suspension.front.bump_stop_multiplier', 1.3, 'front',
                   {'suspension.geometry_physical.front.bump_stop_mult': 1.3}),
    'front_arb': ('suspension.geometry_physical.front_arb.ratio', 0.2, 'front',
                  {'suspension.front.arb_ratio': 0.2}),
    'rear_rebound': ('suspension.rear.rebound_damp_multiplier', 2.7, 'rear',
                     {'suspension.geometry_physical.rear.damper_rebound_Ns_per_m': 44000.0}),
    'rear_bump': ('suspension.rear.bump_damp_multiplier', 1.8, 'rear',
                  {'suspension.geometry_physical.rear.damper_bump_Ns_per_m': 28000.0}),
    'rear_damping': ('suspension.rear.damping_ratio', 1.06, 'rear',
                     {'suspension.geometry_physical.rear.damper_bump_Ns_per_m': 14000.0 * (1.06 / 0.71),
                      'suspension.geometry_physical.rear.damper_rebound_Ns_per_m': 22000.0 * (1.06 / 0.71)}),
    'rear_stop': ('suspension.rear.bump_stop_multiplier', 1.4, 'rear',
                  {'suspension.geometry_physical.rear.bump_stop_mult': 1.4}),
    'rear_arb': ('suspension.geometry_physical.rear_arb.ratio', 0.4, 'rear',
                 {'suspension.rear.arb_ratio': 0.4}),
}
for label, (path, value, axle, physical) in CONTROLS.items():
    height = 1.3 if label == 'front_stop' else 0.7
    reference_design = run(FIXED, label + '_design', height=height) if height != 0.7 else design
    moved = run(FIXED, label + '_garage', patch={path: value}, height=height)
    reference_element = run(FIXED, label + '_element', profile=write_profile(label + '_element', physical), height=height)
    results[label] = compare(reference_design, moved) | {
        'initial_height_m': height,
        'control': path,
        'requested': value,
        'resolves_to': physical,
        'design_axle': axle,
        # Purity, not "nothing else moved": the trajectory must equal the one of a
        # profile that states exactly this element, so the control can only have
        # translated into that element (chassis coupling still reaches the other axle).
        'matches_the_equivalent_physical_element': moved == reference_element,
        'front_right_unloaded_samples': unloaded(reference_design, moved),
    }

# 4. Idempotence: the stored regraje applied twice is the regraje applied once.
saved_once = run(FIXED, 'regraje_once', patch=SETUP)
saved_twice = run(FIXED, 'regraje_twice', patch=SETUP, replay=[SETUP])
results['replayed_saved_regraje'] = compare(saved_once, saved_twice) | {
    'setup_name': SETUP_NAME, 'editable_fields': len(SETUP)}

# The user's actual regraje: installed (pre-fix, controls inert) against fixed.
saved_installed = run(INSTALLED, 'regraje_installed', patch=SETUP)
results['saved_regraje_installed_versus_fixed'] = compare(saved_installed, saved_once) | {
    'front_right_unloaded_samples': unloaded(saved_installed, saved_once),
    'note': 'the same stored values, before and after the connection fix'}

summary = {
    'rig': '480 steps at 480 Hz, drop on a 30 mm split-height step, no steering, '
           'neutral, clutch disengaged, origin (0, 0.7, 0), 20 kg fuel; '
           'front stop uses height 1.3 m to excite the stop',
    'wheel_order_in_rows': 'FL, FR, RL, RR (ABI snapshot order)',
    'design_reference': DESIGN,
    'saved_regraje_values': SETUP,
    'dlls': {
        'installed': {'sha256': hashlib.sha256(
            args.reference_dll
            .read_bytes()).hexdigest(),
            'provenance': INSTALLED.ofp_build_provenance().decode()},
        'fixed': {'sha256': hashlib.sha256(
            args.candidate_dll.read_bytes()).hexdigest(),
            'provenance': FIXED.ofp_build_provenance().decode()},
    },
    'results': results,
    'limitations': [
        'This synthetic step is not a replay of Melbourne T3: it proves that each control '
        'now reaches its own element and how the front right wheel responds to a 30 mm '
        'side offset, not lap time or the corner itself.',
        'Snapshots are one per accepted step; contact between samples is unknown.',
        'The stored regraje carries the values saved before the fix, so applying it now also '
        'applies its bump stop and anti-roll requests.',
    ],
}
(OUT / 'verification.json').write_text(json.dumps(summary, indent=2))

assert results['regression_design_builds_agree']['identical_numeric_trajectory']
assert results['replayed_saved_regraje']['identical_numeric_trajectory']
for label in CONTROLS:
    assert results[label]['matches_the_equivalent_physical_element'], (label, 'wrong physical element')
    assert not results[label]['identical_numeric_trajectory'], (label, 'fixture did not excite control')

print(f"{'caso':<38} {'Δ máx':>12} {'idéntico':>9} {'FR sin muestra (base→caso)':>26}")
for label, data in results.items():
    samples = data.get('front_right_unloaded_samples') or {}
    text = f"{samples.get('reference', '-')}→{samples.get('result', '-')}" if samples else '-'
    print(f"{label:<38} {data['maximum_absolute_difference']:>12.4f} "
          f"{str(data['identical_numeric_trajectory']):>9} {text:>26}")
    if 'matches_the_equivalent_physical_element' in data:
        print(f"{'':<38} coincide con el elemento físico equivalente: "
              f"{data['matches_the_equivalent_physical_element']}")
