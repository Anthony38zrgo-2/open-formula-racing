use super::vertical_suspension_integration::GroundContactConstraint;
use super::*;

/// Rigid wheel-ground contact expressed in the unsprung (suspension compression)
/// coordinate of the simplified vertical integration.
///
/// The transverse rays report a ground plane. Projected onto the body axis, the wheel
/// cannot rise less than `radius / projection` above the contact point, which is the
/// one-sided limit consumed by the vertical solver. No elastic energy and no radial
/// damping are associated with this object: the only unknown the solver produces is
/// the non-negative normal impulse.
#[derive(Clone, Copy)]
pub(super) struct SimplifiedNormalContact {
    /// Ground-limited compression at the start of the axle step (metres).
    pub road_compression_metres: f64,
    /// Rate of the ground-limited compression (metres per second).
    pub road_velocity_metres_per_second: f64,
    /// Cosine between the contact normal and the body up axis.
    pub normal_projection: f64,
    /// Whether any transverse ray reported ground for this wheel.
    pub active: bool,
}

impl SimplifiedNormalContact {
    pub fn inactive() -> Self {
        Self {
            road_compression_metres: 0.0,
            road_velocity_metres_per_second: 0.0,
            normal_projection: 1.0,
            active: false,
        }
    }

    #[inline]
    fn projection(&self) -> f64 {
        self.normal_projection.clamp(1e-3, 1.0)
    }

    /// Constraint for a substep starting at `elapsed_seconds` after the axle step began.
    pub fn constraint_at(&self, elapsed_seconds: f64) -> GroundContactConstraint {
        if !self.active {
            return GroundContactConstraint::inactive();
        }
        let projection = self.projection();
        GroundContactConstraint {
            limit_compression_metres: (self.road_compression_metres
                + self.road_velocity_metres_per_second * elapsed_seconds)
                / projection,
            limit_velocity_metres_per_second: self.road_velocity_metres_per_second / projection,
            active: true,
        }
    }

    /// Normal load (N) implied by an accepted generalized contact impulse. The solver
    /// works in the unsprung coordinate, so the virtual work of the normal reaction is
    /// `J_normal * projection`.
    pub fn normal_force_from_impulse(&self, impulse_newton_seconds: f64, duration_seconds: f64) -> f64 {
        if !self.active || duration_seconds <= 0.0 || impulse_newton_seconds <= 0.0 {
            return 0.0;
        }
        (impulse_newton_seconds / self.projection()) / duration_seconds
    }
}

impl SuspensionSystem {
    /// Single authoritative route for the contact-height target of the Simplified
    /// geometric backend.
    ///
    /// `sample_contact` only seeds a raw estimate for the body-less compatibility
    /// entry point; every production step ends here, where the target is derived
    /// from the body pose and then **rate-limited** (and optionally low-passed)
    /// before the vertical solver consumes it:
    ///
    /// * the target may not move more than `road_height_max_rate_m_s * dt` per
    ///   step, so a kerb/step cannot demand an unbounded compression in one tick;
    /// * the published normal velocity is clamped to the same rate, so the
    ///   in-step extrapolation `limit_at(t)` cannot outrun the slew limit;
    /// * the optional `road_height_filter_tau_s` low-pass runs on the same route
    ///   (never on the force, never twice).
    ///
    /// `previous_compression_metres` is the value that entered the solver on the
    /// previous step (captured before `sample_contact` rewrites the field).
    pub(super) fn configure_simplified_contact_kinematics(
        &mut self,
        configuration: &VehicleConfig,
        body: crate::simulation::BodyKinematics,
        previous_compression_metres: [f64; 4],
        duration_seconds: f64,
    ) {
        let axis = body.transform.basis.y;
        let axis_velocity = body.angular_velocity.cross(axis);
        let center = crate::simulation::center_of_mass_world_for_suspension_backend(
            configuration,
            self.backend,
            &body.transform,
        );
        let filters = configuration.suspension_filters.as_ref();
        let max_rate = filters.map(|f| f.road_height_max_rate_m_s()).unwrap_or(0.0);
        let tau = filters.map(|f| f.road_height_filter_tau_s()).unwrap_or(0.0);
        let dt = duration_seconds.max(1e-6);
        for wheel in WheelIndex::ALL {
            let state = &mut self.wheels[wheel as usize];
            if state.contact_fraction <= 0.0 {
                continue;
            }
            let normal = state.effective_normal.normalized();
            let anchor = configuration.tires.wheel_anchor_local(
                &configuration.chassis,
                &configuration.suspension,
                wheel,
            );
            let base =
                body.transform.transform_point(anchor) - axis * spring_length(configuration, wheel);
            let raw_compression = tire_radius(configuration, wheel)
                - normal.dot(base - state.effective_contact_point);
            let raw_velocity =
                -normal.dot(body.linear_velocity + body.angular_velocity.cross(base - center));
            let mut target = raw_compression;
            if state.normal_road_target_initialized {
                let previous = previous_compression_metres[wheel as usize];
                if tau > 0.0 {
                    let alpha = 1.0 - (-dt / tau).exp();
                    target = previous + (raw_compression - previous) * alpha;
                }
                if max_rate > 0.0 {
                    let allowed = max_rate * dt;
                    target = previous + (target - previous).clamp(-allowed, allowed);
                }
            }
            state.normal_contact_projection = normal.dot(axis).clamp(0.0, 1.0);
            state.normal_contact_projection_rate_per_second = normal.dot(axis_velocity);
            state.normal_road_compression_metres = target;
            state.normal_road_velocity_metres_per_second = if max_rate > 0.0 {
                raw_velocity.clamp(-max_rate, max_rate)
            } else {
                raw_velocity
            };
            state.normal_road_target_initialized = true;
        }
    }

    pub(super) fn update_simplified_contact_application_points(
        &mut self,
        configuration: &VehicleConfig,
        body: crate::simulation::BodyKinematics,
    ) {
        let axis = body.transform.basis.y;
        for wheel in WheelIndex::ALL {
            let state = &mut self.wheels[wheel as usize];
            if state.contact_fraction <= 0.0 {
                continue;
            }
            let normal = state.effective_normal.normalized();
            let anchor = configuration.tires.wheel_anchor_local(
                &configuration.chassis,
                &configuration.suspension,
                wheel,
            );
            let hub = body.transform.transform_point(anchor)
                - axis
                    * (spring_length(configuration, wheel)
                        - state.normal_force_application_compression_metres);
            state.effective_contact_point =
                hub - normal * normal.dot(hub - state.effective_contact_point);
        }
    }
}
