"""Apply the Formula HUD coexistence layout to the Open Formula HUD configuration.

The HUD migrated from Formula90s replaces four legacy widget groups and shares the screen
with the groups that stay. This tool is the single place that encodes that arrangement:

* disables only the groups whose information the migrated HUD now provides;
* moves the standings board and the position/dash/status widgets out of the migrated
  rectangles, without changing their content;
* installs the layout configuration of the migrated HUD itself.

The same transformation is applied to the project configuration, to the initial settings
and to the active user settings, so a regenerated distribution keeps the arrangement.
"""

import argparse
import pathlib
import shutil
import xml.etree.ElementTree as element_tree

# Widget groups replaced by the migrated HUD: tachometer + speed + gear, lap times, lap
# counter and the four-wheel pressure/temperature panel.
REPLACED_GROUPS = ['carinfoWidget', 'laptimeWidget', 'racelapsWidget', 'tiresWidget',
                   'rpmWidget', 'shiftlightsWidget', 'driverinputWidget',
                   'carstatusWidget', 'dashitemsWidget', 'trackdataWidget']

# Position/standings move to the upper-left; the minimap sits beside them.
# The old lower-left status/dashboard groups are disabled above, freeing the wheel panel.
MOVED_SECTIONS = {
    # section path inside the group -> (attribute, value)
    ('boardWidget', 'board-player1-background'): [('position-horizontalModifier', '14')],
    ('racepositionWidget', 'position-container'): [('position-horizontalModifier', '14')],
    ('carstatusWidget', 'tcs-icon'): [
        ('position-refObjPoint', 'bl'),
        ('position-myPoint', 'bl'),
        ('position-verticalModifier', '20'),
        ('position-horizontalModifier', '10'),
    ],
    ('dashitemsWidget', 'dash-items-container'): [
        ('position-refObjPoint', 'bl'),
        ('position-myPoint', 'bl'),
        ('position-verticalModifier', '95'),
        ('position-horizontalModifier', '10'),
    ],
}


def apply_hud_layout(path):
    if not path.exists():
        return False
    document = element_tree.parse(path)
    widgets = document.getroot().find("section[@name='widgets']")
    if widgets is None:
        return False
    for group in widgets.findall('section'):
        name = group.get('name')
        if name in REPLACED_GROUPS:
            for attribute in group.findall("attnum[@name='enabled']"):
                attribute.set('val', '0')
        for section in group.findall('section'):
            updates = MOVED_SECTIONS.get((name, section.get('name')))
            if not updates:
                continue
            for attribute_name, value in updates:
                target = section.find("attstr[@name='%s']" % attribute_name)
                if target is None:
                    target = section.find("attnum[@name='%s']" % attribute_name)
                if target is None:
                    tag = 'attnum' if value.replace('-', '').replace('.', '').isdigit() else 'attstr'
                    target = element_tree.SubElement(section, tag, name=attribute_name)
                target.set('val', value)
    element_tree.indent(document)
    document.write(path, encoding='utf-8', xml_declaration=True)
    return True


def install_layout_configuration(project):
    distribution = project / 'distribution'
    data_directory = distribution / 'share/games/speed-dreams-2'
    # The canonical layout document lives in a versioned path: the initial settings tree
    # is a generated artefact and must not be the only copy of a build input.
    layout_source = project / 'implementation/hud/formula_hud.xml'
    for configuration_directory in [
        data_directory / 'config',
        project / 'implementation/initial-settings/config',
        distribution / 'initial-settings/config',
        distribution / 'user-settings/config',
    ]:
        if not configuration_directory.exists():
            configuration_directory.mkdir(parents=True, exist_ok=True)
        destination = configuration_directory / 'formula_hud.xml'
        if destination.resolve() == layout_source.resolve():
            continue
        shutil.copyfile(layout_source, destination)


def configure(project):
    distribution = project / 'distribution'
    data_directory = distribution / 'share/games/speed-dreams-2'
    changed = []
    for configuration_directory in [
        data_directory / 'config',
        project / 'implementation/initial-settings/config',
        distribution / 'initial-settings/config',
        distribution / 'user-settings/config',
    ]:
        path = configuration_directory / 'osghudconfig.xml'
        if apply_hud_layout(path):
            changed.append(str(path))
    install_layout_configuration(project)
    return changed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--project', required=True, type=pathlib.Path)
    arguments = parser.parse_args()
    for path in configure(arguments.project):
        print('updated ' + path)


if __name__ == '__main__':
    main()
