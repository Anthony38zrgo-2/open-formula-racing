# Williams chassis and suspension correction — 20 October 2026

Status: corrected candidate installed, with native component and geometry checks. Full-vehicle rollover/driving acceptance remains open. This supersedes the installed values recorded in `williams_chassis_suspension_audit.md`; that audit remains a record of the defects before correction.

## Scope and provenance

Work remains on `codex/initial-port`, HEAD `57ddca9375bc18136f319234f53a930db42a6563`, with prior dirty work preserved. Backup, initial branch/status/patch, the previous portable model/configuration and physics library are in `D:/open-formula-migration-evidence/chassis-correction-20261020/`. No commit, reset, branch switch or push was performed.

Formula90s was inspected at `main-clean`, HEAD `f3efcf246c2a2abf62477037ed0243f2f9a93a9b`, with its existing dirty state inventoried. Its physical geometry was evaluated using an external Cargo package with a fresh target directory under the evidence directory, not by modifying the source project or reusing its ignored binaries.

## Corrections

| Contract | Before | Corrected |
| --- | --- | --- |
| Chassis/driver export vertical offset | 0.538650 m | 0.488650 m |
| Native center-of-gravity reference height | 0.295000 m | 0.245000 m |
| Nominal ground-relative center of gravity | 0.345000 m | **0.295000 m** |
| Nominal lowest chassis vertex above ground | 0.074899 m | **0.024899 m** |
| Front compression / droop | 35 / 60 mm | **25 / 85 mm** |
| Rear compression / droop | 35 / 70 mm | **30 / 90 mm** |
| Front absolute minimum / rest / maximum extension | 15 / 50 / 110 mm | **25 / 50 / 135 mm** |
| Rear absolute minimum / rest / maximum extension | 15 / 50 / 120 mm | **20 / 50 / 140 mm** |
| Wheel reaction includes bar and heave support | Disabled | **Native fixed wheel force enabled for Williams** |

The shared vertical export offset also corrects aerodynamic application positions, so their ground-relative heights stay consistent with the lowered chassis rather than retaining the earlier 50 mm displacement. Native wheel centers, wheelbase 2.890 m and tracks 1.669974/1.600 m are preserved. The generator applies the change to both chassis and static driver assets. No global graphics or simulation code change is needed.

## Spring and damper conversion

The reference program calls the original Rust `solve_corner` and `jacobian` functions for all four expanded corners at travel -20, -10, 0, +10 and +20 mm. At design rest, both sides agree exactly and converge without rocker or steering clamps. Measured motion ratios are:

- Front: 0.8759227555360516.
- Rear: 0.37049714771142384.

The native linear suspension uses a **constant motion-ratio approximation at design rest**. Spring and damper element coefficients are multiplied by the squared measured ratio, and the damper knee velocity is divided by that ratio. Native bellcrank remains one; there is no second multiplication. Native spring preload continues to balance the configured static vehicle weight. The conversion does not import the source's much larger design preload or claim to reproduce its varying Jacobian and geometric preload derivative.

| Wheel coefficient | Before front / rear | Corrected front / rear |
| --- | --- | --- |
| Spring, N/m | 134300 / 142800 | 103040.42 / 19601.89 |
| Slow bump, N·s/m | 6970 / 14000 | 5347.67 / 1921.75 |
| Slow rebound, N·s/m | 11475 / 22000 | 8804.09 / 3019.90 |
| Fast bump, N·s/m | 3485 / 7000 | 2673.83 / 960.88 |
| Fast rebound, N·s/m | 5737.5 / 11000 | 4402.04 / 1509.95 |

These are mechanical-advantage conversions, not arbitrary softening factors. The rear's reduction is large because the previous port treated a damper/spring element as if it moved one-to-one with the wheel.

`tools/williams_suspension_reference.json` records the measured ratios and SHA-256 hashes of the source suspension profile and kinematic/configuration implementation. The generator rejects changed source inputs until that reference is regenerated. The external reference package, full measurements and build log are preserved under `reference/`, `linkage-reference.json` and `reference-build.log` in the evidence directory.

## Anti-roll reaction and numerical checks

Bar stiffness stays 100000/80000 N/m. Static restoring signs were already correct, so stiffness was not increased. Enabling the existing `fixed wheel force` feature includes the bar contribution in the wheel suspension-velocity reaction. It does not change other cars' feature settings or the shared solver implementation.

`validate_williams_chassis` calls the real native axle and suspension configuration/force functions and checks actual travel, center-of-gravity reference and the selected feature. Its reduced two-wheel perturbation bench integrates the explicit local suspension equations at 2 ms, around static support, with bounded small displacements and no road-contact clamps. It is not a substitute for the full tire/body/track solver.

For the antisymmetric axle mode, the discrete spring/damper stability expression is below its boundary of two: approximately 1.4011 front and 0.39994 rear after conversion. The rear's old slow-rebound term alone was 2.75.

In the damped native-force bench, perturbation energy falls from approximately 0.308 J front and 0.186 J rear to less than 1e-9 J after two seconds, without travel-limit hits or non-finite values. In a conservative bar-only bench, including wheel reaction keeps energy bounded within the expected explicit-integration oscillation. Excluding bar reaction grows the model's energy from 0.205 to 1.805 J front and 0.166 to 1.446 J rear over 0.2 seconds. This comparison supports the reaction correction in the reduced model; it does not prove complete-vehicle passivity during tire unloading or collisions.

## Validation and limits

`validate_williams_chassis_export.py` checks all 48216 exported chassis vertices against the intended source world pose, the nominal ground-relative center of gravity, all four hub positions and suspension limits. The corrected source suspension hubs align with the native rolling radii. Native aerodynamic and thermal regressions pass with the new configuration. Existing control, material-coordinate and coast-differential checks remain required for final installation verification.

The portable session is checked through `distribution/OpenFormula.exe` with diagnostics temporarily enabled, then diagnostics are restored to zero. A stationary startup/settling check is evidence only for that condition; full-lap, lane-change/slalom, curbs and rollover acceptance require a separate driving run.

The observed smoke session reached approximately 46 seconds of simulation with finite near-zero idle roll/rate, positive wheel support and suspension travel inside the corrected limits. The log explicitly confirms `SimuV5 feature FIXED WHEEL FORCE on`. It also records a legacy generic driver-setup compatibility message rejecting that setup's `fixed wheel force=no`; the effective loaded Williams feature remains enabled. The shared driver setup was preserved rather than changing other vehicles' defaults. See `runtime-smoke.log` for the actual diagnostics.

Donor-derived inertia and the source tire friction multiplier were audited as uncalibrated contributors, not confirmed conversion defects. They are deliberately not retuned without force/inertia evidence. The constant native suspension representation still does not implement the source's nonlinear linkage, kinematic camber/toe or progressive bump-stop curve. No historical FW19 accuracy or guarantee against physically possible rollover is claimed.

Changed production owner: `tools/prepare_williams_vehicle.py`, its new hashed suspension-reference input, and regenerated Williams assets/configuration. Verification owners: `tools/validate_williams_chassis.cpp`, `tools/validate_williams_chassis_export.py`, and its CMake target. No Formula90s source file, other vehicle profile, shared category or native production solver was edited for this correction.
