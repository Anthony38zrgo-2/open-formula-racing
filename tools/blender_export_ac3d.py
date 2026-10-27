"""Exportador AC3D robusto (se ejecuta dentro de Blender).
Variables de entorno: AC_OUT, AC_MODE (stage|car), AC_OFFSET ("dx,dy,dz" aplicado tras el mapeo).
Mapeo: x_ac = Y_b + dx, y_ac = Z_b + dy, z_ac = X_b + dz.
Un objeto AC3D por (objeto Blender, material) para que cada uno tenga su textura;
nombres de material saneados; 'kids 0' tras cada poly; triangulos degenerados descartados.
"""
import bpy, os, re

out = os.environ['AC_OUT']
mode = os.environ.get('AC_MODE', 'stage')
dx, dy, dz = [float(v) for v in os.environ.get('AC_OFFSET', '0,0,0').split(',')]

STAGE_NAMES = {
    'Track_Road_And_Terrain': 'road_gravel_track_terrain',
    'Fences_And_Barriers': 'fence_barrier_rail',
    'Buildings_And_Structures': 'wall_building_structure',
    'Foliage_And_Vegetation': 'foliage_trees_grass',
    'Props_And_Markers': 'props_markers_sign',
}
CAR_EXCLUDE = ('body_crash', 'hood_crash', 'trunk_crash', 'body_lod', 'tire_', 'shadow_tire_',
               'interior_lo', 'steering_wheel_lo', 'light_', 'ext_pipe', 'indicator_', 'wipers')


def clean(n):
    return re.sub(r'[^A-Za-z0-9_]', '_', n)


def tex_of(mat):
    if mat and mat.use_nodes and mat.node_tree:
        for node in mat.node_tree.nodes:
            if node.type == 'TEX_IMAGE' and node.image:
                n = os.path.basename(node.image.filepath) or node.image.name
                return os.path.splitext(n)[0] + '.png'
    return ''


def color_of(mat):
    if mat and mat.use_nodes and mat.node_tree:
        b = mat.node_tree.nodes.get('Principled BSDF')
        if b:
            c = b.inputs['Base Color'].default_value
            return c[0], c[1], c[2]
    return 0.8, 0.8, 0.8


if mode == 'stage':
    objs = [(o, STAGE_NAMES[o.name]) for o in bpy.data.objects if o.type == 'MESH' and o.name in STAGE_NAMES]
else:
    objs = [(o, clean(o.name)) for o in bpy.data.objects
            if o.type == 'MESH' and not o.name.startswith(CAR_EXCLUDE)]

mats = []
mat_idx = {}
polys = []
dropped = 0
for obj, name in objs:
    mesh = obj.data
    mesh.calc_loop_triangles()
    uv = mesh.uv_layers.active.data if mesh.uv_layers.active else None
    groups = {}
    for tri in mesh.loop_triangles:
        groups.setdefault(tri.material_index, []).append(tri)
    for mi, tris in groups.items():
        mat = mesh.materials[mi] if mi < len(mesh.materials) else None
        key = mat.name if mat else '_none'
        if key not in mat_idx:
            mat_idx[key] = len(mats)
            mats.append((clean(key), color_of(mat)))
        verts, remap, surfs = [], {}, []
        for tri in tris:
            pts = [obj.matrix_world @ mesh.vertices[v].co for v in tri.vertices]
            if mode == 'stage':
                pts = [mesh.vertices[v].co for v in tri.vertices]  # stage: geometria ya en mundo
            if tri.vertices[0] == tri.vertices[1] or tri.vertices[1] == tri.vertices[2] or tri.vertices[0] == tri.vertices[2] \
                    or ((pts[1] - pts[0]).cross(pts[2] - pts[0])).length < 1e-9:
                dropped += 1
                continue
            refs = []
            for loop, v, p in zip(tri.loops, tri.vertices, pts):
                u, w = (uv[loop].uv if uv else (0.0, 0.0))
                k = (v, round(u, 5), round(w, 5))
                if k not in remap:
                    remap[k] = len(verts)
                    verts.append((p.y + dx, p.z + dy, p.x + dz))
                refs.append((remap[k], u, w))
            surfs.append(refs)
        if surfs:
            uvs = {}
            polys.append((name, tex_of(mat), mat_idx[key], verts, surfs, remap))

with open(out, 'w', encoding='ascii', errors='replace', newline='\n') as f:
    f.write('AC3Db\n')
    for n, (r, g, b) in mats:
        f.write(f'MATERIAL "{n}" rgb {r:.4f} {g:.4f} {b:.4f} amb 0.4 0.4 0.4 emis 0 0 0 spec 0.1 0.1 0.1 shi 10 trans 0\n')
    f.write(f'OBJECT world\nkids {len(polys)}\n')
    for name, tex, mi, verts, surfs, _ in polys:
        f.write(f'OBJECT poly\nname "{name}"\n')
        if tex:
            f.write(f'texture "{tex}"\n')
        f.write(f'numvert {len(verts)}\n')
        for x, y, z in verts:
            f.write(f'{x:.6f} {y:.6f} {z:.6f}\n')
        f.write(f'numsurf {len(surfs)}\n')
        for refs in surfs:
            f.write(f'SURF 0x30\nmat {mi}\nrefs 3\n')
            for vi, u, w in refs:
                f.write(f'{vi} {u:.6f} {w:.6f}\n')
        f.write('kids 0\n')
print('AC3D', out, 'polys', len(polys), 'dropped degenerate', dropped)
