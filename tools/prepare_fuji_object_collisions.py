import argparse
import json
import pathlib

import numpy

from convert_binary_scene import BinaryScene


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    parser.add_argument('--alignment', required=True)
    parser.add_argument('--destination', required=True)
    arguments = parser.parse_args()
    alignment = json.loads(pathlib.Path(arguments.alignment).read_text())
    transformation = numpy.array(alignment['transformation'])
    scene = BinaryScene(arguments.source)
    groups = []
    ground_triangles = []
    surface_definitions = json.loads((pathlib.Path(arguments.source).parent / 'metadata/surfaces.json').read_text())['surface_definitions']
    rolling_resistance = {'ROAD': 0.002, 'CURB': 0.003, 'METAL': 0.004, 'GRASS': 0.08, 'GRAVEL': 0.12, 'SAND': 0.14}
    for name, material, vertices, indices, coordinates in scene.primitives():
        transformed = (numpy.column_stack((vertices, numpy.ones(len(vertices)))) @ transformation.T)[:, :3]
        transformed = transformed[:, [0, 2, 1]] * numpy.array([1, -1, 1])
        triangles = transformed[indices]
        normals = numpy.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
        lengths = numpy.linalg.norm(normals, axis=1)
        category = name.split('__')[1]
        if category in rolling_resistance:
            definitions = [definition['dry_grip'] for definition in surface_definitions if definition['surface_id'].upper() == category]
            friction = float(numpy.median(definitions)) if definitions else 1.0
            for triangle in triangles[(lengths > 0.000001) & (numpy.abs(normals[:, 2]) > 0.05 * lengths)]:
                ground_triangles.append((category, friction, rolling_resistance[category], triangle))
        if category != 'WALL':
            continue
        selected = (lengths > 0.000001) & (numpy.abs(normals[:, 2]) < 0.95 * lengths)
        triangles = triangles[selected]
        if len(triangles):
            groups.append((name, triangles))
    destination = pathlib.Path(arguments.destination)
    with destination.open('w', encoding='utf-8', newline='\n') as output:
        output.write(f'FORMULA_STATIC_WALLS_1 {len(groups)}\n')
        for name, triangles in groups:
            output.write(f'{name} {len(triangles)}\n')
            for triangle in triangles:
                output.write(' '.join(f'{value:.8f}' for value in triangle.flatten()) + '\n')
    with destination.with_name('fuji_ground_collisions.txt').open('w', encoding='utf-8', newline='\n') as output:
        output.write(f'FORMULA_GROUND_1 {len(ground_triangles)}\n')
        for category, friction, resistance, triangle in ground_triangles:
            output.write(f'{friction:.8f} {resistance:.8f} ' + ' '.join(f'{value:.8f}' for value in triangle.flatten()) + '\n')
    print(json.dumps({'ground_triangles': len(ground_triangles), 'ground_categories': sorted(set(category for category, friction, resistance, triangle in ground_triangles)),
                      'groups': len(groups), 'lateral_triangles': sum(len(triangles) for name, triangles in groups),
                      'source': arguments.source, 'destination': str(destination)}, indent=2))


if __name__ == '__main__':
    main()
