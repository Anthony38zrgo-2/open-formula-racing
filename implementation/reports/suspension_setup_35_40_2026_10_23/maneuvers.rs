//! Synthetic flat-road maneuvers, actual production integrator and force laws.
use std::{fs::File,io::{BufWriter,Write},path::Path};
use vehicle_physics_engine::{
 aero::AeroEnvironment,simulation::{default_spawn_height,VehicleSimulator},
 suspension_setup::adopt_antiroll_design_bar,
 types::{RaycastHit,SurfaceType,TriRaycastSample,Vec3,VehicleInput,WheelIndex},
 vehicle_config::{load_vehicle_configuration,VehicleConfig},wheel_mechanics::{tire_radius,tire_width},
};
const DT:f64=0.002;
fn point(c:&VehicleConfig,z:f64)->Vec3 {
 let g=c.suspension.geometric_suspension.as_ref().unwrap();let f=g.corners.fl.hub_center.z;let r=g.corners.rl.hub_center.z;
 let a=&c.aerodynamics.aero_model.underfloor;let h=a.floor_clearance_front_m.unwrap()+(a.floor_clearance_rear_m.unwrap()-a.floor_clearance_front_m.unwrap())*(z-f)/(r-f);
 Vec3::new(0.,-default_spawn_height(c)+h,z)
}
fn ride(s:&VehicleSimulator)->[f64;2] {
 let g=s.config.suspension.geometric_suspension.as_ref().unwrap();[g.corners.fl.hub_center.z,g.corners.rl.hub_center.z].map(|z|s.state.transform.transform_point(point(&s.config,z)).y)
}
fn advance(s:&mut VehicleSimulator,input:VehicleInput,heights:[f64;4])->bool {
 let c=&s.config;let b=s.state.transform.basis;
 let rays=WheelIndex::ALL.map(|w|{
  let a=c.tires.wheel_anchor_local(&c.chassis,&c.suspension,w);
  let max=(if w.is_front(){c.suspension.front_spring_length}else{c.suspension.rear_spring_length})+tire_radius(c,w);
  let hits=[-1.,0.,1.].map(|side|{
   let o=s.state.transform.transform_point(a+Vec3::RIGHT*(side*tire_width(c,w)*c.suspension.tri_ray_spacing_ratio));
   let d=(o.y-heights[w as usize])/b.y.y;
   if d<0.||d>max{return RaycastHit::default();}
   RaycastHit{is_colliding:true,distance:d,point:o-b.y*d,normal:Vec3::UP,surface:SurfaceType::Road}
  });TriRaycastSample{inner:hits[0],center:hits[1],outer:hits[2]}
 });
 let mut env=AeroEnvironment::unavailable();env.valid_mask=9;
 for (i,z) in [(0,-0.9),(3,0.85)]{env.clearance_m[i]=s.state.transform.transform_point(point(c,z)).y/b.y.y;}
 s.step_with_aerodynamics_environment(&input,&rays,&env,DT);
 s.state.suspension.mechanical_failure.is_none()
}
fn main()->Result<(),Box<dyn std::error::Error>> {
 let args:Vec<String>=std::env::args().collect();let out=Path::new(&args[1]);
 let mut summary=BufWriter::new(File::create(out.join("maneuver_summary.csv"))?);
 writeln!(summary,"profile,case,accepted_s,failed,min_floor_m,max_roll_deg,front_unsupported_wheel_seconds,rear_unsupported_wheel_seconds,stop_wheel_fraction,max_stop_n,final_speed_kph")?;
 for (profile,path) in [("baseline",args[2].as_str()),("candidate",args[3].as_str())]{
  let mut cfg=load_vehicle_configuration(Path::new(path))?;adopt_antiroll_design_bar(&mut cfg);
  for (case,speed,duration) in [("straight_brake",250./3.6,5.),("trail_brake_reversal",180./3.6,4.),("slalom",120./3.6,5.),("one_wheel_bump",80./3.6,3.)]{
   let mut s=VehicleSimulator::new(cfg.clone(),Vec3::new(0.,default_spawn_height(&cfg),0.),0.);
   for _ in 0..2000{assert!(advance(&mut s,VehicleInput{clutch:1.,gear_request:Some(0),..Default::default()},[0.;4]));}
   for w in WheelIndex::ALL{s.state.tires.wheels[w as usize].spin=speed/tire_radius(&cfg,w);}
   for _ in 0..750{s.state.linear_velocity.z=-speed;assert!(advance(&mut s,VehicleInput{clutch:1.,gear_request:Some(0),..Default::default()},[0.;4]));}
   let mut trace=BufWriter::new(File::create(out.join(format!("{profile}_{case}.csv")))?);
   writeln!(trace,"time_s,speed_kph,steer,brake,ride_front_m,ride_rear_m,roll_rad,yaw_rate_rad_s,sideslip_rad,N_FL,N_FR,N_RL,N_RR,stop_FL,stop_FR,stop_RL,stop_RR")?;
   let mut min_floor=1e9f64;let mut roll=0f64;let mut unsupported=[0.;2];let mut stop=0f64;let mut maxstop=0f64;let mut n=0;let mut failed=false;
   for tick in 0..(duration/DT) as usize {
    let t=tick as f64*DT;
    let brake=match case{"straight_brake"=>1.,"trail_brake_reversal"=>if t<0.8{0.7}else{0.},_=>0.};
    let steer=match case{"trail_brake_reversal"=>if t<0.7{0.12*(t/0.25).min(1.)}else if t<1.4{-0.12}else{0.},"slalom"=>0.12*(t*std::f64::consts::TAU*0.7).sin(),_=>0.};
    let mut road=[0.;4];if case=="one_wheel_bump" && (0.7..0.95).contains(&t){road[0]=0.01;}
    if !advance(&mut s,VehicleInput{clutch:1.,gear_request:Some(0),steering:steer,brake,..Default::default()},road){failed=true;break;}
    let h=ride(&s);let angle=s.state.transform.basis.x.y.clamp(-1.,1.).asin();min_floor=min_floor.min(h[0]).min(h[1]);roll=roll.max(angle.abs());
    for (i,w) in s.state.suspension.wheels.iter().enumerate(){
     if w.total_normal_force<1. && s.state.linear_velocity.length()>5.{unsupported[i/2]+=DT;}
     if w.bottom_out_force>1.{stop+=1.;}maxstop=maxstop.max(w.bottom_out_force);
    }
    if tick%5==0 {
     let w=&s.state.suspension.wheels;
     writeln!(trace,"{t:.4},{:.4},{steer:.5},{brake:.5},{:.6},{:.6},{angle:.6},{:.6},{:.6},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4},{:.4}",s.state.linear_velocity.length()*3.6,h[0],h[1],s.state.angular_velocity.y,s.state.transform.basis.inverse_transform_vector(s.state.linear_velocity).x.atan2(-s.state.transform.basis.inverse_transform_vector(s.state.linear_velocity).z),w[0].total_normal_force,w[1].total_normal_force,w[2].total_normal_force,w[3].total_normal_force,w[0].bottom_out_force,w[1].bottom_out_force,w[2].bottom_out_force,w[3].bottom_out_force)?;
    }n+=1;
    if case=="straight_brake" && s.state.linear_velocity.length()<1.{break;}
   }
   writeln!(summary,"{profile},{case},{:.4},{failed},{min_floor:.6},{:.4},{:.4},{:.4},{:.6},{maxstop:.3},{:.3}",n as f64*DT,roll.to_degrees(),unsupported[0],unsupported[1],stop/(4.*n.max(1) as f64),s.state.linear_velocity.length()*3.6)?;
  }
 }
 Ok(())
}
