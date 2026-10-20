import argparse
import copy
import hashlib
import json
import math
import pathlib
import shutil
import wave
import xml.etree.ElementTree as element_tree

import numpy

from convert_binary_scene import export_scene


def find_section(parent, name):
    existing = parent.find("section[@name='" + name + "']")
    return existing if existing is not None else element_tree.SubElement(parent, 'section', name=name)


def set_number(parent, name, value, unit=None):
    existing = parent.find("attnum[@name='" + name + "']")
    if existing is None:
        existing = element_tree.SubElement(parent, 'attnum')
    existing.attrib.clear()
    existing.set('name', name)
    existing.set('val', f'{value:.12g}')
    if unit:
        existing.set('unit', unit)


def set_text(parent, name, value):
    existing = parent.find("attstr[@name='" + name + "']")
    if existing is None:
        existing = element_tree.SubElement(parent, 'attstr')
    existing.attrib.clear()
    existing.set('name', name)
    existing.set('val', str(value))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source-root', required=True)
    parser.add_argument('--data-root', required=True)
    parser.add_argument('--destination', required=True)
    parser.add_argument('--report', required=True)
    arguments = parser.parse_args()
    source_root = pathlib.Path(arguments.source_root)
    data_root = pathlib.Path(arguments.data_root)
    destination_directory = pathlib.Path(arguments.destination)
    destination_directory.mkdir(parents=True, exist_ok=True)
    profile_directory = source_root / 'game/data/vehicles/f1_1997/williams/profiles/hockenheim'
    asset_directory = source_root / 'game/assets/models/vehicles/f1-1997/williams/variants/hockenheim/villenueve'
    profile = {name: json.loads((profile_directory / (name + '.json')).read_text())
               for name in ['chassis', 'powertrain', 'suspension', 'tires', 'aerodynamics', 'brakes', 'fuel', 'steering']}
    powertrain = profile['powertrain']['powertrain']
    chassis = profile['chassis']['chassis']
    geometry = profile['chassis']['geometry']
    tires = profile['tires']['tires']
    suspension = profile['suspension']['suspension']
    mass = chassis['vehicle_mass'] + 2 * tires['front']['wheel_mass'] + 2 * tires['rear']['wheel_mass']
    front_hub = suspension['geometry_physical']['corners']['FL']['hub_center']
    rear_hub = suspension['geometry_physical']['corners']['RL']['hub_center']
    vertical_offset = tires['front']['radius'] - front_hub[1]
    transformation = numpy.array([[0, 0, -1, 0], [0, 1, 0, vertical_offset], [1, 0, 0, 0], [0, 0, 0, 1]], dtype=float)
    identifier = 'williams-nineteen-ninety-seven'
    conversion_reports = []
    conversion_reports.append(export_scene(asset_directory / 'car/runtime/williams_fw19_hockenheim_1997_chassis.glb', destination_directory / (identifier + '.acc'), transformation))
    conversion_reports.append(export_scene(asset_directory / 'driver/runtime/jacques_villeneuve_williams_fw19_1997_driver.glb', destination_directory / 'static_driver.acc', transformation))
    model_prefixes = {}
    for axle_name in ['front', 'rear']:
        for side_name in ['left', 'right']:
            prefix = axle_name + '_' + side_name + '_wheel_'
            model_prefixes[axle_name + ' ' + side_name + ' wheel model'] = prefix
            diameter = 2 * tires[axle_name]['radius']
            width = tires[axle_name]['width']
            wheel_transformation = numpy.array([[0, 0, -1 / diameter, 0], [0, 1 / diameter, 0, 0], [1 / width, 0, 0, 0], [0, 0, 0, 1]], dtype=float)
            if side_name == 'right':
                wheel_transformation[0] *= -1
                wheel_transformation[2] *= -1
            source_path = asset_directory / ('car/runtime/williams_fw19_hockenheim_1997_wheel_' + axle_name + '_' + side_name + '.glb')
            conversion_reports.append(export_scene(source_path, destination_directory / (prefix + '0.acc'), wheel_transformation))
            for speed_index in range(1, 4):
                shutil.copyfile(destination_directory / (prefix + '0.acc'), destination_directory / (prefix + str(speed_index) + '.acc'))
    document = element_tree.parse(data_root / 'cars/models/sc-cavallo-360/sc-cavallo-360.xml').getroot()
    document.set('name', 'Williams FW19 1997 Hockenheim Villeneuve')
    for section_name in ['Exhaust', 'Bonnet', 'Driver', 'Graphic Objects']:
        existing = document.find("section[@name='" + section_name + "']")
        if existing is not None:
            document.remove(existing)
    graphics = find_section(document, 'Graphic Objects')
    ranges = find_section(graphics, 'Ranges')
    first_range = find_section(ranges, '1')
    set_text(first_range, 'car', identifier + '.acc')
    set_number(first_range, 'threshold', 0)
    for attribute, prefix in model_prefixes.items():
        set_text(graphics, attribute, prefix)
    drivers = find_section(graphics, 'Driver')
    static_driver = find_section(drivers, '1')
    set_text(static_driver, 'driver', 'static_driver.acc')
    set_number(static_driver, 'steer', 0)
    for coordinate in ['xpos', 'ypos', 'zpos']:
        set_number(static_driver, coordinate, 0, 'm')
    for section_name in ['Driver', 'Bonnet']:
        camera = find_section(document, section_name)
        for coordinate, value in [('xpos', -0.2), ('ypos', 0), ('zpos', 0.80)]:
            set_number(camera, coordinate, value, 'm')
    features = find_section(document, 'Features')
    set_text(find_section(document, 'Formula Controls'), 'simultaneous gear controls', 'yes')
    set_text(find_section(document, 'Formula Controls'), 'simultaneous pedal controls', 'yes')
    set_text(find_section(document, 'Formula Controls'), 'simultaneous steering controls', 'yes')
    for feature in ['enable abs', 'enable tcl', 'enable esp']:
        set_text(features, feature, 'no')
    set_text(features, 'tire temperature and degradation', 'yes')
    car = find_section(document, 'Car')
    set_text(car, 'category', 'Formula-nineteen-ninety-seven')
    for attribute, value, unit in [('mass', mass, 'kg'), ('body length', 4.44427, 'm'), ('body width', 1.661375, 'm'),
                                   ('body height', 1.154363, 'm'), ('overall length', 4.44427, 'm'), ('overall width', 2.0, 'm'),
                                   ('GC height', vertical_offset + chassis['center_of_gravity_height_offset'], 'm'),
                                   ('front-rear weight repartition', chassis['front_weight_distribution'], None),
                                   ('fuel tank', profile['fuel']['fuel']['capacity_kg'] / 0.75, 'l'),
                                   ('initial fuel', profile['fuel']['fuel']['initial_kg'] / 0.75, 'l'), ('fuel mass mult', 0.75, None)]:
        set_number(car, attribute, value, unit)
    engine = find_section(document, 'Engine')
    for attribute, value, unit in [('capacity', 3, 'l'), ('cylinders', 10, None), ('inertia', powertrain['motor_moment'], 'kg.m2'),
                                   ('revs maxi', powertrain['max_rpm'] + 500, 'rpm'), ('revs limiter', powertrain['max_rpm'], 'rpm'),
                                   ('tickover', powertrain['idle_rpm'], 'rpm'), ('fuel cons factor', 1.0, None)]:
        set_number(engine, attribute, value, unit)
    set_text(engine, 'turbo', 'false')
    data_points = find_section(engine, 'data points')
    data_points.clear()
    data_points.set('name', 'data points')
    for point_index, (normalized_regime, normalized_torque) in enumerate(powertrain['torque_curve']):
        point = find_section(data_points, str(point_index + 1))
        set_number(point, 'rpm', normalized_regime * powertrain['max_rpm'], 'rpm')
        set_number(point, 'Tq', normalized_torque * powertrain['max_torque'], 'N.m')
    transmission = find_section(document, 'Drivetrain')
    set_text(transmission, 'type', 'RWD')
    gearbox = find_section(document, 'Gearbox')
    set_number(gearbox, 'shift time', powertrain['shift_time'], 's')
    gears = find_section(gearbox, 'gears')
    gears.clear()
    gears.set('name', 'gears')
    for gear_name, ratio in [('r', powertrain['reverse_ratio']), ('n', 0)] + [(str(index + 1), ratio) for index, ratio in enumerate(powertrain['gear_ratios'])]:
        gear = find_section(gears, gear_name)
        set_number(gear, 'ratio', ratio)
        set_number(gear, 'efficiency', 0.98)
        set_number(gear, 'inertia', powertrain['gear_inertia'], 'kg.m2')
    differential = find_section(document, 'Rear Differential')
    set_number(differential, 'ratio', powertrain['final_drive'])
    set_number(differential, 'locking input torque', powertrain['differential']['preload_nm'], 'N.m')
    set_number(differential, 'max slip bias', 0.3)
    steer = find_section(document, 'Steer')
    set_number(steer, 'steer lock', profile['steering']['steering']['max_steering_angle'], 'rad')
    set_number(steer, 'steering wheel rotation', 2 * math.pi, 'rad')
    set_number(steer, 'max steer speed', profile['steering']['steering']['steering_speed'], 'rad/s')
    brake_system = find_section(document, 'Brake System')
    set_number(brake_system, 'front-rear brake repartition', profile['brakes']['brakes']['front_brake_bias'])
    set_number(brake_system, 'max pressure', 25000000, 'Pa')
    set_number(brake_system, 'emergency brake pressure', 0, 'Pa')
    for axle_name, hub in [('front', front_hub), ('rear', rear_hub)]:
        axle = find_section(document, axle_name.title() + ' Axle')
        set_number(axle, 'xpos', -hub[2], 'm')
        anti_roll_bar = find_section(document, axle_name.title() + ' Anti-Roll Bar')
        set_number(anti_roll_bar, 'spring', 100000 if axle_name == 'front' else 80000, 'N/m')
        physical_suspension = suspension['geometry_physical'][axle_name]
        for side_name, sign in [('Right', -1), ('Left', 1)]:
            wheel = find_section(document, axle_name.title() + ' ' + side_name + ' Wheel')
            tire = tires[axle_name]
            for attribute, value, unit in [('ypos', sign * abs(hub[0]), 'm'), ('rim diameter', 0.3302, 'm'),
                                           ('tire width', tire['width'], 'm'), ('tire height-width ratio', (tire['radius'] - 0.1651) / tire['width'], None),
                                           ('mass', tire['wheel_mass'], 'kg'), ('inertia', 0.6 * tire['wheel_mass'] * tire['radius'] ** 2, 'kg.m2'),
                                           ('ride height', 0.05, 'm'), ('camber', suspension[axle_name]['camber'], 'rad'),
                                           ('toe', suspension[axle_name]['toe'], 'rad'), ('mu', tires['surfaces']['Road']['friction'], None),
                                           ('stiffness', 18, None), ('pressure', 138000, 'Pa')]:
                set_number(wheel, attribute, value, unit)
            corner_suspension = find_section(document, axle_name.title() + ' ' + side_name + ' Suspension')
            for attribute, value, unit in [('spring', physical_suspension['spring_rate_N_per_m'], 'N/m'),
                                           ('slow bump', physical_suspension['damper_bump_Ns_per_m'], 'N/m/s'),
                                           ('slow rebound', physical_suspension['damper_rebound_Ns_per_m'], 'N/m/s'),
                                           ('fast bump', physical_suspension['damper_bump_Ns_per_m'] * physical_suspension['damper_fast_factor'], 'N/m/s'),
                                           ('fast rebound', physical_suspension['damper_rebound_Ns_per_m'] * physical_suspension['damper_fast_factor'], 'N/m/s'),
                                           ('bump limit velocity', physical_suspension['damper_knee_m_per_s'], 'm/s'),
                                           ('rebound limit velocity', physical_suspension['damper_knee_m_per_s'], 'm/s'),
                                           ('suspension course', physical_suspension['wheel_bump_m'] + physical_suspension['wheel_droop_m'], 'm'),
                                           ('bellcrank', 1, None), ('packers', 0.015, 'm')]:
                set_number(corner_suspension, attribute, value, unit)
            brake = find_section(document, axle_name.title() + ' ' + side_name + ' Brake')
            rotor_diameter = profile['brakes']['brakes']['thermal'][axle_name]['rotor_outer_diameter_m']
            set_number(brake, 'disk diameter', rotor_diameter, 'm')
            set_number(brake, 'piston area', profile['brakes']['brakes']['max_brake_torque'] / 25000000 / (rotor_diameter / 2 * 0.45), 'm2')
            set_number(brake, 'mu', 0.45)
    aero = profile['aerodynamics']['aero']
    aerodynamics = find_section(document, 'Aerodynamics')
    set_number(aerodynamics, 'Cx', aero['drag_coefficient'])
    set_number(aerodynamics, 'front area', aero['frontal_area'], 'm2')
    set_number(aerodynamics, 'front Clift', 0)
    set_number(aerodynamics, 'Clift bias', 0.445)
    set_number(aerodynamics, 'rear Clift', 0)
    for axle_name, hub in [('front', front_hub), ('rear', rear_hub)]:
        wing_profile = aero['model'][axle_name + '_wing']
        wing = find_section(document, axle_name.title() + ' Wing')
        set_number(wing, 'area', wing_profile['area_m2'], 'm2')
        set_number(wing, 'angle', wing_profile['incidence_deg'], 'deg')
        set_number(wing, 'xpos', -hub[2], 'm')
    source_audio = source_root / 'game/sounds/banks/v10-v2-bank/engine_powered_maximum_loop.wav'
    source_audio_digest = hashlib.sha256(source_audio.read_bytes()).hexdigest()
    if source_audio_digest != '9bec0ea0aef8424b7062ab18471e849e660c906c609848a5a3adb8ff1411414c':
        raise ValueError('Selected source audio differs from the approved sample')
    shutil.copyfile(source_audio, destination_directory / source_audio.name)
    bank = json.loads((source_audio.parent / 'manifest.json').read_text())
    sample = next(loop for loop in bank['loops'] if loop['id'] == 'engine_powered_maximum_loop')
    sound = find_section(document, 'Sound')
    set_text(sound, 'engine sample', source_audio.name)
    set_number(sound, 'rpm scale', 600 / (sample['reference_revolutions_per_minute'] * math.pi / 30))
    element_tree.indent(document)
    element_tree.ElementTree(document).write(destination_directory / (identifier + '.xml'), encoding='utf-8', xml_declaration=True)
    with wave.open(str(source_audio), 'rb') as sample_file:
        audio_description = {'channels': sample_file.getnchannels(), 'sample_width_bytes': sample_file.getsampwidth(), 'sample_rate': sample_file.getframerate(), 'frames': sample_file.getnframes()}
    report = {'dry_mass_kilograms': mass, 'chassis_mass_kilograms': chassis['vehicle_mass'], 'wheel_mass_kilograms': mass - chassis['vehicle_mass'],
              'vertical_mesh_offset_meters': vertical_offset, 'audio_sha256': source_audio_digest, 'audio_format': audio_description,
              'audio_reference_revolutions_per_minute': sample['reference_revolutions_per_minute'], 'geometry': conversion_reports,
              'adapted_parameters': ['native tire force curve', 'wheel stiffness', 'differential slip bias', 'wheel inertia', 'native suspension bellcrank', 'native wing polar'],
              'physics_calibration_validated': False, 'driver_static': True, 'runtime_visual_validated': False}
    pathlib.Path(arguments.report).write_text(json.dumps(report, indent=2))
    print(json.dumps({key: value for key, value in report.items() if key != 'geometry'}, indent=2))


if __name__ == '__main__':
    main()
