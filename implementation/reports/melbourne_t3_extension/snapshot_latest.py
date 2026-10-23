"""Freeze the latest completed runtime log and read-only setup context."""
import csv
import hashlib
import json
import re
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent / 'latest_telemetry'
OUT.mkdir(exist_ok=True)
source = ROOT / 'distribution/user-settings/runtime.log'
raw = source.read_bytes()
text = raw.decode('utf-8', errors='replace')
assert 'Exiting normally from Speed Dreams.' in text, 'latest session has not closed normally'
assert not (OUT / 'runtime.log').exists(), 'snapshot already exists; preserve it'
(OUT / 'runtime.log').write_bytes(raw)
rows = []
episode = 0
previous_time = -1
for line_number, line in enumerate(text.splitlines(), 1):
    if 'RUSTDIAG time=' not in line:
        continue
    fields = dict(re.findall(r'(\w+)=(\([^)]*\)|[^ ]+)', line))
    time = float(fields['time'])
    if time < previous_time:
        episode += 1
    previous_time = time
    row = {'session_segment': episode, 'source_line': line_number, 'time_s': time}
    for key in ['speed', 'yaw', 'pitch', 'roll', 'rejected', 'backends', 'rays']:
        row[key] = fields[key]
    for key in ['pos', 'N', 'travel', 'contact', 'floor_impulse']:
        labels = ['x', 'y', 'z'] if key == 'pos' else ['FL', 'FR', 'RL', 'RR'] if key in ['N', 'travel', 'contact'] else ['front', 'rear']
        for label, value in zip(labels, fields[key][1:-1].split(',')):
            row[key + '_' + label] = float(value)
    rows.append(row)
with (OUT / 'samples.csv').open('w', newline='', encoding='utf-8') as stream:
    writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)
with sqlite3.connect((ROOT / 'distribution/user-settings/open_formula.db').as_uri() + '?mode=ro', uri=True) as db:
    db.row_factory = sqlite3.Row
    setups = [dict(row) for row in db.execute('select * from car_setup')]
    for row in setups:
        row['values'] = json.loads(row.pop('values_json'))
(OUT / 'saved_setups.json').write_text(json.dumps(setups, indent=2))
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
profile_dir = ROOT / 'distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim'
profiles = {p.name: json.loads(p.read_text()) for p in sorted(profile_dir.glob('*.json'))}
(OUT / 'installed_profile_documents.json').write_text(json.dumps(profiles, indent=2))
runs, current = [], []
for row in rows:
    unloaded = row['contact_FR'] == 0 and row['N_FR'] == 0 and float(row['speed']) > 5 and row['N_FL'] > 0
    if current and (not unloaded or row['session_segment'] != current[-1]['session_segment']):
        runs.append(current)
        current = []
    if unloaded:
        current.append(row)
if current:
    runs.append(current)
summary = {
    'captured_utc': datetime.now(timezone.utc).isoformat(),
    'runtime_sha256': hashlib.sha256(raw).hexdigest(), 'runtime_size_bytes': len(raw),
    'log_session_start': re.search(r'Date and time : (.+)', text).group(1),
    'sample_count': len(rows),
    'installed_dll_sha256_at_capture': sha(ROOT / 'distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll'),
    'profile_sha256_at_capture': {p.name: sha(p) for p in sorted(profile_dir.glob('*.json'))},
    'setup_events': [line for line in text.splitlines() if 'Formula garage setup:' in line or 'Formula reglaje' in line],
    'FR_zero_support_FL_positive_sample_runs': [
        {'session_segment': run[0]['session_segment'], 'start_s': run[0]['time_s'], 'last_zero_sample_s': run[-1]['time_s'],
         'sample_span_s': run[-1]['time_s'] - run[0]['time_s'], 'sample_count': len(run),
         'source_lines': [r['source_line'] for r in run], 'start_position_sd': [run[0]['pos_' + axis] for axis in ['x', 'y', 'z']]}
        for run in runs],
    'limitations': ['Log snapshots do not prove continuous loss of contact between samples.',
                   'Saved setups and installed profile are capture-time state, not a history of each live setup change.',
                   'DLL hash is measured at capture, not recorded by the historical log itself.',
                   'Wheel order in RUSTDIAG is FL, FR, RL, RR. travel is absolute suspension compression, not droop from design.']}
(OUT / 'capture.json').write_text(json.dumps(summary, indent=2))
print(json.dumps({'sample_count':len(rows), 'sha256':summary['runtime_sha256'],
                  'session_start':summary['log_session_start'], 'unloaded_runs':summary['FR_zero_support_FL_positive_sample_runs']}, indent=2))
