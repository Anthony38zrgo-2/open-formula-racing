//! Setup authority: every garage control must move the element the active simplified
//! axle evaluates, nothing else, and never twice.
//!
//! These run through the host-facing surface (`OpenFormulaSimulation`, the same one
//! the `rustphysics` bridge drives over ABI v6), because the defect being guarded
//! against was a connection between that surface and the solver, not a JSON read.
//!
//! NOTE: the crate's `cargo test --lib` target does not compile in this workspace at
//! `5f5f04d` — unrelated modules still `include_str!` donor paths that are not in the
//! tree (`data/vehicles/f1_2030/f1_2030_v10_geometric.json`,
//! `data/race_intelligence/automatic_transmission_tuning.json`) and reference
//! `f1_2030_geometric_canonical`. Integration targets build against the library
//! normally, so this suite runs today:
//! `cargo test --offline -p vehicle_physics_engine --test setup_authority`.

use serde_json::Value;
use vehicle_physics_engine::open_formula::OpenFormulaSimulation;
use vehicle_physics_engine::suspension_geo_config::AntiRollConfig;
use vehicle_physics_engine::suspension_setup::{
    adopt_antiroll_design_bar, adopt_antiroll_knob, AxleEffectiveElements, SuspensionSetupAuthority,
};
use vehicle_physics_engine::types::Vec3;
use vehicle_physics_engine::vehicle_config::VehicleConfig;

fn simulation() -> OpenFormulaSimulation {
    // Keep the original authority regression witness independent of later tuning.
    // The current production candidate has its own physical-height/stop tests.
    let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("tests/fixtures/williams_suspension_pre_35_40.json");
    OpenFormulaSimulation::create(Some(&path), Vec3::ZERO, 0.0, None).expect("reference profile")
}

fn effective(simulation: &OpenFormulaSimulation, axle: &str, element: &str) -> f64 {
    summary(simulation)[axle][element]
        .as_f64()
        .unwrap_or_else(|| panic!("no effective {axle}.{element}"))
}

fn summary(simulation: &OpenFormulaSimulation) -> Value {
    simulation.effective_suspension_summary()
}

fn document_number(document: &Value, path: &str) -> Option<f64> {
    document
        .pointer(&format!("/{}", path.replace('.', "/")))
        .and_then(Value::as_f64)
}

/// The published setup document: live authoring state plus the untouched base.
fn documents(simulation: &OpenFormulaSimulation) -> (Value, Value) {
    let published: Value =
        serde_json::from_str(&simulation.setup_document_json()).expect("setup document json");
    (
        published["document"].clone(),
        published["base"].clone(),
    )
}

fn design_trims(simulation: &OpenFormulaSimulation, front: bool) -> [f64; 4] {
    let axle = if front { "front" } else { "rear" };
    let summary = summary(simulation);
    [
        summary[axle]["damping_ratio"].as_f64().expect("damping"),
        summary[axle]["bump_damp_multiplier"].as_f64().expect("bump"),
        summary[axle]["rebound_damp_multiplier"].as_f64().expect("rebound"),
        summary[axle]["bump_stop_multiplier"].as_f64().expect("stop"),
    ]
}

fn patch(path: &str, value: f64) -> String {
    format!(r#"{{"{path}": {value}}}"#)
}

/// The controls the Melbourne regraje uses. Their dotted paths are also the storage
/// keys of every already-saved reglaje, so renaming one orphans that regraje.
const GARAGE_SUSPENSION_CONTROLS: &[&str] = &[
    "suspension.front.damping_ratio",
    "suspension.rear.damping_ratio",
    "suspension.front.bump_damp_multiplier",
    "suspension.rear.bump_damp_multiplier",
    "suspension.front.rebound_damp_multiplier",
    "suspension.rear.rebound_damp_multiplier",
    "suspension.front.bump_stop_multiplier",
    "suspension.rear.bump_stop_multiplier",
    "suspension.geometry_physical.front_arb.ratio",
    "suspension.geometry_physical.rear_arb.ratio",
];

#[test]
fn the_garage_controls_stay_the_stored_paths_and_stay_editable() {
    let simulation = simulation();
    let published: Value =
        serde_json::from_str(&simulation.setup_document_json()).expect("setup document json");
    for path in GARAGE_SUSPENSION_CONTROLS {
        let field = published["fields"]
            .as_array()
            .expect("descriptor fields")
            .iter()
            .find(|field| field["path"].as_str() == Some(path))
            .unwrap_or_else(|| panic!("{path} left the descriptor: existing reglajes would be rejected"));
        assert_eq!(
            field["editable"].as_bool(),
            Some(true),
            "{path} must stay editable"
        );
    }
}

#[test]
fn the_design_profile_keeps_its_authored_elements() {
    let simulation = simulation();
    let (live, base) = documents(&simulation);
    // The installed profile authored a raised flat bar (front 1.00, rear 0.80) on top
    // of the inherited geometric block (0.15, 0.09). The geometric coefficients are
    // the design values, untouched.
    assert_eq!(effective(&simulation, "front", "damper_bump_Ns_per_m"), 6_970.0);
    assert_eq!(
        effective(&simulation, "front", "damper_rebound_Ns_per_m"),
        11_475.0
    );
    assert_eq!(effective(&simulation, "front", "effective_bump_stop_mult"), 2.6);
    assert_eq!(effective(&simulation, "rear", "damper_bump_Ns_per_m"), 14_000.0);
    assert_eq!(effective(&simulation, "rear", "damper_rebound_Ns_per_m"), 22_000.0);
    assert_eq!(effective(&simulation, "rear", "effective_bump_stop_mult"), 2.8);

    // One anti-roll authority: both representations state the effective bar.
    assert_eq!(effective(&simulation, "front", "arb_ratio"), 1.0);
    assert_eq!(effective(&simulation, "front", "arb_geometric_ratio"), 1.0);
    assert_eq!(effective(&simulation, "rear", "arb_ratio"), 0.8);
    assert_eq!(effective(&simulation, "rear", "arb_geometric_ratio"), 0.8);
    // The knob the garage reads is that bar, so the driver is not dragging a stale
    // number, while the base profile stays exactly as shipped (its hash is what a
    // stored reglaje is validated against).
    assert_eq!(
        document_number(&live, "suspension.geometry_physical.front_arb.ratio"),
        Some(1.0)
    );
    assert_eq!(
        document_number(&live, "suspension.geometry_physical.rear_arb.ratio"),
        Some(0.8)
    );
    assert_eq!(
        document_number(&base, "suspension.geometry_physical.front_arb.ratio"),
        Some(0.15)
    );
    assert_eq!(
        document_number(&base, "suspension.geometry_physical.rear_arb.ratio"),
        Some(0.09)
    );
    assert_eq!(document_number(&base, "suspension.front.arb_ratio"), Some(1.0));
}

#[test]
fn the_rebound_knob_moves_its_own_axle_and_nothing_else() {
    let mut simulation = simulation();
    let design = design_trims(&simulation, true);
    simulation
        .apply_setup_json(&patch(
            "suspension.front.rebound_damp_multiplier",
            design[2] * 2.0,
        ))
        .expect("rebound transaction");
    assert_eq!(
        effective(&simulation, "front", "damper_rebound_Ns_per_m"),
        11_475.0 * 2.0
    );
    assert_eq!(effective(&simulation, "front", "damper_bump_Ns_per_m"), 6_970.0);
    assert_eq!(effective(&simulation, "front", "effective_bump_stop_mult"), 2.6);
    assert_eq!(effective(&simulation, "rear", "damper_rebound_Ns_per_m"), 22_000.0);
    assert_eq!(effective(&simulation, "rear", "damper_bump_Ns_per_m"), 14_000.0);

    // The garage reads back what it asked for.
    let expected = (design[2] * 2.0).to_string();
    let fields = simulation.setup_fields_text();
    assert!(fields.lines().any(|line| {
        let columns: Vec<&str> = line.split('\t').collect();
        columns.first() == Some(&"suspension.front.rebound_damp_multiplier")
            && columns.get(3) == Some(&expected.as_str())
    }));
}

#[test]
fn the_bump_and_damping_knobs_reach_the_active_damper() {
    let mut simulation = simulation();
    let design = design_trims(&simulation, false);
    simulation
        .apply_setup_json(&patch(
            "suspension.rear.bump_damp_multiplier",
            design[1] * 0.5,
        ))
        .expect("bump transaction");
    assert_eq!(effective(&simulation, "rear", "damper_bump_Ns_per_m"), 14_000.0 * 0.5);
    assert_eq!(effective(&simulation, "rear", "damper_rebound_Ns_per_m"), 22_000.0);
    assert_eq!(effective(&simulation, "front", "damper_bump_Ns_per_m"), 6_970.0);

    // The axle-wide damper trim scales both sides of that axle only, on top of the
    // bump edit already in the document.
    simulation
        .apply_setup_json(&patch("suspension.rear.damping_ratio", design[0] * 0.5))
        .expect("damping transaction");
    assert_eq!(effective(&simulation, "rear", "damper_bump_Ns_per_m"), 14_000.0 * 0.25);
    assert_eq!(
        effective(&simulation, "rear", "damper_rebound_Ns_per_m"),
        22_000.0 * 0.5
    );
    assert_eq!(
        effective(&simulation, "front", "damper_rebound_Ns_per_m"),
        11_475.0
    );
}

#[test]
fn the_bump_stop_knob_reaches_the_geometric_stops() {
    let mut simulation = simulation();
    let design = design_trims(&simulation, true);
    simulation
        .apply_setup_json(&patch(
            "suspension.front.bump_stop_multiplier",
            design[3] * 0.25,
        ))
        .expect("bump stop transaction");
    assert_eq!(effective(&simulation, "front", "effective_bump_stop_mult"), 2.6 * 0.25);
    assert_eq!(effective(&simulation, "rear", "effective_bump_stop_mult"), 2.8);
    assert_eq!(
        effective(&simulation, "front", "damper_rebound_Ns_per_m"),
        11_475.0
    );
}

#[test]
fn the_antiroll_knob_is_the_effective_bar() {
    let mut simulation = simulation();
    simulation
        .apply_setup_json(&patch("suspension.geometry_physical.front_arb.ratio", 0.2))
        .expect("front bar transaction");
    assert_eq!(effective(&simulation, "front", "arb_ratio"), 0.2);
    assert_eq!(effective(&simulation, "front", "arb_geometric_ratio"), 0.2);
    assert_eq!(effective(&simulation, "rear", "arb_ratio"), 0.8);

    let (live, base) = documents(&simulation);
    // The knob the driver moved is what the document states. The flat design value is
    // not an editable control, so a transaction never rewrites it in storage.
    assert_eq!(
        document_number(&live, "suspension.geometry_physical.front_arb.ratio"),
        Some(0.2)
    );
    assert_eq!(document_number(&live, "suspension.front.arb_ratio"), Some(1.0));
    // The base profile is untouched, so an already-saved reglaje still matches it.
    assert_eq!(document_number(&base, "suspension.front.arb_ratio"), Some(1.0));
    assert_eq!(
        document_number(&base, "suspension.geometry_physical.front_arb.ratio"),
        Some(0.15)
    );
}

#[test]
fn replaying_a_setup_never_multiplies_the_profile_again() {
    let mut simulation = simulation();
    let design = design_trims(&simulation, true);
    let transaction = format!(
        r#"{{"suspension.front.rebound_damp_multiplier": {},
             "suspension.front.damping_ratio": {},
             "suspension.geometry_physical.front_arb.ratio": 0.4}}"#,
        design[2] * 1.5,
        design[0] * 1.25,
    );
    simulation
        .apply_setup_json(&transaction)
        .expect("first transaction");
    let first_rebound = effective(&simulation, "front", "damper_rebound_Ns_per_m");
    let first_bump = effective(&simulation, "front", "damper_bump_Ns_per_m");
    let first_bar = effective(&simulation, "front", "arb_ratio");
    for round in 1..=3 {
        simulation
            .apply_setup_json(&transaction)
            .unwrap_or_else(|error| panic!("transaction {round} rejected: {error}"));
        assert_eq!(
            effective(&simulation, "front", "damper_rebound_Ns_per_m"),
            first_rebound
        );
        assert_eq!(
            effective(&simulation, "front", "damper_bump_Ns_per_m"),
            first_bump
        );
        assert_eq!(effective(&simulation, "front", "arb_ratio"), first_bar);
    }
    // 1.25 of the design damping times 1.5 of the design rebound, not 1.25^4·1.5^4.
    assert!((first_rebound / 11_475.0 - 1.875).abs() < 1e-9);

    // Reset returns the design elements, not a re-multiplied copy of them.
    simulation.reset_setup().expect("reset setup");
    assert_eq!(
        effective(&simulation, "front", "damper_rebound_Ns_per_m"),
        11_475.0
    );
    assert_eq!(effective(&simulation, "front", "damper_bump_Ns_per_m"), 6_970.0);
    assert_eq!(effective(&simulation, "front", "arb_ratio"), 1.0);
    let (live, _) = documents(&simulation);
    assert_eq!(
        document_number(&live, "suspension.geometry_physical.front_arb.ratio"),
        Some(1.0)
    );
}

#[test]
fn a_rejected_transaction_leaves_the_running_elements_untouched() {
    let mut simulation = simulation();
    let before = summary(&simulation);
    let (live_before, base_before) = documents(&simulation);
    assert!(simulation
        .apply_setup_json(r#"{"suspension.front.rebound_damp_multiplier": 99.0}"#)
        .is_err());
    assert_eq!(
        summary(&simulation)["front"]["damper_rebound_Ns_per_m"],
        before["front"]["damper_rebound_Ns_per_m"]
    );
    // A physical coefficient is not a back door: only the published controls are open.
    assert!(simulation
        .apply_setup_json(r#"{"suspension.geometry_physical.front.damper_rebound_Ns_per_m": 1.0}"#)
        .is_err());
    assert_eq!(summary(&simulation), before);
    let (live_after, base_after) = documents(&simulation);
    assert_eq!(live_after, live_before, "a rejected patch mutated the car");
    assert_eq!(base_after, base_before, "a rejected patch mutated the profile");
}

/// The same authority one level down: the translation a transaction runs, checked
/// against the design profile with no host in the loop.
mod translation {
    use super::*;

    fn design_profile() -> VehicleConfig {
        VehicleConfig::from_json_str(include_str!("fixtures/williams_suspension_pre_35_40.json"))
            .expect("reference authority profile")
    }

    fn coefficients(config: &VehicleConfig) -> (AxleEffectiveElements, AxleEffectiveElements) {
        (
            SuspensionSetupAuthority::effective_elements(config, true).expect("front elements"),
            SuspensionSetupAuthority::effective_elements(config, false).expect("rear elements"),
        )
    }

    fn front_elements(config: &VehicleConfig) -> AxleEffectiveElements {
        coefficients(config).0
    }

    #[test]
    fn the_design_setup_resolves_to_the_authored_coefficients() {
        let design = design_profile();
        let authority = SuspensionSetupAuthority::capture(&design).expect("design authority");
        let authored = coefficients(&design);
        let mut config = design.clone();
        authority.apply(&mut config).expect("design trims translate");
        assert_eq!(coefficients(&config), authored);
    }

    #[test]
    fn each_trim_moves_only_its_own_element() {
        let design = design_profile();
        let authority = SuspensionSetupAuthority::capture(&design).expect("design authority");
        let (front_authored, rear_authored) = coefficients(&design);

        let mut rebound = design.clone();
        rebound.suspension.front_rebound_damp_multiplier *= 2.0;
        authority.apply(&mut rebound).expect("rebound trim");
        let (front, rear) = coefficients(&rebound);
        assert_eq!(
            front.damper_rebound_Ns_per_m,
            front_authored.damper_rebound_Ns_per_m * 2.0
        );
        assert_eq!(front.damper_bump_Ns_per_m, front_authored.damper_bump_Ns_per_m);
        assert_eq!(front.bump_stop_mult, front_authored.bump_stop_mult);
        assert_eq!(rear, rear_authored);

        let mut bump = design.clone();
        bump.suspension.front_bump_damp_multiplier *= 0.5;
        authority.apply(&mut bump).expect("bump trim");
        let (front, rear) = coefficients(&bump);
        assert_eq!(
            front.damper_bump_Ns_per_m,
            front_authored.damper_bump_Ns_per_m * 0.5
        );
        assert_eq!(
            front.damper_rebound_Ns_per_m,
            front_authored.damper_rebound_Ns_per_m
        );
        assert_eq!(rear, rear_authored);

        let mut stops = design.clone();
        stops.suspension.rear_bump_stop_multiplier *= 3.0;
        authority.apply(&mut stops).expect("bump stop trim");
        let (front, rear) = coefficients(&stops);
        assert_eq!(rear.bump_stop_mult, rear_authored.bump_stop_mult * 3.0);
        assert_eq!(rear.damper_bump_Ns_per_m, rear_authored.damper_bump_Ns_per_m);
        assert_eq!(
            rear.damper_rebound_Ns_per_m,
            rear_authored.damper_rebound_Ns_per_m
        );
        assert_eq!(front, front_authored);

        // The axle-wide damper trim scales both sides of its own axle and nothing else.
        let mut damped = design.clone();
        damped.suspension.rear_damping_ratio *= 0.25;
        authority.apply(&mut damped).expect("damper trim");
        let (front, rear) = coefficients(&damped);
        assert_eq!(
            rear.damper_bump_Ns_per_m,
            rear_authored.damper_bump_Ns_per_m * 0.25
        );
        assert_eq!(
            rear.damper_rebound_Ns_per_m,
            rear_authored.damper_rebound_Ns_per_m * 0.25
        );
        assert_eq!(rear.bump_stop_mult, rear_authored.bump_stop_mult);
        assert_eq!(front, front_authored);
    }

    #[test]
    fn the_translation_never_compounds() {
        let design = design_profile();
        let authority = SuspensionSetupAuthority::capture(&design).expect("design authority");
        let mut config = design.clone();
        config.suspension.front_rebound_damp_multiplier = 2.24;
        config.suspension.front_damping_ratio = 0.37;
        authority.apply(&mut config).expect("first translate");
        let first = front_elements(&config);
        authority.apply(&mut config).expect("second translate");
        authority.apply(&mut config).expect("third translate");
        assert_eq!(first, front_elements(&config));

        // Rebuilding from the design profile and translating once lands on the same
        // numbers, so no reset path can drift.
        let mut fresh = design_profile();
        fresh.suspension.front_rebound_damp_multiplier = 2.24;
        fresh.suspension.front_damping_ratio = 0.37;
        authority.apply(&mut fresh).expect("translate after reset");
        assert_eq!(first, front_elements(&fresh));
        // Half the design damping times double the design rebound puts the rebound back
        // on its design coefficient; the bump keeps that half damping, so only the
        // rebound side returns to the authored value.
        let design_front = front_elements(&design_profile());
        assert_eq!(
            first.damper_rebound_Ns_per_m,
            design_front.damper_rebound_Ns_per_m
        );
        assert_eq!(
            first.damper_bump_Ns_per_m,
            design_front.damper_bump_Ns_per_m * 0.5
        );
    }

    #[test]
    fn a_zero_design_trim_disables_the_translation_instead_of_dividing_by_zero() {
        let mut design = design_profile();
        design.suspension.front_damping_ratio = 0.0;
        assert!(SuspensionSetupAuthority::capture(&design).is_none());
    }

    #[test]
    fn a_non_finite_requested_trim_is_rejected() {
        let design = design_profile();
        let authority = SuspensionSetupAuthority::capture(&design).expect("design authority");
        let mut nan = design.clone();
        nan.suspension.front_bump_damp_multiplier = f64::NAN;
        assert!(authority.apply(&mut nan).is_err());
        assert_eq!(
            front_elements(&nan),
            front_elements(&design),
            "a rejected translate must not touch the model"
        );
        let mut infinite = design;
        infinite.suspension.rear_damping_ratio = f64::INFINITY;
        assert!(authority.apply(&mut infinite).is_err());
    }

    #[test]
    fn one_bar_per_axle_for_both_representations() {
        let mut config = design_profile();
        assert_eq!(config.suspension.front_arb_ratio, 1.00);
        assert_eq!(config.suspension.rear_arb_ratio, 0.80);
        assert_eq!(
            config
                .suspension
                .geometric_suspension
                .as_ref()
                .expect("geometric profile")
                .front_arb
                .legacy_ratio(),
            Some(0.15)
        );
        // Profile load: the geometric block states the authored flat bar.
        adopt_antiroll_design_bar(&mut config);
        let geometry = config
            .suspension
            .geometric_suspension
            .as_ref()
            .expect("geometric profile");
        assert_eq!(geometry.front_arb.legacy_ratio(), Some(1.00));
        assert_eq!(geometry.rear_arb.legacy_ratio(), Some(0.80));
        // The transaction rule then keeps the consumed value equal to that block.
        adopt_antiroll_knob(&mut config);
        assert_eq!(config.suspension.front_arb_ratio, 1.00);
        assert_eq!(config.suspension.rear_arb_ratio, 0.80);

        // A driver edit moves the knob only; adopting it moves the bar exactly once.
        config
            .suspension
            .geometric_suspension
            .as_mut()
            .expect("geometric profile")
            .front_arb = AntiRollConfig::LegacyRatio { ratio: 0.20 };
        adopt_antiroll_knob(&mut config);
        assert_eq!(config.suspension.front_arb_ratio, 0.20);
        adopt_antiroll_knob(&mut config);
        assert_eq!(config.suspension.front_arb_ratio, 0.20);
    }

    #[test]
    fn an_explicit_bar_rate_is_never_rewritten() {
        let mut config = design_profile();
        config
            .suspension
            .geometric_suspension
            .as_mut()
            .expect("geometric profile")
            .front_arb = AntiRollConfig::MotionRatio {
            bar_rate_N_per_m: 40_000.0,
            motion_ratio: 1.1,
        };
        adopt_antiroll_design_bar(&mut config);
        adopt_antiroll_knob(&mut config);
        assert_eq!(
            config
                .suspension
                .geometric_suspension
                .as_ref()
                .expect("geometric profile")
                .front_arb,
            AntiRollConfig::MotionRatio {
                bar_rate_N_per_m: 40_000.0,
                motion_ratio: 1.1
            }
        );
        // With no ratio to copy, the flat design value still governs.
        assert_eq!(config.suspension.front_arb_ratio, 1.00);
    }
}
