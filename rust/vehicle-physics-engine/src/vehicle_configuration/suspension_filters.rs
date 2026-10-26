//! Suspension numerical-filter tuning, owned by the `suspension_filters` domain.
//!
//! These values are deliberately *not* physical suspension elements: they only
//! shape the solver's numerical conditioning (road-height slew limit and the
//! optional 1 ms filters of the last phase). Physical coefficients — bump-stop
//! law, damper knee blend, bump-stop damper, ARB damper — live in
//! `suspension.json` under `geometry_physical`, because they travel with the
//! setup. No value is duplicated across the two documents.
//!
//! Everything is optional: an absent document (or an absent field) preserves the
//! profile's historical behaviour, so old profiles load unchanged.
//!
//! The 1 ms ceiling on force-path filters is enforced here, not just documented:
//! filtering above 1 ms on a force adds 14° of phase at the 20 Hz wheel-hop mode
//! and can create a limit cycle (see the 2026-10 tire `relaxation` regression).

use serde::{Deserialize, Serialize};

use crate::types::WheelIndex;

/// Largest time constant accepted on a force-path (post-solver) signal.
pub const MAX_FORCE_FILTER_TAU_S: f64 = 0.001;

/// Per-axle numerical filters. `None` disables the filter.
#[derive(Debug, Clone, Copy, Default, PartialEq, Serialize, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct SuspensionAxleFilterConfiguration {
    /// Low-pass on the damper shaft velocity (s), `<= 1 ms`.
    pub damper_velocity_filter_tau_s: Option<f64>,
    /// Low-pass on the anti-roll relative travel (s), `<= 1 ms`.
    pub arb_travel_filter_tau_s: Option<f64>,
}

impl SuspensionAxleFilterConfiguration {
    pub fn damper_velocity_tau_s(&self) -> f64 {
        self.damper_velocity_filter_tau_s.unwrap_or(0.0).max(0.0)
    }

    pub fn arb_travel_tau_s(&self) -> f64 {
        self.arb_travel_filter_tau_s.unwrap_or(0.0).max(0.0)
    }

    fn validate(&self, axle: &str) -> Result<(), String> {
        validate_range(
            &format!("{axle}.damper_velocity_filter_tau_s"),
            self.damper_velocity_filter_tau_s,
            0.0,
            MAX_FORCE_FILTER_TAU_S,
        )?;
        validate_range(
            &format!("{axle}.arb_travel_filter_tau_s"),
            self.arb_travel_filter_tau_s,
            0.0,
            MAX_FORCE_FILTER_TAU_S,
        )
    }
}

/// Road-height limiter/filter (global: the contact target is per wheel and every
/// wheel uses the same numerical contract) plus the per-axle force-path filters.
#[derive(Debug, Clone, Copy, Default, PartialEq, Serialize, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct SuspensionFilterConfiguration {
    /// Slew limit on the contact-height target (m/s). `None` or `0` keeps the
    /// historical unlimited target. Recommended: 15 m/s.
    pub road_height_max_rate_m_s: Option<f64>,
    /// Optional low-pass on the contact-height target (s), `<= 10 ms`; 2-4 ms is
    /// the documented band. `None` or `0` disables it (default).
    pub road_height_filter_tau_s: Option<f64>,
    pub front: SuspensionAxleFilterConfiguration,
    pub rear: SuspensionAxleFilterConfiguration,
}

impl SuspensionFilterConfiguration {
    /// Slew limit in m/s; `0` means "no limit" (historical behaviour).
    pub fn road_height_max_rate_m_s(&self) -> f64 {
        self.road_height_max_rate_m_s.unwrap_or(0.0).max(0.0)
    }

    /// Contact-target low-pass time constant in seconds; `0` disables it.
    pub fn road_height_filter_tau_s(&self) -> f64 {
        self.road_height_filter_tau_s.unwrap_or(0.0).max(0.0)
    }

    pub fn for_wheel(&self, wheel: WheelIndex) -> &SuspensionAxleFilterConfiguration {
        if wheel.is_front() {
            &self.front
        } else {
            &self.rear
        }
    }

    pub fn validate(&self) -> Result<(), String> {
        validate_range(
            "road_height_max_rate_m_s",
            self.road_height_max_rate_m_s,
            0.0,
            100.0,
        )?;
        validate_range(
            "road_height_filter_tau_s",
            self.road_height_filter_tau_s,
            0.0,
            0.010,
        )?;
        self.front.validate("front")?;
        self.rear.validate("rear")?;
        Ok(())
    }
}

fn validate_range(
    name: &str,
    value: Option<f64>,
    minimum: f64,
    maximum: f64,
) -> Result<(), String> {
    let Some(value) = value else {
        return Ok(());
    };
    if !value.is_finite() || value < minimum || value > maximum {
        return Err(format!(
            "suspension_filters.{name} must be finite and in [{minimum}, {maximum}] (got {value})"
        ));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn force_filters_above_one_millisecond_are_rejected() {
        let filters = SuspensionFilterConfiguration {
            front: SuspensionAxleFilterConfiguration {
                damper_velocity_filter_tau_s: Some(0.002),
                ..SuspensionAxleFilterConfiguration::default()
            },
            ..SuspensionFilterConfiguration::default()
        };
        let error = filters.validate().expect_err("2 ms on a force path");
        assert!(error.contains("damper_velocity_filter_tau_s"), "{error}");
    }

    #[test]
    fn road_height_filter_allows_the_documented_band() {
        let filters = SuspensionFilterConfiguration {
            road_height_max_rate_m_s: Some(15.0),
            road_height_filter_tau_s: Some(0.003),
            ..SuspensionFilterConfiguration::default()
        };
        filters.validate().expect("3 ms input filter is allowed");
        assert_eq!(filters.road_height_max_rate_m_s(), 15.0);
        assert_eq!(filters.road_height_filter_tau_s(), 0.003);
    }

    #[test]
    fn absent_document_disables_everything() {
        let filters = SuspensionFilterConfiguration::default();
        filters.validate().expect("defaults are valid");
        assert_eq!(filters.road_height_max_rate_m_s(), 0.0);
        assert_eq!(filters.road_height_filter_tau_s(), 0.0);
        assert_eq!(filters.front.damper_velocity_tau_s(), 0.0);
    }
}
