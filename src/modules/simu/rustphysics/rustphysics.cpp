/*
    rustphysics.cpp -- Open Formula physics module backed by the Rust library.
*/

#include "rustphysics.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <map>
#include <plib/sg.h>

#include <car.h>
#include <portability.h>
#include <raceman.h>
#include <robottools.h>
#include <formula_gear_controls.h>
#include <tgf.h>
#include <track.h>

#include "rust_library.h"
#include "frame_transform.h"
#include "../../interfaces/ofp_audio_presentation.h"

namespace
{
    const double PI = 3.14159265358979323846;

    //! Speed Dreams publishes wheels in FR, FL, RR, RL order; the Rust physics keeps
    //! FL, FR, RL, RR. These two tables are the single mapping between both orders:
    //! `SD_TO_RUST_WHEEL[sd]` yields the Rust index of a Speed Dreams wheel and
    //! `RUST_TO_SD_WHEEL[rust]` the Speed Dreams index of a Rust wheel.
    const int SD_TO_RUST_WHEEL[4] = { 1, 0, 3, 2 };
    const int RUST_TO_SD_WHEEL[4] = { 1, 0, 3, 2 };

    inline double clampValue(double value, double lower, double upper)
    {
        return value < lower ? lower : (value > upper ? upper : value);
    }

    // Open Formula / Speed Dreams (X forward, Y left, Z up) -> Rust (X right,
    // Y up, -Z forward). One definition for the whole adapter: `frame_transform.h`,
    // which is verified on its own by the MSVC harness in
    // implementation/reports/collision_unification_2026_10_24/_tools.
    inline void sdToRust(double x, double y, double z, OfpVec3& out)
    {
        const ofp::frame::Vec3 mapped = ofp::frame::sd_to_rust(ofp::frame::make(x, y, z));
        out.x = mapped.x;
        out.y = mapped.y;
        out.z = mapped.z;
    }

    inline ofp::frame::Vec3 toFrame(const OfpVec3& in)
    {
        return ofp::frame::make(in.x, in.y, in.z);
    }

    inline ofp::frame::Vec3 sdToRustOf(double x, double y, double z)
    {
        return ofp::frame::sd_to_rust(ofp::frame::make(x, y, z));
    }

    inline OfpVec3 toOfp(const ofp::frame::Vec3& in)
    {
        OfpVec3 out;
        out.x = in.x;
        out.y = in.y;
        out.z = in.z;
        return out;
    }

    inline void rustToSd(const OfpVec3& in, double& x, double& y, double& z)
    {
        const ofp::frame::Vec3 mapped = ofp::frame::rust_to_sd(toFrame(in));
        x = mapped.x;
        y = mapped.y;
        z = mapped.z;
    }

    inline double dot3(const double a[3], const double b[3])
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    //! Maps a Rust world vector (X right, Y up, -Z forward) to the Speed Dreams world
    //! frame (X forward, Y left, Z up). The same rigid mapping used for positions must
    //! also be applied to directions, otherwise the published orientation and the local
    //! velocities mix both frames.
    inline void mapRustDirectionToSd(const OfpVec3& in, double out[3])
    {
        const ofp::frame::Vec3 mapped = ofp::frame::rust_to_sd(toFrame(in));
        out[0] = mapped.x;
        out[1] = mapped.y;
        out[2] = mapped.z;
    }

    void basisFromSnapshot(const OfpSnapshot& snapshot,
                           double forward[3], double left[3], double up[3])
    {
        // Local axes are expressed in the Rust world frame; map each of them before
        // computing the Speed Dreams Euler angles.
        const OfpVec3 forward_rust{ -snapshot.basis_z.x, -snapshot.basis_z.y, -snapshot.basis_z.z };
        const OfpVec3 left_rust{ -snapshot.basis_x.x, -snapshot.basis_x.y, -snapshot.basis_x.z };
        const OfpVec3 up_rust = snapshot.basis_y;
        mapRustDirectionToSd(forward_rust, forward);
        mapRustDirectionToSd(left_rust, left);
        mapRustDirectionToSd(up_rust, up);
    }

    //! Euler angles expected by sgMakeCoordMat4 (Rz * Ry * Rx order).
    void eulerFromBasis(const double forward[3], const double left[3], const double up[3],
                        double& yaw, double& pitch, double& roll)
    {
        yaw = std::atan2(forward[1], forward[0]);
        pitch = -std::asin(clampValue(forward[2], -1.0, 1.0));
        roll = std::atan2(left[2], up[2]);
    }

    struct RustCar
    {
        OfpHandle handle = nullptr;
        tCarElt* car = nullptr;
        int index = -1;
        OfpSnapshot snapshot{};
        OfpVehicleSpecs specs{};
        OfpVec3 center_of_mass_local{};
        OfpVec3 inertia{};
        double spawn_height = 0.5;
        double mesh_origin_height = 0.0;
        double mass = 600.0;
        bool ready = false;
        bool failed = false;
        double accumulated_damage = 0.0;
        int last_barrier_contacts = 0;
        std::map<std::string, double> setup_overrides;
        std::string last_error;
        double forward_speed = 0.0;
        double lateral_speed = 0.0;
        double wheel_rotation[4]{};
        //! Lock-to-lock steering-wheel rotation of the visual car, in radians.
        double steering_wheel_lock_rad = 0.0;
        //! Next simulation time at which this car emits its summary block.
        double next_diagnostic_s = 0.0;
        //! Rejection counter already reported, so the failure block is logged once.
        std::uint32_t reported_rejections = 0;
        //! True from the first rejection until the session resets: the published
        //! state no longer advances, and the log must say so instead of repeating
        //! the same frozen timestamp as if it were live telemetry.
        bool frozen = false;
        int consecutive_failures = 0;
        //! Etapa 3/4 audio presentation: one raw step per accepted physics step,
        //! written into the `tCarElt` ring that `snddefault` drains (see
        //! `ofp_audio_presentation.h`). Session increments on car reset so
        //! stale packets can never resurrect a cut window.
        std::uint32_t audio_session = 1;
    };

    //! Builds one raw audio step from the just-accepted snapshot + specs.
    //! Pure mapping: SI units preserved (m/s), no acoustic decision, no
    //! pedal-for-load substitution, no gear-number transmission sign.
    inline OfpAudioStepV1 BuildAudioStep(const OfpSnapshot& s, const OfpVehicleSpecs& specs,
                                         int car_index, std::uint32_t session)
    {
        OfpAudioStepV1 step{};
        step.schema_version = OFP_AUDIO_STEP_VERSION;
        step.struct_size = (std::uint32_t)sizeof(OfpAudioStepV1);
        step.car_index = (std::int32_t)car_index;
        step.session_generation = session;
        step.sequence = s.sequence;
        step.sim_time_s = s.sim_time_s;
        step.dt_s = (float)s.step_dt_s;
        step.rpm = s.engine_rpm;
        step.throttle = (float)s.throttle;
        step.speed_m_s = std::sqrt(s.linear_velocity.x * s.linear_velocity.x
                                 + s.linear_velocity.y * s.linear_velocity.y
                                 + s.linear_velocity.z * s.linear_velocity.z);
        step.gear = s.gear;
        step.target_gear = s.target_gear;
        step.engine_torque_nm = s.engine_torque_nm;
        step.clutch_torque_nm = s.clutch_torque_nm;
        step.max_torque_nm = specs.max_torque_nm;
        step.clutch_engagement = s.clutch_engagement;
        step.tc_cut_ratio_smoothed = s.tc_cut_ratio_smoothed;
        step.is_rev_limited = s.is_rev_limited;
        step.shift_timer_s = s.shift_timer_s;
        for (int i = 0; i < OFP_AUDIO_STEP_WHEELS; ++i)
        {
            step.wheel_spin_rad_s[i] = s.wheels[i].spin_rad_s;
            step.slip_ratio[i] = (float)s.wheels[i].slip_ratio;
            step.slip_angle_rad[i] = (float)s.wheels[i].slip_angle_rad;
            step.contact_fraction[i] = (float)s.wheels[i].contact_fraction;
            step.normal_force_n[i] = (float)s.wheels[i].normal_force_n;
            step.surface[i] = s.wheels[i].surface;
        }
        step.final_drive = specs.final_drive;
        step.front_torque_split = specs.front_torque_split;
        step.floor_valid_mask = s.floor_valid_mask;
        step.floor_impulse_ns[0] = s.floor_impulse_ns[0];
        step.floor_impulse_ns[1] = s.floor_impulse_ns[1];
        step.body_contact_count = s.body_contact_count;
        for (int i = 0; i < OFP_AUDIO_STEP_MAX_CONTACTS; ++i)
            step.body_contact_impulse_ns[i] = s.body_contact_impulse_ns[i];
        step.scrape_speed_m_s = (float)std::sqrt(s.linear_velocity.x * s.linear_velocity.x
                                               + s.linear_velocity.z * s.linear_velocity.z);
        return step;
    }

    //! Publishes the raw audio step for the accepted physics step into the
    //! `tCarElt` ring. Controlled discard on overflow (the consumer counts it
    //! from the write counter); never fabricates a step.
    inline void publishAudioStep(RustCar& car)
    {
        if (!car.ready || car.failed || !car.car)
            return;
        tCarElt* elt = car.car;
        const OfpAudioStepV1 step = BuildAudioStep(car.snapshot, car.specs, car.index,
                                                   car.audio_session);
        elt->audio_steps[elt->audio_step_write % OFP_AUDIO_STEP_DEPTH] = step;
        elt->audio_step_write += 1;
        elt->audio_session = car.audio_session;
    }

    //! New generation on reset: stale packets can never resurrect a cut window.
    inline void resetAudioSession(RustCar& car)
    {
        car.audio_session += 1;
        if (car.audio_session == 0)
            car.audio_session = 1;
        if (car.car)
        {
            car.car->audio_step_write = 0;
            car.car->audio_session = car.audio_session;
        }
    }

    void applyIntegrationScenario(RustCar& car, double delta_time);
    void publishFormulaHud(RustCar& car);
    void logDiagnostics(RustCar& car);
    void logCarFailure(RustCar& car);
    void dumpTrace(RustCar& car, const char* reason);
    void logEffectiveSetup(RustCar& car, const char* reason);

    std::vector<RustCar> g_cars;
    tTrack* g_track = nullptr;
    std::string g_profile_path;
    std::string g_library_path;
    std::string g_ground_mesh_path;
    bool g_track_ready = false;

    // Wheel order of every diagnostic line, matching the Rust ABI (FL, FR, RL, RR).
    const char* const kWheelNames[OFP_WHEELS] = { "FL", "FR", "RL", "RR" };

    // Instrumentation / integration scenario (single session, no separate test suite).
    bool g_diagnostics = false;
    bool g_scenario = false;
    //! Summary cadence. 50-100 Hz is the range that still resolves a brief loss of
    //! contact; the old 0.25 s hid entire recoveries.
    double g_diagnostic_interval = 0.02;
    double g_elapsed = 0.0;
    int g_scenario_flags = 0;
    //! Substep/iteration ring in the Rust library (ABI v7).
    bool g_diagnostic_trace = false;
    std::string g_trace_directory;

    //! True when any structured diagnostic output is enabled. Used by the setup report
    //! so the verbose block is never written in a plain race.
    bool garageDiagnosticsEnabled() { return g_diagnostics; }

    //! Where the session ground geometry comes from, and how to identify it.
    struct TrackWorldSource
    {
        std::string path;
        std::string circuit_id;
        unsigned revision = 0;
        bool declared = false;
        bool is_visual_model = false;
    };

    static bool g_visualWorldInstalled = false;

    TrackWorldSource trackGroundSource(tTrack* track)
    {
        TrackWorldSource source;
        if (!track)
            return source;

        // Circuit identity and revision
        const char* name = GfParmGetStr(track->params, "Header", "name", "");
        source.circuit_id = (name && *name) ? name : (track->name ? track->name : "");
        if (source.circuit_id.empty() && track->internalname)
            source.circuit_id = track->internalname;
        const double version = GfParmGetNum(track->params, "Header", "version", nullptr, 0.0);
        source.revision = version > 0.0 ? static_cast<unsigned>(version) : 1u;

        // 1. Single source of truth: visual track model (.acc or .ac)
        const char* model3d = GfParmGetStr(track->params, TRK_SECT_GRAPH, TRK_ATT_3DDESC, "");
        if (!model3d || !*model3d)
            model3d = track->graphic.model3d;

        auto fileExists = [](const std::string& p) -> bool {
            if (p.empty()) return false;
            FILE* f = fopen(p.c_str(), "rb");
            if (f) { fclose(f); return true; }
            return false;
        };

        const char* category = track->category ? track->category : "circuit";
        const char* internalname = track->internalname ? track->internalname : "";

        std::string visualPath;
        if (model3d && *model3d)
        {
            char buf[512];
            snprintf(buf, sizeof(buf), "tracks/%s/%s/%s", category, internalname, model3d);
            std::string fullLocal = std::string(GfLocalDir()) + buf;
            std::string fullData = std::string(GfDataDir()) + buf;
            if (fileExists(fullLocal)) visualPath = fullLocal;
            else if (fileExists(fullData)) visualPath = fullData;
        }

        if (visualPath.empty() && *internalname)
        {
            char buf[512];
            snprintf(buf, sizeof(buf), "tracks/%s/%s/%s.acc", category, internalname, internalname);
            std::string fullLocal = std::string(GfLocalDir()) + buf;
            std::string fullData = std::string(GfDataDir()) + buf;
            if (fileExists(fullLocal)) visualPath = fullLocal;
            else if (fileExists(fullData)) visualPath = fullData;
        }

        if (!visualPath.empty())
        {
            source.declared = true;
            source.path = visualPath;
            source.is_visual_model = true;
            return source;
        }

        // 2. Fallback: legacy transfer document
        const char* relative =
            GfParmGetStr(track->params, "Header", "ground collision mesh", "");
        if (relative && *relative)
        {
            source.declared = true;
            source.path = std::string(GfDataDir()) + relative;
            source.is_visual_model = false;
            return source;
        }

        return source;
    }

    //! Installs the circuit geometry as the single session world, replacing any previous
    //! one. Returns the triangle count, or -1 when the session has no usable geometry.
    //! A previous session's world is never reused: that was the stale-track path.
    int installTrackWorld(const TrackWorldSource& source)
    {
        g_visualWorldInstalled = false;
        if (!rustPhysicsLibrary.isLoaded())
            return -1;
        if (!source.declared)
        {
            GfLogError("Rust physics: no visual track model (.acc) or ground collision mesh found\n");
            return -1;
        }
        int triangles = -1;
        if (source.is_visual_model && rustPhysicsLibrary.loadTrackVisualWorld)
        {
            triangles = rustPhysicsLibrary.loadTrackVisualWorld(
                source.path.c_str(), source.circuit_id.c_str(),
                static_cast<std::uint32_t>(source.revision));
            if (triangles > 0)
            {
                g_visualWorldInstalled = true;
                int obstacles = rustPhysicsLibrary.trackObstacleTriangles ? rustPhysicsLibrary.trackObstacleTriangles() : 0;
                GfLogInfo("Rust physics: visual track collision loaded from %s (%d driveable triangles, %d obstacle triangles)\n",
                          source.path.c_str(), triangles, obstacles);
            }
        }
        else if (rustPhysicsLibrary.loadTrackGroundMeshV2)
        {
            triangles = rustPhysicsLibrary.loadTrackGroundMeshV2(
                source.path.c_str(), source.circuit_id.c_str(),
                static_cast<std::uint32_t>(source.revision));
        }
        else if (rustPhysicsLibrary.loadTrackGroundMesh)
        {
            triangles = rustPhysicsLibrary.loadTrackGroundMesh(source.path.c_str());
        }
        if (triangles <= 0)
        {
            char error[1024] = { 0 };
            if (rustPhysicsLibrary.worldLastError)
                rustPhysicsLibrary.worldLastError(error, sizeof(error));
            GfLogError("Rust physics: cannot install track world %s (%s): %s\n",
                       source.path.c_str(), source.circuit_id.c_str(),
                       error[0] ? error : "unknown cause");
            return -1;
        }
        return triangles;
    }

    //! One-line identity of the installed world: circuit, revision, payload hash,
    //! triangle count and AABB. Written to the log so two sessions can be compared.
    void logTrackWorldIdentity(const char* reason)
    {
        if (!rustPhysicsLibrary.worldIdentityJson)
            return;
        static char buffer[2048];
        buffer[0] = '\0';
        const int length = rustPhysicsLibrary.worldIdentityJson(buffer, sizeof(buffer));
        const unsigned long long generation =
            rustPhysicsLibrary.worldGeneration ? rustPhysicsLibrary.worldGeneration() : 0ull;
        const unsigned long long installs =
            rustPhysicsLibrary.worldInstallCount ? rustPhysicsLibrary.worldInstallCount() : 0ull;
        GfLogInfo("RUSTWORLD reason=%s generation=%llu installs=%llu bytes=%d identity=%s\n",
                  reason, generation, installs, length, buffer);
    }

    void publishCar(RustCar& car)
    {
        tCarElt* elt = car.car;
        if (!elt)
            return;
        const OfpSnapshot& s = car.snapshot;

        // Published centre of gravity, from the single conversion in frame_transform.h.
        // Speed Dreams publishes `DynGCg.pos` as the CG, not as the body origin; the
        // inverse direction lives in `RustPhysics::setCar`.
        const ofp::frame::Basis body_basis = {
            toFrame(s.basis_x), toFrame(s.basis_y), toFrame(s.basis_z)
        };
        const ofp::frame::Vec3 com_rust =
            ofp::frame::com_from_origin(toFrame(s.position), body_basis,
                                        toFrame(car.center_of_mass_local));

        double com_x, com_y, com_z;
        rustToSd(toOfp(com_rust), com_x, com_y, com_z);
        double vel_x, vel_y, vel_z;
        rustToSd(s.linear_velocity, vel_x, vel_y, vel_z);
        double acc_x, acc_y, acc_z;
        rustToSd(s.linear_acceleration, acc_x, acc_y, acc_z);

        double forward[3], left[3], up[3];
        basisFromSnapshot(s, forward, left, up);
        double yaw, pitch, roll;
        eulerFromBasis(forward, left, up, yaw, pitch, roll);

        const double omega_sd[3] = { -s.angular_velocity.z, -s.angular_velocity.x, s.angular_velocity.y };

        tDynPt& global = elt->pub.DynGCg;
        global.pos.x = (tdble)com_x;
        global.pos.y = (tdble)com_y;
        global.pos.z = (tdble)com_z;
        global.pos.ax = (tdble)roll;
        global.pos.ay = (tdble)pitch;
        global.pos.az = (tdble)yaw;
        global.vel.x = (tdble)vel_x;
        global.vel.y = (tdble)vel_y;
        global.vel.z = (tdble)vel_z;
        global.vel.ax = (tdble)omega_sd[0];
        global.vel.ay = (tdble)omega_sd[1];
        global.vel.az = (tdble)omega_sd[2];
        global.acc.x = (tdble)acc_x;
        global.acc.y = (tdble)acc_y;
        global.acc.z = (tdble)acc_z;
        global.acc.ax = 0.0f;
        global.acc.ay = 0.0f;
        global.acc.az = 0.0f;

        tDynPt& local = elt->pub.DynGC;
        local.pos = global.pos;
        const double v_sd[3] = { vel_x, vel_y, vel_z };
        const double a_sd[3] = { acc_x, acc_y, acc_z };
        const double forward_speed = dot3(v_sd, forward);
        const double lateral_speed = dot3(v_sd, left);
        local.vel.x = (tdble)forward_speed;
        local.vel.y = (tdble)lateral_speed;
        local.vel.z = (tdble)vel_z;
        local.vel.xy = (tdble)std::sqrt(forward_speed * forward_speed + lateral_speed * lateral_speed);
        local.vel.ax = (tdble)omega_sd[0];
        local.vel.ay = (tdble)omega_sd[1];
        local.vel.az = (tdble)omega_sd[2];
        local.acc.x = (tdble)dot3(a_sd, forward);
        local.acc.y = (tdble)dot3(a_sd, left);
        local.acc.z = (tdble)acc_z;
        local.acc.ax = 0.0f;
        local.acc.ay = 0.0f;
        local.acc.az = 0.0f;
        car.forward_speed = forward_speed;
        car.lateral_speed = lateral_speed;

        // The ACC exporter added a constant vertical offset to chassis/driver
        // vertices. Their frame is the authored body origin, NOT the moving COM.
        // Preserve the exact solved basis (no Euler round-trip / world-Z correction).
        double body_x, body_y, body_z;
        rustToSd(s.position, body_x, body_y, body_z);
        sgMakeIdentMat4(elt->pub.posMat);
        for (int axis = 0; axis < 3; ++axis) {
            elt->pub.posMat[0][axis] = (float)forward[axis];
            elt->pub.posMat[1][axis] = (float)left[axis];
            elt->pub.posMat[2][axis] = (float)up[axis];
        }
        elt->pub.posMat[3][0] = (float)(body_x - up[0] * car.mesh_origin_height);
        elt->pub.posMat[3][1] = (float)(body_y - up[1] * car.mesh_origin_height);
        elt->pub.posMat[3][2] = (float)(body_z - up[2] * car.mesh_origin_height);

        elt->_gear = (int)s.gear;
        elt->_gearNext = (int)s.target_gear;
        // The Rust powertrain publishes engine speed in revolutions per minute; this is
        // the single conversion to the Speed Dreams rad/s contract.
        elt->_enginerpm = (tdble)(s.engine_rpm * 2.0 * PI / 60.0);
        elt->_fuel = (tdble)(s.fuel_kg / rustPhysicsLibrary.fuelDensity(car.handle));
        elt->_tank = (tdble)(s.fuel_capacity_kg / rustPhysicsLibrary.fuelDensity(car.handle));
        elt->_airtemp = 293.15f;
        elt->_airPressure = 101300.0f;
        elt->_steerTqCenter = (tdble)(-elt->ctrl.steer);
        elt->_steerTqAlign = 0.0f;

        // Speed Dreams wheel order is FR, FL, RR, RL; Rust publishes FL, FR, RL, RR.
        sgMat4 inverse_model;
        sgInvertMat4(inverse_model, elt->pub.posMat);
        for (int sd_index = 0; sd_index < 4; ++sd_index)
        {
            const OfpWheelState& wheel = s.wheels[SD_TO_RUST_WHEEL[sd_index]];
            tWheelState& view = elt->priv.wheel[sd_index];
            view.spinVel = (tdble)wheel.spin_rad_s;
            // CarElt keeps the brake state normalized to 0..1. The normalization uses the
            // profile's critical disc temperature, so a cold disc reads near zero without
            // pretending the physical temperature is zero.
            const double brake_critical = car.specs.brake_critical_temperature_c;
            view.brakeTemp = (tdble)(brake_critical > 1.0
                ? clampValue(wheel.brake_disc_temperature_c / brake_critical, 0.0, 1.0)
                : 0.0);
            view.state = wheel.grounded ? 0 : 16 /* SIM_WH_INAIR */;
            // CarElt exposes the three tread zones in kelvin and the pressure as an
            // absolute value in pascals. Each zone is its own measurement.
            view.temp_in = (tdble)(wheel.tread_inner_temperature_c + 273.15);
            view.temp_mid = (tdble)(wheel.tread_temperature_c + 273.15);
            view.temp_out = (tdble)(wheel.tread_outer_temperature_c + 273.15);
            // Optimal tread temperature of this axle, straight from the active profile.
            view.temp_opt = (tdble)(car.specs.tread_optimal_temperature_c[SD_TO_RUST_WHEEL[sd_index]]
                                    + 273.15);
            view.condition = (tdble)wheel.wear_remaining_fraction;
            view.treadDepth = (tdble)wheel.wear_remaining_fraction;
            view.Fx = (tdble)wheel.longitudinal_force_n;
            view.Fy = (tdble)wheel.lateral_force_n;
            view.Fz = (tdble)wheel.normal_force_n;
            view.effectiveMu = (tdble)wheel.wear_grip_scale;
            // Speed Dreams publishes absolute pressure in pascals; the ABI carries gauge
            // pressure, so the profile atmospheric pressure is added exactly once.
            view.currentPressure = (tdble)((wheel.pressure_kpa_gauge + 101.325) * 1000.0);
            // Legacy normalized-slip contract consumed by the retired tire widget. The
            // ratio is bounded and never divides by zero: the Rust tire model has no
            // published normalized peak, so 1.0 is the display unit.
            const double combined_slip = std::sqrt(
                wheel.slip_ratio * wheel.slip_ratio
                + std::tan(clampValue(wheel.slip_angle_rad, -1.4, 1.4))
                    * std::tan(clampValue(wheel.slip_angle_rad, -1.4, 1.4)));
            view.slipNorm = std::isfinite(combined_slip) ? (tdble)combined_slip : 0.0f;
            view.slipOpt = 1.0f;

            const double radius = wheel.tire_radius_m;
            elt->_wheelRadius(sd_index) = (tdble)radius;
            elt->_tireHeight(sd_index) = (tdble)(radius - elt->_rimRadius(sd_index));
            sgVec3 hub;
            double hub_x, hub_y, hub_z;
            rustToSd(wheel.hub_position, hub_x, hub_y, hub_z);
            sgSetVec3(hub, (float)hub_x, (float)hub_y, (float)hub_z);
            sgXformPnt3(hub, inverse_model);
            view.relPos.x = hub[0];
            view.relPos.y = hub[1];
            view.relPos.z = hub[2];
            view.relPos.ay = (tdble)car.wheel_rotation[sd_index];
            const double side = (sd_index % 2) ? -1.0 : 1.0;
            view.relPos.ax = (tdble)(wheel.dynamic_camber_rad * side);
            // Rust +Y yaw maps to Speed Dreams +Z yaw: preserve its sign,
            // including per-side toe already resolved by Rust.
            view.relPos.az = (tdble)wheel.steer_angle_rad;

            double contact_x, contact_y, contact_z;
            rustToSd(wheel.contact_point, contact_x, contact_y, contact_z);
            elt->pub.corner[sd_index].ax = (tdble)contact_x;
            elt->pub.corner[sd_index].ay = (tdble)contact_y;
            elt->pub.corner[sd_index].az = (tdble)contact_z;
            view.seg = elt->_trkPos.seg;
        }
        publishFormulaHud(car);
    }

    //! Publishes the Formula HUD presentation snapshot for the step just accepted by the
    //! physics. All values come from the same snapshot; nothing is estimated here.
    void publishFormulaHud(RustCar& car)
    {
        tCarElt* elt = car.car;
        if (!elt)
            return;
        const OfpSnapshot& s = car.snapshot;
        const OfpVehicleSpecs& specs = car.specs;
        tFormulaHudTelemetry& hud = elt->formulaHud;
        std::memset(&hud, 0, sizeof(hud));
        std::memset(hud.wheel, 0, sizeof(hud.wheel));

        const bool wheel_thermal = (s.flags & OFP_FLAG_WHEEL_THERMAL) != 0;
        const bool engine_thermal = (s.flags & OFP_FLAG_POWERTRAIN_THERMAL) != 0;
        const bool brake_thermal = (s.flags & OFP_FLAG_BRAKE_THERMAL) != 0;
        const bool fuel_ok = std::isfinite(s.fuel_kg) && std::isfinite(s.fuel_capacity_kg)
            && s.fuel_capacity_kg > 0.0;
        const bool rpm_ok = std::isfinite(s.engine_rpm) && specs.max_rpm > 0.0;
        const bool limits_ok = specs.tread_optimal_temperature_c[0] > 0.0
            && specs.water_optimal_max_temperature_c > specs.water_optimal_min_temperature_c;

        if (wheel_thermal && brake_thermal)
            hud.validMask |= FHT_VALID_WHEEL_THERMAL;
        if (engine_thermal)
            hud.validMask |= FHT_VALID_ENGINE_THERMAL;
        if (fuel_ok)
            hud.validMask |= FHT_VALID_FUEL;
        if (rpm_ok)
            hud.validMask |= FHT_VALID_RPM;
        if (limits_ok)
            hud.validMask |= FHT_VALID_LIMITS;
        if (std::isfinite(s.steer_input) && car.steering_wheel_lock_rad > 1.0e-6)
            hud.validMask |= FHT_VALID_STEERING;
        if (std::isfinite(s.throttle) && std::isfinite(s.brake))
            hud.validMask |= FHT_VALID_PEDALS;

        for (int sd_index = 0; sd_index < 4; ++sd_index)
        {
            const OfpWheelState& wheel = s.wheels[SD_TO_RUST_WHEEL[sd_index]];
            tFormulaHudWheel& target = hud.wheel[sd_index];
            target.treadInnerC = (tdble)wheel.tread_inner_temperature_c;
            target.treadCenterC = (tdble)wheel.tread_temperature_c;
            target.treadOuterC = (tdble)wheel.tread_outer_temperature_c;
            target.carcassC = (tdble)wheel.carcass_temperature_c;
            target.gasC = (tdble)wheel.gas_temperature_c;
            target.rimC = (tdble)wheel.brake_rim_temperature_c;
            target.brakeDiscC = (tdble)wheel.brake_disc_temperature_c;
            target.brakeEfficiency = (tdble)wheel.brake_efficiency;
            target.pressureKPaGauge = (tdble)wheel.pressure_kpa_gauge;
            const int rust_index = SD_TO_RUST_WHEEL[sd_index];
            hud.treadOptimalC[sd_index] = (tdble)specs.tread_optimal_temperature_c[rust_index];
            hud.treadWindowC[sd_index] = (tdble)specs.tread_optimal_window_c[rust_index];
            hud.treadOverheatC[sd_index] = (tdble)specs.tread_overheat_temperature_c[rust_index];
            hud.carcassOptimalC[sd_index] = (tdble)specs.carcass_optimal_temperature_c[rust_index];
        }

        hud.waterC = (tdble)s.water_temperature_celsius;
        hud.oilC = (tdble)s.oil_temperature_celsius;
        hud.waterOptimalMinC = (tdble)specs.water_optimal_min_temperature_c;
        hud.waterOptimalMaxC = (tdble)specs.water_optimal_max_temperature_c;
        hud.waterDeratingC = (tdble)specs.water_derating_temperature_c;
        hud.waterCriticalC = (tdble)specs.water_critical_temperature_c;
        hud.oilOptimalMinC = (tdble)specs.oil_optimal_min_temperature_c;
        hud.oilOptimalMaxC = (tdble)specs.oil_optimal_max_temperature_c;
        hud.oilDeratingC = (tdble)specs.oil_derating_temperature_c;
        hud.oilCriticalC = (tdble)specs.oil_critical_temperature_c;
        hud.brakeOptimalMinC = (tdble)specs.brake_optimal_min_temperature_c;
        hud.brakeOptimalMaxC = (tdble)specs.brake_optimal_max_temperature_c;
        hud.brakeFadeStartC = (tdble)specs.brake_fade_start_temperature_c;
        hud.brakeCriticalC = (tdble)specs.brake_critical_temperature_c;
        hud.fuelKg = (tdble)s.fuel_kg;
        hud.fuelCapacityKg = (tdble)s.fuel_capacity_kg;
        hud.fuelDensityKgPerL = (tdble)specs.fuel_density_kg_per_litre;
        hud.engineRpm = (tdble)s.engine_rpm;
        hud.engineRpmMax = (tdble)specs.max_rpm;
        hud.engineRpmIdle = (tdble)specs.idle_rpm;
        // Derived steering-wheel angle: the modelled car has no steering-column state.
        // Half the lock-to-lock rotation maps the normalized effective input to degrees,
        // keeping the sign convention of the physics (positive = left).
        hud.steerWheelDeg = (tdble)(s.steer_input
            * (car.steering_wheel_lock_rad * 0.5) * 180.0 / PI);
        hud.steerWheelFraction = (tdble)clampValue(s.steer_input, -1.0, 1.0);
        hud.throttleApplied = (tdble)s.throttle;
        hud.brakeApplied = (tdble)s.brake;
        hud.sampleTime = (tdble)s.sim_time_s;
    }

    void updateTrackPosition(tCarElt* elt)
    {
        if (!elt || !elt->_trkPos.seg)
            return;
        tTrkLocPos position;
        ::RtTrackGlobal2Local(elt->_trkPos.seg, (tdble)elt->_pos_X, (tdble)elt->_pos_Y,
                              &position, TR_LPOS_MAIN);
        elt->_trkPos = position;
    }

    int collectBarrierContacts(RustCar& car, OfpBodyContact* contacts, int capacity)
    {
        tCarElt* elt = car.car;
        if (!elt || !elt->_trkPos.seg || capacity <= 0)
            return 0;
        int count = 0;
        for (int i = 0; i < 4 && count < capacity; ++i)
        {
            const OfpWheelState& wheel = car.snapshot.wheels[i];
            const OfpVec3& source = wheel.grounded ? wheel.contact_point : wheel.hub_position;
            double x, y, z;
            rustToSd(source, x, y, z);
            tTrkLocPos local;
            ::RtTrackGlobal2Local(elt->_trkPos.seg, (tdble)x, (tdble)y, &local, TR_LPOS_TRACK);
            int side = 0;
            double toSide = 0.0;
            if (local.toRight < 0.0) { side = TR_SIDE_RGT; toSide = local.toRight; }
            else if (local.toLeft < 0.0) { side = TR_SIDE_LFT; toSide = local.toLeft; }
            else continue;
            tTrackSeg* segment = local.seg;
            if (!segment || !segment->barrier[side])
                continue;
            tTrackBarrier* barrier = segment->barrier[side];
            if (barrier->style == TR_NO_BARRIER || !barrier->surface)
                continue;
            OfpBodyContact& contact = contacts[count];
            std::memset(&contact, 0, sizeof(contact));
            contact.activation = 1;
            contact.surface = static_cast<std::int32_t>(side);
            const double nx = barrier->normal.x;
            const double ny = barrier->normal.y;
            const double px = x + nx * (-toSide);
            const double py = y + ny * (-toSide);
            contact.point = OfpVec3{ -py, z, -px };
            contact.normal = OfpVec3{ -ny, 0.0, -nx };
            contact.penetration_m = -toSide;
            contact.friction = barrier->surface->kFriction;
            contact.restitution = clampValue(barrier->surface->kRebound, 0.0, 0.2);
            contact.damage = barrier->surface->kDammage;
            ++count;
        }
        return count;
    }

    void applyBarrierDamage(RustCar& car, const OfpBodyContact* contacts, int count)
    {
        tCarElt* elt = car.car;
        if (!elt)
            return;
        for (int i = 0; i < OFP_MAX_BODY_CONTACTS; ++i)
        {
            const double impulse = car.snapshot.body_contact_impulse_ns[i];
            if (impulse <= 0.0)
                continue;
            car.last_barrier_contacts += 1;
            elt->priv.collision |= SEM_COLLISION_XYSCENE;
            elt->priv.simcollision = elt->priv.collision;
            const double damageCoeff = (i < count && contacts && contacts[i].activation)
                                           ? contacts[i].damage
                                           : 1.0;
            car.accumulated_damage += damageCoeff * impulse * impulse * 1.0e-4;
            elt->_dammage = (int)car.accumulated_damage;
            tCollisionState* state = &elt->priv.collision_state;
            state->collision_count += 1;
            if (i < count && contacts && contacts[i].activation)
            {
                for (int axis = 0; axis < 3; ++axis)
                {
                    const double* point = &contacts[i].point.x;
                    state->pos[axis] = (float)point[axis];
                    state->force[axis] = (float)(0.0001 * impulse);
                }
            }
            else
            {
                state->force[0] = (float)(0.0001 * impulse);
                state->force[1] = 0.0f;
                state->force[2] = 0.0f;
            }
        }
    }

    void stepCar(RustCar& car, tSituation* situation, double delta_time, tTrack* track)
    {
        (void)situation;
        (void)track;
        if (!car.ready || car.failed || !car.car)
            return;
        tCarElt* elt = car.car;
        if (elt->_state & RM_CAR_STATE_NO_SIMU)
            return;

        applyIntegrationScenario(car, delta_time);
        OfpControlInput input{};
        tCarCtrl* ctrl = &elt->ctrl;
        input.throttle = (double)ctrl->accelCmd;
        input.brake = (double)ctrl->brakeCmd;
        input.handbrake = (double)ctrl->ebrakeCmd;
        input.clutch = (double)ctrl->clutchCmd;
        input.steering = (double)ctrl->steer;
        input.gear_request = (std::int32_t)ctrl->gear;

        OfpBodyContact contacts[OFP_MAX_BODY_CONTACTS];
        int contact_count = 0;
        if (!g_visualWorldInstalled)
        {
            contact_count = collectBarrierContacts(car, contacts, OFP_MAX_BODY_CONTACTS);
        }

        const int code = rustPhysicsLibrary.step(car.handle, &input, contacts,
                                                 (std::uint32_t)contact_count, delta_time);
        if (code != 0)
        {
            char buffer[1024] = { 0 };
            rustPhysicsLibrary.getLastError(car.handle, buffer, sizeof(buffer));
            car.last_error = buffer;
            car.consecutive_failures += 1;
            GfLogWarning("Rust physics rejected step for car %d (%d consecutive): %s\n",
                         car.index, car.consecutive_failures, car.last_error.c_str());
            if (car.consecutive_failures >= 30)
            {
                car.failed = true;
                car.frozen = true;
                GfLogError("Rust physics permanently froze car %d: %s\n", car.index,
                           car.last_error.c_str());
            }
            if (rustPhysicsLibrary.snapshot(car.handle, &car.snapshot) != 0)
            {
                GfLogWarning("Rust physics: cannot read the snapshot for car %d\n",
                             car.index);
            }
            if (g_diagnostics && car.snapshot.rejected_steps != car.reported_rejections)
            {
                car.reported_rejections = car.snapshot.rejected_steps;
                logCarFailure(car);
                logDiagnostics(car);
                dumpTrace(car, "rejection");
            }
            return;
        }
        car.consecutive_failures = 0;
        if (rustPhysicsLibrary.snapshot(car.handle, &car.snapshot) != 0)
        {
            car.failed = true;
            car.last_error = "cannot read the Rust snapshot";
            return;
        }
        // Rendering phase only: Rust remains the owner of angular velocity and forces.
        for (int i = 0; i < 4; ++i)
            car.wheel_rotation[i] = std::remainder(car.wheel_rotation[i] +
                car.snapshot.wheels[SD_TO_RUST_WHEEL[i]].spin_rad_s *
                clampValue(delta_time, 1.0 / 1000.0, 1.0 / 20.0), 2.0 * PI);
        applyBarrierDamage(car, contacts, contact_count);
        updateTrackPosition(elt);
        publishCar(car);
        publishAudioStep(car);
    }

    void logDiagnostics(RustCar& car)
    {
        const OfpSnapshot& s = car.snapshot;
        double x, y, z;
        rustToSd(s.position, x, y, z);
        const double speed = std::sqrt(s.linear_velocity.x * s.linear_velocity.x
                                     + s.linear_velocity.z * s.linear_velocity.z);
        double forward_axis[3], left_axis[3], up_axis[3];
        basisFromSnapshot(s, forward_axis, left_axis, up_axis);
        double yaw = 0.0, pitch = 0.0, roll = 0.0;
        eulerFromBasis(forward_axis, left_axis, up_axis, yaw, pitch, roll);
        const OfpWheelDiagnostics* d = s.wheel_diagnostics;
        // One summary row per sample. `steer_rad` replaces the old `reach`, which
        // actually carried `steering_angle_rad`; `failed`/`frozen` make the rejection
        // explicit instead of leaving a repeated timestamp looking like live data.
        GfLogInfo("RUSTDIAG time=%.4f car=%d pos=(%.3f,%.3f,%.3f) speed=%.3f gear=%d rpm=%.1f "
                  "yaw=%.4f pitch=%.4f roll=%.4f vf=%.3f vl=%.3f "
                  "fuel=%.3f N=(%.1f,%.1f,%.1f,%.1f) travel=(%.4f,%.4f,%.4f,%.4f) "
                  "vax=(%.3f,%.3f,%.3f,%.3f) sep=(%.4f,%.4f,%.4f,%.4f) "
                  "ground=(%d,%d,%d,%d) contact=(%d,%d,%d,%d) rays=%llu/%llu floor=%llu/%llu "
                  "clearance=(%.4f,%.4f) floor_impulse=(%.3f,%.3f) steer_rad=%.4f "
                  "setup_contact=%d backends=0x%x rejected=%u "
                  "iters=(%u,%u) conv=(%d,%d) resid=(%.3e,%.3e) "
                  "failed=%d frozen=%d trace=%u\n",
                  s.sim_time_s, car.index, x, y, z, speed, s.gear, s.engine_rpm,
                  yaw, pitch, roll, car.forward_speed, car.lateral_speed,
                  s.fuel_kg, s.wheels[0].normal_force_n, s.wheels[1].normal_force_n,
                  s.wheels[2].normal_force_n, s.wheels[3].normal_force_n,
                  s.wheels[0].suspension_travel_m, s.wheels[1].suspension_travel_m,
                  s.wheels[2].suspension_travel_m, s.wheels[3].suspension_travel_m,
                  d[0].axial_velocity_m_s, d[1].axial_velocity_m_s,
                  d[2].axial_velocity_m_s, d[3].axial_velocity_m_s,
                  d[0].ground_separation_m, d[1].ground_separation_m,
                  d[2].ground_separation_m, d[3].ground_separation_m,
                  d[0].ground_detected, d[1].ground_detected,
                  d[2].ground_detected, d[3].ground_detected,
                  s.wheels[0].grounded, s.wheels[1].grounded, s.wheels[2].grounded,
                  s.wheels[3].grounded,
                  (unsigned long long)s.wheel_ray_hits, (unsigned long long)s.wheel_ray_queries,
                  (unsigned long long)s.floor_ray_hits, (unsigned long long)s.floor_ray_queries,
                  s.floor_clearance[0], s.floor_clearance[1],
                  s.floor_impulse_ns[0], s.floor_impulse_ns[1],
                  s.steering_angle_rad, s.body_contact_count, s.backends, s.rejected_steps,
                  s.axle_diagnostics[0].iterations, s.axle_diagnostics[1].iterations,
                  s.axle_diagnostics[0].converged, s.axle_diagnostics[1].converged,
                  s.axle_diagnostics[0].residual_m_s, s.axle_diagnostics[1].residual_m_s,
                  s.failure.active ? 1 : 0, car.frozen ? 1 : 0, s.trace_records);
        // One balance row per corner: sign, source and magnitude of every element,
        // the limits the solver honoured and the anchor motion that moved the mount.
        for (int wheel = 0; wheel < OFP_WHEELS; ++wheel)
        {
            const OfpWheelDiagnostics& w = d[wheel];
            GfLogInfo("RUSTSUSP time=%.4f car=%d axle=%d wheel=%s ground=%d binding=%d "
                      "sep_m=%.4f sep_n_m=%.4f shortfall_m=%.4f comp_m=%.4f vax_m_s=%.3f "
                      "travel_m=%.4f lim_m=(%.4f,%.4f) demand_m=%.4f pred_m=%.4f headroom_m=%.4f "
                      "spring_n=%.1f damper_n=%.1f arb_n=%.1f stop_n=%.1f sum_n=%.1f mr=%.3f "
                      "jN=%.4f jBump=%.4f jDroop=%.4f stop=(%d,%d) trapped=%d "
                      "anchor_p=(%.3f,%.3f,%.3f) anchor_v=(%.3f,%.3f,%.3f) "
                      "anchor_a=(%.3f,%.3f,%.3f) axial_a=%.3f "
                      "ray_mask=%u contact_p=(%.6f,%.6f,%.6f) contact_n=(%.6f,%.6f,%.6f)\n",
                      s.sim_time_s, car.index, wheel / 2, kWheelNames[wheel],
                      w.ground_detected, w.ground_binding,
                      w.ground_separation_m, w.ground_separation_normal_m,
                      w.ray_shortfall_m, w.compression_m, w.axial_velocity_m_s,
                      w.travel_m, w.min_compression_m, w.max_compression_m,
                      w.ground_demand_m, w.ground_demand_predicted_m, w.headroom_to_bump_m,
                      w.spring_force_n, w.damper_force_n, w.antiroll_force_n,
                      w.stop_force_n, w.element_force_sum_n, w.motion_ratio,
                      w.contact_impulse_ns, w.bump_stop_impulse_ns, w.droop_stop_impulse_ns,
                      w.bump_stop_active, w.droop_stop_active, w.trapped_at_stop,
                      w.anchor_position.x, w.anchor_position.y, w.anchor_position.z,
                      w.anchor_velocity.x, w.anchor_velocity.y, w.anchor_velocity.z,
                      w.anchor_acceleration.x, w.anchor_acceleration.y,
                      w.anchor_acceleration.z, w.axial_anchor_acceleration_m_s2,
                      s.wheels[wheel].ray_hit_mask,
                      s.wheels[wheel].contact_point.x, s.wheels[wheel].contact_point.y, s.wheels[wheel].contact_point.z,
                      s.wheels[wheel].contact_normal.x, s.wheels[wheel].contact_normal.y, s.wheels[wheel].contact_normal.z);
        }
        // Full contact telemetry: one JSON document per sample with the wheel-centre
        // geometry, the three rays, the gaps, the applied impulses, the energy ledger
        // and the identity of the collision world that produced them. Diagnostics only.
        if (rustPhysicsLibrary.contactReport && car.handle)
        {
            static char report[8192];
            report[0] = '\0';
            const int length = rustPhysicsLibrary.contactReport(car.handle, report, sizeof(report));
            if (length > 0)
            {
                GfLogInfo("RUSTCONTACT time=%.4f car=%d bytes=%d report=%s\n",
                          s.sim_time_s, car.index, length, report);
            }
        }
        // Per-axle solve quality and the bar load: the coupling that can hold a
        // corner while the other one is free.
        for (int axle = 0; axle < 2; ++axle)
        {
            const OfpAxleDiagnostics& a = s.axle_diagnostics[axle];
            GfLogInfo("RUSTAXLE time=%.4f car=%d axle=%d iterations=%u converged=%d "
                      "substeps=%u residual_m_s=%.3e arb_stiffness_n_per_m=%.1f "
                      "arb_force_n=%.1f normal_load_n=%.1f\n",
                      s.sim_time_s, car.index, axle, a.iterations, a.converged,
                      a.substeps, a.residual_m_s, a.antiroll_stiffness_n_per_m,
                      a.antiroll_force_n, a.normal_load_n);
        }
    }

    //! Exact rejection record. Emitted once per rejection, before the frozen state
    //! starts repeating its timestamp, and only when the counter actually advanced.
    void logCarFailure(RustCar& car)
    {
        const OfpSnapshot& s = car.snapshot;
        const OfpFailureDiagnostics& f = s.failure;
        const char* reason = car.last_error.empty() ? "unknown" : car.last_error.c_str();
        if (f.wheel >= 0 && f.wheel < OFP_WHEELS)
        {
            GfLogError("RUSTFAIL time=%.6f car=%d code=%d reason=\"%s\" wheel=%s(%d) axle=%d "
                       "iteration=%u subdivisions=%u demand_m=%.6f limit_m=%.6f excess_m=%.6f "
                       "limit_velocity_m_s=%.6f residual=%.3e rejected=%u failed=1 frozen=1 "
                       "state_time=%.6f note=\"state frozen at the last accepted step\"\n",
                       s.sim_time_s, car.index, f.code, reason, kWheelNames[f.wheel], f.wheel,
                       f.axle, f.iteration, f.subdivisions, f.demand_m, f.limit_m, f.excess_m,
                       f.limit_velocity_m_s, f.residual_m_s, s.rejected_steps, s.sim_time_s);
        }
        else
        {
            GfLogError("RUSTFAIL time=%.6f car=%d code=%d reason=\"%s\" wheel=none axle=%d "
                       "iteration=%u subdivisions=%u residual=%.3e rejected=%u failed=1 frozen=1 "
                       "state_time=%.6f note=\"state frozen at the last accepted step\"\n",
                       s.sim_time_s, car.index, f.code, reason, f.axle, f.iteration,
                       f.subdivisions, f.residual_m_s, s.rejected_steps, s.sim_time_s);
        }
    }

    //! Writes the Rust ring next to the log and reports where it went. The dump holds
    //! the rejected candidate row next to the last accepted tick; nothing is invented
    //! if tracing was never enabled.
    void dumpTrace(RustCar& car, const char* reason)
    {
        if (!rustPhysicsLibrary.traceDump || g_trace_directory.empty())
            return;
        const std::uint32_t buffered = rustPhysicsLibrary.traceBuffered
            ? rustPhysicsLibrary.traceBuffered() : 0u;
        const std::string path = g_trace_directory + "rustphysics_" + reason + "_car" +
            std::to_string(car.index) + "_t" + std::to_string((long long)(car.snapshot.sim_time_s * 1000.0)) +
            ".csv";
        const int rows = rustPhysicsLibrary.traceDump(path.c_str());
        GfLogInfo("RUSTTRACE car=%d reason=%s path=\"%s\" buffered=%u rows=%d\n",
                  car.index, reason, path.c_str(), buffered, rows);
    }

    //! The effective setup, straight from the running configuration: value, unit,
    //! admitted range and the derived coefficients the solver consumed.
    void logEffectiveSetup(RustCar& car, const char* reason)
    {
        if (!garageDiagnosticsEnabled() || !car.handle || !rustPhysicsLibrary.effectiveSetup)
            return;
        static char buffer[16384];
        const int length = rustPhysicsLibrary.effectiveSetup(car.handle, buffer, (std::uint32_t)sizeof(buffer));
        if (length <= 0)
            return;
        GfLogInfo("RUSTSETUPEFFECTIVE car=%d reason=%s bytes=%d\n", car.index, reason, length);
        const char* line = buffer;
        while (*line)
        {
            const char* end = std::strchr(line, '\n');
            const int span = end ? (int)(end - line) : (int)std::strlen(line);
            if (span > 0)
                GfLogInfo("RUSTSETUP %.*s\n", span, line);
            if (!end)
                break;
            line = end + 1;
        }
    }

    double normaliseAngle(double angle)
    {
        while (angle > PI) angle -= 2.0 * PI;
        while (angle < -PI) angle += 2.0 * PI;
        return angle;
    }

    //! One instrumented integration script: a deterministic track-following manoeuvre
    //! (launch, gear changes, braking, cornering) plus the setup transaction
    //! (apply, read back, restore) and a reset. It exists only for the final
    //! integration case and is disabled in normal play.
    void applyIntegrationScenario(RustCar& car, double delta_time)
    {
        if (!g_scenario || !car.ready || !car.car)
            return;
        tCarElt* elt = car.car;
        tCarCtrl* ctrl = &elt->ctrl;
        const OfpSnapshot& s = car.snapshot;
        const double t = g_elapsed;
        if ((g_scenario_flags & 8) == 0) {
            g_scenario_flags |= 8;
            elt->_gearCmd = 0;
            const bool enabled = applyFormulaGearControls(elt, true, false, t);
            const int first = elt->_gearCmd;
            applyFormulaGearControls(elt, false, true, t);
            const int neutral = elt->_gearCmd;
            applyFormulaGearControls(elt, false, true, t);
            const int reverse = elt->_gearCmd;
            for (int i = 0; i < 12; ++i) applyFormulaGearControls(elt, true, false, t);
            const int top = elt->_gearCmd;
            for (int i = 0; i < 6; ++i) applyFormulaGearControls(elt, false, true, t);
            GfLogInfo("RUSTHUMANGEAR enabled=%d first=%d neutral=%d reverse=%d top=%d final=%d\n",
                enabled ? 1 : 0, first, neutral, reverse, top, elt->_gearCmd);
        }

        if (t < 2.0)
        {
            ctrl->steer = 0.0f;
            ctrl->accelCmd = 0.0f;
            ctrl->brakeCmd = 1.0f;
            ctrl->clutchCmd = 0.0f;
            ctrl->gear = 0;
        }
        else if (t < 20.0 && elt->_trkPos.seg)
        {
            const tTrackSeg* segment = elt->_trkPos.seg;
            double speed_target = 55.0;
            const tTrackSeg* look = segment;
            for (int index = 0; index < 6 && look; ++index, look = look->next)
            {
                if (look->type != TR_STR && look->radius > 1.0)
                {
                    const double corner_speed = std::sqrt(1.55 * 9.80665 * (double)look->radius);
                    speed_target = std::min(speed_target, corner_speed);
                }
            }
            const double speed = std::sqrt(s.linear_velocity.x * s.linear_velocity.x
                                         + s.linear_velocity.z * s.linear_velocity.z);
            const double speed_error = speed_target - speed;
            ctrl->accelCmd = (float)clampValue(0.16 * speed_error, 0.0, 0.85);
            ctrl->brakeCmd = (float)clampValue(-0.18 * speed_error, 0.0, 0.9);
            ctrl->clutchCmd = 0.0f;

            const double track_yaw = ::RtTrackSideTgAngleL(&elt->_trkPos);
            const double heading_error = normaliseAngle(track_yaw - (double)elt->_yaw);
            const double half_width = std::max(2.0, (double)segment->width * 0.5);
            const double lateral_right =
                ((double)segment->width * 0.5 - (double)elt->_trkPos.toRight) / half_width;
            ctrl->steer = (float)clampValue(1.25 * heading_error + 0.30 * lateral_right, -0.5, 0.5);

            int gear = (int)s.gear;
            if (s.engine_rpm > 15200.0 && gear < 6) gear += 1;
            else if (s.engine_rpm < 6500.0 && gear > 1) gear -= 1;
            // Feed a requested edge through the real human control path. This must
            // respect published gear limits, just like keyboard/controller input.
            if ((int)ctrl->gear != gear)
                applyFormulaGearControls(elt, gear > (int)ctrl->gear, gear < (int)ctrl->gear, t);
        }
        else
        {
            ctrl->accelCmd = 0.0f;
            ctrl->brakeCmd = 0.25f;
            ctrl->steer = 0.0f;
        }

        if ((g_scenario_flags & 1) == 0 && t >= 23.0)
        {
            g_scenario_flags |= 1;
            std::string error;
            std::vector<PhysicsSetupField> before;
            const bool readable = RustPhysics::self().garageFields(car.index, before);
            const bool applied = readable && RustPhysics::self().applyGarageFields(car.index,
                {{"suspension.geometry_physical.front_arb.ratio", 0.55}, {"brakes.front_brake_bias", 0.58}}, error);
            std::vector<PhysicsSetupField> after;
            const bool readback = RustPhysics::self().garageFields(car.index, after);
            bool arb_readback = false, bias_readback = false;
            for (const auto& field : after) {
                if (field.path == "suspension.geometry_physical.front_arb.ratio") arb_readback = std::abs(field.value - 0.55) < 1e-9;
                if (field.path == "brakes.front_brake_bias") bias_readback = std::abs(field.value - 0.58) < 1e-9;
            }
            GfLogInfo("RUSTGARAGE fields=%u readback=%d arb=%d bias=%d fuel_litres=%.4f density=%.4f\n",
                (unsigned)before.size(), readback ? 1 : 0, arb_readback ? 1 : 0, bias_readback ? 1 : 0,
                car.car->_fuel, rustPhysicsLibrary.fuelDensity(car.handle));
            GfLogInfo("RUSTSETUP apply=%d error=%s\n", applied ? 1 : 0, error.c_str());
            std::string document;
            if (RustPhysics::self().setupDocument(car.index, document))
            {
                const bool arb = document.find("\"ratio\":0.55") != std::string::npos;
                const bool bias = document.find("\"front_brake_bias\":0.58") != std::string::npos;
                const bool third_spring = document.find("285000") != std::string::npos;
                GfLogInfo("RUSTSETUP document arb=%d bias=%d third_spring=%d bytes=%u\n",
                          arb ? 1 : 0, bias ? 1 : 0, third_spring ? 1 : 0,
                          (unsigned)document.size());
            }
        }
        if ((g_scenario_flags & 2) == 0 && t >= 25.0)
        {
            g_scenario_flags |= 2;
            std::string error;
            const bool restored = RustPhysics::self().resetSetup(car.index, error);
            GfLogInfo("RUSTSETUP restore=%d error=%s\n", restored ? 1 : 0, error.c_str());
        }
        if ((g_scenario_flags & 4) == 0 && t >= 27.0)
        {
            g_scenario_flags |= 4;
            const OfpVec3 origin = car.snapshot.position;
            const double yaw = std::atan2(-car.snapshot.basis_z.y, -car.snapshot.basis_z.x);
            const int code = rustPhysicsLibrary.reset(car.handle, origin, yaw);
            rustPhysicsLibrary.snapshot(car.handle, &car.snapshot);
            resetAudioSession(car);
            GfLogInfo("RUSTSETUP reset=%d yaw=%.4f\n", code, yaw);
        }
        (void)delta_time;
    }

    void refreshDisplayLimits(RustCar& car)
    {
        OfpVehicleSpecs specs{};
        if (rustPhysicsLibrary.vehicleSpecs(car.handle, &specs) != 0) return;
        car.specs = specs;
        tCarElt* elt = car.car;
        elt->_gearOffset = specs.has_reverse ? 1 : 0;
        elt->_gearNb = specs.forward_gears + 1; // Speed Dreams limit is highest forward gear + 1.
        std::fill(elt->priv.gearRatio, elt->priv.gearRatio + MAX_GEARS, 0.0);
        if (specs.has_reverse) elt->_gearRatio[0] = (tdble)(-specs.reverse_ratio * specs.final_drive);
        for (int gear = 1; gear <= specs.forward_gears; ++gear)
            elt->_gearRatio[gear + elt->_gearOffset] = (tdble)(specs.gear_ratios[gear - 1] * specs.final_drive);
        static const char* wheel_sections[4] = {SECT_FRNTRGTWHEEL, SECT_FRNTLFTWHEEL, SECT_REARRGTWHEEL, SECT_REARLFTWHEEL};
        static const char* brake_sections[4] = {SECT_FRNTRGTBRAKE, SECT_FRNTLFTBRAKE, SECT_REARRGTBRAKE, SECT_REARLFTBRAKE};
        for (int i = 0; i < 4; ++i) {
            const double radius = car.snapshot.wheels[SD_TO_RUST_WHEEL[i]].tire_radius_m;
            // Rim and disc dimensions describe visual assets, not a second tire model.
            const double rim = GfParmGetNum(elt->_carHandle, wheel_sections[i], PRM_RIMDIAM, nullptr, 0.3302) * 0.5;
            elt->_rimRadius(i) = (tdble)std::min(rim, radius);
            elt->_tireHeight(i) = (tdble)(radius - elt->_rimRadius(i));
            elt->_wheelRadius(i) = (tdble)radius;
            elt->_tireWidth(i) = (tdble)specs.tire_width_m[SD_TO_RUST_WHEEL[i]];
            elt->_brakeDiskRadius(i) = GfParmGetNum(elt->_carHandle, brake_sections[i], PRM_BRKDIAM, nullptr, 0.28) * 0.5;
        }
        // Steering-wheel rotation is visual car metadata, not a physics constant: it is
        // the range the HUD uses to turn the effective normalized steering into degrees.
        car.steering_wheel_lock_rad = GfParmGetNum(elt->_carHandle, SECT_STEER, PRM_STEERROT,
                                                   nullptr, 0.0);
        if (!std::isfinite(car.steering_wheel_lock_rad) || car.steering_wheel_lock_rad < 0.0)
            car.steering_wheel_lock_rad = 0.0;
        GfLogInfo("RUSTSPECS forward=%d offset=%d ratios=(%.4f,%.4f) wheel_fr=(r=%.6f,w=%.6f) wheel_rr=(r=%.6f,w=%.6f) "
                  "rpm_max=%.0f rpm_idle=%.0f steer_lock=%.4f rad fuel_capacity=%.3f kg density=%.4f kg/L\n",
            specs.forward_gears, elt->_gearOffset, (double)elt->_gearRatio[elt->_gearOffset + 1],
            (double)elt->_gearRatio[elt->_gearOffset + specs.forward_gears],
            (double)elt->_wheelRadius(0), (double)elt->_tireWidth(0),
            (double)elt->_wheelRadius(2), (double)elt->_tireWidth(2),
            specs.max_rpm, specs.idle_rpm, car.steering_wheel_lock_rad,
            specs.fuel_capacity_kg, specs.fuel_density_kg_per_litre);
        std::vector<PhysicsSetupField> fields;
        if (!RustPhysics::self().garageFields(car.index, fields)) return;
        for (const auto& field : fields) {
            if (field.path == "powertrain.max_rpm" && field.value > 0.0) {
                car.car->_enginerpmMax = (tdble)(field.value * 2.0 * PI / 60.0);
                car.car->_enginerpmRedLine = car.car->_enginerpmMax;
            }
        }
    }

    void createCar(RustCar& car, int index, tCarElt* elt, tTrack* track)
    {
        car.index = index;
        // A new car never inherits the previous session's frozen state or counters.
        car.frozen = false;
        car.reported_rejections = 0;
        car.consecutive_failures = 0;
        car.next_diagnostic_s = 0.0;
        car.last_error.clear();
        car.car = elt;
        // Clear any telemetry inherited from a previous car or session: the HUD must show
        // unknown values until the new vehicle has published an accepted step.
        std::memset(&elt->formulaHud, 0, sizeof(elt->formulaHud));
        // The audio ring is valid only with schema_version == OFP_AUDIO_STEP_VERSION;
        // clear it so `snddefault` never mistakes stale memory for a step.
        std::memset(&elt->audio_steps, 0, sizeof(elt->audio_steps));
        elt->audio_step_write = 0;
        elt->audio_session = car.audio_session;
        car.specs = OfpVehicleSpecs{};
        car.steering_wheel_lock_rad = 0.0;
        // Presentation metadata remains in the visual car XML, independent of the
        // authoritative Rust mass/inertia and suspension configuration.
        void* visual = elt->_carHandle;
        car.mesh_origin_height = GfParmGetNum(visual, SECT_GROBJECTS,
            "source mesh origin offset z", nullptr, 0.0);
        elt->_dimension_x = GfParmGetNum(visual, SECT_CAR, PRM_LEN, nullptr, 4.7);
        elt->_dimension_y = GfParmGetNum(visual, SECT_CAR, PRM_WIDTH, nullptr, 1.9);
        elt->_dimension_z = GfParmGetNum(visual, SECT_CAR, PRM_HEIGHT, nullptr, 1.2);
        elt->_statGC_z = GfParmGetNum(visual, SECT_CAR, PRM_GCHEIGHT, nullptr, 0.5);
        elt->_drvPos_x = GfParmGetNum(visual, SECT_DRIVER, PRM_XPOS, nullptr, 0.0);
        elt->_drvPos_y = GfParmGetNum(visual, SECT_DRIVER, PRM_YPOS, nullptr, 0.0);
        elt->_drvPos_z = GfParmGetNum(visual, SECT_DRIVER, PRM_ZPOS, nullptr, 0.0);
        elt->_bonnetPos_x = GfParmGetNum(visual, SECT_BONNET, PRM_XPOS, nullptr, elt->_drvPos_x);
        elt->_bonnetPos_y = GfParmGetNum(visual, SECT_BONNET, PRM_YPOS, nullptr, elt->_drvPos_y);
        elt->_bonnetPos_z = GfParmGetNum(visual, SECT_BONNET, PRM_ZPOS, nullptr, elt->_drvPos_z);
        sgMakeCoordMat4(elt->pub.posMat, elt->_pos_X, elt->_pos_Y,
            elt->_pos_Z - elt->_statGC_z, (float)(elt->_yaw * 180.0 / PI), 0.0f, 0.0f);
        OfpVec3 origin;
        // Rust body origin = COM - basis * cg_local; at spawn the basis is yaw only.
        const double yaw = (double)elt->_yaw;

        // Speed Dreams basis: forward = (cos, sin, 0), left = (-sin, cos, 0), up = (0,0,1).
        const double com_x = (double)elt->_pos_X;
        const double com_y = (double)elt->_pos_Y;
        const double com_z = (double)elt->_pos_Z;
        if (!rustPhysicsLibrary.create)
            return;
        // First create at the COM position, then shift by the published COM offset.
        sdToRust(com_x, com_y, com_z, origin);
        car.handle = rustPhysicsLibrary.create(
            g_profile_path.empty() ? nullptr : g_profile_path.c_str(), origin, yaw, 0.0);
        if (!car.handle)
        {
            car.failed = true;
            car.last_error = "the Rust library could not create the vehicle";
            GfLogError("Rust physics: cannot create car %d\n", index);
            return;
        }
        rustPhysicsLibrary.vehicleGeometry(car.handle, &car.center_of_mass_local, &car.inertia,
                                           &car.spawn_height, &car.mass);
        // Ensure the initial height places the tires on the track, regardless of whether
        // raceman XML specified initial height = 0.20 or 0.35.
        const double track_ground_z = RtTrackHeightL(&(elt->_trkPos));
        double effective_com_z = com_z;
        if (car.spawn_height > 0.1)
        {
            const double desired_com_z = track_ground_z + car.spawn_height + car.center_of_mass_local.y;
            if (effective_com_z < desired_com_z)
            {
                effective_com_z = desired_com_z;
            }
        }
        // Reposition so that the published centre of gravity matches the placement.
        const ofp::frame::Basis spawn_basis = ofp::frame::basis_from_sd_euler(0.0, 0.0, yaw);
        const ofp::frame::Vec3 com_rust = sdToRustOf(com_x, com_y, effective_com_z);
        const ofp::frame::Vec3 body_origin =
            ofp::frame::origin_from_com(com_rust, spawn_basis, toFrame(car.center_of_mass_local));
        origin = toOfp(body_origin);
        rustPhysicsLibrary.reset(car.handle, origin, yaw);
        car.ready = true;
        car.failed = false;
        resetAudioSession(car);
        if (rustPhysicsLibrary.snapshot(car.handle, &car.snapshot) != 0)
        {
            car.failed = true;
            car.ready = false;
            return;
        }
        refreshDisplayLimits(car);
        // Named setups are loaded explicitly in the garage. A car-index TSV
        // cannot identify a car/track pair and must not override this profile.
        updateTrackPosition(elt);
        publishCar(car);
        // The car exists with a known setup: record what is actually in force before
        // the first step, so a later transaction has a baseline to compare against.
        logEffectiveSetup(car, "create");
    }

    void destroyCar(RustCar& car)
    {
        if (car.car)
        {
            // Never leave the previous car's presentation telemetry behind.
            std::memset(&car.car->formulaHud, 0, sizeof(car.car->formulaHud));
            // Same for the audio ring: stale steps must never look fresh.
            std::memset(&car.car->audio_steps, 0, sizeof(car.car->audio_steps));
            car.car->audio_step_write = 0;
        }
        if (car.handle && rustPhysicsLibrary.destroy)
            rustPhysicsLibrary.destroy(car.handle);
        car.handle = nullptr;
        car.ready = false;
        car.car = nullptr;
        car.specs = OfpVehicleSpecs{};
        car.snapshot = OfpSnapshot{};
        car.steering_wheel_lock_rad = 0.0;
        std::fill(car.wheel_rotation, car.wheel_rotation + 4, 0.0);
    }
}

RustPhysics* RustPhysics::_pSelf = 0;

int openGfModule(const char* pszShLibName, void* hShLibHandle)
{
    RustPhysics::_pSelf = new RustPhysics(pszShLibName, hShLibHandle);
    if (RustPhysics::_pSelf)
        GfModule::register_(RustPhysics::_pSelf);
    return RustPhysics::_pSelf ? 0 : 1;
}

int closeGfModule()
{
    if (RustPhysics::_pSelf)
        RustPhysics::unregister(RustPhysics::_pSelf);
    delete RustPhysics::_pSelf;
    RustPhysics::_pSelf = 0;
    return 0;
}

RustPhysics& RustPhysics::self()
{
    return *_pSelf;
}

RustPhysics::RustPhysics(const std::string& strShLibName, void* hShLibHandle)
    : GfModule(strShLibName, hShLibHandle)
{
}

RustPhysics::~RustPhysics()
{
}

void RustPhysics::initialize(int nCars, struct Track* pTrack)
{
    g_track = pTrack;
    g_cars.clear();
    g_cars.resize(nCars);
    for (int index = 0; index < nCars; ++index)
        g_cars[index].index = index;

    std::string configuration_path = std::string(GfLocalDir()) + "config/rust_physics.xml";
    if (!std::ifstream(configuration_path.c_str()).good())
        configuration_path = std::string(GfDataDir()) + "config/rust_physics.xml";
    void* configuration = ::GfParmReadFile(configuration_path.c_str(), GFPARM_RMODE_STD);
    if (!rustPhysicsLibrary.isLoaded())
    {
        const char* configured = configuration
            ? ::GfParmGetStr(configuration, "RustPhysics", "library",
                             "modules/simu/vehicle_physics_engine.dll")
            : "modules/simu/vehicle_physics_engine.dll";
        g_library_path = std::string(GfLibDir()) + configured;
        if (!rustPhysicsLibrary.load(g_library_path))
        {
            GfLogError("Rust physics: cannot load %s: %s\n", g_library_path.c_str(),
                       rustPhysicsLibrary.lastError().c_str());
        }
        else
        {
            GfLogInfo("Rust physics: loaded %s\n", g_library_path.c_str());
            GfLogInfo("Rust physics: %s\n", rustPhysicsLibrary.buildProvenance());
        }
    }

    const char* profile = configuration
        ? ::GfParmGetStr(configuration, "RustPhysics", "profile",
                         "cars/rust/williams-fw19-hockenheim/vehicle.json")
        : "cars/rust/williams-fw19-hockenheim/vehicle.json";
    g_profile_path = std::string(GfDataDir()) + profile;

    // One world per session. The install replaces any previous circuit inside the same
    // process; the Rust registry bumps its generation, so vehicles of the previous
    // session refuse to step instead of querying the old triangles.
    const TrackWorldSource world_source = trackGroundSource(pTrack);
    g_ground_mesh_path = world_source.path;
    g_track_ready = false;
    if (rustPhysicsLibrary.isLoaded())
    {
        const int triangles = installTrackWorld(world_source);
        if (triangles > 0)
        {
            g_track_ready = true;
            GfLogInfo("Rust physics: ground world %s circuit=%s revision=%u (%d triangles)\n",
                      world_source.path.c_str(), world_source.circuit_id.c_str(),
                      world_source.revision, triangles);
            logTrackWorldIdentity("initialize");
        }
        else
        {
            GfLogError("Rust physics: the session runs without authoritative ground "
                       "geometry; car creation will be refused\n");
        }
    }
    GfLogInfo("Rust physics: profile %s\n", g_profile_path.c_str());
    if (configuration)
    {
        const double diagnostics = ::GfParmGetNum(configuration, "RustPhysics",
                                                "diagnostic telemetry", nullptr, 0.0);
        const double scenario = ::GfParmGetNum(configuration, "RustPhysics",
                                             "integration scenario", nullptr, 0.0);
        g_diagnostic_interval = ::GfParmGetNum(configuration, "RustPhysics",
                                              "diagnostic interval seconds", nullptr, 0.04);
        if (!std::isfinite(g_diagnostic_interval) || g_diagnostic_interval < 0.01)
            g_diagnostic_interval = 0.01;
        g_diagnostic_trace = ::GfParmGetNum(configuration, "RustPhysics",
                                           "diagnostic trace", nullptr, 0.0) > 0.0;
        const char* trace_directory = ::GfParmGetStr(configuration, "RustPhysics",
                                                    "diagnostic trace directory", "rustphysics/");
        g_trace_directory = std::string(GfLocalDir())
            + (trace_directory && *trace_directory ? trace_directory : "rustphysics/");
        g_diagnostics = diagnostics > 0.0;
        g_scenario = scenario > 0.0;
        ::GfParmReleaseHandle(configuration);
    }
    if (g_diagnostics && g_diagnostic_trace && rustPhysicsLibrary.isLoaded()
        && rustPhysicsLibrary.traceConfigure)
    {
        const int trace = rustPhysicsLibrary.traceConfigure(g_trace_directory.c_str());
        GfLogInfo("Rust physics: trace directory %s enabled=%d\n",
                  g_trace_directory.c_str(), trace);
    }
    GfLogInfo("Rust physics: diagnostics=%d scenario=%d interval=%.4f trace=%d\n",
              g_diagnostics ? 1 : 0, g_scenario ? 1 : 0, g_diagnostic_interval,
              (g_diagnostics && g_diagnostic_trace) ? 1 : 0);
}

void RustPhysics::configureCar(struct CarElt* pCar)
{
    if (!pCar || pCar->index < 0 || pCar->index >= (int)g_cars.size())
        return;
    if (!rustPhysicsLibrary.isLoaded())
        return;
    RustCar& car = g_cars[pCar->index];
    destroyCar(car);
    createCar(car, pCar->index, pCar, g_track);
}

void RustPhysics::reconfigureCar(struct CarElt* pCar)
{
    if (!pCar || pCar->index < 0 || pCar->index >= (int)g_cars.size())
        return;
    RustCar& car = g_cars[pCar->index];
    if (!car.ready || !car.handle)
        return;

    if (pCar->pitcmd.fuel > 0.0)
    {
        const double target = car.snapshot.fuel_kg + (double)pCar->pitcmd.fuel * rustPhysicsLibrary.fuelDensity(car.handle);
        rustPhysicsLibrary.pitFuel(car.handle, target);
    }
    if (pCar->pitcmd.tireChange == tCarPitCmd::ALL)
        rustPhysicsLibrary.pitTires(car.handle);

    if (pCar->pitcmd.setupChanged)
    {
        // Rust setup transactions are applied atomically by applyGarageFields before
        // the shared pit/reset service. Never treat an empty native setup as applied.
        pCar->pitcmd.setupChanged = false;
    }
    rustPhysicsLibrary.snapshot(car.handle, &car.snapshot);
    publishCar(car);
}

void RustPhysics::toggleCarTelemetry(int nCarIndex, bool bOn)
{
    (void)nCarIndex;
    (void)bOn;
}

void RustPhysics::updateSituation(struct Situation* pSituation, double fDeltaTime)
{
    if (!pSituation)
        return;
    for (int index = 0; index < pSituation->_ncars; ++index)
    {
        if (index >= (int)g_cars.size())
            break;
        RustCar& car = g_cars[index];
        if (!car.car)
            continue;
        if (g_diagnostics && car.ready && !car.frozen && g_elapsed >= car.next_diagnostic_s)
        {
            logDiagnostics(car);
            car.next_diagnostic_s = g_elapsed + g_diagnostic_interval;
        }
        if (pSituation->_raceState & RM_RACE_PRESTART)
        {
            car.car->ctrl.brakeCmd = 1.0f;
            car.car->ctrl.clutchCmd = 1.0f;
        }
        stepCar(car, pSituation, fDeltaTime, g_track);
    }
    g_elapsed += fDeltaTime;
}

void RustPhysics::updateCar(struct Situation* pSituation, double fDeltaTime, int nCarIndex)
{
    if (!pSituation || nCarIndex < 0 || nCarIndex >= (int)g_cars.size())
        return;
    stepCar(g_cars[nCarIndex], pSituation, fDeltaTime, g_track);
}

void RustPhysics::setCar(const struct DynPt& dynGCG, int nCarIndex)
{
    if (nCarIndex < 0 || nCarIndex >= (int)g_cars.size())
        return;
    RustCar& car = g_cars[nCarIndex];
    if (!car.car || !car.handle)
        return;
    car.car->pub.DynGCg = dynGCG;
    // `DynGCg.pos` is the centre of gravity. Rust owns the body origin, so the CG must
    // be converted back with the inverse of `publishCar`: origin = com - basis * cg.
    // The previous implementation forwarded the CG as the origin (a displacement of
    // |com_local|, ~0.3 m) and rebuilt the basis from the yaw alone, discarding pitch
    // and roll; a rolled or banked car was therefore re-seated level.
    const ofp::frame::Basis basis = ofp::frame::basis_from_sd_euler(
        (double)dynGCG.pos.ax, (double)dynGCG.pos.ay, (double)dynGCG.pos.az);
    if (ofp::frame::orthonormality_error(basis) > 1.0e-6)
    {
        GfLogWarning("Rust physics: setCar received a non-orthonormal attitude for car %d "
                     "(yaw=%.6f pitch=%.6f roll=%.6f); the pose is normalised by Rust\n",
                     nCarIndex, (double)dynGCG.pos.az, (double)dynGCG.pos.ay,
                     (double)dynGCG.pos.ax);
    }
    const ofp::frame::Vec3 com_rust =
        sdToRustOf((double)dynGCG.pos.x, (double)dynGCG.pos.y, (double)dynGCG.pos.z);
    const ofp::frame::Vec3 body_origin = ofp::frame::origin_from_com(
        com_rust, basis, toFrame(car.center_of_mass_local));
    rustPhysicsLibrary.setPose(car.handle, toOfp(body_origin),
                               toOfp(basis.x), toOfp(basis.y), toOfp(basis.z));
}

struct DynPt* RustPhysics::getCar(int nCarIndex)
{
    if (nCarIndex < 0 || nCarIndex >= (int)g_cars.size() || !g_cars[nCarIndex].car)
        return nullptr;
    return &g_cars[nCarIndex].car->pub.DynGCg;
}

void RustPhysics::shutdown()
{
    // Drain/join while the host is running normally, before module unload/DllMain.
    if (rustPhysicsLibrary.traceShutdown) rustPhysicsLibrary.traceShutdown();
    for (RustCar& car : g_cars)
        destroyCar(car);
    g_cars.clear();
    g_track = nullptr;
    g_track_ready = false;
    g_ground_mesh_path.clear();
    if (rustPhysicsLibrary.releaseTrackGroundMesh)
    {
        const int released = rustPhysicsLibrary.releaseTrackGroundMesh();
        GfLogInfo("Rust physics: ground world released=%d\n", released);
    }
}

bool RustPhysics::applySetupPatch(int car_index, const std::string& patch_json, std::string& error)
{
    if (car_index < 0 || car_index >= (int)g_cars.size())
    {
        error = "invalid car index";
        return false;
    }
    RustCar& car = g_cars[car_index];
    if (!car.ready || !car.handle)
    {
        error = "the Rust vehicle is not ready";
        return false;
    }
    if (patch_json.empty())
        return true;
    if (rustPhysicsLibrary.applySetup(car.handle, patch_json.c_str()) != 0)
    {
        char buffer[1024] = { 0 };
        rustPhysicsLibrary.getLastError(car.handle, buffer, sizeof(buffer));
        error = buffer;
        return false;
    }
    rustPhysicsLibrary.snapshot(car.handle, &car.snapshot);
    refreshDisplayLimits(car);
    publishCar(car);
    return true;
}

bool RustPhysics::setupDocument(int car_index, std::string& document)
{
    if (car_index < 0 || car_index >= (int)g_cars.size())
        return false;
    RustCar& car = g_cars[car_index];
    if (!car.ready || !car.handle)
        return false;
    const int length = rustPhysicsLibrary.setupDocument(car.handle, nullptr, 0);
    if (length <= 0)
        return false;
    std::vector<char> buffer((size_t)length + 1, '\0');
    rustPhysicsLibrary.setupDocument(car.handle, buffer.data(), (std::uint32_t)buffer.size());
    document.assign(buffer.data(), (size_t)length);
    return true;
}

bool RustPhysics::resetSetup(int car_index, std::string& error)
{
    if (car_index < 0 || car_index >= (int)g_cars.size())
    {
        error = "invalid car index";
        return false;
    }
    RustCar& car = g_cars[car_index];
    if (!car.ready || !car.handle)
    {
        error = "the Rust vehicle is not ready";
        return false;
    }
    if (rustPhysicsLibrary.resetSetup(car.handle) != 0)
    {
        char buffer[1024] = { 0 };
        rustPhysicsLibrary.getLastError(car.handle, buffer, sizeof(buffer));
        error = buffer;
        return false;
    }
    car.setup_overrides.clear();
    rustPhysicsLibrary.snapshot(car.handle, &car.snapshot);
    refreshDisplayLimits(car);
    publishCar(car);
    return true;
}


bool RustPhysics::garageFields(int car_index, std::vector<PhysicsSetupField>& fields)
{
    fields.clear();
    if (car_index < 0 || car_index >= (int)g_cars.size() || !g_cars[car_index].ready) return false;
    OfpHandle handle = g_cars[car_index].handle;
    const int length = rustPhysicsLibrary.setupFields(handle, nullptr, 0);
    if (length <= 0) return false;
    std::vector<char> buffer(length + 1, 0);
    rustPhysicsLibrary.setupFields(handle, buffer.data(), (std::uint32_t)buffer.size());
    std::istringstream lines(buffer.data());
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream row(line); row.imbue(std::locale::classic());
        PhysicsSetupField field; int editable = 0;
        if (!std::getline(row, field.path, '\t') || !std::getline(row, field.label, '\t') ||
            !std::getline(row, field.unit, '\t') ||
            !(row >> field.value >> field.minimum >> field.maximum >> field.step >> editable)) return false;
        field.editable = editable != 0;
        fields.push_back(field);
    }
    return !fields.empty();
}

bool RustPhysics::applyGarageFields(int car_index,
    const std::vector<std::pair<std::string, double> >& values, std::string& error)
{
    if (values.empty()) return true;
    std::vector<PhysicsSetupField> descriptors;
    if (!garageFields(car_index, descriptors)) { error = "Rust setup is unavailable"; return false; }
    std::ostringstream patch; patch.imbue(std::locale::classic()); patch << std::setprecision(17) << '{';
    bool first = true;
    for (const auto& value : values) {
        const auto field = std::find_if(descriptors.begin(), descriptors.end(),
            [&](const PhysicsSetupField& f) { return f.path == value.first; });
        if (field == descriptors.end() || !field->editable || !std::isfinite(value.second) ||
            value.second < field->minimum || value.second > field->maximum) {
            error = "Invalid Rust setup field: " + value.first; return false;
        }
        if (!first) patch << ',';
        first = false;
        patch << '"' << value.first << "\":" << value.second;
    }
    patch << '}';
    const bool result = applySetupPatch(car_index, patch.str(), error);
    if (result) {
        RustCar& car = g_cars[car_index];
        for (const auto& value : values) car.setup_overrides[value.first] = value.second;
        // Read back from the running configuration, never from the request: this is
        // the only record that proves which values the axle solver is using.
        logEffectiveSetup(car, "apply");
    }
    GfLogInfo("Rust garage transaction: applied=%d fields=%u error=%s\n", result ? 1 : 0,
        (unsigned)values.size(), error.c_str());
    return result;
}
