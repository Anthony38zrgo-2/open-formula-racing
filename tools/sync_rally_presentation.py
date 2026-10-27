from pathlib import Path
import xml.etree.ElementTree as E,json
root=Path(r'D:\open-formula');p=root/'distribution/share/games/speed-dreams-2/cars/models/ford-escort-rs2000/ford-escort-rs2000.xml';t=E.parse(p);r=t.getroot()
def setval(section,name,value):
 s=r.find(f"section[@name='{section}']");a=s.find(f"*[@name='{name}']");a.set('val',str(value))
setval('Car','mass',990);setval('Car','front-rear weight repartition',(1.205-.04)/2.41)
for sec in ['Front Right Wheel','Front Left Wheel','Rear Right Wheel','Rear Left Wheel']:setval(sec,'tire height-width ratio',(.28-.3302/2)/.2)
for sec in ['Driver','Bonnet']:
 a=r.find(f"section[@name='{sec}']/attnum[@name='zpos']");a.set('val',str(float(a.get('val'))-.506701))
power=json.loads((root/'rust/data/vehicles/ford-escort-rs2000/powertrain.json').read_text())['powertrain'];setval('Engine','tickover',power['idle_rpm']);setval('Engine','revs maxi',power['max_rpm']);setval('Engine','revs limiter',power['max_rpm']);setval('Engine','inertia',power['motor_moment']);setval('Gearbox','shift time',power['shift_time']);setval('Rear Differential','ratio',power['final_drive'])
gears=r.find("section[@name='Gearbox']/section[@name='gears']")
for s in list(gears):
 n=s.get('name')
 if n.isdigit() and int(n)>4:gears.remove(s)
 elif n.isdigit():s.find("attnum[@name='ratio']").set('val',str(power['gear_ratios'][int(n)-1]))
 elif n=='r':s.find("attnum[@name='ratio']").set('val',str(-power['reverse_ratio']))
# Distinct original outside wheel geometry is animated by OsgWheel; body ACC excludes it.
E.indent(t);t.write(p,encoding='utf-8',xml_declaration=True)
