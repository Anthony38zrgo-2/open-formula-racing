from pathlib import Path
import xml.etree.ElementTree as E
root=Path(r'D:\open-formula');car=root/'distribution/share/games/speed-dreams-2/cars/models/ford-escort-rs2000/ford-escort-rs2000.xml';t=E.parse(car);r=t.getroot()
for a in r.iter('attnum'):
 if a.get('name')=='initial fuel':a.set('min','0');a.set('max','60')
for a in r.iter('attstr'):
 if a.get('name')=='fixed wheel force':a.set('in','yes,no')
for sec,values in [('Driver',{'xpos':.15,'ypos':.325,'zpos':.57}),('Bonnet',{'xpos':1.0,'ypos':0,'zpos':.65})]:
 s=r.find(f"section[@name='{sec}']")
 for a in s:
  if a.get('name') in values:a.set('val',str(values[a.get('name')]))
E.indent(t);t.write(car,encoding='utf-8',xml_declaration=True)
p=root/'distribution/user-settings-rally/config/graph.xml';t=E.parse(p)
for a in t.getroot().iter('attnum'):
 if a.get('name')=='camera head list':a.set('val','1')
 if a.get('name')=='camera':a.set('val','0')
E.indent(t);t.write(p,encoding='utf-8',xml_declaration=True)
p=root/'distribution/user-settings-rally/drivers/human/preferences.xml';t=E.parse(p)
for a in t.getroot().iter('attnum'):
 if a.get('name')=='brake max':a.set('val','1');a.set('max','1')
E.indent(t);t.write(p,encoding='utf-8',xml_declaration=True)
