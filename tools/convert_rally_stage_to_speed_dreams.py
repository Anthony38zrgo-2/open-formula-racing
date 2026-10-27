"""
Convierte el tramo Russia Stage 1 de Rally Trophy para Speed Dreams y el motor de físicas Rust.
1. Convierte texturas TGA a PNG.
2. Exporta la malla 3D desde Blender a formato AC3D (.acc).
   Mapeo de coordenadas:
     x_ac = Y_blender - 38.3882
     y_ac = Z_blender
     z_ac = X_blender + 2.1750
   Esto asegura que el inicio del tramo (nodo 0 de reset1.bin) queda exactamente en (0, 0, 0)
   y que las coordenadas coinciden al 100% con Rust physics (ac3d_track.rs) y Speed Dreams (sdToRust).
3. Lee reset1.bin (493 nodos) y genera russia-stage1.xml con los segmentos de pista Speed Dreams.
"""

import os
import sys
import struct
import math
import shutil
from pathlib import Path
from PIL import Image

ROOT = Path(r"D:\open-formula")
STAGE_SRC = ROOT / "staging_rally/tracks/russia-stage1"
STAGE_DST = ROOT / "distribution/share/games/speed-dreams-2/tracks/circuit/russia-stage1"
BLENDER_EXE = r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"

def convert_textures():
    print("=== 1. Convirtiendo texturas TGA a PNG ===")
    STAGE_DST.mkdir(parents=True, exist_ok=True)
    tex_dir = STAGE_SRC / "textures"
    count = 0
    for tga_file in tex_dir.glob("*.tga"):
        png_name = tga_file.stem + ".png"
        dst_png = STAGE_DST / png_name
        if not dst_png.exists() or dst_png.stat().st_mtime < tga_file.stat().st_mtime:
            try:
                with Image.open(tga_file) as im:
                    im.save(dst_png, "PNG")
                    count += 1
            except Exception as e:
                print(f"Error convirtiendo {tga_file.name}: {e}")
    print(f"Convertidas {count} texturas a PNG en {STAGE_DST}")

def export_ac3d_mesh():
    print("=== 2. Exportando malla AC3D (.acc) desde Blender ===")
    export_script = STAGE_SRC / "export_stage_ac.py"
    blend_path = STAGE_SRC / "russia_stage1.blend"
    acc_path = STAGE_DST / "russia-stage1.acc"

    py_code = f"""
import bpy
import os

acc_path = r"{acc_path}"
print("Exportando escena Blender a AC3D:", acc_path)

# Nombres y prefijos segun clasificacion de ac3d_track.rs:
# Driveable ground: road, track, gravel, terrain, grass
# Solid obstacles: wall, fence, building
NAME_MAPPING = {{
    'Track_Road_And_Terrain': 'road_gravel_track_terrain',
    'Fences_And_Barriers': 'fence_barrier_rail',
    'Buildings_And_Structures': 'wall_building_structure',
    'Foliage_And_Vegetation': 'foliage_trees_grass',
    'Props_And_Markers': 'props_markers_sign'
}}

materials_list = []
mat_index_map = {{}}

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
    f.write('AC3Db\\n')
    for mname, r, g, b in materials_list:
        f.write(f'MATERIAL "{{mname}}" rgb {{r:.4f}} {{g:.4f}} {{b:.4f}}  amb 0.3 0.3 0.3  emis 0 0 0  spec 0.1 0.1 0.1  shi 10  trans 0\\n')

    # Objeto raiz
    f.write('OBJECT world\\n')
    
    export_objs = [o for o in bpy.data.objects if o.type == 'MESH' and o.name in NAME_MAPPING]
    f.write(f'kids {{len(export_objs)}}\\n')

    for obj in export_objs:
        ac_name = NAME_MAPPING[obj.name]
        mesh = obj.data
        mesh.calc_loop_triangles()
        
        f.write('OBJECT poly\\n')
        f.write(f'name "{{ac_name}}"\\n')
        
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
            f.write(f'texture "{{tex_file}}"\\n')

        # Vertices transformados:
        # x_ac = Y_b - 38.3882
        # y_ac = Z_b
        # z_ac = X_b + 2.1750
        verts = mesh.vertices
        f.write(f'numvert {{len(verts)}}\\n')
        for v in verts:
            x_b, y_b, z_b = v.co
            x_ac = y_b - 38.388248
            y_ac = z_b
            z_ac = x_b + 2.175004
            f.write(f'{{x_ac:.6f}} {{y_ac:.6f}} {{z_ac:.6f}}\\n')

        # Superficies (triangulos)
        triangles = mesh.loop_triangles
        f.write(f'numsurf {{len(triangles)}}\\n')
        
        uv_layer = mesh.uv_layers.active.data if mesh.uv_layers.active else None

        for tri in triangles:
            mat_idx = 0
            if tri.material_index < len(mesh.materials) and mesh.materials[tri.material_index]:
                mname = mesh.materials[tri.material_index].name
                mat_idx = mat_index_map.get(mname, 0)

            f.write('SURF 0x30\\n')
            f.write(f'mat {{mat_idx}}\\n')
            f.write('refs 3\\n')
            
            # Winding: Blender tri.vertices son [v0, v1, v2]
            for loop_idx, v_idx in zip(tri.loops, tri.vertices):
                u, v = 0.0, 0.0
                if uv_layer:
                    u, v = uv_layer[loop_idx].uv
                f.write(f'{{v_idx}} {{u:.6f}} {{v:.6f}}\\n')

print("Exportacion AC3D completada con exito.")
"""
    export_script.write_text(py_code, encoding="utf-8")
    
    import subprocess
    cmd = [BLENDER_EXE, "-b", str(blend_path), "--python", str(export_script)]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("Error exportando AC3D desde Blender:")
        print(res.stderr)
        sys.exit(1)
    else:
        print("Malla AC3D exportada correctamente:")
        size_mb = acc_path.stat().st_size / (1024 * 1024)
        print(f"  Archivo: {acc_path} ({size_mb:.2f} MB)")

def generate_track_xml():
    print("=== 3. Generando russia-stage1.xml a partir de reset1.bin ===")
    reset_bin = STAGE_SRC / "reset1.bin"
    with open(reset_bin, "rb") as f:
        data = f.read()

    stride = 72
    n_nodes = len(data) // stride
    print(f"Total nodos en reset1.bin: {n_nodes}")

    nodes_sd = []
    for i in range(n_nodes):
        offset = i * stride
        # Rally coords: x=lateral, y=elevation, z=progression
        x_r, y_r, z_r = struct.unpack('<3f', data[offset+52 : offset+64])
        # SD coords (centradas en el start line):
        # x_sd = z_r - 38.388248
        # y_sd = -(x_r - (-2.175004)) = -x_r - 2.175004
        # z_sd = y_r
        x_sd = z_r - 38.388248
        y_sd = -x_r - 2.175004
        z_sd = y_r
        dist = struct.unpack('<f', data[offset+68 : offset+72])[0]
        nodes_sd.append((x_sd, y_sd, z_sd, dist))

    # Construir segmentos Speed Dreams
    # Segment 0 es el segmento de salida
    xml_lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<!-- Russia Stage 1 - Rally Trophy conversion for Speed Dreams 2 & Rust Physics -->',
        '<!DOCTYPE params SYSTEM "../../../../src/libs/tgf/params.dtd">',
        '',
        '<params name="Russia Stage 1" type="param" mode="mw">',
        '  <section name="Header">',
        '    <attstr name="name" val="Russia Stage 1" />',
        '    <attstr name="category" val="circuit" />',
        '    <attnum name="version" val="4" />',
        '    <attstr name="author" val="Bugbear Entertainment / Open Formula Port" />',
        '    <attstr name="description" val="Russia Stage 1 - 5.0 km Gravel Rally Stage" />',
        '  </section>',
        '',
        '  <section name="Graphic">',
        '    <attstr name="3d description" val="russia-stage1.acc" />',
        '    <attstr name="background image" val="background.png" />',
        '    <attnum name="background color R" val="0.6" />',
        '    <attnum name="background color G" val="0.7" />',
        '    <attnum name="background color B" val="0.8" />',
        '  </section>',
        '',
        '  <section name="Surfaces">',
        '    <section name="gravel">',
        '      <attnum name="friction" val="0.65" />',
        '      <attnum name="rolling resistance" val="0.08" />',
        '      <attnum name="roughness" unit="m" val="0.03" />',
        '      <attnum name="roughness wavelength" unit="m" val="0.5" />',
        '      <attstr name="texture name" val="road_gravel_1.png" />',
        '      <attstr name="texture type" val="continuous" />',
        '    </section>',
        '    <section name="grass">',
        '      <attnum name="friction" val="0.55" />',
        '      <attnum name="rolling resistance" val="0.10" />',
        '      <attnum name="roughness" unit="m" val="0.05" />',
        '      <attstr name="texture name" val="terrain_grass_1.png" />',
        '      <attstr name="texture type" val="continuous" />',
        '    </section>',
        '    <section name="barrier">',
        '      <attnum name="friction" val="0.30" />',
        '      <attnum name="restitution" val="0.20" />',
        '    </section>',
        '  </section>',
        '',
        '  <section name="Starting Grid">',
        '    <attnum name="rows" val="1" />',
        '    <attstr name="pole position side" val="left" />',
        '    <attnum name="distance to start" unit="m" val="0.0" />',
        '    <attnum name="distance between columns" unit="m" val="15.0" />',
        '    <attnum name="offset within a column" unit="m" val="0.0" />',
        '    <attnum name="initial height" unit="m" val="0.28" />',
        '    <attnum name="initial speed" unit="m/s" val="0.0" />',
        '  </section>',
        '',
        '  <section name="Main Track">',
        '    <attnum name="width" unit="m" val="6.0" />',
        '    <attnum name="profil steps length" unit="m" val="4.0" />',
        '    <attstr name="surface" val="gravel" />',
        '    <!-- Left Side -->',
        '    <section name="Left Side">',
        '      <attnum name="width" unit="m" val="6.0" />',
        '      <attstr name="surface" val="grass" />',
        '    </section>',
        '    <section name="Left Border">',
        '      <attnum name="width" unit="m" val="0.5" />',
        '      <attnum name="height" unit="m" val="0.0" />',
        '      <attstr name="surface" val="grass" />',
        '      <attstr name="style" val="plan" />',
        '    </section>',
        '    <section name="Left Barrier">',
        '      <attnum name="width" unit="m" val="0.2" />',
        '      <attnum name="height" unit="m" val="1.0" />',
        '      <attstr name="surface" val="barrier" />',
        '      <attstr name="style" val="wall" />',
        '    </section>',
        '    <!-- Right Side -->',
        '    <section name="Right Side">',
        '      <attnum name="width" unit="m" val="6.0" />',
        '      <attstr name="surface" val="grass" />',
        '    </section>',
        '    <section name="Right Border">',
        '      <attnum name="width" unit="m" val="0.5" />',
        '      <attnum name="height" unit="m" val="0.0" />',
        '      <attstr name="surface" val="grass" />',
        '      <attstr name="style" val="plan" />',
        '    </section>',
        '    <section name="Right Barrier">',
        '      <attnum name="width" unit="m" val="0.2" />',
        '      <attnum name="height" unit="m" val="1.0" />',
        '      <attstr name="surface" val="barrier" />',
        '      <attstr name="style" val="wall" />',
        '    </section>',
        '    <section name="Track Segments">'
    ]

    total_len = 0.0
    for i in range(n_nodes - 1):
        x0, y0, z0, d0 = nodes_sd[i]
        x1, y1, z1, d1 = nodes_sd[i+1]
        dx = x1 - x0
        dy = y1 - y0
        seg_len = math.sqrt(dx*dx + dy*dy)
        if seg_len < 0.1:
            continue
        total_len += seg_len

        # Calculo de curvatura horizontal
        # Si i > 0, medimos cambio de rumbo
        seg_type = "str"
        radius = 0.0
        arc = 0.0

        if i > 0 and i < n_nodes - 2:
            x_prev, y_prev, _, _ = nodes_sd[i-1]
            x_next, y_next, _, _ = nodes_sd[i+2]
            v_in = (dx, dy)
            v_out = (x_next - x1, y_next - y1)
            len_in = math.sqrt(v_in[0]**2 + v_in[1]**2)
            len_out = math.sqrt(v_out[0]**2 + v_out[1]**2)
            if len_in > 1e-4 and len_out > 1e-4:
                dot = (v_in[0]*v_out[0] + v_in[1]*v_out[1]) / (len_in * len_out)
                dot = max(-1.0, min(1.0, dot))
                cross = v_in[0]*v_out[1] - v_in[1]*v_out[0]
                turn_angle = math.acos(dot)
                if turn_angle > 0.015: # mas de ~0.8 grados de curva
                    arc = turn_angle
                    radius = max(15.0, seg_len / arc)
                    seg_type = "lft" if cross > 0 else "rgt"

        seg_name = f"seg_{i:04d}" if i > 0 else "start_line"
        xml_lines.append(f'      <section name="{seg_name}">')
        xml_lines.append(f'        <attstr name="type" val="{seg_type}" />')
        if seg_type == "str":
            xml_lines.append(f'        <attnum name="lg" unit="m" val="{seg_len:.4f}" />')
        else:
            xml_lines.append(f'        <attnum name="radius" unit="m" val="{radius:.4f}" />')
            xml_lines.append(f'        <attnum name="arc" unit="rad" val="{arc:.6f}" />')

        xml_lines.append(f'        <attnum name="z start" unit="m" val="{z0:.4f}" />')
        xml_lines.append(f'        <attnum name="z end" unit="m" val="{z1:.4f}" />')
        xml_lines.append(f'        <attstr name="surface" val="gravel" />')
        xml_lines.append('      </section>')

    xml_lines.append('    </section>')
    xml_lines.append('  </section>')
    xml_lines.append('</params>')

    xml_path = STAGE_DST / "russia-stage1.xml"
    xml_path.write_text('\n'.join(xml_lines), encoding='utf-8')
    print(f"russia-stage1.xml generado correctamente ({total_len:.2f} m de longitud, {n_nodes-1} segmentos)")

def main():
    convert_textures()
    export_ac3d_mesh()
    generate_track_xml()
    print("=== Conversion de Rusia Stage 1 finalizada con exito ===")

if __name__ == "__main__":
    main()
