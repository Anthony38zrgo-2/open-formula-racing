//! Parser, surface classifier and binary cache for AC3D visual track models (.acc / .ac).
//!
//! Visual Track as the Single Source of Truth
//! ------------------------------------------
//! Speed Dreams renders circuits from AC3D models (`<track>.acc` or `<track>.ac`).
//! This module directly loads the exact visual track that Speed Dreams renders, eliminating
//! offline python preprocessing scripts and separate collision text files.
//!
//! Triangles are automatically classified into:
//! 1. Driveable Ground (track asphalt, pit lane, pit floor, pit boxes, curbs, verges, grass, gravel, sand)
//!    used by 12 wheel rays and underfloor probes.
//! 2. Solid Obstacles (boundary walls, catch fences, pit walls, garages, pit buildings, barrier rails, tire walls)
//!    used by 3D body collision volumes (chassis OBBs + wheel hubs).
//! 3. Ignored visual-only elements (sky, horizon, distant backdrop, crowds, shadows, reflection planes).
//!
//! Coordinate mapping:
//! AC3D coordinates (X right, Y up, Z forward) map to the Rust world frame (X right, Y up, -Z forward) via:
//!   X_rust = Z_ac
//!   Y_rust = Y_ac
//!   Z_rust = -X_ac

use std::fs::File;
use std::io::{BufRead, BufReader, Read, Write};
use std::path::Path;
use std::time::UNIX_EPOCH;

use crate::collision_world::TriangleMaterial;
use crate::types::Vec3;

const CACHE_MAGIC: &[u8; 16] = b"OFP_TRACK_CACHE2";

/// One driveable ground triangle with physical tire interaction properties.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct DriveableTriangle {
    pub vertices: [Vec3; 3],
    pub friction: f64,
    pub rolling_resistance: f64,
}

/// One solid obstacle triangle with impact restitution and Coulomb friction.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ObstacleTriangle {
    pub vertices: [Vec3; 3],
    pub friction: f64,
    pub restitution: f64,
    pub damage: f64,
}

/// Parsed and classified geometry of one circuit.
#[derive(Debug, Clone)]
pub struct VisualTrackGeometry {
    pub circuit_id: String,
    pub revision: u32,
    pub source_path: String,
    pub driveable: Vec<DriveableTriangle>,
    pub obstacles: Vec<ObstacleTriangle>,
}

impl VisualTrackGeometry {
    pub fn driveable_triangles(&self) -> Vec<[Vec3; 3]> {
        self.driveable.iter().map(|t| t.vertices).collect()
    }

    pub fn driveable_materials(&self) -> Vec<TriangleMaterial> {
        self.driveable
            .iter()
            .map(|t| TriangleMaterial::new(t.friction, t.rolling_resistance))
            .collect()
    }

    pub fn obstacle_triangles(&self) -> Vec<[Vec3; 3]> {
        self.obstacles.iter().map(|t| t.vertices).collect()
    }
}

/// Loads a track from the visual AC3D file (.acc or .ac).
/// Checks `<path>.ofpcache` first; if present and fresh, loads in ~1-2 ms.
/// Otherwise parses the text model, classifies triangles, saves `.ofpcache`, and returns.
pub fn load_or_parse_visual_track(
    path: &str,
    circuit_id: Option<&str>,
    revision: Option<u32>,
) -> Result<VisualTrackGeometry, String> {
    let source_path = Path::new(path);
    if !source_path.exists() {
        return Err(format!("visual track file not found: {path}"));
    }

    let id = circuit_id
        .filter(|s| !s.trim().is_empty())
        .map(|s| s.to_string())
        .unwrap_or_else(|| {
            source_path
                .file_stem()
                .map(|stem| stem.to_string_lossy().to_string())
                .unwrap_or_else(|| "track".to_string())
        });
    let rev = revision.unwrap_or(1);

    let metadata = source_path
        .metadata()
        .map_err(|e| format!("cannot stat track file {path}: {e}"))?;
    let file_size = metadata.len();
    let mtime_secs = metadata
        .modified()
        .ok()
        .and_then(|t| t.duration_since(UNIX_EPOCH).ok())
        .map(|d| d.as_secs())
        .unwrap_or(0);

    let cache_path = source_path.with_extension(format!(
        "{}.ofpcache",
        source_path.extension().and_then(|e| e.to_str()).unwrap_or("acc")
    ));

    if let Ok(cached) = load_cache(&cache_path, file_size, mtime_secs, &id, rev) {
        return Ok(cached);
    }

    // Parse and classify AC3D text model
    let parsed = parse_ac3d_file(source_path, &id, rev)?;

    // Try to save cache for fast next startup
    let _ = save_cache(&cache_path, file_size, mtime_secs, &parsed);

    Ok(parsed)
}

fn load_cache(
    cache_path: &Path,
    expected_size: u64,
    expected_mtime: u64,
    expected_id: &str,
    expected_rev: u32,
) -> Result<VisualTrackGeometry, String> {
    let mut file = File::open(cache_path).map_err(|e| e.to_string())?;
    let mut header = [0u8; 16];
    file.read_exact(&mut header).map_err(|e| e.to_string())?;
    if &header != CACHE_MAGIC {
        return Err("invalid cache magic".into());
    }

    let mut meta = [0u8; 8 + 8 + 4 + 4 + 4 + 4];
    file.read_exact(&mut meta).map_err(|e| e.to_string())?;

    let file_size = u64::from_le_bytes(meta[0..8].try_into().unwrap());
    let mtime_secs = u64::from_le_bytes(meta[8..16].try_into().unwrap());
    let rev = u32::from_le_bytes(meta[16..20].try_into().unwrap());
    let id_len = u32::from_le_bytes(meta[20..24].try_into().unwrap()) as usize;
    let driveable_count = u32::from_le_bytes(meta[24..28].try_into().unwrap()) as usize;
    let obstacle_count = u32::from_le_bytes(meta[28..32].try_into().unwrap()) as usize;

    if file_size != expected_size || mtime_secs != expected_mtime || rev != expected_rev {
        return Err("cache is stale".into());
    }

    let mut id_bytes = vec![0u8; id_len];
    file.read_exact(&mut id_bytes).map_err(|e| e.to_string())?;
    let circuit_id = String::from_utf8(id_bytes).map_err(|e| e.to_string())?;
    if circuit_id != expected_id {
        return Err("circuit id mismatch".into());
    }

    // Read driveable triangles: (9 * 8) + (2 * 8) = 88 bytes per triangle
    let mut driveable = Vec::with_capacity(driveable_count);
    let mut buf = [0u8; 88];
    for _ in 0..driveable_count {
        file.read_exact(&mut buf).map_err(|e| e.to_string())?;
        let v0 = Vec3::new(
            f64::from_le_bytes(buf[0..8].try_into().unwrap()),
            f64::from_le_bytes(buf[8..16].try_into().unwrap()),
            f64::from_le_bytes(buf[16..24].try_into().unwrap()),
        );
        let v1 = Vec3::new(
            f64::from_le_bytes(buf[24..32].try_into().unwrap()),
            f64::from_le_bytes(buf[32..40].try_into().unwrap()),
            f64::from_le_bytes(buf[40..48].try_into().unwrap()),
        );
        let v2 = Vec3::new(
            f64::from_le_bytes(buf[48..56].try_into().unwrap()),
            f64::from_le_bytes(buf[56..64].try_into().unwrap()),
            f64::from_le_bytes(buf[64..72].try_into().unwrap()),
        );
        let friction = f64::from_le_bytes(buf[72..80].try_into().unwrap());
        let rolling = f64::from_le_bytes(buf[80..88].try_into().unwrap());
        driveable.push(DriveableTriangle {
            vertices: [v0, v1, v2],
            friction,
            rolling_resistance: rolling,
        });
    }

    // Read obstacle triangles: (9 * 8) + (3 * 8) = 96 bytes per triangle
    let mut obstacles = Vec::with_capacity(obstacle_count);
    let mut buf_obs = [0u8; 96];
    for _ in 0..obstacle_count {
        file.read_exact(&mut buf_obs).map_err(|e| e.to_string())?;
        let v0 = Vec3::new(
            f64::from_le_bytes(buf_obs[0..8].try_into().unwrap()),
            f64::from_le_bytes(buf_obs[8..16].try_into().unwrap()),
            f64::from_le_bytes(buf_obs[16..24].try_into().unwrap()),
        );
        let v1 = Vec3::new(
            f64::from_le_bytes(buf_obs[24..32].try_into().unwrap()),
            f64::from_le_bytes(buf_obs[32..40].try_into().unwrap()),
            f64::from_le_bytes(buf_obs[40..48].try_into().unwrap()),
        );
        let v2 = Vec3::new(
            f64::from_le_bytes(buf_obs[48..56].try_into().unwrap()),
            f64::from_le_bytes(buf_obs[56..64].try_into().unwrap()),
            f64::from_le_bytes(buf_obs[64..72].try_into().unwrap()),
        );
        let friction = f64::from_le_bytes(buf_obs[72..80].try_into().unwrap());
        let restitution = f64::from_le_bytes(buf_obs[80..88].try_into().unwrap());
        let damage = f64::from_le_bytes(buf_obs[88..96].try_into().unwrap());
        obstacles.push(ObstacleTriangle {
            vertices: [v0, v1, v2],
            friction,
            restitution,
            damage,
        });
    }

    Ok(VisualTrackGeometry {
        circuit_id,
        revision: rev,
        source_path: cache_path.to_string_lossy().to_string(),
        driveable,
        obstacles,
    })
}

fn save_cache(
    cache_path: &Path,
    file_size: u64,
    mtime_secs: u64,
    track: &VisualTrackGeometry,
) -> Result<(), String> {
    let mut file = File::create(cache_path).map_err(|e| e.to_string())?;
    file.write_all(CACHE_MAGIC).map_err(|e| e.to_string())?;

    let id_bytes = track.circuit_id.as_bytes();
    let mut meta = [0u8; 8 + 8 + 4 + 4 + 4 + 4];
    meta[0..8].copy_from_slice(&file_size.to_le_bytes());
    meta[8..16].copy_from_slice(&mtime_secs.to_le_bytes());
    meta[16..20].copy_from_slice(&track.revision.to_le_bytes());
    meta[20..24].copy_from_slice(&(id_bytes.len() as u32).to_le_bytes());
    meta[24..28].copy_from_slice(&(track.driveable.len() as u32).to_le_bytes());
    meta[28..32].copy_from_slice(&(track.obstacles.len() as u32).to_le_bytes());
    file.write_all(&meta).map_err(|e| e.to_string())?;
    file.write_all(id_bytes).map_err(|e| e.to_string())?;

    let mut buf = Vec::with_capacity(track.driveable.len() * 88);
    for t in &track.driveable {
        for v in &t.vertices {
            buf.extend_from_slice(&v.x.to_le_bytes());
            buf.extend_from_slice(&v.y.to_le_bytes());
            buf.extend_from_slice(&v.z.to_le_bytes());
        }
        buf.extend_from_slice(&t.friction.to_le_bytes());
        buf.extend_from_slice(&t.rolling_resistance.to_le_bytes());
    }
    file.write_all(&buf).map_err(|e| e.to_string())?;

    let mut buf_obs = Vec::with_capacity(track.obstacles.len() * 96);
    for t in &track.obstacles {
        for v in &t.vertices {
            buf_obs.extend_from_slice(&v.x.to_le_bytes());
            buf_obs.extend_from_slice(&v.y.to_le_bytes());
            buf_obs.extend_from_slice(&v.z.to_le_bytes());
        }
        buf_obs.extend_from_slice(&t.friction.to_le_bytes());
        buf_obs.extend_from_slice(&t.restitution.to_le_bytes());
        buf_obs.extend_from_slice(&t.damage.to_le_bytes());
    }
    file.write_all(&buf_obs).map_err(|e| e.to_string())?;

    Ok(())
}

fn parse_ac3d_file(
    path: &Path,
    circuit_id: &str,
    revision: u32,
) -> Result<VisualTrackGeometry, String> {
    let file = File::open(path).map_err(|e| format!("cannot open {}: {e}", path.display()))?;
    let mut reader = BufReader::with_capacity(1024 * 1024, file);

    let mut line = String::with_capacity(256);
    let mut current_name = String::new();
    let mut current_verts: Vec<Vec3> = Vec::new();

    let mut driveable = Vec::with_capacity(32768);
    let mut obstacles = Vec::with_capacity(32768);

    while reader.read_line(&mut line).map_err(|e| e.to_string())? > 0 {
        let trimmed = line.trim();
        if trimmed.starts_with("OBJECT") {
            current_name.clear();
            current_verts.clear();
        } else if trimmed.starts_with("name \"") {
            if let Some(end) = trimmed[6..].find('\"') {
                current_name = trimmed[6..6 + end].to_string();
            }
        } else if trimmed.starts_with("numvert ") {
            let count: usize = trimmed[8..].trim().parse().unwrap_or(0);
            current_verts.clear();
            current_verts.reserve(count);
            for _ in 0..count {
                line.clear();
                if reader.read_line(&mut line).map_err(|e| e.to_string())? == 0 {
                    break;
                }
                let mut parts = line.split_whitespace();
                let x: f64 = parts.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                let y: f64 = parts.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                let z: f64 = parts.next().and_then(|s| s.parse().ok()).unwrap_or(0.0);
                // AC3D (x, y, z) -> Rust (z, y, -x)
                current_verts.push(Vec3::new(z, y, -x));
            }
        } else if trimmed.starts_with("numsurf ") {
            let surf_count: usize = trimmed[8..].trim().parse().unwrap_or(0);
            for _ in 0..surf_count {
                // Read until 'refs'
                let mut refs_count = 0;
                loop {
                    line.clear();
                    if reader.read_line(&mut line).map_err(|e| e.to_string())? == 0 {
                        break;
                    }
                    let t = line.trim();
                    if t.starts_with("refs ") {
                        refs_count = t[5..].trim().parse().unwrap_or(0);
                        break;
                    }
                }

                let mut ref_indices = Vec::with_capacity(refs_count);
                for _ in 0..refs_count {
                    line.clear();
                    if reader.read_line(&mut line).map_err(|e| e.to_string())? == 0 {
                        break;
                    }
                    if let Some(idx_str) = line.split_whitespace().next() {
                        if let Ok(idx) = idx_str.parse::<usize>() {
                            ref_indices.push(idx);
                        }
                    }
                }

                if ref_indices.len() < 3 || current_verts.is_empty() {
                    continue;
                }

                let name_lower = current_name.to_ascii_lowercase();

                // 1. Ignored elements check
                if is_ignored_element(&name_lower) {
                    continue;
                }

                // Triangulate (triangle, quad or fan)
                for k in 2..ref_indices.len() {
                    let i0 = ref_indices[0];
                    let i1 = ref_indices[k - 1];
                    let i2 = ref_indices[k];

                    if i0 >= current_verts.len()
                        || i1 >= current_verts.len()
                        || i2 >= current_verts.len()
                    {
                        continue;
                    }

                    let v0 = current_verts[i0];
                    let v1 = current_verts[i1];
                    let v2 = current_verts[i2];

                    let e1 = v1 - v0;
                    let e2 = v2 - v0;
                    let normal = e1.cross(e2);
                    let area = normal.length();
                    if area < 1e-9 {
                        continue;
                    }

                    let ny_abs = (normal.y / area).abs();

                    let is_obstacle_kw = is_obstacle_element(&name_lower);
                    let is_driveable_kw = is_driveable_element(&name_lower);

                    let min_y = v0.y.min(v1.y).min(v2.y);
                    let max_y = v0.y.max(v1.y).max(v2.y);
                    let height_span = max_y - min_y;

                    if is_obstacle_kw {
                        obstacles.push(ObstacleTriangle {
                            vertices: [v0, v1, v2],
                            friction: 0.50,
                            restitution: 0.15,
                            damage: 1.0,
                        });
                    } else if is_driveable_kw {
                        if ny_abs >= 0.55 {
                            let (friction, rolling) = surface_material_from_name(&name_lower);
                            driveable.push(DriveableTriangle {
                                vertices: [v0, v1, v2],
                                friction,
                                rolling_resistance: rolling,
                            });
                        } else if height_span >= 0.35 {
                            // Substantial vertical drop, retaining wall or embankment (> 35 cm)
                            obstacles.push(ObstacleTriangle {
                                vertices: [v0, v1, v2],
                                friction: 0.50,
                                restitution: 0.15,
                                damage: 1.0,
                            });
                        }
                    } else if ny_abs < 0.40 && height_span >= 0.35 {
                        // Unclassified vertical surface near track: obstacle
                        obstacles.push(ObstacleTriangle {
                            vertices: [v0, v1, v2],
                            friction: 0.50,
                            restitution: 0.15,
                            damage: 1.0,
                        });
                    }
                }
            }
        }
        line.clear();
    }

    if driveable.is_empty() {
        return Err(format!(
            "visual track {} has no driveable ground triangles",
            path.display()
        ));
    }

    Ok(VisualTrackGeometry {
        circuit_id: circuit_id.to_string(),
        revision,
        source_path: path.to_string_lossy().to_string(),
        driveable,
        obstacles,
    })
}

fn is_ignored_element(name: &str) -> bool {
    const IGNORED: &[&str] = &[
        "sky",
        "horizon",
        "shadow",
        "shad",
        "reflect_",
        "bestline",
        "crowd",
        "cameraman",
        "marshall",
        "pitman",
        "chopper",
        "billboard",
        "tree",
        "people",
        "ppl",
        "videotv",
        "fbuild",
        "largesign",
        "smallsign",
        "pitslot",
        "stslot",
        "aisle",
        "bridge",
        "stand",
        "tribune",
        "light",
    ];
    IGNORED.iter().any(|k| name.contains(k))
}

fn is_obstacle_element(name: &str) -> bool {
    const OBSTACLES: &[&str] = &[
        "wall",
        "fence",
        "tirewall",
        "tyrewall",
        "armco",
        "barrier",
        "garage",
        "pitbld",
        "building",
        "mbox",
        "cone",
        "flag_pitblock",
        "ctrlcntr",
        "ctrltwr",
        "rearwall",
        "guard",
        "podium",
        "rvboxes",
        "trailer",
        "amblnc",
        "towtrk",
        "pickup",
    ];
    OBSTACLES.iter().any(|k| name.contains(k))
}

fn is_driveable_element(name: &str) -> bool {
    const DRIVEABLE: &[&str] = &[
        "track", "trak", "pitlane", "pitfloor", "pit_box", "paddock",
        "kerb", "curb", "verge", "sidl", "sidr", "nland", "grass", "gravel", "sand", "road",
        "asphalt",
    ];
    DRIVEABLE.iter().any(|k| name.contains(k))
}

fn surface_material_from_name(name: &str) -> (f64, f64) {
    if name.contains("kerb") || name.contains("curb") {
        (0.95, 0.003)
    } else if name.contains("grass") {
        (0.70, 0.05)
    } else if name.contains("gravel") {
        (0.55, 0.12)
    } else if name.contains("sand") {
        (0.45, 0.18)
    } else if name.contains("nland") {
        // Runoff landscape in Melbourne / Fuji
        (0.75, 0.03)
    } else {
        // Track asphalt, pitlane, pitfloor, paddock
        (1.00, 0.002)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_and_cache_ac3d() {
        let temp_dir = std::env::temp_dir().join("ofp_test_ac3d");
        let _ = std::fs::create_dir_all(&temp_dir);
        let track_acc = temp_dir.join("test_circuit.acc");

        // Small test AC3D containing 1 driveable quad and 1 obstacle quad
        let ac_content = r#"AC3Db
MATERIAL "ac3dmat1" rgb 1 1 1  amb 0.2 0.2 0.2  emis 0 0 0  spec 0 0 0  shi 10  trans 0
OBJECT world
kids 2
OBJECT poly
name "Track_asphalt"
numvert 4
0 0 0
10 0 0
10 0 10
0 0 10
numsurf 1
SURF 0x30
mat 0
refs 4
0 0 0
1 1 0
2 1 1
3 0 1
kids 0
OBJECT poly
name "TRACK_wall_concrete"
numvert 4
0 0 0
10 0 0
10 2 0
0 2 0
numsurf 1
SURF 0x30
mat 0
refs 4
0 0 0
1 1 0
2 1 1
3 0 1
kids 0
"#;
        std::fs::write(&track_acc, ac_content).unwrap();

        let track = load_or_parse_visual_track(track_acc.to_str().unwrap(), Some("test-circuit"), Some(1))
            .expect("must parse test track");
        assert_eq!(track.circuit_id, "test-circuit");
        assert_eq!(track.driveable.len(), 2); // 1 quad = 2 tris
        assert_eq!(track.obstacles.len(), 2); // 1 quad = 2 tris
        assert_eq!(track.driveable[0].friction, 1.0);
        assert_eq!(track.obstacles[0].restitution, 0.15);

        // Verify cache file was generated
        let cache_file = temp_dir.join("test_circuit.acc.ofpcache");
        assert!(cache_file.exists());

        // Fast load from cache
        let cached = load_or_parse_visual_track(track_acc.to_str().unwrap(), Some("test-circuit"), Some(1))
            .expect("must load from cache");
        assert_eq!(cached.driveable.len(), 2);
        assert_eq!(cached.obstacles.len(), 2);

        let _ = std::fs::remove_dir_all(&temp_dir);
    }

    #[test]
    fn test_parse_real_melbourne_track() {
        let path = "d:/open-formula/distribution/share/games/speed-dreams-2/tracks/circuit/melbourne-nineteen-ninety-seven/melbourne-nineteen-ninety-seven.acc";
        if !std::path::Path::new(path).exists() {
            return;
        }
        let t0 = std::time::Instant::now();
        let track = load_or_parse_visual_track(path, Some("melbourne-nineteen-ninety-seven"), Some(1)).unwrap();
        let parse_time = t0.elapsed();
        eprintln!("Melbourne parsed in {:?}: driveable={}, obstacles={}", parse_time, track.driveable.len(), track.obstacles.len());
        assert!(track.driveable.len() > 10000);
        assert!(track.obstacles.len() > 5000);

        // Second load from cache
        let t1 = std::time::Instant::now();
        let cached = load_or_parse_visual_track(path, Some("melbourne-nineteen-ninety-seven"), Some(1)).unwrap();
        let cache_time = t1.elapsed();
        eprintln!("Melbourne loaded from cache in {:?}", cache_time);
        assert_eq!(cached.driveable.len(), track.driveable.len());
        assert_eq!(cached.obstacles.len(), track.obstacles.len());
    }
}

