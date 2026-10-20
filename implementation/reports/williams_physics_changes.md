# Williams physics implementation — 20 October 2026

Status: implemented candidate, awaiting driving validation and human acceptance. The four reported driving problems are not declared resolved.

## Provenance and backup

Work started on `codex/initial-port`, HEAD `57ddca9375bc18136f319234f53a930db42a6563`, with a clean working tree. No branch switch, reset, commit or push was performed. Tracked source backup: `D:/open-formula-migration-evidence/physics-20261020-094427/source-before.zip`, with branch, HEAD and initial status alongside it. Installed XML, native libraries and previous runtime log were copied to `D:/open-formula-migration-evidence/physics-distribution-before/` before installation. Formula90s was read as the source of authored profiles and media; it was not changed.

## Implementation and ownership

| Files | Behavior |
| --- | --- |
| `src/libs/robottools/formula_steering_filter.h`, `formula_gear_controls.cpp` | Controller steering now has bounded application, reversal and centering rates. A reversal integrates through zero using the remaining time in the same step. Keyboard steering retains immediate response; triggers and shift buttons retain their mappings. Disconnect returns the command to center. |
| `tools/prepare_williams_vehicle.py` | Updated Williams settings: application 1.5, reversal 2 and centering 3 normalized command units/second, exponential response time 0.16 seconds and speed sensitivity 0.018 per meter/second. Diagnostics default to off. Explicit command-line parameters allow separate bar and convection candidates. |
| `src/modules/simu/simuv5/aero.cpp`, `aero.h` | Opt-in total-vehicle drag contract, tabulated wing polars, source wing locations, session air density, and an approximate underfloor height/rake/roll envelope. Nominal wing and floor reference drag is subtracted from the body reference before actual element forces are added. Outside the polar, lift fades toward stall over 35 degrees and drag approaches 1.2, following the source model defaults. The authored soft/hard load ratios and absolute downforce limit bound the resulting element loads. |
| `src/modules/simu/simuv5/car.cpp`, `carstruct.h` | Separate floor force/moment at its authored location. Optional 20 Hz diagnostics for body dynamics, rack/wheel steering, effective pedals, clutch transfer, gear ratio, engine output, aerodynamic forces and individual wheel state. |
| `src/modules/simu/simuv5/wheel.cpp`, `wheel.h` | Williams opt-in thermal camber conversion uses degrees = radians × 180/π. Corrected thermal normal load cannot be negative and is zero without wheel normal force. Additional cooling exchanges heat toward ambient instead of cooling a below-ambient tire further. Diagnostics retain hysteresis, lateral/longitudinal heating, convection, additional cooling and heat capacity. Other vehicles retain the existing thermal branch. |
| `tools/validate_formula_pedal_controls.cpp` | Virtual SDL regression, controller reversal, center noise, keyboard fallback, disconnect and five integration steps from 1 to 50 milliseconds. |
| `tools/validate_formula_aerodynamics.cpp` | Calls native configuration/update functions with the generated XML. Checks nominal drag/downforce at three densities and five speeds, floor height envelope, wing stall and authored force limit. |
| `tools/validate_formula_tire_energy.cpp`, `CMakeLists.txt` | Native thermal balance checks across three starting temperatures and three loads, including below-ambient and unloaded cases. Accounts for the simulator's single-precision temperature rounding. |

No engine power, tire friction, suspension spring or damper calibration was changed. Bars remain 100000/80000 N/m. Subsequent controls and thermal follow-ups below supersede the initial reverse ratio and thermal settings: installed convection factor is now 0.25 and hysteresis factor 1.25. The 350000/280000 N/m bar settings remain unaccepted comparison candidates.

## Validation evidence

Build, install and all three native validation executables pass with the existing UCRT64 Release toolchain. Logs are in `D:/open-formula-migration-evidence/physics-build.log`, `physics-install.log`, `physics-controls-validation.log`, `physics-aerodynamics-validation.log` and `physics-tire-validation.log`.

The portable assembly reports 4614 files, 97 inspected binaries and no unresolved dependencies. The actual runtime was started through `distribution/OpenFormula.exe`, with diagnostics temporarily enabled. `physics-runtime-smoke.log` records the Williams/Fuji session, density 1.219 kg/m³, connected controller discovery and finite idle dynamics/wheel data. That session was closed; the distributed XML restores diagnostics to zero. The final provenance manifest records binary/configuration hashes separately from the earlier smoke log.

This is a startup and stationary telemetry check. It does not validate physical Xbox response, full-lap driving, rollover resistance, collision behavior, temperature curves or maximum speed. The final wing stall/force-limit changes are covered by native tests; a further driving session is still required.

## Model limits and acceptance still required

The underfloor uses the previous physics step's native wheel ground distances, offset from the configured 0.05 m native ride-height reference to the source front/rear floor clearances. It restores a bounded, height-sensitive floor contribution and its moment; it does not reproduce the source probe-confidence system, local airflow at each element, flow attachment memory, pressure migration or map-based ground sampling. No full aerodynamic equivalence or historical Williams calibration is claimed.

Thermal energy records are instantaneous per-step joules sampled at 20 Hz, not integrated totals between records. Temperatures are kelvin, pressures pascals, steering/roll/pitch radians, angular speeds radians/second, speeds meters/second and forces newtons. `mesh_contact` means a mesh sample was found; wheel `state` and normal load must be checked separately to establish physical support. Engine output power is not wheel power: use wheel drive torque, spin speed, clutch transfer, brakes and resistance when evaluating the drivetrain balance.

Required next evidence:

1. Flat-surface and Fuji steering transients with raw stick, shaped target, filtered command, actual wheel angles and loads. Repeat lane changes/slalom at increasing speeds, including curbs afterwards. A virtual controller is not a physical-controller acceptance test.
2. Independent bar comparisons at equal tire state, speed and inputs; prove restoring moments and absence of spurious energy in the actual vehicle before choosing 350000/280000 N/m or another candidate.
3. Hot straight, cold warm-up, steady corner and braking thermal curves; compare convection factors 1.0 and 0.25 with complete heat balance before changing the installed default. No historical cooling target has been invented.
4. Controlled fifth/sixth acceleration and steady-speed measurements with throttle, brakes, clutch, engine/wheel power, slope, damage and equivalent weather. No new maximum speed is claimed.
5. Full lap with the physical Xbox controller and human acceptance. Keep `human_approved` and driving validation flags false until that evidence exists.

Enable the numeric `Formula Controls/diagnostic telemetry` setting for a measurement session, then restore it to zero. Persistent changes belong in the generator as well as regenerated XML. Generator options `--front-anti-roll-stiffness`, `--rear-anti-roll-stiffness` and `--tire-speed-cooling-factor` support controlled candidates without changing shared categories.

## Controls follow-up — reverse and stronger smoothing

The user reported that reverse could not be selected and steering was still too direct. Inspection confirmed two independent reverse defects: the input adapter clamped the minimum command to zero, and the converter exported positive reverse ratio 3 although native simuv5 uses signed ratios. The adapter now uses the native reverse availability offset for its lower bound; the converter exports -3 for this Williams. The forward ratios are unchanged. Sequential controller X and keyboard Z can now traverse 1 → N → R; controller A and keyboard A traverse R → N → 1. Holding a controller button produces one change per press.

The existing speed reduction was present in code but disabled by the generated zero coefficient. It is now enabled with target scaling `1 / (1 + 0.018 × abs(speed_meters_per_second))`: full stationary travel, 66.7 percent at 100 km/h, 50 percent at 200 km/h and 40 percent at 300 km/h. This reduces controller steering travel, not engine power or vehicle speed. Keyboard steering bypasses that scaling.

Controller smoothing combines the lower rate limits above with an exact piecewise integration of a bounded exponential response. Small stick movements therefore no longer jump directly to their target merely because they fit within one rate-limit step. Reversals integrate continuously through zero with their own rate; the remaining time is applied in the new direction. Full stationary stick application takes about 0.76 seconds to reach 95 percent, versus about 0.33 seconds to reach full command under the previous application limit. The physical rack still applies its existing downstream rate limit.

`controls-followup-validation.log` verifies reverse selection with virtual SDL buttons and keyboard, held-button behavior, forward bounds from the native gearbox contract, 25 exponential response cases across five integration steps, the generated negative reverse ratio, installed-profile speed scaling at four speeds, and immediate keyboard steering. The helper's original zero-response-time behavior is also retained and tested. These automated checks do not replace driving acceptance with the physical controller.

Follow-up backup: `D:/open-formula-migration-evidence/controls-followup-20261020-101046/`; build and validation logs use the `controls-followup-` prefix in that evidence directory. Earlier physics provenance describes the earlier candidate and is superseded for the controls library and vehicle XML by `controls-followup-provenance.json`. Other pending physics acceptance gates remain open.

## Materials, rear wing and braking differential follow-up

The chassis GLB's `fw19_cf.tGA:fw19_cf.tga` material contains a charcoal-preview constant factor and no base color texture. The adjacent `fw19_cf.png` is effectively black; the original Williams `car/source/textures/Carbon_Fiber_diffuse.jpg` contains the weave. The Williams generator now explicitly binds that original weave to the eight chassis carbon primitive groups, with a dark 0.25 material factor. Source GLBs remain unchanged. Other untextured chassis colors, including the steering column and display, receive equivalent constant-color texture swatches so their intended colors are explicit in the portable asset. Ambient color follows the material factor for this export.

Four steering-wheel carbon primitive groups reference `TEXCOORD_1`; the old exporter always chose `TEXCOORD_0`. The exporter now reads the material's declared base-color coordinate channel and rejects a missing required channel. `validate_vehicle_material_export.py` independently compares all exported triangle texture references against the corresponding raw source accessor. It verified 54 primitive groups, four secondary-coordinate groups, eight restored carbon groups, and all referenced texture files. This establishes bindings and coordinate preservation, not a visually accepted in-game render or full glTF PBR equivalence.

Rear wing incidence increases from 16 to 17 degrees. The existing polar gives rear lift coefficient 1.03 instead of 1.00 and drag coefficient 0.176 instead of 0.160. The reference drag subtraction deliberately stays at the original 16-degree setup, so the increased wing drag is retained. Native force tests verify the added drag and downforce, including density scaling and the existing total-force limit. No geometry or front-wing adjustment was made.

The previous native `LIMITED SLIP` setup acted as an open differential under negative drive torque. The Williams now selects `1.5 WAY LSD`, retaining acceleration slip bias 0.30 and locking input torque 90 N·m, with coast slip bias 0.10. A native test exposed a sign defect: uncorrected positive coast bias increased wheel-speed separation under negative torque. An explicit `Rear Differential/restoring coast lock=yes` option reverses that bias sign only for this Williams; its default remains off for other cars. No shared category or global differential behavior was changed.

`validate_formula_differential` uses the real configuration and differential update with independently initialized wheel axes and no engine coupling. At -200 N·m, the wheel-speed separation changes from 10.00000 to 9.96204 radians/second in one 2 ms step, rather than the uncorrected increase to 10.03796. At +200 N·m, both old and revised setups give 9.88612 radians/second: the traction branch is preserved in this controlled case. These values demonstrate the direction of coast coupling; they are not a percentage locking calibration or a driving stability claim.

Evidence and backup use `D:/open-formula-migration-evidence/materials-setup-20261020-102234/` and `materials-setup-*.log/json`. The user closed the existing session before installation. Full visual confirmation and the subjective braking/turn-in balance remain pending a new driving session.

## Thermal retention and doubled coast coupling follow-up

The user reported excessive cooling and difficult warm-up after driving, and explicitly requested twice the current coast coupling. The installed profile still used convection factor 1.0 and hysteresis factor 1.0. Only Williams-generated parameters change in this follow-up; no production solver or shared category changes are introduced.

The new default speed cooling factor is 0.25. The native convection multiplier becomes 1.375 instead of 2.5, reducing this loss term by 45 percent at equal wheel speed, temperature, surface and weather. Hysteresis increases from 1.0 to 1.25, adding 25 percent to deformation/slip hysteresis energy at equivalent state. Lateral and longitudinal heating coefficients, initial 30 °C, optimal 80 °C, thermal capacity, wear, friction and static cooling remain unchanged. No temperature clamp, warmer or artificial fixed temperature is introduced. The persistent generator also exposes `--tire-hysteresis-factor`; both thermal defaults are applied to all four Williams tires.

`validate_formula_tire_energy <vehicle.xml>` now loads the real tire geometry and thermal parameters through native wheel configuration, then integrates prescribed constant wheel speed, forces and slip at 2 ms. It compares original parameters, reduced cooling alone, and reduced cooling plus increased hysteresis separately. These are isolated thermal bench scenarios with fixed inputs, not vehicle acceleration/braking or complete driving maneuvers. Air is 30 °C; tire starting pressure is consistent with its starting temperature. Tire wear and pressure continue to evolve.

| Prescribed scenario | Duration | Previous front/rear final °C | Cooling-only front/rear final °C | Revised front/rear final °C |
| --- | --- | --- | --- | --- |
| Start at 80 °C, 260 km/h wheel speed, 1800 N normal load, no slip or horizontal force | 30 s | 44.93 / 46.44 | 56.82 / 58.16 | 58.67 / 59.44 |
| Start at 30 °C, 45 m/s wheel speed, 2500 N normal/lateral load, slip 0.05 | 60 s | 52.68 / 47.34 | 62.20 / 53.59 | 69.02 / 58.71 |
| Start at 30 °C, 35 m/s wheel speed, 2500 N normal/longitudinal load, slip 0.10 | 60 s | 61.52 / 53.53 | 72.51 / 60.45 | 81.78 / 67.23 |

The initial-step checks also verify the 0.55 convection-power ratio and 1.25 hysteresis-power ratio. All nine existing energy-balance tests and six comparative thermal scenarios pass with finite temperatures. Front and rear are intentionally reported separately; these results do not assert historical accuracy or that both axles must run at identical temperatures.

Rear coast slip bias increases exactly from 0.10 to 0.20; power slip bias remains 0.30 and locking input torque 90 N·m. The restoring coast sign correction stays enabled. In the same native differential bench, wheel-speed separation after one step is 9.92408 rad/s with 0.20 versus 9.96204 with 0.10 and 10.00000 without coast coupling. The reduction from the open result doubles within numerical tolerance. The positive-torque comparison remains 9.88612 for both configurations. Slip bias is a model parameter, not a measured percentage of physical locking.

Backup: `D:/open-formula-migration-evidence/thermal-coast-20261020-103308/`. Generation, build, native validation and provenance artifacts use the `thermal-coast-` prefix in that evidence directory. No game process was active at inspection. Full-lap thermal behavior and the subjective coast-lock balance remain pending driving acceptance.
