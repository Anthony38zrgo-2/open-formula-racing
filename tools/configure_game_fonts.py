import argparse
import pathlib
import shutil
import xml.etree.ElementTree as element_tree


def configure_font_settings(configuration_directory):
    for filename in ['screen.xml', 'osghudconfig.xml']:
        path = configuration_directory / filename
        if not path.exists():
            continue
        document = element_tree.parse(path)
        if filename == 'screen.xml':
            for section_name, font_name in [
                ('Menu Font TrueType', 'Libron-Bold.ttf'),
                ('Text Font TrueType', 'Libron-Regular.ttf'),
                ('Console Font TrueType', 'Libron-Bold.ttf'),
            ]:
                section = document.getroot().find("section[@name='" + section_name + "']")
                if section is None:
                    section = element_tree.SubElement(document.getroot(), 'section', name=section_name)
                attribute = section.find("attstr[@name='name']")
                if attribute is None:
                    attribute = element_tree.SubElement(section, 'attstr', name='name')
                attribute.set('val', 'Libron/' + font_name)
        else:
            for attribute in document.iter('attstr'):
                if attribute.get('name', '').startswith('fontFileUrl'):
                    font_name = 'Libron-Bold.ttf' if 'bold' in attribute.get('val', '').lower() else 'Libron-Regular.ttf'
                    attribute.set('val', '/Libron/' + font_name)
        element_tree.indent(document)
        document.write(path, encoding='utf-8', xml_declaration=True)


def install_game_fonts(project):
    distribution = project / 'distribution'
    data_directory = distribution / 'share/games/speed-dreams-2'
    shutil.copytree(project / 'implementation/fonts/Libron',
                    data_directory / 'data/fonts/Libron', dirs_exist_ok=True)
    for configuration_directory in [data_directory / 'config',
                                    project / 'implementation/initial-settings/config',
                                    distribution / 'initial-settings/config',
                                    distribution / 'user-settings/config']:
        configure_font_settings(configuration_directory)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--project', required=True, type=pathlib.Path)
    arguments = parser.parse_args()
    install_game_fonts(arguments.project)


if __name__ == '__main__':
    main()
