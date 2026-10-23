//! Configure and measure the actual production suspension. No alternate integrator.
use std::{fs, path::Path};
use vehicle_physics_engine::{
    aero::AeroEnvironment,
    simulation::{default_spawn_height, VehicleSimulator},
    suspension_kinematics::{jacobian, solve_corner},
    suspension_setup::adopt_antiroll_design_bar,
    types::{RaycastHit, SurfaceType, TriRaycastSample, Vec3, VehicleInput, WheelIndex},
    vehicle_config::{load_vehicle_configuration, VehicleConfig},
    vehicle_mass_properties::resolve_vehicle_mass_properties_for_suspension_backend,
    suspension::SuspensionBackend,
    wheel_mechanics::{tire_radius, tire_width, WheelMechanicalTuning},
};

fn floor_local(c: &VehicleConfig, z: f64) -> Vec3 {
    let g=c.suspension.geometric_suspension.as_ref().unwrap();
    let f=g.corners.fl.hub_center.z; let r=g.corners.rl.hub_center.z;
    let a=&c.aerodynamics.aero_model.underfloor;
    let h=a.floor_clearance_front_m.unwrap()+(a.floor_clearance_rear_m.unwrap()-a.floor_clearance_front_m.unwrap())*(z-f)/(r-f);
    Vec3::new(0.,-default_spawn_height(c)+h,z)
}
fn rides(s:&VehicleSimulator)->[f64;2] {
    let g=s.config.suspension.geometric_suspension.as_ref().unwrap();
    [g.corners.fl.hub_center.z,g.corners.rl.hub_center.z].map(|z|s.state.transform.transform_point(floor_local(&s.config,z)).y)
}
fn samples(s:&VehicleSimulator)->[TriRaycastSample;4] {
    let c=&s.config;let b=s.state.transform.basis;
    WheelIndex::ALL.map(|w|{
        let anchor=c.tires.wheel_anchor_local(&c.chassis,&c.suspension,w);
        let len=if w.is_front(){c.suspension.front_spring_length}else{c.suspension.rear_spring_length}+tire_radius(c,w);
        let hits=[-1.,0.,1.].map(|side|{
            let o=s.state.transform.transform_point(anchor+Vec3::RIGHT*(side*tire_width(c,w)*c.suspension.tri_ray_spacing_ratio));
            let d=o.y/b.y.y;
            if d<0.||d>len{return RaycastHit::default();}
            RaycastHit{is_colliding:true,distance:d,point:o-b.y*d,normal:Vec3::UP,surface:SurfaceType::Road}
        });
        TriRaycastSample{inner:hits[0],center:hits[1],outer:hits[2]}
    })
}
fn environment(s:&VehicleSimulator)->AeroEnvironment {
    let mut e=AeroEnvironment::unavailable();e.valid_mask=9;
    for (i,z) in [(0,-0.90),(3,0.85)] {
        let p=s.state.transform.transform_point(floor_local(&s.config,z));
        e.clearance_m[i]=p.y/s.state.transform.basis.y.y;
    }e
}
fn advance(s:&mut VehicleSimulator,input:VehicleInput) {
    let r=samples(s);let e=environment(s);
    s.step_with_aerodynamics_environment(&input,&r,&e,1./500.);
    assert!(s.state.suspension.mechanical_failure.is_none(),"{:?}",s.state.suspension.mechanical_failure);
}
fn settle(c:&VehicleConfig)->VehicleSimulator {
    let mut s=VehicleSimulator::new(c.clone(),Vec3::new(0.,default_spawn_height(c),0.),0.);
    for _ in 0..2000 {advance(&mut s,VehicleInput{clutch:1.,gear_request:Some(0),..Default::default()});}
    s
}
fn spring(c:&VehicleConfig,w:WheelIndex,q:f64)->(f64,f64) {
    let g=c.suspension.geometric_suspension.as_ref().unwrap();let a=g.axle(w);let p=g.corners.get(w);
    let s=solve_corner(p,q,0.,w.is_front(),0.,0.,1.).unwrap().damper_compression;
    let r=jacobian(p,q,0.,w.is_front()).unwrap().motion_ratio;
    (a.spring_rate_N_per_m*(a.spring_free_length_m-a.spring_installed_length_m+s).max(0.)*r,r)
}
fn run(c:&VehicleConfig,speed:f64)->(VehicleSimulator,f64,f64,f64) {
    let mut s=settle(c);let mut min=1e9f64;let mut stops=0.;let mut frames=0.;
    for w in WheelIndex::ALL {s.state.tires.wheels[w as usize].spin=speed/tire_radius(c,w);}
    for n in 0..1500 {
        s.state.linear_velocity.x=0.;s.state.linear_velocity.z=-speed;
        advance(&mut s,VehicleInput{clutch:1.,gear_request:Some(0),..Default::default()});
        if n>1000 {
            min=min.min(rides(&s).into_iter().fold(1e9,f64::min));
            stops+=s.state.suspension.wheels.iter().filter(|w|w.bottom_out_force>1.).count() as f64;
            frames+=4.;
        }
    }
    let nr=s.state.suspension.wheels.iter().map(|w|w.total_normal_force).fold(f64::INFINITY,f64::min);
    (s,min,stops/frames,nr)
}
fn main()->Result<(),Box<dyn std::error::Error>> {
    let args:Vec<String>=std::env::args().collect();let out=Path::new(&args[1]);
    let original=load_vehicle_configuration(Path::new("D:/open-formula/rust/vehicle-physics-engine/tests/fixtures/williams_suspension_pre_35_40.json"))?;
    fs::write(out.join("authority_reference.json"),original.to_resolved_configuration_value().to_string())?;
    let mut c=original.clone();
    let kf=args.get(2).map(|x|x.parse()).transpose()?.unwrap_or(200000.);
    let kr=args.get(3).map(|x|x.parse()).transpose()?.unwrap_or(1200000.);
    c.suspension.front_arb_ratio=args.get(4).map(|x|x.parse()).transpose()?.unwrap_or(0.25);c.suspension.rear_arb_ratio=args.get(5).map(|x|x.parse()).transpose()?.unwrap_or(0.06);
    c.suspension.front_bump_stop_multiplier=1.;c.suspension.rear_bump_stop_multiplier=1.;
    {
        let g=c.suspension.geometric_suspension.as_mut().unwrap();
        g.front.spring_rate_N_per_m=kf;g.rear.spring_rate_N_per_m=kr;g.rear.wheel_bump_m=0.040;
        for a in [&mut g.front,&mut g.rear] {
            a.bump_stop_mult=1.;a.bump_stop_travel_m=Some(0.005);a.hard_stop_overtravel_m=Some(0.0001);
        }
    }
    adopt_antiroll_design_bar(&mut c);
    let props=resolve_vehicle_mass_properties_for_suspension_backend(&c,SuspensionBackend::Simplified);
    // Initial preload from the desired +/-5 mm pose. Static production runs below
    // refine it using the coupled body/corner response and the existing floor datum.
    for (w,q) in [(WheelIndex::FrontLeft,-0.005),(WheelIndex::RearLeft,0.005)] {
        let g=c.suspension.geometric_suspension.as_ref().unwrap();let p=g.corners.get(w);
        let r=jacobian(p,q,0.,w.is_front()).unwrap().motion_ratio;
        let s=solve_corner(p,q,0.,w.is_front(),0.,0.,1.).unwrap().damper_compression;
        let front=(g.corners.rl.hub_center.z-props.center_of_mass_local_metres.z)/(g.corners.rl.hub_center.z-g.corners.fl.hub_center.z);
        let normal=props.total_mass_kilograms*9.80665*if w.is_front(){front}else{1.-front}/2.;
        let mass=WheelMechanicalTuning::for_wheel(&c,w).physical_unsprung_mass_kg;
        let target=(normal-mass*9.80665).max(1.);
        let e=1e-5;let k=(spring(&c,w,q+e).0-spring(&c,w,q-e).0)/(2.*e);
        let damp=2.*(mass.max(normal/9.80665-mass)*k.max(1.)).sqrt()/(r*r);
        let g=c.suspension.geometric_suspension.as_mut().unwrap();let a=if w.is_front(){&mut g.front}else{&mut g.rear};
        a.spring_free_length_m=a.spring_installed_length_m+target/(a.spring_rate_N_per_m*r)-s;
        a.damper_bump_Ns_per_m=damp*0.65;a.damper_rebound_Ns_per_m=damp*0.80;
    }
    // Correct damping after the actual preload, rather than the donor preload.
    for w in [WheelIndex::FrontLeft,WheelIndex::RearLeft] {
        let q=if w.is_front(){-0.005}else{0.005};let e=1e-5;
        let k=(spring(&c,w,q+e).0-spring(&c,w,q-e).0)/(2.*e);let r=spring(&c,w,q).1;
        let normal=vehicle_physics_engine::wheel_mechanics::static_wheel_load(&c,w);
        let mu=WheelMechanicalTuning::for_wheel(&c,w).physical_unsprung_mass_kg;
        let critical=2.*((normal/9.80665-mu).max(1.)*k).sqrt()/(r*r);
        let g=c.suspension.geometric_suspension.as_mut().unwrap();let a=if w.is_front(){&mut g.front}else{&mut g.rear};
        a.damper_bump_Ns_per_m=0.65*critical;a.damper_rebound_Ns_per_m=0.80*critical;
    }
    let target=[0.035,0.040];
    for i in 0..8 {
        let s=settle(&c);let actual=rides(&s);eprintln!("CALIBRATE {i} ride={actual:?}");
        if (actual[0]-target[0]).abs().max((actual[1]-target[1]).abs())<0.00005 {break;}
        for a in 0..2 {
            let w=if a==0{WheelIndex::FrontLeft}else{WheelIndex::RearLeft};
            let q=s.state.suspension.wheels[w as usize].suspension_compression_m-vehicle_physics_engine::suspension::rest_compression_m(&c,w);
            let r=spring(&c,w,q).1;
            let g=c.suspension.geometric_suspension.as_mut().unwrap();let el=if a==0{&mut g.front}else{&mut g.rear};
            el.spring_free_length_m+=(target[a]-actual[a])*r*0.7;
        }
    }
    fs::write(out.join("calibrated.json"),c.to_resolved_configuration_value().to_string())?;
    println!("speed_kph,ride_front_m,ride_rear_m,min_floor_m,stop_fraction,min_normal_n,spring_FL,spring_RL,stop_FL,stop_RL");
    for v in [0.,100./3.6,200./3.6,250./3.6,300./3.6] {
        let (s,min,stop,nr)=run(&c,v);let r=rides(&s);
        println!("{:.1},{:.6},{:.6},{:.6},{:.6},{:.3},{:.3},{:.3},{:.3},{:.3}",v*3.6,r[0],r[1],min,stop,nr,s.state.suspension.wheels[0].spring_force,s.state.suspension.wheels[2].spring_force,s.state.suspension.wheels[0].bottom_out_force,s.state.suspension.wheels[2].bottom_out_force);
    }
    Ok(())
}
