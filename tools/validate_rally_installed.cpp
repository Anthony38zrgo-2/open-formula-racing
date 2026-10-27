#include <windows.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include "rust_library.h"
int main(){
 HMODULE dll=LoadLibraryW(L"D:/open-formula/distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll");if(!dll)return 1;
 #define FN(name,type,symbol) auto name=reinterpret_cast<RustPhysicsLibrary::type>(GetProcAddress(dll,symbol));if(!name)return 2
 FN(load,LoadTrackVisualWorldFn,"ofp_load_track_visual_world");FN(create,CreateFn,"ofp_create");FN(reset,ResetFn,"ofp_reset");FN(step,StepFn,"ofp_step");FN(snapshot,SnapshotFn,"ofp_snapshot");FN(error,LastErrorFn,"ofp_last_error");FN(geometry,VehicleGeometryFn,"ofp_vehicle_geometry");FN(destroy,DestroyFn,"ofp_destroy");FN(specs,VehicleSpecsFn,"ofp_vehicle_specs");
 std::cout<<"Loading installed collision world"<<std::endl;
 if(load("D:/open-formula/distribution/share/games/speed-dreams-2/tracks/circuit/russia-stage1/russia-stage1.acc","Russia Stage 1",4)<=0)return 3;
 auto h=create("D:/open-formula/distribution/share/games/speed-dreams-2/cars/rust/ford-escort-rs2000/vehicle.json",{-110.653647,7.,-5.},0.,0.);if(!h)return 4;
 OfpVec3 cg{},inertia{};double spawn=0,mass=0;geometry(h,&cg,&inertia,&spawn,&mass);OfpVehicleSpecs spec{};specs(h,&spec);
 std::cout<<"ACTIVE mass="<<mass<<" spawn="<<spawn<<" cg="<<cg.y<<" gears="<<spec.forward_gears<<" final="<<spec.final_drive<<" torque="<<spec.max_torque_nm<<std::endl;
 reset(h,{-110.653647,6.065+spawn+0.01,-5.},0.);
 std::ofstream out("D:/open-formula/staging_rally/integration/installed_physics.csv");out<<"step,time,x,y,z,speed,rpm,gear,grounded,load,rejected,body_contacts,floor_impulse,front_travel,rear_travel\n";
 for(int i=0;i<600;i++){
  OfpControlInput input{};if(i<250){input.brake=1.;input.handbrake=1.;}else if(i<450){input.throttle=.45;input.gear_request=1;}else{input.brake=1.;input.gear_request=1;}
  if(step(h,&input,nullptr,0,.02)!=0){char msg[8192]{};error(h,msg,8192);std::cerr<<"STEP_FAILED "<<i<<" "<<msg<<std::endl;destroy(h);return 5;}
  OfpSnapshot s{};snapshot(h,&s);int grounded=0;double load=0;for(auto w:s.wheels){grounded+=w.grounded!=0;load+=w.normal_force_n;}
  double speed=std::sqrt(s.linear_velocity.x*s.linear_velocity.x+s.linear_velocity.y*s.linear_velocity.y+s.linear_velocity.z*s.linear_velocity.z);
  if(!std::isfinite(speed)||s.rejected_steps){std::cerr<<"INVALID_STATE "<<i<<std::endl;return 6;}
  out<<i<<','<<s.sim_time_s<<','<<s.position.x<<','<<s.position.y<<','<<s.position.z<<','<<speed<<','<<s.engine_rpm<<','<<s.gear<<','<<grounded<<','<<load<<','<<s.rejected_steps<<','<<s.body_contact_count<<','<<(s.floor_impulse_ns[0]+s.floor_impulse_ns[1])<<','<<s.wheels[0].suspension_travel_m<<','<<s.wheels[2].suspension_travel_m<<'\n';
  if(i==249||i==449||i==599)std::cout<<"PHASE "<<i<<" speed="<<speed<<" grounded="<<grounded<<" load="<<load<<std::endl;
 }
 destroy(h);std::cout<<"INSTALLED_PHYSICS_PASS"<<std::endl;return 0;
}


