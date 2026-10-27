//! 2.5D Obstacle Wall representation with track-oriented outward normals.
//!
//! Converts 3D obstacle triangles from visual track models (.acc/.ac) into vertical
//! 2D wall segments with a height range [y_min, y_max].
//! Each wall segment has a fixed, authoritative horizontal normal pointing towards
//! the driveable track surface, determined at track load time.

use std::collections::HashMap;

use parry3d_f64::na::{Isometry3, Point3, Vector3};
use parry3d_f64::query::{Ray, RayCast};
use parry3d_f64::shape::TriMesh;

use crate::ac3d_track::{DriveableTriangle, ObstacleTriangle};
use crate::types::Vec3;

/// Vertical slack allowed when deciding whether a body polygon overlaps a wall's
/// height band. This is a surface tolerance, not the horizontal speculative
/// margin: the prediction must not make a low kerb collide with a high wing.
const VERTICAL_TOLERANCE_M: f64 = 0.03;

/// A one-sided wall may push a body that is shallowly behind it (a car that has
/// just tunnelled through the face). A body farther behind is on the outside of
/// that wall, not a penetration, and must never be dragged through it.
const BACK_RECOVERY_BAND_M: f64 = 0.50;

/// Centre of the convex polygon in the XZ plane. Only used to pick which side of
/// a two-sided wall the query body is on; the contact itself is always evaluated
/// from the polygon's vertices.
fn polygon_centroid(poly: &ConvexPoly2) -> (f64, f64) {
    let mut x = 0.0;
    let mut z = 0.0;
    for i in 0..poly.n {
        x += poly.v[i][0];
        z += poly.v[i][1];
    }
    let count = poly.n.max(1) as f64;
    (x / count, z / count)
}

/// True when `point` is inside (or on the boundary of) a convex polygon.
fn point_in_convex_polygon(point: [f64; 2], poly: &ConvexPoly2) -> bool {
    let mut positive = false;
    let mut negative = false;
    for i in 0..poly.n {
        let j = (i + 1) % poly.n;
        let ex = poly.v[j][0] - poly.v[i][0];
        let ez = poly.v[j][1] - poly.v[i][1];
        let px = point[0] - poly.v[i][0];
        let pz = point[1] - poly.v[i][1];
        let cross = ex * pz - ez * px;
        if cross > 1e-9 {
            positive = true;
        } else if cross < -1e-9 {
            negative = true;
        }
        if positive && negative {
            return false;
        }
    }
    true
}

/// Distance from `point` to the nearest polygon edge and that edge's outward
/// normal. Used when a wall endpoint is inside the body: the shortest way out of
/// the body is through its nearest edge.
fn nearest_polygon_edge(point: [f64; 2], poly: &ConvexPoly2) -> (f64, [f64; 2]) {
    let (cx, cz) = polygon_centroid(poly);
    let mut best_distance = f64::INFINITY;
    let mut best_normal = [1.0, 0.0];
    for i in 0..poly.n {
        let j = (i + 1) % poly.n;
        let ax = poly.v[i][0];
        let az = poly.v[i][1];
        let bx = poly.v[j][0];
        let bz = poly.v[j][1];
        let ex = bx - ax;
        let ez = bz - az;
        let len_sq = ex * ex + ez * ez;
        if len_sq < 1e-12 {
            continue;
        }
        let t = (((point[0] - ax) * ex + (point[1] - az) * ez) / len_sq).clamp(0.0, 1.0);
        let qx = ax + ex * t;
        let qz = az + ez * t;
        let distance = ((point[0] - qx).powi(2) + (point[1] - qz).powi(2)).sqrt();
        if distance < best_distance {
            best_distance = distance;
            let len = len_sq.sqrt();
            // Right-hand normal of the edge; orient it away from the centroid so
            // the push always leaves the body, whatever the hull winding.
            let mut normal = [ez / len, -ex / len];
            let mid_x = (ax + bx) * 0.5;
            let mid_z = (az + bz) * 0.5;
            if normal[0] * (cx - mid_x) + normal[1] * (cz - mid_z) > 0.0 {
                normal = [-normal[0], -normal[1]];
            }
            best_normal = normal;
        }
    }
    (best_distance, best_normal)
}

/// Closest point on the polygon boundary to `point`, and its distance. Used for
/// the speculative contact between a wall end (a rounded post) and a body edge
/// that passes near it without any body vertex coming close.
fn closest_polygon_edge(point: [f64; 2], poly: &ConvexPoly2) -> (f64, [f64; 2]) {
    let mut best_distance = f64::INFINITY;
    let mut best_point = point;
    for i in 0..poly.n {
        let j = (i + 1) % poly.n;
        let ax = poly.v[i][0];
        let az = poly.v[i][1];
        let ex = poly.v[j][0] - ax;
        let ez = poly.v[j][1] - az;
        let len_sq = ex * ex + ez * ez;
        if len_sq < 1e-12 {
            continue;
        }
        let t = (((point[0] - ax) * ex + (point[1] - az) * ez) / len_sq).clamp(0.0, 1.0);
        let qx = ax + ex * t;
        let qz = az + ez * t;
        let distance = ((point[0] - qx).powi(2) + (point[1] - qz).powi(2)).sqrt();
        if distance < best_distance {
            best_distance = distance;
            best_point = [qx, qz];
        }
    }
    (best_distance, best_point)
}

/// Height at which a wall contact is reported: the middle of the vertical band
/// the body and the wall actually share, clamped to the wall segment.
fn contact_height(poly: &ConvexPoly2, seg: &WallSegment) -> f64 {
    let low = poly.y_min.max(seg.y_min);
    let high = poly.y_max.min(seg.y_max);
    if high > low {
        (low + high) * 0.5
    } else {
        ((poly.y_min + poly.y_max) * 0.5).clamp(seg.y_min, seg.y_max)
    }
}

/// Single 2.5D wall segment in the Rust XZ plane with height span in Y.
#[derive(Debug, Clone, PartialEq)]
pub struct WallSegment {
    /// Start point in XZ plane: [x, z]
    pub a: [f64; 2],
    /// End point in XZ plane: [x, z]
    pub b: [f64; 2],
    /// Unit tangent along segment: (b - a) / len
    pub tangent: [f64; 2],
    /// Segment length in horizontal plane (m)
    pub len: f64,
    /// Authoritative unit normal pointing towards the track: [nx, nz]
    pub normal: [f64; 2],
    /// True if both sides have adjacent driveable track (e.g. pit wall between pitlane and main straight)
    pub two_sided: bool,
    /// Vertical extent (m)
    pub y_min: f64,
    pub y_max: f64,
    /// Physical properties
    pub friction: f64,
    pub restitution: f64,
    pub damage: f64,
}

/// Uniform spatial hash grid for fast 2D wall segment candidate lookup.
#[derive(Debug, Clone)]
pub struct WallGrid {
    cell_size: f64,
    min_x: f64,
    min_z: f64,
    cols: usize,
    rows: usize,
    /// CSR format: cell_offsets has length cols * rows + 1
    cell_offsets: Vec<usize>,
    /// Flattened segment indices
    cell_indices: Vec<u32>,
}

impl WallGrid {
    pub fn empty() -> Self {
        Self {
            cell_size: 4.0,
            min_x: 0.0,
            min_z: 0.0,
            cols: 0,
            rows: 0,
            cell_offsets: vec![0],
            cell_indices: Vec::new(),
        }
    }

    pub fn build(segments: &[WallSegment], cell_size: f64) -> Self {
        if segments.is_empty() {
            return Self::empty();
        }

        let mut min_x = f64::INFINITY;
        let mut min_z = f64::INFINITY;
        let mut max_x = f64::NEG_INFINITY;
        let mut max_z = f64::NEG_INFINITY;

        for seg in segments {
            min_x = min_x.min(seg.a[0]).min(seg.b[0]);
            min_z = min_z.min(seg.a[1]).min(seg.b[1]);
            max_x = max_x.max(seg.a[0]).max(seg.b[0]);
            max_z = max_z.max(seg.a[1]).max(seg.b[1]);
        }

        // Add padding around bounds
        min_x -= cell_size;
        min_z -= cell_size;
        max_x += cell_size;
        max_z += cell_size;

        let cols = ((max_x - min_x) / cell_size).ceil().max(1.0) as usize;
        let rows = ((max_z - min_z) / cell_size).ceil().max(1.0) as usize;
        let total_cells = cols * rows;

        let mut cell_entries: Vec<Vec<u32>> = vec![Vec::new(); total_cells];

        for (idx, seg) in segments.iter().enumerate() {
            let seg_idx = idx as u32;
            let len = seg.len;
            let steps = (len / (cell_size * 0.5)).ceil().max(1.0) as usize;
            let mut last_cell = usize::MAX;

            for s in 0..=steps {
                let t = s as f64 / steps as f64;
                let x = seg.a[0] + (seg.b[0] - seg.a[0]) * t;
                let z = seg.a[1] + (seg.b[1] - seg.a[1]) * t;

                let c = ((x - min_x) / cell_size).floor().clamp(0.0, (cols - 1) as f64) as usize;
                let r = ((z - min_z) / cell_size).floor().clamp(0.0, (rows - 1) as f64) as usize;
                let cell = r * cols + c;

                if cell != last_cell {
                    if !cell_entries[cell].contains(&seg_idx) {
                        cell_entries[cell].push(seg_idx);
                    }
                    last_cell = cell;
                }
            }
        }

        let mut cell_offsets = Vec::with_capacity(total_cells + 1);
        let mut cell_indices = Vec::new();
        cell_offsets.push(0);

        for entries in cell_entries {
            cell_indices.extend(entries);
            cell_offsets.push(cell_indices.len());
        }

        Self {
            cell_size,
            min_x,
            min_z,
            cols,
            rows,
            cell_offsets,
            cell_indices,
        }
    }

    /// Queries unique candidate wall segment indices overlapping bounding box [min_x, min_z] .. [max_x, max_z].
    pub fn query_range(&self, q_min_x: f64, q_min_z: f64, q_max_x: f64, q_max_z: f64, out: &mut Vec<u32>) {
        out.clear();
        if self.cols == 0 || self.rows == 0 {
            return;
        }

        let c_start = ((q_min_x - self.min_x) / self.cell_size).floor().clamp(0.0, (self.cols - 1) as f64) as usize;
        let c_end = ((q_max_x - self.min_x) / self.cell_size).floor().clamp(0.0, (self.cols - 1) as f64) as usize;
        let r_start = ((q_min_z - self.min_z) / self.cell_size).floor().clamp(0.0, (self.rows - 1) as f64) as usize;
        let r_end = ((q_max_z - self.min_z) / self.cell_size).floor().clamp(0.0, (self.rows - 1) as f64) as usize;

        for r in r_start..=r_end {
            let row_offset = r * self.cols;
            for c in c_start..=c_end {
                let cell = row_offset + c;
                let start = self.cell_offsets[cell];
                let end = self.cell_offsets[cell + 1];
                for &idx in &self.cell_indices[start..end] {
                    if !out.contains(&idx) {
                        out.push(idx);
                    }
                }
            }
        }
    }
}

/// 2D Convex polygon projection of a 3D chassis box or oriented tire.
#[derive(Debug, Clone)]
pub struct ConvexPoly2 {
    /// Vertices in XZ plane (up to 8 points)
    pub v: [[f64; 2]; 8],
    pub n: usize,
    pub y_min: f64,
    pub y_max: f64,
}

impl ConvexPoly2 {
    pub fn from_box(center: Vec3, basis_x: Vec3, basis_y: Vec3, basis_z: Vec3, half_extents: Vec3) -> Self {
        // Project the 8 box corners to XZ
        let mut corners = [[0.0f64; 2]; 8];
        let mut y_min = f64::INFINITY;
        let mut y_max = f64::NEG_INFINITY;

        let signs = [
            (-1.0, -1.0, -1.0),
            ( 1.0, -1.0, -1.0),
            ( 1.0,  1.0, -1.0),
            (-1.0,  1.0, -1.0),
            (-1.0, -1.0,  1.0),
            ( 1.0, -1.0,  1.0),
            ( 1.0,  1.0,  1.0),
            (-1.0,  1.0,  1.0),
        ];

        for (i, &(sx, sy, sz)) in signs.iter().enumerate() {
            let pt = center
                + basis_x * (sx * half_extents.x)
                + basis_y * (sy * half_extents.y)
                + basis_z * (sz * half_extents.z);
            corners[i] = [pt.x, pt.z];
            y_min = y_min.min(pt.y);
            y_max = y_max.max(pt.y);
        }

        // Compute 2D convex hull of 8 points using Andrew's monotone chain algorithm
        let mut sorted = corners;
        sorted.sort_by(|a, b| {
            a[0].partial_cmp(&b[0])
                .unwrap_or(std::cmp::Ordering::Equal)
                .then_with(|| a[1].partial_cmp(&b[1]).unwrap_or(std::cmp::Ordering::Equal))
        });

        fn cross2(o: [f64; 2], a: [f64; 2], b: [f64; 2]) -> f64 {
            (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
        }

        let mut lower = [[0.0f64; 2]; 8];
        let mut lower_len = 0usize;
        for p in &sorted {
            while lower_len >= 2 && cross2(lower[lower_len - 2], lower[lower_len - 1], *p) <= 1e-9 {
                lower_len -= 1;
            }
            if lower_len < 8 {
                lower[lower_len] = *p;
                lower_len += 1;
            }
        }

        let mut upper = [[0.0f64; 2]; 8];
        let mut upper_len = 0usize;
        for p in sorted.iter().rev() {
            while upper_len >= 2 && cross2(upper[upper_len - 2], upper[upper_len - 1], *p) <= 1e-9 {
                upper_len -= 1;
            }
            if upper_len < 8 {
                upper[upper_len] = *p;
                upper_len += 1;
            }
        }

        if lower_len > 0 { lower_len -= 1; }
        if upper_len > 0 { upper_len -= 1; }

        let mut v = [[0.0; 2]; 8];
        let mut n = 0usize;
        for i in 0..lower_len {
            if n < 8 { v[n] = lower[i]; n += 1; }
        }
        for i in 0..upper_len {
            if n < 8 { v[n] = upper[i]; n += 1; }
        }

        Self { v, n, y_min, y_max }
    }

    /// Creates an oriented rectangular bounding polygon for a physical wheel.
    pub fn from_wheel(
        center: Vec3,
        forward_axis: Vec3,
        lateral_axis: Vec3,
        radius: f64,
        half_width: f64,
    ) -> Self {
        let f_h = Vec3::new(forward_axis.x, 0.0, forward_axis.z).normalized();
        let l_h = Vec3::new(lateral_axis.x, 0.0, lateral_axis.z).normalized();

        let c_xz = [center.x, center.z];
        let p0 = [c_xz[0] - f_h.x * radius - l_h.x * half_width, c_xz[1] - f_h.z * radius - l_h.z * half_width];
        let p1 = [c_xz[0] + f_h.x * radius - l_h.x * half_width, c_xz[1] + f_h.z * radius - l_h.z * half_width];
        let p2 = [c_xz[0] + f_h.x * radius + l_h.x * half_width, c_xz[1] + f_h.z * radius + l_h.z * half_width];
        let p3 = [c_xz[0] - f_h.x * radius + l_h.x * half_width, c_xz[1] - f_h.z * radius + l_h.z * half_width];

        let mut v = [[0.0; 2]; 8];
        v[0] = p0;
        v[1] = p1;
        v[2] = p2;
        v[3] = p3;

        Self {
            v,
            n: 4,
            y_min: center.y - radius,
            y_max: center.y + radius,
        }
    }
}

/// A detected obstacle contact with signed gap and authoritative normal.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WallContact {
    /// Contact location in 3D world space
    pub point: Vec3,
    /// Authoritative unit normal pointing away from the wall toward track
    pub normal: Vec3,
    /// Signed gap along normal: < 0 is penetration depth, > 0 is separation
    pub gap_m: f64,
    /// Segment index in WallField
    pub wall_index: u32,
    /// Physical properties
    pub friction: f64,
    pub restitution: f64,
    pub damage: f64,
}

/// Hysteresis for the normal of a two-sided wall.
///
/// The side is normally taken from the query polygon's centroid. The memory only
/// breaks the tie when the centroid lies exactly on the wall plane; it must not
/// latch a side and keep pushing after the body has clearly crossed the wall,
/// which is what made free-standing posts behave as attractors.
#[derive(Debug, Clone, Default)]
pub struct WallSideMemory {
    /// (wall_index, last_side_sign: f64, last_seen_time_s)
    entries: Vec<(u32, f64, f64)>,
}

impl WallSideMemory {
    pub fn get_or_set(&mut self, wall_index: u32, current_sign: f64, current_time_s: f64) -> f64 {
        // Prune old entries (> 2.0 s)
        self.entries.retain(|e| (current_time_s - e.2).abs() < 2.0);

        let sign = if current_sign >= 0.0 { 1.0 } else { -1.0 };
        if let Some(entry) = self.entries.iter_mut().find(|e| e.0 == wall_index) {
            // A centroid that is clearly on a side is authoritative; only a
            // near-zero distance keeps the previous sign as hysteresis.
            if current_sign.abs() >= 0.02 {
                entry.1 = sign;
            }
            entry.2 = current_time_s;
            entry.1
        } else {
            self.entries.push((wall_index, sign, current_time_s));
            sign
        }
    }
}

/// Invisible collision boundary at the edge of the driveable mesh.
///
/// A car that leaves the rally road drives on grass/gravel that is still the same
/// driveable mesh. Where that mesh ends (outer edge or a hole in the terrain) there
/// is nothing to catch the car. The old terrain-bank walls were removed because
/// they stopped the car at 5-10 m; this function creates a wall only where the
/// ground really ends: an edge is kept when at least one side of it has no ground
/// under the ray used by the wheels. Road/terrain seams have ground on both sides
/// and stay open.
fn driveable_boundary_segments(
    driveable: &[DriveableTriangle],
    mesh: Option<&TriMesh>,
) -> Vec<WallSegment> {
    const SNAP: f64 = 0.05;
    struct EdgeInfo {
        count: u32,
        a: [f64; 2],
        b: [f64; 2],
        y_min: f64,
        y_max: f64,
    }
    let key = |p: [f64; 2]| ((p[0] / SNAP).round() as i64, (p[1] / SNAP).round() as i64);
    let mut edges: HashMap<((i64, i64), (i64, i64)), EdgeInfo> = HashMap::new();
    for tri in driveable {
        for (i, j) in [(0usize, 1usize), (1, 2), (2, 0)] {
            let p = tri.vertices[i];
            let q = tri.vertices[j];
            let kp = key([p.x, p.z]);
            let kq = key([q.x, q.z]);
            let k = if kp <= kq { (kp, kq) } else { (kq, kp) };
            let entry = edges.entry(k).or_insert(EdgeInfo {
                count: 0,
                a: [p.x, p.z],
                b: [q.x, q.z],
                y_min: p.y.min(q.y),
                y_max: p.y.max(q.y),
            });
            entry.count += 1;
        }
    }
    let mut out = Vec::new();
    // Boundary only outside a 15 m band around the road. Near ditches, banks and
    // bumps must stay passable: the car is allowed to leave the road there. A
    // boundary inside the road band was what the player felt as "the stage does
    // not let me go off".
    const ROAD_CELL: f64 = 20.0;
    const ROAD_CLEARANCE: f64 = 15.0;
    let mut road_grid: HashMap<(i32, i32), Vec<[f64; 2]>> = HashMap::new();
    for tri in driveable {
        if tri.rolling_resistance < 0.09 {
            continue; // asphalt/grass, not the rally road reference
        }
        for vertex in tri.vertices {
            let key = (
                (vertex.x / ROAD_CELL).floor() as i32,
                (vertex.z / ROAD_CELL).floor() as i32,
            );
            road_grid.entry(key).or_default().push([vertex.x, vertex.z]);
        }
    }
    let near_road = |x: f64, z: f64| -> bool {
        if road_grid.is_empty() {
            return false;
        }
        let cell_x = (x / ROAD_CELL).floor() as i32;
        let cell_z = (z / ROAD_CELL).floor() as i32;
        for dz in -1..=1 {
            for dx in -1..=1 {
                let Some(points) = road_grid.get(&(cell_x + dx, cell_z + dz)) else {
                    continue;
                };
                for point in points {
                    let d_sq = (point[0] - x) * (point[0] - x) + (point[1] - z) * (point[1] - z);
                    if d_sq <= ROAD_CLEARANCE * ROAD_CLEARANCE {
                        return true;
                    }
                }
            }
        }
        false
    };
    for info in edges.values() {
        if info.count != 1 {
            continue; // shared by two driveable triangles: interior
        }
        let dx = info.b[0] - info.a[0];
        let dz = info.b[1] - info.a[1];
        let len = (dx * dx + dz * dz).sqrt();
        if len <= 0.05 {
            continue;
        }
        let normal = [dz / len, -dx / len];
        let mid = [
            (info.a[0] + info.b[0]) * 0.5,
            (info.a[1] + info.b[1]) * 0.5,
        ];
        if near_road(mid[0], mid[1]) {
            continue;
        }
        if let Some(mesh) = mesh {
            let mut grounded_sides = 0;
            for sign in [-1.0f64, 1.0] {
                let origin = Point3::new(
                    mid[0] + normal[0] * 0.35 * sign,
                    info.y_max + 40.0,
                    mid[1] + normal[1] * 0.35 * sign,
                );
                let ray = Ray::new(origin, Vector3::new(0.0, -1.0, 0.0));
                if mesh
                    .cast_ray_and_get_normal(&Isometry3::identity(), &ray, 80.0, true)
                    .is_some()
                {
                    grounded_sides += 1;
                }
            }
            if grounded_sides >= 2 {
                continue; // ground continues on both sides (road/terrain seam)
            }
        }
        out.push(WallSegment {
            a: info.a,
            b: info.b,
            tangent: [dx / len, dz / len],
            len,
            normal,
            two_sided: false,
            y_min: info.y_min - 1.0,
            y_max: info.y_max + 2.5,
            friction: 0.45,
            restitution: 0.0,
            damage: 1.0,
        });
    }
    out
}
/// Complete 2.5D visual wall field of a circuit.
#[derive(Debug, Clone)]
pub struct WallField {
    pub segments: Vec<WallSegment>,
    pub grid: WallGrid,
    pub one_sided_count: usize,
    pub two_sided_count: usize,
}

impl WallField {
    pub fn empty() -> Self {
        Self {
            segments: Vec::new(),
            grid: WallGrid::empty(),
            one_sided_count: 0,
            two_sided_count: 0,
        }
    }

    pub fn is_empty(&self) -> bool {
        self.segments.is_empty()
    }

    pub fn len(&self) -> usize {
        self.segments.len()
    }

    /// Builds a WallField from obstacle triangles only. Synthetic worlds and unit
    /// tests use this; no ground boundary is added.
    pub fn build(driveable: &[DriveableTriangle], obstacles: &[ObstacleTriangle]) -> Self {
        Self::build_internal(driveable, obstacles, false, None)
    }

    /// Builds the session wall field with a simple edge-count boundary (used by
    /// tests that do not have a ground mesh).
    pub fn build_visual(driveable: &[DriveableTriangle], obstacles: &[ObstacleTriangle]) -> Self {
        Self::build_internal(driveable, obstacles, true, None)
    }

    /// Builds the session wall field used by the game: obstacle triangles plus an
    /// invisible wall where the driveable mesh really ends. `mesh` is the same
    /// ground mesh the wheel rays use, so road/terrain seams stay open and only
    /// the outer edge/holes get a boundary.
    pub fn build_visual_with_mesh(
        driveable: &[DriveableTriangle],
        obstacles: &[ObstacleTriangle],
        mesh: &TriMesh,
    ) -> Self {
        Self::build_internal(driveable, obstacles, true, Some(mesh))
    }

    fn build_internal(
        driveable: &[DriveableTriangle],
        obstacles: &[ObstacleTriangle],
        ground_boundary: bool,
        mesh: Option<&TriMesh>,
    ) -> Self {
        if obstacles.is_empty() && !ground_boundary {
            return Self::empty();
        }

        // 1. Extract 2D edges from obstacle triangles
        let mut raw_segments: Vec<WallSegment> = Vec::with_capacity(obstacles.len());

        for tri in obstacles {
            let v0 = tri.vertices[0];
            let v1 = tri.vertices[1];
            let v2 = tri.vertices[2];

            let e1 = v1 - v0;
            let e2 = v2 - v0;
            let n = e1.cross(e2);
            let n_h = Vec3::new(n.x, 0.0, n.z);
            let n_len = n_h.length();
            if n_len <= 1e-5 {
                continue; // Skip horizontal cap triangles
            }
            let normal_2d = [n_h.x / n_len, n_h.z / n_len];

            // Tangent perpendicular to normal: [-nz, nx]
            let tangent = [-normal_2d[1], normal_2d[0]];

            // Project 3 vertices onto tangent
            let p0 = v0.x * tangent[0] + v0.z * tangent[1];
            let p1 = v1.x * tangent[0] + v1.z * tangent[1];
            let p2 = v2.x * tangent[0] + v2.z * tangent[1];

            let (min_v, max_v) = if p0 <= p1 && p0 <= p2 {
                let max = if p1 >= p2 { v1 } else { v2 };
                (v0, max)
            } else if p1 <= p0 && p1 <= p2 {
                let max = if p0 >= p2 { v0 } else { v2 };
                (v1, max)
            } else {
                let max = if p0 >= p1 { v0 } else { v1 };
                (v2, max)
            };

            let ax = min_v.x;
            let az = min_v.z;
            let bx = max_v.x;
            let bz = max_v.z;
            let dx = bx - ax;
            let dz = bz - az;
            let len = (dx * dx + dz * dz).sqrt();
            if len <= 0.05 {
                continue; // Ignore degenerate micro-edges
            }

            let y_min = v0.y.min(v1.y).min(v2.y);
            let y_max = v0.y.max(v1.y).max(v2.y);

            raw_segments.push(WallSegment {
                a: [ax, az],
                b: [bx, bz],
                tangent: [dx / len, dz / len],
                len,
                normal: normal_2d,
                two_sided: false,
                y_min,
                y_max,
                friction: tri.friction,
                restitution: tri.restitution,
                damage: tri.damage,
            });
        }

        if ground_boundary {
            raw_segments.extend(driveable_boundary_segments(driveable, mesh));
        }

        // 2. Collinear segment merging:
        // Group by normal angle (bucket ~2 degrees) and plane offset (bucket ~3 cm)
        let mut merged: Vec<WallSegment> = Vec::with_capacity(raw_segments.len());
        // Simple sort to cluster parallel / coplanar segments
        raw_segments.sort_by(|s1, s2| {
            let ang1 = s1.normal[1].atan2(s1.normal[0]);
            let ang2 = s2.normal[1].atan2(s2.normal[0]);
            ang1.partial_cmp(&ang2)
                .unwrap_or(std::cmp::Ordering::Equal)
                .then_with(|| {
                    let d1 = s1.a[0] * s1.normal[0] + s1.a[1] * s1.normal[1];
                    let d2 = s2.a[0] * s2.normal[0] + s2.a[1] * s2.normal[1];
                    d1.partial_cmp(&d2).unwrap_or(std::cmp::Ordering::Equal)
                })
        });

        for seg in raw_segments {
            let mut absorbed = false;
            if let Some(prev) = merged.last_mut() {
                let dot_n = prev.normal[0] * seg.normal[0] + prev.normal[1] * seg.normal[1];
                if dot_n >= 0.998 {
                    // Check plane offset
                    let d_prev = prev.a[0] * prev.normal[0] + prev.a[1] * prev.normal[1];
                    let d_seg = seg.a[0] * seg.normal[0] + seg.a[1] * seg.normal[1];
                    if (d_prev - d_seg).abs() <= 0.04 {
                        // Check if height extents overlap or touch
                        if prev.y_max >= seg.y_min - 0.20 && seg.y_max >= prev.y_min - 0.20 {
                            // Project onto prev's tangent
                            let t = prev.tangent;
                            let s0 = (seg.a[0] - prev.a[0]) * t[0] + (seg.a[1] - prev.a[1]) * t[1];
                            let s1 = (seg.b[0] - prev.a[0]) * t[0] + (seg.b[1] - prev.a[1]) * t[1];
                            let seg_min_s = s0.min(s1);
                            let seg_max_s = s0.max(s1);

                            // If endpoints overlap or are within 10 cm, merge
                            if seg_min_s <= prev.len + 0.10 && seg_max_s >= -0.10 {
                                let new_min_s = 0.0f64.min(seg_min_s);
                                let new_max_s = prev.len.max(seg_max_s);
                                let new_len = new_max_s - new_min_s;
                                if new_len <= 60.0 {
                                    let orig_a = prev.a;
                                    prev.a = [orig_a[0] + t[0] * new_min_s, orig_a[1] + t[1] * new_min_s];
                                    prev.b = [orig_a[0] + t[0] * new_max_s, orig_a[1] + t[1] * new_max_s];
                                    prev.len = new_len;
                                    prev.y_min = prev.y_min.min(seg.y_min);
                                    prev.y_max = prev.y_max.max(seg.y_max);
                                    absorbed = true;
                                }
                            }
                        }
                    }
                }
            }
            if !absorbed {
                merged.push(seg);
            }
        }

        // 3. Track-oriented normal determination:
        // Sample vertices and centroids over driveable triangles. The samples are
        // bucketed into a 10 m grid: the old brute-force nearest search was
        // O(segments x samples) and dominated the stage load (83 s on Russia after
        // the terrain/prop classification). The maximum useful search radius is
        // 6 m * 2.5 = 15 m, so a 5x5 cell window (20 m) is exact for the scoring
        // below and turns the search into a handful of cells per probe.
        const SAMPLE_CELL: f64 = 10.0;
        const SAMPLE_REACH: i32 = 2;
        let mut sample_grid: HashMap<(i32, i32), Vec<Vec3>> =
            HashMap::with_capacity(driveable.len() * 2);
        for t in driveable {
            let centroid = Vec3::new(
                (t.vertices[0].x + t.vertices[1].x + t.vertices[2].x) / 3.0,
                (t.vertices[0].y + t.vertices[1].y + t.vertices[2].y) / 3.0,
                (t.vertices[0].z + t.vertices[1].z + t.vertices[2].z) / 3.0,
            );
            for sample in [t.vertices[0], t.vertices[1], t.vertices[2], centroid] {
                let key = (
                    (sample.x / SAMPLE_CELL).floor() as i32,
                    (sample.z / SAMPLE_CELL).floor() as i32,
                );
                sample_grid.entry(key).or_default().push(sample);
            }
        }
        let nearest_driveable = |x: f64, z: f64, mid_y: f64| -> f64 {
            let cell_x = (x / SAMPLE_CELL).floor() as i32;
            let cell_z = (z / SAMPLE_CELL).floor() as i32;
            let mut best_sq = f64::INFINITY;
            for dz in -SAMPLE_REACH..=SAMPLE_REACH {
                for dx in -SAMPLE_REACH..=SAMPLE_REACH {
                    let Some(points) = sample_grid.get(&(cell_x + dx, cell_z + dz)) else {
                        continue;
                    };
                    for point in points {
                        if (point.y - mid_y).abs() <= 2.5 {
                            let d_sq =
                                (point.x - x) * (point.x - x) + (point.z - z) * (point.z - z);
                            if d_sq < best_sq {
                                best_sq = d_sq;
                            }
                        }
                    }
                }
            }
            best_sq.sqrt()
        };

        let mut one_sided = 0;
        let mut two_sided = 0;

        for seg in &mut merged {
            let mid_x = (seg.a[0] + seg.b[0]) * 0.5;
            let mid_z = (seg.a[1] + seg.b[1]) * 0.5;
            let mid_y = (seg.y_min + seg.y_max) * 0.5;

            // Probe on both sides along normal: +normal and -normal at distances 0.6m, 1.5m, 3.0m, 6.0m
            let probe_distances = [0.6, 1.5, 3.0, 6.0];
            let mut score_pos = 0;
            let mut score_neg = 0;

            for &dist in &probe_distances {
                let pos_x = mid_x + seg.normal[0] * dist;
                let pos_z = mid_z + seg.normal[1] * dist;
                let neg_x = mid_x - seg.normal[0] * dist;
                let neg_z = mid_z - seg.normal[1] * dist;

                let d_pos = nearest_driveable(pos_x, pos_z, mid_y);
                let d_neg = nearest_driveable(neg_x, neg_z, mid_y);

                if d_pos < d_neg - 0.25 && d_pos <= dist * 2.5 {
                    score_pos += 1;
                } else if d_neg < d_pos - 0.25 && d_neg <= dist * 2.5 {
                    score_neg += 1;
                }
            }

            if score_pos > score_neg {
                // Already pointing toward track
                seg.two_sided = false;
                one_sided += 1;
            } else if score_neg > score_pos {
                // Invert normal to point toward track
                seg.normal = [-seg.normal[0], -seg.normal[1]];
                seg.tangent = [-seg.tangent[0], -seg.tangent[1]];
                std::mem::swap(&mut seg.a, &mut seg.b);
                seg.two_sided = false;
                one_sided += 1;
            } else {
                // Ambiguous or track on both sides (pit wall)
                seg.two_sided = true;
                two_sided += 1;
            }
        }

        let grid = WallGrid::build(&merged, 4.0);

        Self {
            segments: merged,
            grid,
            one_sided_count: one_sided,
            two_sided_count: two_sided,
        }
    }

    /// Queries contact points and signed gaps against a convex polygon with prediction margin.
    /// Queries candidate wall segment indices overlapping bounding box [q_min_x, q_min_z] .. [q_max_x, q_max_z].
    #[inline]
    pub fn query_candidate_walls(&self, q_min_x: f64, q_min_z: f64, q_max_x: f64, q_max_z: f64, out: &mut Vec<u32>) {
        self.grid.query_range(q_min_x, q_min_z, q_max_x, q_max_z, out);
    }

    /// Queries contact points and signed gaps against a convex polygon using pre-queried candidate wall segments.
    ///
    /// The query is a **finite** segment-to-polygon query. A wall segment is a
    /// zero-thickness obstacle with two ends, not an infinite plane: a body vertex
    /// whose nearest point on the segment is an endpoint is at the Euclidean
    /// distance from that endpoint and gets a radial normal, so a car sweeping past
    /// a post is not reported as metres deep just because it crossed the plane of
    /// one of the post's faces. The signed side of a two-sided segment is taken from
    /// the query polygon itself (never from the car centre, which could be on the
    /// other side of the wall and latch a stale direction for two seconds).
    pub fn query_poly_with_candidates(
        &self,
        poly: &ConvexPoly2,
        prediction: f64,
        side_memory: &mut WallSideMemory,
        car_ref_pos: Vec3,
        current_time_s: f64,
        candidates: &[u32],
        out: &mut Vec<WallContact>,
    ) {
        if poly.n < 3 || self.segments.is_empty() || candidates.is_empty() {
            return;
        }

        let pred = prediction.max(0.01);
        let _ = car_ref_pos; // The contact side is a property of the polygon, not of the car centre.
        let (centroid_x, centroid_z) = polygon_centroid(poly);
        let mut poly_min_x = f64::INFINITY;
        let mut poly_min_z = f64::INFINITY;
        let mut poly_max_x = f64::NEG_INFINITY;
        let mut poly_max_z = f64::NEG_INFINITY;
        for i in 0..poly.n {
            poly_min_x = poly_min_x.min(poly.v[i][0]);
            poly_min_z = poly_min_z.min(poly.v[i][1]);
            poly_max_x = poly_max_x.max(poly.v[i][0]);
            poly_max_z = poly_max_z.max(poly.v[i][1]);
        }

        for &wall_idx in candidates.iter() {
            let seg = &self.segments[wall_idx as usize];

            // 0. Segment AABB against the polygon AABB: the grid candidate set is
            // a disc around the car, most of which cannot touch this polygon.
            if seg.a[0].min(seg.b[0]) > poly_max_x + pred
                || seg.a[0].max(seg.b[0]) < poly_min_x - pred
                || seg.a[1].min(seg.b[1]) > poly_max_z + pred
                || seg.a[1].max(seg.b[1]) < poly_min_z - pred
            {
                continue;
            }

            // 1. Height overlap. The speculative prediction is a horizontal
            // margin; it must not lift a 10 cm wall into a front wing 40 cm above.
            if poly.y_max < seg.y_min - VERTICAL_TOLERANCE_M
                || poly.y_min > seg.y_max + VERTICAL_TOLERANCE_M
            {
                continue;
            }

            // 2. Effective normal. Two-sided segments face the body's own centroid
            // (hysteresis only matters when the centroid is exactly on the plane);
            // one-sided segments keep their authoritative track-facing normal.
            let centroid_side = (centroid_x - seg.a[0]) * seg.normal[0]
                + (centroid_z - seg.a[1]) * seg.normal[1];
            // A one-sided wall may recover a body that is shallowly behind it (a
            // tunnelling correction). A body metres behind a barrier is outside
            // that wall and must never be dragged back through it.
            if !seg.two_sided && centroid_side < -BACK_RECOVERY_BAND_M {
                continue;
            }
            let effective_normal = if seg.two_sided {
                let locked = side_memory.get_or_set(wall_idx, centroid_side, current_time_s);
                if locked < 0.0 {
                    [-seg.normal[0], -seg.normal[1]]
                } else {
                    seg.normal
                }
            } else {
                seg.normal
            };

            let contact_y = contact_height(poly, seg);
            let tangent = seg.tangent;
            let len = seg.len;

            // At most 8 polygon vertices + the two segment endpoints.
            let mut candidates_local: [(f64, Vec3, Vec3); 10] =
                [(f64::INFINITY, Vec3::ZERO, Vec3::ZERO); 10];
            let mut candidate_count = 0usize;

            // 3. Every polygon vertex against the finite segment.
            for i in 0..poly.n {
                let px = poly.v[i][0];
                let pz = poly.v[i][1];

                let along = (px - seg.a[0]) * tangent[0] + (pz - seg.a[1]) * tangent[1];
                let clamped = along.clamp(0.0, len);
                let qx = seg.a[0] + tangent[0] * clamped;
                let qz = seg.a[1] + tangent[1] * clamped;
                let dx = px - qx;
                let dz = pz - qz;
                let distance = (dx * dx + dz * dz).sqrt();

                // Only an interior projection lies on a side of the wall; beyond
                // an end the wall stops and the distance is always positive.
                let interior = along > 0.0 && along < len;
                let signed = (px - seg.a[0]) * effective_normal[0]
                    + (pz - seg.a[1]) * effective_normal[1];
                let gap = if interior {
                    if signed >= 0.0 { distance } else { -distance }
                } else {
                    distance
                };

                if gap <= pred && candidate_count < candidates_local.len() {
                    let normal = if interior {
                        Vec3::new(effective_normal[0], 0.0, effective_normal[1])
                    } else if distance > 1e-9 {
                        Vec3::new(dx / distance, 0.0, dz / distance)
                    } else {
                        Vec3::new(effective_normal[0], 0.0, effective_normal[1])
                    };
                    candidates_local[candidate_count] =
                        (gap, Vec3::new(qx, contact_y, qz), normal);
                    candidate_count += 1;
                }
            }

            // 4. A segment endpoint inside the body polygon means the wall end is
            // under the car: push the car out through the nearest body edge.
            for endpoint in [seg.a, seg.b] {
                let point = [endpoint[0], endpoint[1]];
                if point_in_convex_polygon(point, poly) {
                    let (depth, outward) = nearest_polygon_edge(point, poly);
                    // Freeing an endpoint that is inside the body means moving the
                    // body opposite the nearest edge's outward normal, so the edge
                    // travels past the endpoint. Push direction = -outward.
                    let normal = [-outward[0], -outward[1]];
                    if candidate_count < candidates_local.len() {
                        candidates_local[candidate_count] = (
                            -depth,
                            Vec3::new(point[0], contact_y, point[1]),
                            Vec3::new(normal[0], 0.0, normal[1]),
                        );
                        candidate_count += 1;
                    }
                    continue;
                }
                // Outside the body: the wall end is a rounded post. A body edge can
                // be closer to it than any body vertex, so this is the missing
                // speculative contact for a corner near a body flank.
                if !seg.two_sided && centroid_side < 0.0 {
                    continue;
                }
                let (distance, closest) = closest_polygon_edge(point, poly);
                if distance > 1e-9 && distance <= pred && candidate_count < candidates_local.len() {
                    let nx = (closest[0] - point[0]) / distance;
                    let nz = (closest[1] - point[1]) / distance;
                    candidates_local[candidate_count] = (
                        distance,
                        Vec3::new(point[0], contact_y, point[1]),
                        Vec3::new(nx, 0.0, nz),
                    );
                    candidate_count += 1;
                }
            }

            // 6. Keep the deepest contact and a second one along the wall for
            // torque stability, never a duplicate of the same point.
            let mut first: Option<(f64, Vec3, Vec3)> = None;
            let mut second: Option<(f64, Vec3, Vec3)> = None;
            for &candidate in &candidates_local[..candidate_count] {
                if first.is_none() || candidate.0 < first.unwrap().0 {
                    second = first;
                    first = Some(candidate);
                } else if second.is_none() || candidate.0 < second.unwrap().0 {
                    second = Some(candidate);
                }
            }

            if let Some((gap, point, normal)) = first {
                if gap <= pred {
                    out.push(WallContact {
                        point,
                        normal,
                        gap_m: gap,
                        wall_index: wall_idx,
                        friction: seg.friction,
                        restitution: seg.restitution,
                        damage: seg.damage,
                    });
                }
            }
            if let Some((gap, point, normal)) = second {
                let separated = first
                    .map(|(_, p, _)| (p - point).length() > 0.10)
                    .unwrap_or(true);
                if gap <= pred && separated {
                    out.push(WallContact {
                        point,
                        normal,
                        gap_m: gap,
                        wall_index: wall_idx,
                        friction: seg.friction,
                        restitution: seg.restitution,
                        damage: seg.damage,
                    });
                }
            }
        }
    }

    /// Queries contact points and signed gaps against a convex polygon with prediction margin.
    pub fn query_poly(
        &self,
        poly: &ConvexPoly2,
        prediction: f64,
        side_memory: &mut WallSideMemory,
        car_ref_pos: Vec3,
        current_time_s: f64,
        candidates_scratch: &mut Vec<u32>,
        out: &mut Vec<WallContact>,
    ) {
        if poly.n < 3 || self.segments.is_empty() {
            return;
        }

        let mut p_min_x = f64::INFINITY;
        let mut p_min_z = f64::INFINITY;
        let mut p_max_x = f64::NEG_INFINITY;
        let mut p_max_z = f64::NEG_INFINITY;

        for i in 0..poly.n {
            p_min_x = p_min_x.min(poly.v[i][0]);
            p_min_z = p_min_z.min(poly.v[i][1]);
            p_max_x = p_max_x.max(poly.v[i][0]);
            p_max_z = p_max_z.max(poly.v[i][1]);
        }

        let pred = prediction.max(0.01);
        self.grid.query_range(
            p_min_x - pred,
            p_min_z - pred,
            p_max_x + pred,
            p_max_z + pred,
            candidates_scratch,
        );

        self.query_poly_with_candidates(
            poly,
            prediction,
            side_memory,
            car_ref_pos,
            current_time_s,
            candidates_scratch,
            out,
        );
    }
}
