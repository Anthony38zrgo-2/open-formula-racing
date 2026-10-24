/*
    adapter_geometry_check.cpp -- Syntax and arithmetic verification of the expressions
    the Rust physics adapter now uses for pose and centre-of-mass conversion.

    The Speed Dreams module itself cannot be compiled here (it needs plib, SDL, OSG and
    the whole game build), so this harness reproduces the *exact* expressions of
    `rustphysics.cpp` (`publishCar`, `setCar`, `createCar`) against stub types of the
    same shape. It catches the mechanical mistakes that a blind edit can introduce:

      * a missing or misspelled helper (`sdToRustOf`, `toFrame`, `toOfp`),
      * wrong brace initialisation of `ofp::frame::Basis`,
      * a `publishCar` / `setCar` pair that is not the exact inverse,
      * a `createCar` spawn offset that still drops the sine terms of the yaw.

    Build (exact command in the delivery report):
      cl /std:c++17 /W4 /EHsc /I src\modules\simu\rustphysics ^
         implementation\reports\collision_unification_2026_10_24\_tools\adapter_geometry_check.cpp
*/

#include "frame_transform.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Stubs with the same shape as the ABI structures used by the adapter
// ---------------------------------------------------------------------------

struct OfpVec3
{
    double x;
    double y;
    double z;
};

struct OfpSnapshotStub
{
    OfpVec3 position;
    OfpVec3 basis_x;
    OfpVec3 basis_y;
    OfpVec3 basis_z;
    double linear_velocity[3];
};

namespace
{
    int g_failures = 0;

    void check(bool condition, const char* what, double detail = 0.0)
    {
        if (!condition)
        {
            std::printf("FAIL %s (%.12g)\n", what, detail);
            ++g_failures;
        }
    }

    // --- exact copies of the helpers added to rustphysics.cpp ---------------

    inline ofp::frame::Vec3 toFrame(const OfpVec3& in)
    {
        return ofp::frame::make(in.x, in.y, in.z);
    }

    inline OfpVec3 toOfp(const ofp::frame::Vec3& in)
    {
        OfpVec3 out;
        out.x = in.x;
        out.y = in.y;
        out.z = in.z;
        return out;
    }

    inline ofp::frame::Vec3 sdToRustOf(double x, double y, double z)
    {
        return ofp::frame::sd_to_rust(ofp::frame::make(x, y, z));
    }

    inline void sdToRust(double x, double y, double z, OfpVec3& out)
    {
        const ofp::frame::Vec3 mapped = ofp::frame::sd_to_rust(ofp::frame::make(x, y, z));
        out.x = mapped.x;
        out.y = mapped.y;
        out.z = mapped.z;
    }

    inline void rustToSd(const OfpVec3& in, double& x, double& y, double& z)
    {
        const ofp::frame::Vec3 mapped = ofp::frame::rust_to_sd(toFrame(in));
        x = mapped.x;
        y = mapped.y;
        z = mapped.z;
    }

    inline void mapRustDirectionToSd(const OfpVec3& in, double out[3])
    {
        const ofp::frame::Vec3 mapped = ofp::frame::rust_to_sd(toFrame(in));
        out[0] = mapped.x;
        out[1] = mapped.y;
        out[2] = mapped.z;
    }

    // --- the three conversions, copied from the adapter --------------------

    /// `publishCar`: body origin + basis * com -> Speed Dreams CG.
    void publishCom(const OfpSnapshotStub& s, const OfpVec3& com_local,
                    double& com_x, double& com_y, double& com_z)
    {
        const ofp::frame::Basis body_basis = {
            toFrame(s.basis_x), toFrame(s.basis_y), toFrame(s.basis_z)
        };
        const ofp::frame::Vec3 com_rust =
            ofp::frame::com_from_origin(toFrame(s.position), body_basis, toFrame(com_local));
        rustToSd(toOfp(com_rust), com_x, com_y, com_z);
    }

    /// `setCar`: Speed Dreams CG + attitude -> Rust body origin and basis.
    void setCarPose(double com_x, double com_y, double com_z,
                    double roll, double pitch, double yaw,
                    const OfpVec3& com_local,
                    OfpVec3& origin, OfpVec3& bx, OfpVec3& by, OfpVec3& bz)
    {
        const ofp::frame::Basis basis = ofp::frame::basis_from_sd_euler(roll, pitch, yaw);
        const ofp::frame::Vec3 com_rust = sdToRustOf(com_x, com_y, com_z);
        const ofp::frame::Vec3 body_origin =
            ofp::frame::origin_from_com(com_rust, basis, toFrame(com_local));
        origin = toOfp(body_origin);
        bx = toOfp(basis.x);
        by = toOfp(basis.y);
        bz = toOfp(basis.z);
    }

    /// `createCar`: spawn at a Speed Dreams CG with a yaw-only attitude.
    OfpVec3 createOrigin(double com_x, double com_y, double com_z, double yaw,
                         const OfpVec3& com_local)
    {
        const ofp::frame::Basis spawn_basis = ofp::frame::basis_from_sd_euler(0.0, 0.0, yaw);
        const ofp::frame::Vec3 com_rust = sdToRustOf(com_x, com_y, com_z);
        const ofp::frame::Vec3 body_origin =
            ofp::frame::origin_from_com(com_rust, spawn_basis, toFrame(com_local));
        return toOfp(body_origin);
    }

    // --- tests -------------------------------------------------------------

    void test_publish_setcar_round_trip()
    {
        const OfpVec3 com_local = { 0.012, 0.226, -0.041 };
        const double cases[][3] = {
            { 0.0, 0.0, 0.0 },
            { 0.0, 0.0, 0.9 },
            { 0.04, -0.03, -2.1 },
            { -0.07, 0.05, 1.4 },
        };
        for (const auto& c : cases)
        {
            OfpSnapshotStub snapshot = {};
            snapshot.position = { 421.5, 2.75, -1188.25 };
            const ofp::frame::Basis basis = ofp::frame::basis_from_sd_euler(c[0], c[1], c[2]);
            snapshot.basis_x = toOfp(basis.x);
            snapshot.basis_y = toOfp(basis.y);
            snapshot.basis_z = toOfp(basis.z);

            double com_x = 0.0, com_y = 0.0, com_z = 0.0;
            publishCom(snapshot, com_local, com_x, com_y, com_z);

            OfpVec3 origin = {}, bx = {}, by = {}, bz = {};
            setCarPose(com_x, com_y, com_z, c[0], c[1], c[2], com_local, origin, bx, by, bz);

            const ofp::frame::Vec3 recovered = toFrame(origin);
            const ofp::frame::Vec3 expected = toFrame(snapshot.position);
            check(ofp::frame::length(ofp::frame::sub(recovered, expected)) < 1e-9,
                  "setCar(publishCar(state)) must preserve the body origin",
                  ofp::frame::length(ofp::frame::sub(recovered, expected)));
            // The published basis must survive the round trip too, including roll/pitch.
            check(ofp::frame::length(ofp::frame::sub(toFrame(bx), basis.x)) < 1e-12,
                  "basis x round trip");
            check(ofp::frame::length(ofp::frame::sub(toFrame(by), basis.y)) < 1e-12,
                  "basis y round trip");
            check(ofp::frame::length(ofp::frame::sub(toFrame(bz), basis.z)) < 1e-12,
                  "basis z round trip");
        }
    }

    void test_create_offset_accounts_for_yaw()
    {
        const OfpVec3 com_local = { 0.012, 0.226, -0.041 };
        // Yaw-only spawn: the horizontal offset must rotate with the heading.
        const double yaw = 0.7;
        const OfpVec3 origin = createOrigin(10.0, 1.0, 20.0, yaw, com_local);
        const ofp::frame::Vec3 rust_origin = toFrame(origin);
        const ofp::frame::Vec3 rust_com = sdToRustOf(10.0, 1.0, 20.0);
        const ofp::frame::Vec3 offset =
            ofp::frame::sub(rust_com, rust_origin);
        const ofp::frame::Vec3 expected = ofp::frame::transform_vector(
            ofp::frame::basis_from_sd_euler(0.0, 0.0, yaw), toFrame(com_local));
        check(ofp::frame::length(ofp::frame::sub(offset, expected)) < 1e-12,
              "spawn offset must be basis * com_local");

        // The retired implementation rotated only the cosine terms:
        // origin = com - (cg.x*cos, cg.y, cg.z*cos).
        const double cos_yaw = std::cos(yaw);
        const ofp::frame::Vec3 retired = ofp::frame::sub(
            rust_com,
            ofp::frame::make(com_local.x * cos_yaw, com_local.y, com_local.z * cos_yaw));
        const double retired_error = ofp::frame::length(ofp::frame::sub(retired, rust_origin));
        check(retired_error > 1e-3,
              "the previous spawn offset must differ measurably from the correct one",
              retired_error);
    }

    void test_direction_mapping_is_consistent_with_positions()
    {
        const OfpVec3 direction = { 0.0, 0.0, -1.0 }; // Rust forward
        double sd[3] = { 0.0, 0.0, 0.0 };
        mapRustDirectionToSd(direction, sd);
        check(std::fabs(sd[0] - 1.0) < 1e-12 && std::fabs(sd[1]) < 1e-12
                  && std::fabs(sd[2]) < 1e-12,
              "Rust forward must map to Speed Dreams forward (X)");
        // A position and a direction expressed in the same Rust vector must map to the
        // same Speed Dreams components.
        double px = 0.0, py = 0.0, pz = 0.0;
        rustToSd(direction, px, py, pz);
        check(px == sd[0] && py == sd[1] && pz == sd[2],
              "positions and directions must use the same rigid mapping");
    }
}

int main()
{
    test_publish_setcar_round_trip();
    test_create_offset_accounts_for_yaw();
    test_direction_mapping_is_consistent_with_positions();
    if (g_failures == 0)
    {
        std::printf("PASS adapter_geometry: 3 groups, 0 failures\n");
        return EXIT_SUCCESS;
    }
    std::printf("FAIL adapter_geometry: %d failure(s)\n", g_failures);
    return EXIT_FAILURE;
}
