import argparse
import json
import math
import pathlib
import sys
import xml.etree.ElementTree as element_tree

import numpy
from scipy.interpolate import CubicSpline

from convert_binary_scene import BinaryScene, export_scene


def cross_section(road_triangles, position, left_direction, declared_left_width, declared_right_width):
    relative = road_triangles[:, :, :2] - position[:2]
    tangent_direction = numpy.array([left_direction[1], -left_direction[0]])
    across_coordinates = relative @ left_direction
    along_coordinates = relative @ tangent_direction
    following_along = numpy.roll(along_coordinates, -1, axis=1)
    following_across = numpy.roll(across_coordinates, -1, axis=1)
    following_height = numpy.roll(road_triangles[:, :, 2], -1, axis=1)
    denominator = following_along - along_coordinates
    crossing = (along_coordinates * following_along <= 0) & (numpy.abs(denominator) > 1e-8)
    fraction = numpy.divide(-along_coordinates, denominator, out=numpy.zeros_like(denominator), where=numpy.abs(denominator) > 1e-8)
    intersections = across_coordinates + fraction * (following_across - across_coordinates)
    heights = road_triangles[:, :, 2] + fraction * (following_height - road_triangles[:, :, 2])
    intervals = []
    for triangle_index in numpy.where(crossing.sum(axis=1) >= 2)[0]:
        coordinates = intersections[triangle_index, crossing[triangle_index]]
        intersection_heights = heights[triangle_index, crossing[triangle_index]]
        ordering = numpy.argsort(coordinates)
        intervals.append([coordinates[ordering[0]], coordinates[ordering[-1]], intersection_heights[ordering[0]], intersection_heights[ordering[-1]]])
    intervals.sort()
    merged = []
    for interval in intervals:
        if merged and interval[0] <= merged[-1][1] + 0.025:
            if interval[1] > merged[-1][1]:
                merged[-1][1] = interval[1]
                merged[-1][3] = interval[3]
        else:
            merged.append(interval.copy())
    containing = [interval for interval in merged if interval[0] <= 0 <= interval[1]]
    if len(containing) != 1:
        raise ValueError(f'Road section does not uniquely contain its waypoint: {position.tolist()}, {merged}')
    road_interval = containing[0]
    road_interval[0] = max(road_interval[0], -declared_right_width)
    road_interval[1] = min(road_interval[1], declared_left_width)
    for boundary_index, height_index in [(0, 2), (1, 3)]:
        coordinate = road_interval[boundary_index]
        boundary_heights = [interval[2] + (coordinate - interval[0]) / (interval[1] - interval[0]) * (interval[3] - interval[2])
                            for interval in intervals if interval[0] - 1e-5 <= coordinate <= interval[1] + 1e-5 and interval[1] - interval[0] > 1e-8]
        if not boundary_heights:
            raise ValueError('Authored road boundary is not on the source road mesh')
        road_interval[height_index] = min(boundary_heights, key=lambda height: abs(height - position[2]))
    return road_interval


def section(parent, name):
    return element_tree.SubElement(parent, 'section', name=name)


def number(parent, name, value, unit=None):
    attributes = {'name': name, 'val': f'{value:.12g}'}
    if unit:
        attributes['unit'] = unit
    element_tree.SubElement(parent, 'attnum', attributes)


def text(parent, name, value):
    element_tree.SubElement(parent, 'attstr', name=name, val=str(value))


def arc_between(start, end, tangent):
    displacement = end - start
    left = numpy.array([-tangent[1], tangent[0]])
    curvature = 2 * numpy.dot(displacement, left) / numpy.dot(displacement, displacement)
    if abs(curvature) < 1e-9:
        return {'type': 'str', 'length': numpy.linalg.norm(displacement), 'curvature': 0}
    angle = 2 * math.atan2(numpy.dot(displacement, left), numpy.dot(displacement, tangent))
    return {'type': 'lft' if curvature > 0 else 'rgt', 'length': abs(angle / curvature), 'curvature': curvature, 'radius': abs(1 / curvature), 'arc': abs(angle)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    parser.add_argument('--destination', required=True)
    parser.add_argument('--report', required=True)
    arguments = parser.parse_args()
    source_directory = pathlib.Path(arguments.source)
    destination_directory = pathlib.Path(arguments.destination)
    destination_directory.mkdir(parents=True, exist_ok=True)
    metadata = json.loads((source_directory / 'metadata/track.json').read_text())
    waypoint_document = json.loads((source_directory / 'metadata/racing_line.json').read_text())
    waypoints = waypoint_document['waypoints']
    positions = numpy.array([waypoint['pos'] for waypoint in waypoints])
    discontinuities = numpy.where(numpy.linalg.norm(numpy.diff(positions, axis=0), axis=1) > 15)[0]
    if len(discontinuities) != 1:
        raise ValueError('Main loop and pit route require an explicit unique separation')
    main_waypoints = sorted(waypoints[:discontinuities[0] + 1], key=lambda waypoint: waypoint['lap_distance_m'])
    distances = numpy.array([waypoint['lap_distance_m'] for waypoint in main_waypoints])
    positions = numpy.array([waypoint['pos'] for waypoint in main_waypoints])
    source_to_simulation = numpy.array([[0, 0, -1], [-1, 0, 0], [0, 1, 0]])
    positions = positions @ source_to_simulation.T
    collision_scene = BinaryScene(source_directory / 'fuji76_77_collision.glb')
    road_triangles = numpy.concatenate([vertices[indices] @ source_to_simulation.T
                                       for name, material, vertices, indices, coordinates in collision_scene.primitives()
                                       if name.startswith('COLLISION__ROAD__')])
    section_results = []
    centers = []
    for waypoint_index, position in enumerate(positions):
        direction = positions[(waypoint_index + 1) % len(positions), :2] - positions[waypoint_index - 1, :2]
        direction /= numpy.linalg.norm(direction)
        left_direction = numpy.array([-direction[1], direction[0]])
        waypoint = main_waypoints[waypoint_index]
        right_distance, left_distance, right_height, left_height = cross_section(road_triangles, position, left_direction, waypoint['width_left'], waypoint['width_right'])
        center = position.copy()
        center[2] = right_height + (-right_distance) / (left_distance - right_distance) * (left_height - right_height)
        centers.append(center)
        section_results.append([left_distance - right_distance, right_height, left_height, -right_distance, left_distance])
    centers = numpy.array(centers)
    section_results = numpy.array(section_results)
    lap_length = metadata['lap_length_m']
    initial_fraction = (lap_length - distances[-1]) / (lap_length - distances[-1] + distances[0])
    initial_center = centers[-1] * (1 - initial_fraction) + centers[0] * initial_fraction
    initial_section = section_results[-1] * (1 - initial_fraction) + section_results[0] * initial_fraction
    distances = numpy.concatenate(([0], distances, [lap_length]))
    centers = numpy.vstack((initial_center, centers, initial_center))
    section_results = numpy.vstack((initial_section, section_results, initial_section))
    center_spline = CubicSpline(distances, centers, bc_type='periodic')
    width_spline = CubicSpline(distances, section_results[:, 0], bc_type='periodic')
    right_height_spline = CubicSpline(distances, section_results[:, 1], bc_type='periodic')
    left_height_spline = CubicSpline(distances, section_results[:, 2], bc_type='periodic')
    right_width_spline = CubicSpline(distances, section_results[:, 3], bc_type='periodic')
    left_width_spline = CubicSpline(distances, section_results[:, 4], bc_type='periodic')
    sample_distances = numpy.linspace(0, lap_length, math.ceil(lap_length / 2) + 1)
    sample_positions = center_spline(sample_distances)
    tangent_directions = center_spline(sample_distances, 1)[:, :2]
    tangent_directions /= numpy.linalg.norm(tangent_directions, axis=1)[:, None]
    initial_heading = math.atan2(tangent_directions[0, 1], tangent_directions[0, 0])
    rotation = numpy.array([[math.cos(initial_heading), math.sin(initial_heading)], [-math.sin(initial_heading), math.cos(initial_heading)]])
    segments = []
    for sample_index in range(len(sample_distances) - 1):
        start = sample_positions[sample_index, :2]
        end = sample_positions[sample_index + 1, :2]
        start_tangent = tangent_directions[sample_index]
        end_tangent = tangent_directions[sample_index + 1]
        displacement = end - start
        tangent_projection = numpy.dot(displacement, start_tangent + end_tangent)
        tangent_difference = 1 - numpy.dot(start_tangent, end_tangent)
        tangent_distance = numpy.dot(displacement, displacement) / (tangent_projection + math.sqrt(tangent_projection * tangent_projection + 2 * tangent_difference * numpy.dot(displacement, displacement)))
        midpoint = (start + end + tangent_distance * (start_tangent - end_tangent)) / 2
        first_arc = arc_between(start, midpoint, start_tangent)
        second_arc = arc_between(end, midpoint, -end_tangent)
        second_arc['curvature'] *= -1
        if second_arc['type'] != 'str':
            second_arc['type'] = 'lft' if second_arc['curvature'] > 0 else 'rgt'
        middle_distance = (sample_distances[sample_index] + sample_distances[sample_index + 1]) / 2
        for arc, start_distance, end_distance in [(first_arc, sample_distances[sample_index], middle_distance), (second_arc, middle_distance, sample_distances[sample_index + 1])]:
            arc.update({'source_start_distance': start_distance, 'source_end_distance': end_distance})
            segments.append(arc)
    main_width = 2.0
    document = element_tree.Element('params', name='Fuji76-77', type='param', mode='mw')
    header = section(document, 'Header')
    number(header, 'version', 4)
    text(header, 'name', 'Fuji76-77')
    text(header, 'category', 'circuit')
    text(header, 'static collision mesh', 'tracks/circuit/fuji-seventy-six-seventy-seven/fuji_object_collisions.txt')
    text(header, 'ground collision mesh', 'tracks/circuit/fuji-seventy-six-seventy-seven/fuji_ground_collisions.txt')
    text(header, 'description', 'Existing Formula90s Fuji76-77 conversion')
    text(header, 'author', 'Source provenance in migration manifest')
    graphics = section(document, 'Graphic')
    text(graphics, '3d description', 'fuji-seventy-six-seventy-seven.acc')
    surfaces = section(document, 'Surfaces')
    for surface_name, friction, resistance in [('asphalt', 1.0, 0.002), ('grass', 0.6, 0.08)]:
        surface = section(surfaces, surface_name)
        number(surface, 'friction', friction)
        number(surface, 'rolling resistance', resistance)
        number(surface, 'roughness', 0)
    main_track = section(document, 'Main Track')
    number(main_track, 'width', main_width, 'm')
    text(main_track, 'surface', 'asphalt')
    text(main_track, 'penalty includes paved borders', 'yes')
    text(main_track, 'track limit penalties', 'no')
    for side_name in ['Left', 'Right']:
        border = section(main_track, side_name + ' Border')
        text(border, 'style', 'plan')
        text(border, 'surface', 'asphalt')
        number(border, 'width', 2, 'm')
        side = section(main_track, side_name + ' Side')
        number(side, 'width', 20, 'm')
        text(side, 'surface', 'grass')
        barrier = section(main_track, side_name + ' Barrier')
        text(barrier, 'style', 'wall')
        number(barrier, 'height', 1.0, 'm')
        number(barrier, 'width', 0.3, 'm')
    track_segments = section(main_track, 'Track Segments')
    for segment_index, segment_description in enumerate(segments):
        segment = section(track_segments, 'section_' + str(segment_index))
        text(segment, 'type', segment_description['type'])
        if segment_description['type'] == 'str':
            number(segment, 'lg', segment_description['length'], 'm')
        else:
            number(segment, 'radius', segment_description['radius'], 'm')
            number(segment, 'arc', segment_description['arc'], 'rad')
        start_distance = segment_description['source_start_distance']
        end_distance = segment_description['source_end_distance']
        midpoint_distance = (start_distance + end_distance) / 2
        for endpoint_name, distance in [('start', start_distance), ('end', end_distance)]:
            width = float(width_spline(distance))
            right_height = float(right_height_spline(distance))
            left_height = float(left_height_spline(distance))
            center_height = right_height + float(right_width_spline(distance)) / width * (left_height - right_height)
            banking = math.atan2(left_height - right_height, width)
            number(segment, 'z ' + endpoint_name, center_height, 'm')
            number(segment, 'banking ' + endpoint_name, banking, 'rad')
        for side_name in ['Left', 'Right']:
            border = section(segment, side_name + ' Border')
            boundary_width = left_width_spline(midpoint_distance) if side_name == 'Left' else right_width_spline(midpoint_distance)
            number(border, 'width', max(0, float(boundary_width) - main_width / 2), 'm')
    starting_grid = section(document, 'Starting Grid')
    number(starting_grid, 'rows', 1)
    number(starting_grid, 'distance to start', 31.12, 'm')
    number(starting_grid, 'initial speed', 0, 'm/s')
    element_tree.indent(document)
    element_tree.ElementTree(document).write(destination_directory / 'fuji-seventy-six-seventy-seven.xml', encoding='utf-8', xml_declaration=True)
    report = {'main_waypoint_count': len(main_waypoints), 'excluded_pit_waypoint_count': len(waypoints) - len(main_waypoints),
              'source_lap_length': lap_length, 'candidate_lap_length': sum(segment['length'] for segment in segments),
              'segment_count': len(segments), 'source_initial_center': initial_center.tolist(),
              'source_initial_heading': initial_heading, 'rotation': rotation.tolist(),
              'main_width': main_width, 'width_percentiles': numpy.percentile(section_results[:, 0], [0, 5, 50, 95, 100]).tolist(),
              'source_centers': centers.tolist(), 'source_distances': distances.tolist(), 'candidate_segments': segments,
              'native_geometry_validated': False, 'barriers_match_source': False, 'curbs_match_source': False}
    pathlib.Path(arguments.report).write_text(json.dumps(report, indent=2))
    print(json.dumps({key: value for key, value in report.items() if key not in ['source_centers', 'source_distances', 'candidate_segments']}, indent=2))


if __name__ == '__main__':
    main()
