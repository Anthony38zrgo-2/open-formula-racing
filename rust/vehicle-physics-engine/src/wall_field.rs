//! 2.5D Obstacle Wall representation with track-oriented outward normals.
//!
//! Converts 3D obstacle triangles from visual track models (.acc/.ac) into vertical
//! 2D wall segments with a height range [y_min, y_max].
//! Each wall segment has a fixed, authoritative horizontal normal pointing towards
//! the driveable track surface, determined at track load time.

use crate::ac3d_track::{DriveableTriangle, ObstacleTriangle};
use crate::types::Vec3;

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

        let mut lower = Vec::with_capacity(8);
        for p in &sorted {
            while lower.len() >= 2 && cross2(lower[lower.len() - 2], lower[lower.len() - 1], *p) <= 1e-9 {
                lower.pop();
            }
            lower.push(*p);
        }

        let mut upper = Vec::with_capacity(8);
        for p in sorted.iter().rev() {
            while upper.len() >= 2 && cross2(upper[upper.len() - 2], upper[upper.len() - 1], *p) <= 1e-9 {
                upper.pop();
            }
            upper.push(*p);
        }

        lower.pop();
        upper.pop();
        lower.extend(upper);

        let n = lower.len().min(8);
        let mut v = [[0.0; 2]; 8];
        for i in 0..n {
            v[i] = lower[i];
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

/// Memory of which side of a two-sided wall the vehicle was last on.
#[derive(Debug, Clone, Default)]
pub struct WallSideMemory {
    /// (wall_index, last_side_sign: f64, last_seen_time_s: f64)
    entries: Vec<(u32, f64, f64)>,
}

impl WallSideMemory {
    pub fn get_or_set(&mut self, wall_index: u32, current_sign: f64, current_time_s: f64) -> f64 {
        // Prune old entries (> 2.0 s)
        self.entries.retain(|e| (current_time_s - e.2).abs() < 2.0);

        if let Some(entry) = self.entries.iter_mut().find(|e| e.0 == wall_index) {
            entry.2 = current_time_s;
            entry.1
        } else {
            let sign = if current_sign >= 0.0 { 1.0 } else { -1.0 };
            self.entries.push((wall_index, sign, current_time_s));
            sign
        }
    }
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

    /// Builds a WallField from parsed driveable triangles and obstacle triangles.
    pub fn build(driveable: &[DriveableTriangle], obstacles: &[ObstacleTriangle]) -> Self {
        if obstacles.is_empty() {
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
        // Sample vertices and centroids over driveable triangles for robust nearest-surface lookup
        let mut d_centers: Vec<Vec3> = Vec::with_capacity(driveable.len() * 4);
        for t in driveable {
            d_centers.push(t.vertices[0]);
            d_centers.push(t.vertices[1]);
            d_centers.push(t.vertices[2]);
            d_centers.push(Vec3::new(
                (t.vertices[0].x + t.vertices[1].x + t.vertices[2].x) / 3.0,
                (t.vertices[0].y + t.vertices[1].y + t.vertices[2].y) / 3.0,
                (t.vertices[0].z + t.vertices[1].z + t.vertices[2].z) / 3.0,
            ));
        }

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

                let mut min_pos_sq = f64::INFINITY;
                let mut min_neg_sq = f64::INFINITY;

                for c in &d_centers {
                    if (c.y - mid_y).abs() <= 2.5 {
                        let d_pos = (c.x - pos_x) * (c.x - pos_x) + (c.z - pos_z) * (c.z - pos_z);
                        let d_neg = (c.x - neg_x) * (c.x - neg_x) + (c.z - neg_z) * (c.z - neg_z);
                        min_pos_sq = min_pos_sq.min(d_pos);
                        min_neg_sq = min_neg_sq.min(d_neg);
                    }
                }

                let d_pos = min_pos_sq.sqrt();
                let d_neg = min_neg_sq.sqrt();

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

        for &wall_idx in candidates_scratch.iter() {
            let seg = &self.segments[wall_idx as usize];

            // 1. Height range overlap
            if poly.y_max < seg.y_min - pred || poly.y_min > seg.y_max + pred {
                continue;
            }

            // 2. Normal orientation: determine effective normal
            let mut effective_normal = seg.normal;
            if seg.two_sided {
                let to_car_x = car_ref_pos.x - seg.a[0];
                let to_car_z = car_ref_pos.z - seg.a[1];
                let side_sign = to_car_x * seg.normal[0] + to_car_z * seg.normal[1];
                let locked_sign = side_memory.get_or_set(wall_idx, side_sign, current_time_s);
                if locked_sign < 0.0 {
                    effective_normal = [-seg.normal[0], -seg.normal[1]];
                }
            }

            // 3. For each vertex of the polygon, evaluate distance along normal and tangent projection
            let mut min_gap = f64::INFINITY;
            let mut deepest_pt = Vec3::ZERO;
            let mut second_gap = f64::INFINITY;
            let mut second_pt = Vec3::ZERO;

            let t = [
                seg.b[0] - seg.a[0],
                seg.b[1] - seg.a[1],
            ];
            let len_sq = seg.len * seg.len;

            for i in 0..poly.n {
                let vx = poly.v[i][0];
                let vz = poly.v[i][1];

                // Tangent projection parameter u in [0, 1]
                let u = ((vx - seg.a[0]) * t[0] + (vz - seg.a[1]) * t[1]) / len_sq;
                // Allow a small end margin (0.10 m) to bridge adjacent wall segments seamlessly
                let end_margin = 0.10 / seg.len;
                if u >= -end_margin && u <= 1.0 + end_margin {
                    // Signed gap along effective normal: gap = (P - A) . normal
                    let gap = (vx - seg.a[0]) * effective_normal[0] + (vz - seg.a[1]) * effective_normal[1];
                    if gap <= pred {
                        let contact_y = (poly.y_min + poly.y_max) * 0.5;
                        let pt = Vec3::new(vx, contact_y, vz);

                        if gap < min_gap {
                            second_gap = min_gap;
                            second_pt = deepest_pt;
                            min_gap = gap;
                            deepest_pt = pt;
                        } else if gap < second_gap && (pt - deepest_pt).length() > 0.05 {
                            second_gap = gap;
                            second_pt = pt;
                        }
                    }
                }
            }

            if min_gap <= pred {
                out.push(WallContact {
                    point: deepest_pt,
                    normal: Vec3::new(effective_normal[0], 0.0, effective_normal[1]),
                    gap_m: min_gap,
                    wall_index: wall_idx,
                    friction: seg.friction,
                    restitution: seg.restitution,
                    damage: seg.damage,
                });

                if second_gap <= pred {
                    out.push(WallContact {
                        point: second_pt,
                        normal: Vec3::new(effective_normal[0], 0.0, effective_normal[1]),
                        gap_m: second_gap,
                        wall_index: wall_idx,
                        friction: seg.friction,
                        restitution: seg.restitution,
                        damage: seg.damage,
                    });
                }
            }
        }
    }
}
