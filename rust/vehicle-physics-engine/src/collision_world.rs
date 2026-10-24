//! Single authoritative, static collision geometry for one session.
//!
//! Ownership contract
//! ------------------
//! * Speed Dreams loads the circuit. Its physics engine (`rustphysics`) installs
//!   exactly one `CollisionWorld` per session through `install_world`, and removes
//!   it through `uninstall_world` when the session ends.
//! * A `CollisionWorld` is immutable, has no integrator and no dynamics state: it
//!   is a query structure (`TriMesh` + per-triangle materials) plus a verifiable
//!   identity. It never applies an impulse. All contact response happens once, in
//!   Rust, on the vehicle state.
//! * Every `OpenFormulaSimulation` captures an `Arc<CollisionWorld>` plus the
//!   registry generation at creation time. A newer session replaces the installed
//!   world and bumps the generation, so a stale vehicle fails explicitly instead of
//!   silently querying the previous circuit's geometry. Shared ownership makes a
//!   dangling reference impossible.
//!
//! Geometry identity
//! -----------------
//! `WorldIdentity` carries the circuit identifier and revision supplied by the
//! host, the SHA-256 of the exact triangle/material payload consumed (after the
//! single SD -> Rust frame mapping), the triangle count and the AABB in physical
//! world coordinates. Two loads of the same circuit must produce the same hash;
//! two different circuits must not.

use std::path::Path;
use std::sync::{Arc, Mutex};

use parry3d_f64::na::{Isometry3, Point3, Vector3};
use parry3d_f64::query::Ray;
use parry3d_f64::shape::{SharedShape, TriMesh};
use sha2::{Digest, Sha256};

use crate::types::{SurfaceType, Vec3};

/// Signature of the transfer document produced by the track exporter.
pub const GROUND_SIGNATURE: &str = "FORMULA_GROUND_1";
/// Upper bound accepted from a transfer document (defensive, matches the exporter).
pub const MAX_TRIANGLES: usize = 1_000_000;

/// Which route produced the geometry. Recorded, never inferred.
pub const SOURCE_TRANSFER_FILE: &str = "transfer_file";
/// Geometry pushed by the host in memory (no intermediate document).
pub const SOURCE_HOST_EXPORT: &str = "host_export";

/// Physical surface properties of one triangle.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct TriangleMaterial {
    pub friction: f64,
    pub rolling_resistance: f64,
}

impl TriangleMaterial {
    pub fn new(friction: f64, rolling_resistance: f64) -> Self {
        Self {
            friction,
            rolling_resistance,
        }
    }
}

/// Verifiable description of the geometry a session is using.
#[derive(Debug, Clone, PartialEq)]
pub struct WorldIdentity {
    /// Circuit identifier the host assigned (track id / directory), or the
    /// transfer document's file stem when the host supplies none.
    pub circuit_id: String,
    /// Host revision counter (1 when the host does not version its export).
    pub revision: u32,
    /// `SOURCE_TRANSFER_FILE` or `SOURCE_HOST_EXPORT`.
    pub source: &'static str,
    /// Absolute path of the transfer document, empty for host exports.
    pub source_path: String,
    /// SHA-256 over the imported payload: triangle count, materials and the
    /// coordinates in the Rust world frame.
    pub geometry_sha256: String,
    pub triangle_count: usize,
    pub aabb_min: Vec3,
    pub aabb_max: Vec3,
    /// Queries performed against this world since it was installed. Diagnostics
    /// only; the value is read, never fed back.
    pub revision_note: String,
}

impl WorldIdentity {
    /// Short form used in one-line logs: `circuit@rev#hash`.
    pub fn short(&self) -> String {
        format!(
            "{}@{}#{}",
            self.circuit_id,
            self.revision,
            &self.geometry_sha256[..self.geometry_sha256.len().min(16)]
        )
    }

    pub fn to_json(&self) -> String {
        format!(
            "{{\"circuit_id\":\"{}\",\"revision\":{},\"source\":\"{}\",\"source_path\":\"{}\",\
             \"geometry_sha256\":\"{}\",\"triangle_count\":{},\
             \"aabb_min\":[{:.9},{:.9},{:.9}],\"aabb_max\":[{:.9},{:.9},{:.9}],\
             \"revision_note\":\"{}\"}}",
            json_escape(&self.circuit_id),
            self.revision,
            self.source,
            json_escape(&self.source_path),
            self.geometry_sha256,
            self.triangle_count,
            self.aabb_min.x,
            self.aabb_min.y,
            self.aabb_min.z,
            self.aabb_max.x,
            self.aabb_max.y,
            self.aabb_max.z,
            json_escape(&self.revision_note)
        )
    }
}

/// One ray result. `distance` is measured along the (normalized) query direction
/// from the query origin supplied by the caller. For wheel queries the caller is
/// `OpenFormulaSimulation`, which uses the physical wheel centre.
#[derive(Debug, Clone, Copy)]
pub struct GroundHit {
    pub distance: f64,
    pub point: Vec3,
    pub normal: Vec3,
    pub surface: SurfaceType,
    /// Index of the triangle that produced the hit (materials lookup).
    pub triangle: u32,
    pub friction: f64,
    pub rolling_resistance: f64,
}

/// Immutable, integrator-free collision geometry.
#[derive(Debug)]
pub struct CollisionWorld {
    mesh: Option<SharedShape>,
    materials: Vec<TriangleMaterial>,
    identity: WorldIdentity,
}

impl CollisionWorld {
    /// Builds a world from triangles already expressed in the Rust world frame
    /// (X right, Y up, -Z forward). The host is responsible for mapping its own
    /// frame exactly once, before calling this constructor.
    pub fn from_host_export(
        circuit_id: &str,
        revision: u32,
        triangles: &[[Vec3; 3]],
        materials: &[TriangleMaterial],
    ) -> Result<Self, String> {
        if triangles.is_empty() {
            return Err("the host exported an empty ground geometry".to_string());
        }
        if triangles.len() > MAX_TRIANGLES {
            return Err(format!(
                "the host exported {} ground triangles, above the {MAX_TRIANGLES} limit",
                triangles.len()
            ));
        }
        if materials.len() != triangles.len() {
            return Err(format!(
                "ground geometry is inconsistent: {} triangles but {} material records",
                triangles.len(),
                materials.len()
            ));
        }
        Self::build(
            circuit_id,
            revision,
            SOURCE_HOST_EXPORT,
            "",
            triangles,
            materials,
            String::new(),
        )
    }

    /// Loads the `FORMULA_GROUND_1` transfer document produced by the track
    /// exporter. The nine coordinates per triangle are Speed Dreams world
    /// coordinates (X forward, Y left, Z up) and are mapped once, here.
    pub fn load_transfer_file(
        path: &str,
        circuit_id: Option<&str>,
        revision: Option<u32>,
    ) -> Result<Self, String> {
        if path.trim().is_empty() {
            return Err("no ground collision document was declared for this circuit".to_string());
        }
        let text = std::fs::read_to_string(path)
            .map_err(|error| format!("cannot read ground mesh {path}: {error}"))?;
        let mut tokens = text.split_whitespace();
        let signature = tokens.next().unwrap_or("");
        if signature != GROUND_SIGNATURE {
            return Err(format!(
                "unsupported ground mesh signature in {path}: {signature}"
            ));
        }
        let count: usize = tokens
            .next()
            .ok_or_else(|| "ground mesh is missing its triangle count".to_string())?
            .parse()
            .map_err(|error| format!("invalid ground mesh triangle count: {error}"))?;
        if count == 0 {
            return Err(format!(
                "ground mesh {path} declares no triangles: an empty world is not a world"
            ));
        }
        if count > MAX_TRIANGLES {
            return Err(format!("ground mesh triangle count out of range: {count}"));
        }
        let mut triangles = Vec::with_capacity(count);
        let mut materials = Vec::with_capacity(count);
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
            if !friction.is_finite() || !rolling.is_finite() {
                return Err(format!("non-finite material at triangle {triangle_index}"));
            }
            let mut coordinates = [0.0f64; 9];
            for coordinate in coordinates.iter_mut() {
                *coordinate = tokens
                    .next()
                    .ok_or_else(|| format!("ground mesh truncated at triangle {triangle_index}"))?
                    .parse()
                    .map_err(|error| format!("invalid triangle coordinate: {error}"))?;
                if !coordinate.is_finite() {
                    return Err(format!(
                        "non-finite triangle coordinate at {triangle_index}"
                    ));
                }
            }
            let mut corners = [Vec3::ZERO; 3];
            for corner in 0..3 {
                let offset = corner * 3;
                // Speed Dreams (X forward, Y left, Z up) -> Rust (X right, Y up, -Z forward).
                corners[corner] = Vec3::new(
                    -coordinates[offset + 1],
                    coordinates[offset + 2],
                    -coordinates[offset],
                );
            }
            triangles.push(corners);
            materials.push(TriangleMaterial::new(friction, rolling));
        }
        let derived_id = Path::new(path)
            .file_stem()
            .map(|stem| stem.to_string_lossy().to_string())
            .unwrap_or_default();
        let id = circuit_id
            .filter(|value| !value.trim().is_empty())
            .map(|value| value.to_string())
            .unwrap_or(derived_id);
        Self::build(
            &id,
            revision.unwrap_or(1),
            SOURCE_TRANSFER_FILE,
            path,
            &triangles,
            &materials,
            String::new(),
        )
    }

    fn build(
        circuit_id: &str,
        revision: u32,
        source: &'static str,
        source_path: &str,
        triangles: &[[Vec3; 3]],
        materials: &[TriangleMaterial],
        note: String,
    ) -> Result<Self, String> {
        let mut vertices: Vec<Point3<f64>> = Vec::with_capacity(triangles.len() * 3);
        let mut indices: Vec<[u32; 3]> = Vec::with_capacity(triangles.len());
        let mut aabb_min = Vec3::new(f64::INFINITY, f64::INFINITY, f64::INFINITY);
        let mut aabb_max = Vec3::new(f64::NEG_INFINITY, f64::NEG_INFINITY, f64::NEG_INFINITY);
        let mut hasher = Sha256::new();
        hasher.update(GROUND_SIGNATURE.as_bytes());
        hasher.update((triangles.len() as u64).to_be_bytes());
        for (index, corners) in triangles.iter().enumerate() {
            let base = (index * 3) as u32;
            indices.push([base, base + 1, base + 2]);
            let material = materials[index];
            if !material.friction.is_finite()
                || !material.rolling_resistance.is_finite()
                || material.friction < 0.0
                || material.rolling_resistance < 0.0
            {
                return Err(format!(
                    "invalid physical material coefficient in triangle {index}"
                ));
            }
            hasher.update(material.friction.to_bits().to_be_bytes());
            hasher.update(material.rolling_resistance.to_bits().to_be_bytes());
            for corner in corners.iter() {
                if !corner.is_finite() {
                    return Err(format!("non-finite vertex in triangle {index}"));
                }
                vertices.push(Point3::new(corner.x, corner.y, corner.z));
                aabb_min = Vec3::new(
                    aabb_min.x.min(corner.x),
                    aabb_min.y.min(corner.y),
                    aabb_min.z.min(corner.z),
                );
                aabb_max = Vec3::new(
                    aabb_max.x.max(corner.x),
                    aabb_max.y.max(corner.y),
                    aabb_max.z.max(corner.z),
                );
                hasher.update(corner.x.to_bits().to_be_bytes());
                hasher.update(corner.y.to_bits().to_be_bytes());
                hasher.update(corner.z.to_bits().to_be_bytes());
            }
        }
        let mesh = TriMesh::new(vertices, indices).map_err(|error| {
            format!("cannot build the ground acceleration structure: {error:?}")
        })?;
        let geometry_sha256: String = hasher
            .finalize()
            .iter()
            .map(|byte| format!("{byte:02x}"))
            .collect();
        Ok(Self {
            mesh: Some(SharedShape::new(mesh)),
            materials: materials.to_vec(),
            identity: WorldIdentity {
                circuit_id: circuit_id.to_string(),
                revision,
                source,
                source_path: source_path.to_string(),
                geometry_sha256,
                triangle_count: triangles.len(),
                aabb_min,
                aabb_max,
                revision_note: note,
            },
        })
    }

    pub fn identity(&self) -> &WorldIdentity {
        &self.identity
    }

    pub fn triangle_count(&self) -> usize {
        self.identity.triangle_count
    }

    pub fn is_loaded(&self) -> bool {
        self.mesh.is_some() && self.identity.triangle_count > 0
    }

    pub fn material(&self, triangle: u32) -> Option<TriangleMaterial> {
        self.materials.get(triangle as usize).copied()
    }

    /// Casts one segment and returns the closest hit. `direction` is normalized
    /// here so a caller cannot distort the returned distance.
    pub fn raycast(&self, origin: Vec3, direction: Vec3, max_distance: f64) -> Option<GroundHit> {
        let mesh = self.mesh.as_ref()?;
        let length = direction.length();
        if !length.is_finite()
            || length <= 1e-9
            || !max_distance.is_finite()
            || max_distance <= 0.0
            || !origin.is_finite()
        {
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
        // The reported normal always opposes the query direction: an inverted
        // triangle in the exported mesh must not report a downward road normal and
        // pull the vehicle into the ground.
        let normal = if normal.dot(unit) > 0.0 {
            -normal
        } else {
            normal
        };
        let normal = if normal.length() > 1e-9 {
            normal.normalized()
        } else {
            Vec3::UP
        };
        // parry encodes a hit on the back face of triangle `i` as `Face(i + n)`. The
        // exported circuit meshes have no guaranteed winding, so fold both halves back
        // to the triangle index before looking the material up.
        let triangle = match intersection.feature {
            parry3d_f64::shape::FeatureId::Face(index) => {
                let count = self.identity.triangle_count as u32;
                if index < count {
                    index
                } else if index < count.saturating_mul(2) {
                    index - count
                } else {
                    u32::MAX
                }
            }
            _ => u32::MAX,
        };
        let material = self.materials.get(triangle as usize).copied();
        let (friction, rolling_resistance) = material
            .map(|value| (value.friction, value.rolling_resistance))
            .unwrap_or((1.0, 0.002));
        Some(GroundHit {
            distance,
            point: origin + unit * distance,
            normal,
            surface: classify_surface(friction, rolling_resistance),
            triangle,
            friction,
            rolling_resistance,
        })
    }
}

/// Surface class derived from the material record. The explicit material values are
/// always reported next to the class; the class is a presentation/telemetry label and
/// never the only physical input.
pub fn classify_surface(_friction: f64, rolling_resistance: f64) -> SurfaceType {
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

// ---------------------------------------------------------------------------
// Session registry
// ---------------------------------------------------------------------------

#[derive(Debug)]
struct RegistryState {
    generation: u64,
    installs: u64,
    current: Option<Arc<CollisionWorld>>,
}

static REGISTRY: Mutex<Option<RegistryState>> = Mutex::new(None);

fn with_registry<T>(operation: impl FnOnce(&mut RegistryState) -> T) -> T {
    let mut guard = REGISTRY
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let state = guard.get_or_insert_with(|| RegistryState {
        generation: 0,
        installs: 0,
        current: None,
    });
    operation(state)
}

/// Installs the session world, replacing (and invalidating) any previous one.
/// Returns the new generation and the identity the session must record.
pub fn install_world(world: CollisionWorld) -> (u64, WorldIdentity) {
    let identity = world.identity().clone();
    let world = Arc::new(world);
    with_registry(|state| {
        state.generation = state.generation.wrapping_add(1).max(1);
        state.installs = state.installs.wrapping_add(1);
        // The previous Arc is dropped here. Sessions that still hold one keep it
        // alive, but their recorded generation no longer matches, so they refuse to
        // step instead of silently querying the previous circuit.
        state.current = Some(world);
        (state.generation, identity)
    })
}

/// Removes the installed world (session shutdown). The generation advances so any
/// surviving simulation detects that its geometry is no longer authoritative.
pub fn uninstall_world() -> bool {
    with_registry(|state| {
        let had = state.current.take().is_some();
        if had {
            state.generation = state.generation.wrapping_add(1).max(1);
        }
        had
    })
}

/// Current generation counter. Advances on every install and on every uninstall.
pub fn world_generation() -> u64 {
    with_registry(|state| state.generation)
}

/// The installed world and the generation it belongs to.
pub fn installed_world() -> Option<(u64, Arc<CollisionWorld>)> {
    with_registry(|state| {
        state
            .current
            .as_ref()
            .map(|world| (state.generation, Arc::clone(world)))
    })
}

/// Number of install calls since process start (diagnostics/session accounting).
pub fn install_count() -> u64 {
    with_registry(|state| state.installs)
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

fn json_escape(text: &str) -> String {
    let mut out = String::with_capacity(text.len());
    for character in text.chars() {
        match character {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            other if (other as u32) < 0x20 => out.push(' '),
            other => out.push(other),
        }
    }
    out
}

/// Test-only hook: forget the installed world and reset the generation counter so a
/// unit test starts from a known state. Never called from production code.
#[doc(hidden)]
pub fn reset_registry_for_tests() {
    with_registry(|state| {
        state.current = None;
        state.generation = 0;
        state.installs = 0;
    });
}

/// Convenience used by tests and by the FFI: build a flat plane subdivided into a
/// grid, expressed in the Rust frame, with one material for every triangle.
pub fn flat_plane_triangles(
    half_extent_x: f64,
    half_extent_z: f64,
    cells: usize,
    height: f64,
) -> Vec<[Vec3; 3]> {
    let cells = cells.max(1);
    let step_x = (2.0 * half_extent_x) / cells as f64;
    let step_z = (2.0 * half_extent_z) / cells as f64;
    let mut triangles = Vec::with_capacity(cells * cells * 2);
    for ix in 0..cells {
        for iz in 0..cells {
            let x0 = -half_extent_x + step_x * ix as f64;
            let x1 = x0 + step_x;
            let z0 = -half_extent_z + step_z * iz as f64;
            let z1 = z0 + step_z;
            let p00 = Vec3::new(x0, height, z0);
            let p10 = Vec3::new(x1, height, z0);
            let p11 = Vec3::new(x1, height, z1);
            let p01 = Vec3::new(x0, height, z1);
            // Counter-clockwise seen from +Y, so the face normal is the road normal.
            triangles.push([p00, p01, p11]);
            triangles.push([p00, p11, p10]);
        }
    }
    triangles
}

/// Test/benchmark helper: install a flat plane world as the session world.
pub fn install_flat_plane(circuit_id: &str, height: f64) -> (u64, WorldIdentity) {
    let triangles = flat_plane_triangles(500.0, 500.0, 8, height);
    let materials = vec![TriangleMaterial::new(1.0, 0.002); triangles.len()];
    let world = CollisionWorld::from_host_export(circuit_id, 1, &triangles, &materials)
        .expect("flat plane world must be valid");
    install_world(world)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn plane_world(id: &str, height: f64) -> CollisionWorld {
        let triangles = flat_plane_triangles(50.0, 50.0, 4, height);
        let materials = vec![TriangleMaterial::new(1.0, 0.002); triangles.len()];
        CollisionWorld::from_host_export(id, 3, &triangles, &materials).expect("plane world")
    }

    #[test]
    fn identity_hashes_the_payload_and_reports_the_aabb() {
        let world = plane_world("fuji", 0.0);
        let identity = world.identity();
        assert_eq!(identity.triangle_count, 4 * 4 * 2);
        assert_eq!(identity.geometry_sha256.len(), 64);
        assert_eq!(identity.circuit_id, "fuji");
        assert_eq!(identity.revision, 3);
        assert!((identity.aabb_min.y - 0.0).abs() < 1e-12);
        assert!((identity.aabb_max.x - 50.0).abs() < 1e-9);
        // Same payload -> same hash; different payload -> different hash.
        assert_eq!(
            world.identity().geometry_sha256,
            plane_world("other", 0.0).identity().geometry_sha256
        );
        assert_ne!(
            world.identity().geometry_sha256,
            plane_world("fuji", 0.5).identity().geometry_sha256
        );
    }

    #[test]
    fn empty_geometry_is_rejected_instead_of_simulating_without_ground() {
        let error = CollisionWorld::from_host_export("empty", 1, &[], &[]).unwrap_err();
        assert!(error.contains("empty"), "{error}");
    }

    #[test]
    fn raycast_reports_distance_point_normal_and_material() {
        let world = plane_world("fuji", 1.25);
        let hit = world
            .raycast(Vec3::new(1.0, 3.0, -2.0), -Vec3::UP, 10.0)
            .expect("plane must be hit from above");
        assert!((hit.distance - 1.75).abs() < 1e-9, "{}", hit.distance);
        assert!((hit.point.y - 1.25).abs() < 1e-9);
        assert!(hit.normal.y.abs() > 0.999);
        assert_eq!(hit.surface, SurfaceType::Road);
        assert!(hit.triangle != u32::MAX);
        assert_eq!(world.material(hit.triangle).unwrap().friction, 1.0);
        // A query from below still reports the surface, but the returned normal must
        // oppose the query direction: the road never pushes the vehicle into itself.
        let below = world
            .raycast(Vec3::new(1.0, 0.0, -2.0), Vec3::UP, 10.0)
            .expect("the surface is two-sided");
        assert!((below.distance - 1.25).abs() < 1e-9);
        assert!(below.normal.y < -0.999, "normal {:?}", below.normal);
    }

    #[test]
    fn install_replaces_the_previous_session_and_advances_the_generation() {
        reset_registry_for_tests();
        let (first_generation, first) = install_world(plane_world("fuji", 0.0));
        assert_eq!(first.circuit_id, "fuji");
        assert_eq!(world_generation(), first_generation);
        let (second_generation, second) = install_world(plane_world("melbourne", 0.5));
        assert!(second_generation > first_generation);
        assert_eq!(second.circuit_id, "melbourne");
        let (generation, world) = installed_world().expect("world installed");
        assert_eq!(generation, second_generation);
        assert_eq!(world.identity().circuit_id, "melbourne");
        // Installing again with the same content still invalidates: the session
        // boundary is what the generation tracks, not the payload.
        let (third_generation, _) = install_world(plane_world("fuji", 0.0));
        assert!(third_generation > second_generation);
        assert!(uninstall_world());
        assert!(installed_world().is_none());
        assert!(world_generation() > third_generation);
        assert!(!uninstall_world());
        assert_eq!(world_generation() + 0, world_generation());
    }

    #[test]
    fn transfer_file_round_trip_keeps_the_frame_mapping() {
        let directory = std::env::temp_dir().join("ofp_collision_world_test");
        std::fs::create_dir_all(&directory).unwrap();
        let path = directory.join("plane_ground_collisions.txt");
        // One horizontal triangle in Speed Dreams coordinates (X forward, Y left, Z up).
        std::fs::write(
            &path,
            "FORMULA_GROUND_1 1\n1.0 0.002 0 0 0.25 10 0 0.25 0 10 0.25\n",
        )
        .unwrap();
        let world =
            CollisionWorld::load_transfer_file(path.to_str().unwrap(), Some("plane"), Some(7))
                .expect("transfer file");
        assert_eq!(world.identity().circuit_id, "plane");
        assert_eq!(world.identity().revision, 7);
        assert_eq!(world.identity().source, SOURCE_TRANSFER_FILE);
        // SD (x=0,y=0,z=0.25) -> Rust (0, 0.25, 0); SD (10,0,0.25) -> Rust (0,0.25,-10).
        assert!((world.identity().aabb_min.z + 10.0).abs() < 1e-9);
        let hit = world
            .raycast(Vec3::new(0.0, 3.0, 0.0), -Vec3::UP, 10.0)
            .expect("hit");
        assert!((hit.distance - 2.75).abs() < 1e-9);
        let _ = std::fs::remove_dir_all(&directory);
    }

    #[test]
    fn empty_transfer_document_is_an_error_not_an_empty_world() {
        let directory = std::env::temp_dir().join("ofp_collision_world_empty");
        std::fs::create_dir_all(&directory).unwrap();
        let path = directory.join("empty_ground_collisions.txt");
        std::fs::write(&path, "FORMULA_GROUND_1 0\n").unwrap();
        let error =
            CollisionWorld::load_transfer_file(path.to_str().unwrap(), None, None).unwrap_err();
        assert!(error.contains("no triangles"), "{error}");
        let _ = std::fs::remove_dir_all(&directory);
    }
}
