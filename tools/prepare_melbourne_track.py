"""Convert the extracted GP4 Melbourne mesh into a native, mesh-contact track.

Source files are read only. Reuses Fuji's biarc and native-loader alignment contract.
"""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET

import numpy as np
from scipy.interpolate import CubicSpline

from convert_binary_scene import BinaryScene, export_scene
from prepare_fuji_track import section, number, text

TRACK_ID = 'melbourne-nineteen-ninety-seven'


def strip_sections(primitive):
    """Recover transverse quad edges, preserving the source road topology."""
    name, material, vertices, triangles, uv = primitive
    vertices, inverse = np.unique(np.round(vertices, 3), axis=0, return_inverse=True)
    triangles = inverse[triangles]
    edge_owners = collections.defaultdict(list)
    for index, triangle in enumerate(triangles):
        for j in range(3):
            edge_owners[tuple(sorted((int(triangle[j]), int(triangle[(j+1) % 3]))))].append(index)
    neighbors = collections.defaultdict(list)
    for owners in edge_owners.values():
        if len(owners) == 2:
            a,b = owners
            neighbors[a].append(b)
            neighbors[b].append(a)
    endpoints = [i for i in range(len(triangles)) if len(neighbors[i]) == 1]
    if len(endpoints) != 2:
        raise ValueError(f'{name}: source road strip is not a triangle chain')
    order, previous, current = [], None, endpoints[0]
    while current is not None:
        order.append(current)
        following = [i for i in neighbors[current] if i != previous]
        previous, current = current, following[0] if following else None
    if len(order) != len(triangles) or len(order) % 2:
        raise ValueError(f'{name}: incomplete road strip')
    rings = {}
    for index in range(0,len(order),2):
        pair = triangles[order[index:index+2]]
        edge_counts = collections.Counter(tuple(sorted((int(t[j]), int(t[(j+1) % 3])))) for t in pair for j in range(3))
        boundary = [e for e, count in edge_counts.items() if count == 1]
        across = sorted(boundary, key=lambda e: np.linalg.norm(vertices[e[1]]-vertices[e[0]]))[-2:]
        for edge in across:
            endpoints = vertices[list(edge)]
            center = endpoints.mean(axis=0)
            rings[tuple(np.round(center, 3))] = (center, float(np.linalg.norm(endpoints[1]-endpoints[0])))
    return list(rings.values())


def write_glb(scene, path, category):
    document = json.loads(json.dumps(scene.document))
    for node in document['nodes']:
        if 'mesh' in node:
            original = node.get('name', '')
            node['name'] = f'COLLISION__{category(original)}__{original}'
    encoded = json.dumps(document, separators=(',', ':')).encode()
    encoded += b' ' * (-len(encoded) % 4)
    binary = scene.binary_bytes
    binary += b'\0' * (-len(binary) % 4)
    path.write_bytes(struct.pack('<III', 0x46546c67, 2, 28+len(encoded)+len(binary)) + struct.pack('<II', len(encoded), 0x4e4f534a) + encoded + struct.pack('<II', len(binary), 0x004e4942) + binary)


def category(name):
    if re.fullmatch(r'Track\d+', name) or name.startswith(('PITLANE', 'TRACK_PitFloor', 'PITVERGE', 'TRACK_pitverge')):
        return 'ROAD'
    if name.startswith(('TRACK_L_Kerb', 'TRACK_RKerb')):
        return 'CURB'
    if name.startswith('TRACK_verge'):
        return 'GRASS'
    if name.startswith(('TRACK_wall', 'TRACK_fence_pits', 'REQUIRED_Garage', 'REQUIRED_pit_building')):
        return 'WALL'
    return 'IGNORE'


def sky_mesh(path, azimuth_offset=0):
    # AC is Y up; the scenery background follows the camera. The source image
    # remains unchanged; only spherical geometry and texture coordinates are built.
    longitude_steps, latitude_steps, radius = 128, 64, 8000
    with path.open('w', newline='\n') as out:
        out.write('AC3Db\nMATERIAL "sky" rgb 1 1 1 amb 1 1 1 emis 1 1 1 spec 0 0 0 shi 0 trans 0\nOBJECT world\nkids 1\nOBJECT poly\nname "Melbourne generated environment"\ntexture "melbourne-skybox.png"\n')
        out.write(f'numvert {(longitude_steps+1)*(latitude_steps+1)}\n')
        for row in range(latitude_steps+1):
            latitude = math.pi/2-math.pi*row/latitude_steps
            for column in range(longitude_steps+1):
                angle = math.tau*column/longitude_steps+azimuth_offset
                out.write(f'{radius*math.cos(latitude)*math.cos(angle):.6f} {radius*math.sin(latitude):.6f} {radius*math.cos(latitude)*math.sin(angle):.6f}\n')
        out.write(f'numsurf {longitude_steps*latitude_steps}\n')
        for row in range(latitude_steps):
            for column in range(longitude_steps):
                out.write('SURF 0x30\nmat 0\nrefs 4\n')
                for r, c in [(row, column), (row+1, column), (row+1, column+1), (row, column+1)]:
                    out.write(f'{r*(longitude_steps+1)+c} {c/longitude_steps:.8f} {1-r/latitude_steps:.8f}\n')
        out.write('kids 0\n')


def mark_box(path, material_index, x, y, height, heading):
    """Paint a geometry outline over the existing apron, without editing textures."""
    rotation = np.array([[math.cos(heading),-math.sin(heading)],[math.sin(heading),math.cos(heading)]])
    quads = [[(-3,-1.5),(3,-1.5),(3,-1.4),(-3,-1.4)], [(-3,1.4),(3,1.4),(3,1.5),(-3,1.5)], [(-3,-1.4),(-2.9,-1.4),(-2.9,1.4),(-3,1.4)], [(2.9,-1.4),(3,-1.4),(3,1.4),(2.9,1.4)]]
    content = path.read_text()
    material = 'MATERIAL "Melbourne box 1 paint" rgb 1 1 1 amb .4 .4 .4 emis 0 0 0 spec 0 0 0 shi 0 trans 0\n'
    content = re.sub(r'OBJECT world\nkids (\d+)\n', lambda m: material+f'OBJECT world\nkids {int(m[1])+1}\n',content,count=1)
    content += 'OBJECT poly\nname "PIT_BOX_1"\nnumvert 16\n'
    for quad in quads:
        for point in quad:
            px,py = rotation@point+np.array([x,y])
            content += f'{px:.8f} {height+.015:.8f} {-py:.8f}\n'
    content += 'numsurf 4\n'
    for index in range(4):
        content += f'SURF 0x30\nmat {material_index}\nrefs 4\n'
        content += ''.join(f'{index*4+j} 0 0\n' for j in range(4))
    path.write_text(content+'kids 0\n',newline='\n')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--inspector', type=Path, required=True)
    parser.add_argument('--skybox', type=Path, required=True)
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    project = args.project.resolve()
    destination = project/'implementation/candidates'/TRACK_ID
    report_dir = project/'implementation/reports/melbourne'
    staging = report_dir/'staging'
    for directory in [destination, staging/'metadata']:
        directory.mkdir(parents=True, exist_ok=True)
    scene = BinaryScene(args.source)
    primitives = list(scene.primitives())
    road = sorted([p for p in primitives if re.fullmatch(r'Track\d+', p[0])], key=lambda p: int(p[0][5:]))
    centers = np.array([p[2].mean(axis=0) for p in road])
    rings = []
    for index, primitive in enumerate(road):
        direction = centers[(index+1) % len(road)]-centers[index-1]
        direction[1] = 0
        local = sorted(strip_sections(primitive), key=lambda item: np.dot(item[0], direction))
        for point, width in local:
            if not rings or np.linalg.norm(point-rings[-1][0]) > .01:
                rings.append((point, width))
    if np.linalg.norm(rings[-1][0]-rings[0][0]) < .01:
        rings.pop()
    positions = np.array([r[0] for r in rings])
    gaps = np.linalg.norm(np.roll(positions, -1, axis=0)-positions, axis=1)
    if gaps.max() > 10 or gaps.min() < .1:
        raise ValueError(f'Invalid reconstructed loop gaps {gaps.min()}..{gaps.max()}')
    length = float(gaps.sum())
    # Avoid duplicated periodic endpoints in the Fuji interpolation contract.
    distances = np.r_[0, np.cumsum(gaps[:-1])]
    midpoint_positions = (positions+np.roll(positions, -1, axis=0))/2
    waypoints = [dict(pos=p.tolist(), lap_distance_m=float(d+g/2), width_left=w/2+.1, width_right=w/2+.1, branch_id='main') for p,d,g,(_,w) in zip(midpoint_positions,distances,gaps,rings)]
    (staging/'metadata/track.json').write_text(json.dumps({'lap_length_m': length}))
    (staging/'metadata/racing_line.json').write_text(json.dumps({'waypoints': waypoints}))
    (staging/'metadata/surfaces.json').write_text(json.dumps({'surface_definitions': [{'surface_id': key, 'dry_grip': grip} for key,grip in [('ROAD',1),('CURB',.95),('GRASS',.6)]]}))
    write_glb(scene, staging/'melbourne_collision.glb', category)
    candidate_path = report_dir/'candidate.json'
    subprocess.run([sys.executable, str(project/'tools/prepare_fuji_track.py'), '--source', str(staging), '--destination', str(destination), '--report', str(candidate_path), '--track-id', TRACK_ID, '--name', 'Melbourne 1997', '--collision-file', 'melbourne_collision.glb', '--collision-prefix', 'melbourne'], check=True)
    native_path = report_dir/'native.json'
    distribution = project/'distribution'
    environment = __import__('os').environ.copy()
    environment['PATH'] = str(distribution/'bin')+';'+environment['PATH']
    def inspect():
        subprocess.run([str(args.inspector), (distribution/'share/games/speed-dreams-2').as_posix()+'/', (distribution/'lib/games/speed-dreams-2').as_posix()+'/', (destination/(TRACK_ID+'.xml')).as_posix(), str(native_path)], env=environment, check=True)
        return json.loads(native_path.read_text())
    native = inspect()
    candidate = json.loads(candidate_path.read_text())
    source_to_sim = np.array([[0,0,-1],[-1,0,0],[0,1,0]])
    sim_to_asset = np.array([[1,0,0],[0,0,1],[0,-1,0]])
    rotation = np.eye(3)
    rotation[:2,:2] = candidate['rotation']
    origin = (np.array(native['segments'][0]['start_left'])+native['segments'][0]['start_right'])/2
    offset = origin-rotation@np.array(candidate['source_initial_center'])
    transform = np.eye(4)
    transform[:3,:3] = sim_to_asset@rotation@source_to_sim
    transform[:3,3] = sim_to_asset@offset
    alignment = report_dir/'alignment.json'
    alignment.write_text(json.dumps({'transformation': transform.tolist()}))
    subprocess.run([sys.executable, str(project/'tools/prepare_fuji_object_collisions.py'), '--source', str(staging/'melbourne_collision.glb'), '--alignment', str(alignment), '--destination', str(destination/'melbourne_object_collisions.txt')], check=True)
    excluded = sorted({p[0] for p in primitives if p[0].startswith(('BESTLINE','horizon_','REFLECT_'))})
    visual = export_scene(args.source, destination/(TRACK_ID+'.acc'), transform, excluded)
    (report_dir/'visual.json').write_text(json.dumps(visual, indent=2))
    # Choose the first garage along the pit-lane direction from the actual mesh.
    garage_groups = collections.defaultdict(list)
    for name, material, vertices, triangles, uv in primitives:
        if name.startswith('REQUIRED_Garage_'):
            garage_groups[name].append(vertices)
    garage_centers = {name: np.concatenate(parts).mean(axis=0) for name,parts in garage_groups.items()}
    sim_positions = positions@source_to_sim.T
    first_direction = sim_positions[1,:2]-sim_positions[0,:2]
    first_direction /= np.linalg.norm(first_direction)
    garage_name, garage_source = max(garage_centers.items(), key=lambda item: -np.dot((source_to_sim@item[1])[:2], first_direction))
    garage_sim = source_to_sim@garage_source
    nearest = int(np.argmin(np.linalg.norm(sim_positions[:,:2]-garage_sim[:2],axis=1)))
    garage_distance = float(distances[nearest])
    lane_primitives = [p for p in primitives if p[0].startswith('PITLANE')]
    lane_vertices = np.concatenate([p[2] for p in lane_primitives])@source_to_sim.T
    tangent = sim_positions[(nearest+1)%len(rings),:2]-sim_positions[nearest-1,:2]
    tangent /= np.linalg.norm(tangent)
    left = np.array([-tangent[1], tangent[0]])
    local_lane = lane_vertices[np.abs((lane_vertices[:,:2]-garage_sim[:2])@tangent)<15]
    across = (local_lane[:,:2]-sim_positions[nearest,:2])@left
    lane_offset = float(np.median(across))
    pit_side = 'left' if lane_offset > 0 else 'right'
    native_segments = native['segments']
    def segment_at(distance):
        return min(native_segments, key=lambda segment: abs(segment['distance']-distance))['name']
    document = ET.parse(destination/(TRACK_ID+'.xml')).getroot()
    text(document.find("section[@name='Graphic']"),'native pit buildings','no')
    main_track = document.find("section[@name='Main Track']")
    pits = section(main_track, 'Pits')
    # One service stall; lane entry/exit still cover the complete source pit road.
    lane_indices = [int(np.argmin(np.linalg.norm(sim_positions[:,:2]-p[:2],axis=1))) for p in lane_vertices[::4]]
    pit_distances = distances[lane_indices]
    # Melbourne pit road wraps start/finish. Values beyond half a lap are negative.
    unwrapped = np.where(pit_distances>length/2,pit_distances-length,pit_distances)
    start_distance = garage_distance-6
    for key, value in [('side',pit_side),('entry', segment_at(float(unwrapped.min()%length))),('exit',segment_at(float(unwrapped.max()%length))),('start',segment_at(float(unwrapped.min()%length))),('end',segment_at(float(unwrapped.max()%length))),('start buildings',segment_at(start_distance)),('stop buildings',segment_at(start_distance+10))]:
        text(pits,key,value)
    for key,value in [('max pits',1),('pit style',1),('length',12),('width',4),('speed limit',80/3.6),('pit indicator',1)]:
        number(pits,key,value)
    # Side widths place the native service coordinate on the source pit apron.
    pit_width = max(4, abs(lane_offset)+2-float(rings[nearest][1])/2)
    for index, segment in enumerate(main_track.find("section[@name='Track Segments']")):
        d = candidate['candidate_segments'][index]['source_start_distance']
        d = d-length if d>length/2 else d
        if unwrapped.min()-20 <= d <= unwrapped.max()+20:
            side = section(segment, pit_side.title()+' Side')
            number(side,'width',pit_width)
            text(side,'surface','asphalt')
    ET.indent(document)
    ET.ElementTree(document).write(destination/(TRACK_ID+'.xml'),encoding='utf-8',xml_declaration=True)
    native = inspect()
    if native['pits']['type'] != 1 or native['pits']['max'] != 1:
        raise ValueError('Native loader did not create the single service box')
    service = native['pits']['positions'][0]
    ground = np.loadtxt(destination/'melbourne_ground_collisions.txt',skiprows=1)[:,2:].reshape(-1,3,3)
    xy = np.array([service['x'],service['y']])
    a,b,c = ground[:,0,:2],ground[:,1,:2],ground[:,2,:2]
    ab,ac,q = b-a,c-a,xy-a
    det = ab[:,0]*ac[:,1]-ab[:,1]*ac[:,0]
    valid = np.abs(det)>1e-9
    u = np.divide(q[:,0]*ac[:,1]-q[:,1]*ac[:,0],det,out=np.zeros_like(det),where=valid)
    v = np.divide(ab[:,0]*q[:,1]-ab[:,1]*q[:,0],det,out=np.zeros_like(det),where=valid)
    covered = valid & (u>=-1e-6) & (v>=-1e-6) & (u+v<=1+1e-6)
    if not covered.any():
        raise ValueError('Service box is outside the source ground mesh')
    heights = ground[:,0,2]*(1-u-v)+ground[:,1,2]*u+ground[:,2,2]*v
    ground_height = float(np.median(heights[covered]))
    heading = min(native['segments'],key=lambda s: abs(s['distance']-service['distance']))['angle']
    mark_box(destination/(TRACK_ID+'.acc'),len(scene.document['materials'])+1,service['x'],service['y'],ground_height,heading)
    garage = section(document,'Formula Garage')
    for key,value in [('enabled',1),('open on start',0),('x',service['x']),('y',service['y']),('ground height',ground_height),('heading',heading)]:
        number(garage,key,value)
    ET.indent(document)
    ET.ElementTree(document).write(destination/(TRACK_ID+'.xml'),encoding='utf-8',xml_declaration=True)
    spline = CubicSpline(candidate['source_distances'],candidate['source_centers'],bc_type='periodic')
    errors = [np.linalg.norm((np.array(s['start_left'])+s['start_right'])/2-(rotation@spline(c['source_start_distance'])+offset)) for s,c in zip(native['segments'],candidate['candidate_segments'])]
    end = (np.array(native['segments'][-1]['end_left'])+native['segments'][-1]['end_right'])/2
    closure = float(np.linalg.norm(end-origin))
    if max(errors)>.05 or closure>.05:
        raise ValueError(f'Native alignment failed: {max(errors)}, closure {closure}')
    shutil.copyfile(args.skybox, destination/'melbourne-skybox.png')
    city = np.concatenate([p[2] for p in primitives if p[0].startswith('FBUILD_CityCentre_main')]).mean(axis=0)
    city_direction = rotation@source_to_sim@(city-positions.mean(axis=0))
    city_azimuth = math.atan2(city_direction[1], city_direction[0])
    sky_mesh(destination/'background-sky.ac',-city_azimuth-math.tau*.68)
    report = dict(source=str(args.source), source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(), source_road_chunks=len(road), source_cross_sections=len(rings), length_m=length, max_source_spacing_m=float(gaps.max()), closure_m=closure, max_native_alignment_error_m=max(errors), spawn_before_finish_m=31.12, first_garage_source=garage_name, garage_distance_m=garage_distance, pit_side=pit_side, lane_offset_m=lane_offset, pit_side_width_m=pit_width, native_pits=native['pits'], service_ground_height_m=ground_height, excluded_visual_nodes=excluded, skybox=str(destination/'melbourne-skybox.png'))
    (report_dir/'port.json').write_text(json.dumps(report,indent=2))
    if args.install:
        install(project)
    print(json.dumps(report,indent=2))


def install(project):
    """Activate Melbourne without resetting player input, setup or graphics settings."""
    distribution = project/'distribution'
    data = distribution/'share/games/speed-dreams-2'
    shutil.copytree(project/'implementation/candidates'/TRACK_ID,data/'tracks/circuit'/TRACK_ID,dirs_exist_ok=True)
    roots = [project/'implementation/initial-settings',distribution/'initial-settings',distribution/'user-settings',data]
    from prepare_williams_vehicle import find_section, set_number, set_text
    for root in roots:
        target = root/'config/raceman/williams-melbourne-practice.xml'
        source = target if target.exists() else root/'config/raceman/williams-fuji-practice.xml'
        if not source.exists():
            source = project/'implementation/initial-settings/config/raceman/williams-fuji-practice.xml'
        session = ET.parse(source).getroot()
        session.set('name','Williams Melbourne Practice')
        set_text(find_section(find_section(session,'Tracks'),'1'),'name',TRACK_ID)
        practice = find_section(session,'Practice')
        for rule in ['penalties','invalidate best lap on wall touch','invalidate best lap on corner cutting','corner cutting time penalty']:
            set_text(practice,rule,'no')
        grid = find_section(practice,'Starting Grid')
        set_number(grid,'distance to start',31.12,'m')
        set_number(grid,'rows',1)
        set_number(grid,'initial speed',0,'m/s')
        set_text(practice,'clouds','clear')
        set_text(practice,'time of day','noon')
        ET.indent(session)
        target.parent.mkdir(parents=True,exist_ok=True)
        ET.ElementTree(session).write(target,encoding='utf-8',xml_declaration=True)


if __name__ == '__main__':
    main()
