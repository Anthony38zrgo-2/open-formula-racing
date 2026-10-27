"""Calibrate the existing runtime proxy using the shipped Rally Trophy configuration."""
from pathlib import Path
import re,math,copy
ROOT=Path(__file__).resolve().parents[1];SOURCE=ROOT/'staging_rally/cars/ra-escort-rs2000'
def values(text,key):
 raw=re.search(r'^'+re.escape(key)+r'\s*=\s*([^;]+);',text,re.M).group(1)
 return [float(x) for x in re.findall(r'[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?',raw)]
def calibrate(files):
 car=(SOURCE/'Car.ini').read_text();engine=(SOURCE/'Engine.ini').read_text()
 chassis=files['chassis.json'];ch=chassis['chassis'];inventory=chassis['physical_mass_inventory'];mass=values(car,'Mass')[0];old=ch['vehicle_mass'];ch['vehicle_mass']=mass;inventory['base_budget_kilograms']=mass
 # The original explicitly separates rigid-body Mass from TireMass. Component allocation remains estimated.
 shell=next(c for c in inventory['components'] if c['properties']['name']=='bodywork_and_cage');shell['properties']['mass_kilograms']+=mass-old
 shell['properties']['provenance']['source']='Residual rigid-body budget after estimated component allocations; total from Rally Trophy Car.ini Mass'
 shell['uncertainty']['lower_mass_kilograms']=shell['properties']['mass_kilograms']*.8;shell['uncertainty']['upper_mass_kilograms']=shell['properties']['mass_kilograms']*1.2
 # Verified Blender wheel centre is z=.28; proxy suspension's FL hub is y=-.226701.
 model_shift=.28+.226701;com=values(car,'CenterOfMass');target=(com[0],com[1]+.1-model_shift,-com[2]);ch['center_of_gravity_height_offset']=target[1];ch['front_weight_distribution']=(1.205-target[2])/2.41
 body=[c for c in inventory['components'] if c['properties']['attachment']['kind']=='body'];bm=sum(c['properties']['mass_kilograms'] for c in body)
 for axis,value in zip('xyz',target):
  current=sum(c['properties']['mass_kilograms']*c['properties']['center_of_mass_in_attachment_metres'][axis] for c in body)/bm
  for c in body:c['properties']['center_of_mass_in_attachment_metres'][axis]+=value-current
 files['vehicle.json']['profile_name']='Ford Escort RS2000 1971 - Rally Trophy port (geometric suspension proxy)'
 pt=files['powertrain.json']['powertrain'];gears=int(values(engine,'NumGears')[0]);pt['gear_ratios']=[values(engine,f'Gear{i}Ratio')[1] for i in range(1,gears+1)];pt['final_drive']=values(engine,'EndRatio')[0];pt['reverse_ratio']=abs(values(engine,'GearRRatio')[1]);pt['idle_rpm']=values(engine,'IdleRpm')[0];pt['max_rpm']=9500.;pt['max_torque']=values(engine,'TorqueMax')[0];pt['motor_moment']=values(engine,'InertiaEngine')[0];pt['shift_time']=values(engine,'GearShiftTime')[0]
 pt['torque_curve']=[[rpm/pt['max_rpm'],torque] for i in range(19) for rpm,torque in [values(engine,f'Torque{i:02d}')]]
 # Four forward gears require four entries in each gear-specific assistance table.
 aids=files['driving_assistance.json']['aids']
 for key in ['traction_control_gear_authority','traction_control_gear_max_cut','traction_control_gear_slip_target']:aids[key]=[0.]*gears
 files['brakes.json']['brakes']['front_brake_bias']=values(car,'BrakeBalance')[1]
 files['brakes.json']['brakes']['max_brake_torque']=values(car,'BrakeTorque')[0]
 files['steering.json']['steering']['max_steering_angle']=math.radians(values(car,'TireTurnAngleIn')[0])
 for axle in ['front','rear']:
  files['tires.json']['tires'][axle].update(radius=values(car,'TireRadius')[0],width=values(car,'TireWidth')[0],wheel_mass=values(car,'TireMass')[0])
 # Rally Trophy's X/Z tire friction multipliers are equal. The old port's
 # longitudinal-only reductions were inherited tuning, not source car data.
 tire_source=(SOURCE/'Tires.ini').read_text()
 for surface,index in [('Road',0),('Curb',0),('Gravel',1),('Dirt',3)]:
  files['tires.json']['tires']['surfaces'][surface]['longitudinal_grip_ratio']=values(tire_source,f'Tire{index}ZFriction')[0]/values(tire_source,f'Tire{index}XFriction')[0]
 # A saloon has neither F1 wings nor an aerodynamic ground-effect floor.
 aero=files['aerodynamics.json']['aero']['model']
 for wing in ['front_wing','rear_wing']:
  aero[wing]['area_m2']=0.
 aero['underfloor']['lift_area_m2']=0.
 for corner in files['suspension.json']['suspension']['geometry_physical']['corners'].values():
  if 'provenance' not in corner:continue
  corner['provenance']['note']+='; Numerical double-wishbone proxy, not a measured Escort MacPherson/live axle model.'
  # Lower the complete linkage relative to the body. Keeping every mounting
  # point together preserves the reconstructed linkage while raising the body.
  def lower_points(value, key=''):
   if isinstance(value, dict):
    for child_key, child in value.items(): lower_points(child, child_key)
   elif isinstance(value, list) and len(value)==3 and all(isinstance(n,(int,float)) for n in value) and key!='axis':
    value[1]-=.06
  lower_points(corner)
 suspension=files['suspension.json']['suspension']; geometry=suspension['geometry_physical']
 # The scaled F1 pullrod loses leverage in bump (r falls to ~0.21): the
 # 32 kN/m element supplies <1 kN at the wheel and the stops carry the car.
 # Use parallel links with a non-degenerate upright and a direct-rate rocker.
 # The full -120/+140 mm envelope is reachable with positive spring leverage.
 # This is a numerical proxy for spring support, not a measured live rear axle.
 rear_original=geometry['corners']['RL'];x,y,z=rear_original['hub_center'];joint_x=x+.12
 rear={
  'hub_center':[x,y,z],
  'lower_wishbone':{'inner_front':[-.1,y-.10,z-.20],'inner_rear':[-.1,y-.10,z+.20],'outer':[joint_x,y-.10,z]},
  'upper_wishbone':{'inner_front':[-.1,y+.10,z-.20],'inner_rear':[-.1,y+.10,z+.20],'outer':[joint_x,y+.10,z]},
  'trackrod':{'inner':[-.1,y,z+.15],'outer':[joint_x,y,z+.15]},
  'rod':{'type':'pushrod','attachment':'lower','outer':[joint_x,y-.10,z]},
  'rocker':{'axis':[1.,0.,0.],'pivot':[-.4,.1,z],'pushrod_arm':[-.4,.1,z-.4],'damper_arm':[-.4,.1,z-.4]},
  'damper':{'chassis':[-.4,.45,z-.4]},
  'driveshaft':copy.deepcopy(rear_original['driveshaft']),
  'provenance':{'origin':'reconstructed','note':'Parallel-link numerical spring proxy; unit design motion ratio and reachable -120/+140 mm stroke. Not a measured Escort live axle.'}
 }
 geometry['corners']['RL']=rear
 for axle,rate,bump,rebound,arb in [('front',36000.,2000.,2400.,.20),('rear',32000.,2000.,1800.,.08)]:
  settings=geometry[axle]
  settings.update(spring_rate_N_per_m=rate,spring_free_length_m=.37,
                  damper_bump_Ns_per_m=bump,damper_rebound_Ns_per_m=rebound,
                  wheel_bump_m=.14,wheel_droop_m=.12)
  geometry[axle+'_arb']['ratio']=arb
  suspension[axle]['arb_ratio']=arb
 geometry['rear'].update(damper_min_m=.10,damper_max_m=.60,
                         damper_bump_Ns_per_m=4500.,damper_rebound_Ns_per_m=5500.,
                         damper_fast_factor=.8)
 return {'source_mass_rigid_body_kg':mass,'separate_wheels_kg':4*values(car,'TireMass')[0],'source_gears':gears,'source_final_drive':pt['final_drive'],'source_torque_nm':pt['max_torque'],'source_idle_rpm':pt['idle_rpm'],'model_vertical_shift_m':model_shift,'rally_suspension_raise_m':.06,'rear_linkage':'parallel-link direct spring proxy; positive leverage through the complete stroke','estimated_domains':['suspension kinematics (mandatory geometric proxy)','component mass allocation and inertia','thermal parameters','aerodynamic drag','tire force calibration'],'source_files':['Car.ini','Engine.ini','Tires.ini'],'claim':'Configuration port, not a measurement-validated reconstruction'}

