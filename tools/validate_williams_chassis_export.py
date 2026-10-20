import argparse
import json
import pathlib
import xml.etree.ElementTree as element_tree

import numpy

from convert_binary_scene import BinaryScene


def exported_vertices(path):
    lines = pathlib.Path(path).read_text().splitlines()
    vertices = []
    for line_index, line in enumerate(lines):
        if line.startswith('numvert '):
            vertex_count = int(line.split()[1])
            vertices.extend([[float(value) for value in point.split()] for point in lines[line_index + 1:line_index + 1 + vertex_count]])
    return numpy.asarray(vertices)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source-root', required=True)
    parser.add_argument('--vehicle-directory', required=True)
    parser.add_argument('--report', required=True)
    arguments = parser.parse_args()
    source_root = pathlib.Path(arguments.source_root)
    vehicle_directory = pathlib.Path(arguments.vehicle_directory)
    document = element_tree.parse(vehicle_directory / 'williams-nineteen-ninety-seven.xml').getroot()
    profile_directory = source_root / 'game/data/vehicles/f1_1997/williams/profiles/hockenheim'
    source_suspension = json.loads((profile_directory / 'suspension.json').read_text())['suspension']['geometry_physical']
    source_tires = json.loads((profile_directory / 'tires.json').read_text())['tires']
    source_chassis = json.loads((profile_directory / 'chassis.json').read_text())
    reference = json.loads(pathlib.Path(__file__).with_name('williams_suspension_reference.json').read_text())

    def number(section, name):
        return float(document.find(f"section[@name='{section}']/attnum[@name='{name}']").get('val'))

    reference_clearance = number('Front Right Wheel', 'ride height')
    source_ground_offset = source_tires['front']['radius'] - source_suspension['corners']['FL']['hub_center'][1]
    native_center_height = number('Car', 'GC height') + reference_clearance
    expected_center_height = source_ground_offset + source_chassis['chassis']['center_of_gravity_height_offset']
    if abs(native_center_height - expected_center_height) > 0.000001:
        raise ValueError('Ground-relative center of gravity differs from source')
    source_asset = source_root / 'game/assets/models/vehicles/f1-1997/williams/variants/hockenheim/villenueve/car/runtime/williams_fw19_hockenheim_1997_chassis.glb'
    source_vertices = numpy.concatenate([primitive[2] for primitive in BinaryScene(source_asset).primitives()])
    native_vertices = exported_vertices(vehicle_directory / 'williams-nineteen-ninety-seven.acc')
    expected_coordinates = numpy.column_stack((-source_vertices[:, 2], source_vertices[:, 1] + source_ground_offset, source_vertices[:, 0]))
    native_vertices[:, 1] += reference_clearance
    if native_vertices.shape != expected_coordinates.shape or not numpy.allclose(native_vertices, expected_coordinates, atol=0.000001, rtol=0.0):
        raise ValueError('Chassis world geometry differs from intended source pose')
    axle_records = []
    for axle_name, corner_name in [('front', 'FL'), ('rear', 'RL')]:
        corner = source_suspension['corners'][corner_name]
        axle = source_suspension[axle_name]
        motion_ratio = reference['axles'][axle_name]['motion_ratio']
        for side_name in ['Right', 'Left']:
            suspension_section = axle_name.title() + ' ' + side_name + ' Suspension'
            wheel_section = axle_name.title() + ' ' + side_name + ' Wheel'
            bump = number(wheel_section, 'ride height') - number(suspension_section, 'packers')
            droop = number(suspension_section, 'suspension course') - number(wheel_section, 'ride height')
            expected_stiffness = axle['spring_rate_N_per_m'] * motion_ratio ** 2
            if abs(bump - axle['wheel_bump_m']) > 0.000001 or abs(droop - axle['wheel_droop_m']) > 0.000001:
                raise ValueError('Wheel travel mapping mismatch: ' + wheel_section)
            if abs(number(suspension_section, 'spring') - expected_stiffness) > 0.001:
                raise ValueError('Equivalent wheel stiffness mismatch: ' + wheel_section)
            if abs(abs(number(wheel_section, 'ypos')) - abs(corner['hub_center'][0])) > 0.000001:
                raise ValueError('Wheel lateral center mismatch: ' + wheel_section)
            if abs(number(axle_name.title() + ' Axle', 'xpos') + corner['hub_center'][2]) > 0.000001:
                raise ValueError('Axle longitudinal center mismatch')
            source_hub_height = corner['hub_center'][1] + source_ground_offset
            if abs(source_hub_height - source_tires[axle_name]['radius']) > 0.000001:
                raise ValueError('Suspension visual hub and native rolling radius mismatch')
        axle_records.append({'axle': axle_name, 'motion_ratio': motion_ratio,
            'bump': axle['wheel_bump_m'], 'droop': axle['wheel_droop_m'], 'wheel_spring': expected_stiffness})
    report = {'world_center_of_gravity_height': native_center_height, 'world_chassis_minimum_height': float(native_vertices[:, 1].min()),
              'world_vertex_alignment_passed': True, 'all_four_wheel_centers_and_travel_passed': True,
              'vertices_checked': len(native_vertices), 'axles': axle_records, 'runtime_driving_validated': False}
    pathlib.Path(arguments.report).write_text(json.dumps(report, indent=2))
    print(json.dumps(report))


if __name__ == '__main__':
    main()
