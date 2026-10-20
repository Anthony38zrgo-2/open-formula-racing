# Williams geometry and suspension verification — 20 October 2026

Result: horizontal dimensions agree with the authored source profile, but the port cannot be accepted as a correct suspension conversion. A nominal vertical reference error, incorrect travel mapping and unresolved wheel-rate/integration contracts remain. This is verification only: no production code, vehicle configuration, binaries or media were changed in this audit.

## Evidence and provenance

Branch `codex/initial-port`, HEAD `57ddca9375bc18136f319234f53a930db42a6563`. Existing dirty work was inventoried and preserved. Audit source, executable, initial status, patch snapshot and machine-readable results are in `D:/open-formula-migration-evidence/chassis-audit-20261020/`. Installed vehicle XML SHA-256: `a9d191b6215ee98f0e60ab8f7fdc2055faf79b801a5ec86058fecc4990c00acb`.

The audit measured transformed runtime GLB vertices, read the installed and generated XML, inspected the native loader and physics equations, and compiled an external probe directly against the current `axle.cpp` and `susp.cpp`. The probe calls native configuration/reconfiguration and force-update functions with the installed XML. It does not launch the game or simulate a complete driving maneuver. The latest driving log has no `Formula dynamics` samples, so it cannot establish the rollover event's cause.

## Horizontal geometry: consistent with the source

| Quantity | Authored source | Installed native geometry |
| --- | --- | --- |
| Wheelbase | 2.890000 m | 2.890000 m |
| Front track, wheel centers | 1.669974 m | 1.669974 m |
| Rear track, wheel centers | 1.600000 m | 1.600000 m |
| Front tire diameter / width | 0.623899 / 0.307541 m, measured GLB | Same reconstructed native dimensions within export precision |
| Rear tire diameter / width | 0.644032 / 0.389105 m, measured GLB | Same reconstructed native dimensions within export precision |
| Chassis bounding dimensions, length/width/height | 4.444269 / 1.661375 / 1.154363 m, measured GLB | 4.444270 / 1.661375 / 1.154363 m in XML |

Native axle positions are +1.146992 m front and -1.743008 m rear. Lateral wheel coordinates are ±0.834987 m front and ±0.800000 m rear. Outside tire widths are approximately 1.977515 m front and 1.989105 m rear, consistent with the configured 2.0 m overall collision envelope. The renderer scales normalized custom wheels by diameter/width/diameter, undoing the exporter normalization. No factor-of-two wheel sizing error was found.

The separate wheel GLBs are centered locally; they do not independently encode vehicle wheelbase. The rendered wheel centers come from the physics wheel positions. Agreement with the authored profile is therefore established, not independently verified historical FW19 dimensions. Native right wheels have negative lateral coordinates, matching the bundled donor convention. Its internal signed `wheeltrack` value is negative by construction; that alone is not a handedness defect.

## Confirmed nominal vertical mismatch: 50 mm

The generator computes a source ground offset of 0.538650 m. With the authored center-of-gravity offset -0.243650 m, the intended source ground-relative center of gravity is 0.295000 m.

Native `GC height=0.295` is relative to the chassis reference plane. The installed suspension reference/ride height is 0.050 m. From `SimCarUpdateWheelPos`, on level ground at the nominal pose:

`center_of_gravity_above_ground = native_reference_clearance + GC_height = 0.050 + 0.295 = 0.345 m`.

The chassis renderer translates its body reference by `car_position_z - GC_height`, while native wheel visual positions subtract suspension extension. The exported chassis and its suspension geometry therefore stand 50 mm above their intended source pose while the tire centers remain at their rolling radii. The source lowest chassis vertex is nominally 24.899 mm above the ground; the port's corresponding nominal value is 74.899 mm. The lateral-force roll arm in `SimCarUpdateForces` also contains `GC height + rideHeight`.

These are configuration/equation-derived flat-ground nominal values, not measured driving heights under fuel, aero and suspension motion. They establish an incorrect vertical reference conversion and a larger nominal roll lever arm; they do not alone prove the full rollover cause.

## Native suspension configuration and travel

| Per-wheel parameter | Front | Rear |
| --- | --- | --- |
| Spring | 134300 N/m | 142800 N/m |
| Slow bump damping | 6970 N·s/m | 14000 N·s/m |
| Slow rebound damping | 11475 N·s/m | 22000 N·s/m |
| Fast bump damping | 3485 N·s/m | 7000 N·s/m |
| Fast rebound damping | 5737.5 N·s/m | 11000 N·s/m |
| Damping knee | 0.127 m/s | 0.127 m/s |
| Bellcrank ratio | 1.0 | 1.0 |
| Native rest/minimum/maximum extension | 50 / 15 / 110 mm | 50 / 15 / 120 mm |
| Actual bump / droop from nominal rest | **35 / 60 mm** | **35 / 70 mm** |
| Source intended wheel bump / droop | **25 / 85 mm** | **30 / 90 mm** |

The native configuration probe confirms these values after unit conversion. The generator copied total source bump-plus-droop into native `suspension course`, but simuv5 treats that parameter as the absolute maximum extension, not a range measured from rest. It additionally imposed an unrelated 15 mm packer. Consequently neither the intended compression/extension limits nor the intended total usable travel is preserved.

The source solver also applies suspension motion ratio to spring displacement/force and to damper velocity/force. The converter directly copies spring/damper element coefficients and sets native bellcrank to one. The source wheel-rate equivalence has not been established; reconstructed pushrod/pullrod hardpoints themselves are not implemented in the native suspension. In particular, large rear damper values cannot be justified as equivalent wheel damping merely because their numbers match the source JSON.

At the standard 2 ms simulation step and 16 kg rear wheel mass, the slow-rebound damping term has `coefficient × timestep / mass = 2.75`. The isolated explicit velocity-update gain is `1 - 2.75 = -1.75`; its magnitude exceeds one. The front counterpart is approximately 1.765 for the ratio and -0.765 for the isolated gain. This is a concrete local numerical-stability concern in the rear branch. The real solver also has springs, ground clamps, bump/fast-damper transitions and body coupling; this local calculation is not a complete stability proof or a demonstrated driving reproduction.

## Anti-roll bars: present and restoring, not missing

Front/rear stiffness is 100000/80000 N/m after native configuration. No additional anti-roll bellcrank multiplier applies in `SimAxleUpdate`.

For a positive 0.001 rad flat-ground roll perturbation, the native probe produces:

| Component roll moment | Front | Rear |
| --- | --- | --- |
| Springs | -187.269 N·m | -182.784 N·m |
| Anti-roll bar | -278.881 N·m | -204.800 N·m |

Negative roll produces the opposite moments. Both spring and bar moments oppose the imposed roll in all tested cases. No reversed static restoring sign was found.

At an imposed +0.005 rad roll, the front raw left support force becomes -75.6 N before the wheel model clamps it to zero. This is the onset of inner-wheel unloading in this prescribed pose, not a measured in-game lateral-acceleration threshold. Blindly multiplying bar stiffness is not justified by these results.

The vehicle does not enable `Features/fixed wheel force`; its heave springs are zero and do not activate that feature automatically. In the current branch, bar force contributes to chassis support, but the wheel's explicit suspension-velocity reaction subtracts only spring/damper force. Native code provides another branch that includes total wheel force. The correctness/passivity of the present bar-to-wheel reaction contract must be checked dynamically before accepting strong bars. Static restoring signs do not establish energy conservation in the complete update.

## Other relevant limits

Roll inertia is still inherited from the donor's `mass repartition coefficient=0.8` and the native uniform-box estimate: approximately 132.058 kg·m². No explicit Williams inertia tensor is installed. Road friction multiplier 2.7 was transferred to a different tire law; equivalent lateral grip is uncalibrated. Both remain contributors to investigate, not proven causes.

No Williams-specific saved suspension override was found in the user settings. Other cars' saved setups were not changed. No runtime or visual acceptance is inferred from this audit.

## Concrete correction order for a subsequent implementation

1. Correct the reference conversion jointly for chassis/driver export, native center of gravity and aerodynamic application heights. With the existing 50 mm native rest clearance, subtracting it from the shared export offset yields 0.488650 m and a native `GC height` of 0.245000 m, restoring the nominal ground-relative center of gravity to 0.295000 m. Recheck physical/visual wheel alignment and ground clearance together.
2. Map the actual travel limits. At the current 50 mm rest extension, preserving source bump/droop requires front minimum/maximum extension 25/135 mm and rear 20/140 mm. These are a coordinate conversion, not a proposed suspension stiffness tune.
3. Establish effective wheel spring/damper rates from the source linkage, then check discrete stability at 2 ms. Do not guess a lower damping coefficient or increase bar stiffness to mask the contract error.
4. Test equal-and-opposite axle reactions and energy/passivity with and without the native fixed-wheel-force feature. Preserve Williams-only scope for any resulting configuration or solver change.
5. Recheck effective inertia and lateral tire forces; then reproduce lane changes/slalom on a controlled plane and Fuji with roll/rate, wheel support, ground normals and suspension travel telemetry. Finish with curbs and a complete lap before accepting the rollover problem as fixed.

Production changes are intentionally not applied by this verification. Existing controls, aero, thermal calibration and differential settings remain installed exactly as before.
