import argparse
import json
import pathlib
import subprocess
import sys

import numpy
from scipy.interpolate import CubicSpline

from convert_binary_scene import export_scene


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--native', required=True)
    parser.add_argument('--destination', required=True)
    parser.add_argument('--report', required=True)
    arguments = parser.parse_args()
    source_directory = pathlib.Path(arguments.source)
    candidate = json.loads(pathlib.Path(arguments.candidate).read_text())
    native = json.loads(pathlib.Path(arguments.native).read_text())
    source_centers = numpy.array(candidate['source_centers'])
    source_distances = numpy.array(candidate['source_distances'])
    source_spline = CubicSpline(source_distances, source_centers, bc_type='periodic')
    first_segment = native['segments'][0]
    native_origin = (numpy.array(first_segment['start_left']) + first_segment['start_right']) / 2
    simulation_rotation = numpy.eye(3)
    simulation_rotation[:2, :2] = candidate['rotation']
    source_origin = numpy.array(candidate['source_initial_center'])
    native_origin_offset = native_origin - simulation_rotation @ source_origin
    center_errors = []
    for index, segment in enumerate(native['segments']):
        native_center = (numpy.array(segment['start_left']) + segment['start_right']) / 2
        expected_center = simulation_rotation @ source_spline(candidate['candidate_segments'][index]['source_start_distance']) + native_origin_offset
        center_errors.append(native_center - expected_center)
    center_errors = numpy.array(center_errors)
    last_segment = native['segments'][-1]
    native_end = (numpy.array(last_segment['end_left']) + last_segment['end_right']) / 2
    closure = numpy.linalg.norm(native_end - native_origin)
    source_to_simulation = numpy.array([[0, 0, -1], [-1, 0, 0], [0, 1, 0]])
    simulation_to_asset = numpy.array([[1, 0, 0], [0, 0, 1], [0, -1, 0]])
    transformation = numpy.eye(4)
    transformation[:3, :3] = simulation_to_asset @ simulation_rotation @ source_to_simulation
    transformation[:3, 3] = simulation_to_asset @ native_origin_offset
    excluded = json.loads((source_directory / 'metadata/visual_profile.json').read_text())['excluded_instances']
    output_report = export_scene(source_directory / 'fuji76_77_visual.glb', arguments.destination, transformation, excluded)
    output_report.update({'native_origin': native_origin.tolist(), 'source_to_simulation': source_to_simulation.tolist(),
                          'simulation_rotation': simulation_rotation.tolist(), 'native_origin_offset': native_origin_offset.tolist(),
                          'closure_meters': float(closure), 'length_relative_error': abs(native['length'] / candidate['source_lap_length'] - 1),
                          'centerline_plan_error_percentiles': numpy.percentile(numpy.linalg.norm(center_errors[:, :2], axis=1), [50, 95, 100]).tolist(),
                          'centerline_height_error_percentiles': numpy.percentile(numpy.abs(center_errors[:, 2]), [50, 95, 100]).tolist(),
                          'full_road_surface_validated': False, 'curbs_validated': False, 'walls_validated': False})
    pathlib.Path(arguments.report).write_text(json.dumps(output_report, indent=2))
    subprocess.run([sys.executable, str(pathlib.Path(__file__).with_name('prepare_fuji_object_collisions.py')),
                    '--source', str(source_directory / 'fuji76_77_collision.glb'), '--alignment', arguments.report,
                    '--destination', str(pathlib.Path(arguments.destination).with_name('fuji_object_collisions.txt'))], check=True)
    print(json.dumps({key: value for key, value in output_report.items() if key not in ['primitive_bounds']}, indent=2))


if __name__ == '__main__':
    main()
