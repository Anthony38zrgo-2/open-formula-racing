#include <windows.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include "rust_library.h"
int main(int argc,char**argv){
 if(argc<4)return 1;
 auto dll=LoadLibraryW(L"D:/open-formula/distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll");if(!dll)return 2;
 #define FN(name,type,symbol) auto name=reinterpret_cast<RustPhysicsLibrary::type>(GetProcAddress(dll,symbol));if(!name)return 3
 FN(load,LoadTrackVisualWorldFn,"ofp_load_track_visual_world");FN(create,CreateFn,"ofp_create");FN(reset,ResetFn,"ofp_reset");FN(step,StepFn,"ofp_step");FN(snapshot,SnapshotFn,"ofp_snapshot");FN(error,LastErrorFn,"ofp_last_error");FN(geometry,VehicleGeometryFn,"ofp_vehicle_geometry");FN(destroy,DestroyFn,"ofp_destroy");
 if(load(argv[2],"Traction diagnostic",1)<=0)return 4;
 double x=argc>6?std::stod(argv[4]):0,ground=argc>6?std::stod(argv[5]):0,z=argc>6?std::stod(argv[6]):0;
 auto h=create(argv[1],{x,ground+1,z},0,0);if(!h)return 5;
 OfpVec3 cg{},inertia{};double spawn=0,mass=0;geometry(h,&cg,&inertia,&spawn,&mass);reset(h,{x,ground+spawn+.01,z},0);
 const bool short_road_test=argc>7;
 std::ofstream out(argv[3]);out<<"step,time,wheel,speed,rpm,gear,throttle,load,fx,slip,spin,travel,min_q,max_q,q,motion_ratio,spring,damper,stop,bump_active,droop_active,camber,toe,body_contacts,x,y,z\n";
 for(int i=0;i<(short_road_test?850:1000);i++){
  OfpControlInput in{};if(i<250){in.brake=1;in.handbrake=1;}else if(i<600){in.throttle=.35;in.gear_request=1;}else if(i<850&&!short_road_test){in.throttle=.60;in.gear_request=2;}else{in.brake=1;in.gear_request=short_road_test?1:2;}
  if(step(h,&in,nullptr,0,.02)!=0){char msg[8192]{};error(h,msg,8192);std::cerr<<"FAIL "<<i<<" "<<msg<<std::endl;return 6;}
  OfpSnapshot s{};snapshot(h,&s);double speed=std::sqrt(s.linear_velocity.x*s.linear_velocity.x+s.linear_velocity.y*s.linear_velocity.y+s.linear_velocity.z*s.linear_velocity.z);
  if(!std::isfinite(speed)||s.rejected_steps)return 7;
  for(int k=0;k<4;k++){auto w=s.wheels[k];auto d=s.wheel_diagnostics[k];out<<i<<','<<s.sim_time_s<<','<<k<<','<<speed<<','<<s.engine_rpm<<','<<s.gear<<','<<s.throttle<<','<<w.normal_force_n<<','<<w.longitudinal_force_n<<','<<w.slip_ratio<<','<<w.spin_rad_s<<','<<d.travel_m<<','<<d.min_compression_m<<','<<d.max_compression_m<<','<<d.compression_m<<','<<d.motion_ratio<<','<<d.spring_force_n<<','<<d.damper_force_n<<','<<d.stop_force_n<<','<<d.bump_stop_active<<','<<d.droop_stop_active<<','<<w.dynamic_camber_rad<<','<<w.toe_rad<<','<<s.body_contact_count<<','<<s.position.x<<','<<s.position.y<<','<<s.position.z<<'\n';}
  if(i==249||i==599||i==849||i==999)std::cout<<"PHASE "<<i<<" speed="<<speed<<" rpm="<<s.engine_rpm<<" rear_load="<<(s.wheels[2].normal_force_n+s.wheels[3].normal_force_n)<<" rear_slip="<<s.wheels[2].slip_ratio<<" rear_travel="<<s.wheel_diagnostics[2].travel_m<<" rear_ratio="<<s.wheel_diagnostics[2].motion_ratio<<std::endl;
 }
 destroy(h);std::cout<<"DIAGNOSTIC_COMPLETE mass="<<mass<<" spawn="<<spawn<<std::endl;
}
