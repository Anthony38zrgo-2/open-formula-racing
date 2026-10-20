import argparse
import hashlib
import json
import pathlib
import shutil
import struct
import urllib.parse

import numpy


class BinaryScene:
    def __init__(self, source_path):
        self.source_path = pathlib.Path(source_path)
        source_bytes = self.source_path.read_bytes()
        magic, version, length = struct.unpack_from('<III', source_bytes)
        if magic != 0x46546C67 or version != 2 or length != len(source_bytes):
            raise ValueError('Expected a complete binary glTF version two scene')
        document_length = struct.unpack_from('<I', source_bytes, 12)[0]
        self.document = json.loads(source_bytes[20:20 + document_length])
        binary_offset = 20 + document_length
        binary_length = struct.unpack_from('<I', source_bytes, binary_offset)[0]
        self.binary_bytes = source_bytes[binary_offset + 8:binary_offset + 8 + binary_length]
        self.world_transforms = {}
        for node_index in self.document['scenes'][self.document.get('scene', 0)]['nodes']:
            self.visit_node(node_index, numpy.eye(4))

    def accessor(self, accessor_index):
        description = self.document['accessors'][accessor_index]
        if 'sparse' in description:
            raise ValueError('Sparse accessors require explicit expansion')
        view = self.document['bufferViews'][description['bufferView']]
        component_types = {5120: '<i1', 5121: '<u1', 5122: '<i2', 5123: '<u2', 5125: '<u4', 5126: '<f4'}
        component_counts = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}
        component_type = numpy.dtype(component_types[description['componentType']])
        component_count = component_counts[description['type']]
        offset = view.get('byteOffset', 0) + description.get('byteOffset', 0)
        stride = view.get('byteStride', component_type.itemsize * component_count)
        values = numpy.ndarray((description['count'], component_count), dtype=component_type,
                               buffer=self.binary_bytes, offset=offset,
                               strides=(stride, component_type.itemsize)).copy()
        if description.get('normalized') and component_type.kind in 'iu':
            values = values.astype(float) / numpy.iinfo(component_type).max
            if component_type.kind == 'i':
                values = numpy.maximum(values, -1)
        return values

    def visit_node(self, node_index, parent_transform):
        node = self.document['nodes'][node_index]
        if 'matrix' in node:
            local_transform = numpy.array(node['matrix']).reshape(4, 4).T
        else:
            quaternion = node.get('rotation', [0, 0, 0, 1])
            horizontal, vertical, depth, scalar = quaternion
            local_transform = numpy.eye(4)
            local_transform[:3, :3] = numpy.array([
                [1 - 2 * (vertical * vertical + depth * depth), 2 * (horizontal * vertical - depth * scalar), 2 * (horizontal * depth + vertical * scalar)],
                [2 * (horizontal * vertical + depth * scalar), 1 - 2 * (horizontal * horizontal + depth * depth), 2 * (vertical * depth - horizontal * scalar)],
                [2 * (horizontal * depth - vertical * scalar), 2 * (vertical * depth + horizontal * scalar), 1 - 2 * (horizontal * horizontal + vertical * vertical)]])
            local_transform[:3, :3] = local_transform[:3, :3] @ numpy.diag(node.get('scale', [1, 1, 1]))
            local_transform[:3, 3] = node.get('translation', [0, 0, 0])
        self.world_transforms[node_index] = parent_transform @ local_transform
        for child_index in node.get('children', []):
            self.visit_node(child_index, self.world_transforms[node_index])

    def primitives(self, excluded_names=()):
        excluded_names = {name.casefold() for name in excluded_names}
        for node_index, world_transform in self.world_transforms.items():
            node = self.document['nodes'][node_index]
            if 'mesh' not in node or node.get('name', '').casefold() in excluded_names:
                continue
            for primitive in self.document['meshes'][node['mesh']]['primitives']:
                if primitive.get('mode', 4) != 4:
                    raise ValueError('Only triangulated primitives are supported')
                positions = self.accessor(primitive['attributes']['POSITION'])
                homogeneous_positions = numpy.column_stack((positions, numpy.ones(len(positions))))
                if 'skin' in node:
                    skin = self.document['skins'][node['skin']]
                    inverse_bind_transforms = self.accessor(skin['inverseBindMatrices']).reshape(-1, 4, 4).transpose(0, 2, 1)
                    joint_indices = self.accessor(primitive['attributes']['JOINTS_0']).astype(int)
                    joint_weights = self.accessor(primitive['attributes']['WEIGHTS_0'])
                    joint_transforms = numpy.array([self.world_transforms[joint] @ inverse_bind_transforms[index]
                                                   for index, joint in enumerate(skin['joints'])])
                    positions = numpy.zeros_like(positions, dtype=float)
                    for influence_index in range(4):
                        transformed = numpy.einsum('nij,nj->ni', joint_transforms[joint_indices[:, influence_index]], homogeneous_positions)
                        positions += transformed[:, :3] * joint_weights[:, influence_index, None]
                else:
                    positions = (homogeneous_positions @ world_transform.T)[:, :3]
                indices = self.accessor(primitive['indices']).reshape(-1, 3) if 'indices' in primitive else numpy.arange(len(positions)).reshape(-1, 3)
                texture_coordinates = self.accessor(primitive['attributes']['TEXCOORD_0']) if 'TEXCOORD_0' in primitive['attributes'] else numpy.zeros((len(positions), 2))
                if numpy.linalg.det(world_transform[:3, :3]) < 0:
                    indices = indices[:, ::-1]
                yield node.get('name', 'geometry'), primitive.get('material', -1), positions, indices, texture_coordinates

    def export_textures(self, destination_directory, texture_prefix):
        filenames = {}
        for image_index, description in enumerate(self.document.get('images', [])):
            if 'uri' in description:
                source_texture = self.source_path.parent / urllib.parse.unquote(description['uri'])
                filename = texture_prefix + '_' + source_texture.name
                shutil.copyfile(source_texture, destination_directory / filename)
            else:
                view = self.document['bufferViews'][description['bufferView']]
                extension = '.png' if description.get('mimeType') == 'image/png' else '.jpg'
                filename = texture_prefix + '_texture_' + str(image_index) + extension
                start = view.get('byteOffset', 0)
                (destination_directory / filename).write_bytes(self.binary_bytes[start:start + view['byteLength']])
            filenames[image_index] = filename
        return filenames


def export_scene(source_path, destination_path, transformation, excluded_names=()):
    scene = BinaryScene(source_path)
    destination_path = pathlib.Path(destination_path)
    destination_path.parent.mkdir(parents=True, exist_ok=True)
    texture_filenames = scene.export_textures(destination_path.parent, destination_path.stem)
    primitives = list(scene.primitives(excluded_names))
    materials = scene.document.get('materials', [])
    total_vertices = 0
    total_triangles = 0
    bounds = []
    with destination_path.open('w', encoding='utf-8', newline='\n') as output:
        output.write('AC3Db\n')
        for material_index, material in enumerate(materials + [{}]):
            color = material.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1])
            emission = material.get('emissiveFactor', [0, 0, 0])
            output.write(f'MATERIAL "material_{material_index}" rgb {color[0]} {color[1]} {color[2]} amb 0.4 0.4 0.4 emis {emission[0]} {emission[1]} {emission[2]} spec 0.08 0.08 0.08 shi 32 trans {1-color[3]}\n')
        output.write(f'OBJECT world\nkids {len(primitives)}\n')
        for name, material_index, positions, indices, texture_coordinates in primitives:
            positions = (numpy.column_stack((positions, numpy.ones(len(positions)))) @ transformation.T)[:, :3]
            material = materials[material_index] if material_index >= 0 else {}
            output.write(f'OBJECT poly\nname {json.dumps(name)}\n')
            texture = material.get('pbrMetallicRoughness', {}).get('baseColorTexture')
            if texture:
                image_index = scene.document['textures'][texture['index']]['source']
                output.write(f'texture "{texture_filenames[image_index]}"\n')
            if material.get('alphaMode') == 'MASK':
                output.write(f'alpha_cutoff {material.get("alphaCutoff", 0.5):.8f}\n')
            output.write(f'crease 45\nnumvert {len(positions)}\n')
            for position in positions:
                output.write(' '.join(f'{coordinate:.8f}' for coordinate in position) + '\n')
            output.write(f'numsurf {len(indices)}\n')
            surface_flags = '0x30' if material.get('doubleSided', False) else '0x10'
            for triangle in indices:
                output.write(f'SURF {surface_flags}\nmat {material_index if material_index >= 0 else len(materials)}\nrefs 3\n')
                for vertex_index in triangle:
                    texture_horizontal, texture_vertical = texture_coordinates[vertex_index]
                    output.write(f'{vertex_index} {texture_horizontal:.8f} {1-texture_vertical:.8f}\n')
            output.write('kids 0\n')
            bounds.append([positions.min(axis=0).tolist(), positions.max(axis=0).tolist()])
            total_vertices += len(positions)
            total_triangles += len(indices)
    return {'source': str(source_path), 'source_sha256': hashlib.sha256(pathlib.Path(source_path).read_bytes()).hexdigest(),
            'output': str(destination_path), 'output_sha256': hashlib.sha256(destination_path.read_bytes()).hexdigest(),
            'vertices': total_vertices, 'triangles': total_triangles, 'primitive_bounds': bounds,
            'transformation': transformation.tolist(), 'excluded_nodes': list(excluded_names)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    parser.add_argument('destination')
    arguments = parser.parse_args()
    transformation = numpy.array([[0, 0, -1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1]], dtype=float)
    report = export_scene(arguments.source, arguments.destination, transformation)
    pathlib.Path(arguments.destination + '.json').write_text(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
