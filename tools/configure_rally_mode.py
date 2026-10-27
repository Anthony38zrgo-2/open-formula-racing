"""Configure the separate rally car, race rules, and sky."""
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SHARE = ROOT / 'distribution/share/games/speed-dreams-2'
LOCAL = ROOT / 'distribution/user-settings-rally'

def set_str(section, name, value):
    assert section is not None
    element = section.find(f"attstr[@name='{name}']")
    if element is None:
        element = ET.SubElement(section, 'attstr', name=name)
    element.set('val', value)

def save(tree, path):
    ET.indent(tree)
    tree.write(path, encoding='utf-8', xml_declaration=True)

car = SHARE / 'cars/models/ford-escort-rs2000/ford-escort-rs2000.xml'
tree = ET.parse(car)
controls = tree.getroot().find("section[@name='Formula Controls']")
if controls is None:
    controls = ET.SubElement(tree.getroot(), 'section', name='Formula Controls')
for name in ('simultaneous gear controls', 'simultaneous pedal controls', 'simultaneous steering controls'):
    set_str(controls, name, 'yes')
save(tree, car)

track = SHARE / 'tracks/circuit/russia-stage1/russia-stage1.xml'
tree = ET.parse(track)
set_str(tree.getroot().find("section[@name='Main Track']"), 'track limit penalties', 'no')
save(tree, track)

for path in (SHARE / 'config/raceman/rally-russia-stage1.xml', LOCAL / 'config/raceman/rally-russia-stage1.xml'):
    tree = ET.parse(path)
    set_str(tree.getroot().find("section[@name='Practice']"), 'penalties', 'no')
    save(tree, path)

path = LOCAL / 'config/graph.xml'
tree = ET.parse(path)
graphics = tree.getroot().find("section[@name='Graphic']")
for name in ('dynamic sky dome', 'background skydome'):
    set_str(graphics, name, 'enabled')
save(tree, path)
print('Xbox controls enabled, penalties disabled, Speed Dreams sky enabled.')
