//! Tire transient-filter tuning, owned by the `tire_filters` profile domain.
//!
//! These values are intentionally outside the steady-state tire model: they only
//! shape how fast the wheel state reaches the steady target (contact ramp, sliding
//! memory, relaxation, combined-slip peak migration and surface transitions). The
//! profile document is read at runtime, so changing it does not require rebuilding
//! the physics library.

use serde::{Deserialize, Serialize};

use crate::types::WheelIndex;
use crate::vehicle_config::TireForceProfile;

/// Optional per-axle overrides. `None` keeps the value already present in the
/// steady-state tire profile (`tires.json`), so legacy profiles remain valid.
#[derive(Debug, Clone, Copy, Default, PartialEq, Serialize, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct TireAxleFilterConfiguration {
    pub contact_ramp_tau_s: Option<f64>,
    pub slip_loss_tau_s: Option<f64>,
    pub slip_recovery_tau_s: Option<f64>,
    pub min_relaxation_tau_s: Option<f64>,
    pub relaxation_recovery_scale: Option<f64>,
    pub combined_lateral_peak_migration: Option<f64>,
    pub combined_longitudinal_peak_migration: Option<f64>,
}

/// `surface_transition_tau_s` is global because the suspension surface filter is
/// per wheel and not per axle. The rest are per axle to match `TireForceProfile`.
#[derive(Debug, Clone, Copy, Default, PartialEq, Serialize, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct TireFilterConfiguration {
    pub surface_transition_tau_s: Option<f64>,
    pub front: TireAxleFilterConfiguration,
    pub rear: TireAxleFilterConfiguration,
}

impl TireFilterConfiguration {
    pub fn for_wheel(&self, wheel: WheelIndex) -> &TireAxleFilterConfiguration {
        if wheel.is_front() {
            &self.front
        } else {
            &self.rear
        }
    }

    pub fn validate(&self) -> Result<(), String> {
        validate_tau("surface_transition_tau_s", self.surface_transition_tau_s, 1.0)?;
        self.front.validate("front")?;
        self.rear.validate("rear")?;
        Ok(())
    }
}

impl TireAxleFilterConfiguration {
    fn validate(&self, axle: &str) -> Result<(), String> {
        validate_tau(&format!("{axle}.contact_ramp_tau_s"), self.contact_ramp_tau_s, 1.0)?;
        validate_tau(&format!("{axle}.slip_loss_tau_s"), self.slip_loss_tau_s, 1.0)?;
        validate_tau(&format!("{axle}.slip_recovery_tau_s"), self.slip_recovery_tau_s, 1.0)?;
        validate_tau(
            &format!("{axle}.min_relaxation_tau_s"),
            self.min_relaxation_tau_s,
            0.5,
        )?;
        validate_range(
            &format!("{axle}.relaxation_recovery_scale"),
            self.relaxation_recovery_scale,
            0.1,
            5.0,
        )?;
        validate_range(
            &format!("{axle}.combined_lateral_peak_migration"),
            self.combined_lateral_peak_migration,
            0.0,
            0.9,
        )?;
        validate_range(
            &format!("{axle}.combined_longitudinal_peak_migration"),
            self.combined_longitudinal_peak_migration,
            0.0,
            0.9,
        )?;
        Ok(())
    }

    /// Override the steady-state profile with the transient-filter values. Only
    /// explicitly authored fields are touched; absent fields keep the profile.
    pub fn apply_to_profile(&self, profile: &mut TireForceProfile) {
        if let Some(value) = self.contact_ramp_tau_s {
            profile.contact_ramp_tau_s = value;
        }
        if let Some(value) = self.slip_loss_tau_s {
            profile.slip_loss_tau_s = value;
        }
        if let Some(value) = self.slip_recovery_tau_s {
            profile.slip_recovery_tau_s = value;
        }
        if let Some(value) = self.min_relaxation_tau_s {
            profile.min_relaxation_tau_s = value;
        }
        if let Some(value) = self.relaxation_recovery_scale {
            profile.relaxation_recovery_scale = value;
        }
        if let Some(value) = self.combined_lateral_peak_migration {
            profile.combined_lateral_peak_migration = value;
        }
        if let Some(value) = self.combined_longitudinal_peak_migration {
            profile.combined_longitudinal_peak_migration = value;
        }
    }
}

fn validate_tau(name: &str, value: Option<f64>, maximum: f64) -> Result<(), String> {
    validate_range(name, value, 0.0, maximum)
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
            "tire_filters.{name} must be finite and in [{minimum}, {maximum}] (got {value})"
        ));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn partial_overrides_leave_unspecified_profile_fields_untouched() {
        let mut profile = TireForceProfile::default();
        profile.slip_recovery_tau_s = 0.25;
        let filters = TireAxleFilterConfiguration {
            contact_ramp_tau_s: Some(0.06),
            ..TireAxleFilterConfiguration::default()
        };
        filters.apply_to_profile(&mut profile);
        assert_eq!(profile.contact_ramp_tau_s, 0.06);
        assert_eq!(profile.slip_recovery_tau_s, 0.25);
    }

    #[test]
    fn validation_rejects_out_of_range_values() {
        let filters = TireFilterConfiguration {
            front: TireAxleFilterConfiguration {
                relaxation_recovery_scale: Some(0.0),
                ..TireAxleFilterConfiguration::default()
            },
            ..TireFilterConfiguration::default()
        };
        assert!(filters.validate().is_err());
    }
}
