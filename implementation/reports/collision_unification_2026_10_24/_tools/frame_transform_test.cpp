/*
    frame_transform_test.cpp -- Standalone verification of `frame_transform.h`.

    The Speed Dreams adapter cannot be compiled without the full game toolchain in this
    environment, but the geometry it injects into the Rust state can be verified on its
    own. This harness compiles the header with MSVC (no Speed Dreams, no plib) and
    checks the properties the adapter relies on:

      1. Rust <-> Speed Dreams mapping is an involution.
      2. Euler -> basis -> Euler is the identity for roll/pitch/yaw combinations.
      3. The basis is orthonormal and right-handed (x cross y = -z, i.e. a right-handed
         trihedron in the Rust convention).
      4. `com_from_origin` / `origin_from_com` are exact inverses, including pitch and
         roll, which the retired `setCar` dropped entirely.
      5. A body origin is NOT its centre of gravity: the displacement equals
         `basis * com_local`, so using `DynGCg.pos` as the origin is measurably wrong.

    Build (the exact command is recorded in the delivery report):
      cl /std:c++17 /W4 /EHsc /I src\modules\simu\rustphysics ^
         implementation\reports\collision_unification_2026_10_24\_tools\frame_transform_test.cpp
*/

#include "frame_transform.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

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

    bool near(double a, double b, double tolerance = 1e-12)
    {
        return std::fabs(a - b) <= tolerance;
    }

    bool near_vec(const ofp::frame::Vec3& a, const ofp::frame::Vec3& b,
                  double tolerance = 1e-12)
    {
        return near(a.x, b.x, tolerance) && near(a.y, b.y, tolerance)
            && near(a.z, b.z, tolerance);
    }

    void test_frame_involution()
    {
        const double samples[][3] = {
            { 0.0, 0.0, 0.0 },
            { 12.5, -3.25, 91.0 },
            { -1000.0, 250.0, 7.0 },
            { 1e-9, -1e-9, 1e-9 },
        };
        for (const auto& s : samples)
        {
            const ofp::frame::Vec3 sd = ofp::frame::make(s[0], s[1], s[2]);
            const ofp::frame::Vec3 rust = ofp::frame::sd_to_rust(sd);
            const ofp::frame::Vec3 back = ofp::frame::rust_to_sd(rust);
            check(near_vec(sd, back, 1e-12), "sd -> rust -> sd must be the identity");
            // The documented mapping, written out independently of the implementation.
            check(near(rust.x, -sd.y) && near(rust.y, sd.z) && near(rust.z, -sd.x),
                  "rust = (-y, z, -x)");
        }
    }

    void test_euler_basis_round_trip()
    {
        // Roll and yaw cover the full circle; pitch is recovered on (-pi/2, pi/2), which
        // is the only interval `asin` can invert.
        const double roll_angles[] = { 0.0, 0.05, -0.31, 1.2, -2.7, 3.0 };
        const double pitch_angles[] = { 0.0, 0.05, -0.31, 0.6, -0.9 };
        const double yaw_angles[] = { 0.0, 0.05, -0.31, 1.2, -2.7, 3.0 };
        for (double roll : roll_angles)
        {
            for (double pitch : pitch_angles)
            {
                for (double yaw : yaw_angles)
                {
                    const ofp::frame::Basis basis =
                        ofp::frame::basis_from_sd_euler(roll, pitch, yaw);
                    check(ofp::frame::orthonormality_error(basis) < 1e-12,
                          "published basis must be orthonormal", roll + pitch + yaw);
                    // Right-handed in the Rust convention (X right, Y up, Z back):
                    // right x up = back.
                    const ofp::frame::Vec3 cross_xy =
                        ofp::frame::cross(basis.x, basis.y);
                    check(near_vec(cross_xy, basis.z, 1e-12), "x cross y must equal z");
                    double roll_back = 0.0, pitch_back = 0.0, yaw_back = 0.0;
                    ofp::frame::sd_euler_from_basis(basis, roll_back, pitch_back, yaw_back);
                    // Pitch is recovered on [-pi/2, pi/2]; the samples stay inside it.
                    check(near(roll_back, roll, 1e-9), "roll must round-trip", roll_back);
                    check(near(pitch_back, pitch, 1e-9), "pitch must round-trip", pitch_back);
                    check(near(std::remainder(yaw_back - yaw, 2.0 * 3.14159265358979323846),
                               0.0, 1e-9),
                          "yaw must round-trip modulo 2pi", yaw_back - yaw);
                }
            }
        }
    }

    void test_com_inversion()
    {
        const ofp::frame::Vec3 com_local = ofp::frame::make(0.012, 0.226, -0.041);
        const ofp::frame::Vec3 origin = ofp::frame::make(123.5, 4.25, -987.75);
        const double cases[][3] = {
            { 0.0, 0.0, 0.0 },
            { 0.0, 0.0, 0.7 },
            { 0.03, -0.02, 1.9 },
            { -0.08, 0.04, -2.4 },
        };
        for (const auto& c : cases)
        {
            const ofp::frame::Basis basis = ofp::frame::basis_from_sd_euler(c[0], c[1], c[2]);
            check(ofp::frame::com_round_trip_error(origin, basis, com_local) < 1e-9,
                  "origin -> com -> origin must be exact");
            const ofp::frame::Vec3 com =
                ofp::frame::com_from_origin(origin, basis, com_local);
            const ofp::frame::Vec3 origin_back =
                ofp::frame::origin_from_com(com, basis, com_local);
            check(near_vec(origin_back, origin, 1e-9), "com inversion");

            // The retired behaviour: forwarding the centre of gravity as the body
            // origin. The resulting error is exactly the centre-of-mass offset, which is
            // what made `setCar(publishCar(state))` non-invertible.
            const double forwarded_error =
                ofp::frame::length(ofp::frame::sub(com, origin));
            check(forwarded_error > 0.2 && forwarded_error < 0.3,
                  "the centre-of-mass offset must be a measurable displacement",
                  forwarded_error);
            check(!near_vec(com, origin, 1e-6),
                  "a centre of gravity must never be forwarded as a body origin");
        }
    }

    void test_pitch_and_roll_are_not_ignored()
    {
        // A yaw-only conversion of a rolled/pitched body would report the same origin.
        const ofp::frame::Vec3 com_local = ofp::frame::make(0.012, 0.226, -0.041);
        const ofp::frame::Vec3 origin = ofp::frame::make(0.0, 0.0, 0.0);
        const ofp::frame::Basis level = ofp::frame::basis_from_sd_euler(0.0, 0.0, 0.4);
        const ofp::frame::Basis tilted = ofp::frame::basis_from_sd_euler(0.06, -0.05, 0.4);
        const ofp::frame::Vec3 level_origin =
            ofp::frame::origin_from_com(ofp::frame::com_from_origin(origin, level, com_local),
                                        level, com_local);
        const ofp::frame::Vec3 tilted_origin =
            ofp::frame::origin_from_com(ofp::frame::com_from_origin(origin, tilted, com_local),
                                        tilted, com_local);
        check(near_vec(level_origin, origin, 1e-9), "level round-trip");
        check(near_vec(tilted_origin, origin, 1e-9), "tilted round-trip");
        // The two bases differ, so a conversion that ignored roll/pitch would move the
        // origin by the difference of the two offsets.
        const ofp::frame::Vec3 level_com =
            ofp::frame::com_from_origin(origin, level, com_local);
        const ofp::frame::Vec3 tilted_com =
            ofp::frame::com_from_origin(origin, tilted, com_local);
        const double difference = ofp::frame::length(ofp::frame::sub(level_com, tilted_com));
        check(difference > 1e-3, "roll and pitch must change the centre of gravity",
              difference);
    }
}

int main()
{
    test_frame_involution();
    test_euler_basis_round_trip();
    test_com_inversion();
    test_pitch_and_roll_are_not_ignored();
    if (g_failures == 0)
    {
        std::printf("PASS frame_transform: 4 groups, 0 failures\n");
        return EXIT_SUCCESS;
    }
    std::printf("FAIL frame_transform: %d failure(s)\n", g_failures);
    return EXIT_FAILURE;
}
