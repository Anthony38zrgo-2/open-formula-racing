use serde::{Deserialize, Serialize};
use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

/// Deterministic 3D vector with f64 precision.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Default for Vec3 {
    #[inline]
    fn default() -> Self {
        Self::ZERO
    }
}

impl Vec3 {
    pub const ZERO: Self = Self {
        x: 0.0,
        y: 0.0,
        z: 0.0,
    };
    pub const ONE: Self = Self {
        x: 1.0,
        y: 1.0,
        z: 1.0,
    };
    pub const UP: Self = Self {
        x: 0.0,
        y: 1.0,
        z: 0.0,
    };
    pub const DOWN: Self = Self {
        x: 0.0,
        y: -1.0,
        z: 0.0,
    };
    pub const RIGHT: Self = Self {
        x: 1.0,
        y: 0.0,
        z: 0.0,
    };
    pub const LEFT: Self = Self {
        x: -1.0,
        y: 0.0,
        z: 0.0,
    };
    pub const FORWARD: Self = Self {
        x: 0.0,
        y: 0.0,
        z: -1.0,
    }; // Godot standard: -Z forward
    pub const BACK: Self = Self {
        x: 0.0,
        y: 0.0,
        z: 1.0,
    };

    #[inline]
    pub const fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    #[inline]
    pub fn dot(self, other: Self) -> f64 {
        self.x * other.x + self.y * other.y + self.z * other.z
    }

    #[inline]
    pub fn cross(self, other: Self) -> Self {
        Self {
            x: self.y * other.z - self.z * other.y,
            y: self.z * other.x - self.x * other.z,
            z: self.x * other.y - self.y * other.x,
        }
    }

    #[inline]
    pub fn length_squared(self) -> f64 {
        self.dot(self)
    }

    #[inline]
    pub fn length(self) -> f64 {
        self.length_squared().sqrt()
    }

    #[inline]
    pub fn is_finite(self) -> bool {
        self.x.is_finite() && self.y.is_finite() && self.z.is_finite()
    }

    #[inline]
    pub fn distance_to(self, other: Self) -> f64 {
        (self - other).length()
    }

    #[inline]
    pub fn normalized(self) -> Self {
        let len_sq = self.length_squared();
        if len_sq > 1e-12 {
            let inv_len = 1.0 / len_sq.sqrt();
            self * inv_len
        } else {
            Self::ZERO
        }
    }

    #[inline]
    pub fn is_zero_approx(self) -> bool {
        self.x.abs() < 1e-6 && self.y.abs() < 1e-6 && self.z.abs() < 1e-6
    }
}

impl Add for Vec3 {
    type Output = Self;
    #[inline]
    fn add(self, rhs: Self) -> Self {
        Self::new(self.x + rhs.x, self.y + rhs.y, self.z + rhs.z)
    }
}

impl AddAssign for Vec3 {
    #[inline]
    fn add_assign(&mut self, rhs: Self) {
        self.x += rhs.x;
        self.y += rhs.y;
        self.z += rhs.z;
    }
}

impl Sub for Vec3 {
    type Output = Self;
    #[inline]
    fn sub(self, rhs: Self) -> Self {
        Self::new(self.x - rhs.x, self.y - rhs.y, self.z - rhs.z)
    }
}

impl SubAssign for Vec3 {
    #[inline]
    fn sub_assign(&mut self, rhs: Self) {
        self.x -= rhs.x;
        self.y -= rhs.y;
        self.z -= rhs.z;
    }
}

impl Mul<f64> for Vec3 {
    type Output = Self;
    #[inline]
    fn mul(self, rhs: f64) -> Self {
        Self::new(self.x * rhs, self.y * rhs, self.z * rhs)
    }
}

impl Mul<Vec3> for f64 {
    type Output = Vec3;
    #[inline]
    fn mul(self, rhs: Vec3) -> Vec3 {
        Vec3::new(self * rhs.x, self * rhs.y, self * rhs.z)
    }
}

impl MulAssign<f64> for Vec3 {
    #[inline]
    fn mul_assign(&mut self, rhs: f64) {
        self.x *= rhs;
        self.y *= rhs;
        self.z *= rhs;
    }
}

impl Div<f64> for Vec3 {
    type Output = Self;
    #[inline]
    fn div(self, rhs: f64) -> Self {
        let inv = 1.0 / rhs;
        Self::new(self.x * inv, self.y * inv, self.z * inv)
    }
}

impl DivAssign<f64> for Vec3 {
    #[inline]
    fn div_assign(&mut self, rhs: f64) {
        let inv = 1.0 / rhs;
        self.x *= inv;
        self.y *= inv;
        self.z *= inv;
    }
}

impl Neg for Vec3 {
    type Output = Self;
    #[inline]
    fn neg(self) -> Self {
        Self::new(-self.x, -self.y, -self.z)
    }
}

/// 3x3 Matrix for 3D basis orientations.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct Mat3 {
    pub x: Vec3, // Right vector
    pub y: Vec3, // Up vector
    pub z: Vec3, // Back vector (+Z back, -Z forward in Godot convention)
}

impl Mat3 {
    pub const IDENTITY: Self = Self {
        x: Vec3::RIGHT,
        y: Vec3::UP,
        z: Vec3::BACK,
    };

    #[inline]
    pub const fn from_cols(x: Vec3, y: Vec3, z: Vec3) -> Self {
        Self { x, y, z }
    }

    #[inline]
    pub fn transform_vector(&self, v: Vec3) -> Vec3 {
        self.x * v.x + self.y * v.y + self.z * v.z
    }

    #[inline]
    pub fn inverse_transform_vector(&self, v: Vec3) -> Vec3 {
        // For orthogonal rotation matrices, inverse is transpose
        Vec3::new(self.x.dot(v), self.y.dot(v), self.z.dot(v))
    }

    #[inline]
    pub fn transpose(&self) -> Self {
        Self {
            x: Vec3::new(self.x.x, self.y.x, self.z.x),
            y: Vec3::new(self.x.y, self.y.y, self.z.y),
            z: Vec3::new(self.x.z, self.y.z, self.z.z),
        }
    }

    /// Construct from Euler angles in radians (Order: YXZ - Yaw, Pitch, Roll).
    pub fn from_euler_yxz(yaw: f64, pitch: f64, roll: f64) -> Self {
        let (sy, cy) = (yaw.sin(), yaw.cos());
        let (sp, cp) = (pitch.sin(), pitch.cos());
        let (sr, cr) = (roll.sin(), roll.cos());

        let x = Vec3::new(cy * cr + sy * sp * sr, cp * sr, -sy * cr + cy * sp * sr);
        let y = Vec3::new(-cy * sr + sy * sp * cr, cp * cr, sy * sr + cy * sp * cr);
        let z = Vec3::new(sy * cp, -sp, cy * cp);

        Self { x, y, z }
    }

    /// Rotation around arbitrary axis by angle in radians.
    pub fn from_axis_angle(axis: Vec3, angle: f64) -> Self {
        let a = axis.normalized();
        let (s, c) = (angle.sin(), angle.cos());
        let omc = 1.0 - c;

        Self {
            x: Vec3::new(
                c + a.x * a.x * omc,
                a.y * a.x * omc + a.z * s,
                a.z * a.x * omc - a.y * s,
            ),
            y: Vec3::new(
                a.x * a.y * omc - a.z * s,
                c + a.y * a.y * omc,
                a.z * a.y * omc + a.x * s,
            ),
            z: Vec3::new(
                a.x * a.z * omc + a.y * s,
                a.y * a.z * omc - a.x * s,
                c + a.z * a.z * omc,
            ),
        }
    }
}

/// Quaternion representation for 3D rotations without singularity.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct Quat {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

impl Quat {
    pub const IDENTITY: Self = Self {
        x: 0.0,
        y: 0.0,
        z: 0.0,
        w: 1.0,
    };

    #[inline]
    pub const fn new(x: f64, y: f64, z: f64, w: f64) -> Self {
        Self { x, y, z, w }
    }

    #[inline]
    pub fn length_squared(self) -> f64 {
        self.x * self.x + self.y * self.y + self.z * self.z + self.w * self.w
    }

    #[inline]
    pub fn normalized(self) -> Self {
        let len_sq = self.length_squared();
        if len_sq > 1e-12 {
            let inv = 1.0 / len_sq.sqrt();
            Self::new(self.x * inv, self.y * inv, self.z * inv, self.w * inv)
        } else {
            Self::IDENTITY
        }
    }

    #[inline]
    pub fn rotate_vec3(self, v: Vec3) -> Vec3 {
        let qv = Vec3::new(self.x, self.y, self.z);
        let uv = qv.cross(v);
        let uuv = qv.cross(uv);
        v + (uv * self.w + uuv) * 2.0
    }

    pub fn to_mat3(self) -> Mat3 {
        let q = self.normalized();
        let x2 = q.x + q.x;
        let y2 = q.y + q.y;
        let z2 = q.z + q.z;
        let xx = q.x * x2;
        let xy = q.x * y2;
        let xz = q.x * z2;
        let yy = q.y * y2;
        let yz = q.y * z2;
        let zz = q.z * z2;
        let wx = q.w * x2;
        let wy = q.w * y2;
        let wz = q.w * z2;

        Mat3 {
            x: Vec3::new(1.0 - (yy + zz), xy + wz, xz - wy),
            y: Vec3::new(xy - wz, 1.0 - (xx + zz), yz + wx),
            z: Vec3::new(xz + wy, yz - wx, 1.0 - (xx + yy)),
        }
    }

    pub fn from_mat3(m: &Mat3) -> Self {
        let trace = m.x.x + m.y.y + m.z.z;
        if trace > 0.0 {
            let s = 0.5 / (trace + 1.0).sqrt();
            Self::new(
                (m.y.z - m.z.y) * s,
                (m.z.x - m.x.z) * s,
                (m.x.y - m.y.x) * s,
                0.25 / s,
            )
        } else if m.x.x > m.y.y && m.x.x > m.z.z {
            let s = 2.0 * (1.0 + m.x.x - m.y.y - m.z.z).sqrt();
            Self::new(
                0.25 * s,
                (m.x.y + m.y.x) / s,
                (m.x.z + m.z.x) / s,
                (m.y.z - m.z.y) / s,
            )
        } else if m.y.y > m.z.z {
            let s = 2.0 * (1.0 + m.y.y - m.x.x - m.z.z).sqrt();
            Self::new(
                (m.x.y + m.y.x) / s,
                0.25 * s,
                (m.y.z + m.z.y) / s,
                (m.z.x - m.x.z) / s,
            )
        } else {
            let s = 2.0 * (1.0 + m.z.z - m.x.x - m.y.y).sqrt();
            Self::new(
                (m.x.z + m.z.x) / s,
                (m.y.z + m.z.y) / s,
                0.25 * s,
                (m.x.y - m.y.x) / s,
            )
        }
    }

    /// Angular integration from angular velocity vector (in rad/s) over timestep dt.
    pub fn integrate_angular_velocity(self, omega: Vec3, dt: f64) -> Self {
        let half_dt = 0.5 * dt;
        let delta_q = Quat::new(omega.x * half_dt, omega.y * half_dt, omega.z * half_dt, 1.0);
        // Quaternion multiplication: self * delta_q
        Self::new(
            self.w * delta_q.x + self.x * delta_q.w + self.y * delta_q.z - self.z * delta_q.y,
            self.w * delta_q.y - self.x * delta_q.z + self.y * delta_q.w + self.z * delta_q.x,
            self.w * delta_q.z + self.x * delta_q.y - self.y * delta_q.x + self.z * delta_q.w,
            self.w * delta_q.w - self.x * delta_q.x - self.y * delta_q.y - self.z * delta_q.z,
        )
        .normalized()
    }
}

/// 3D Transform holding position and orientation basis.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct Transform3D {
    pub origin: Vec3,
    pub basis: Mat3,
}

impl Transform3D {
    pub const IDENTITY: Self = Self {
        origin: Vec3::ZERO,
        basis: Mat3::IDENTITY,
    };

    #[inline]
    pub const fn new(origin: Vec3, basis: Mat3) -> Self {
        Self { origin, basis }
    }

    #[inline]
    pub fn transform_point(&self, p: Vec3) -> Vec3 {
        self.origin + self.basis.transform_vector(p)
    }

    #[inline]
    pub fn inverse_transform_point(&self, p: Vec3) -> Vec3 {
        self.basis.inverse_transform_vector(p - self.origin)
    }
}

/// Surface types for traction, rolling resistance and acoustics.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize, Default)]
pub enum SurfaceType {
    #[default]
    Road,
    Curb,
    Dirt,
    Grass,
    Gravel,
    Sand,
    Wall,
    Metal,
}

/// Wheel index enumeration (FL=0, FR=1, RL=2, RR=3).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum WheelIndex {
    FrontLeft = 0,
    FrontRight = 1,
    RearLeft = 2,
    RearRight = 3,
}

/// Stable short name of a wheel, for telemetry and logs. Rust order is FL, FR, RL, RR.
pub fn wheel_name(wheel: WheelIndex) -> &'static str {
    match wheel {
        WheelIndex::FrontLeft => "FL",
        WheelIndex::FrontRight => "FR",
        WheelIndex::RearLeft => "RL",
        WheelIndex::RearRight => "RR",
    }
}

impl WheelIndex {
    pub const ALL: [Self; 4] = [
        Self::FrontLeft,
        Self::FrontRight,
        Self::RearLeft,
        Self::RearRight,
    ];

    #[inline]
    pub const fn is_front(self) -> bool {
        matches!(self, Self::FrontLeft | Self::FrontRight)
    }

    #[inline]
    pub const fn is_rear(self) -> bool {
        matches!(self, Self::RearLeft | Self::RearRight)
    }

    #[inline]
    pub const fn is_left(self) -> bool {
        matches!(self, Self::FrontLeft | Self::RearLeft)
    }

    #[inline]
    pub const fn is_right(self) -> bool {
        matches!(self, Self::FrontRight | Self::RearRight)
    }

    #[inline]
    pub const fn opposite(self) -> Self {
        match self {
            Self::FrontLeft => Self::FrontRight,
            Self::FrontRight => Self::FrontLeft,
            Self::RearLeft => Self::RearRight,
            Self::RearRight => Self::RearLeft,
        }
    }
}

/// Single raycast hit result from environment collision query.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct RaycastHit {
    pub is_colliding: bool,
    pub distance: f64,
    pub point: Vec3,
    pub normal: Vec3,
    pub surface: SurfaceType,
    /// Physical per-triangle grip coefficient from the collision-world source.
    /// Legacy callers that do not provide material data preserve the configured
    /// vehicle surface coefficient by using the neutral multiplier of 1.
    #[serde(default = "neutral_surface_multiplier")]
    pub surface_friction_multiplier: f64,
    /// Physical per-triangle rolling-resistance coefficient. Neutral for legacy
    /// callers that only provide a surface category.
    #[serde(default = "neutral_surface_multiplier")]
    pub surface_rolling_resistance_multiplier: f64,
}

fn neutral_surface_multiplier() -> f64 {
    1.0
}

impl Default for RaycastHit {
    fn default() -> Self {
        Self {
            is_colliding: false,
            distance: f64::INFINITY,
            point: Vec3::ZERO,
            normal: Vec3::UP,
            surface: SurfaceType::Road,
            surface_friction_multiplier: 1.0,
            surface_rolling_resistance_multiplier: 1.0,
        }
    }
}

/// Geometric context of one tricast query.
///
/// `from_wheel_centre` selects the distance contract that produced the three hits:
///
/// * `true` (current contract, used by the Speed Dreams host through
///   `OpenFormulaSimulation`): every distance is measured from the **physical wheel
///   centre** along the suspension axis. The conversion to a suspension compression
///   is `compression_at_sample - signed_gap`, where
///   `signed_gap = distance * (up . normal) - radius` and positive means the wheel
///   floats above the road. This is the only contract in which moving the ray origin
///   actually moves the wheel.
/// * `false` (retired chassis-anchor contract, reachable from the legacy Godot FFI
///   entry points and from unit tests that build samples by hand): distances are
///   measured from the chassis attachment point and the old
///   `spring_length + radius - distance` conversion still applies. It is kept so the
///   retired `f1_94_physics_*` ABI keeps its documented meaning, and it must never be
///   produced by the Speed Dreams adapter.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct WheelRayContext {
    /// Distance contract of the three hits. See the type documentation.
    pub from_wheel_centre: bool,
    /// Physical wheel centre at sample time (world, m).
    pub centre_world: Vec3,
    /// Wheel-width direction after steer, toe and camber at sample time.
    pub lateral_axis_world: Vec3,
    /// Chassis attachment point at sample time (world, m).
    pub anchor_world: Vec3,
    /// Suspension up axis at sample time (unit, world). May be zero when unknown;
    /// the consumer then falls back to the body up axis.
    pub up: Vec3,
    /// Unsprung coordinate `q` (compression) at sample time (m).
    pub compression_m: f64,
    /// Physical tire radius (m), never the effective rolling radius.
    pub radius_m: f64,
    /// Transverse half-spread of the outer rays from the wheel centre (m). The inner
    /// ray sits at `+span` on a left wheel and `-span` on a right wheel.
    pub span_m: f64,
    /// Transverse rays the producer actually cast for this corner: `1` (centre only;
    /// inner/outer repeat the centre hit so the patch is represented as one plane) or
    /// `3` (tricast). `0` means "not declared" and is read as a tricast, so hand-built
    /// samples keep their historical meaning.
    #[serde(default)]
    pub ray_count: u8,
}

impl WheelRayContext {
    /// Rays actually cast for this corner: 1 or 3. `0` (unspecified) means 3.
    pub fn effective_ray_count(&self) -> usize {
        if self.ray_count == 1 {
            1
        } else {
            3
        }
    }
}

impl Default for WheelRayContext {
    fn default() -> Self {
        Self {
            from_wheel_centre: false,
            centre_world: Vec3::ZERO,
            lateral_axis_world: Vec3::RIGHT,
            anchor_world: Vec3::ZERO,
            up: Vec3::UP,
            compression_m: 0.0,
            radius_m: 0.0,
            span_m: 0.0,
            ray_count: 0,
        }
    }
}

impl WheelRayContext {
    /// Current contract: hits measured from the physical wheel centre.
    pub fn from_wheel_centre(
        centre_world: Vec3,
        lateral_axis_world: Vec3,
        anchor_world: Vec3,
        up: Vec3,
        compression_m: f64,
        radius_m: f64,
        span_m: f64,
    ) -> Self {
        let up = if up.length() > 0.5 {
            up.normalized()
        } else {
            Vec3::UP
        };
        Self {
            from_wheel_centre: true,
            centre_world,
            lateral_axis_world,
            anchor_world,
            up,
            compression_m,
            radius_m,
            span_m,
            ray_count: 0,
        }
    }

    /// Declare how many transverse rays produced this sample (1 or 3). The sampler
    /// sets it; hand-built samples leave it at 0, which reads as a tricast.
    pub fn with_ray_count(mut self, ray_count: u8) -> Self {
        self.ray_count = ray_count;
        self
    }

    /// Retired chassis-anchor contract (legacy FFI and hand-built unit-test samples).
    pub fn from_chassis_anchor() -> Self {
        Self::default()
    }

    /// Suspension axis used by the consumer, with a safe fallback.
    pub fn axis_or(&self, fallback: Vec3) -> Vec3 {
        if self.up.length() > 0.5 {
            self.up.normalized()
        } else if fallback.length() > 0.5 {
            fallback.normalized()
        } else {
            Vec3::UP
        }
    }
}

/// Tri-Raycast sample containing 3 parallel transverse rays (Inner, Center, Outer).
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize, Default)]
pub struct TriRaycastSample {
    pub inner: RaycastHit,
    pub center: RaycastHit,
    pub outer: RaycastHit,
    /// Origin and interpretation of the three hits.
    pub context: WheelRayContext,
}

impl TriRaycastSample {
    /// Samples produced by the current wheel-centre contract.
    pub fn wheel_centre(
        inner: RaycastHit,
        center: RaycastHit,
        outer: RaycastHit,
        context: WheelRayContext,
    ) -> Self {
        Self {
            inner,
            center,
            outer,
            context,
        }
    }

    /// True when the three distances are measured from the physical wheel centre.
    #[inline]
    pub fn measured_from_wheel_centre(&self) -> bool {
        self.context.from_wheel_centre
    }

    /// Signed gap of one hit: positive = the ray origin floats above the road.
    ///
    /// `distance` is measured along `axis` from the ray origin to the surface. The
    /// perpendicular clearance is `distance * (axis . normal)`, so a banked or ramped
    /// triangle reports the true normal gap instead of a shortened slant distance.
    /// A surface whose normal is almost perpendicular to the suspension axis (a
    /// wall) is not a support plane and reports no gap.
    pub fn signed_gap(hit: &RaycastHit, axis: Vec3, radius: f64) -> f64 {
        if !hit.is_colliding || !hit.distance.is_finite() {
            return f64::INFINITY;
        }
        let alignment = axis.dot(hit.normal);
        if alignment < 0.15 {
            return f64::INFINITY;
        }
        hit.distance * alignment - radius.max(0.0)
    }
}

impl TriRaycastSample {
    #[inline]
    pub fn has_any_contact(&self) -> bool {
        self.inner.is_colliding || self.center.is_colliding || self.outer.is_colliding
    }

    #[inline]
    pub fn contact_count(&self) -> usize {
        (self.inner.is_colliding as usize)
            + (self.center.is_colliding as usize)
            + (self.outer.is_colliding as usize)
    }

    /// Calculate weighted average hit distance using 1:2:1 transverse weighting.
    /// Only colliding rays contribute to the average; if none collide, returns max_length.
    pub fn weighted_distance(&self, max_length: f64) -> f64 {
        let mut sum = 0.0;
        let mut weight_sum = 0.0;
        if self.inner.is_colliding {
            sum += self.inner.distance;
            weight_sum += 1.0;
        }
        if self.center.is_colliding {
            sum += 2.0 * self.center.distance;
            weight_sum += 2.0;
        }
        if self.outer.is_colliding {
            sum += self.outer.distance;
            weight_sum += 1.0;
        }
        if weight_sum > 0.0 {
            sum / weight_sum
        } else {
            max_length
        }
    }

    /// Calculate weighted average surface normal using 1:2:1 transverse weighting.
    /// Only colliding rays contribute; falls back to Vec3::UP if none collide.
    pub fn weighted_normal(&self) -> Vec3 {
        let mut sum = Vec3::ZERO;
        let mut weight_sum = 0.0;
        if self.inner.is_colliding {
            sum += self.inner.normal;
            weight_sum += 1.0;
        }
        if self.center.is_colliding {
            sum += self.center.normal * 2.0;
            weight_sum += 2.0;
        }
        if self.outer.is_colliding {
            sum += self.outer.normal;
            weight_sum += 1.0;
        }
        if weight_sum > 0.0 {
            sum.normalized()
        } else {
            Vec3::UP
        }
    }
}

/// Real-time driver input frame.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct VehicleInput {
    pub steering: f64,            // -1.0 (full left) to +1.0 (full right)
    pub throttle: f64,            // 0.0 to 1.0
    pub brake: f64,               // 0.0 to 1.0
    pub handbrake: f64,           // 0.0 to 1.0
    pub clutch: f64,              // 0.0 (engaged) to 1.0 (disengaged)
    pub gear_request: Option<i8>, // None = no change, Some(0) = Neutral, Some(1..6) = Gear, Some(-1) = Reverse
}

impl Default for VehicleInput {
    fn default() -> Self {
        Self {
            steering: 0.0,
            throttle: 0.0,
            brake: 0.0,
            handbrake: 0.0,
            clutch: 0.0,
            gear_request: None,
        }
    }
}
