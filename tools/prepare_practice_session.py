import argparse
import json
import pathlib
import shutil
import xml.etree.ElementTree as element_tree

from prepare_williams_vehicle import find_section, set_number, set_text


def write_document(document, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    element_tree.indent(document)
    element_tree.ElementTree(document).write(path, encoding='utf-8', xml_declaration=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--data-root', required=True)
    parser.add_argument('--destination', required=True)
    parser.add_argument('--source-root', required=True)
    arguments = parser.parse_args()
    data_root = pathlib.Path(arguments.data_root)
    destination = pathlib.Path(arguments.destination)
    source_root = pathlib.Path(arguments.source_root)
    for directory in ['config', 'drivers/human']:
        shutil.copytree(data_root / directory, destination / directory, dirs_exist_ok=True)
    screen = element_tree.parse(destination / 'config/screen.xml').getroot()
    properties = find_section(screen, 'Validated Screen Properties')
    set_number(properties, 'window width', 1280)
    set_number(properties, 'window height', 720)
    set_text(properties, 'full-screen', 'no')
    write_document(screen, destination / 'config/screen.xml')
    engine = element_tree.parse(destination / 'config/raceengine.xml').getroot()
    modules = find_section(engine, 'Modules')
    set_text(modules, 'graphic', 'osggraph')
    set_text(modules, 'simu', 'simuv5')
    set_text(find_section(engine, 'Race Engine'), 'startpaused', 'off')
    write_document(engine, destination / 'config/raceengine.xml')
    session = element_tree.parse(data_root / 'config/raceman/practice.xml').getroot()
    session.set('name', 'Williams Fuji Practice')
    set_text(find_section(find_section(session, 'Tracks'), '1'), 'name', 'fuji-seventy-six-seventy-seven')
    set_text(find_section(find_section(session, 'Tracks'), '1'), 'category', 'circuit')
    practice = find_section(session, 'Practice')
    set_number(practice, 'laps', 999)
    set_number(practice, 'sessiontime', 0, 's')
    set_text(practice, 'display results', 'no')
    starting_grid = find_section(practice, 'Starting Grid')
    set_number(starting_grid, 'distance to start', 31.12, 'm')
    set_number(starting_grid, 'rows', 1)
    set_number(starting_grid, 'initial speed', 0, 'm/s')
    set_number(starting_grid, 'initial height', 0.35, 'm')
    driver = find_section(find_section(session, 'Drivers'), '1')
    set_text(driver, 'driver name', 'Jacques Villeneuve')
    set_text(driver, 'module', 'human')
    set_number(driver, 'idx', 1)
    set_text(driver, 'car name', 'williams-nineteen-ninety-seven')
    configuration = find_section(session, 'Configuration')
    set_number(configuration, 'current configuration', 4)
    write_document(session, destination / 'config/raceman/williams-fuji-practice.xml')
    human = element_tree.parse(destination / 'drivers/human/human.xml').getroot()
    human_driver = find_section(find_section(find_section(human, 'Robots'), 'index'), '1')
    set_text(human_driver, 'name', 'Jacques Villeneuve')
    set_text(human_driver, 'short name', 'Villeneuve')
    set_text(human_driver, 'car name', 'williams-nineteen-ninety-seven')
    set_text(human_driver, 'skill level', 'pro')
    write_document(human, destination / 'drivers/human/human.xml')
    preferences = element_tree.parse(destination / 'drivers/human/preferences.xml').getroot()
    preference_driver = find_section(find_section(find_section(preferences, 'Preferences'), 'Drivers'), '1')
    set_text(preference_driver, 'control', 'joystick')
    set_text(preference_driver, 'transmission', 'sequential')
    set_text(preference_driver, 'auto reverse', 'no')
    set_text(preference_driver, 'ABS on', 'no')
    set_text(preference_driver, 'ASR on', 'no')
    for binding in ['ABS cmd', 'neutral gear', 'reverse gear']:
        set_text(preference_driver, binding, 'None')
    for attribute, binding in [('throttle', 'Up Arrow'), ('brake', 'Down Arrow'), ('left steer', 'Left Arrow'),
                                ('right steer', 'Right Arrow'), ('up shift', 'a'), ('down shift', 'z'),
                                ('clutch', 'x'), ('ebrake cmd', 'Space'), ('ASR cmd', 't')]:
        set_text(preference_driver, attribute, binding)
    write_document(preferences, destination / 'drivers/human/preferences.xml')
    shutil.copyfile(source_root / 'game/data/input/input_profile.json', destination / 'config/formula_input_profile.json')
    category = element_tree.Element('params', name='Formula 1997', type='template')
    category_car = find_section(category, 'Car')
    set_text(category_car, 'category', 'Formula-nineteen-ninety-seven')
    write_document(category, destination / 'cars/categories/Formula-nineteen-ninety-seven.xml')


if __name__ == '__main__':
    main()
