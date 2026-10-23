//! Setup authority for the simplified geometric suspension.
//!
//! The garage exposes per-axle trims (`suspension.<axle>.damping_ratio`,
//! `bump_damp_multiplier`, `rebound_damp_multiplier`, `bump_stop_multiplier`) while
//! the active axle solver evaluates the geometric elements directly
//! (`suspension.geometry_physical.<axle>.damper_bump_Ns_per_m`,
//! `damper_rebound_Ns_per_m`, `bump_stop_mult`). Those two representations describe
//! the same hardware, so a setup transaction must translate one into the other;
//! before this module the geometric coefficients were consumed untouched and the
//! damper controls could not change the running car.
//!
//! Authority, in exactly one place:
//! * [`SuspensionSetupAuthority::capture`] snapshots the design coefficients and the
//!   design trim values from the profile the simulation was created with.
//! * [`SuspensionSetupAuthority::apply`] re-derives the effective coefficients
//!   *absolutely* from that snapshot. Applying a reglaje once, twice or after any
//!   number of edits therefore produces the same numbers: a profile is never
//!   multiplied against itself.
//! * At the design trim values every factor is exactly `1.0`, so an untouched profile
//!   keeps the shipped response bit for bit.
//!
//! Translation, per axle and in the damper's own coordinate:
//! * `damper_bump_Ns_per_m    = design * (damping_ratio / design) * (bump / design)`
//! * `damper_rebound_Ns_per_m = design * (damping_ratio / design) * (rebound / design)`
//! * `bump_stop_mult          = design * (bump_stop / design)`
//!
//! `damping_ratio` is the axle-wide damper trim (it scales both sides, matching the
//! legacy one-DOF law where it multiplies the critical-damping base), while `bump`
//! and `rebound` bias one side of the digressive curve. The coefficients stay in the
//! damper coordinate; the axle solver maps them to the wheel through the motion ratio,
//! exactly as it does for the design values.
//!
//! The anti-roll bar needs no ratio translation: `suspension.<axle>.arb_ratio` is the
//! flat value the solvers consume and
//! `suspension.geometry_physical.<axle>_arb.ratio` is the knob the garage writes, both
//! the same `LegacyRatio` quantity. [`adopt_antiroll_design_bar`] makes a loaded
//! profile state one bar, [`adopt_antiroll_knob`] then lets that knob govern the
//! running car. Each is an absolute copy of a single number, never a product, so no
//! sequence of transactions can move the bar twice.
//!
//! Tests for this authority live in `tests/setup_authority.rs`: the `cargo test --lib`
//! target of this crate does not compile in this workspace at `5f5f04d` because
//! unrelated modules still `include_str!` donor paths that are not in the tree (for
//! example `data/vehicles/f1_2030/f1_2030_v10_geometric.json`), while integration
//! targets build against the library normally.

#![allow(non_snake_case)]

use crate::suspension_geo_config::AntiRollConfig;
use crate::types::WheelIndex;
use crate::vehicle_config::VehicleConfig;

/// One axle's design elements plus the trim values that produced them.
#[derive(Debug, Clone, Copy, PartialEq)]
struct AxleSetupAuthority {
    design_damper_bump_Ns_per_m: f64,
    design_damper_rebound_Ns_per_m: f64,
    design_bump_stop_mult: f64,
    design_damping_ratio: f64,
    design_bump_multiplier: f64,
    design_rebound_multiplier: f64,
    design_bump_stop_multiplier: f64,
}

/// The garage trims requested by a setup, before translation.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct AxleSetupTrim {
    pub damping_ratio: f64,
    pub bump_multiplier: f64,
    pub rebound_multiplier: f64,
    pub bump_stop_multiplier: f64,
}

impl AxleSetupTrim {
    /// The requested trims of one axle straight out of a configuration.
    pub fn of_axle(config: &VehicleConfig, front: bool) -> Self {
        let suspension = &config.suspension;
        if front {
            Self {
                damping_ratio: suspension.front_damping_ratio,
                bump_multiplier: suspension.front_bump_damp_multiplier,
                rebound_multiplier: suspension.front_rebound_damp_multiplier,
                bump_stop_multiplier: suspension.front_bump_stop_multiplier,
            }
        } else {
            Self {
                damping_ratio: suspension.rear_damping_ratio,
                bump_multiplier: suspension.rear_bump_damp_multiplier,
                rebound_multiplier: suspension.rear_rebound_damp_multiplier,
                bump_stop_multiplier: suspension.rear_bump_stop_multiplier,
            }
        }
    }

    fn is_usable(&self) -> bool {
        self.damping_ratio.is_finite()
            && self.bump_multiplier.is_finite()
            && self.rebound_multiplier.is_finite()
            && self.bump_stop_multiplier.is_finite()
    }
}

/// The coefficients one setup resolves to, in the damper's own coordinate.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct AxleEffectiveElements {
    pub damper_bump_Ns_per_m: f64,
    pub damper_rebound_Ns_per_m: f64,
    pub bump_stop_mult: f64,
}

/// Snapshot of the design suspension of one profile.
#[derive(Debug, Clone, Copy)]
pub struct SuspensionSetupAuthority {
    front: AxleSetupAuthority,
    rear: AxleSetupAuthority,
}

fn design_axle_authority(config: &VehicleConfig, front: bool) -> Option<AxleSetupAuthority> {
    let elements = config
        .suspension
        .geometric_suspension
        .as_ref()?
        .axle(if front {
            WheelIndex::FrontLeft
        } else {
            WheelIndex::RearLeft
        });
    let trim = AxleSetupTrim::of_axle(config, front);
    let authority = AxleSetupAuthority {
        design_damper_bump_Ns_per_m: elements.damper_bump_Ns_per_m,
        design_damper_rebound_Ns_per_m: elements.damper_rebound_Ns_per_m,
        design_bump_stop_mult: elements.bump_stop_mult,
        design_damping_ratio: trim.damping_ratio,
        design_bump_multiplier: trim.bump_multiplier,
        design_rebound_multiplier: trim.rebound_multiplier,
        design_bump_stop_multiplier: trim.bump_stop_multiplier,
    };
    // Every denominator must be a usable design value. A profile that does not declare
    // one keeps the coefficients it authored instead of inventing a reference.
    let denominators_usable = [
        authority.design_damping_ratio,
        authority.design_bump_multiplier,
        authority.design_rebound_multiplier,
        authority.design_bump_stop_multiplier,
    ]
    .into_iter()
    .all(|value| value.is_finite() && value > 0.0);
    let design_coefficients_usable = [
        authority.design_damper_bump_Ns_per_m,
        authority.design_damper_rebound_Ns_per_m,
        authority.design_bump_stop_mult,
    ]
    .into_iter()
    .all(|value| value.is_finite() && value >= 0.0);
    if denominators_usable && design_coefficients_usable && trim.is_usable() {
        Some(authority)
    } else {
        None
    }
}

impl SuspensionSetupAuthority {
    /// Snapshot the design elements and design trims of a freshly loaded profile.
    /// `None` means the profile cannot be translated (no geometric elements, or a
    /// design trim of zero), in which case the setup layer leaves the authored
    /// coefficients untouched instead of inventing a reference.
    pub fn capture(config: &VehicleConfig) -> Option<Self> {
        Some(Self {
            front: design_axle_authority(config, true)?,
            rear: design_axle_authority(config, false)?,
        })
    }

    fn scale(design: f64, current: f64) -> Result<f64, String> {
        let factor = current / design;
        if !factor.is_finite() || factor < 0.0 {
            return Err(format!(
                "the requested suspension trim resolves to a non-physical scale ({design} -> {current})"
            ));
        }
        Ok(factor)
    }

    fn axle_factors(
        authority: &AxleSetupAuthority,
        trim: &AxleSetupTrim,
    ) -> Result<(f64, f64, f64), String> {
        let damping = Self::scale(authority.design_damping_ratio, trim.damping_ratio)?;
        Ok((
            damping * Self::scale(authority.design_bump_multiplier, trim.bump_multiplier)?,
            damping * Self::scale(authority.design_rebound_multiplier, trim.rebound_multiplier)?,
            Self::scale(authority.design_bump_stop_multiplier, trim.bump_stop_multiplier)?,
        ))
    }

    /// Write the effective geometric coefficients for the trims currently held by
    /// `config`. Absolute, idempotent assignment derived from the design snapshot.
    pub fn apply(&self, config: &mut VehicleConfig) -> Result<(), String> {
        let front_trim = AxleSetupTrim::of_axle(config, true);
        let rear_trim = AxleSetupTrim::of_axle(config, false);
        if !front_trim.is_usable() || !rear_trim.is_usable() {
            return Err("the setup carries a non-finite suspension trim".into());
        }
        let (front_bump, front_rebound, front_stop) = Self::axle_factors(&self.front, &front_trim)?;
        let (rear_bump, rear_rebound, rear_stop) = Self::axle_factors(&self.rear, &rear_trim)?;
        let geometry = config
            .suspension
            .geometric_suspension
            .as_ref()
            .ok_or("the simplified backend requires geometric suspension elements")?;
        // Translate copies first, then validate before committing: a rejected setup must
        // never leave the model half-translated.
        let mut translated = geometry.clone();
        translated.front.damper_bump_Ns_per_m = self.front.design_damper_bump_Ns_per_m * front_bump;
        translated.front.damper_rebound_Ns_per_m =
            self.front.design_damper_rebound_Ns_per_m * front_rebound;
        translated.front.bump_stop_mult = self.front.design_bump_stop_mult * front_stop;
        translated.rear.damper_bump_Ns_per_m = self.rear.design_damper_bump_Ns_per_m * rear_bump;
        translated.rear.damper_rebound_Ns_per_m =
            self.rear.design_damper_rebound_Ns_per_m * rear_rebound;
        translated.rear.bump_stop_mult = self.rear.design_bump_stop_mult * rear_stop;
        translated.validate()?;
        let target = config
            .suspension
            .geometric_suspension
            .as_mut()
            .expect("the geometric elements were just read");
        *target = translated;
        Ok(())
    }

    /// The coefficients one configuration actually evaluates.
    pub fn effective_elements(
        config: &VehicleConfig,
        front: bool,
    ) -> Option<AxleEffectiveElements> {
        let elements = config
            .suspension
            .geometric_suspension
            .as_ref()?
            .axle(if front {
                WheelIndex::FrontLeft
            } else {
                WheelIndex::RearLeft
            });
        Some(AxleEffectiveElements {
            damper_bump_Ns_per_m: elements.damper_bump_Ns_per_m,
            damper_rebound_Ns_per_m: elements.damper_rebound_Ns_per_m,
            bump_stop_mult: elements.bump_stop_mult,
        })
    }
}

/// Anti-roll authority for a freshly loaded or reset profile.
///
/// `suspension.<axle>.arb_ratio` is the flat value both the simplified axle and the
/// geometric solver consume (see `SuspensionBackend::effective_antiroll_configuration`
/// and `solve_force_legacy`), while the garage knob lives in
/// `suspension.geometry_physical.<axle>_arb.ratio`. The installed profile authored a
/// raised flat bar (front 0.15 to 1.00, rear 0.09 to 0.80) on top of an inherited
/// geometric block, so on load the geometric block is made to state the flat authority
/// instead of silently carrying a second bar. A `MotionRatio` bar is an explicit bar
/// rate and is never rewritten.
pub fn adopt_antiroll_design_bar(config: &mut VehicleConfig) {
    let front_ratio = config.suspension.front_arb_ratio;
    let rear_ratio = config.suspension.rear_arb_ratio;
    let Some(geometry) = config.suspension.geometric_suspension.as_mut() else {
        return;
    };
    for (is_front, ratio) in [(true, front_ratio), (false, rear_ratio)] {
        if !ratio.is_finite() {
            continue;
        }
        // An explicit `MotionRatio` bar is a different contract and stays authored.
        let Some(authored) = geometry.arb(is_front).legacy_ratio() else {
            continue;
        };
        if authored == ratio {
            continue;
        }
        let replacement = AntiRollConfig::LegacyRatio { ratio };
        if is_front {
            geometry.front_arb = replacement;
        } else {
            geometry.rear_arb = replacement;
        }
    }
}

/// Anti-roll authority for a setup transaction.
///
/// Once a profile is loaded its knob holds the effective bar, so from then on the knob
/// is the authority the driver moves: the value the geometric block states becomes the
/// value the axles consume. This is an absolute copy of one number, never a product, so
/// replaying the same reglaje any number of times cannot stiffen or soften the bar
/// again. A `MotionRatio` bar keeps its explicit rate.
pub fn adopt_antiroll_knob(config: &mut VehicleConfig) {
    let Some(geometry) = config.suspension.geometric_suspension.as_ref() else {
        return;
    };
    for (is_front, authored) in [
        (true, geometry.front_arb.legacy_ratio()),
        (false, geometry.rear_arb.legacy_ratio()),
    ] {
        let Some(ratio) = authored else { continue };
        if !ratio.is_finite() {
            continue;
        }
        if is_front {
            config.suspension.front_arb_ratio = ratio;
        } else {
            config.suspension.rear_arb_ratio = ratio;
        }
    }
}
