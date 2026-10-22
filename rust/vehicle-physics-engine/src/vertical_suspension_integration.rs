//! Rigid unilateral wheel-ground contact for the simplified vertical integration.
//!
//! Open Formula removes the elastic/damped radial tyre carcass from the vertical
//! dynamics. The wheel/ground interaction is a single-sided constraint on the
//! unsprung coordinate (the suspension compression measured along the body axis):
//!
//! * feasibility: `compression >= ground_limit` (the wheel centre cannot pass below
//!   the road surface),
//! * the reaction `J >= 0` is a normal impulse that pushes the wheel out of the
//!   ground,
//! * complementarity: the reaction is zero whenever the gap is positive.
//!
//! The impulse is solved inside the same implicit Euler / active-set step that
//! integrates the unsprung mass, together with the suspension spring, damper,
//! anti-roll bar and travel stops. There is no penalty stiffness, no radial damper
//! and no restitution: impacts are perfectly inelastic in the normal direction.
//! The published normal load is the accepted impulse divided by the step duration,
//! which is exactly what the tangential tyre model consumes.

/// Moving one-sided limit imposed by the ground on the unsprung coordinate.
#[derive(Clone, Copy, Debug)]
pub(super) struct GroundContactConstraint {
    /// Compression at which the wheel just touches the ground, at the start of the step.
    pub limit_compression_metres: f64,
    /// Rate of change of that limit (terrain slope/body motion).
    pub limit_velocity_metres_per_second: f64,
    /// True when at least one transverse ray reported ground.
    pub active: bool,
}

impl GroundContactConstraint {
    pub fn inactive() -> Self {
        Self {
            limit_compression_metres: f64::NEG_INFINITY,
            limit_velocity_metres_per_second: 0.0,
            active: false,
        }
    }

    #[inline]
    pub fn limit_at(&self, elapsed_seconds: f64) -> f64 {
        self.limit_compression_metres + self.limit_velocity_metres_per_second * elapsed_seconds
    }
}

/// Result of one accepted vertical step for a single corner.
#[derive(Clone, Copy, Debug)]
pub(super) struct VerticalSuspensionStep {
    pub compression_metres: f64,
    pub velocity_metres_per_second: f64,
    /// Accepted ground reaction impulse (N s), always >= 0. This is the normal load
    /// the tangential tyre model must consume (`J / h`).
    pub normal_contact_impulse_newton_seconds: f64,
    /// Impulse delivered by the suspension element (spring/damper/ARB/stops).
    pub suspension_impulse_newton_seconds: f64,
    /// Impulse delivered by the suspension travel limit (droop negative, bump positive).
    pub travel_limit_impulse_newton_seconds: f64,
}

/// Implicit Euler step of one unsprung corner with a rigid ground constraint.
///
/// `suspension_force_at(compression, velocity)` returns the suspension element force
/// that pushes the wheel *down* (positive value), i.e. the resistance the ground has
/// to balance. It must not contain any tyre radial force: the tyre no longer stores
/// or dissipates vertical energy.
pub(super) fn integrate_vertical_suspension(
    use_implicit_integration: bool,
    compression_metres: f64,
    velocity_metres_per_second: f64,
    mass_kilograms: f64,
    duration_seconds: f64,
    travel_bounds_metres: (f64, f64),
    ground_contact: GroundContactConstraint,
    suspension_force_at: impl Fn(f64, f64) -> f64,
) -> VerticalSuspensionStep {
    if duration_seconds <= 0.0 {
        return VerticalSuspensionStep {
            compression_metres,
            velocity_metres_per_second,
            normal_contact_impulse_newton_seconds: 0.0,
            suspension_impulse_newton_seconds: 0.0,
            travel_limit_impulse_newton_seconds: 0.0,
        };
    }
    let ground_limit = ground_contact.limit_at(duration_seconds);
    let ground_bound_velocity = (ground_limit - compression_metres) / duration_seconds;

    if !use_implicit_integration {
        // Explicit predictor used only by the retired Standard path. The ground is
        // still enforced as a projection with zero restitution so that the fallback
        // never lets the wheel penetrate the surface.
        let suspension_force_newtons =
            suspension_force_at(compression_metres, velocity_metres_per_second);
        let mut next_velocity_metres_per_second = velocity_metres_per_second
            - suspension_force_newtons / mass_kilograms * duration_seconds;
        let mut normal_contact_impulse_newton_seconds = 0.0;
        if ground_contact.active && next_velocity_metres_per_second < ground_bound_velocity {
            let reaction = mass_kilograms
                * (ground_bound_velocity - next_velocity_metres_per_second);
            if reaction > 0.0 {
                next_velocity_metres_per_second = ground_bound_velocity;
                normal_contact_impulse_newton_seconds = reaction;
            }
        }
        let mut next_compression_metres =
            compression_metres + next_velocity_metres_per_second * duration_seconds;
        if next_compression_metres < travel_bounds_metres.0 {
            next_compression_metres = travel_bounds_metres.0;
            next_velocity_metres_per_second = next_velocity_metres_per_second.max(0.0);
        } else if next_compression_metres > travel_bounds_metres.1 {
            next_compression_metres = travel_bounds_metres.1;
            next_velocity_metres_per_second = next_velocity_metres_per_second.min(0.0);
        }
        return VerticalSuspensionStep {
            compression_metres: next_compression_metres,
            velocity_metres_per_second: next_velocity_metres_per_second,
            normal_contact_impulse_newton_seconds,
            suspension_impulse_newton_seconds: suspension_force_newtons * duration_seconds,
            travel_limit_impulse_newton_seconds: mass_kilograms
                * (next_velocity_metres_per_second - velocity_metres_per_second)
                - duration_seconds * (0.0 - suspension_force_newtons),
        };
    }

    // Position is advanced implicitly: x' = x + h v'. The reaction impulse at either
    // one-sided limit is J = m (v' - v) + h F_susp(x', v'), which must be >= 0 for an
    // admissible contact.
    let momentum_residual = |next_velocity_metres_per_second: f64| {
        let next_compression_metres =
            compression_metres + duration_seconds * next_velocity_metres_per_second;
        let suspension_force_newtons =
            suspension_force_at(next_compression_metres, next_velocity_metres_per_second);
        mass_kilograms * (next_velocity_metres_per_second - velocity_metres_per_second)
            + duration_seconds * suspension_force_newtons
    };

    let droop_velocity_bound = (travel_bounds_metres.0 - compression_metres) / duration_seconds;
    let bump_velocity_bound = (travel_bounds_metres.1 - compression_metres) / duration_seconds;
    let ground_is_binding =
        ground_contact.active && ground_bound_velocity > droop_velocity_bound;
    let mut lower_velocity_bound = if ground_is_binding {
        ground_bound_velocity
    } else {
        droop_velocity_bound
    };
    // Degenerate geometry (terrain pushed above the bump stop in one step): keep the
    // bracket ordered and let the ground reaction carry the remainder.
    if lower_velocity_bound > bump_velocity_bound {
        lower_velocity_bound = bump_velocity_bound;
    }
    let upper_velocity_bound = bump_velocity_bound.max(lower_velocity_bound);

    let mut next_velocity_metres_per_second;
    let mut constrained_lower = false;
    let mut constrained_upper = false;
    if momentum_residual(lower_velocity_bound) >= 0.0 {
        next_velocity_metres_per_second = lower_velocity_bound;
        constrained_lower = true;
    } else if momentum_residual(upper_velocity_bound) <= 0.0 {
        next_velocity_metres_per_second = upper_velocity_bound;
        constrained_upper = true;
    } else {
        let mut low = lower_velocity_bound;
        let mut high = upper_velocity_bound;
        next_velocity_metres_per_second = velocity_metres_per_second.clamp(low, high);
        for _ in 0..48 {
            let residual_newton_seconds = momentum_residual(next_velocity_metres_per_second);
            if residual_newton_seconds.abs() <= 1e-9 {
                break;
            }
            if residual_newton_seconds > 0.0 {
                high = next_velocity_metres_per_second;
            } else {
                low = next_velocity_metres_per_second;
            }
            next_velocity_metres_per_second = 0.5 * (low + high);
        }
    }

    let next_compression_metres =
        compression_metres + duration_seconds * next_velocity_metres_per_second;
    let suspension_force_newtons =
        suspension_force_at(next_compression_metres, next_velocity_metres_per_second);
    let accepted_residual = mass_kilograms
        * (next_velocity_metres_per_second - velocity_metres_per_second)
        + duration_seconds * suspension_force_newtons;

    let mut normal_contact_impulse_newton_seconds = 0.0;
    let mut travel_limit_impulse_newton_seconds = 0.0;
    if constrained_lower {
        if ground_is_binding {
            normal_contact_impulse_newton_seconds = accepted_residual.max(0.0);
        } else {
            travel_limit_impulse_newton_seconds = accepted_residual;
        }
    } else if constrained_upper {
        travel_limit_impulse_newton_seconds = accepted_residual;
    }

    VerticalSuspensionStep {
        compression_metres: next_compression_metres,
        velocity_metres_per_second: next_velocity_metres_per_second,
        normal_contact_impulse_newton_seconds,
        suspension_impulse_newton_seconds: suspension_force_newtons * duration_seconds,
        travel_limit_impulse_newton_seconds,
    }
}

#[cfg(test)]
mod tests {
    use super::{integrate_vertical_suspension, GroundContactConstraint};

    fn ground(limit: f64, velocity: f64) -> GroundContactConstraint {
        GroundContactConstraint {
            limit_compression_metres: limit,
            limit_velocity_metres_per_second: velocity,
            active: true,
        }
    }

    #[test]
    fn resting_wheel_gets_the_static_reaction_and_never_penetrates() {
        let mass = 18.0;
        let stiffness = 180_000.0;
        let load = mass * 9.80665;
        let static_compression = load / stiffness;
        let step = integrate_vertical_suspension(
            true,
            static_compression,
            0.0,
            mass,
            1.0 / 500.0,
            (-0.05, 0.10),
            ground(static_compression, 0.0),
            |compression, _| stiffness * compression,
        );
        assert!(step.compression_metres >= static_compression - 1e-12);
        assert!(step.normal_contact_impulse_newton_seconds > 0.0);
        assert!(
            (step.normal_contact_impulse_newton_seconds / (1.0 / 500.0) - load).abs() < 1e-3
        );
    }

    #[test]
    fn wheel_above_the_ground_carries_no_reaction() {
        let step = integrate_vertical_suspension(
            true,
            0.06,
            0.0,
            18.0,
            1.0 / 500.0,
            (-0.05, 0.10),
            ground(0.001, 0.0),
            |compression, _| 180_000.0 * compression,
        );
        assert_eq!(step.normal_contact_impulse_newton_seconds, 0.0);
    }

    #[test]
    fn impact_is_inelastic_and_conserves_momentum() {
        let mass = 18.0;
        let step = integrate_vertical_suspension(
            true,
            0.02,
            -3.0,
            mass,
            1.0 / 500.0,
            (-0.05, 0.10),
            ground(0.0195, 0.0),
            |_, _| 0.0,
        );
        assert!(step.compression_metres >= 0.0195 - 1e-12);
        let momentum_change = mass * (step.velocity_metres_per_second + 3.0);
        assert!(
            (momentum_change - step.normal_contact_impulse_newton_seconds).abs() < 1e-9,
            "momentum change {momentum_change} != impulse {}",
            step.normal_contact_impulse_newton_seconds
        );
        assert!(step.normal_contact_impulse_newton_seconds > 0.0);
    }
}
