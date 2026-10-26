use super::vertical_suspension_integration::{
    integrate_vertical_suspension, GroundContactConstraint, VerticalSuspensionStep,
};
use super::SimplifiedSuspension;
use super::*;
use crate::vertical_diagnostics::{
    AxleVerticalDiagnostics, FailureCode, FailureDiagnostics, WheelVerticalDiagnostics,
};

#[derive(Clone, Copy)]
pub(super) struct GeometricAxleCornerIntegration {
    pub initial_spring_compression_metres: f64,
    pub initial_motion_ratio: f64,
    pub initial_geometry_invalid: bool,
    pub compression_metres: f64,
    pub velocity_metres_per_second: f64,
    /// Accepted ground reaction impulse over the whole axle step (N s).
    pub normal_impulse_newton_seconds: f64,
    /// Mean normal load over the last accepted substep (N).
    pub final_normal_force_newtons: f64,
    pub suspension_impulse_newton_seconds: f64,
    pub travel_limit_impulse_newton_seconds: f64,
    pub damper_dissipated_joules: f64,
    pub clamped: bool,
    pub antiroll_force_newtons: f64,
    pub force_application_compression_metres: f64,
    pub motion_ratio_rate_per_m: f64,
}

/// Diagnostics of one coupled axle substep (trace only, no force impact).
#[derive(Clone, Copy, Debug, Default)]
pub(super) struct AxleSolveDiagnostics {
    /// Fixed-point iterations consumed (1..=32).
    pub iterations: u32,
    /// True when both projected residuals closed within tolerance.
    pub converged: bool,
    /// Worst projected residual of the accepted velocities (m/s).
    pub residual_m_s: f64,
    /// Projected residual per corner (m/s), same order as the caller's pair.
    pub corner_residual_m_s: [f64; 2],
}

fn ground_gap(compression: f64, limit: f64) -> f64 {
    compression - limit
}

/// One corner of one axle substep, expressed in the units the host log reports.
#[allow(clippy::too_many_arguments)]
fn corner_diagnostics(
    corner: &PreparedAxleCorner,
    compression_metres: f64,
    velocity_metres_per_second: f64,
    contact: GroundContactConstraint,
    substep_seconds: f64,
    normal_impulse: f64,
    travel_impulse: f64,
    anchor_position: Vec3,
    anchor_velocity: Vec3,
    anchor_acceleration: Vec3,
    body_up: Vec3,
) -> WheelVerticalDiagnostics {
    let (min_compression, max_compression) = corner.compression_bounds_metres();
    let projection = corner.contact.normal_projection.clamp(1e-3, 1.0);
    // A corner with no transverse ray hit has no ground plane at all: the demanded
    // compression is undefined, not zero, and must not be read as "touching".
    let (demand_now, demand_predicted, separation) = if contact.active {
        (
            contact.limit_compression_metres,
            contact.limit_at(substep_seconds),
            // Gap at the end of the interval: how much compression the wheel is
            // still missing before the plane, using the same implicit endpoint the
            // solver used.
            ground_gap(compression_metres, contact.limit_at(substep_seconds)),
        )
    } else {
        (f64::NAN, f64::NAN, f64::NAN)
    };
    let bump_impulse = travel_impulse.max(0.0);
    let droop_impulse = travel_impulse.min(0.0);
    let bump_stop_active = bump_impulse > 1e-9;
    let droop_stop_active = droop_impulse < -1e-9;
    WheelVerticalDiagnostics {
        ground_detected: contact.active,
        ground_binding: normal_impulse > 0.0,
        droop_stop_active,
        bump_stop_active,
        trapped_at_stop: (bump_stop_active && velocity_metres_per_second > 0.0)
            || (droop_stop_active && velocity_metres_per_second < 0.0),
        compression_m: compression_metres,
        ground_separation_m: separation,
        ground_separation_normal_m: separation * projection,
        ray_shortfall_m: 0.0,
        ground_demand_m: demand_now,
        ground_demand_predicted_m: demand_predicted,
        max_compression_m: max_compression,
        min_compression_m: min_compression,
        headroom_to_bump_m: max_compression - demand_predicted,
        travel_m: compression_metres - corner.reference_compression_metres,
        axial_velocity_m_s: velocity_metres_per_second,
        motion_ratio: corner.motion_ratio,
        contact_impulse_ns: normal_impulse,
        contact_dt_s: substep_seconds,
        bump_stop_impulse_ns: bump_impulse,
        droop_stop_impulse_ns: droop_impulse,
        anchor_position,
        anchor_velocity,
        anchor_acceleration,
        axial_anchor_acceleration_m_s2: anchor_acceleration.dot(body_up),
        ..WheelVerticalDiagnostics::default()
    }
}

struct PreparedAxleCorner {
    wheel: WheelIndex,
    reference_compression_metres: f64,
    initial_compression_metres: f64,
    initial_velocity_metres_per_second: f64,
    spring_compression_metres: f64,
    motion_ratio: f64,
    geometry_invalid: bool,
    envelope: PreparedWheelEnvelope,
    tuning: WheelMechanicalTuning,
    /// Overtravel plus compliant hard-stop band, at the wheel (m).
    hard_stop_allowance_metres: f64,
    contact: SimplifiedNormalContact,
    simplified: std::sync::Arc<SimplifiedSuspension>,
}

impl PreparedAxleCorner {
    fn element_forces(
        &self,
        geometry: &GeometricSuspensionConfig,
        compression_metres: f64,
        velocity_metres_per_second: f64,
    ) -> (f64, f64, f64, f64, f64) {
        let travel_metres = compression_metres - self.reference_compression_metres;
        let (spring_compression_metres, motion_ratio, motion_ratio_rate_per_m) = self
            .simplified
            .vertical_reference_with_rate_at(self.wheel, travel_metres)
            .unwrap_or((self.spring_compression_metres, self.motion_ratio, 0.0));
        if self.geometry_invalid {
            return (
                0.0,
                0.0,
                spring_compression_metres,
                motion_ratio,
                motion_ratio_rate_per_m,
            );
        }
        let axle = geometry.axle(self.wheel);
        let total_deflection_metres =
            axle.spring_free_length_m - axle.spring_installed_length_m + spring_compression_metres;
        let spring_force_newtons =
            axle.spring_rate_N_per_m * total_deflection_metres.max(0.0) * motion_ratio;
        let damping_force_newtons =
            geometric_damper_element(axle, motion_ratio * velocity_metres_per_second)
                * motion_ratio;
        (
            spring_force_newtons,
            damping_force_newtons,
            spring_compression_metres,
            motion_ratio,
            motion_ratio_rate_per_m,
        )
    }

    fn stop_force(
        &self,
        geometry: &GeometricSuspensionConfig,
        compression_metres: f64,
        velocity_metres_per_second: f64,
        spring_compression_metres: f64,
        motion_ratio: f64,
    ) -> (f64, bool) {
        geometric_stop_force(
            geometry,
            self.wheel,
            compression_metres,
            self.reference_compression_metres,
            self.envelope,
            spring_compression_metres,
            motion_ratio,
            velocity_metres_per_second,
            self.tuning,
        )
    }

    fn compression_bounds_metres(&self) -> (f64, f64) {
        let bounds = geometric_travel_bounds(self.envelope, self.reference_compression_metres);
        (
            bounds.0 - self.hard_stop_allowance_metres,
            bounds.1 + self.hard_stop_allowance_metres,
        )
    }
}

#[allow(clippy::too_many_arguments)]
fn integrate_implicit_axle(
    compressions_metres: [f64; 2],
    velocities_metres_per_second: [f64; 2],
    masses_kilograms: [f64; 2],
    duration_seconds: f64,
    compression_bounds_metres: [(f64, f64); 2],
    reference_compressions_metres: [f64; 2],
    antiroll_stiffness_newtons_per_metre: f64,
    antiroll_damping_newtons_per_second_per_metre: f64,
    ground_contacts: [GroundContactConstraint; 2],
    force_at: impl Fn(usize, f64, f64) -> f64,
) -> ([VerticalSuspensionStep; 2], AxleSolveDiagnostics) {
    let corner_step = |index: usize, opposite_velocity_metres_per_second: f64| {
        let opposite_index = 1 - index;
        let opposite_travel_metres = compressions_metres[opposite_index]
            + duration_seconds * opposite_velocity_metres_per_second
            - reference_compressions_metres[opposite_index];
        integrate_vertical_suspension(
            true,
            compressions_metres[index],
            velocities_metres_per_second[index],
            masses_kilograms[index],
            duration_seconds,
            compression_bounds_metres[index],
            ground_contacts[index],
            |compression_metres, velocity_metres_per_second| {
                let antiroll_force_newtons = antiroll_stiffness_newtons_per_metre
                    * (compression_metres
                        - reference_compressions_metres[index]
                        - opposite_travel_metres);
                // Bar damper: proportional to the relative travel velocity of the
                // pair and opposite to it. Antisymmetric between the two corners,
                // so the axle momentum and the static equilibrium are untouched
                // and the power `F_self * v_self + F_opp * v_opp` is never negative.
                let antiroll_damping_newtons = antiroll_damping_newtons_per_second_per_metre
                    * (velocity_metres_per_second - opposite_velocity_metres_per_second);
                force_at(index, compression_metres, velocity_metres_per_second)
                    + antiroll_force_newtons
                    + antiroll_damping_newtons
            },
        )
    };
    let residual_at = |index: usize, candidate_velocities: [f64; 2]| {
        let compression_metres =
            compressions_metres[index] + duration_seconds * candidate_velocities[index];
        let opposite_index = 1 - index;
        let opposite_compression_metres = compressions_metres[opposite_index]
            + duration_seconds * candidate_velocities[opposite_index];
        let suspension_force_newtons =
            force_at(index, compression_metres, candidate_velocities[index]);
        let antiroll_force_newtons = antiroll_stiffness_newtons_per_metre
            * (compression_metres
                - reference_compressions_metres[index]
                - opposite_compression_metres
                + reference_compressions_metres[opposite_index]);
        let antiroll_damping_newtons = antiroll_damping_newtons_per_second_per_metre
            * (candidate_velocities[index] - candidate_velocities[opposite_index]);
        masses_kilograms[index]
            * (candidate_velocities[index] - velocities_metres_per_second[index])
            + duration_seconds * suspension_force_newtons
            + duration_seconds * antiroll_force_newtons
            + duration_seconds * antiroll_damping_newtons
    };
    // Admissible velocity interval per corner: the suspension travel stop AND, when
    // the road is binding, the ground plane. Both are unilateral bounds with the
    // same complementarity rule; a projection that ignores the ground can never
    // close a reaction that the corner solve has already accepted.
    let velocity_bounds = |index: usize| {
        let droop =
            (compression_bounds_metres[index].0 - compressions_metres[index]) / duration_seconds;
        let bump =
            (compression_bounds_metres[index].1 - compressions_metres[index]) / duration_seconds;
        let ground = if ground_contacts[index].active {
            (ground_contacts[index].limit_at(duration_seconds) - compressions_metres[index])
                / duration_seconds
        } else {
            f64::NEG_INFINITY
        };
        let mut lower = droop.max(ground);
        if lower > bump {
            lower = bump;
        }
        (lower, bump.max(lower))
    };
    let projected_residual = |index: usize, candidate_velocities: [f64; 2]| {
        let (lower, upper) = velocity_bounds(index);
        let residual = residual_at(index, candidate_velocities);
        let at_lower = candidate_velocities[index] <= lower + 1e-12;
        let at_upper = candidate_velocities[index] >= upper - 1e-12;
        if (at_lower && residual >= 0.0) || (at_upper && residual <= 0.0) {
            0.0
        } else {
            residual
        }
    };
    let bounds = std::array::from_fn::<_, 2, _>(|index| velocity_bounds(index));
    let mut next_velocities = std::array::from_fn(|index| {
        velocities_metres_per_second[index].clamp(bounds[index].0, bounds[index].1)
    });
    // Two-corner coupling: the plain Gauss-Seidel converges in a handful of iterations
    // at the host rates (1/240 .. 1/480). When the bar is stiff relative to the step
    // (map gain h*k_arb/m, the 120 Hz case) that map can oscillate, so a damped variant
    // derived from the gain takes over before the sequential fallback.
    let tolerance = 2e-9_f64;
    let projected_norm = |velocities: [f64; 2]| {
        projected_residual(0, velocities)
            .abs()
            .max(projected_residual(1, velocities).abs())
    };
    let mut best = projected_norm(next_velocities);
    let mut converged = best <= tolerance;
    let mut iterations = 0u32;
    if !converged {
        for iteration in 0..32 {
            iterations = iteration + 1;
            next_velocities[0] = corner_step(0, next_velocities[1]).velocity_metres_per_second;
            next_velocities[1] = corner_step(1, next_velocities[0]).velocity_metres_per_second;
            let norm = projected_norm(next_velocities);
            if norm <= tolerance {
                converged = true;
                break;
            }
            if norm > best * 1.5 {
                break;
            }
            best = best.min(norm);
        }
    }
    if !converged {
        let lightest = masses_kilograms
            .iter()
            .copied()
            .fold(f64::INFINITY, f64::min)
            .max(1.0);
        let gain = (duration_seconds * antiroll_stiffness_newtons_per_metre / lightest).max(0.0);
        let mut omega = (1.0 / (1.0 + gain)).clamp(0.02, 1.0);
        for _ in 0..64 {
            iterations += 1;
            let left = corner_step(0, next_velocities[1]).velocity_metres_per_second;
            let right = corner_step(1, left).velocity_metres_per_second;
            let candidate = [
                omega * left + (1.0 - omega) * next_velocities[0],
                omega * right + (1.0 - omega) * next_velocities[1],
            ];
            let norm = projected_norm(candidate);
            if norm <= tolerance {
                next_velocities = candidate;
                converged = true;
                break;
            }
            if norm > best * 0.999 {
                omega = (omega * 0.5).max(0.02);
            }
            best = best.min(norm);
            next_velocities = candidate;
        }
    }
    if !converged {
        // Eliminate the right corner with its exact unilateral solve and bracket
        // the left momentum equation. Unlike sequential recomputation, both final
        // corners must satisfy the same pair of velocities before acceptance.
        let mut low = bounds[0].0;
        let mut high = bounds[0].1;
        for _ in 0..96 {
            iterations += 1;
            let left = 0.5 * (low + high);
            let right = corner_step(1, left).velocity_metres_per_second;
            next_velocities = [left, right];
            if projected_norm(next_velocities) <= tolerance {
                converged = true;
                break;
            }
            if residual_at(0, next_velocities) > 0.0 {
                high = left;
            } else {
                low = left;
            }
        }
    }
    let steps = [
        corner_step(0, next_velocities[1]),
        corner_step(1, next_velocities[0]),
    ];
    let accepted = steps.map(|step| step.velocity_metres_per_second);
    let corner_residual_m_s = [
        projected_residual(0, accepted),
        projected_residual(1, accepted),
    ];
    let residual_m_s = corner_residual_m_s[0]
        .abs()
        .max(corner_residual_m_s[1].abs());
    converged &= residual_m_s <= 1e-7;
    (
        steps,
        AxleSolveDiagnostics {
            iterations,
            converged,
            residual_m_s,
            corner_residual_m_s,
        },
    )
}

/// Kinematics of one corner's chassis-side attachment, in world space.
#[derive(Clone, Copy, Debug, Default)]
struct AnchorKinematics {
    position: Vec3,
    velocity: Vec3,
    mount_position: Vec3,
    mount_velocity: Vec3,
    mount_acceleration: Vec3,
}

/// Publishes both corners of one axle. Results and contacts are read-only here: the
/// published values are measurements of the solve that just ran, never inputs.
#[allow(clippy::too_many_arguments)]
fn publish_corner_diagnostics(
    vertical: &mut [WheelVerticalDiagnostics; 4],
    corners: &[PreparedAxleCorner; 2],
    results: &[GeometricAxleCornerIntegration; 2],
    contacts: &[GroundContactConstraint; 2],
    anchors: &[AnchorKinematics; 2],
    accelerations: &[Vec3; 2],
    substep_seconds: f64,
    body_up: Vec3,
) {
    for index in 0..2 {
        let wheel_index = corners[index].wheel as usize;
        let mut diagnostic = corner_diagnostics(
            &corners[index],
            results[index].compression_metres,
            results[index].velocity_metres_per_second,
            contacts[index],
            substep_seconds,
            results[index].normal_impulse_newton_seconds,
            results[index].travel_limit_impulse_newton_seconds,
            anchors[index].position,
            anchors[index].velocity,
            accelerations[index],
            body_up,
        );
        // Ray evidence belongs to the contact query, not to this solve.
        diagnostic.ray_shortfall_m = vertical[wheel_index].ray_shortfall_m;
        diagnostic.contact_point = vertical[wheel_index].contact_point;
        diagnostic.contact_normal = vertical[wheel_index].contact_normal;
        diagnostic.ray_hit_mask = vertical[wheel_index].ray_hit_mask;
        diagnostic.solve_frame_acceleration = accelerations[index];
        diagnostic.anchor_position = anchors[index].mount_position;
        diagnostic.anchor_velocity = anchors[index].mount_velocity;
        diagnostic.anchor_acceleration = anchors[index].mount_acceleration;
        diagnostic.axial_anchor_acceleration_m_s2 = anchors[index].mount_acceleration.dot(body_up);
        vertical[wheel_index] = diagnostic;
    }
}

impl SuspensionSystem {
    fn prepare_simplified_axle_corner(
        &self,
        configuration: &VehicleConfig,
        wheel: WheelIndex,
        modifiers: TireMechanicalModifiers,
    ) -> PreparedAxleCorner {
        let state = &self.wheels[wheel as usize];
        let reference_compression_metres = rest_compression_m(configuration, wheel);
        let travel_metres = state.suspension_compression_m - reference_compression_metres;
        let simplified = self
            .simplified_prepared
            .as_ref()
            .expect("Equivalent suspension parameters must be prepared")
            .clone();
        let prepared_pose = simplified.vertical_reference_with_rate_at(wheel, travel_metres);
        if let Some(counts) = &self.benchmark_evaluation_counts {
            counts[0].fetch_add(1, std::sync::atomic::Ordering::Relaxed);
            if prepared_pose.is_some() {
                counts[1].fetch_add(1, std::sync::atomic::Ordering::Relaxed);
            }
        }
        let (spring_compression_metres, motion_ratio, _) =
            prepared_pose.expect("Equivalent suspension parameters and travel must be valid");
        let geometry_invalid = false;
        let tuning = WheelMechanicalTuning::for_wheel(configuration, wheel);
        // Unilateral contact: enforce the ground limit only while the tyre is touching
        // or penetrating it. A floating corner keeps the constraint inactive, so the
        // solver never produces a normal impulse for a wheel that is in the air.
        let contact = if state.contact_fraction > 0.0
            && state.ground_gap_m <= crate::suspension::CONTACT_GAP_TOLERANCE_M
        {
            SimplifiedNormalContact {
                road_compression_metres: state.normal_road_compression_metres,
                road_velocity_metres_per_second: state.normal_road_velocity_metres_per_second,
                normal_projection: state.normal_contact_projection,
                active: true,
            }
        } else {
            SimplifiedNormalContact::inactive()
        };
        let _ = modifiers;
        PreparedAxleCorner {
            wheel,
            reference_compression_metres,
            initial_compression_metres: state.suspension_compression_m,
            initial_velocity_metres_per_second: state.unsprung_velocity_m_s,
            spring_compression_metres,
            motion_ratio,
            geometry_invalid,
            envelope: self.geometric_prepared.as_ref().unwrap().for_wheel(wheel),
            hard_stop_allowance_metres: hard_stop_allowance_m(
                configuration
                    .suspension
                    .geometric_suspension
                    .as_ref()
                    .expect("geometric suspension")
                    .axle(wheel),
                tuning,
            ),
            tuning,
            contact,
            simplified,
        }
    }

    pub(super) fn simplified_antiroll_stiffness(
        &self,
        configuration: &VehicleConfig,
        wheels: [WheelIndex; 2],
    ) -> f64 {
        let geometry = configuration
            .suspension
            .geometric_suspension
            .as_ref()
            .unwrap();
        let wheel_rate = |wheel| {
            self.geometric_prepared
                .as_ref()
                .map(|prepared| prepared.for_wheel(wheel).k_wheel_rest_n_per_m)
                .unwrap_or_else(|| geometric_k_wheel_rest(geometry, wheel).max(1.0))
        };
        let axle_ratio = if wheels[0].is_front() {
            configuration.suspension.front_arb_ratio
        } else {
            configuration.suspension.rear_arb_ratio
        };
        let antiroll_configuration = self
            .backend
            .effective_antiroll_configuration(*geometry.arb(wheels[0].is_front()), axle_ratio);
        geometric_arb_force(
            &antiroll_configuration,
            1.0,
            0.0,
            0.5 * (wheel_rate(wheels[0]) + wheel_rate(wheels[1])),
        )
    }

    /// Anti-roll bar damper for one axle (N·s/m at the wheel). Authored per axle,
    /// already expressed in wheel-space relative travel velocity.
    pub(super) fn simplified_antiroll_damping(
        &self,
        configuration: &VehicleConfig,
        wheels: [WheelIndex; 2],
    ) -> f64 {
        configuration
            .suspension
            .geometric_suspension
            .as_ref()
            .map(|geometry| geometry.axle(wheels[0]).arb_damper_Ns_per_m.max(0.0))
            .unwrap_or(0.0)
    }

    pub(super) fn solve_simplified_geometric_axle(
        &mut self,
        configuration: &VehicleConfig,
        modifiers: &[TireMechanicalModifiers; 4],
        wheels: [WheelIndex; 2],
        duration_seconds: f64,
        body_orientation: Mat3,
    ) {
        let geometry = configuration
            .suspension
            .geometric_suspension
            .as_ref()
            .unwrap();
        self.ensure_geometric_prepared(configuration);
        let corners = wheels.map(|wheel| {
            self.prepare_simplified_axle_corner(configuration, wheel, modifiers[wheel as usize])
        });
        let antiroll_stiffness = self.simplified_antiroll_stiffness(configuration, wheels);
        let antiroll_damping = self.simplified_antiroll_damping(configuration, wheels);
        // Weight plus body acceleration along the suspension axis. The corner mass is
        // physical, so it carries its own weight and the pseudo-force of the anchor
        // acceleration; the chassis is charged only the element reaction.
        let axle_index = if wheels[0].is_front() { 0usize } else { 1usize };
        let body_up = body_orientation.y;
        let omega = self.body_angular_velocity_world;
        let alpha = self.body_angular_acceleration_world;
        let body_acceleration = self.body_acceleration_world;
        let body_velocity = self.body_linear_velocity_world;
        let cg = crate::simulation::center_of_mass_local_for_suspension_backend(
            configuration,
            self.backend,
        );
        let com_world = self.body_reference_world + body_orientation.transform_vector(cg);
        // The corner's own attachment point: the same point the axial pseudo-force
        // acts on, published so the host can compare "the wheel goes down" with
        // "the chassis pulls the mount up faster".
        let anchor_kinematics: [AnchorKinematics; 2] = std::array::from_fn(|index| {
            let corner = &corners[index];
            let anchor = configuration.tires.wheel_anchor_local(
                &configuration.chassis,
                &configuration.suspension,
                corner.wheel,
            );
            let length = super::spring_length(configuration, corner.wheel);
            let arm = body_orientation.transform_vector(
                anchor - Vec3::UP * (length - corner.initial_compression_metres) - cg,
            );
            let mount_arm = body_orientation.transform_vector(anchor - cg);
            AnchorKinematics {
                mount_position: com_world + mount_arm,
                mount_velocity: body_velocity + omega.cross(mount_arm),
                mount_acceleration: body_acceleration
                    + alpha.cross(mount_arm)
                    + omega.cross(omega.cross(mount_arm)),
                position: com_world + arm,
                velocity: body_velocity + omega.cross(arm),
            }
        });
        let anchor_acceleration: [Vec3; 2] = std::array::from_fn(|index| {
            let arm = anchor_kinematics[index].position - com_world;
            body_acceleration
                + alpha.cross(arm)
                + omega.cross(omega.cross(arm))
                + omega.cross(body_up) * (2.0 * corners[index].initial_velocity_metres_per_second)
        });
        let axial_gravity = anchor_acceleration.map(|acceleration| {
            crate::wheel_mechanics::GRAVITY_M_S2 * body_orientation.y.dot(Vec3::UP)
                + acceleration.dot(body_orientation.y)
        });
        let target_duration_seconds = corners
            .iter()
            .map(|corner| corner.tuning.mechanical_substep_s.max(1e-5))
            .fold(f64::INFINITY, f64::min);
        // Body and corner use one common implicit interval. Subdividing only the
        // corner would hold the body trajectory at a different time level.
        let _ = target_duration_seconds;
        let substeps = 1;
        let substep_duration_seconds = duration_seconds / substeps as f64;
        let mut results = std::array::from_fn::<_, 2, _>(|index| GeometricAxleCornerIntegration {
            initial_spring_compression_metres: corners[index].spring_compression_metres,
            initial_motion_ratio: corners[index].motion_ratio,
            initial_geometry_invalid: corners[index].geometry_invalid,
            compression_metres: corners[index].initial_compression_metres,
            velocity_metres_per_second: corners[index].initial_velocity_metres_per_second,
            normal_impulse_newton_seconds: 0.0,
            final_normal_force_newtons: 0.0,
            suspension_impulse_newton_seconds: 0.0,
            travel_limit_impulse_newton_seconds: 0.0,
            damper_dissipated_joules: 0.0,
            clamped: corners[index].geometry_invalid,
            antiroll_force_newtons: 0.0,
            force_application_compression_metres: 0.0,
            motion_ratio_rate_per_m: 0.0,
        });
        let mut last_ground_contacts = [GroundContactConstraint::inactive(); 2];
        let mut axle_iterations = 0u32;
        let mut axle_converged = true;
        let mut axle_residual_m_s = 0.0_f64;
        for substep_index in 0..substeps {
            let elapsed_seconds = substep_index as f64 * substep_duration_seconds;
            let mut ground_contacts = std::array::from_fn(|index| {
                let mut contact = corners[index].contact.constraint_at(elapsed_seconds);
                // Backward Euler body endpoint: y' = y + h*(v+h*a).
                // The road limit must use that SAME endpoint, not the old velocity.
                let normal = self.wheels[corners[index].wheel as usize].effective_normal;
                let projection = normal.dot(body_orientation.y).max(1e-3);
                contact.limit_velocity_metres_per_second -=
                    normal.dot(anchor_acceleration[index]) / projection * substep_duration_seconds;
                contact
            });
            last_ground_contacts = ground_contacts;
            for index in 0..2 {
                let limit_m = corners[index].compression_bounds_metres().1;
                if ground_contacts[index].active
                    && ground_contacts[index].limit_at(substep_duration_seconds) > limit_m
                {
                    // Clamp predicted ground demand to suspension bump stop limit instead
                    // of rejecting the step and permanently freezing the vehicle.
                    if ground_contacts[index].limit_compression_metres > limit_m {
                        ground_contacts[index].limit_compression_metres = limit_m;
                    }
                    let max_vel = (limit_m - ground_contacts[index].limit_compression_metres)
                        / substep_duration_seconds;
                    if ground_contacts[index].limit_velocity_metres_per_second > max_vel {
                        ground_contacts[index].limit_velocity_metres_per_second = max_vel;
                    }
                }
            }
            let (steps, diagnostics) = integrate_implicit_axle(
                results.map(|result| result.compression_metres),
                results.map(|result| result.velocity_metres_per_second),
                std::array::from_fn(|index| {
                    corners[index].tuning.physical_unsprung_mass_kg.max(1.0)
                }),
                substep_duration_seconds,
                std::array::from_fn(|index| corners[index].compression_bounds_metres()),
                std::array::from_fn(|index| corners[index].reference_compression_metres),
                antiroll_stiffness,
                antiroll_damping,
                ground_contacts,
                |index, compression_metres, velocity_metres_per_second| {
                    let corner = &corners[index];
                    let (spring_force, damping_force, spring_compression, motion_ratio, _) = corner
                        .element_forces(geometry, compression_metres, velocity_metres_per_second);
                    spring_force
                        + damping_force
                        + corner
                            .stop_force(
                                geometry,
                                compression_metres,
                                velocity_metres_per_second,
                                spring_compression,
                                motion_ratio,
                            )
                            .0
                        + corner.tuning.physical_unsprung_mass_kg * axial_gravity[index]
                },
            );
            axle_iterations = axle_iterations.max(diagnostics.iterations);
            axle_converged &= diagnostics.converged;
            axle_residual_m_s = axle_residual_m_s.max(diagnostics.residual_m_s);
            if !diagnostics.converged && (diagnostics.residual_m_s > 100.0 || !diagnostics.residual_m_s.is_finite()) {
                let worst = if diagnostics.corner_residual_m_s[0].abs()
                    >= diagnostics.corner_residual_m_s[1].abs()
                {
                    0
                } else {
                    1
                };
                let demand_m = if last_ground_contacts[worst].active {
                    last_ground_contacts[worst].limit_at(substep_duration_seconds)
                } else {
                    f64::NAN
                };
                let limit_m = corners[worst].compression_bounds_metres().1;
                let failure = FailureDiagnostics {
                    code: FailureCode::AxleNotConverged,
                    wheel: corners[worst].wheel as i32,
                    axle: axle_index as i32,
                    iteration: diagnostics.iterations,
                    subdivisions: 0,
                    demand_m,
                    limit_m,
                    excess_m: demand_m - limit_m,
                    limit_velocity_m_s: last_ground_contacts[worst]
                        .limit_velocity_metres_per_second,
                    residual_m_s: diagnostics.residual_m_s,
                    axle_wheels: [corners[0].wheel as i32, corners[1].wheel as i32],
                };
                publish_corner_diagnostics(
                    &mut self.vertical,
                    &corners,
                    &results,
                    &last_ground_contacts,
                    &anchor_kinematics,
                    &anchor_acceleration,
                    substep_duration_seconds,
                    body_up,
                );
                // The rejected row is written once, by `advance_transaction`.
                self.failure = Some(failure);
                self.mechanical_failure =
                    Some("Axle solve did not close simultaneous contact/travel constraints".into());
                return;
            }
            for (index, step) in steps.into_iter().enumerate() {
                let result = &mut results[index];
                let corner = &corners[index];
                result.compression_metres = step.compression_metres;
                result.velocity_metres_per_second = step.velocity_metres_per_second;
                let normal_impulse = step.normal_contact_impulse_newton_seconds;
                result.normal_impulse_newton_seconds += normal_impulse;
                result.force_application_compression_metres +=
                    step.compression_metres * normal_impulse;
                result.final_normal_force_newtons = corner
                    .contact
                    .normal_force_from_impulse(normal_impulse, substep_duration_seconds);
                // Remove weight and moving-frame inertia: only internal elements
                // and the opposite hard-stop impulse belong on the body.
                result.suspension_impulse_newton_seconds += step.suspension_impulse_newton_seconds
                    - corner.tuning.physical_unsprung_mass_kg
                        * axial_gravity[index]
                        * substep_duration_seconds;
                result.travel_limit_impulse_newton_seconds +=
                    step.travel_limit_impulse_newton_seconds;
                let (_, damping_force, spring_compression, motion_ratio, motion_ratio_rate) =
                    corner.element_forces(
                        geometry,
                        step.compression_metres,
                        step.velocity_metres_per_second,
                    );
                result.damper_dissipated_joules +=
                    (damping_force * step.velocity_metres_per_second * substep_duration_seconds)
                        .max(0.0);
                result.motion_ratio_rate_per_m = motion_ratio_rate;
                result.clamped |= corner
                    .stop_force(
                        geometry,
                        step.compression_metres,
                        step.velocity_metres_per_second,
                        spring_compression,
                        motion_ratio,
                    )
                    .1;
            }
            // Phase A instrumentation: the acceptance criterion is observable so the
            // host trace can report how often the fallback runs and how hard the
            // fixed point works. No force impact.
            self.axle_max_iterations = self.axle_max_iterations.max(diagnostics.iterations);
        }
        let antiroll_force = antiroll_stiffness
            * (results[0].compression_metres
                - corners[0].reference_compression_metres
                - results[1].compression_metres
                + corners[1].reference_compression_metres);
        results[0].antiroll_force_newtons = antiroll_force;
        results[1].antiroll_force_newtons = -antiroll_force;
        publish_corner_diagnostics(
            &mut self.vertical,
            &corners,
            &results,
            &last_ground_contacts,
            &anchor_kinematics,
            &anchor_acceleration,
            substep_duration_seconds,
            body_up,
        );
        self.axles[axle_index] = AxleVerticalDiagnostics {
            iterations: axle_iterations,
            converged: axle_converged,
            substeps: substeps as u32,
            residual_m_s: axle_residual_m_s,
            antiroll_stiffness_n_per_m: antiroll_stiffness,
            antiroll_force_n: antiroll_force,
            normal_load_n: results[0].final_normal_force_newtons
                + results[1].final_normal_force_newtons,
        };
        for index in 0..2 {
            results[index].force_application_compression_metres =
                if results[index].normal_impulse_newton_seconds > 0.0 {
                    results[index].force_application_compression_metres
                        / results[index].normal_impulse_newton_seconds
                } else {
                    results[index].compression_metres
                };
            self.solve_force_geometric(
                configuration,
                wheels[index],
                modifiers[wheels[index] as usize],
                results[1 - index].compression_metres,
                duration_seconds,
                true,
                Some(&results[index]),
            );
        }
    }
}

#[cfg(test)]
mod tests {
    use super::integrate_implicit_axle;
    use crate::suspension::vertical_suspension_integration::GroundContactConstraint;

    fn inactive_ground() -> [GroundContactConstraint; 2] {
        [GroundContactConstraint::inactive(); 2]
    }

    #[test]
    fn differential_mode_dissipates_energy_and_preserves_axle_momentum() {
        for stiffness in [20_000.0, 20_000_000.0] {
            let mut compressions: [f64; 2] = [0.01, -0.01];
            let mut velocities = [0.0, 0.0];
            let mut previous_energy = 0.5 * stiffness * (compressions[0] - compressions[1]).powi(2);
            for _ in 0..120 {
                let (steps, _) = integrate_implicit_axle(
                    compressions,
                    velocities,
                    [18.0; 2],
                    1.0 / 480.0,
                    [(-0.1, 0.1); 2],
                    [0.0; 2],
                    stiffness,
                    0.0,
                    inactive_ground(),
                    |_, _, _| 0.0,
                );
                for index in 0..2 {
                    let momentum_change =
                        18.0 * (steps[index].velocity_metres_per_second - velocities[index]);
                    assert!(
                        (momentum_change + steps[index].suspension_impulse_newton_seconds).abs()
                            < 1e-7
                    );
                }
                compressions = steps.each_ref().map(|step| step.compression_metres);
                velocities = steps.each_ref().map(|step| step.velocity_metres_per_second);
                let energy = 0.5 * stiffness * (compressions[0] - compressions[1]).powi(2)
                    + 9.0 * (velocities[0].powi(2) + velocities[1].powi(2));
                assert!(energy <= previous_energy + 1e-7);
                previous_energy = energy;
            }
        }
    }

    #[test]
    fn ground_reaction_closes_momentum_on_a_rigid_contact() {
        let mass = 18.0;
        let (steps, _) = integrate_implicit_axle(
            [0.02; 2],
            [-2.0; 2],
            [mass; 2],
            1.0 / 500.0,
            [(-0.1, 0.1); 2],
            [0.0; 2],
            0.0,
            0.0,
            [
                GroundContactConstraint {
                    limit_compression_metres: 0.019,
                    limit_velocity_metres_per_second: 0.0,
                    active: true,
                },
                GroundContactConstraint::inactive(),
            ],
            |_, _, _| 0.0,
        );
        let momentum_change = mass * (steps[0].velocity_metres_per_second + 2.0);
        assert!((momentum_change - steps[0].normal_contact_impulse_newton_seconds).abs() < 1e-9);
        assert!(steps[0].compression_metres >= 0.019 - 1e-12);
        assert_eq!(steps[1].normal_contact_impulse_newton_seconds, 0.0);
    }
}

#[cfg(test)]
mod telemetry_gap_tests {
    #[test]
    fn airborne_is_positive_and_penetration_negative() {
        assert!((super::ground_gap(0.08, 0.05) - 0.03).abs() < 1e-12);
        assert!(super::ground_gap(0.04, 0.05) < 0.0);
    }
}
