use vehicle_physics_engine::{suspension::{SuspensionSystem,rest_compression_m},suspension_setup::adopt_antiroll_design_bar,vehicle_config::load_vehicle_configuration,types::{TriRaycastSample,WheelIndex}};
fn main() {
 let mut c=load_vehicle_configuration(std::path::Path::new("D:/open-formula/rust/data/vehicles/williams-fw19-hockenheim/vehicle.json")).unwrap();adopt_antiroll_design_bar(&mut c);
 println!("axle,q_m,motion_ratio,wheel_rate_n_m,bar_rate_n_m,bar_roll_share_pct");
 for (w,q) in [(WheelIndex::FrontLeft,-0.005),(WheelIndex::RearLeft,0.005)] {
  let mut samples=Vec::new();
  for delta in [-0.000001,0.000001] {
   let mut s=SuspensionSystem::new(&c);s.step(&c,&[TriRaycastSample::default();4],1e-5);
   for k in WheelIndex::ALL {s.wheels[k as usize].suspension_compression_m=rest_compression_m(&c,k)+if k.is_front(){-0.005}else{0.005};s.wheels[k as usize].unsprung_velocity_m_s=0.;}
   s.wheels[w as usize].suspension_compression_m+=delta;s.step(&c,&[TriRaycastSample::default();4],1e-5);samples.push(s);
  }
  let a=&samples[0].wheels[w as usize];let b=&samples[1].wheels[w as usize];let k=(b.spring_force-a.spring_force)/(b.suspension_compression_m-a.suspension_compression_m);let kb=samples[1].axles[(w as usize)/2].antiroll_stiffness_n_per_m;
  println!("{w:?},{q},{:.9},{k:.6},{kb:.6},{:.6}",b.motion_ratio,100.*2.*kb/(k+2.*kb));
 }
}
