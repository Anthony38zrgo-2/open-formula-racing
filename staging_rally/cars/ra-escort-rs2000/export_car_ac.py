
import bpy
import os
import mathutils

car_acc_path = r"D:\open-formula\distribution\share\games\speed-dreams-2\cars\models\ford-escort-rs2000\ford-escort-rs2000.acc"
wheel_acc_path = r"D:\open-formula\distribution\share\games\speed-dreams-2\cars\models\ford-escort-rs2000\ford_escort_wheel_0.acc"

# Objetos para el cuerpo del coche (excluyendo ruedas, crash, lod, lights, camera)
EXCLUDE_OBJ_PREFIX = ('body_crash', 'hood_crash', 'trunk_crash', 'body_lod', 
                      'tire_', 'shadow_tire_', 'interior_lo', 'steering_wheel_lo')
EXCLUDE_TYPES = ('CAMERA', 'LIGHT')

car_objs = []
for obj in bpy.data.objects:
    if obj.type in EXCLUDE_TYPES:
        continue
    if any(obj.name.startswith(p) for p in EXCLUDE_OBJ_PREFIX):
        continue
    if obj.type == 'MESH':
        car_objs.append(obj)

print("Objetos seleccionados para car body:", [o.name for o in car_objs])

# Materiales para el cuerpo
materials_list = []
mat_index_map = {}

for mat in bpy.data.materials:
    if mat.name not in mat_index_map:
        mat_index_map[mat.name] = len(materials_list)
        r, g, b = 0.8, 0.8, 0.8
        if mat.use_nodes and mat.node_tree:
            bsdf = mat.node_tree.nodes.get("Principled BSDF")
            if bsdf and 'Base Color' in bsdf.inputs:
                col = bsdf.inputs['Base Color'].default_value
                r, g, b = col[0], col[1], col[2]
        materials_list.append((mat.name, r, g, b))

if not materials_list:
    materials_list.append(('default', 0.8, 0.8, 0.8))
    mat_index_map['default'] = 0

# 1. Exportar ford-escort-rs2000.acc
with open(car_acc_path, 'w', encoding='ascii', errors='replace') as f:
    f.write('AC3Db\n')
    for mname, r, g, b in materials_list:
        f.write(f'MATERIAL "{mname}" rgb {r:.4f} {g:.4f} {b:.4f}  amb 0.3 0.3 0.3  emis 0 0 0  spec 0.2 0.2 0.2  shi 20  trans 0\n')
    f.write('OBJECT world\n')
    f.write(f'kids {len(car_objs)}\n')

    for obj in car_objs:
        mesh = obj.data
        mesh.calc_loop_triangles()
        
        f.write('OBJECT poly\n')
        f.write(f'name "{obj.name}"\n')
        
        # Textura asociada al objeto
        tex_file = ""
        for m in mesh.materials:
            if m and m.use_nodes and m.node_tree:
                for node in m.node_tree.nodes:
                    if node.type == 'TEX_IMAGE' and node.image:
                        tex_file = os.path.basename(node.image.filepath)
                        if tex_file.lower().endswith('.tga'):
                            tex_file = tex_file[:-4] + '.png'
                        break
            if tex_file:
                break
        if tex_file:
            f.write(f'texture "{tex_file}"\n')

        # Vertices en AC3D coordinates:
        # x_ac = Y_world
        # y_ac = Z_world
        # z_ac = X_world
        verts = mesh.vertices
        f.write(f'numvert {len(verts)}\n')
        for v in verts:
            co_world = obj.matrix_world @ v.co
            x_b, y_b, z_b = co_world.x, co_world.y, co_world.z
            x_ac = y_b
            y_ac = z_b
            z_ac = x_b
            f.write(f'{x_ac:.6f} {y_ac:.6f} {z_ac:.6f}\n')

        triangles = mesh.loop_triangles
        f.write(f'numsurf {len(triangles)}\n')
        uv_layer = mesh.uv_layers.active.data if mesh.uv_layers.active else None

        for tri in triangles:
            mat_idx = 0
            if tri.material_index < len(mesh.materials) and mesh.materials[tri.material_index]:
                mname = mesh.materials[tri.material_index].name
                mat_idx = mat_index_map.get(mname, 0)
            
            f.write('SURF 0x30\n')
            f.write(f'mat {mat_idx}\n')
            f.write('refs 3\n')
            for loop_idx, v_idx in zip(tri.loops, tri.vertices):
                u, v = 0.0, 0.0
                if uv_layer:
                    u, v = uv_layer[loop_idx].uv
                f.write(f'{v_idx} {u:.6f} {v:.6f}\n')

print("Car body AC3D exportado:", car_acc_path)

# 2. Exportar modelo de rueda (tire_fl) normalizado a unit box [-0.5, 0.5]
tire_fl = bpy.data.objects.get('tire_fl')
if tire_fl:
    wheel_mesh = tire_fl.data
    wheel_mesh.calc_loop_triangles()
    loc = tire_fl.location
    radius = 0.280
    width = 0.200

    wheel_materials = []
    wheel_mat_map = {}
    for m in wheel_mesh.materials:
        if m and m.name not in wheel_mat_map:
            wheel_mat_map[m.name] = len(wheel_materials)
            wheel_materials.append((m.name, 0.8, 0.8, 0.8))
    if not wheel_materials:
        wheel_materials.append(('tire_mat', 0.8, 0.8, 0.8))
        wheel_mat_map['tire_mat'] = 0

    wheel_acc_base = r"D:\open-formula\distribution\share\games\speed-dreams-2\cars\models\ford-escort-rs2000\ford_escort_wheel_"
    for stage_idx in range(4):
        out_w_path = f"{wheel_acc_base}{stage_idx}.acc"
        with open(out_w_path, 'w', encoding='ascii', errors='replace') as wf:
            wf.write('AC3Db\n')
            for mname, r, g, b in wheel_materials:
                wf.write(f'MATERIAL "{mname}" rgb {r:.4f} {g:.4f} {b:.4f} amb 0.4 0.4 0.4 spec 0.1 0.1 0.1 shi 10 trans 0\n')
            wf.write('OBJECT world\nkids 1\n')
            wf.write('OBJECT poly\nname "wheel"\n')
            wf.write('texture "Tire_intermediate.png"\n')

            # Normalizar vertices a [-0.5, 0.5]
            wverts = wheel_mesh.vertices
            wf.write(f'numvert {len(wverts)}\n')
            for v in wverts:
                # relative to wheel center loc
                co_rel = (tire_fl.matrix_world @ v.co) - loc
                # x_ac = forward / dia, y_ac = vert / dia, z_ac = lat / width
                x_ac = co_rel.y / (2.0 * radius)
                y_ac = co_rel.z / (2.0 * radius)
                z_ac = co_rel.x / width
                wf.write(f'{x_ac:.6f} {y_ac:.6f} {z_ac:.6f}\n')

            wtris = wheel_mesh.loop_triangles
            wf.write(f'numsurf {len(wtris)}\n')
            wuv = wheel_mesh.uv_layers.active.data if wheel_mesh.uv_layers.active else None

            for tri in wtris:
                mat_idx = 0
                if tri.material_index < len(wheel_mesh.materials) and wheel_mesh.materials[tri.material_index]:
                    mat_idx = wheel_mat_map.get(wheel_mesh.materials[tri.material_index].name, 0)
                wf.write('SURF 0x30\n')
                wf.write(f'mat {mat_idx}\n')
                wf.write('refs 3\n')
                for loop_idx, v_idx in zip(tri.loops, tri.vertices):
                    u, v = 0.0, 0.0
                    if wuv:
                        u, v = wuv[loop_idx].uv
                    wf.write(f'{v_idx} {u:.6f} {v:.6f}\n')
    print("Ruedas normalizadas exportadas (0-3.acc)")
