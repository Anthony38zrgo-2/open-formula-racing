"""Export verified FlatOut/Blender scenes to Speed Dreams ACC (run in Blender)."""
import bpy,sys,os,json,math,re
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'D:\open-formula');SHARE=ROOT/'distribution/share/games/speed-dreams-2';MODE=sys.argv[-1]
T=json.loads((ROOT/'staging_rally/integration/transform.json').read_text());dropped=0
if MODE=='stage':out=SHARE/'tracks/circuit/russia-stage1/russia-stage1.acc'
else:out=SHARE/'cars/models/ford-escort-rs2000/ford-escort-rs2000.acc'
def lineage(o):
 s=[]
 while o:s.append(o.get('source_node',o.name).lower());o=o.parent
 return '/'.join(s)
def visible(o):
 while o:
  if o.hide_render or o.hide_get():return False
  o=o.parent
 return True

def ac(p):
 if MODE=='stage':
  x0,y0,z0=T['source_start'];dx,dy=p.y-z0,-(p.x-x0);c,s=math.cos(T['heading']),math.sin(T['heading']);x=c*dx+s*dy-T['minimum'][0];y=-s*dx+c*dy+3-T['minimum'][1];z=p.z-T['minimum'][2]
  return (x,z,-y)
 return (p.y,p.z-.506701,p.x)

def classify(mat):
 n=mat.name.lower()
 if any(k in n for k in ['sky','horizon']):return 'sky_visual'
 if any(k in n for k in ['tree','forest','bush','audience','string','grass_1','grass_plane','power_line','sign','null']):return 'tree_visual'
 if any(k in n for k in ['fence','wall','building','house','rock','trunk','train','polyn']):return 'wall_solid'
 if any(k in n for k in ['terrain','field','ground','grass']):return 'grass_terrain'
 if any(k in n for k in ['road','gravel','crossroad','bridge']):return 'road_gravel'
 return 'tree_visual'

def export(objs,path,wheel=False):
 global dropped
 groups={};mats=[];mi={}
 for obj in objs:
  mesh=obj.data;mesh.calc_loop_triangles();uv=mesh.uv_layers.active.data if mesh.uv_layers.active else None;colors=mesh.color_attributes.active_color
  for tri in mesh.loop_triangles:
   mat=mesh.materials[tri.material_index] if tri.material_index<len(mesh.materials) else None
   if not mat:continue
   tex=next((n.image for n in mat.node_tree.nodes if n.type=='TEX_IMAGE' and n.image),None) if mat.node_tree else None
   texname=Path(mat.get('source_texture') or tex.filepath or tex.name).stem+'.png' if tex else ''
   if texname=='.png':raise RuntimeError('Unnamed texture: '+mat.name)
   tag=classify(mat) if MODE=='stage' else ('wheel' if wheel else 'body')
   key=(tag,mat.name,texname);group=groups.setdefault(key,{'verts':[],'surfs':[],'remap':{}})
   pts=[obj.matrix_world@mesh.vertices[v].co for v in tri.vertices]
   if (pts[1]-pts[0]).cross(pts[2]-pts[0]).length<1e-9:dropped+=1;continue
   rgb=(1.,1.,1.)
   if colors and MODE=='stage' and tag!='sky_visual':
    ids=tri.loops if colors.domain=='CORNER' else tri.vertices
    rgb=tuple(round(sum(colors.data[k].color[i] for k in ids)/3*16)/16 for i in range(3))
   mk=(mat.name,rgb)
   if mk not in mi:mi[mk]=len(mats);mats.append((re.sub('[^A-Za-z0-9_]','_',mat.name),rgb,tag=='sky_visual'))
   refs=[]
   for loop,v,p in zip(tri.loops,tri.vertices,pts):
    if wheel:
     # FL hub in the verified Blender scene; model normalized for OsgWheel scale(dia,width,dia).
     co=((p.y-1.205)/.56,(p.z-.28)/.56,(p.x+.64)/.2)
    else:co=ac(p)
    u,w=uv[loop].uv if uv else (0,0);vk=tuple(round(t,7) for t in (*co,u,w))
    if vk not in group['remap']:group['remap'][vk]=len(group['verts']);group['verts'].append(co)
    refs.append((group['remap'][vk],u,w))
   group['surfs'].append((mi[mk],refs))
 with path.open('w',encoding='ascii',errors='replace',newline='\n') as f:
  f.write('AC3Db\n')
  for name,rgb,sky in mats:
   col=' '.join(f'{v:.5f}' for v in rgb);emis='1 1 1' if sky else '0 0 0';f.write(f'MATERIAL "{name}" rgb {col} amb 0.5 0.5 0.5 emis {emis} spec 0.05 0.05 0.05 shi 10 trans 0\n')
  f.write(f'OBJECT world\nkids {len(groups)}\n')
  for (tag,name,texture),g in groups.items():
   safe=re.sub('[^A-Za-z0-9_]','_',name);f.write(f'OBJECT poly\nname "{tag}_{safe}"\n')
   if texture:f.write(f'texture "{texture}"\n')
   f.write(f"numvert {len(g['verts'])}\n")
   for v in g['verts']:f.write(' '.join(f'{x:.7f}' for x in v)+'\n')
   f.write(f"numsurf {len(g['surfs'])}\n")
   for m,refs in g['surfs']:
    f.write(f'SURF 0x30\nmat {m}\nrefs 3\n')
    for v,u,w in refs:f.write(f'{v} {u:.7f} {w:.7f}\n')
   f.write('kids 0\n')
 print('RALLY_ACC',path,'groups',len(groups),'triangles',sum(len(g['surfs']) for g in groups.values()),'dropped',dropped,flush=True)
 return sum(len(g['surfs']) for g in groups.values())
objs=[o for o in bpy.data.objects if o.type=='MESH' and visible(o)
      and (MODE!='stage' or not o.name.startswith('Sky_'))]
if MODE=='car':
 wheels=[o for o in objs if 'tire_fl' in lineage(o)];assert wheels,'FL wheel geometry missing'
 export([o for o in objs if 'tire_' not in lineage(o)],out)
 wheelout=out.parent/'ford_escort_wheel_0.acc';export(wheels,wheelout,True)
 import shutil
 for i in [1,2,3]:shutil.copy2(wheelout,out.parent/f'ford_escort_wheel_{i}.acc')
else:export(objs,out)


