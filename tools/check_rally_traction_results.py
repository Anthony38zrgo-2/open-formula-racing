"""Check measured suspension support and traction, not just DLL load success."""
from pathlib import Path
import csv
import json
import statistics

ROOT = Path(__file__).resolve().parents[1]
AUDIT = ROOT / 'staging_rally/traction_audit'

def rows(name):
    with (AUDIT / f'{name}.csv').open() as stream:
        return list(csv.DictReader(stream))

def at(data, step, wheel=2):
    return next(row for row in data if int(row['step']) == step and int(row['wheel']) == wheel)

before = rows('baseline')
flat = rows('final_flat')
rough_before = rows('original_rippled')
rough = rows('final_rippled')
road = rows('russia_accel_brake')
rest_before, rest_after = at(before, 249), at(flat, 249)
assert float(rest_before['travel']) > .10 and float(rest_before['stop']) > 1500
assert abs(float(rest_after['travel'])) < .03 and abs(float(rest_after['stop'])) < 1
assert float(rest_after['spring']) > 2000 and .8 < float(rest_after['motion_ratio']) < 1.3
assert float(at(flat, 599)['speed']) > 10
assert float(at(flat, 849)['speed']) > 2 * float(at(before, 849)['speed'])
assert float(at(rough, 849)['speed']) > 12
assert float(at(road, 599)['speed']) > 10 and float(at(road, 849)['speed']) < .1
assert all(int(row['body_contacts']) == 0 for row in road)
for step in (249, 599, 849):
    for wheel in (2, 3):
        row = at(road, step, wheel)
        assert float(row['load']) > 1000 and abs(float(row['stop'])) < 1

profile = ROOT / 'rust/data/vehicles/ford-escort-rs2000'
for destination in (ROOT / 'distribution/share/games/speed-dreams-2/cars/rust/ford-escort-rs2000',
                    ROOT / 'distribution/user-settings-rally/cars/rust/ford-escort-rs2000'):
    for path in profile.glob('*.json'):
        assert json.loads(path.read_text()) == json.loads((destination / path.name).read_text())

report = {
    'rear_rest_travel_before_mm': 1000 * float(rest_before['travel']),
    'rear_rest_travel_after_mm': 1000 * float(rest_after['travel']),
    'rear_rest_stop_force_before_n': float(rest_before['stop']),
    'rear_rest_stop_force_after_n': float(rest_after['stop']),
    'rear_rest_spring_force_before_n': float(rest_before['spring']),
    'rear_rest_spring_force_after_n': float(rest_after['spring']),
    'flat_speed_before_kmh': 3.6 * float(at(before, 849)['speed']),
    'flat_speed_after_kmh': 3.6 * float(at(flat, 849)['speed']),
    'rough_speed_before_kmh': 3.6 * float(at(rough_before, 849)['speed']),
    'rough_speed_after_kmh': 3.6 * float(at(rough, 849)['speed']),
    'russia_accelerated_speed_kmh': 3.6 * float(at(road, 599)['speed']),
    'russia_braked_speed_kmh': 3.6 * float(at(road, 849)['speed']),
    'russia_body_contacts': 0,
    'validation': 'passed',
}
(AUDIT / 'result_summary.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
