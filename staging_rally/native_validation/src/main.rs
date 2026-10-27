use std::path::Path;
use vehicle_physics_engine::{vehicle_configuration::load_vehicle_configuration, open_formula::{validate_production_configuration,install_track_visual_world,OpenFormulaSimulation},open_formula_ffi::OfpControlInput,types::Vec3};
fn main() {
 let args:Vec<String>=std::env::args().collect();let p=&args[1];let config=load_vehicle_configuration(Path::new(p)).expect("native profile loader");validate_production_configuration(&config).expect("production configuration");
 println!("NATIVE_PROFILE_VALID mass={} gears={} radius={}",config.chassis.vehicle_mass,config.powertrain.gear_ratios.len(),config.tires.front_tire_radius);
 if args.get(2).map(String::as_str)==Some("--kinematics") {
  let geometry=config.suspension.geometric_suspension.as_ref().unwrap();
  for wheel in vehicle_physics_engine::types::WheelIndex::ALL {
   let corner=geometry.corners.get(wheel);let axle=geometry.axle(wheel);
   let limits=vehicle_physics_engine::suspension_kinematics::travel_envelope(corner,axle.wheel_droop_m,axle.wheel_bump_m,0.,wheel.is_front());
   println!("LIMITS {wheel:?} {limits:?}");
   if !wheel.is_front() {
    assert!((limits.0+axle.wheel_droop_m).abs()<1e-6 && (limits.1-axle.wheel_bump_m).abs()<1e-6,"rear linkage cannot reach the declared stroke");
    for index in 0..=26 {
     let travel=-axle.wheel_droop_m+(axle.wheel_droop_m+axle.wheel_bump_m)*index as f64/26.;
     let solution=vehicle_physics_engine::suspension_kinematics::solve_corner(corner,travel,0.,false,0.,0.,1.).expect("rear kinematic solve");
     let ratio=vehicle_physics_engine::suspension_kinematics::jacobian(corner,travel,0.,false).expect("rear Jacobian").motion_ratio;
     assert!(solution.converged&&!solution.rocker_clamped&&ratio>0.5&&ratio<2.,"rear linkage singular or unreachable at {travel}, ratio={ratio}");
    }
    println!("REAR_FULL_STROKE_VALID {wheel:?}");
   }
   for travel in [-0.06,-0.03,0.,0.03,0.06,0.09,0.12] {
    let solution=vehicle_physics_engine::suspension_kinematics::solve_corner(corner,travel,0.,wheel.is_front(),0.,0.,1.);
    let jacobian=vehicle_physics_engine::suspension_kinematics::jacobian(corner,travel,0.,wheel.is_front());
    if let (Some(sol),Some(jac))=(solution,jacobian){println!("KIN {wheel:?} travel={travel:.3} valid={} rocker_clamp={} ratio={:.6} damper={:.6}",sol.converged,sol.rocker_clamped,jac.motion_ratio,sol.damper_length);}
   }
  }
  return;
 }
 if args.len()<3{return}
 let (identity,_)=install_track_visual_world(&args[2],Some("Russia Stage 1"),Some(4)).expect("track world");println!("TRACK_VALID {:?}",identity);
 let x:f64=args[3].parse().unwrap();let ground:f64=args[4].parse().unwrap();let z:f64=args[5].parse().unwrap();
 let mut sim=OpenFormulaSimulation::create(Some(Path::new(p)),Vec3::new(x,ground+1.,z),0.,None).expect("simulation");let geom=sim.geometry();println!("GEOMETRY {:?}",geom);sim.reset(Vec3::new(x,ground+geom.spawn_height+0.01,z),0.);
 let mut records=Vec::new();
 for i in 0..600 {
  let input=if i<250 {OfpControlInput{brake:1.,..Default::default()}} else if i<450 {OfpControlInput{throttle:0.45,gear_request:1,..Default::default()}} else {OfpControlInput{brake:1.,gear_request:1,..Default::default()}};
  if let Err(e)=sim.step(&input,&[],0.02){eprintln!("STEP_FAILED i={i}: {e}");eprintln!("SNAPSHOT {:?}",sim.snapshot());std::process::exit(2)}
  if i%10==0 || [249,449,599].contains(&i){let s=sim.snapshot();records.push(serde_json::json!({"step":i,"time":s.sim_time_s,"position":[s.position.x,s.position.y,s.position.z],"velocity":[s.linear_velocity.x,s.linear_velocity.y,s.linear_velocity.z],"rpm":s.engine_rpm,"gear":s.gear,"rejected":s.rejected_steps,"grounded":s.wheels.map(|w|w.grounded),"loads":s.wheels.map(|w|w.normal_force_n)}));}
 }
 println!("RUNTIME_RESULT {}",serde_json::to_string_pretty(&records).unwrap());std::fs::write("staging_rally/integration/native_run.json",serde_json::to_string_pretty(&records).unwrap()).unwrap();
}
