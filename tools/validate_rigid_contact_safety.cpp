#include "tire_vertical_solver.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

static int checks = 0, failures = 0;
static void Check(bool condition, const char* label)
{
    ++checks;
    if (!condition) { ++failures; std::printf("FAIL %s\n", label); }
}

static TireVerticalSolverConfiguration Configuration()
{
    TireVerticalSolverConfiguration c = {};
    c.total_mass_kilograms = 600.0;
    c.roll_inertia_kilogram_meters_squared = 300.0;
    c.pitch_inertia_kilogram_meters_squared = 800.0;
    for (int w = 0; w < 4; ++w)
    {
        c.wheel_x_meters[w] = w < 2 ? 1.5 : -1.5;
        c.wheel_y_meters[w] = w % 2 ? 0.75 : -0.75;
        c.unsprung_mass_kilograms[w] = 13.0;
        auto& k = c.corner[w];
        k.spring.spring_constant_newtons_per_meter = -100000.0;
        k.spring.reference_position_meters = 0.05;
        k.spring.minimum_position_meters = 0.01;
        k.spring.maximum_position_meters = 0.2;
        k.spring.bellcrank_ratio = 1.0;
        k.unloaded_radius_meters = 0.312;
        k.damper.efficiency = 1.0;
        k.damper.bump.transition_velocity_meters_per_second = 0.15;
        k.damper.rebound.transition_velocity_meters_per_second = 0.15;
    }
    c.chassis_contact_count = 1;
    c.chassis_contact[0].enabled = 1;
    c.chassis_contact[0].z_meters = -0.05;
    return c;
}

static TireVerticalSolverState State()
{
    TireVerticalSolverState s = {};
    s.body_position[0] = 0.05;
    for (double& extension : s.suspension_extension_meters) extension = 0.05;
    return s;
}

static TireVerticalSolverStepInput Input(double h)
{
    TireVerticalSolverStepInput in = {};
    in.elapsed_seconds = h;
    for (double& height : in.ground_height_meters) height = -1.0;
    return in;
}

int main()
{
    for (double h : {0.002, 0.001, 0.0005, 0.00003125})
    {
        auto c = Configuration(); auto a = State(); auto in = Input(h);
        a.body_position[0] -= 0.01;
        TireVerticalSolverState b = {};
        TireVerticalSolverDiagnostics d = {};
        Check(!TireVerticalSolverStep(c, a, in, b, d), "10mm overlap is rejected rather than catapulted");
        Check(d.failure_reason == TIRE_VERTICAL_FAILURE_INITIAL_GEOMETRY, "overlap is not retried with smaller dt");
        Check(std::memcmp(&a, &b, sizeof(a)) == 0, "rejected geometry preserves the input state");
        Check(d.chassis_contact_impulse_newton_seconds[0] == 0.0, "rejected geometry has no impulse");
        std::printf("overlap h=%.9g rejected=%d vz=%.9g impulse=%.9g\n", h,
            d.failed, b.body_velocity[0], d.chassis_contact_impulse_newton_seconds[0]);
    }
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
        for (int field = 0; field < 9; ++field)
        {
            auto c = Configuration(); auto a = State(); auto in = Input(0.002);
            switch (field)
            {
                case 0: a.body_position[0] = bad; break;
                case 1: a.body_velocity[1] = bad; break;
                case 2: a.suspension_extension_meters[2] = bad; break;
                case 3: a.suspension_extension_velocity_meters_per_second[3] = bad; break;
                case 4: in.ground_height_meters[0] = bad; break;
                case 5: in.chassis_ground_height_velocity_meters_per_second[0] = bad; break;
                case 6: in.elapsed_seconds = bad; break;
                case 7: c.external_body_generalized_force[0] = bad; break;
                case 8: c.corner[0].spring.spring_constant_newtons_per_meter = bad; break;
            }
            TireVerticalSolverState b = {}; TireVerticalSolverDiagnostics d = {};
            Check(!TireVerticalSolverStep(c, a, in, b, d), "NaN/Inf never converges");
            Check(d.failed && !d.converged, "nonfinite input reports failure");
        }
    {
        auto c = Configuration(); auto a = State(); auto in = Input(0.002);
        c.external_body_generalized_force[0] = 6000.0;
        TireVerticalSolverState b = {}; TireVerticalSolverDiagnostics d = {};
        Check(TireVerticalSolverStep(c, a, in, b, d), "upward force releases floor contact");
        Check(b.body_velocity[0] > 0.01, "floor cannot pull car down");
        Check(d.chassis_contact_impulse_newton_seconds[0] == 0.0, "released floor has no hidden tensile impulse");
        for (int i = 0; i < d.active_constraint_count; ++i)
            Check(d.active_constraint_kind[i] != TIRE_VERTICAL_CONSTRAINT_CHASSIS
                || d.constraint_impulse_newton_seconds[i] >= -1e-10, "all actual floor impulses have admissible sign");
    }
    // Regression: a tolerated negative initial gap used to re-add a released
    // contact forever, despite a separating solution with near-zero residual.
    // Cover both tire/floor constraints, small/large forces, world elevations
    // and the minimum interval observed in the user's freeze log.
    for (double origin : {0.0, 33.21259947})
        for (double gap : {0.0, -1e-10, -1e-8, -5e-8})
            for (double h : {0.002, 0.001, 0.0005, 0.00003125})
                for (double upward : {1.0, 6000.0})
                    for (bool tire : {false, true})
                    {
                        auto c = Configuration(); auto a = State(); auto in = Input(h);
                        a.body_position[0] += origin + gap;
                        for (double& ground : in.ground_height_meters) ground += origin;
                        in.chassis_ground_height_meters[0] = origin;
                        if (tire)
                        {
                            c.chassis_contact_count = 0;
                            in.ground_height_meters[0] = origin;
                            c.external_wheel_generalized_force[0] = -upward;
                        }
                        else c.external_body_generalized_force[0] = upward;
                        TireVerticalSolverState b = {}; TireVerticalSolverDiagnostics d = {};
                        bool ok = TireVerticalSolverStep(c, a, in, b, d) != 0;
                        Check(ok, "roundoff overlap cannot cycle when support releases");
                        Check(!d.failed && TireVerticalStateIsFinite(b), "released solution is accepted and finite");
                        const double impulse = tire ? d.tire_contact_impulse_newton_seconds[0]
                            : d.chassis_contact_impulse_newton_seconds[0];
                        Check(std::fabs(impulse) < 1e-9, "released support has no artificial impulse");
                        const double separation = tire
                            ? b.body_velocity[0] - c.wheel_x_meters[0] * b.body_velocity[2]
                                + c.wheel_y_meters[0] * b.body_velocity[1]
                                - b.suspension_extension_velocity_meters_per_second[0]
                            : b.body_velocity[0];
                        Check(separation > 0.0, "wheel or floor separates rather than freezing");
                    }
    for (double gap : {-1e-8, -5e-8})
        for (double h : {0.002, 0.00003125})
        {
            auto c = Configuration(); auto a = State(); auto in = Input(h);
            a.body_position[0] += gap;
            a.body_velocity[0] = 0.2;
            TireVerticalSolverState b = {}; TireVerticalSolverDiagnostics d = {};
            Check(TireVerticalSolverStep(c, a, in, b, d), "already separating roundoff contact is not reactivated");
            Check(std::fabs(b.body_velocity[0] - 0.2) < 1e-9,
                "separation preserves physical momentum without impulse");
            c.external_body_generalized_force[0] = -6000.0;
            a.body_velocity[0] = 0.0;
            Check(TireVerticalSolverStep(c, a, in, b, d), "closing roundoff contact still supports the car");
            Check(d.chassis_contact_impulse_newton_seconds[0] > 0.0
                && std::fabs(b.body_velocity[0]) < 1e-7, "compressive support remains effective");
        }
    for (double h : {0.002, 0.001, 0.0005, 0.00003125})
    {
        auto c = Configuration(); auto a = State(); auto in = Input(h);
        a.body_velocity[0] = -3.0;
        TireVerticalSolverState b = {}; TireVerticalSolverDiagnostics d = {};
        double e0 = TireVerticalSystemEnergyJoules(c, a, in.ground_height_meters);
        Check(TireVerticalSolverStep(c, a, in, b, d), "admissible impact converges");
        double e1 = TireVerticalSystemEnergyJoules(c, b, in.ground_height_meters);
        Check(std::isfinite(e1) && e1 <= e0 + 1e-6, "stationary floor impact is passive");
        Check(std::fabs(b.body_velocity[0]) < 1e-7, "touching impact has zero restitution");
    }
    {
        auto c = Configuration(); auto a = State(); auto in = Input(0.002);
        c.chassis_contact_count = 3;
        for (int i = 0; i < 3; ++i)
        {
            c.chassis_contact[i].enabled = 1;
            c.chassis_contact[i].z_meters = -0.05;
            c.chassis_contact[i].x_meters = i == 0 ? -1.0 : (i == 1 ? 1.0 : 0.0);
            c.chassis_contact[i].y_meters = i == 2 ? 0.5 : -0.5;
        }
        c.corner[0].spring.minimum_position_meters = 0.05;
        c.external_body_generalized_force[0] = -6000.0;
        for (double& g : in.ground_height_meters) g = 0.0;
        TireVerticalSolverState b = {}; TireVerticalSolverDiagnostics d = {};
        Check(TireVerticalSolverStep(c, a, in, b, d), "four wheels three floor points and stop converge");
        Check(std::fabs(b.body_velocity[0]) < 1e-7, "redundant contacts hold body");
        for (double gap : d.tire_gap_meters) Check(gap >= -1e-7, "all wheel gaps feasible");
        for (int i = 0; i < 3; ++i) Check(d.chassis_contact_gap_meters[i] >= -1e-7, "all floor gaps feasible");
        std::swap(c.chassis_contact[0], c.chassis_contact[2]);
        TireVerticalSolverState p = {}; TireVerticalSolverDiagnostics pd = {};
        Check(TireVerticalSolverStep(c, a, in, p, pd), "permuted contact set converges");
        Check(std::fabs(p.body_velocity[0] - b.body_velocity[0]) < 1e-8, "contact ordering preserves motion");
    }
    std::printf("checks=%d failures=%d\n", checks, failures);
    return failures ? 1 : 0;
}
