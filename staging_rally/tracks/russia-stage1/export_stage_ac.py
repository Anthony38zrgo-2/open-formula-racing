
import bpy
import os

acc_path = r"D:\open-formula\distribution\share\games\speed-dreams-2\tracks\circuit\russia-stage1\russia-stage1.acc"
print("Exportando escena Blender a AC3D:", acc_path)

# Nombres y prefijos segun clasificacion de ac3d_track.rs:
# Driveable ground: road, track, gravel, terrain, grass
# Solid obstacles: wall, fence, building
NAME_MAPPING = {
    'Track_Road_And_Terrain': 'road_gravel_track_terrain',
    'Fences_And_Barriers': 'fence_barrier_rail',
    'Buildings_And_Structures': 'wall_building_structure',
    'Foliage_And_Vegetation': 'foliage_trees_grass',
    'Props_And_Markers': 'props_markers_sign'
}

materials_list = []
mat_index_map = {}

for mat in bpy.data.materials:
    if mat.name not in mat_index_map:
        mat_index_map[mat.name] = len(materials_list)
        # Extraer color difuso
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

with open(acc_path, 'w', encoding='ascii', errors='replace') as f:
    f.write('AC3Db\n')
    for mname, r, g, b in materials_list:
        f.write(f'MATERIAL "{mname}" rgb {r:.4f} {g:.4f} {b:.4f}  amb 0.3 0.3 0.3  emis 0 0 0  spec 0.1 0.1 0.1  shi 10  trans 0\n')

    # Objeto raiz
    f.write('OBJECT world\n')
    
    export_objs = [o for o in bpy.data.objects if o.type == 'MESH' and o.name in NAME_MAPPING]
    f.write(f'kids {len(export_objs)}\n')

    for obj in export_objs:
        ac_name = NAME_MAPPING[obj.name]
        mesh = obj.data
        mesh.calc_loop_triangles()
        
        f.write('OBJECT poly\n')
        f.write(f'name "{ac_name}"\n')
        
        # Textura principal del objeto
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

        # Vertices transformados:
        # x_ac = Y_b - 38.3882
        # y_ac = Z_b
        # z_ac = X_b + 2.1750
        verts = mesh.vertices
        f.write(f'numvert {len(verts)}\n')
        for v in verts:
            x_b, y_b, z_b = v.co
            x_ac = y_b - 38.388248
            y_ac = z_b
            z_ac = x_b + 2.175004
            f.write(f'{x_ac:.6f} {y_ac:.6f} {z_ac:.6f}\n')

        # Superficies (triangulos)
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
            
            # Winding: Blender tri.vertices son [v0, v1, v2]
            for loop_idx, v_idx in zip(tri.loops, tri.vertices):
                u, v = 0.0, 0.0
                if uv_layer:
                    u, v = uv_layer[loop_idx].uv
                f.write(f'{v_idx} {u:.6f} {v:.6f}\n')

print("Exportacion AC3D completada con exito.")
