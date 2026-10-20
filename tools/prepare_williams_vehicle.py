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


def configure_aerodynamic_load_distribution(document):
    def number(section, name):
        return float(section.find("attnum[@name='" + name + "']").get('val'))

    front = find_section(document, 'Front Wing')
    rear = find_section(document, 'Rear Wing')
    floor = find_section(document, 'Formula Underfloor')
    wings = [front, rear]
    lift_areas = []
    for wing in wings:
        points = find_section(wing, 'Polar').findall('section')
        lift_areas.append(number(wing, 'area') * float(numpy.interp(number(wing, 'angle'),
            [number(point, 'angle') for point in points],
            [number(point, 'downforce coefficient') for point in points])))
    original_floor_area = number(floor, 'lift area')
    total_lift_area = sum(lift_areas) + original_floor_area
    target_floor_area = total_lift_area * 0.4
    transferred_lift_area = original_floor_area - target_floor_area
    front_transfer_fraction = (number(floor, 'xpos') - number(rear, 'xpos')) / (number(front, 'xpos') - number(rear, 'xpos'))
    for wing, lift_area, transfer_fraction in zip(wings, lift_areas, [front_transfer_fraction, 1.0 - front_transfer_fraction]):
        lift_scale = (lift_area + transferred_lift_area * transfer_fraction) / lift_area
        points = find_section(wing, 'Polar').findall('section')
        minimum_drag = min(number(point, 'drag coefficient') for point in points)
        for point in points:
            set_number(point, 'downforce coefficient', number(point, 'downforce coefficient') * lift_scale)
            set_number(point, 'drag coefficient', minimum_drag + (number(point, 'drag coefficient') - minimum_drag) * lift_scale ** 2)
    set_number(floor, 'lift area', target_floor_area, 'm2')


def native_tire_stiffness(sliding_grip_ratio, elasticity_factor, peak_slip_angle):
    shape_factor = 2.0 - math.asin(sliding_grip_ratio) * 2.0 / math.pi
    target_argument = math.tan(math.pi / (2.0 * shape_factor))
    lower_bound, upper_bound = 0.0, 1000.0
    for iteration in range(80):
        candidate = (lower_bound + upper_bound) * 0.5
        if candidate * (1.0 - elasticity_factor) + elasticity_factor * math.atan(candidate) < target_argument:
            lower_bound = candidate
        else:
            upper_bound = candidate
    return (lower_bound + upper_bound) * 0.5 * shape_factor / math.sin(peak_slip_angle)


def configure_tire_grip(document, tires):
    for axle_name in ['front', 'rear']:
        force_profile = tires['force_model'][axle_name]
        sliding_grip_ratio = force_profile['lateral_slide_mu_ratio']
        for side in ['Right', 'Left']:
            wheel = find_section(document, axle_name.title() + ' ' + side + ' Wheel')
            set_number(wheel, 'dynamic friction', sliding_grip_ratio * 100.0, '%')
            set_number(wheel, 'elasticity factor', 0.86)
            set_number(wheel, 'stiffness', native_tire_stiffness(sliding_grip_ratio, 0.86,
                force_profile['lateral_peak_slip_angle_rad']))
            set_number(wheel, 'load factor min', 0.8)
            set_number(wheel, 'load factor max', 1.6)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source-root', required=True)
    parser.add_argument('--data-root', required=True)
    parser.add_argument('--destination', required=True)
    parser.add_argument('--report', required=True)
    parser.add_argument('--front-anti-roll-stiffness', type=float, default=100000.0)
    parser.add_argument('--rear-anti-roll-stiffness', type=float, default=80000.0)
    parser.add_argument('--tire-speed-cooling-factor', type=float, default=0.25)
    parser.add_argument('--tire-hysteresis-factor', type=float, default=1.25)
    arguments = parser.parse_args()
    if not all(math.isfinite(value) and value >= 0.0 for value in
               [arguments.front_anti_roll_stiffness, arguments.rear_anti_roll_stiffness, arguments.tire_speed_cooling_factor,
                arguments.tire_hysteresis_factor]):
        parser.error('Calibration candidate parameters must be finite and nonnegative')
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
    suspension_reference = json.loads(pathlib.Path(__file__).with_name('williams_suspension_reference.json').read_text())
    for relative_path, expected_digest in suspension_reference['source_files'].items():
        if hashlib.sha256((source_root / relative_path).read_bytes()).hexdigest() != expected_digest:
            raise ValueError('Suspension linkage reference requires regeneration: ' + relative_path)
    native_reference_clearance = 0.05
    mass = chassis['vehicle_mass'] + 2 * tires['front']['wheel_mass'] + 2 * tires['rear']['wheel_mass']
    front_hub = suspension['geometry_physical']['corners']['FL']['hub_center']
    rear_hub = suspension['geometry_physical']['corners']['RL']['hub_center']
    source_ground_offset = tires['front']['radius'] - front_hub[1]
    vertical_offset = source_ground_offset - native_reference_clearance
    transformation = numpy.array([[0, 0, -1, 0], [0, 1, 0, vertical_offset], [1, 0, 0, 0], [0, 0, 0, 1]], dtype=float)
    identifier = 'williams-nineteen-ninety-seven'
    conversion_reports = []
    carbon_material_overrides = {'fw19_cf.tGA:fw19_cf.tga': {
        'texture': asset_directory / 'car/source/textures/Carbon_Fiber_diffuse.jpg', 'color_factor': [0.25, 0.25, 0.25, 1]}}
    conversion_reports.append(export_scene(asset_directory / 'car/runtime/williams_fw19_hockenheim_1997_chassis.glb',
        destination_directory / (identifier + '.acc'), transformation,
        material_texture_overrides=carbon_material_overrides, bake_untextured_colors=True))
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
    for parameter_name, parameter_value in [('steering application rate', 1.5), ('steering reversal rate', 2.0),
                                             ('steering centering rate', 3.0), ('steering speed sensitivity', 0.018),
                                             ('steering response time', 0.16),
                                             ('diagnostic telemetry', 0.0), ('correct thermal camber units', 1.0)]:
        set_number(find_section(document, 'Formula Controls'), parameter_name, parameter_value)
    for feature in ['enable abs', 'enable tcl', 'enable esp']:
        set_text(features, feature, 'no')
    set_text(features, 'tire temperature and degradation', 'yes')
    set_text(features, 'shifting aero coordinates', 'yes')
    set_text(features, 'fixed wheel force', 'yes')
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
    for gear_name, ratio in [('r', -abs(powertrain['reverse_ratio'])), ('n', 0)] + [(str(index + 1), ratio) for index, ratio in enumerate(powertrain['gear_ratios'])]:
        gear = find_section(gears, gear_name)
        set_number(gear, 'ratio', ratio)
        set_number(gear, 'efficiency', 0.98)
        set_number(gear, 'inertia', powertrain['gear_inertia'], 'kg.m2')
    differential = find_section(document, 'Rear Differential')
    set_text(differential, 'type', '1.5 WAY LSD')
    set_text(differential, 'restoring coast lock', 'yes')
    set_number(differential, 'ratio', powertrain['final_drive'])
    set_number(differential, 'locking input torque', 90, 'N.m')
    set_number(differential, 'preload torque', 10, 'N.m')
    set_number(differential, 'max slip bias', 0.3)
    set_number(differential, 'coast max slip bias', 0.7)
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
        set_number(anti_roll_bar, 'spring', arguments.front_anti_roll_stiffness if axle_name == 'front' else arguments.rear_anti_roll_stiffness, 'N/m')
        physical_suspension = suspension['geometry_physical'][axle_name]
        suspension_motion_ratio = suspension_reference['axles'][axle_name]['motion_ratio']
        wheel_rate_multiplier = suspension_motion_ratio ** 2
        for side_name, sign in [('Right', -1), ('Left', 1)]:
            wheel = find_section(document, axle_name.title() + ' ' + side_name + ' Wheel')
            tire = tires[axle_name]
            set_number(wheel, 'tire speed cooling factor', arguments.tire_speed_cooling_factor)
            set_number(wheel, 'hysteresis', arguments.tire_hysteresis_factor)
            for attribute, value, unit in [('ypos', sign * abs(hub[0]), 'm'), ('rim diameter', 0.3302, 'm'),
                                           ('tire width', tire['width'], 'm'), ('tire height-width ratio', (tire['radius'] - 0.1651) / tire['width'], None),
                                           ('mass', tire['wheel_mass'], 'kg'), ('inertia', 0.6 * tire['wheel_mass'] * tire['radius'] ** 2, 'kg.m2'),
                                           ('ride height', native_reference_clearance, 'm'), ('camber', suspension[axle_name]['camber'], 'rad'),
                                           ('toe', suspension[axle_name]['toe'], 'rad'), ('mu', tires['surfaces']['Road']['friction'], None),
                                           ('stiffness', 18, None), ('pressure', 1103161.16691, 'Pa')]:
                set_number(wheel, attribute, value, unit)
            corner_suspension = find_section(document, axle_name.title() + ' ' + side_name + ' Suspension')
            for attribute, value, unit in [('spring', physical_suspension['spring_rate_N_per_m'] * wheel_rate_multiplier, 'N/m'),
                                           ('slow bump', physical_suspension['damper_bump_Ns_per_m'] * wheel_rate_multiplier, 'N/m/s'),
                                           ('slow rebound', physical_suspension['damper_rebound_Ns_per_m'] * wheel_rate_multiplier, 'N/m/s'),
                                           ('fast bump', physical_suspension['damper_bump_Ns_per_m'] * physical_suspension['damper_fast_factor'] * wheel_rate_multiplier, 'N/m/s'),
                                           ('fast rebound', physical_suspension['damper_rebound_Ns_per_m'] * physical_suspension['damper_fast_factor'] * wheel_rate_multiplier, 'N/m/s'),
                                           ('bump limit velocity', physical_suspension['damper_knee_m_per_s'] / suspension_motion_ratio, 'm/s'),
                                           ('rebound limit velocity', physical_suspension['damper_knee_m_per_s'] / suspension_motion_ratio, 'm/s'),
                                           ('suspension course', native_reference_clearance + physical_suspension['wheel_droop_m'], 'm'),
                                           ('bellcrank', 1, None), ('packers', native_reference_clearance - physical_suspension['wheel_bump_m'], 'm')]:
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
    set_text(aerodynamics, 'drag coefficient scope', 'whole vehicle reference')
    reference_wing_drag_area = 0.0
    for axle_name, hub in [('front', front_hub), ('rear', rear_hub)]:
        wing_profile = aero['model'][axle_name + '_wing']
        wing = find_section(document, axle_name.title() + ' Wing')
        set_number(wing, 'area', wing_profile['area_m2'], 'm2')
        set_number(wing, 'angle', wing_profile['incidence_deg'] + (1.0 if axle_name == 'rear' else 0.0), 'deg')
        set_number(wing, 'xpos', -hub[2], 'm')
        set_text(wing, 'wing type', 'TABULATED')
        set_number(wing, 'xpos', -wing_profile['position_m'][2], 'm')
        set_number(wing, 'zpos', wing_profile['position_m'][1] + vertical_offset, 'm')
        polar_section = find_section(wing, 'Polar')
        polar_section.clear()
        polar_section.set('name', 'Polar')
        for point_index, polar_point in enumerate(wing_profile['polar']):
            point_section = find_section(polar_section, str(point_index + 1))
            set_number(point_section, 'angle', polar_point['angle_deg'], 'deg')
            set_number(point_section, 'drag coefficient', polar_point['cd'])
            set_number(point_section, 'downforce coefficient', polar_point['cl'])
        reference_drag_coefficient = float(numpy.interp(wing_profile['incidence_deg'],
            [point['angle_deg'] for point in wing_profile['polar']], [point['cd'] for point in wing_profile['polar']]))
        reference_wing_drag_area += wing_profile['area_m2'] * reference_drag_coefficient
    floor_profile = aero['model']['underfloor']
    floor_section = find_section(document, 'Formula Underfloor')
    for parameter_name, source_name in [('soft maximum load ratio', 'soft_max_load_ratio'),
                                         ('hard maximum load ratio', 'hard_max_load_ratio'),
                                         ('hard maximum downforce', 'hard_max_downforce_n')]:
        set_number(floor_section, parameter_name, aero['model']['limits'][source_name])
    for parameter_name, source_name, unit in [
            ('lift area', 'lift_area_m2', 'm2'), ('induced drag ratio', 'induced_drag_ratio', None),
            ('choke height', 'choke_height_m', 'm'), ('optimal height', 'optimal_height_m', 'm'),
            ('high height', 'high_height_m', 'm'), ('front clearance', 'floor_clearance_front_m', 'm'),
            ('rear clearance', 'floor_clearance_rear_m', 'm'), ('optimal rake', 'optimal_rake_deg', 'deg'),
            ('rake window', 'rake_window_deg', 'deg'), ('seal roll', 'seal_roll_rad', 'rad')]:
        set_number(floor_section, parameter_name, floor_profile[source_name], unit)
    set_number(floor_section, 'xpos', -floor_profile['position_m'][2], 'm')
    set_number(floor_section, 'zpos', floor_profile['position_m'][1] + vertical_offset, 'm')
    set_number(aerodynamics, 'reference wing drag area', reference_wing_drag_area +
               floor_profile['lift_area_m2'] * floor_profile['induced_drag_ratio'], 'm2')
    configure_aerodynamic_load_distribution(document)
    configure_tire_grip(document, tires)
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
              'native_reference_clearance': native_reference_clearance,
              'source_ground_offset': source_ground_offset,
              'suspension_reference': suspension_reference,
              'audio_reference_revolutions_per_minute': sample['reference_revolutions_per_minute'], 'geometry': conversion_reports,
              'adapted_parameters': ['native tire force curve', 'wheel stiffness', 'differential slip bias', 'wheel inertia', 'native suspension bellcrank', 'native wing polar'],
              'calibration_candidate': {'front_anti_roll_stiffness': arguments.front_anti_roll_stiffness,
                  'rear_anti_roll_stiffness': arguments.rear_anti_roll_stiffness,
                  'tire_speed_cooling_factor': arguments.tire_speed_cooling_factor,
                  'tire_hysteresis_factor': arguments.tire_hysteresis_factor},
              'underfloor_model': 'native wheel-height approximation with authored height, rake and roll envelopes; no probe confidence or flow memory',
              'physics_calibration_validated': False, 'driver_static': True, 'runtime_visual_validated': False}
    pathlib.Path(arguments.report).write_text(json.dumps(report, indent=2))
    print(json.dumps({key: value for key, value in report.items() if key != 'geometry'}, indent=2))


if __name__ == '__main__':
    main()
