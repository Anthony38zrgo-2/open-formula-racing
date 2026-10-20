import argparse
import hashlib
import json
import pathlib
import re

import numpy

from convert_binary_scene import BinaryScene


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    parser.add_argument('--export', required=True)
    parser.add_argument('--report', required=True)
    arguments = parser.parse_args()
    scene = BinaryScene(arguments.source)
    export_path = pathlib.Path(arguments.export)
    exported_groups = export_path.read_text().split('OBJECT poly\n')[1:]
    source_primitives = []
    for node_index in scene.world_transforms:
        node = scene.document['nodes'][node_index]
        if 'mesh' in node:
            for primitive in scene.document['meshes'][node['mesh']]['primitives']:
                source_primitives.append((node, primitive))
    if len(exported_groups) != len(source_primitives):
        raise ValueError('Exported primitive count differs from source')
    texture_records = {}
    verified_secondary_coordinate_groups = 0
    verified_carbon_groups = 0
    for exported_group, (node, primitive) in zip(exported_groups, source_primitives):
        material = scene.document['materials'][primitive['material']]
        texture_match = re.search(r'^texture "([^"]+)"$', exported_group, re.MULTILINE)
        if not texture_match:
            raise ValueError('Missing explicit material texture on ' + node['name'])
        texture_path = export_path.parent / texture_match.group(1)
        if not texture_path.is_file():
            raise ValueError('Missing exported texture: ' + str(texture_path))
        texture_records[texture_path.name] = hashlib.sha256(texture_path.read_bytes()).hexdigest()
        coordinate_channel = material.get('pbrMetallicRoughness', {}).get('baseColorTexture', {}).get('texCoord', 0)
        expected_coordinates = scene.accessor(primitive['attributes']['TEXCOORD_' + str(coordinate_channel)])
        exported_references = []
        for reference_block in re.findall(r'^refs 3\n((?:[^\n]+\n){3})', exported_group, re.MULTILINE):
            for reference_line in reference_block.splitlines():
                vertex_index, horizontal, vertical = reference_line.split()
                exported_references.append((int(vertex_index), float(horizontal), float(vertical)))
        reference_values = numpy.asarray(exported_references)
        if len(reference_values) == 0:
            raise ValueError('Missing triangle references on ' + node['name'])
        expected_values = expected_coordinates[reference_values[:, 0].astype(int)].copy()
        expected_values[:, 1] = 1.0 - expected_values[:, 1]
        if not numpy.allclose(reference_values[:, 1:], expected_values, atol=0.0000001, rtol=0.0):
            raise ValueError('Texture coordinates differ from source material channel on ' + node['name'])
        verified_secondary_coordinate_groups += coordinate_channel == 1
        if material['name'] == 'fw19_cf.tGA:fw19_cf.tga':
            if not texture_path.name.endswith('Carbon_Fiber_diffuse.jpg'):
                raise ValueError('Chassis carbon weave texture is not bound')
            verified_carbon_groups += 1
    report = {'primitive_count': len(exported_groups), 'secondary_coordinate_groups': verified_secondary_coordinate_groups,
              'restored_carbon_groups': verified_carbon_groups, 'textures': texture_records,
              'all_texture_files_present': True, 'all_material_coordinate_channels_preserved': True,
              'runtime_visual_validated': False}
    if verified_secondary_coordinate_groups == 0 or verified_carbon_groups == 0:
        raise ValueError('Expected Williams carbon material groups were not tested')
    pathlib.Path(arguments.report).write_text(json.dumps(report, indent=2))
    print(json.dumps({name: value for name, value in report.items() if name != 'textures'}))


if __name__ == '__main__':
    main()
