# Williams physics investigation — 2026-10-20

Scope: investigate rollover during sudden steering on smooth asphalt, rapid tire cooling, and approximately 260 km/h in fifth gear. This investigation does not change production physics parameters.

## Provenance

Destination D:/open-formula has no root Git repository. Formula90s source HEAD is f3efcf246c2a2abf62477037ed0243f2f9a93a9b. Source checkout has pre-existing changes and was not edited.

## Anti-roll bars and rollover

Current front/rear settings are 100000/80000 N/m. Bundled Speed Dreams MP1 and 92MP1 category files specify 2000 lbs/in for both bars, equivalent to approximately 350254 N/m. simuv5 axle.cpp directly computes bar force as spring stiffness times left/right suspension travel difference; its bar calculation does not apply the authored anti-roll bellcrank as an additional stiffness multiplier.

A defensible next calibration candidate is 350000/280000 N/m, multiplying both current bars by 3.5 while preserving the existing front/rear ratio. This is a simulator reference, not a verified Williams 1997 specification or a demonstrated cure. Increasing stiffness transfers load more abruptly and can increase inside-wheel unloading. The source tire friction multiplier 2.7 was copied into a different tire model; the native 92MP1 category uses 1.5. These are configuration values, not measured effective grip. Direction input reaches full steering lock without speed-dependent controller reduction, although the simulator still rate-limits the steering rack. Rollover root cause remains unconfirmed without roll angle/rate, individual wheel loads, steering angle and lateral acceleration telemetry during the maneuver.

## Aerodynamics and top speed

The source hockenheim/aerodynamics.json explicitly labels drag_coefficient_scope as whole_vehicle_reference. The converter copies Cx=0.83 and frontal area=1.15 m2, then adds native FLAT wings with areas 0.5347/0.7122 m2 and angles 18/16 degrees. It omits the source wing polar tables and underfloor model and sets front/rear body lift to zero. Therefore this is not equivalent to the source aerodynamic package: it adds flat-wing drag to a whole-car drag reference while losing authored downforce behavior.

Native aero.cpp computes body drag coefficient 0.645*Cx*area and flat-wing drag rho*wing_area*sin(angle). Its global rho initializes from SimAirDensity before session atmosphere configuration, nominally 1.29 kg/m3, instead of following the logged session density around 1.219 kg/m3. The resulting coefficient is approximately 1.082 N/(m/s)^2. In straight, level, undamaged, zero-pitch conditions the predicted aerodynamic power alone is about 408 kW at 260 km/h and 626 kW at 300 km/h. This excludes rolling resistance and drivetrain losses and is not runtime telemetry.

Authored fifth gear reaches the limiter kinematically near 317 km/h; sixth near 371 km/h, using rear radius 0.322016 m, final drive 4.65 and limiter 17500 RPM. At 260 km/h fifth predicts approximately 14351 RPM. Thus 260 km/h is not the fifth-gear limiter according to configuration. Aerodynamic loss, road gradient, available power, clutch and brake inputs must be separated before shortening gears. Correct total-car drag accounting and calibrated native wing/floor downforce should precede gear tuning.

## Tire temperature

The converted profile retains Cavallo donor thermal parameters: initial 30 C, optimal 80 C, tire speed cooling factor 1.0, rim mass default 7 kg and generic heating coefficients. simuv5 wheel.cpp multiplies convective heat loss by 1+1.5*speed_cooling_factor, which is 2.5 here. Changing only the small static tire cooling factor 0.000075 will not address the dominant speed-dependent loss.

Analytical estimate at 260 km/h, tire 80 C, air 30 C, fresh tread, no water and no heat generation: front heat capacity approximately 12.37 kJ/K, rear 18.55 kJ/K; convective loss approximately 35.6/43.3 kW. Initial cooling rates are approximately 2.88/2.33 C/s and temperature-excess half-lives approximately 12.0/14.9 seconds. Frictional and hysteresis heating reduce the net cooling during actual driving. These estimates explain why the configuration can cool rapidly; they do not establish a measured cooling curve. A diagnostic cooling-factor sweep from 1.0 to 0.25 would reduce this convection multiplier from 2.5 to 1.375, or 45 percent, but requires warm/cold tire energy-balance validation. It is a calibration candidate, not a validated replacement.

## Sources

- Local primary source: speed-dreams-data/data/cars/categories/MP1.xml and 92MP1.xml.
- Local implementation: src/modules/simu/simuv5/axle.cpp, steer.cpp, car.cpp, wheel.cpp and aero.cpp; tools/prepare_williams_vehicle.py.
- Original authored package: D:/Formula90s/game/data/vehicles/f1_1997/williams/profiles/hockenheim/aerodynamics.json and tires.json.
- iRacing anti-roll construction and tuning: https://www.iracing.com/commodores-garage-11-swayanti-roll-bars/
- iRacing Skip Barber anti-roll skidpad comparison: https://www.iracing.com/skidpadman-effect-of-the-rear-anti-roll-bar/
- Studio 397 tire heat transfer definitions: https://www.studio-397.com/wp-content/uploads/2016/12/rF2_TGM_TyreTool_QuickStart_V3.pdf

## Implementation order

1. Capture sudden-steering wheel loads and roll telemetry and compare existing bars with 350000/280000 N/m.
2. Correct total-car drag accounting, preserve a tested downforce balance and measure straight-line acceleration in fifth and sixth.
3. Calibrate tire warming and cooling using complete energy balance; retain measured cooling/heating curves.
4. Validate a full lap and repeated steering transients before accepting any new setup as stable.
