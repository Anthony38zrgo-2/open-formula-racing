"""Prepare an open-stage route and its shared model transform; no engine changes."""
from pathlib import Path
import math,struct,json,xml.etree.ElementTree as E,shutil
ROOT=Path(__file__).resolve().parents[1];DIST=ROOT/'distribution';SHARE=DIST/'share/games/speed-dreams-2';OUT=ROOT/'staging_rally/integration';OUT.mkdir(exist_ok=True)
TRACK=SHARE/'tracks/circuit/russia-stage1';CAR=SHARE/'cars/models/ford-escort-rs2000'
backup=OUT/'original_runtime'
if not backup.exists():
 backup.mkdir();shutil.copytree(TRACK,backup/'track');shutil.copytree(CAR,backup/'car');shutil.copytree(DIST/'user-settings-rally/config',backup/'rally-config')
data=(ROOT/'staging_rally/tracks/russia-stage1/reset1.bin').read_bytes();raw=[struct.unpack_from('<3f',data,i*72+52) for i in range(len(data)//72)]
a=math.atan2(-(raw[1][0]-raw[0][0]),raw[1][2]-raw[0][2]);c,s=math.cos(a),math.sin(a)
points=[]
for x,y,z in raw:
 dx,dy=z-raw[0][2],-(x-raw[0][0]);points.append((c*dx+s*dy,-s*dx+c*dy+3,y-.2))
def dist(a,b):return math.hypot(b[0]-a[0],b[1]-a[1])
def lerp(a,b,t):return tuple(x+(y-x)*t for x,y in zip(a,b))
entries=list(points);exits=list(points);turns={}
for i in range(1,len(points)-1):
 p,q,r=points[i-1:i+2];l1,l2=dist(p,q),dist(q,r);h1=math.atan2(q[1]-p[1],q[0]-p[0]);h2=math.atan2(r[1]-q[1],r[0]-q[0]);d=(h2-h1+math.pi)%(2*math.pi)-math.pi
 if abs(d)<1e-5:continue
 radius=max(3.1,min(15,.20*min(l1,l2)/math.tan(abs(d)/2)));assert radius*math.tan(abs(d)/2)<.45*min(l1,l2),(i,radius,d)
 trim=radius*math.tan(abs(d)/2);entries[i]=lerp(q,p,trim/l1);exits[i]=lerp(q,r,trim/l2);turns[i]=(radius,d,h1)
segments=[]
for i in range(len(points)-1):
 p,q=exits[i],entries[i+1];length=dist(p,q)
 if length>1e-6:segments.append(dict(kind='str',length=length,start=p,end=q,heading=math.atan2(q[1]-p[1],q[0]-p[0])))
 if i+1 in turns:
  radius,d,h=turns[i+1];segments.append(dict(kind='lft' if d>0 else 'rgt',length=radius*abs(d),radius=radius,arc=abs(d),start=entries[i+1],end=exits[i+1],heading=h))
# Match track4's positive-coordinate normalization, including both road edges.
xs=[0.];ys=[0.];zs=[0.]
for seg in segments:
 p,q=seg['start'],seg['end'];h=seg['heading'];zs.extend([p[2],q[2]])
 if seg['kind']=='str':samples=[(p[0],p[1],h),(q[0],q[1],h)]
 else:
  sign=1 if seg['kind']=='lft' else -1;r=seg['radius'];cx=p[0]-sign*r*math.sin(h);cy=p[1]+sign*r*math.cos(h);end=h+sign*seg['arc'];angles=[h,end]
  for k in range(-8,9):
   t=k*math.pi/2
   if min(h,end)<t<max(h,end):angles.append(t)
  samples=[(cx+sign*r*math.sin(t),cy-sign*r*math.cos(t),t) for t in angles]
 for x,y,t in samples:
  for edge in [-3,3]:xs.append(x-edge*math.sin(t));ys.append(y+edge*math.cos(t))
minimum=(min(xs),min(ys),min(zs));length=sum(seg['length'] for seg in segments)
transform=dict(source_start=list(raw[0]),heading=a,minimum=minimum,route_length=length,spawn_distance=5.0,road_width=6.0)
(OUT/'transform.json').write_text(json.dumps(transform,indent=2));(OUT/'route_segments.json').write_text(json.dumps(segments,indent=2))
tree=E.parse(TRACK/'russia-stage1.xml');root=tree.getroot()
def section(parent,name):
 found=parent.find(f"section[@name='{name}']")
 return found if found is not None else E.SubElement(parent,'section',name=name)
def num(parent,name,value,unit=None):
 el=parent.find(f"attnum[@name='{name}']")
 if el is None:el=E.SubElement(parent,'attnum',name=name)
 el.set('val',str(value))
 if unit:el.set('unit',unit)
def string(parent,name,value):
 el=parent.find(f"attstr[@name='{name}']")
 if el is None:el=E.SubElement(parent,'attstr',name=name)
 el.set('val',value)
main=section(root,'Main Track');num(main,'width',6);num(main,'profil steps length',4);string(main,'track limit penalties','no')
for side in ['Left','Right']:
 for suffix in ['Side','Border','Barrier']:
  sec=section(main,side+' '+suffix);num(sec,'width',0);num(sec,'height',0)
segs=section(main,'Track Segments');segs.clear();segs.set('name','Track Segments')
for i,seg in enumerate(segments):
 sec=E.SubElement(segs,'section',name=f'seg_{i:04d}');string(sec,'type',seg['kind']);string(sec,'profil','linear');string(sec,'surface','gravel')
 if seg['kind']=='str':num(sec,'lg',f"{seg['length']:.9f}",'m')
 else:num(sec,'radius',f"{seg['radius']:.9f}",'m');num(sec,'arc',f"{seg['arc']:.12f}",'rad')
 num(sec,'z start',f"{seg['start'][2]:.8f}",'m');num(sec,'z end',f"{seg['end'][2]:.8f}",'m')
grid=section(root,'Starting Grid');num(grid,'rows',1);num(grid,'distance to start',f'{length-5:.9f}','m');num(grid,'initial height',.6,'m')
num(section(root,'Header'),'version',4)
E.indent(tree);tree.write(TRACK/'russia-stage1.xml',encoding='utf-8',xml_declaration=True)
print(json.dumps(transform,indent=2))

