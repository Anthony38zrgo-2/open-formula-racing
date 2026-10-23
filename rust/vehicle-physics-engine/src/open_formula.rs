//! Open Formula host layer: ground queries, authoritative simulation and the setup
//! transaction consumed by the C++ `rustphysics` module.
//!
//! Ownership summary:
//! * Rust owns pose, velocities, orientation, suspension, wheel spin, transmission,
//!   fuel, thermal state and contact state.
//! * The host supplies the track ground collision mesh (the same `FORMULA_GROUND_1`
//!   document the retired C++ ground sampler used) and reports body/barrier contacts.
//! * All ray queries (3 per wheel + 2 underfloor) are performed here, once per step.

use std::path::Path;
use std::sync::OnceLock;

use parry3d_f64::na::{Isometry3, Point3, Vector3};
use parry3d_f64::query::Ray;
use parry3d_f64::shape::{SharedShape, TriMesh};
use serde_json::Value;

use crate::open_formula_ffi::{
    OfpAxleDiagnostics, OfpBodyContact, OfpControlInput, OfpFailureDiagnostics, OfpSnapshot,
    OfpVec3, OfpWheelDiagnostics, OfpWheelState, OFP_ABI_VERSION, OFP_FLAG_BRAKE_THERMAL,
    OFP_FLAG_POWERTRAIN_THERMAL, OFP_FLAG_WHEEL_THERMAL, OFP_WHEELS,
};
use crate::simulation::{default_spawn_height, VehicleSimulator, VehicleState};
use crate::suspension_setup::{
    adopt_antiroll_design_bar, adopt_antiroll_knob, SuspensionSetupAuthority,
};
use crate::types::{Mat3, SurfaceType, TriRaycastSample, Vec3, VehicleInput, WheelIndex};
use crate::vehicle_configuration::{VehicleConfig, load_vehicle_configuration};

/// Number of transverse wheel rays (inner / center / outer).
pub const WHEEL_RAYS_PER_WHEEL: usize = 3;
/// The simplified aerodynamics backend uses the centre-front and diffuser-throat probes.
pub const FLOOR_PROBE_COUNT: usize = 2;
/// Probe mask carried by the two underfloor sensors: slots 0 (front centre) and 3 (throat).
pub const FLOOR_PROBE_MASK: u32 = (1 << 0) | (1 << 3);

// ---------------------------------------------------------------------------
// Ground world
// ---------------------------------------------------------------------------

#[derive(Debug, Clone, Copy)]
struct GroundTriangleMaterial {
    friction: f64,
    rolling_resistance: f64,
}

#[derive(Debug)]
pub struct GroundWorld {
    mesh: Option<SharedShape>,
    materials: Vec<GroundTriangleMaterial>,
    triangle_count: usize,
}

#[derive(Debug, Clone, Copy)]
pub struct GroundHit {
    pub distance: f64,
    pub point: Vec3,
    pub normal: Vec3,
    pub surface: SurfaceType,
}

impl GroundWorld {
    pub fn empty() -> Self {
        Self {
            mesh: None,
            materials: Vec::new(),
            triangle_count: 0,
        }
    }

    /// Loads the `FORMULA_GROUND_1` collision document produced by the track exporter:
    /// friction, rolling resistance and nine coordinates per triangle. Triangles and
    /// materials are imported into a Parry `TriMesh` whose acceleration structure is
    /// built once, at load time.
    pub fn load(path: &Path) -> Result<Self, String> {
        let text = std::fs::read_to_string(path)
            .map_err(|error| format!("cannot read ground mesh {}: {error}", path.display()))?;
        let mut tokens = text.split_whitespace();
        let signature = tokens.next().unwrap_or("");
        if signature != "FORMULA_GROUND_1" {
            return Err(format!(
                "unsupported ground mesh signature in {}: {signature}",
                path.display()
            ));
        }
        let count: usize = tokens
            .next()
            .ok_or_else(|| "ground mesh is missing its triangle count".to_string())?
            .parse()
            .map_err(|error| format!("invalid ground mesh triangle count: {error}"))?;
        if count == 0 || count > 1_000_000 {
            return Err(format!("ground mesh triangle count out of range: {count}"));
        }
        let mut vertices: Vec<Point3<f64>> = Vec::with_capacity(count * 3);
        let mut indices: Vec<[u32; 3]> = Vec::with_capacity(count);
        let mut materials: Vec<GroundTriangleMaterial> = Vec::with_capacity(count);
        for triangle_index in 0..count {
            let friction: f64 = tokens
                .next()
                .ok_or_else(|| format!("ground mesh truncated at triangle {triangle_index}"))?
                .parse()
                .map_err(|error| format!("invalid triangle friction: {error}"))?;
            let rolling: f64 = tokens
                .next()
                .ok_or_else(|| format!("ground mesh truncated at triangle {triangle_index}"))?
                .parse()
                .map_err(|error| format!("invalid triangle rolling resistance: {error}"))?;
            let mut coordinates = [0.0f64; 9];
            for coordinate in coordinates.iter_mut() {
                *coordinate = tokens
                    .next()
                    .ok_or_else(|| format!("ground mesh truncated at triangle {triangle_index}"))?
                    .parse()
                    .map_err(|error| format!("invalid triangle coordinate: {error}"))?;
                if !coordinate.is_finite() {
                    return Err(format!("non-finite triangle coordinate at {triangle_index}"));
                }
            }
            let base = (triangle_index * 3) as u32;
            // The exporter stores Speed Dreams world coordinates (X forward, Y left,
            // Z up). The Rust world is X right, Y up, -Z forward, so the track is
            // imported once through the same rigid mapping the host uses.
            for corner in 0..3 {
                let offset = corner * 3;
                vertices.push(Point3::new(
                    -coordinates[offset + 1],
                    coordinates[offset + 2],
                    -coordinates[offset],
                ));
            }
            indices.push([base, base + 1, base + 2]);
            materials.push(GroundTriangleMaterial {
                friction,
                rolling_resistance: rolling,
            });
        }
        let mesh = TriMesh::new(vertices, indices)
            .map_err(|error| format!("cannot build the ground acceleration structure: {error:?}"))?;
        Ok(Self {
            mesh: Some(SharedShape::new(mesh)),
            materials,
            triangle_count: count,
        })
    }

    pub fn triangle_count(&self) -> usize {
        self.triangle_count
    }

    pub fn is_loaded(&self) -> bool {
        self.mesh.is_some()
    }

    /// Casts one segment and returns the closest hit. The direction is normalized here
    /// so callers cannot distort the returned distance.
    pub fn raycast(&self, origin: Vec3, direction: Vec3, max_distance: f64) -> Option<GroundHit> {
        let mesh = self.mesh.as_ref()?;
        let length = direction.length();
        if !length.is_finite() || length <= 1e-9 || !max_distance.is_finite() || max_distance <= 0.0 {
            return None;
        }
        let unit = direction * (1.0 / length);
        let ray = Ray::new(
            Point3::new(origin.x, origin.y, origin.z),
            Vector3::new(unit.x, unit.y, unit.z),
        );
        let intersection =
            mesh.cast_ray_and_get_normal(&Isometry3::identity(), &ray, max_distance, true)?;
        let distance = intersection.time_of_impact;
        if !distance.is_finite() || !(0.0..=max_distance).contains(&distance) {
            return None;
        }
        let normal = Vec3::new(
            intersection.normal.x,
            intersection.normal.y,
            intersection.normal.z,
        );
        let normal = if normal.length() > 1e-9 {
            normal.normalized()
        } else {
            Vec3::UP
        };
        let point = origin + unit * distance;
        let surface = match intersection.feature {
            parry3d_f64::shape::FeatureId::Face(index) => self
                .materials
                .get(index as usize)
                .map(|material| classify_surface(material.friction, material.rolling_resistance))
                .unwrap_or(SurfaceType::Road),
            _ => SurfaceType::Road,
        };
        Some(GroundHit {
            distance,
            point,
            normal,
            surface,
        })
    }
}

fn classify_surface(_friction: f64, rolling_resistance: f64) -> SurfaceType {
    if rolling_resistance <= 0.0025 {
        SurfaceType::Road
    } else if rolling_resistance <= 0.0035 {
        SurfaceType::Curb
    } else if rolling_resistance <= 0.005 {
        SurfaceType::Metal
    } else if rolling_resistance <= 0.09 {
        SurfaceType::Grass
    } else if rolling_resistance <= 0.13 {
        SurfaceType::Gravel
    } else {
        SurfaceType::Sand
    }
}

static TRACK_GROUND: OnceLock<GroundWorld> = OnceLock::new();

/// Loads the session ground mesh once. Returns the triangle count.
pub fn set_track_ground_mesh(path: &str) -> Result<usize, String> {
    if TRACK_GROUND.get().is_some() {
        return Ok(TRACK_GROUND.get().map(|g| g.triangle_count()).unwrap_or(0));
    }
    let world = if path.is_empty() {
        GroundWorld::empty()
    } else {
        GroundWorld::load(Path::new(path))?
    };
    let count = world.triangle_count();
    let _ = TRACK_GROUND.set(world);
    Ok(count)
}

pub fn track_ground() -> Option<&'static GroundWorld> {
    TRACK_GROUND.get()
}

// ---------------------------------------------------------------------------
// Setup descriptor and transaction
// ---------------------------------------------------------------------------

#[derive(Debug, Clone, Copy)]
struct SetupField {
    path: &'static str,
    label: &'static str,
    unit: &'static str,
    minimum: f64,
    maximum: f64,
    step: f64,
    editable: bool,
}

const SETUP_FIELDS: &[SetupField] = &[
    SetupField { path: "suspension.front.resting_ratio", label: "Front ride height ratio", unit: "ratio", minimum: 0.20, maximum: 0.80, step: 0.005, editable: true },
    SetupField { path: "suspension.rear.resting_ratio", label: "Rear ride height ratio", unit: "ratio", minimum: 0.20, maximum: 0.80, step: 0.005, editable: true },
    SetupField { path: "suspension.front.damping_ratio", label: "Front damper ratio", unit: "ratio", minimum: 0.05, maximum: 1.20, step: 0.01, editable: true },
    SetupField { path: "suspension.rear.damping_ratio", label: "Rear damper ratio", unit: "ratio", minimum: 0.05, maximum: 1.20, step: 0.01, editable: true },
    SetupField { path: "suspension.front.bump_damp_multiplier", label: "Front bump multiplier", unit: "ratio", minimum: 0.20, maximum: 3.00, step: 0.01, editable: true },
    SetupField { path: "suspension.rear.bump_damp_multiplier", label: "Rear bump multiplier", unit: "ratio", minimum: 0.20, maximum: 3.00, step: 0.01, editable: true },
    SetupField { path: "suspension.front.rebound_damp_multiplier", label: "Front rebound multiplier", unit: "ratio", minimum: 0.20, maximum: 3.00, step: 0.01, editable: true },
    SetupField { path: "suspension.rear.rebound_damp_multiplier", label: "Rear rebound multiplier", unit: "ratio", minimum: 0.20, maximum: 3.00, step: 0.01, editable: true },
    SetupField { path: "suspension.geometry_physical.front_arb.ratio", label: "Front anti-roll ratio", unit: "ratio", minimum: 0.0, maximum: 2.0, step: 0.01, editable: true },
    SetupField { path: "suspension.geometry_physical.rear_arb.ratio", label: "Rear anti-roll ratio", unit: "ratio", minimum: 0.0, maximum: 2.0, step: 0.01, editable: true },
    SetupField { path: "suspension.front.toe", label: "Front toe (local)", unit: "rad", minimum: -0.05, maximum: 0.05, step: 0.001, editable: true },
    SetupField { path: "suspension.rear.toe", label: "Rear toe (local)", unit: "rad", minimum: -0.05, maximum: 0.05, step: 0.001, editable: true },
    SetupField { path: "suspension.front.camber", label: "Front camber", unit: "rad", minimum: -0.10, maximum: 0.05, step: 0.001, editable: true },
    SetupField { path: "suspension.rear.camber", label: "Rear camber", unit: "rad", minimum: -0.10, maximum: 0.05, step: 0.001, editable: true },
    SetupField { path: "suspension.front.bump_stop_multiplier", label: "Front bump stop", unit: "ratio", minimum: 0.0, maximum: 4.0, step: 0.05, editable: true },
    SetupField { path: "suspension.rear.bump_stop_multiplier", label: "Rear bump stop", unit: "ratio", minimum: 0.0, maximum: 4.0, step: 0.05, editable: true },
    SetupField { path: "brakes.front_brake_bias", label: "Front brake bias", unit: "fraction", minimum: 0.30, maximum: 0.80, step: 0.005, editable: true },
    SetupField { path: "brakes.max_brake_torque", label: "Maximum brake torque", unit: "N m", minimum: 500.0, maximum: 8000.0, step: 50.0, editable: true },
    SetupField { path: "powertrain.differential.preload_nm", label: "Differential preload", unit: "N m", minimum: 0.0, maximum: 120.0, step: 1.0, editable: true },
    SetupField { path: "powertrain.differential.power_ramp_angle_deg", label: "Differential power ramp", unit: "deg", minimum: 5.0, maximum: 80.0, step: 1.0, editable: true },
    SetupField { path: "powertrain.differential.coast_ramp_angle_deg", label: "Differential coast ramp", unit: "deg", minimum: 5.0, maximum: 80.0, step: 1.0, editable: true },
    SetupField { path: "powertrain.differential.clutches", label: "Differential clutches", unit: "count", minimum: 1.0, maximum: 8.0, step: 1.0, editable: true },
    SetupField { path: "powertrain.final_drive", label: "Final drive", unit: "ratio", minimum: 2.0, maximum: 6.0, step: 0.01, editable: true },
    SetupField { path: "powertrain.max_rpm", label: "Rev limiter", unit: "rpm", minimum: 6000.0, maximum: 22000.0, step: 100.0, editable: true },
    SetupField { path: "aero.model.front_wing.incidence_deg", label: "Front wing incidence", unit: "deg", minimum: 0.0, maximum: 20.0, step: 0.5, editable: true },
    SetupField { path: "aero.model.rear_wing.incidence_deg", label: "Rear wing incidence", unit: "deg", minimum: 0.0, maximum: 20.0, step: 0.5, editable: true },
    SetupField { path: "steering.max_steering_angle", label: "Maximum steering angle", unit: "rad", minimum: 0.15, maximum: 0.70, step: 0.005, editable: true },
    SetupField { path: "steering.steering_speed", label: "Steering speed", unit: "1/s", minimum: 1.0, maximum: 12.0, step: 0.05, editable: true },
    SetupField { path: "fuel.initial_kg", label: "Fuel load", unit: "kg", minimum: 0.0, maximum: 120.0, step: 0.5, editable: true },
    SetupField { path: "fuel.density_kg_per_litre", label: "Fuel density (conversion)", unit: "kg/L", minimum: 0.0, maximum: 0.0, step: 0.0, editable: false },
    SetupField { path: "tires.pressure.cold_kpa_gauge.FL", label: "Front left cold pressure", unit: "kPa gauge", minimum: 80.0, maximum: 220.0, step: 1.0, editable: true },
    SetupField { path: "tires.pressure.cold_kpa_gauge.FR", label: "Front right cold pressure", unit: "kPa gauge", minimum: 80.0, maximum: 220.0, step: 1.0, editable: true },
    SetupField { path: "tires.pressure.cold_kpa_gauge.RL", label: "Rear left cold pressure", unit: "kPa gauge", minimum: 80.0, maximum: 220.0, step: 1.0, editable: true },
    SetupField { path: "tires.pressure.cold_kpa_gauge.RR", label: "Rear right cold pressure", unit: "kPa gauge", minimum: 80.0, maximum: 220.0, step: 1.0, editable: true },
    SetupField { path: "suspension.front.spring_length", label: "Front spring length (design)", unit: "m", minimum: 0.0, maximum: 0.0, step: 0.0, editable: false },
    SetupField { path: "suspension.rear.spring_length", label: "Rear spring length (design)", unit: "m", minimum: 0.0, maximum: 0.0, step: 0.0, editable: false },
    SetupField { path: "tires.front.radius", label: "Front tire radius (design)", unit: "m", minimum: 0.0, maximum: 0.0, step: 0.0, editable: false },
    SetupField { path: "tires.rear.radius", label: "Rear tire radius (design)", unit: "m", minimum: 0.0, maximum: 0.0, step: 0.0, editable: false },
];

fn setup_descriptor_value() -> Value {
    let fields: Vec<Value> = SETUP_FIELDS
        .iter()
        .map(|field| {
            serde_json::json!({
                "path": field.path,
                "label": field.label,
                "unit": field.unit,
                "min": field.minimum,
                "max": field.maximum,
                "step": field.step,
                "editable": field.editable,
            })
        })
        .collect();
    Value::Array(fields)
}

fn set_document_path(document: &mut Value, path: &str, value: Value) -> Result<(), String> {
    let segments: Vec<&str> = path.split('.').collect();
    let mut cursor = document;
    for (index, segment) in segments.iter().enumerate() {
        let object = cursor
            .as_object_mut()
            .ok_or_else(|| format!("setup path {path} does not address an object at '{segment}'"))?;
        if index + 1 == segments.len() {
            if !object.contains_key(*segment) {
                return Err(format!("setup path {path} is not part of the profile"));
            }
            object.insert((*segment).to_string(), value);
            return Ok(());
        }
        cursor = object
            .get_mut(*segment)
            .ok_or_else(|| format!("setup path {path} is missing '{segment}'"))?;
    }
    Err(format!("setup path {path} is empty"))
}

fn field_is_editable(path: &str) -> bool {
    SETUP_FIELDS
        .iter()
        .any(|field| field.editable && field.path == path)
}

/// One anti-roll bar, expressed twice in the same profile: the flat value the
/// simplified and geometric axles consume, and the `geometry_physical` knob the
/// garage writes. Both numbers are the same `LegacyRatio` quantity, so the setup
/// transaction keeps them equal with absolute copies and never with a product.
const ANTIROLL_AXLE_PATHS: [(&str, &str); 2] = [
    (
        "suspension.front.arb_ratio",
        "suspension.geometry_physical.front_arb.ratio",
    ),
    (
        "suspension.rear.arb_ratio",
        "suspension.geometry_physical.rear_arb.ratio",
    ),
];

fn document_number(document: &Value, path: &str) -> Option<f64> {
    document
        .pointer(&format!("/{}", path.replace('.', "/")))
        .and_then(Value::as_f64)
}

/// Profile load and reset: the garage knob must read the bar the car actually runs
/// on, otherwise the driver drags an inherited number while an unrelated flat value
/// governs the axle. After this fold the two agree, and from then on the knob is the
/// authority (`adopt_antiroll_knob`). Only the live document is folded: `base_document`
/// keeps the profile bytes, so the hash a saved reglaje carries still matches.
fn adopt_antiroll_design_knobs(document: &mut Value) {
    for (authority, knob) in ANTIROLL_AXLE_PATHS {
        let (Some(bar), Some(knobs)) = (
            document_number(document, authority),
            document_number(document, knob),
        ) else {
            // A `MotionRatio` bar carries no ratio knob; leave such a profile alone.
            continue;
        };
        if knobs != bar && (0.0..=2.0).contains(&bar) {
            // The knob path was just read as a number, so this cannot fail.
            let _ = set_document_path(document, knob, Value::from(bar));
        }
    }
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

#[derive(Debug, Clone, Copy)]
struct FloorProbe {
    origin_local: Vec3,
    ray_length: f64,
}

#[derive(Debug, Clone, Copy, Default)]
pub struct StepCounters {
    pub wheel_queries: u64,
    pub wheel_hits: u64,
    pub floor_queries: u64,
    pub floor_hits: u64,
    pub rejected_steps: u32,
}

#[derive(Clone)]
pub struct OpenFormulaSimulation {
    simulator: VehicleSimulator,
    document: Value,
    base_document: Value,
    /// Design elements and design trims of `base_document`: the single reference the
    /// garage controls are translated against, captured once so replaying a reglaje
    /// cannot multiply the profile against itself. `None` only for a profile that
    /// declares no usable design trim, in which case its coefficients stay authored.
    setup_authority: Option<SuspensionSetupAuthority>,
    floor_probes: [FloorProbe; FLOOR_PROBE_COUNT],
    base_floor_clearance: [f64; FLOOR_PROBE_COUNT],
    counters: StepCounters,
    last_floor_clearance: [f64; FLOOR_PROBE_COUNT],
    last_floor_impulse: [f64; FLOOR_PROBE_COUNT],
    last_floor_points: [Vec3; FLOOR_PROBE_COUNT],
    last_floor_normals: [Vec3; FLOOR_PROBE_COUNT],
    last_floor_mask: u32,
    last_body_contact_count: u32,
    last_body_contact_impulse: [f64; 32],
    last_error: String,
    failed: bool,
    /// Exact numbers of the last rejection. It lives here rather than in the
    /// simulator because a rejection restores the simulator to the accepted state,
    /// and the frozen state must still be able to report why it froze.
    last_failure: crate::vertical_diagnostics::FailureDiagnostics,
    backends: u32,
    // Etapa 3: audio presentation clock. `sequence` counts accepted steps for
    // packet ordering; `last_step_dt` is the real accepted-step duration.
    sequence: u64,
    last_step_dt: f64,
}

const BACKEND_SUSPENSION_SIMPLIFIED: u32 = 1 << 0;
const BACKEND_AERO_SIMPLIFIED: u32 = 1 << 1;
const BACKEND_TIRE_RELAXATION: u32 = 1 << 2;
const BACKEND_SLOW_STATES_SIMPLIFIED: u32 = 1 << 3;

pub fn validate_production_configuration(config: &VehicleConfig) -> Result<(), String> {
    if config.suspension.geometric_suspension.is_none() {
        return Err(
            "the simplified suspension backend requires a geometric suspension definition in the profile"
                .into(),
        );
    }
    if !matches!(
        config.suspension.suspension_model,
        crate::suspension_geo_config::SuspensionModelKind::Geometric
    ) {
        return Err("the profile does not declare the geometric suspension model".into());
    }
    if config.chassis.total_vehicle_mass(&config.tires, &config.fuel) <= 1.0 {
        return Err("the profile declares a non-positive vehicle mass".into());
    }
    if config.tires.front_tire_radius <= 0.0 || config.tires.rear_tire_radius <= 0.0 {
        return Err("the profile declares a non-positive tire radius".into());
    }
    if config.powertrain.gear_ratios.is_empty() || config.powertrain.gear_ratios.len() > 8 || config.powertrain.final_drive <= 0.0 {
        return Err("the profile declares an unusable transmission".into());
    }
    if let Some(underfloor) = &config.underfloor_contact {
        underfloor.validate()?;
    }
    Ok(())
}

impl OpenFormulaSimulation {
    pub fn create(
        profile_path: Option<&Path>,
        origin: Vec3,
        yaw: f64,
        fuel_kg: Option<f64>,
    ) -> Result<Self, String> {
        let (config, document) = match profile_path {
            Some(path) => {
                let config = load_vehicle_configuration(path)?;
                let document = resolve_profile_document(path)?;
                (config, document)
            }
            None => {
                let config = VehicleConfig::williams_fw19_hockenheim_canonical();
                let document: Value = serde_json::from_str(
                    VehicleConfig::williams_fw19_hockenheim_embedded_document(),
                )
                .map_err(|error| format!("embedded profile is invalid: {error}"))?;
                (config, document)
            }
        };
        validate_production_configuration(&config)?;
        let mut config = config;
        // One anti-roll authority: the flat axle bar wins, and the garage knob is
        // made to read that same number. `base_document` keeps the profile bytes, so
        // the hash stored with an existing reglaje still matches.
        adopt_antiroll_design_bar(&mut config);
        let mut live_document = document.clone();
        adopt_antiroll_design_knobs(&mut live_document);
        let setup_authority = SuspensionSetupAuthority::capture(&config);
        if let Some(fuel) = fuel_kg {
            config.fuel.current_kg = fuel.clamp(0.0, config.fuel.capacity_kg);
        }
        if config.fuel.current_kg <= 0.0 {
            config.fuel.current_kg = config.fuel.initial_kg;
        }
        let simulator = VehicleSimulator::new(config, origin, yaw);
        let mut simulation = Self {
            simulator,
            document: live_document,
            base_document: document,
            setup_authority,
            floor_probes: [FloorProbe {
                origin_local: Vec3::ZERO,
                ray_length: 0.35,
            }; FLOOR_PROBE_COUNT],
            base_floor_clearance: [0.09; FLOOR_PROBE_COUNT],
            counters: StepCounters::default(),
            last_floor_clearance: [0.0; FLOOR_PROBE_COUNT],
            last_floor_impulse: [0.0; FLOOR_PROBE_COUNT],
            last_floor_points: [Vec3::ZERO; FLOOR_PROBE_COUNT],
            last_floor_normals: [Vec3::UP; FLOOR_PROBE_COUNT],
            last_floor_mask: 0,
            last_body_contact_count: 0,
            last_body_contact_impulse: [0.0; 32],
            last_error: String::new(),
            failed: false,
            last_failure: crate::vertical_diagnostics::FailureDiagnostics::default(),
            backends: BACKEND_SUSPENSION_SIMPLIFIED
                | BACKEND_AERO_SIMPLIFIED
                | BACKEND_TIRE_RELAXATION
                | BACKEND_SLOW_STATES_SIMPLIFIED,
            sequence: 0,
            last_step_dt: 0.0,
        };
        simulation.configure_floor_probes()?;
        Ok(simulation)
    }

    pub fn backends(&self) -> u32 {
        self.backends
    }

    pub fn last_error(&self) -> &str {
        &self.last_error
    }

    pub fn failed(&self) -> bool {
        self.failed
    }

    /// Vehicle-local underfloor probe geometry, derived from the declared floor
    /// clearances exactly as the retired five-probe host derived it, but only for the
    /// two probes the simplified backend keeps (centre front and diffuser throat).
    fn configure_floor_probes(&mut self) -> Result<(), String> {
        let config = &self.simulator.config;
        let floor = &config.aerodynamics.aero_model.underfloor;
        let design_plane_local_y = -default_spawn_height(config);
        let (front_clearance, rear_clearance) = match (
            floor.floor_clearance_front_m,
            floor.floor_clearance_rear_m,
        ) {
            (Some(front), Some(rear)) => (front, rear),
            _ => {
                self.floor_probes = [
                    FloorProbe {
                        origin_local: Vec3::new(0.0, design_plane_local_y + 0.090, -0.90),
                        ray_length: 0.24,
                    },
                    FloorProbe {
                        origin_local: Vec3::new(0.0, design_plane_local_y + 0.090 + floor.throat_probe_offset_m, 0.85),
                        ray_length: 0.24,
                    },
                ];
                self.base_floor_clearance = [0.090, 0.090];
                return Ok(());
            }
        };
        let front_anchor = config.tires.wheel_anchor_local(
            &config.chassis,
            &config.suspension,
            WheelIndex::FrontLeft,
        );
        let rear_anchor = config.tires.wheel_anchor_local(
            &config.chassis,
            &config.suspension,
            WheelIndex::RearLeft,
        );
        let axle_span = rear_anchor.z - front_anchor.z;
        let mut stations = [(-0.90f64, 0.0f64), (0.85f64, 0.0f64)];
        for (index, (z, offset)) in stations.iter_mut().enumerate() {
            let fraction = if axle_span.abs() > 1e-6 {
                ((*z - front_anchor.z) / axle_span).clamp(0.0, 1.0)
            } else {
                0.0
            };
            let station_clearance =
                front_clearance + (rear_clearance - front_clearance) * fraction;
            let probe_offset = if index == 1 {
                floor.throat_probe_offset_m
            } else {
                0.0
            };
            *offset = probe_offset;
            self.floor_probes[index] = FloorProbe {
                origin_local: Vec3::new(
                    0.0,
                    design_plane_local_y + station_clearance + probe_offset,
                    *z,
                ),
                ray_length: station_clearance + probe_offset + 0.15,
            };
            self.base_floor_clearance[index] = station_clearance;
        }
        Ok(())
    }

    pub fn reset(&mut self, origin: Vec3, yaw: f64) {
        self.simulator.reset(origin, yaw);
        self.failed = false;
        self.counters = StepCounters::default();
        // New session/generation for audio packet ordering.
        self.sequence = 0;
        self.last_step_dt = 0.0;
        self.last_error.clear();
    }

    pub fn set_pose(&mut self, origin: Vec3, basis: Mat3) {
        let state: &mut VehicleState = &mut self.simulator.state;
        state.transform = crate::types::Transform3D::new(origin, basis);
        state.orientation = crate::types::Quat::from_mat3(&basis);
        state.linear_velocity = Vec3::ZERO;
        state.angular_velocity = Vec3::ZERO;
        state.suspension.reset_mechanical_state();
        self.failed = false;
        self.last_error.clear();
    }

    pub fn pose(&self) -> (Vec3, Vec3, Vec3, Vec3) {
        let state = &self.simulator.state;
        (
            state.transform.origin,
            state.transform.basis.x,
            state.transform.basis.y,
            state.transform.basis.z,
        )
    }

    pub fn velocity(&self) -> Vec3 {
        self.simulator.state.linear_velocity
    }

    fn wheel_ray_length(&self, wheel: WheelIndex) -> f64 {
        let config = &self.simulator.config;
        let spring_length = if wheel.is_front() {
            config.suspension.front_spring_length
        } else {
            config.suspension.rear_spring_length
        };
        let radius = if wheel.is_front() {
            config.tires.front_tire_radius
        } else {
            config.tires.rear_tire_radius
        };
        spring_length + radius
    }

    fn wheel_span(&self, wheel: WheelIndex) -> f64 {
        let config = &self.simulator.config;
        let width = if wheel.is_front() {
            config.tires.front_tire_width
        } else {
            config.tires.rear_tire_width
        };
        width * config.suspension.tri_ray_spacing_ratio
    }

    /// Twelve independent transverse wheel rays. A ray that misses stays a miss: the
    /// centre impact is never replicated onto the inner/outer samples.
    fn sample_wheel_rays(&mut self) -> [TriRaycastSample; 4] {
        let state = &self.simulator.state;
        let config = &self.simulator.config;
        let basis = state.transform.basis;
        let down = -basis.y;
        let mut samples_out = [TriRaycastSample::default(); 4];
        let ground = track_ground();
        for wheel in WheelIndex::ALL {
            let index = wheel as usize;
            let anchor = config.tires.wheel_anchor_local(
                &config.chassis,
                &config.suspension,
                wheel,
            );
            let anchor_world = state.transform.transform_point(anchor);
            let span = self.wheel_span(wheel);
            let length = self.wheel_ray_length(wheel);
            let lateral_sign = if wheel.is_left() { 1.0 } else { -1.0 };
            let lateral = basis.x * (span * lateral_sign);
            let origins = [
                anchor_world + lateral,
                anchor_world,
                anchor_world - lateral,
            ];
            let hits = [
                ground.and_then(|world| world.raycast(origins[0], down, length)),
                ground.and_then(|world| world.raycast(origins[1], down, length)),
                ground.and_then(|world| world.raycast(origins[2], down, length)),
            ];
            let mut sample = TriRaycastSample::default();
            for (slot, hit) in hits.iter().enumerate() {
                self.counters.wheel_queries += 1;
                let raycast = match hit {
                    Some(hit) => {
                        self.counters.wheel_hits += 1;
                        crate::types::RaycastHit {
                            is_colliding: true,
                            distance: hit.distance,
                            point: hit.point,
                            normal: hit.normal,
                            surface: hit.surface,
                        }
                    }
                    None => crate::types::RaycastHit {
                        is_colliding: false,
                        distance: length,
                        ..Default::default()
                    },
                };
                match slot {
                    0 => sample.inner = raycast,
                    1 => sample.center = raycast,
                    _ => sample.outer = raycast,
                }
            }
            samples_out[index] = sample;
        }
        samples_out
    }

    fn sample_floor(&mut self) -> (f64, f64, [f64; FLOOR_PROBE_COUNT], [Vec3; FLOOR_PROBE_COUNT], [Vec3; FLOOR_PROBE_COUNT], u32) {
        let state = &self.simulator.state;
        let basis = state.transform.basis;
        let down = -basis.y;
        let mut clearance = [self.floor_probes[0].ray_length, self.floor_probes[1].ray_length];
        let mut points = [Vec3::ZERO; FLOOR_PROBE_COUNT];
        let mut normals = [Vec3::UP; FLOOR_PROBE_COUNT];
        let mut mask = 0u32;
        let ground = track_ground();
        for index in 0..FLOOR_PROBE_COUNT {
            let probe = self.floor_probes[index];
            let origin = state.transform.transform_point(probe.origin_local);
            self.counters.floor_queries += 1;
            if let Some(hit) = ground.and_then(|world| world.raycast(origin, down, probe.ray_length)) {
                self.counters.floor_hits += 1;
                mask |= 1 << index;
                clearance[index] = hit.distance;
                points[index] = hit.point;
                normals[index] = hit.normal;
            } else {
                clearance[index] = probe.ray_length;
            }
        }
        let mut aero_clearance = [0.35f64; 5];
        aero_clearance[0] = clearance[0];
        aero_clearance[3] = clearance[1] + self.simulator.config.aerodynamics.aero_model.underfloor.throat_probe_offset_m;
        let _ = mask;
        (
            aero_clearance[0],
            aero_clearance[3],
            clearance,
            points,
            normals,
            mask,
        )
    }

    /// Applies a rigid, perfectly inelastic impulse at a body contact point. Used for
    /// barrier/body contacts reported by the host and for floor contact from the two
    /// underfloor probes. Returns the impulse magnitude (N s).
    fn apply_body_impulse(&mut self, point_world: Vec3, normal_world: Vec3, restitution: f64) -> f64 {
        let state = &mut self.simulator.state;
        let config = &self.simulator.config;
        let normal = if normal_world.length() > 1e-9 {
            normal_world.normalized()
        } else {
            Vec3::UP
        };
        let com_local = crate::simulation::center_of_mass_local_for_suspension_backend(
            config,
            state.suspension.backend,
        );
        let com = state.transform.transform_point(com_local);
        let arm = point_world - com;
        let point_velocity =
            state.linear_velocity + state.angular_velocity.cross(arm);
        let normal_velocity = point_velocity.dot(normal);
        if normal_velocity >= 0.0 {
            return 0.0;
        }
        let basis = state.transform.basis;
        let zero = crate::simplified_body_coupling::body_accelerations(config, state, crate::simulation::ForceTorqueOutput::default());
        let unit = crate::simplified_body_coupling::body_accelerations(config, state,
            crate::simulation::ForceTorqueOutput { force_world:normal, torque_world:arm.cross(normal) });
        let (Ok((a0, alpha0)), Ok((a1, alpha1))) = (zero, unit) else { return 0.0; };
        let response = a1-a0;
        let angular_response = alpha1-alpha0;
        let inverse_effective_mass = (response + angular_response.cross(arm)).dot(normal);
        if inverse_effective_mass <= 1e-9 {
            return 0.0;
        }
        let impulse = -(1.0 + restitution.clamp(0.0, 0.2)) * normal_velocity / inverse_effective_mass;
        if !impulse.is_finite() || impulse <= 0.0 {
            return 0.0;
        }
        let arms = WheelIndex::ALL.map(|w| crate::simplified_body_coupling::hub_arm(config,state,w));
        state.linear_velocity += response * impulse;
        state.angular_velocity += angular_response * impulse;
        for wheel in WheelIndex::ALL {
            state.suspension.wheels[wheel as usize].unsprung_velocity_m_s -=
                (response + angular_response.cross(arms[wheel as usize])).dot(basis.y)*impulse;
        }
        impulse
    }

    pub fn step(
        &mut self,
        input: &OfpControlInput,
        contacts: &[OfpBodyContact],
        duration_seconds: f64,
    ) -> Result<(), String> {
        if self.failed {
            return Err(self.last_error.clone());
        }
        if !duration_seconds.is_finite() || duration_seconds <= 0.0 {
            return Ok(());
        }
        let dt = duration_seconds.clamp(1.0 / 1000.0, 1.0 / 20.0);
        // Host identity; each nested solve subsequently sets its own endpoint and duration.
        crate::vertical_diagnostics::begin_step(self.simulator.state.sim_time, dt, self.sequence);
        let accepted_before_step = self.clone();
        self.counters.wheel_queries = 0;
        self.counters.wheel_hits = 0;
        self.counters.floor_queries = 0;
        self.counters.floor_hits = 0;
        let vehicle_input = VehicleInput {
            steering: input.steering.clamp(-1.0, 1.0),
            throttle: input.throttle.clamp(0.0, 1.0),
            brake: input.brake.clamp(0.0, 1.0),
            handbrake: input.handbrake.clamp(0.0, 1.0),
            clutch: input.clutch.clamp(0.0, 1.0),
            gear_request: match input.gear_request {
                -1 => Some(-1),
                0 => Some(0),
                1..=8 => Some(input.gear_request as i8),
                _ => None,
            },
        };

        // Ground sampling for this step: 12 wheel rays + 2 floor probes.
        let samples = self.sample_wheel_rays();
        let (front_clearance, throat_clearance, floor_clearance, floor_points, floor_normals, floor_mask) =
            self.sample_floor();
        let mut aero_environment = crate::aero::AeroEnvironment::unavailable();
        aero_environment.valid_mask = (floor_mask & 1) | ((floor_mask & 2) << 2);
        aero_environment.clearance_m[0] = front_clearance;
        aero_environment.clearance_m[3] = throat_clearance;
        aero_environment.contact_confidence = 0.0;

        // Barrier/body contacts reported by the host: geometry only, response here.
        self.last_body_contact_count = contacts.len().min(32) as u32;
        self.last_body_contact_impulse = [0.0; 32];
        for (contact_index, contact) in contacts.iter().enumerate() {
            if contact.activation == 0 {
                continue;
            }
            if !contact.point.x.is_finite()
                || !contact.point.y.is_finite()
                || !contact.point.z.is_finite()
            {
                continue;
            }
            let impulse = self.apply_body_impulse(
                Vec3::new(contact.point.x, contact.point.y, contact.point.z),
                Vec3::new(contact.normal.x, contact.normal.y, contact.normal.z),
                contact.restitution,
            );
            if contact_index < 32 {
                self.last_body_contact_impulse[contact_index] = impulse.max(0.0);
            }
        }

        // Floor contact: a rigid unilateral constraint at each probe whose clearance is
        // exhausted. No spring, no damping, no second support for the wheels.
        let mut floor_impulse = [0.0f64; FLOOR_PROBE_COUNT];
        if let Some(underfloor) = self.simulator.config.underfloor_contact {
            if underfloor.enabled {
                for index in 0..FLOOR_PROBE_COUNT {
                    if floor_mask & (1 << index) == 0 {
                        continue;
                    }
                    if floor_clearance[index] > underfloor.approach_clearance_m
                        && floor_clearance[index] > 0.0
                    {
                        continue;
                    }
                    // Closing speed along the ground normal is negative when the floor
                    // approaches the surface, so the predicted clearance must add it.
                    let normal_velocity =
                        self.point_normal_velocity(floor_points[index], floor_normals[index]);
                    let predicted = floor_clearance[index] + normal_velocity * dt;
                    if predicted > 0.0 && floor_clearance[index] > 0.0 {
                        continue;
                    }
                    let impulse = self.apply_body_impulse(
                        floor_points[index],
                        floor_normals[index],
                        0.0,
                    );
                    if impulse > 0.0 {
                        floor_impulse[index] = impulse;
                    }
                }
            }
        }
        self.last_floor_impulse = floor_impulse;
        self.last_floor_points = floor_points;
        self.last_floor_normals = floor_normals;
        self.last_floor_mask = floor_mask;
        self.last_floor_clearance = floor_clearance;

        self.simulator
            .step_with_aerodynamics_environment(&vehicle_input, &samples, &aero_environment, dt);

        if let Some(error) = self.simulator.state.suspension.mechanical_failure.clone() {
            // Publish the exact constraint numbers before restoring the accepted
            // state: the trace keeps the rejected candidate next to it.
            let mut failure = self.simulator.state.suspension.failure.unwrap_or_default();
            if failure.code == crate::vertical_diagnostics::FailureCode::None {
                failure.code = crate::vertical_diagnostics::FailureCode::BodyCornerNotConverged;
            }
            let rejected_time_s = self.simulator.state.sim_time;
            crate::vertical_diagnostics::interval(accepted_before_step.simulator.state.sim_time, 0.0);
            let accepted = &accepted_before_step.simulator.state;
            crate::vertical_diagnostics::record(crate::vertical_diagnostics::kind::RESTORED, -1, -1, 0, 0, 0.0,
                accepted.linear_acceleration, accepted.angular_acceleration,
                [&accepted.suspension.vertical[0], &accepted.suspension.vertical[1],
                 &accepted.suspension.vertical[2], &accepted.suspension.vertical[3]]);
            crate::vertical_diagnostics::dump_rejection(failure, rejected_time_s);
            *self = accepted_before_step;
            self.counters.rejected_steps += 1;
            self.failed = true;
            self.last_error = error;
            self.last_failure = failure;
            return Err(self.last_error.clone());
        }

        let state = &self.simulator.state;
        let finite = state.transform.origin.is_finite()
            && state.linear_velocity.is_finite()
            && state.angular_velocity.is_finite()
            && state.transform.basis.x.is_finite()
            && state.transform.basis.y.is_finite()
            && state.transform.basis.z.is_finite()
            && state.suspension.wheels.iter().all(|wheel| {
                wheel.total_normal_force.is_finite()
                    && wheel.suspension_compression_m.is_finite()
                    && wheel.unsprung_velocity_m_s.is_finite()
            })
            && state.tires.wheels.iter().all(|tire| {
                tire.spin.is_finite()
                    && tire.longitudinal_force.is_finite()
                    && tire.lateral_force.is_finite()
            });
        if !finite {
            let failure = crate::vertical_diagnostics::FailureDiagnostics {
                code: crate::vertical_diagnostics::FailureCode::NonFiniteState,
                wheel: -1,
                axle: -1,
                ..crate::vertical_diagnostics::FailureDiagnostics::default()
            };
            let rejected = &self.simulator.state;
            crate::vertical_diagnostics::record_failure(failure, 0, 0,
                rejected.linear_acceleration, rejected.angular_acceleration,
                [&rejected.suspension.vertical[0], &rejected.suspension.vertical[1],
                 &rejected.suspension.vertical[2], &rejected.suspension.vertical[3]]);
            let rejected_time_s = self.simulator.state.sim_time;
            crate::vertical_diagnostics::interval(accepted_before_step.simulator.state.sim_time, 0.0);
            let accepted = &accepted_before_step.simulator.state;
            crate::vertical_diagnostics::record(crate::vertical_diagnostics::kind::RESTORED, -1, -1, 0, 0, 0.0,
                accepted.linear_acceleration, accepted.angular_acceleration,
                [&accepted.suspension.vertical[0], &accepted.suspension.vertical[1],
                 &accepted.suspension.vertical[2], &accepted.suspension.vertical[3]]);
            crate::vertical_diagnostics::dump_rejection(failure, rejected_time_s);
            *self = accepted_before_step;
            self.counters.rejected_steps += 1;
            self.failed = true;
            self.last_error =
                "the Rust physics step produced a non-finite vehicle state; the last valid state is preserved"
                    .to_string();
            self.last_failure = failure;
            return Err(self.last_error.clone());
        }
        // Accepted step: advance the audio presentation clock. The snapshot and
        // every audio packet reuse this sequence/dt, so cuts resolved here are
        // never replaced by a later graphic frame.
        self.sequence = self.sequence.saturating_add(1);
        self.last_step_dt = dt;
        self.record_accepted_tick();
        Ok(())
    }

    /// One accepted-tick row plus the event detector and the pending-window writer.
    /// Diagnostics only: the simulation result is already committed.
    fn record_accepted_tick(&self) {
        let state = &self.simulator.state;
        crate::vertical_diagnostics::interval(state.sim_time, self.last_step_dt);
        let vertical = &state.suspension.vertical;
        let wheels = [&vertical[0], &vertical[1], &vertical[2], &vertical[3]];
        crate::vertical_diagnostics::record(
            crate::vertical_diagnostics::kind::TICK,
            -1,
            -1,
            0,
            0,
            0.0,
            state.linear_acceleration,
            state.angular_acceleration,
            wheels,
        );
        crate::vertical_diagnostics::observe_accepted(state.sim_time, wheels);
        crate::vertical_diagnostics::service_window(state.sim_time);
    }

    fn point_normal_velocity(&self, point_world: Vec3, normal_world: Vec3) -> f64 {
        let state = &self.simulator.state;
        let config = &self.simulator.config;
        let com_local = crate::simulation::center_of_mass_local_for_suspension_backend(
            config,
            state.suspension.backend,
        );
        let com = state.transform.transform_point(com_local);
        let arm = point_world - com;
        (state.linear_velocity + state.angular_velocity.cross(arm)).dot(normal_world)
    }

    pub fn snapshot(&self) -> OfpSnapshot {
        let state = &self.simulator.state;
        let config = &self.simulator.config;
        let basis = state.transform.basis;
        let mut snapshot = OfpSnapshot::zeroed();
        snapshot.abi_version = OFP_ABI_VERSION;
        snapshot.sim_time_s = state.sim_time;
        snapshot.physics_hz = state.physics_hz;
        snapshot.gear = state.powertrain.current_gear as i32;
        snapshot.target_gear = state.powertrain.target_gear as i32;
        snapshot.position = vec3(state.transform.origin);
        snapshot.linear_velocity = vec3(state.linear_velocity);
        snapshot.angular_velocity = vec3(state.angular_velocity);
        snapshot.linear_acceleration = vec3(state.linear_acceleration);
        snapshot.basis_x = vec3(basis.x);
        snapshot.basis_y = vec3(basis.y);
        snapshot.basis_z = vec3(basis.z);
        snapshot.flags = OFP_FLAG_POWERTRAIN_THERMAL | OFP_FLAG_WHEEL_THERMAL | OFP_FLAG_BRAKE_THERMAL;
        // The powertrain state publishes revolutions per minute; the host converts once.
        snapshot.engine_rpm = state.powertrain.rpm;
        snapshot.engine_torque_nm = state.powertrain.engine_torque;
        snapshot.clutch_engagement = state.powertrain.clutch_engagement;
        // ABI v6 raw audio-mechanical state: read the existing physical state,
        // never an acoustic comparison or a controller pulse.
        snapshot.clutch_torque_nm = state.powertrain.clutch_torque;
        snapshot.tc_cut_ratio_smoothed = state.powertrain.tc_cut_ratio_smoothed;
        snapshot.is_rev_limited = u32::from(state.powertrain.is_rev_limited);
        snapshot.shift_timer_s = state.powertrain.shift_timer;
        snapshot.step_dt_s = self.last_step_dt;
        snapshot.sequence = self.sequence;
        snapshot.fuel_kg = config.fuel.current_kg;
        snapshot.fuel_capacity_kg = config.fuel.capacity_kg;
        snapshot.throttle = state.throttle_input_smoothed;
        snapshot.brake = state.brake_input_smoothed;
        snapshot.steer_input = state.steer_input_smoothed;
        snapshot.steering_angle_rad = state
            .tires
            .wheels
            .iter()
            .map(|tire| tire.steer_angle_rad)
            .fold(0.0, f64::max);
        snapshot.aero_front_downforce_n = state.aero.front_downforce;
        snapshot.aero_floor_downforce_n = state.aero.diffuser_downforce;
        snapshot.aero_rear_downforce_n = state.aero.rear_downforce;
        snapshot.aero_drag_n = state.aero.drag_force;
        // Engine thermal state is owned by the powertrain thermal system; the HUD never
        // derives it from torque or from a fixed initial value.
        snapshot.water_temperature_celsius = state.powertrain_thermal.water_temperature_celsius;
        snapshot.oil_temperature_celsius = state.powertrain_thermal.oil_temperature_celsius;
        for wheel in WheelIndex::ALL {
            let index = wheel as usize;
            let suspension = &state.suspension.wheels[index];
            let tire = &state.tires.wheels[index];
            let thermal = &state.tire_thermal.wheels[index];
            let wear = &state.tire_wear.wheels[index];
            let mut wheel_state = OfpWheelState::zeroed();
            wheel_state.grounded = if suspension.is_grounded { 1 } else { 0 };
            wheel_state.surface = surface_code(suspension.effective_surface);
            wheel_state.spin_rad_s = tire.spin;
            wheel_state.steer_angle_rad = tire.steer_angle_rad;
            wheel_state.slip_ratio = tire.slip_ratio;
            wheel_state.slip_angle_rad = tire.slip_angle_rad;
            wheel_state.longitudinal_force_n = tire.longitudinal_force;
            wheel_state.lateral_force_n = tire.lateral_force;
            wheel_state.aligning_torque_nm = tire.aligning_torque;
            wheel_state.normal_force_n = suspension.total_normal_force;
            wheel_state.suspension_travel_m = suspension.suspension_compression_m;
            wheel_state.suspension_velocity_m_s = suspension.unsprung_velocity_m_s;
            wheel_state.spring_force_n = suspension.spring_force;
            wheel_state.damper_force_n = suspension.damping_force;
            wheel_state.contact_fraction = suspension.contact_fraction;
            // Render orientation is relative to the body. Road-effective camber is
            // a tire/contact quantity and would add terrain inclination a second time.
            wheel_state.dynamic_camber_rad = suspension.kinematic_camber_rad;
            wheel_state.toe_rad = suspension.geometric_toe_rad;
            wheel_state.tire_deflection_m = suspension.tire_deflection_m;
            // Independent thermal nodes: the tread centre is never copied onto the inner
            // and outer shoulders, and the brake disc is not the rim.
            wheel_state.tread_temperature_c = thermal.tread_center_c;
            wheel_state.tread_inner_temperature_c = thermal.tread_inner_c;
            wheel_state.tread_outer_temperature_c = thermal.tread_outer_c;
            wheel_state.carcass_temperature_c = thermal.carcass_c;
            wheel_state.gas_temperature_c = thermal.gas_c;
            wheel_state.pressure_kpa_gauge = thermal.pressure_kpa_gauge;
            let brake_thermal = &state.brake_thermal.wheels[index];
            wheel_state.brake_disc_temperature_c = brake_thermal.disc_c;
            wheel_state.brake_rim_temperature_c = brake_thermal.rim_c;
            wheel_state.brake_efficiency = brake_thermal.efficiency;
            wheel_state.wear_remaining_fraction = wear.tread_center_millimeters
                / config
                    .tires
                    .tire_wear
                    .for_wheel(wheel)
                    .initial_tread_depth_millimeters
                    .max(1e-6);
            wheel_state.wear_grip_scale = wear.wear_grip_scale;
            wheel_state.contact_point = vec3(suspension.effective_contact_point);
            wheel_state.contact_normal = vec3(suspension.effective_normal);
            let anchor = config.tires.wheel_anchor_local(
                &config.chassis,
                &config.suspension,
                wheel,
            );
            let spring_length = if wheel.is_front() {
                config.suspension.front_spring_length
            } else {
                config.suspension.rear_spring_length
            };
            wheel_state.spring_length_m = spring_length;
            wheel_state.tire_radius_m = if wheel.is_front() {
                config.tires.front_tire_radius
            } else {
                config.tires.rear_tire_radius
            };
            wheel_state.hub_position = vec3(
                state.transform.transform_point(anchor)
                    - basis.y * (spring_length - suspension.suspension_compression_m),
            );
            wheel_state.brake_torque_nm = state.powertrain.brake_torques[index];
            wheel_state.ray_hit_mask = (suspension.ray_grounded[0] as u32)
                | ((suspension.ray_grounded[1] as u32) << 1)
                | ((suspension.ray_grounded[2] as u32) << 2);
            wheel_state.ray_distance[0] = suspension.ray_compressions_m[0];
            wheel_state.ray_distance[1] = suspension.ray_compressions_m[1];
            wheel_state.ray_distance[2] = suspension.ray_compressions_m[2];
            wheel_state.ray_hits = wheel_state.ray_hit_mask.count_ones() as i32;
            snapshot.wheels[index] = wheel_state;
        }
        snapshot.floor_clearance = self.last_floor_clearance;
        snapshot.floor_valid_mask = self.last_floor_mask;
        snapshot.floor_impulse_ns = self.last_floor_impulse;
        for index in 0..FLOOR_PROBE_COUNT {
            snapshot.floor_contact_point[index] = vec3(self.last_floor_points[index]);
            snapshot.floor_contact_normal[index] = vec3(self.last_floor_normals[index]);
        }
        snapshot.wheel_ray_queries = self.counters.wheel_queries;
        snapshot.wheel_ray_hits = self.counters.wheel_hits;
        snapshot.floor_ray_queries = self.counters.floor_queries;
        snapshot.floor_ray_hits = self.counters.floor_hits;
        snapshot.backends = self.backends;
        snapshot.rejected_steps = self.counters.rejected_steps;
        snapshot.body_contact_count = self.last_body_contact_count;
        snapshot.body_contact_impulse_ns = self.last_body_contact_impulse;
        // ABI v7: vertical-suspension diagnostics. These are measurements of the last
        // accepted solve plus the rejection record; they never feed a force.
        snapshot.angular_acceleration = vec3(state.angular_acceleration);
        for index in 0..OFP_WHEELS {
            snapshot.wheel_diagnostics[index] = wheel_diagnostics(&state.suspension.vertical[index]);
        }
        for axle in 0..2 {
            let source = &state.suspension.axles[axle];
            snapshot.axle_diagnostics[axle] = OfpAxleDiagnostics {
                iterations: source.iterations,
                converged: i32::from(source.converged),
                substeps: source.substeps,
                reserved0: 0,
                residual_m_s: source.residual_m_s,
                antiroll_stiffness_n_per_m: source.antiroll_stiffness_n_per_m,
                antiroll_force_n: source.antiroll_force_n,
                normal_load_n: source.normal_load_n,
            };
        }
        let failure = self.last_failure;
        snapshot.failure = OfpFailureDiagnostics {
            active: i32::from(self.failed),
            code: failure.code.as_i32(),
            wheel: failure.wheel,
            axle: failure.axle,
            iteration: failure.iteration,
            subdivisions: failure.subdivisions,
            demand_m: failure.demand_m,
            limit_m: failure.limit_m,
            excess_m: failure.excess_m,
            limit_velocity_m_s: failure.limit_velocity_m_s,
            residual_m_s: failure.residual_m_s,
            axle_wheel: failure.axle_wheels,
            reserved1: 0,
            reserved2: 0,
        };
        snapshot.trace_records = crate::vertical_diagnostics::buffered() as u32;
        snapshot
    }

    pub fn vehicle_specs(&self) -> crate::open_formula_ffi::OfpVehicleSpecs {
        let config = &self.simulator.config;
        let mut specs = crate::open_formula_ffi::OfpVehicleSpecs::default();
        specs.forward_gears = config.powertrain.gear_ratios.len() as i32;
        specs.has_reverse = (config.powertrain.reverse_ratio > 0.0) as i32;
        specs.final_drive = config.powertrain.final_drive;
        specs.reverse_ratio = config.powertrain.reverse_ratio;
        for (target, ratio) in specs.gear_ratios.iter_mut().zip(&config.powertrain.gear_ratios) {
            *target = *ratio;
        }
        specs.tire_width_m = [config.tires.front_tire_width, config.tires.front_tire_width,
            config.tires.rear_tire_width, config.tires.rear_tire_width];
        // Active-profile metadata: the presentation layer must not duplicate physics
        // constants or fall back to the retired reference car.
        specs.max_rpm = config.powertrain.max_rpm;
        specs.idle_rpm = config.powertrain.idle_rpm;
        specs.fuel_capacity_kg = config.fuel.capacity_kg;
        specs.fuel_density_kg_per_litre = config.fuel.density_kg_per_litre;
        specs.steering_max_angle_rad = config.steering.max_steering_angle;
        for wheel in WheelIndex::ALL {
            let index = wheel as usize;
            let thermal = config.tires.tire_thermal.for_wheel(wheel);
            specs.tread_optimal_temperature_c[index] = thermal.optimal_tread_temperature_c;
            specs.tread_optimal_window_c[index] = thermal.optimal_tread_window_c;
            specs.tread_overheat_temperature_c[index] = thermal.overheat_tread_temperature_c;
            specs.carcass_optimal_temperature_c[index] = thermal.optimal_carcass_temperature_c;
        }
        let brakes = &config.brakes.brake_thermal;
        specs.brake_optimal_min_temperature_c = brakes.optimal_min_temperature_c;
        specs.brake_optimal_max_temperature_c = brakes.optimal_max_temperature_c;
        specs.brake_fade_start_temperature_c = brakes.fade_start_temperature_c;
        specs.brake_critical_temperature_c = brakes.critical_temperature_c;
        let thermal = &config.powertrain.powertrain_thermal;
        specs.water_optimal_min_temperature_c = thermal.water_optimal_minimum_temperature_celsius;
        specs.water_optimal_max_temperature_c = thermal.water_optimal_maximum_temperature_celsius;
        specs.water_derating_temperature_c = thermal.water_hot_derating_temperature_celsius;
        specs.water_critical_temperature_c = thermal.water_critical_temperature_celsius;
        specs.oil_optimal_min_temperature_c = thermal.oil_optimal_minimum_temperature_celsius;
        specs.oil_optimal_max_temperature_c = thermal.oil_optimal_maximum_temperature_celsius;
        specs.oil_derating_temperature_c = thermal.oil_hot_derating_temperature_celsius;
        specs.oil_critical_temperature_c = thermal.oil_critical_temperature_celsius;
        // ABI v6 audio references: torque scale for delivered load, split mask
        // for the driven-wheel output-shaft frequency.
        specs.max_torque_nm = config.powertrain.max_torque;
        specs.front_torque_split = config.powertrain.front_torque_split;
        specs
    }

    pub fn fuel_density_kg_per_litre(&self) -> f64 {
        self.simulator.config.fuel.density_kg_per_litre
    }

    /// Effective setup actually in force, as newline-separated `FIELD` and `DERIVED`
    /// lines. Every value carries its unit and the range the garage admits, and the
    /// derived block states the coefficients the solver consumes. This is written
    /// after the transaction commits, so it can never show a requested value that the
    /// running car did not adopt.
    pub fn effective_setup_report(&self) -> String {
        let config = &self.simulator.config;
        let mut text = String::with_capacity(8192);
        for field in SETUP_FIELDS {
            let pointer = format!("/{}", field.path.replace('.', "/"));
            let Some(stored) = self.document.pointer(&pointer).and_then(Value::as_f64) else {
                continue;
            };
            let value = if field.path == "fuel.initial_kg" {
                config.fuel.current_kg
            } else {
                stored
            };
            text.push_str(&format!(
                "FIELD path={} label=\"{}\" value={:.6} unit=\"{}\" min={:.6} max={:.6} step={:.6} editable={}\n",
                field.path,
                field.label,
                value,
                field.unit,
                field.minimum,
                field.maximum,
                field.step,
                i32::from(field.editable),
            ));
        }
        if let Some(geometry) = config.suspension.geometric_suspension.as_ref() {
            for wheel in WheelIndex::ALL {
                let axle = geometry.axle(wheel);
                let rest_compression_m = crate::suspension::rest_compression_m(config, wheel);
                let spring_length_m = if wheel.is_front() {
                    config.suspension.front_spring_length
                } else {
                    config.suspension.rear_spring_length
                };
                let tire_radius_m = if wheel.is_front() {
                    config.tires.front_tire_radius
                } else {
                    config.tires.rear_tire_radius
                };
                text.push_str(&format!(
                    "DERIVED wheel={:?} rest_compression_m={:.6} spring_length_m={:.6} spring_free_length_m={:.6} spring_installed_length_m={:.6} preload_m={:.6} spring_rate_N_per_m={:.3} damper_bump_Ns_per_m={:.3} damper_rebound_Ns_per_m={:.3} damper_knee_m_per_s={:.6} damper_fast_factor={:.6} droop_m={:.6} bump_m={:.6} tire_radius_m={:.6}\n",
                    wheel,
                    rest_compression_m,
                    spring_length_m,
                    axle.spring_free_length_m,
                    axle.spring_installed_length_m,
                    axle.spring_free_length_m - axle.spring_installed_length_m,
                    axle.spring_rate_N_per_m,
                    axle.damper_bump_Ns_per_m,
                    axle.damper_rebound_Ns_per_m,
                    axle.damper_knee_m_per_s,
                    axle.damper_fast_factor,
                    axle.wheel_droop_m,
                    axle.wheel_bump_m,
                    tire_radius_m,
                ));
            }
            for front in [true, false] {
                text.push_str(&format!(
                    "DERIVED axle={} arb_ratio={:.6} arb_config={:?}\n",
                    if front { "front" } else { "rear" },
                    if front {
                        config.suspension.front_arb_ratio
                    } else {
                        config.suspension.rear_arb_ratio
                    },
                    geometry.arb(front),
                ));
            }
        }
        text.push_str(&format!(
            "DERIVED mass_kg={:.3} fuel_kg={:.3} front_weight_fraction={:.6} brake_bias_front={:.6}\n",
            self.simulator.config.chassis.vehicle_mass,
            config.fuel.current_kg,
            config.chassis.front_weight_distribution,
            config.brakes.front_brake_bias,
        ));
        // The trims the axle solver actually evaluates (damper multipliers and bar),
        // not the design coefficients the garage text stated.
        text.push_str(&format!(
            "DERIVED effective_elements={}\n",
            self.effective_suspension_summary()
        ));
        text
    }

    pub fn setup_fields_text(&self) -> String {
        let mut text = String::new();
        for field in SETUP_FIELDS {
            let pointer = format!("/{}", field.path.replace('.', "/"));
            if let Some(mut value) = self.document.pointer(&pointer).and_then(Value::as_f64) {
                if field.path == "fuel.initial_kg" { value = self.simulator.config.fuel.current_kg; }
                text.push_str(&format!("{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
                    field.path, field.label, field.unit, value, field.minimum, field.maximum,
                    field.step, if field.editable { 1 } else { 0 }));
            }
        }
        text
    }

    pub fn setup_document_json(&self) -> String {
        serde_json::to_string(&serde_json::json!({
            "document": self.document,
            "base": self.base_document,
            "fields": setup_descriptor_value(),
            // The elements the active axle solver actually evaluates, after the setup
            // translation. The authoring document keeps the design coefficients and
            // the garage trims; this is what support needs to compare them.
            "effective_suspension": self.effective_suspension_summary(),
        }))
        .unwrap_or_else(|_| "{}".to_string())
    }

    /// Suspension authority for one running car: the trim the garage asked for, the
    /// geometric elements the active axle solver actually evaluates, and the bar both
    /// representations state. The authoring document keeps the design coefficients, so
    /// this is what support compares a reglaje against.
    pub fn effective_suspension_summary(&self) -> Value {
        let config = &self.simulator.config;
        let axle = |front: bool| {
            let trim = crate::suspension_setup::AxleSetupTrim::of_axle(config, front);
            let elements = SuspensionSetupAuthority::effective_elements(config, front);
            let geometric_bar = config
                .suspension
                .geometric_suspension
                .as_ref()
                .and_then(|geometry| geometry.arb(front).legacy_ratio());
            serde_json::json!({
                "damping_ratio": trim.damping_ratio,
                "bump_damp_multiplier": trim.bump_multiplier,
                "rebound_damp_multiplier": trim.rebound_multiplier,
                "bump_stop_multiplier": trim.bump_stop_multiplier,
                "damper_bump_Ns_per_m": elements.map(|elements| elements.damper_bump_Ns_per_m),
                "damper_rebound_Ns_per_m": elements.map(|elements| elements.damper_rebound_Ns_per_m),
                "effective_bump_stop_mult": elements.map(|elements| elements.bump_stop_mult),
                "arb_ratio": if front { config.suspension.front_arb_ratio } else { config.suspension.rear_arb_ratio },
                "arb_geometric_ratio": geometric_bar,
            })
        };
        serde_json::json!({ "front": axle(true), "rear": axle(false) })
    }

    pub fn apply_setup_json(&mut self, text: &str) -> Result<(), String> {
        let patch: Value = serde_json::from_str(text)
            .map_err(|error| format!("setup patch is not valid JSON: {error}"))?;
        let object = patch
            .as_object()
            .ok_or_else(|| "setup patch must be a JSON object of dotted paths".to_string())?;
        if object.is_empty() {
            return Err("setup patch is empty".into());
        }
        let mut candidate = self.document.clone();
        for (path, value) in object {
            if !field_is_editable(path) {
                return Err(format!("setup path {path} is read-only or unknown"));
            }
            if !value.is_number() {
                return Err(format!("setup path {path} requires a numeric value"));
            }
            let descriptor = SETUP_FIELDS.iter().find(|f| f.path == path).unwrap();
            let number = value.as_f64().ok_or_else(|| format!("invalid number for {path}"))?;
            if !number.is_finite() || number < descriptor.minimum || number > descriptor.maximum {
                return Err(format!("setup path {path} is outside its limits"));
            }
            set_document_path(&mut candidate, path, value.clone())?;
        }
        let text = serde_json::to_string(&candidate)
            .map_err(|error| format!("cannot serialize the candidate setup: {error}"))?;
        let config = VehicleConfig::from_json_str(&text)
            .map_err(|error| format!("Rust rejected the setup: {error}"))?;
        validate_production_configuration(&config)?;
        let previous = self.simulator.config.clone();
        let mut config = config;
        // The ratio knob the garage writes is the bar the axles run on, and the
        // requested trims become the geometric coefficients the active solver
        // evaluates. Both are absolute over the loaded profile, so replaying this
        // transaction any number of times lands on the same numbers.
        adopt_antiroll_knob(&mut config);
        if let Some(authority) = &self.setup_authority {
            if let Err(error) = authority.apply(&mut config) {
                return Err(error);
            }
        }
        config.fuel.current_kg = if object.contains_key("fuel.initial_kg") {
            config.fuel.initial_kg.min(config.fuel.capacity_kg)
        } else { previous.fuel.current_kg.min(config.fuel.capacity_kg) };
        self.simulator.config = config;
        self.simulator
            .state
            .suspension
            .rebuild_geometric_prepared(&self.simulator.config);
        if let Err(error) = self.configure_floor_probes() {
            self.simulator.config = previous;
            self.simulator
                .state
                .suspension
                .rebuild_geometric_prepared(&self.simulator.config);
            return Err(error);
        }
        self.document = candidate;
        Ok(())
    }

    pub fn reset_setup(&mut self) -> Result<(), String> {
        let mut restored = self.base_document.clone();
        adopt_antiroll_design_knobs(&mut restored);
        let text = serde_json::to_string(&restored)
            .map_err(|error| format!("cannot serialize the base setup: {error}"))?;
        let config = VehicleConfig::from_json_str(&text)
            .map_err(|error| format!("Rust rejected the base setup: {error}"))?;
        validate_production_configuration(&config)?;
        let previous = self.simulator.config.clone();
        let mut config = config;
        adopt_antiroll_design_bar(&mut config);
        if let Some(authority) = &self.setup_authority {
            authority.apply(&mut config)?;
        }
        self.simulator.config = config;
        self.simulator
            .state
            .suspension
            .rebuild_geometric_prepared(&self.simulator.config);
        if let Err(error) = self.configure_floor_probes() {
            self.simulator.config = previous;
            self.simulator
                .state
                .suspension
                .rebuild_geometric_prepared(&self.simulator.config);
            return Err(error);
        }
        self.document = restored;
        Ok(())
    }

    pub fn pit_fuel(&mut self, kilograms: f64) -> Result<(), String> {
        if !kilograms.is_finite() || kilograms < 0.0 {
            return Err("refuel amount must be finite and non-negative".into());
        }
        let capacity = self.simulator.config.fuel.capacity_kg;
        let target = kilograms.min(capacity);
        self.simulator.config.fuel.current_kg = target;
        self.simulator.config.fuel.consumed_kg = 0.0;
        Ok(())
    }

    pub fn pit_tires(&mut self) {
        self.simulator.replace_tire_set();
    }

    pub fn set_error(&mut self, message: &str) {
        self.last_error = message.to_string();
    }

    pub fn geometry(&self) -> VehicleGeometry {
        let config = &self.simulator.config;
        let backend = self.simulator.state.suspension.backend;
        VehicleGeometry {
            center_of_mass: crate::simulation::center_of_mass_local_for_suspension_backend(
                config, backend,
            ),
            inertia: crate::simulation::principal_inertia_for_suspension_backend(config, backend),
            spawn_height: default_spawn_height(config),
            mass: config.chassis.total_vehicle_mass(&config.tires, &config.fuel),
        }
    }
}

/// Vehicle-mass geometry published to the host for placement and presentation.
#[derive(Debug, Clone, Copy)]
pub struct VehicleGeometry {
    pub center_of_mass: Vec3,
    pub inertia: Vec3,
    pub spawn_height: f64,
    pub mass: f64,
}

/// Resolves a domain manifest into the single assembly document the setup transaction
/// and the garage inspector operate on. A flat assembly document is used as-is.
fn resolve_profile_document(path: &Path) -> Result<Value, String> {
    let text = std::fs::read_to_string(path)
        .map_err(|error| format!("cannot read {}: {error}", path.display()))?;
    let document: Value = serde_json::from_str(&text)
        .map_err(|error| format!("cannot parse {}: {error}", path.display()))?;
    if document.get("domains").is_none() {
        return Ok(document);
    }
    let directory = path
        .parent()
        .ok_or_else(|| format!("profile {} has no directory", path.display()))?;
    let domains = document
        .get("domains")
        .and_then(Value::as_object)
        .ok_or_else(|| "profile manifest domains must be an object".to_string())?;
    let mut merged = serde_json::Map::new();
    merged.insert("schema_version".to_string(), serde_json::json!(3));
    merged.insert(
        "vehicle_id".to_string(),
        document.get("vehicle_identifier").cloned().unwrap_or(Value::Null),
    );
    merged.insert(
        "profile_name".to_string(),
        document.get("profile_name").cloned().unwrap_or(Value::Null),
    );
    merged.insert("units".to_string(), document.get("units").cloned().unwrap_or(Value::Null));
    merged.insert(
        "coordinate_contract".to_string(),
        document.get("coordinate_contract").cloned().unwrap_or(Value::Null),
    );
    let section_map: [(&str, &[&str]); 11] = [
        ("chassis", &["chassis", "geometry", "physical_mass_inventory"]),
        ("fuel", &["fuel"]),
        ("steering", &["steering"]),
        ("powertrain", &["powertrain"]),
        ("suspension", &["suspension"]),
        ("tires", &["tires"]),
        ("brakes", &["brakes"]),
        ("aerodynamics", &["aero"]),
        ("driving_assistance", &["aids"]),
        ("underfloor_contact", &["underfloor_contact"]),
        ("coupled_world", &["coupled_world"]),
    ];
    for (domain_name, section_names) in section_map {
        let reference = domains
            .get(domain_name)
            .and_then(Value::as_str)
            .ok_or_else(|| format!("profile manifest domains.{domain_name} is required"))?;
        let domain_path = directory.join(reference);
        let domain_text = std::fs::read_to_string(&domain_path)
            .map_err(|error| format!("cannot read {}: {error}", domain_path.display()))?;
        let domain_document: Value = serde_json::from_str(&domain_text)
            .map_err(|error| format!("cannot parse {}: {error}", domain_path.display()))?;
        for section_name in section_names {
            if let Some(value) = domain_document.get(*section_name) {
                if !value.is_null() {
                    merged.insert((*section_name).to_string(), value.clone());
                }
            }
        }
    }
    Ok(Value::Object(merged))
}

fn vec3(value: Vec3) -> OfpVec3 {
    OfpVec3 {
        x: value.x,
        y: value.y,
        z: value.z,
    }
}

/// Maps one corner's diagnostics onto the public ABI record. Pure translation: no
/// clamp, no sign change, no unit conversion (the ABI is SI).
fn wheel_diagnostics(
    source: &crate::vertical_diagnostics::WheelVerticalDiagnostics,
) -> OfpWheelDiagnostics {
    OfpWheelDiagnostics {
        ground_detected: i32::from(source.ground_detected),
        ground_binding: i32::from(source.ground_binding),
        droop_stop_active: i32::from(source.droop_stop_active),
        bump_stop_active: i32::from(source.bump_stop_active),
        trapped_at_stop: i32::from(source.trapped_at_stop),
        reserved0: 0,
        compression_m: source.compression_m,
        ground_separation_m: source.ground_separation_m,
        ground_separation_normal_m: source.ground_separation_normal_m,
        ray_shortfall_m: source.ray_shortfall_m,
        ground_demand_m: source.ground_demand_m,
        ground_demand_predicted_m: source.ground_demand_predicted_m,
        max_compression_m: source.max_compression_m,
        min_compression_m: source.min_compression_m,
        headroom_to_bump_m: source.headroom_to_bump_m,
        travel_m: source.travel_m,
        axial_velocity_m_s: source.axial_velocity_m_s,
        spring_force_n: source.spring_force_n,
        damper_force_n: source.damper_force_n,
        antiroll_force_n: source.antiroll_force_n,
        stop_force_n: source.stop_force_n,
        motion_ratio: source.motion_ratio,
        element_force_sum_n: source.element_force_sum_n,
        contact_impulse_ns: source.contact_impulse_ns,
        droop_stop_impulse_ns: source.droop_stop_impulse_ns,
        bump_stop_impulse_ns: source.bump_stop_impulse_ns,
        anchor_position: vec3(source.anchor_position),
        anchor_velocity: vec3(source.anchor_velocity),
        anchor_acceleration: vec3(source.anchor_acceleration),
        axial_anchor_acceleration_m_s2: source.axial_anchor_acceleration_m_s2,
    }
}

fn surface_code(surface: SurfaceType) -> i32 {
    match surface {
        SurfaceType::Road => 0,
        SurfaceType::Curb => 1,
        SurfaceType::Dirt => 2,
        SurfaceType::Grass => 3,
        SurfaceType::Gravel => 4,
        SurfaceType::Sand => 5,
        SurfaceType::Wall => 6,
        SurfaceType::Metal => 7,
    }
}

#[cfg(test)]
mod audio_presentation_tests {
    use super::*;
    use crate::open_formula_ffi::{OFP_ABI_VERSION, OfpControlInput};

    fn neutral_input() -> OfpControlInput {
        OfpControlInput {
            throttle: 0.15,
            brake: 0.0,
            handbrake: 0.0,
            clutch: 0.0,
            steering: 0.0,
            gear_request: 1,
            reserved: 0,
        }
    }

    #[test]
    fn abi_is_v6_and_specs_carry_audio_references() {
        assert_eq!(OFP_ABI_VERSION, 6);
        let simulation =
            OpenFormulaSimulation::create(None, Vec3::ZERO, 0.0, None).expect("williams profile");
        let specs = simulation.vehicle_specs();
        assert_eq!(specs.idle_rpm, 3_500.0);
        assert_eq!(specs.max_rpm, 17_500.0);
        assert!(specs.max_torque_nm > 100.0);
        assert_eq!(specs.front_torque_split, 0.0);
        assert_eq!(specs.final_drive, 4.65);
    }

    #[test]
    fn accepted_steps_publish_sequence_dt_and_raw_mechanical_state() {
        let mut simulation =
            OpenFormulaSimulation::create(None, Vec3::ZERO, 0.0, None).expect("williams profile");
        let input = neutral_input();
        let empty: &[crate::open_formula_ffi::OfpBodyContact] = &[];
        simulation.step(&input, empty, 0.01).expect("step 1");
        simulation.step(&input, empty, 0.01).expect("step 2");
        let snapshot = simulation.snapshot();
        assert_eq!(snapshot.abi_version, 6);
        assert_eq!(snapshot.sequence, 2);
        assert!((snapshot.step_dt_s - 0.01).abs() < 1e-12);
        // Raw mechanical fields come from the live powertrain state.
        assert!(snapshot.engine_rpm.is_finite());
        assert!(snapshot.clutch_torque_nm.is_finite());
        assert!(snapshot.tc_cut_ratio_smoothed >= 0.0 && snapshot.tc_cut_ratio_smoothed <= 1.0);
        assert!(snapshot.shift_timer_s >= 0.0);
        assert!(snapshot.is_rev_limited <= 1);
        // Williams RWD: rear spins exist; output-shaft math stays in the audio wrapper.
        let rear_mean =
            (snapshot.wheels[2].spin_rad_s + snapshot.wheels[3].spin_rad_s) * 0.5;
        assert!(rear_mean.is_finite());
    }

    #[test]
    fn low_fps_step_keeps_its_own_clock_for_cut_windows() {
        // A 50 ms graphic frame is one accepted step with dt 0.05: the packet
        // clock must be 0.05 (not an interpolated frame time) so limiter/shift
        // windows resolved here are never replaced by a later frame.
        let mut simulation =
            OpenFormulaSimulation::create(None, Vec3::ZERO, 0.0, None).expect("williams profile");
        let input = neutral_input();
        let empty: &[crate::open_formula_ffi::OfpBodyContact] = &[];
        simulation.step(&input, empty, 0.05).expect("low-fps step");
        let snapshot = simulation.snapshot();
        assert_eq!(snapshot.sequence, 1);
        assert!((snapshot.step_dt_s - 0.05).abs() < 1e-12);
        assert_eq!(snapshot.physics_hz, 20);
    }
}
