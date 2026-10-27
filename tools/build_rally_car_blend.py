"""
Extractor e importador de modelos BMF de Rally Trophy a Blender (.blend).
Genera la escena completa del Ford Escort RS 2000 (1971) con materiales, texturas,
normales corregidas, iluminacion y vista configurada para mostrar texturas al abrir.
"""

import bpy
import struct
import os
import math

def read_str(f):
    s = []
    while True:
        b = f.read(1)
        if not b or b == b'\x00':
            break
        s.append(b)
    return b''.join(s).decode('latin1', errors='replace')

def parse_bmf(filepath):
    print(f"Leyendo BMF: {filepath}")
    with open(filepath, 'rb') as f:
        head = f.read(4)
        if head == b'(c) ':
            read_str(f)
            filesize = struct.unpack('<I', f.read(4))[0]
        else:
            filesize = struct.unpack('<I', head)[0]

        materials = []
        objects = []
        cur_mesh = None

        token = read_str(f)
        while token:
            if token in ('MAIN', 'INFO'):
                f.read(4)
                token = read_str(f)
            elif token == 'BMF':
                exporter = read_str(f)
                f.read(4)
                token = read_str(f)
            elif token == 'MATERIALLIST':
                mat_cnt, tmp = struct.unpack('<II', f.read(8))
                token = read_str(f)
            elif token == 'MATERIAL':
                ints1 = struct.unpack('<5i', f.read(20))
                flts = struct.unpack('<16f', f.read(64))
                ints2 = struct.unpack('<2i', f.read(8))
                mat_name = read_str(f)
                tex0 = read_str(f)
                tex1 = read_str(f)
                tex2 = read_str(f)
                f.read(4)
                materials.append({
                    'name': mat_name,
                    'shader_id': ints1[2],
                    'tex0': tex0,
                    'tex1': tex1,
                    'tex2': tex2
                })
                token = read_str(f)
            elif token == 'TRACK':
                f.read(44)
                token = read_str(f)
            elif token == 'MESH':
                mesh_name = read_str(f)
                tmp = f.read(4)
                matrix = struct.unpack('<16f', f.read(64))
                pos_extra = struct.unpack('<3f', f.read(12))
                tmpf = f.read(14*4)
                tmp2 = f.read(8)
                cur_mesh = {
                    'name': mesh_name,
                    'matrix': matrix,
                    'pos_extra': pos_extra,
                    'batches': []
                }
                objects.append(cur_mesh)
                token = read_str(f)
            elif token == 'LIGHT':
                read_str(f)
                f.read(0xC6 - 6)
                token = read_str(f)
            elif token == 'CAMERA2':
                read_str(f)
                f.read(0xA5 - 9)
                token = read_str(f)
            elif token == 'HIERARCHY':
                f.read(8)
                read_str(f) # SCENE
                read_str(f) # sc_name
                f.read(4 + 64 + 0x50 + 4)
                token = read_str(f)
            elif token == 'MODEL':
                f.read(4 + 12 + 12 + 16 + 4)
                target = cur_mesh
                if target is None:
                    target = {
                        'name': f'static_mesh_{len(objects):04d}',
                        'matrix': [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
                        'pos_extra': (0.0, 0.0, 0.0),
                        'batches': []
                    }
                    objects.append(target)

                while True:
                    sub_token = read_str(f)
                    if sub_token != 'BATCH2':
                        token = sub_token
                        break
                    hdr_bytes = f.read(12)
                    if len(hdr_bytes) < 12:
                        token = None
                        break
                    mat_id, v_cnt, p_cnt = struct.unpack('<3I', hdr_bytes)
                    v_raw = f.read(v_cnt * 36)
                    i_raw = f.read(p_cnt * 3 * 2)
                    tr_bytes = f.read(4)

                    verts = []
                    norms = []
                    uvs = []
                    for vi in range(v_cnt):
                        x, y, z, nx, ny, nz, col, u, v = struct.unpack('<3f3fI2f', v_raw[vi*36 : (vi+1)*36])
                        verts.append((x, y, z))
                        norms.append((nx, ny, nz))
                        uvs.append((u, v))

                    indices = struct.unpack(f'<{p_cnt*3}H', i_raw)
                    faces = []
                    for pi in range(p_cnt):
                        # Invertir orden de indices por cambio de mano DirectX (LH) -> Blender (RH)
                        faces.append((indices[pi*3+2], indices[pi*3+1], indices[pi*3]))

                    target['batches'].append({
                        'mat_id': mat_id,
                        'verts': verts,
                        'norms': norms,
                        'uvs': uvs,
                        'faces': faces
                    })
            else:
                f.read(4)
                token = read_str(f)

    return materials, objects

_IMAGE_CACHE = {}

def load_texture_image(tex_name, textures_dirs):
    key = tex_name.lower()
    if key in _IMAGE_CACHE:
        return _IMAGE_CACHE[key]

    img_path = None
    for tdir in textures_dirs:
        candidate = os.path.join(tdir, tex_name)
        if os.path.exists(candidate):
            img_path = candidate
            break
        for root, _, files in os.walk(tdir):
            for fn in files:
                if fn.lower() == key:
                    img_path = os.path.join(root, fn)
                    break
            if img_path:
                break
        if img_path:
            break

    if img_path:
        img = bpy.data.images.load(img_path, check_existing=True)
        img.alpha_mode = 'STRAIGHT'
        _IMAGE_CACHE[key] = img
        return img
    return None

def get_or_create_material(mat_def, textures_dirs):
    mat_name = mat_def['name']
    if mat_name in bpy.data.materials:
        return bpy.data.materials[mat_name]

    mat = bpy.data.materials.new(name=mat_name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    bsdf = nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Roughness'].default_value = 0.35

    tex_name = mat_def.get('tex0', '')
    shader_id = mat_def.get('shader_id', 2)

    if tex_name and bsdf:
        img = load_texture_image(tex_name, textures_dirs)
        if img:
            tex_node = nodes.new('ShaderNodeTexImage')
            tex_node.image = img
            tex_node.location = (-350, 200)
            links.new(tex_node.outputs['Color'], bsdf.inputs['Base Color'])
            if shader_id == 5:
                links.new(tex_node.outputs['Alpha'], bsdf.inputs['Alpha'])
                if hasattr(mat, 'surface_render_method'):
                    mat.surface_render_method = 'DITHERED'

    return mat

def create_blender_object(obj_data, materials_list, textures_dirs, parent_collection):
    mesh_name = obj_data['name']
    batches = obj_data['batches']
    if not batches:
        return None

    b_mesh = bpy.data.meshes.new(mesh_name)

    all_verts = []
    all_norms = []
    all_faces = []
    all_uvs = []
    all_mat_ids = []

    m = obj_data.get('matrix', [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])
    pos_extra = obj_data.get('pos_extra', (0.0, 0.0, 0.0))
    px, py, pz = pos_extra

    vert_offset = 0
    mat_map = {}

    for b in batches:
        mat_id = b['mat_id']
        if mat_id not in mat_map:
            mat_map[mat_id] = len(mat_map)

        slot_idx = mat_map[mat_id]

        for (vx, vy, vz), (nx, ny, nz) in zip(b['verts'], b['norms']):
            rx = vx * m[0] + vy * m[4] + vz * m[8]
            ry = vx * m[1] + vy * m[5] + vz * m[9]
            rz = vx * m[2] + vy * m[6] + vz * m[10]

            rnx = nx * m[0] + ny * m[4] + nz * m[8]
            rny = nx * m[1] + ny * m[5] + nz * m[9]
            rnz = nx * m[2] + ny * m[6] + nz * m[10]

            all_verts.append((rx, rz, ry))
            all_norms.append((rnx, rnz, rny))

        for f0, f1, f2 in b['faces']:
            if f0 != f1 and f1 != f2 and f2 != f0:
                all_faces.append((f0 + vert_offset, f1 + vert_offset, f2 + vert_offset))
                all_mat_ids.append(slot_idx)

        all_uvs.extend(b['uvs'])
        vert_offset += len(b['verts'])

    b_mesh.from_pydata(all_verts, [], all_faces)

    for orig_id, slot_idx in sorted(mat_map.items(), key=lambda x: x[1]):
        if orig_id < len(materials_list):
            mat_def = materials_list[orig_id]
            b_mat = get_or_create_material(mat_def, textures_dirs)
            b_mesh.materials.append(b_mat)

    for fi, poly in enumerate(b_mesh.polygons):
        poly.material_index = all_mat_ids[fi]
        poly.use_smooth = True

    if all_uvs and b_mesh.polygons:
        uv_layer = b_mesh.uv_layers.new(name="UVMap")
        for poly in b_mesh.polygons:
            for loop_index in poly.loop_indices:
                vi = b_mesh.loops[loop_index].vertex_index
                u, v = all_uvs[vi]
                uv_layer.data[loop_index].uv = (u, 1.0 - v)

    b_mesh.update()
    try:
        b_mesh.normals_split_custom_set_from_vertices(all_norms)
    except Exception:
        pass

    blender_obj = bpy.data.objects.new(mesh_name, b_mesh)
    parent_collection.objects.link(blender_obj)
    blender_obj.location = (px, pz, py)

    return blender_obj

def configure_viewport_and_lighting(cam_loc, cam_rot_deg):
    # Crear luz solar e iluminacion de entorno
    light_data = bpy.data.lights.new(name="Sun_Light", type='SUN')
    light_data.energy = 3.5
    light_data.color = (1.0, 0.95, 0.85)
    light_obj = bpy.data.objects.new(name="Sun_Light", object_data=light_data)
    bpy.context.scene.collection.objects.link(light_obj)
    light_obj.rotation_euler = (math.radians(45), math.radians(15), math.radians(35))

    # Crear camara
    cam_data = bpy.data.cameras.new(name="Main_Camera")
    cam_data.lens = 38.0
    cam_data.clip_start = 0.1
    cam_data.clip_end = 10000.0
    cam_obj = bpy.data.objects.new(name="Main_Camera", object_data=cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    cam_obj.location = cam_loc
    cam_obj.rotation_euler = tuple(math.radians(a) for a in cam_rot_deg)
    bpy.context.scene.camera = cam_obj

    # Configurar vistas 3D para que muestren las texturas inmediatamente (tanto en Solid como en Material Preview)
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == 'VIEW_3D':
                for space in area.spaces:
                    if space.type == 'VIEW_3D':
                        space.clip_start = 0.1
                        space.clip_end = 10000.0
                        space.shading.type = 'MATERIAL'
                        space.shading.color_type = 'TEXTURE'

def build_car_blend():
    car_root = r"D:\open-formula\staging_rally\cars\ra-escort-rs2000"
    model_bmf = os.path.join(car_root, "model.BMF")
    output_blend = os.path.join(car_root, "ford_escort_rs2000_1971.blend")

    # Usar plantilla con UI completa para conservar los espacios VIEW_3D configurados con texturas
    bpy.ops.wm.read_factory_settings(use_empty=False)
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh, do_unlink=True)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat, do_unlink=True)
    _IMAGE_CACHE.clear()

    col_car = bpy.data.collections.new("Ford_Escort_RS2000_1971")
    bpy.context.scene.collection.children.link(col_car)

    col_body = bpy.data.collections.new("Body_And_Interior")
    col_wheels = bpy.data.collections.new("Wheels")
    col_lod = bpy.data.collections.new("LOD_And_Crash")
    col_helpers = bpy.data.collections.new("Helpers_And_Shadows")

    col_car.children.link(col_body)
    col_car.children.link(col_wheels)
    col_car.children.link(col_lod)
    col_car.children.link(col_helpers)

    col_lod.hide_viewport = True
    col_lod.hide_render = True
    col_helpers.hide_viewport = True
    col_helpers.hide_render = True

    materials, objects = parse_bmf(model_bmf)
    print(f"Parseados {len(materials)} materiales y {len(objects)} objetos del coche.")

    for obj in objects:
        name = obj['name'].lower()
        if 'lod' in name or 'crash' in name or name in ('interior_lo', 'steering_wheel_lo'):
            target_col = col_lod
        elif name.startswith('light_') or name.startswith('ext_pipe') or name.startswith('shadow_'):
            target_col = col_helpers
        elif 'tire' in name or ('wheel' in name and 'steering' not in name):
            target_col = col_wheels
        else:
            target_col = col_body

        b_obj = create_blender_object(obj, materials, [car_root], target_col)
        # En Rally Trophy las ruedas estan modeladas en el tope superior de la suspension;
        # desplazamos la carroceria e interior +0.10m en Z (WheelDisplacement = 0.1 en car.ini)
        # para que el coche repose en su altura de marcha natural con las ruedas a Z = 0.0.
        if b_obj and target_col in (col_body, col_lod):
            b_obj.location.z += 0.10

    configure_viewport_and_lighting(cam_loc=(3.2, 3.8, 1.6), cam_rot_deg=(72, 0, 140))

    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=output_blend)
    print(f"Archivo .blend del coche generado con exito: {output_blend}")

if __name__ == "__main__":
    build_car_blend()
