"""
Convierte el modelo del Ford Escort RS2000 (1971) de Rally Trophy a Speed Dreams:
1. Convierte texturas TGA a PNG y prepara audio y preview.
2. Exporta el modelo AC3D (ford-escort-rs2000.acc) y las ruedas (ford_escort_wheel_*.acc) desde Blender.
3. Crea la categoria Rally-Historic.xml.
4. Genera ford-escort-rs2000.xml con la configuracion completa compatible con Speed Dreams y el motor Rust.
"""

import os
import sys
import shutil
import subprocess
from pathlib import Path
from PIL import Image

ROOT = Path(r"D:\open-formula")
CAR_SRC = ROOT / "staging_rally/cars/ra-escort-rs2000"
CAR_DST = ROOT / "distribution/share/games/speed-dreams-2/cars/models/ford-escort-rs2000"
CAT_DST = ROOT / "distribution/share/games/speed-dreams-2/cars/categories"
BLENDER_EXE = r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"

def convert_textures_and_assets():
    print("=== 1. Convirtiendo texturas y assets del coche ===")
    CAR_DST.mkdir(parents=True, exist_ok=True)
    
    # 1. Texturas TGA -> PNG
    tga_files = list(CAR_SRC.glob("*.tga"))
    print(f"Encontradas {len(tga_files)} texturas TGA.")
    for tga in tga_files:
        png_path = CAR_DST / (tga.stem + ".png")
        try:
            with Image.open(tga) as im:
                im.save(png_path, "PNG")
        except Exception as e:
            print(f"Error convirtiendo {tga.name}: {e}")

    # Preview images
    preview_src = CAR_SRC / "InfoPhotoFull.tga"
    if preview_src.exists():
        with Image.open(preview_src) as im:
            im.convert("RGB").save(CAR_DST / "ford-escort-rs2000-preview.jpg", "JPEG", quality=90)
            im.save(CAR_DST / "ford-escort-rs2000.png", "PNG")

    # Audio sample
    audio_src = CAR_SRC / "EE_6k5.wav"
    if audio_src.exists():
        shutil.copyfile(audio_src, CAR_DST / "engine.wav")
        print("Copiado sample de audio engine.wav")

def export_ac3d_car():
    print("=== 2. Exportando AC3D desde Blender ===")
    export_py = CAR_SRC / "export_car_ac.py"
    blend_path = CAR_SRC / "ford_escort_rs2000_1971.blend"
    car_acc = CAR_DST / "ford-escort-rs2000.acc"
    wheel_acc = CAR_DST / "ford_escort_wheel_0.acc"

    script_content = f"""
import bpy
import os
import mathutils

car_acc_path = r"{car_acc}"
wheel_acc_path = r"{wheel_acc}"

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
mat_index_map = {{}}

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
    f.write('AC3Db\\n')
    for mname, r, g, b in materials_list:
        f.write(f'MATERIAL "{{mname}}" rgb {{r:.4f}} {{g:.4f}} {{b:.4f}}  amb 0.3 0.3 0.3  emis 0 0 0  spec 0.2 0.2 0.2  shi 20  trans 0\\n')
    f.write('OBJECT world\\n')
    f.write(f'kids {{len(car_objs)}}\\n')

    for obj in car_objs:
        mesh = obj.data
        mesh.calc_loop_triangles()
        
        f.write('OBJECT poly\\n')
        f.write(f'name "{{obj.name}}"\\n')
        
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
            f.write(f'texture "{{tex_file}}"\\n')

        # Vertices en AC3D coordinates:
        # x_ac = Y_world
        # y_ac = Z_world
        # z_ac = X_world
        verts = mesh.vertices
        f.write(f'numvert {{len(verts)}}\\n')
        for v in verts:
            co_world = obj.matrix_world @ v.co
            x_b, y_b, z_b = co_world.x, co_world.y, co_world.z
            x_ac = y_b
            y_ac = z_b
            z_ac = x_b
            f.write(f'{{x_ac:.6f}} {{y_ac:.6f}} {{z_ac:.6f}}\\n')

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
            for loop_idx, v_idx in zip(tri.loops, tri.vertices):
                u, v = 0.0, 0.0
                if uv_layer:
                    u, v = uv_layer[loop_idx].uv
                f.write(f'{{v_idx}} {{u:.6f}} {{v:.6f}}\\n')

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
    wheel_mat_map = {{}}
    for m in wheel_mesh.materials:
        if m and m.name not in wheel_mat_map:
            wheel_mat_map[m.name] = len(wheel_materials)
            wheel_materials.append((m.name, 0.8, 0.8, 0.8))
    if not wheel_materials:
        wheel_materials.append(('tire_mat', 0.8, 0.8, 0.8))
        wheel_mat_map['tire_mat'] = 0

    wheel_acc_base = r"{CAR_DST / 'ford_escort_wheel_'}"
    for stage_idx in range(4):
        out_w_path = f"{{wheel_acc_base}}{{stage_idx}}.acc"
        with open(out_w_path, 'w', encoding='ascii', errors='replace') as wf:
            wf.write('AC3Db\\n')
            for mname, r, g, b in wheel_materials:
                wf.write(f'MATERIAL "{{mname}}" rgb {{r:.4f}} {{g:.4f}} {{b:.4f}} amb 0.4 0.4 0.4 spec 0.1 0.1 0.1 shi 10 trans 0\\n')
            wf.write('OBJECT world\\nkids 1\\n')
            wf.write('OBJECT poly\\nname "wheel"\\n')
            wf.write('texture "Tire_intermediate.png"\\n')

            # Normalizar vertices a [-0.5, 0.5]
            wverts = wheel_mesh.vertices
            wf.write(f'numvert {{len(wverts)}}\\n')
            for v in wverts:
                # relative to wheel center loc
                co_rel = (tire_fl.matrix_world @ v.co) - loc
                # x_ac = forward / dia, y_ac = vert / dia, z_ac = lat / width
                x_ac = co_rel.y / (2.0 * radius)
                y_ac = co_rel.z / (2.0 * radius)
                z_ac = co_rel.x / width
                wf.write(f'{{x_ac:.6f}} {{y_ac:.6f}} {{z_ac:.6f}}\\n')

            wtris = wheel_mesh.loop_triangles
            wf.write(f'numsurf {{len(wtris)}}\\n')
            wuv = wheel_mesh.uv_layers.active.data if wheel_mesh.uv_layers.active else None

            for tri in wtris:
                mat_idx = 0
                if tri.material_index < len(wheel_mesh.materials) and wheel_mesh.materials[tri.material_index]:
                    mat_idx = wheel_mat_map.get(wheel_mesh.materials[tri.material_index].name, 0)
                wf.write('SURF 0x30\\n')
                wf.write(f'mat {{mat_idx}}\\n')
                wf.write('refs 3\\n')
                for loop_idx, v_idx in zip(tri.loops, tri.vertices):
                    u, v = 0.0, 0.0
                    if wuv:
                        u, v = wuv[loop_idx].uv
                    wf.write(f'{{v_idx}} {{u:.6f}} {{v:.6f}}\\n')
    print("Ruedas normalizadas exportadas (0-3.acc)")
"""
    export_py.write_text(script_content, encoding="utf-8")
    cmd = [BLENDER_EXE, "-b", str(blend_path), "--python", str(export_py)]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("Error exportando modelo car AC3D:")
        print(res.stderr)
        sys.exit(1)
    else:
        print(f"Modelo AC3D exportado exitosamente: {car_acc.stat().st_size} bytes")

def create_category():
    CAT_DST.mkdir(parents=True, exist_ok=True)
    cat_file = CAT_DST / "Rally-Historic.xml"
    content = """<?xml version='1.0' encoding='utf-8'?>
<params name="Rally Historic" type="template">
  <section name="Car">
    <attstr name="category" val="Rally-Historic" />
  </section>
</params>
"""
    cat_file.write_text(content, encoding="utf-8")
    print(f"Categoria creada: {cat_file}")

def create_car_xml():
    xml_path = CAR_DST / "ford-escort-rs2000.xml"
    content = """<?xml version='1.0' encoding='utf-8'?>
<params name="Ford Escort RS2000 1971" type="template">
  <section name="Features">
    <attstr name="shifting aero coordinate" in="yes" val="yes" />
    <attstr name="tire temperature and degradation" val="yes" />
    <attstr name="enable abs" val="no" />
    <attstr name="enable tcl" val="no" />
    <attstr name="enable esp" val="no" />
    <attstr name="fixed wheel force" val="yes" />
  </section>
  <section name="Sound">
    <attstr name="engine sample" val="engine.wav" />
    <attnum name="rpm scale" val="0.88" />
  </section>
  <section name="Car">
    <attstr name="category" val="Rally-Historic" />
    <attnum name="body length" val="4.03" unit="m" />
    <attnum name="body width" val="1.62" unit="m" />
    <attnum name="body height" val="1.30" unit="m" />
    <attnum name="overall length" val="4.03" unit="m" />
    <attnum name="overall width" val="1.62" unit="m" />
    <attnum name="mass" val="900" unit="kg" />
    <attnum name="GC height" val="0.45" unit="m" />
    <attnum name="front-rear weight repartition" val="0.53" />
    <attnum name="front right-left weight repartition" val="0.5" />
    <attnum name="rear right-left weight repartition" val="0.5" />
    <attnum name="mass repartition coefficient" val="0.8" />
    <attnum name="fuel tank" val="60" unit="l" />
    <attnum name="initial fuel" val="25" unit="l" />
    <attnum name="fuel mass mult" val="0.75" />
  </section>
  <section name="Aerodynamics">
    <attnum name="Cx" val="0.42" />
    <attnum name="front area" val="1.85" unit="m2" />
    <attnum name="front Clift" val="0" />
    <attnum name="rear Clift" val="0" />
    <attnum name="Clift bias" val="0.53" />
    <attstr name="drag coefficient scope" val="whole vehicle reference" />
  </section>
  <section name="Engine">
    <attnum name="capacity" val="2.0" unit="l" />
    <attnum name="cylinders" val="4" />
    <attstr name="shape" in="v,l,h,w" val="l" />
    <attstr name="position" in="front,front-mid,mid,rear-mid,rear," val="front" />
    <attnum name="inertia" val="0.12" unit="kg.m2" />
    <attnum name="revs maxi" val="8500" unit="rpm" />
    <attnum name="revs limiter" val="8000" unit="rpm" />
    <attnum name="tickover" val="1000" unit="rpm" />
    <attnum name="fuel cons factor" val="1" />
    <attnum name="brake linear coefficient" val="0.015" />
    <attnum name="brake coefficient" val="0.026" />
    <attstr name="turbo" val="false" />
    <section name="data points">
      <section name="1">
        <attnum name="rpm" val="0" unit="rpm" />
        <attnum name="Tq" val="90" unit="N.m" />
      </section>
      <section name="2">
        <attnum name="rpm" val="1000" unit="rpm" />
        <attnum name="Tq" val="115" unit="N.m" />
      </section>
      <section name="3">
        <attnum name="rpm" val="2500" unit="rpm" />
        <attnum name="Tq" val="160" unit="N.m" />
      </section>
      <section name="4">
        <attnum name="rpm" val="4000" unit="rpm" />
        <attnum name="Tq" val="200" unit="N.m" />
      </section>
      <section name="5">
        <attnum name="rpm" val="5500" unit="rpm" />
        <attnum name="Tq" val="215" unit="N.m" />
      </section>
      <section name="6">
        <attnum name="rpm" val="6800" unit="rpm" />
        <attnum name="Tq" val="195" unit="N.m" />
      </section>
      <section name="7">
        <attnum name="rpm" val="8000" unit="rpm" />
        <attnum name="Tq" val="155" unit="N.m" />
      </section>
    </section>
  </section>
  <section name="Clutch">
    <attnum name="inertia" unit="kg.m2" val="0.085" />
  </section>
  <section name="Gearbox">
    <attnum name="shift time" val="0.18" unit="s" />
    <section name="gears">
      <section name="r">
        <attnum name="ratio" val="-3.65" />
        <attnum name="efficiency" val="0.96" />
        <attnum name="inertia" val="0.012" unit="kg.m2" />
      </section>
      <section name="n">
        <attnum name="ratio" val="0" />
        <attnum name="efficiency" val="0.96" />
        <attnum name="inertia" val="0.012" unit="kg.m2" />
      </section>
      <section name="1">
        <attnum name="ratio" val="2.54" />
        <attnum name="efficiency" val="0.96" />
        <attnum name="inertia" val="0.012" unit="kg.m2" />
      </section>
      <section name="2">
        <attnum name="ratio" val="1.66" />
        <attnum name="efficiency" val="0.96" />
        <attnum name="inertia" val="0.012" unit="kg.m2" />
      </section>
      <section name="3">
        <attnum name="ratio" val="1.25" />
        <attnum name="efficiency" val="0.96" />
        <attnum name="inertia" val="0.012" unit="kg.m2" />
      </section>
      <section name="4">
        <attnum name="ratio" val="1.00" />
        <attnum name="efficiency" val="0.96" />
        <attnum name="inertia" val="0.012" unit="kg.m2" />
      </section>
    </section>
  </section>
  <section name="Drivetrain">
    <attstr name="type" val="RWD" />
    <attnum name="inertia" unit="kg.m2" val="0.009" />
  </section>
  <section name="Steer">
    <attnum name="steer lock" val="0.60" unit="rad" />
    <attnum name="steering wheel rotation" val="6.28318530718" unit="rad" />
    <attnum name="max steer speed" val="8" unit="rad/s" />
  </section>
  <section name="Brake System">
    <attnum name="front-rear brake repartition" val="0.62" min="0.50" max="0.75" />
    <attnum name="max pressure" val="15000000" unit="Pa" />
    <attnum name="emergency brake pressure" val="5000000" unit="Pa" />
  </section>
  <section name="Front Axle">
    <attnum name="xpos" val="1.205" unit="m" />
    <attnum name="inertia" unit="kg.m2" val="0.006" />
  </section>
  <section name="Rear Axle">
    <attnum name="xpos" val="-1.205" unit="m" />
    <attnum name="inertia" unit="kg.m2" val="0.008" />
  </section>
  <section name="Front Differential">
    <attstr name="type" in="NONE" val="NONE" />
  </section>
  <section name="Rear Differential">
    <attstr name="type" val="1.5 WAY LSD" />
    <attnum name="locking input torque" val="80" unit="N.m" />
    <attnum name="max slip bias" val="0.25" />
    <attnum name="inertia" unit="kg.m2" val="0.04" />
    <attnum name="ratio" val="4.444" />
    <attnum name="efficiency" val="0.95" />
    <attstr name="restoring coast lock" val="yes" />
    <attnum name="preload torque" val="15" unit="N.m" />
    <attnum name="coast max slip bias" val="0.20" />
  </section>
  <section name="Front Right Wheel">
    <attnum name="ypos" val="-0.640" unit="m" />
    <attnum name="rim diameter" val="0.3302" unit="m" />
    <attnum name="tire width" val="0.200" unit="m" />
    <attnum name="tire height-width ratio" val="0.574" />
    <attnum name="inertia" val="0.80" unit="kg.m2" />
    <attnum name="mass" val="20" unit="kg" />
    <attnum name="ride height" val="0.18" unit="m" />
    <attnum name="toe" val="0.0" unit="rad" />
    <attnum name="camber" val="-0.017" unit="rad" />
    <attnum name="pressure" val="180000" unit="Pa" />
    <attnum name="stiffness" val="45.0" />
    <attnum name="dynamic friction" val="85" unit="%" />
    <attnum name="elasticity factor" val="0.85" />
    <attnum name="operating load" unit="N" val="2500" />
    <attnum name="mu" val="1.15" />
  </section>
  <section name="Front Left Wheel">
    <attnum name="ypos" val="0.640" unit="m" />
    <attnum name="rim diameter" val="0.3302" unit="m" />
    <attnum name="tire width" val="0.200" unit="m" />
    <attnum name="tire height-width ratio" val="0.574" />
    <attnum name="inertia" val="0.80" unit="kg.m2" />
    <attnum name="mass" val="20" unit="kg" />
    <attnum name="ride height" val="0.18" unit="m" />
    <attnum name="toe" val="0.0" unit="rad" />
    <attnum name="camber" val="-0.017" unit="rad" />
    <attnum name="pressure" val="180000" unit="Pa" />
    <attnum name="stiffness" val="45.0" />
    <attnum name="dynamic friction" val="85" unit="%" />
    <attnum name="elasticity factor" val="0.85" />
    <attnum name="operating load" unit="N" val="2500" />
    <attnum name="mu" val="1.15" />
  </section>
  <section name="Rear Right Wheel">
    <attnum name="ypos" val="-0.640" unit="m" />
    <attnum name="rim diameter" val="0.3302" unit="m" />
    <attnum name="tire width" val="0.200" unit="m" />
    <attnum name="tire height-width ratio" val="0.574" />
    <attnum name="inertia" val="0.80" unit="kg.m2" />
    <attnum name="mass" val="20" unit="kg" />
    <attnum name="ride height" val="0.18" unit="m" />
    <attnum name="toe" val="0.0" unit="rad" />
    <attnum name="camber" val="0.0" unit="rad" />
    <attnum name="pressure" val="180000" unit="Pa" />
    <attnum name="stiffness" val="45.0" />
    <attnum name="dynamic friction" val="85" unit="%" />
    <attnum name="elasticity factor" val="0.85" />
    <attnum name="operating load" unit="N" val="2500" />
    <attnum name="mu" val="1.15" />
  </section>
  <section name="Rear Left Wheel">
    <attnum name="ypos" val="0.640" unit="m" />
    <attnum name="rim diameter" val="0.3302" unit="m" />
    <attnum name="tire width" val="0.200" unit="m" />
    <attnum name="tire height-width ratio" val="0.574" />
    <attnum name="inertia" val="0.80" unit="kg.m2" />
    <attnum name="mass" val="20" unit="kg" />
    <attnum name="ride height" val="0.18" unit="m" />
    <attnum name="toe" val="0.0" unit="rad" />
    <attnum name="camber" val="0.0" unit="rad" />
    <attnum name="pressure" val="180000" unit="Pa" />
    <attnum name="stiffness" val="45.0" />
    <attnum name="dynamic friction" val="85" unit="%" />
    <attnum name="elasticity factor" val="0.85" />
    <attnum name="operating load" unit="N" val="2500" />
    <attnum name="mu" val="1.15" />
  </section>
  <section name="Front Anti-Roll Bar">
    <attnum name="spring" val="25000" unit="N/m" />
  </section>
  <section name="Rear Anti-Roll Bar">
    <attnum name="spring" val="18000" unit="N/m" />
  </section>
  <section name="Front Right Suspension">
    <attnum name="spring" val="32000" unit="N/m" />
    <attnum name="suspension course" val="0.22" unit="m" />
    <attnum name="bellcrank" val="1" />
    <attnum name="slow bump" val="2500" unit="N/m/s" />
    <attnum name="slow rebound" val="4200" unit="N/m/s" />
    <attnum name="fast bump" val="1400" unit="N/m/s" />
    <attnum name="fast rebound" val="2600" unit="N/m/s" />
  </section>
  <section name="Front Left Suspension">
    <attnum name="spring" val="32000" unit="N/m" />
    <attnum name="suspension course" val="0.22" unit="m" />
    <attnum name="bellcrank" val="1" />
    <attnum name="slow bump" val="2500" unit="N/m/s" />
    <attnum name="slow rebound" val="4200" unit="N/m/s" />
    <attnum name="fast bump" val="1400" unit="N/m/s" />
    <attnum name="fast rebound" val="2600" unit="N/m/s" />
  </section>
  <section name="Rear Right Suspension">
    <attnum name="spring" val="26000" unit="N/m" />
    <attnum name="suspension course" val="0.24" unit="m" />
    <attnum name="bellcrank" val="1" />
    <attnum name="slow bump" val="2200" unit="N/m/s" />
    <attnum name="slow rebound" val="3800" unit="N/m/s" />
    <attnum name="fast bump" val="1200" unit="N/m/s" />
    <attnum name="fast rebound" val="2200" unit="N/m/s" />
  </section>
  <section name="Rear Left Suspension">
    <attnum name="spring" val="26000" unit="N/m" />
    <attnum name="suspension course" val="0.24" unit="m" />
    <attnum name="bellcrank" val="1" />
    <attnum name="slow bump" val="2200" unit="N/m/s" />
    <attnum name="slow rebound" val="3800" unit="N/m/s" />
    <attnum name="fast bump" val="1200" unit="N/m/s" />
    <attnum name="fast rebound" val="2200" unit="N/m/s" />
  </section>
  <section name="Front Right Brake">
    <attnum name="disk diameter" val="0.244" unit="m" />
    <attnum name="piston area" val="0.0022" unit="m2" />
    <attnum name="mu" val="0.42" />
    <attnum name="inertia" unit="kg.m2" val="0.05" />
  </section>
  <section name="Front Left Brake">
    <attnum name="disk diameter" val="0.244" unit="m" />
    <attnum name="piston area" val="0.0022" unit="m2" />
    <attnum name="mu" val="0.42" />
    <attnum name="inertia" unit="kg.m2" val="0.05" />
  </section>
  <section name="Rear Right Brake">
    <attnum name="disk diameter" val="0.228" unit="m" />
    <attnum name="piston area" val="0.0018" unit="m2" />
    <attnum name="mu" val="0.38" />
    <attnum name="inertia" unit="kg.m2" val="0.04" />
  </section>
  <section name="Rear Left Brake">
    <attnum name="disk diameter" val="0.228" unit="m" />
    <attnum name="piston area" val="0.0018" unit="m2" />
    <attnum name="mu" val="0.38" />
    <attnum name="inertia" unit="kg.m2" val="0.04" />
  </section>
  <section name="Formula Controls">
    <attstr name="simultaneous gear controls" val="yes" />
    <attstr name="simultaneous pedal controls" val="yes" />
    <attstr name="simultaneous steering controls" val="yes" />
  </section>
  <section name="Graphic Objects">
    <section name="Ranges">
      <section name="1">
        <attstr name="car" val="ford-escort-rs2000.acc" />
        <attnum name="threshold" val="0" />
        <attstr name="wheels" val="yes" />
      </section>
    </section>
    <attstr name="front left wheel model" val="ford_escort_wheel_" />
    <attstr name="front right wheel model" val="ford_escort_wheel_" />
    <attstr name="rear left wheel model" val="ford_escort_wheel_" />
    <attstr name="rear right wheel model" val="ford_escort_wheel_" />
  </section>
  <section name="Driver">
    <attnum name="xpos" val="0.10" unit="m" />
    <attnum name="ypos" val="0.32" unit="m" />
    <attnum name="zpos" val="0.85" unit="m" />
  </section>
  <section name="Bonnet">
    <attnum name="xpos" val="0.80" unit="m" />
    <attnum name="ypos" val="0.0" unit="m" />
    <attnum name="zpos" val="0.80" unit="m" />
  </section>
</params>
"""
    xml_path.write_text(content, encoding="utf-8")
    print(f"Definicion de coche creada: {xml_path}")

def main():
    convert_textures_and_assets()
    export_ac3d_car()
    create_category()
    create_car_xml()
    print("=== Conversion de coche completada exitosamente ===")

if __name__ == '__main__':
    main()
