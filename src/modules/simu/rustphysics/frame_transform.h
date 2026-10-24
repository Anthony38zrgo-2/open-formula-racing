/*
    frame_transform.h -- Pure frame, pose and centre-of-mass conversions shared by the
    Rust physics adapter.

    This header deliberately depends on nothing but <cmath>: it is the single definition
    of the geometry that crosses the Speed Dreams <-> Rust boundary, and it is compiled
    and exercised on its own by `implementation/reports/collision_unification_2026_10_24/_tools/frame_transform_test.cpp`
    (MSVC, no Speed Dreams headers), so the conversions can be verified without building
    the whole game.

    Conventions
    -----------
    Speed Dreams world:  X forward, Y left, Z up.  Yaw/pitch/roll are the angles
                         `sgMakeCoordMat4` expects and `tDynPt::pos` stores (`az` = yaw
                         about Z, `ay` = pitch about Y, `ax` = roll about X).
    Rust world:          X right, Y up, -Z forward.

    Body frame:          both sides use (X right, Y up, Z back) for the *body* axes, so
                         `center_of_mass_local` is expressed in the same trihedron as the
                         published `basis_x/basis_y/basis_z` columns.

    Pose contract
    -------------
    `OpenFormulaSimulation` owns the body **origin** (the point the published basis is
    attached to). Speed Dreams publishes `pub.DynGCg.pos` as the **centre of gravity**
    and `pub.posMat` for the model origin. The two differ by the body-local centre of
    mass:

        com_world    = origin + basis * center_of_mass_local
        origin       = com_world - basis * center_of_mass_local

    Both directions are used: `publishCar` (origin -> com) and `setCar` (com -> origin).
    Using `DynGCg.pos` as the origin, as the previous adapter did, displaced the vehicle
    by the whole centre-of-mass offset on every network/pose injection.
*/

#ifndef OPEN_FORMULA_FRAME_TRANSFORM_H
#define OPEN_FORMULA_FRAME_TRANSFORM_H

#include <cmath>

namespace ofp
{
namespace frame
{
    struct Vec3
    {
        double x;
        double y;
        double z;
    };

    inline Vec3 make(double x, double y, double z)
    {
        Vec3 v = { x, y, z };
        return v;
    }

    inline Vec3 add(const Vec3& a, const Vec3& b) { return make(a.x + b.x, a.y + b.y, a.z + b.z); }
    inline Vec3 sub(const Vec3& a, const Vec3& b) { return make(a.x - b.x, a.y - b.y, a.z - b.z); }
    inline Vec3 scale(const Vec3& v, double s) { return make(v.x * s, v.y * s, v.z * s); }
    inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    inline Vec3 cross(const Vec3& a, const Vec3& b)
    {
        return make(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    inline double length(const Vec3& v) { return std::sqrt(dot(v, v)); }

    inline Vec3 normalized(const Vec3& v)
    {
        const double l = length(v);
        return l > 1e-12 ? scale(v, 1.0 / l) : make(0.0, 0.0, 0.0);
    }

    /// Three body axes expressed in the Rust world frame. Column `x` is the body right
    /// axis, `y` the up axis and `z` the back axis, exactly as `OfpSnapshot` publishes
    /// `basis_x`, `basis_y`, `basis_z`.
    struct Basis
    {
        Vec3 x;
        Vec3 y;
        Vec3 z;
    };

    inline Vec3 transform_vector(const Basis& basis, const Vec3& local)
    {
        return add(add(scale(basis.x, local.x), scale(basis.y, local.y)), scale(basis.z, local.z));
    }

    inline Vec3 inverse_transform_vector(const Basis& basis, const Vec3& world)
    {
        return make(dot(basis.x, world), dot(basis.y, world), dot(basis.z, world));
    }

    // -----------------------------------------------------------------------
    // World mapping
    // -----------------------------------------------------------------------

    /// Speed Dreams (X forward, Y left, Z up) -> Rust (X right, Y up, -Z forward).
    inline Vec3 sd_to_rust(double x, double y, double z) { return make(-y, z, -x); }
    inline Vec3 sd_to_rust(const Vec3& v) { return sd_to_rust(v.x, v.y, v.z); }

    /// Rust -> Speed Dreams. Exact inverse of `sd_to_rust`.
    inline Vec3 rust_to_sd(const Vec3& v) { return make(-v.z, -v.x, v.y); }

    // -----------------------------------------------------------------------
    // Attitude
    // -----------------------------------------------------------------------

    /// Body axes from the Speed Dreams Euler angles, as `sgMakeCoordMat4` composes them
    /// (Rz * Ry * Rx): roll about X, then pitch about Y, then yaw about Z. The columns
    /// of that product are the three local axes expressed in the world frame:
    ///
    /// forward = ( cos p cos y,  cos p sin y, -sin p)
    /// left    = ( cos y sin p sin r - sin y cos r,
    ///             sin y sin p sin r + cos y cos r,
    ///             cos p sin r)
    /// up      = ( cos y sin p cos r + sin y sin r,
    ///             sin y sin p cos r - cos y sin r,
    ///             cos p cos r)
    ///
    /// With zero roll and pitch this reduces to forward = (cos y, sin y, 0),
    /// left = (-sin y, cos y, 0), up = (0, 0, 1).
    inline void sd_axes(double roll, double pitch, double yaw,
                        Vec3& forward, Vec3& left, Vec3& up)
    {
        const double cr = std::cos(roll), sr = std::sin(roll);
        const double cp = std::cos(pitch), sp = std::sin(pitch);
        const double cy = std::cos(yaw), sy = std::sin(yaw);
        forward = make(cp * cy, cp * sy, -sp);
        left = make(cy * sp * sr - sy * cr, sy * sp * sr + cy * cr, cp * sr);
        up = make(cy * sp * cr + sy * sr, sy * sp * cr - cy * sr, cp * cr);
    }

    /// Rust body axes from the Speed Dreams Euler angles. The mapping is the same rigid
    /// transform applied to every axis, never a separate hand-derived formula.
    inline Basis basis_from_sd_euler(double roll, double pitch, double yaw)
    {
        Vec3 forward, left, up;
        sd_axes(roll, pitch, yaw, forward, left, up);
        // Rust basis: right = -left, up = up, back = -forward.
        Basis basis;
        basis.x = sd_to_rust(scale(left, -1.0));
        basis.y = sd_to_rust(up);
        basis.z = sd_to_rust(scale(forward, -1.0));
        return basis;
    }

    /// Speed Dreams Euler angles from the published Rust axes: the inverse of
    /// `basis_from_sd_euler`, using the same atan2/asin convention as the adapter's
    /// `publishCar` (`yaw = atan2(forward.y, forward.x)`, `pitch = -asin(forward.z)`,
    /// `roll = atan2(left.z, up.z)`).
    inline void sd_euler_from_basis(const Basis& rust_basis,
                                    double& roll, double& pitch, double& yaw)
    {
        const Vec3 forward = rust_to_sd(scale(rust_basis.z, -1.0));
        const Vec3 left = rust_to_sd(scale(rust_basis.x, -1.0));
        const Vec3 up = rust_to_sd(rust_basis.y);
        yaw = std::atan2(forward.y, forward.x);
        const double forward_z = forward.z < -1.0 ? -1.0 : (forward.z > 1.0 ? 1.0 : forward.z);
        pitch = -std::asin(forward_z);
        roll = std::atan2(left.z, up.z);
    }

    // -----------------------------------------------------------------------
    // Centre of mass and body origin
    // -----------------------------------------------------------------------

    /// World centre of gravity from the body origin and the body-local centre of mass.
    inline Vec3 com_from_origin(const Vec3& origin, const Basis& basis, const Vec3& com_local)
    {
        return add(origin, transform_vector(basis, com_local));
    }

    /// Body origin from the world centre of gravity: the inverse of `com_from_origin`.
    ///
    /// `setCar` must use this. The previous implementation forwarded `DynGCg.pos` (a
    /// centre of gravity) as the body origin, which displaced the car by
    /// `|com_local|` (~0.3 m) on every injection and made `setCar(publishCar(state))`
    /// non-invertible.
    inline Vec3 origin_from_com(const Vec3& com, const Basis& basis, const Vec3& com_local)
    {
        return sub(com, transform_vector(basis, com_local));
    }

    /// Round-trip error of the two conversions, for the self-check in the test harness
    /// and for a cheap runtime assertion in debug builds.
    inline double com_round_trip_error(const Vec3& origin, const Basis& basis,
                                       const Vec3& com_local)
    {
        const Vec3 com = com_from_origin(origin, basis, com_local);
        const Vec3 back = origin_from_com(com, basis, com_local);
        return length(sub(back, origin));
    }

    /// Largest absolute eigenvalue-free orthonormality error of a published basis.
    inline double orthonormality_error(const Basis& basis)
    {
        const double ex = std::fabs(dot(basis.x, basis.x) - 1.0);
        const double ey = std::fabs(dot(basis.y, basis.y) - 1.0);
        const double ez = std::fabs(dot(basis.z, basis.z) - 1.0);
        const double exy = std::fabs(dot(basis.x, basis.y));
        const double exz = std::fabs(dot(basis.x, basis.z));
        const double eyz = std::fabs(dot(basis.y, basis.z));
        double worst = ex > ey ? ex : ey;
        if (ez > worst) worst = ez;
        if (exy > worst) worst = exy;
        if (exz > worst) worst = exz;
        if (eyz > worst) worst = eyz;
        return worst;
    }
}
}

#endif // OPEN_FORMULA_FRAME_TRANSFORM_H
