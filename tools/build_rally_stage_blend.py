"""
Script para generar los archivos .blend del Tramo 1 de Rusia (stage1.BMF + stage1sky.BMF)
y el archivo integrado con el Ford Escort RS 2000 (1971) situado en la linea de salida.
Incluye correccion de matrices 3x3 por objeto, inversion de winding DirectX->Blender,
separacion de triggers invisibles (nul_1 / split / start) y empaquetado de texturas.
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
                read_str(f) # exporter
                f.read(4)
                token = read_str(f)
            elif token == 'MATERIALLIST':
                f.read(8)
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
                f.read(4) # tmp
                matrix = struct.unpack('<16f', f.read(64))
                pos_extra = struct.unpack('<3f', f.read(12))
                f.read(14*4 + 8) # tmpf + tmp2
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
                        'name': f'road_terrain_{len(objects):04d}',
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
                    hdr = f.read(12)
                    if len(hdr) < 12:
                        token = None
                        break
                    mat_id, v_cnt, p_cnt = struct.unpack('<3I', hdr)
                    v_raw = f.read(v_cnt * 36)
                    i_raw = f.read(p_cnt * 3 * 2)
                    f.read(4) # trailer

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
                        # Invertir orden de indices por cambio de sistema de coordenadas (DirectX LH -> Blender RH)
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
        bsdf.inputs['Roughness'].default_value = 0.85
        if ' specular ior level' in [inp.name.lower() for inp in bsdf.inputs]:
            bsdf.inputs['Specular IOR Level'].default_value = 0.15

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

    if 'twosided' in mat_name.lower() or 'twosides' in mat_name.lower() or shader_id == 5:
        mat.use_backface_culling = False

    return mat

def categorize_object(obj, materials):
    n = obj['name'].lower()
    # Detectar muros invisibles de tiempos parciales (split_00..09) y puntos de salida (start..start06)
    if n.startswith('split_') or n.startswith('start') or any(
        b['mat_id'] < len(materials) and materials[b['mat_id']]['name'].lower().startswith('nul')
        for b in obj['batches']
    ):
        return 'Triggers_And_Startpoints'
    elif 'road_terrain' in n:
        return 'Track_Road_And_Terrain'
    elif any(k in n for k in ['tree', 'bush', 'grass', 'birch', 'pine', 'spruce', 'hay', 'forest']):
        return 'Foliage_And_Vegetation'
    elif any(k in n for k in ['house', 'barn', 'church', 'shed', 'building', 'bridge', 'tunnel', 'tent', 'cowhouse']):
        return 'Buildings_And_Structures'
    elif any(k in n for k in ['fence', 'railing', 'wall', 'gate', 'barrier', 'string', 'roadbar']):
        return 'Fences_And_Barriers'
    else:
        return 'Props_And_Markers'

def build_stage_objects(objects, materials, textures_dirs, collections_dict):
    category_data = {
        cat: {'verts': [], 'norms': [], 'faces': [], 'uvs': [], 'mat_slots': {}, 'mat_indices': []}
        for cat in collections_dict.keys()
    }

    print("Consolidando tramo por categorias aplicando rotaciones 3x3 y traslaciones...")
    for obj in objects:
        cat = categorize_object(obj, materials)
        cd = category_data[cat]

        m = obj.get('matrix', [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])
        pos_extra = obj.get('pos_extra', (0.0, 0.0, 0.0))
        px, py, pz = pos_extra

        for b in obj['batches']:
            mat_id = b['mat_id']
            if mat_id not in cd['mat_slots']:
                cd['mat_slots'][mat_id] = len(cd['mat_slots'])
            slot_idx = cd['mat_slots'][mat_id]

            v_offset = len(cd['verts'])
            for (vx, vy, vz), (nx, ny, nz) in zip(b['verts'], b['norms']):
                # Aplicar matriz de rotacion/escala 3x3 + traslacion (DirectX row-vector v * M + pos_extra)
                wx = vx * m[0] + vy * m[4] + vz * m[8] + px
                wy = vx * m[1] + vy * m[5] + vz * m[9] + py
                wz = vx * m[2] + vy * m[6] + vz * m[10] + pz

                wnx = nx * m[0] + ny * m[4] + nz * m[8]
                wny = nx * m[1] + ny * m[5] + nz * m[9]
                wnz = nx * m[2] + ny * m[6] + nz * m[10]

                # Convertir a coordenadas Blender (X=wx, Y=wz, Z=wy)
                cd['verts'].append((wx, wz, wy))
                cd['norms'].append((wnx, wnz, wny))

            for f0, f1, f2 in b['faces']:
                if f0 != f1 and f1 != f2 and f2 != f0:
                    cd['faces'].append((f0 + v_offset, f1 + v_offset, f2 + v_offset))
                    cd['mat_indices'].append(slot_idx)

            cd['uvs'].extend(b['uvs'])

    for cat_name, cd in category_data.items():
        if not cd['verts']:
            continue
        print(f"Generando malla para {cat_name}: {len(cd['verts']):,} vertices, {len(cd['faces']):,} triangulos")
        mesh = bpy.data.meshes.new(cat_name)
        mesh.from_pydata(cd['verts'], [], cd['faces'])

        for orig_id, slot_idx in sorted(cd['mat_slots'].items(), key=lambda x: x[1]):
            if orig_id < len(materials):
                mat_def = materials[orig_id]
                b_mat = get_or_create_material(mat_def, textures_dirs)
                mesh.materials.append(b_mat)

        for fi, poly in enumerate(mesh.polygons):
            poly.material_index = cd['mat_indices'][fi]
            poly.use_smooth = True

        if cd['uvs'] and mesh.polygons:
            uv_layer = mesh.uv_layers.new(name="UVMap")
            for poly in mesh.polygons:
                for loop_idx in poly.loop_indices:
                    vi = mesh.loops[loop_idx].vertex_index
                    u, v = cd['uvs'][vi]
                    uv_layer.data[loop_idx].uv = (u, 1.0 - v)

        mesh.update()
        try:
            mesh.normals_split_custom_set_from_vertices(cd['norms'])
        except Exception:
            pass

        obj = bpy.data.objects.new(cat_name, mesh)
        collections_dict[cat_name].objects.link(obj)

def configure_viewport_and_lighting(cam_loc, cam_rot_deg):
    # Configurar cielo/mundo luminoso de Rusia
    world = bpy.data.worlds.get("World") or bpy.data.worlds.new("World")
    bpy.context.scene.world = world
    if world.node_tree:
        bg = world.node_tree.nodes.get("Background")
        if bg:
            bg.inputs['Color'].default_value = (0.58, 0.68, 0.82, 1.0)
            bg.inputs['Strength'].default_value = 1.1

    # Luz solar
    light_data = bpy.data.lights.new(name="Sun_Light", type='SUN')
    light_data.energy = 3.5
    light_data.color = (1.0, 0.96, 0.88)
    light_obj = bpy.data.objects.new(name="Sun_Light", object_data=light_data)
    bpy.context.scene.collection.objects.link(light_obj)
    light_obj.rotation_euler = (math.radians(48), math.radians(15), math.radians(35))

    # Camara enfocada a la linea de salida
    cam_data = bpy.data.cameras.new(name="Stage_Start_Camera")
    cam_data.lens = 35.0
    cam_data.clip_start = 0.1
    cam_data.clip_end = 10000.0
    cam_obj = bpy.data.objects.new(name="Stage_Start_Camera", object_data=cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    cam_obj.location = cam_loc
    cam_obj.rotation_euler = tuple(math.radians(a) for a in cam_rot_deg)
    bpy.context.scene.camera = cam_obj

    # Configurar todas las vistas 3D para que muestren las texturas inmediatamente al abrir el archivo
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == 'VIEW_3D':
                for space in area.spaces:
                    if space.type == 'VIEW_3D':
                        space.clip_start = 0.1
                        space.clip_end = 10000.0
                        space.shading.type = 'MATERIAL'
                        space.shading.color_type = 'TEXTURE'
                        if space.region_3d:
                            # Situar el pivot de la vista 3D exactamente en la salida del tramo
                            space.region_3d.view_location = (-2.18, 38.39, -0.8)
                            space.region_3d.view_distance = 7.5

def build_stage_blend():
    track_root = r"D:\open-formula\staging_rally\tracks\russia-stage1"
    stage_bmf = os.path.join(track_root, "stage1.BMF")
    tex_dir = os.path.join(track_root, "textures")
    output_blend = os.path.join(track_root, "russia_stage1.blend")

    # Usar plantilla con UI para conservar la configuracion de sombreado con texturas en VIEW_3D
    bpy.ops.wm.read_factory_settings(use_empty=False)
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh, do_unlink=True)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat, do_unlink=True)
    _IMAGE_CACHE.clear()

    master_col = bpy.data.collections.new("Russia_Stage1")
    bpy.context.scene.collection.children.link(master_col)

    categories = [
        'Track_Road_And_Terrain',
        'Foliage_And_Vegetation',
        'Buildings_And_Structures',
        'Fences_And_Barriers',
        'Props_And_Markers',
        'Triggers_And_Startpoints'
    ]
    cols = {}
    for c in categories:
        col = bpy.data.collections.new(c)
        master_col.children.link(col)
        cols[c] = col

    # Ocultar los muros invisibles de split/start para que no tapen la carretera
    cols['Triggers_And_Startpoints'].hide_viewport = True
    cols['Triggers_And_Startpoints'].hide_render = True

    materials, objects = parse_bmf(stage_bmf)
    print(f"Materiales tramo: {len(materials)}, Objetos brutos: {len(objects)}")
    build_stage_objects(objects, materials, [tex_dir, track_root], cols)

    # Camara mirando hacia la linea de salida y el inicio del tramo (Y+ es el avance del tramo)
    configure_viewport_and_lighting(cam_loc=(1.8, 32.2, 0.2), cam_rot_deg=(76, 0, 28))

    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=output_blend)
    print(f"Guardado exitoso: {output_blend}")

    # Ahora generar el archivo COMBINADO con el tramo y el Ford Escort RS 2000 en la salida
    combined_blend = r"D:\open-formula\staging_rally\russia_stage1_with_escort_rs2000.blend"
    car_blend = r"D:\open-formula\staging_rally\cars\ra-escort-rs2000\ford_escort_rs2000_1971.blend"

    print("Integrando Ford Escort RS 2000 en la escena del tramo...")
    with bpy.data.libraries.load(car_blend, link=False) as (data_from, data_to):
        data_to.collections = [c for c in data_from.collections if c == "Ford_Escort_RS2000_1971"]

    # Posicion y pendiente exacta sobre la calzada en la salida de Russia Stage 1 (reset1.bin nodo 0):
    # X = -2.18, Y_Blender = 38.39, Z_Blender = -1.78, inclinacion (pitch) = -2.57 grados cuesta abajo
    start_x, start_y, start_z = -2.18, 38.39, -1.78
    pitch_rad = math.radians(-2.575)

    car_root_empty = bpy.data.objects.new("Ford_Escort_Start_Transform", None)
    car_root_empty.location = (start_x, start_y, start_z)
    car_root_empty.rotation_euler = (pitch_rad, 0.0, 0.0)

    for car_col in data_to.collections:
        bpy.context.scene.collection.children.link(car_col)
        car_col.objects.link(car_root_empty)
        for sub_col in car_col.children:
            if sub_col.name in ("LOD_And_Crash", "Helpers_And_Shadows"):
                sub_col.hide_viewport = True
                sub_col.hide_render = True
        for obj in car_col.all_objects:
            if obj != car_root_empty and obj.parent is None:
                obj.parent = car_root_empty

    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=combined_blend)
    print(f"Guardada escena combinada: {combined_blend}")

if __name__ == "__main__":
    build_stage_blend()
