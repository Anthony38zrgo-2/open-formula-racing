/*
    Phase 0 integration bench for the vertical authority handoff.

    This bench reproduces, with the production functions, the interaction that
    the handoff describes:

        publish vertical state -> update horizontal position -> collide Z

    It does not replace the coupled seven-degree-of-freedom bench. Its purpose
    is to make the current double author visible and measurable before the
    implementation changes the ownership of the vertical state.
*/

#include "sim.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

tdble SimDeltaTime = 0.001f;
float SimAirPressure = 101325.0f;
float SimAirDensity = 1.225f;
double Tair = 298.15;
tdble Ttrack = 298.15f;
tdble SimRain = 0.0f;
int SimTelemetry = -1;
tCar *SimCarTable = nullptr;

static int checks = 0;
static int failures = 0;

static void Check(bool condition, const char* name)
{
    checks += 1;
    if (!condition)
    {
        failures += 1;
        std::printf("FAIL %s\n", name);
    }
    else
    {
        std::printf("PASS %s\n", name);
    }
}

static void CheckClose(double actual, double expected, double tolerance, const char* name)
{
    checks += 1;
    if (!std::isfinite(actual) || !std::isfinite(expected)
        || std::fabs(actual - expected) > tolerance)
    {
        failures += 1;
        std::printf("FAIL %s: actual=%.12f expected=%.12f tolerance=%.12f\n",
            name, actual, expected, tolerance);
    }
    else
    {
        std::printf("PASS %s: %.12f\n", name, actual);
    }
}

static bool SameState(const TireVerticalSolverState& left,
    const TireVerticalSolverState& right)
{
    for (int body = 0; body < 3; ++body)
    {
        if (left.body_position[body] != right.body_position[body]
            || left.body_velocity[body] != right.body_velocity[body])
        {
            return false;
        }
    }
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        if (left.suspension_extension_meters[wheel]
                != right.suspension_extension_meters[wheel]
            || left.suspension_extension_velocity_meters_per_second[wheel]
                != right.suspension_extension_velocity_meters_per_second[wheel])
        {
            return false;
        }
    }
    return true;
}

struct Snapshot
{
    TireVerticalSolverState state;
    tDynPt dyn_gc;
    tDynPt dyn_gc_g;
    double ride_height[4];
    double packers[4];
    int wheel_state[4];
};

static void TakeSnapshot(const tCar& car, Snapshot& snapshot)
{
    snapshot.state = car.formula_vertical_state;
    snapshot.dyn_gc = car.DynGC;
    snapshot.dyn_gc_g = car.DynGCg;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        snapshot.ride_height[wheel] = car.wheel[wheel].rideHeight;
        snapshot.packers[wheel] = car.wheel[wheel].susp.spring.packers;
        snapshot.wheel_state[wheel] = car.wheel[wheel].state;
    }
}

static void PrintSnapshot(const char* label, const Snapshot& snapshot)
{
    std::printf("%s state body=%.9f,%.9f,%.9f vel=%.9f,%.9f,%.9f "
        "dyn_gc_z=%.9f dyn_gc_vz=%.9f dyn_gc_g_z=%.9f dyn_gc_g_vz=%.9f "
        "ride=%.9f,%.9f,%.9f,%.9f packers=%.9f,%.9f,%.9f,%.9f "
        "flags=%d,%d,%d,%d\n",
        label,
        snapshot.state.body_position[0], snapshot.state.body_position[1],
        snapshot.state.body_position[2], snapshot.state.body_velocity[0],
        snapshot.state.body_velocity[1], snapshot.state.body_velocity[2],
        snapshot.dyn_gc.pos.z, snapshot.dyn_gc.vel.z,
        snapshot.dyn_gc_g.pos.z, snapshot.dyn_gc_g.vel.z,
        snapshot.ride_height[0], snapshot.ride_height[1],
        snapshot.ride_height[2], snapshot.ride_height[3],
        snapshot.packers[0], snapshot.packers[1],
        snapshot.packers[2], snapshot.packers[3],
        snapshot.wheel_state[0], snapshot.wheel_state[1],
        snapshot.wheel_state[2], snapshot.wheel_state[3]);
}

static void ConfigureFlatSegment(tTrackSeg& segment, tTrackSurface& surface)
{
    std::memset(&segment, 0, sizeof(segment));
    std::memset(&surface, 0, sizeof(surface));

    surface.kFriction = 1.0f;
    surface.kRollRes = 0.0f;
    surface.kRebound = 1.0f;
    surface.kDammage = 0.0f;

    segment.type = TR_STR;
    segment.type2 = TR_MAIN;
    segment.style = TR_PLAN;
    segment.length = 1000.0f;
    segment.width = 20.0f;
    segment.startWidth = 20.0f;
    segment.endWidth = 20.0f;
    segment.cos = 1.0f;
    segment.sin = 0.0f;
    segment.radius = 0.0f;
    segment.surface = &surface;
    segment.vertex[TR_SL].x = 0.0f;
    segment.vertex[TR_SL].y = 10.0f;
    segment.vertex[TR_SL].z = 0.0f;
    segment.vertex[TR_SR].x = 0.0f;
    segment.vertex[TR_SR].y = -10.0f;
    segment.vertex[TR_SR].z = 0.0f;
    segment.vertex[TR_EL].x = 1000.0f;
    segment.vertex[TR_EL].y = 10.0f;
    segment.vertex[TR_EL].z = 0.0f;
    segment.vertex[TR_ER].x = 1000.0f;
    segment.vertex[TR_ER].y = -10.0f;
    segment.vertex[TR_ER].z = 0.0f;
    segment.next = &segment;
    segment.prev = &segment;
}

static bool ConfigureFixture(const char* path, tCar& car, tCarElt& vehicle,
    void*& parameters)
{
    parameters = GfParmReadFile(path, GFPARM_RMODE_STD);
    if (parameters == nullptr)
    {
        std::printf("FAIL cannot read %s\n", path);
        return false;
    }
    std::printf("Fixture: car data loaded\n");

    const char* wheel_sections[4] = {
        "Front Right Wheel", "Front Left Wheel", "Rear Right Wheel", "Rear Left Wheel"
    };
    const char* suspension_sections[4] = {
        "Front Right Suspension", "Front Left Suspension",
        "Rear Right Suspension", "Rear Left Suspension"
    };

    car.params = parameters;
    car.carElt = &vehicle;
    car.options = new SimulationOptions;
    car.options->SetFromSkill(ROOKIE);

    vehicle.index = 0;
    vehicle._skillLevel = ROOKIE;
    vehicle._state = 0;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        vehicle.setup.rideHeight[wheel].value =
            (tdble)GfParmGetNum(parameters, wheel_sections[wheel],
                "ride height", nullptr, 0.05);
        vehicle.setup.rideHeight[wheel].desired_value =
            vehicle.setup.rideHeight[wheel].value;
    }

    const double mass = GfParmGetNum(parameters, "Car", "mass", nullptr, 0.0);
    const double initial_fuel = GfParmGetNum(parameters, "Car",
        "initial fuel", nullptr, 0.0);
    const double fuel_mass = GfParmGetNum(parameters, "Car",
        "fuel mass mult", nullptr, 1.0);
    const double body_length = GfParmGetNum(parameters, "Car",
        "body length", nullptr, 4.0);
    const double body_width = GfParmGetNum(parameters, "Car",
        "body width", nullptr, 1.8);
    const double body_height = GfParmGetNum(parameters, "Car",
        "body height", nullptr, 1.0);
    const double overall_width = GfParmGetNum(parameters, "Car",
        "overall width", nullptr, body_width);
    const double repartition = GfParmGetNum(parameters, "Car",
        "front-rear weight repartition", nullptr, 0.5);
    const double left_repartition = GfParmGetNum(parameters, "Car",
        "front right-left weight repartition", nullptr, 0.5);
    const double rear_left_repartition = GfParmGetNum(parameters, "Car",
        "rear right-left weight repartition", nullptr, 0.5);
    const double combined_left = repartition * left_repartition
        + (1.0 - repartition) * rear_left_repartition;
    const double inertia_scale = GfParmGetNum(parameters, "Car",
        "mass repartition coefficient", nullptr, 1.0);

    car.dimension.x = (tdble)body_length;
    car.dimension.y = (tdble)body_width;
    car.dimension.z = (tdble)body_height;
    car.mass = (tdble)mass;
    car.Minv = (tdble)(1.0 / mass);
    car.tank = (tdble)GfParmGetNum(parameters, "Car", "fuel tank", nullptr, 80.0);
    car.fuel = (tdble)initial_fuel;
    car.fuelMass = (tdble)fuel_mass;
    car.statGC.z = (tdble)GfParmGetNum(parameters, "Car", "GC height", nullptr, 0.5);
    car.statGC.y = (tdble)(-combined_left * car.dimension.y + car.dimension.y * 0.5);

    const double scale = inertia_scale * inertia_scale;
    const double roll_inertia = mass * scale
        * (body_width * body_width + body_height * body_height) / 12.0;
    const double pitch_inertia = mass * scale
        * (body_length * body_length + body_height * body_height) / 12.0;
    const double yaw_inertia = mass * scale
        * (body_width * body_width + body_length * body_length) / 12.0;
    car.Iinv.x = (tdble)(1.0 / roll_inertia);
    car.Iinv.y = (tdble)(1.0 / pitch_inertia);
    car.Iinv.z = (tdble)(1.0 / yaw_inertia);

    const double front_axle_x = GfParmGetNum(parameters, "Front Axle",
        "xpos", nullptr, 1.5);
    const double rear_axle_x = GfParmGetNum(parameters, "Rear Axle",
        "xpos", nullptr, -1.5);

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        tWheel& native_wheel = car.wheel[wheel];
        const double axle_x = wheel < 2 ? front_axle_x : rear_axle_x;
        native_wheel.relPos.x = (tdble)axle_x;
        native_wheel.relPos.y = (tdble)GfParmGetNum(parameters,
            wheel_sections[wheel], "ypos", nullptr, 0.0);
        native_wheel.staticPos.x = native_wheel.relPos.x;
        native_wheel.staticPos.y = native_wheel.relPos.y;
    }

    car.statGC.x = (tdble)(car.wheel[FRNT_RGT].relPos.x * repartition
        + car.wheel[REAR_RGT].relPos.x * (1.0 - repartition));

    const double front_share = repartition;
    double radii[4] = {};
    double widths[4] = {};
    double masses[4] = {};
    double static_loads[4] = {};

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        tWheel& native_wheel = car.wheel[wheel];
        native_wheel.staticPos.x -= car.statGC.x;
        native_wheel.staticPos.y -= car.statGC.y;

        const double rim_diameter = GfParmGetNum(parameters,
            wheel_sections[wheel], "rim diameter", nullptr, 0.33);
        const double tire_width = GfParmGetNum(parameters,
            wheel_sections[wheel], "tire width", nullptr, 0.3);
        const double tire_height = GfParmGetNum(parameters,
            wheel_sections[wheel], "tire height", nullptr, -1.0);
        const double tire_ratio = GfParmGetNum(parameters,
            wheel_sections[wheel], "tire height-width ratio", nullptr, 0.45);

        widths[wheel] = tire_width;
        radii[wheel] = tire_height > 0.0
            ? rim_diameter * 0.5 + tire_height
            : rim_diameter * 0.5 + tire_width * tire_ratio;
        masses[wheel] = GfParmGetNum(parameters, wheel_sections[wheel],
            "mass", nullptr, 12.0);
        static_loads[wheel] = mass * 9.81
            * (wheel < 2 ? front_share : 1.0 - front_share) * 0.5;

        native_wheel.radius = (tdble)radii[wheel];
        native_wheel.mass = (tdble)masses[wheel];
        native_wheel.weight0 = (tdble)static_loads[wheel];

        SimSuspConfig(&car, parameters, suspension_sections[wheel],
            &native_wheel.susp, wheel);
        SimSuspReConfig(&car, &native_wheel.susp, wheel, native_wheel.weight0,
            vehicle.setup.rideHeight[wheel].value);
        native_wheel.susp.x = native_wheel.susp.spring.x0;
    }

    for (int axle = 0; axle < 2; ++axle)
    {
        SimAxleConfig(&car, axle);
        SimAxleReConfig(&car, axle, 0.0);
    }

    TireModelConfiguration tire_configuration;
    TireModelConfigurationSetDefaults(tire_configuration);
    const bool tire_valid = TireModelConfigurationRead(tire_configuration,
        parameters, static_loads, radii, widths, masses, 101325.0, 298.15);
    std::printf("Fixture: tire configuration read=%d enabled=%d\n",
        tire_valid ? 1 : 0, tire_configuration.enabled ? 1 : 0);
    car.formula_tire_configuration = tire_configuration;
    car.formula_tire_model_enabled =
        (tire_valid && tire_configuration.enabled) ? 1 : 0;

    car.formula_vertical_authority_diagnostic = 0;
    car.formula_vertical_authority_every_step = 0;
    car.formula_vertical_time = 1.0;
    car.formula_vertical_solver_initialized = 0;

    std::printf("Fixture: mass=%.6f fuel=%.6f fuel_mass=%.6f statGC=%.9f,%.9f,%.9f "
        "wheelbase=%.6f track=%.6f tire_model=%d\n",
        car.mass, car.fuel, car.fuelMass,
        car.statGC.x, car.statGC.y, car.statGC.z,
        (car.wheel[FRNT_RGT].relPos.x - car.wheel[REAR_RGT].relPos.x),
        (car.wheel[FRNT_LFT].relPos.y - car.wheel[FRNT_RGT].relPos.y),
        car.formula_tire_model_enabled);
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        std::printf("Wheel %d: static=%.9f,%.9f radius=%.9f mass=%.6f load=%.6f "
            "spring=%.6f preload=%.6f x0=%.9f packers=%.9f xmax=%.9f bellcrank=%.9f\n",
            wheel, car.wheel[wheel].staticPos.x, car.wheel[wheel].staticPos.y,
            car.wheel[wheel].radius, car.wheel[wheel].mass,
            car.wheel[wheel].weight0, car.wheel[wheel].susp.spring.K,
            car.wheel[wheel].susp.spring.F0, car.wheel[wheel].susp.spring.x0,
            car.wheel[wheel].susp.spring.packers, car.wheel[wheel].susp.spring.xMax,
            car.wheel[wheel].susp.spring.bellcrank);
    }
    (void)overall_width;
    return true;
}

static void PrepareWheelTrackState(tCar& car, tTrackSeg& segment)
{
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        tWheel& native_wheel = car.wheel[wheel];
        native_wheel.trkPos.seg = &segment;
        native_wheel.trkPos.toStart = 0.0f;
        native_wheel.trkPos.toRight = native_wheel.staticPos.y;
        native_wheel.trkPos.type = TR_LPOS_MAIN;
        native_wheel.ground_mesh_contact = false;
        native_wheel.zRoad = 0.0f;
    }
    car.trkPos.seg = &segment;
    car.trkPos.toStart = 0.0f;
    car.trkPos.toRight = 0.0f;
    car.trkPos.type = TR_LPOS_MAIN;
}

static TireVerticalSolverState BuildCompressedState(const tCar& car,
    const TireVerticalSolverState& equilibrium, double body_height,
    double body_velocity)
{
    TireVerticalSolverState state = equilibrium;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        const double bellcrank = car.wheel[wheel].susp.spring.bellcrank;
        state.suspension_extension_meters[wheel] =
            car.wheel[wheel].susp.spring.packers / bellcrank;
        state.suspension_extension_velocity_meters_per_second[wheel] = 0.0;
    }
    state.body_position[0] = body_height;
    state.body_position[1] = 0.0;
    state.body_position[2] = 0.0;
    state.body_velocity[0] = body_velocity;
    state.body_velocity[1] = 0.0;
    state.body_velocity[2] = 0.0;
    return state;
}

int main(int argument_count, char** arguments)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* default_path =
        "implementation/candidates/williams-nineteen-ninety-seven/"
        "williams-nineteen-ninety-seven.xml";
    const char* path = argument_count > 1 ? arguments[1] : default_path;

    GfInit();
    GfLogSetLevelThreshold(GfLogger::eWarning);

    std::printf("Vertical authority phase 0 bench\n");
    std::printf("Car data: %s\n", path);

    tCar* car_storage = new tCar();
    tCarElt* vehicle_storage = new tCarElt();
    tCar& car = *car_storage;
    tCarElt& vehicle = *vehicle_storage;
    void* parameters = nullptr;
    if (!ConfigureFixture(path, car, vehicle, parameters))
    {
        return 1;
    }

    TireVerticalSolverConfiguration configuration;
    SimCarBuildVerticalConfiguration(&car, configuration);

    const double floor_point_count = GfParmGetNum(parameters, "Formula Floor",
        "contact count", nullptr, -1.0);
    const double floor_point0_x = GfParmGetNum(parameters, "Formula Floor Point 0",
        "x", nullptr, -999.0);
    const double floor_point0_y = GfParmGetNum(parameters, "Formula Floor Point 0",
        "y", nullptr, -999.0);
    const double floor_point0_z = GfParmGetNum(parameters, "Formula Floor Point 0",
        "z", nullptr, -999.0);
    CheckClose(floor_point_count, 14.0, 0.0,
        "Formula Floor XML exposes 14 points");
    CheckClose(floor_point0_x, 1.306958, 1.0e-6,
        "Formula Floor XML exposes point 0 x");
    CheckClose(floor_point0_y, 0.0, 1.0e-9,
        "Formula Floor XML exposes point 0 y");
    CheckClose(floor_point0_z, -0.02510, 1.0e-6,
        "Formula Floor XML exposes point 0 z");


    double ground_height[4] = {};
    double ground_velocity[4] = {};
    TireVerticalSolverState equilibrium = {};
    TireVerticalSolverDiagnostics equilibrium_diagnostics = {};
    const bool equilibrium_ok = TireVerticalSolverInitializeEquilibrium(
        configuration, ground_height, equilibrium, equilibrium_diagnostics);
    Check(equilibrium_ok, "equilibrium initialization");
    if (!equilibrium_ok)
    {
        return 1;
    }

    for (int wheel = 0; wheel < 4; ++wheel)
    {
        equilibrium_diagnostics.tire_contact[wheel] = 1;
        equilibrium_diagnostics.tire_normal_force_newtons[wheel] =
            car.wheel[wheel].weight0;
    }

    car.formula_vertical_state = equilibrium;
    car.formula_vertical_solver_initialized = 1;
    car.formula_vertical_time = 1.0;

    tTrackSeg segment = {};
    tTrackSurface surface = {};
    ConfigureFlatSegment(segment, surface);
    PrepareWheelTrackState(car, segment);

    const double body_height = 0.010;
    const double body_velocity = -3.0;
    TireVerticalSolverState compressed = BuildCompressedState(car, equilibrium,
        body_height, body_velocity);
    car.DynGCg.pos.z = compressed.body_position[0] + car.statGC.z;
    car.DynGCg.pos.x = 0.0;
    car.DynGCg.pos.y = 0.0;
    car.DynGCg.vel.x = 0.0;
    car.DynGCg.vel.y = 0.0;
    car.DynGCg.vel.z = (tdble)body_velocity;
    car.DynGC.pos.z = car.DynGCg.pos.z;
    car.DynGC.vel.z = (tdble)body_velocity;

    SimCarVerticalPublish(&car, compressed, compressed,
        equilibrium_diagnostics, ground_height, ground_velocity, 0.0);

    Snapshot published;
    TakeSnapshot(car, published);
    PrintSnapshot("Published", published);

    Check(published.ride_height[0] < published.packers[0],
        "published front ride height is below the packer");
    Check(published.ride_height[2] < published.packers[2],
        "published rear ride height is below the packer");
    Check((published.wheel_state[0] & SIM_SUSP_COMP) != 0,
        "front suspension reports the compression flag");
    Check((published.wheel_state[2] & SIM_SUSP_COMP) != 0,
        "rear suspension reports the compression flag");

    SimCarUpdatePos(&car);
    Snapshot positioned;
    TakeSnapshot(car, positioned);
    PrintSnapshot("Positioned", positioned);
    CheckClose(positioned.dyn_gc_g.pos.z, published.dyn_gc_g.pos.z, 1.0e-12,
        "horizontal position update does not change published vertical position");
    CheckClose(positioned.dyn_gc.pos.z, published.dyn_gc_g.pos.z, 1.0e-12,
        "horizontal position update copies the published vertical position to DynGC");

    SimCarCollideZ(&car);
    Snapshot collided;
    TakeSnapshot(car, collided);
    PrintSnapshot("Collided", collided);

    Check(SameState(collided.state, published.state),
        "collision does not modify formula_vertical_state");
    CheckClose(collided.dyn_gc_g.pos.z - published.dyn_gc_g.pos.z, 0.0,
        1.0e-6, "collision never projects the published height");
    CheckClose(collided.dyn_gc.pos.z, published.dyn_gc.pos.z, 1.0e-12,
        "collision leaves DynGC.pos.z at its pre-collision value");
    CheckClose(collided.dyn_gc_g.vel.z, body_velocity, 1.0e-6,
        "collision never reflects the published vertical velocity");
    CheckClose(collided.dyn_gc.vel.z, body_velocity, 1.0e-9,
        "collision leaves DynGC.vel.z at its pre-collision value");
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        char name[128] = {};
        std::snprintf(name, sizeof(name),
            "collision preserves authoritative rideHeight for wheel %d", wheel);
        CheckClose(collided.ride_height[wheel], published.ride_height[wheel],
            1.0e-12, name);
    }
    CheckClose(collided.dyn_gc.pos.z, collided.dyn_gc_g.pos.z, 1.0e-6,
        "collision preserves agreement between vehicle views");

    SimCarVerticalPublish(&car, compressed, compressed,
        equilibrium_diagnostics, ground_height, ground_velocity, 0.0);
    Snapshot republished;
    TakeSnapshot(car, republished);
    PrintSnapshot("Republished", republished);

    CheckClose(republished.dyn_gc_g.pos.z, published.dyn_gc_g.pos.z, 1.0e-12,
        "next publish discards the collision position correction");
    CheckClose(republished.dyn_gc_g.vel.z, body_velocity, 1.0e-12,
        "next publish discards the collision velocity correction");
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        char name[128] = {};
        std::snprintf(name, sizeof(name),
            "next publish restores the published rideHeight for wheel %d", wheel);
        CheckClose(republished.ride_height[wheel], published.ride_height[wheel],
            1.0e-12, name);
    }

    TireVerticalSolverState clamped = equilibrium;
    clamped.body_position[1] = 1.2;
    clamped.body_velocity[1] = 0.5;
    clamped.body_position[2] = -1.2;
    clamped.body_velocity[2] = -0.5;
    car.DynGCg.vel.x = 0.0;
    car.DynGCg.vel.y = 0.0;
    SimCarVerticalPublish(&car, clamped, clamped, equilibrium_diagnostics,
        ground_height, ground_velocity, 0.0);

    Snapshot unclamped;
    TakeSnapshot(car, unclamped);
    PrintSnapshot("Unclamped", unclamped);
    CheckClose(unclamped.dyn_gc_g.pos.ax, 1.2, 1.0e-6,
        "publish exposes the roll beyond one radian");
    CheckClose(unclamped.dyn_gc_g.pos.ay, -1.2, 1.0e-6,
        "publish exposes the pitch beyond one radian");

    SimCarUpdatePos(&car);
    Snapshot clamped_view;
    TakeSnapshot(car, clamped_view);
    PrintSnapshot("ClampedView", clamped_view);
    CheckClose(clamped_view.dyn_gc_g.pos.ax, 1.2, 1.0e-6,
        "position update preserves authoritative roll");
    CheckClose(clamped_view.dyn_gc_g.vel.ax, 0.5, 1.0e-12,
        "position update preserves authoritative roll rate");
    CheckClose(clamped_view.dyn_gc_g.pos.ay, -1.2, 1.0e-6,
        "position update preserves authoritative pitch");
    CheckClose(clamped_view.state.body_position[1], 1.2, 1.0e-12,
        "external clamp leaves the authoritative roll at 1.2");
    CheckClose(clamped_view.state.body_velocity[1], 0.5, 1.0e-12,
        "external clamp leaves the authoritative roll rate at 0.5");

    SimCarVerticalPublish(&car, clamped, clamped, equilibrium_diagnostics,
        ground_height, ground_velocity, 0.0);
    Snapshot reclamped;
    TakeSnapshot(car, reclamped);
    PrintSnapshot("Reclamped", reclamped);
    CheckClose(reclamped.dyn_gc_g.pos.ax, 1.2, 1.0e-6,
        "next publish restores the roll view beyond one radian");
    CheckClose(reclamped.dyn_gc_g.vel.ax, 0.5, 1.0e-12,
        "next publish restores the roll rate beyond one radian");

    TireVerticalSolverConfiguration contact_configuration = configuration;
    contact_configuration.chassis_contact_count = 1;
    contact_configuration.chassis_contact[0].x_meters = 0.0;
    contact_configuration.chassis_contact[0].y_meters = 0.0;
    contact_configuration.chassis_contact[0].z_meters = -0.025;
    contact_configuration.chassis_contact[0].group = 0;
    contact_configuration.chassis_contact[0].enabled = 1;

    TireVerticalSolverState contact_state = equilibrium;
    contact_state.body_position[0] = 0.026;
    contact_state.body_position[1] = 0.0;
    contact_state.body_position[2] = 0.0;
    contact_state.body_velocity[0] = -3.0;
    contact_state.body_velocity[1] = 0.0;
    contact_state.body_velocity[2] = 0.0;

    TireVerticalSolverStepInput contact_input = {};
    contact_input.elapsed_seconds = 0.002;
    for (int wheel = 0; wheel < 4; ++wheel)
    {
        contact_input.ground_height_meters[wheel] = -1.0;
        contact_input.ground_height_velocity_meters_per_second[wheel] = 0.0;
    }
    contact_input.chassis_ground_height_meters[0] = 0.0;
    contact_input.chassis_ground_height_velocity_meters_per_second[0] = 0.0;

    TireVerticalSolverState contact_next = {};
    TireVerticalSolverDiagnostics contact_diagnostics = {};
    const bool contact_ok = TireVerticalSolverStep(contact_configuration,
        contact_state, contact_input, contact_next, contact_diagnostics);
    Check(contact_ok, "solver chassis contact step converges");
    Check(contact_diagnostics.chassis_contact_active[0] == 1,
        "solver chassis contact is active");
    Check(contact_diagnostics.chassis_contact_gap_meters[0] >= -1.0e-5,
        "solver chassis contact does not penetrate");
    Check(contact_next.body_position[0] >= 0.025 - 1.0e-5,
        "solver chassis contact raises the body reference");
    Check(contact_diagnostics.chassis_contact_impulse_newton_seconds[0] > 0.0,
        "solver chassis contact produces a positive impulse");
    Check(contact_diagnostics.chassis_contact_energy_loss_joules > 0.0,
        "solver chassis contact dissipates energy");

    TireVerticalSolverConfiguration pair_configuration = configuration;
    pair_configuration.chassis_contact_count = 2;
    for (int contact = 0; contact < 2; ++contact)
    {
        pair_configuration.chassis_contact[contact].x_meters = 0.0;
        pair_configuration.chassis_contact[contact].y_meters =
            contact == 0 ? 0.5 : -0.5;
        pair_configuration.chassis_contact[contact].z_meters = -0.02;
        pair_configuration.chassis_contact[contact].group = 0;
        pair_configuration.chassis_contact[contact].enabled = 1;
    }

    TireVerticalSolverState pair_state = equilibrium;
    pair_state.body_position[0] = 0.021;
    pair_state.body_position[1] = 0.0;
    pair_state.body_position[2] = 0.0;
    pair_state.body_velocity[0] = -2.0;
    pair_state.body_velocity[1] = 0.0;
    pair_state.body_velocity[2] = 0.0;
    TireVerticalSolverStepInput pair_input = {};
    pair_input.elapsed_seconds = 0.002;
    for (double& ground : pair_input.ground_height_meters) ground = -1.0;
    TireVerticalSolverState pair_next = {};
    TireVerticalSolverDiagnostics pair_diagnostics = {};
    const bool pair_ok = TireVerticalSolverStep(pair_configuration, pair_state,
        pair_input, pair_next, pair_diagnostics);
    Check(pair_ok, "solver paired chassis contacts converge");
    Check(pair_diagnostics.chassis_contact_active[0] == 1
        && pair_diagnostics.chassis_contact_active[1] == 1,
        "solver paired chassis contacts are active");
    CheckClose(pair_diagnostics.chassis_contact_impulse_newton_seconds[0],
        pair_diagnostics.chassis_contact_impulse_newton_seconds[1], 1.0e-6,
        "symmetric paired chassis contacts produce equal impulses");
    CheckClose(pair_next.body_velocity[1], 0.0, 1.0e-6,
        "symmetric paired chassis contacts produce no roll rate");

    TireVerticalSolverState rolled_state = pair_state;
    rolled_state.body_position[0] = 0.031;
    rolled_state.body_position[1] = -0.02;
    TireVerticalSolverState rolled_next = {};
    TireVerticalSolverDiagnostics rolled_diagnostics = {};
    const bool rolled_ok = TireVerticalSolverStep(pair_configuration, rolled_state,
        pair_input, rolled_next, rolled_diagnostics);
    Check(rolled_ok, "solver rolled chassis contacts converge");
    Check(rolled_diagnostics.chassis_contact_impulse_newton_seconds[0]
        > rolled_diagnostics.chassis_contact_impulse_newton_seconds[1],
        "left chassis contact carries more impulse when the left side is low");
    Check(rolled_next.body_velocity[1] > 0.0,
        "left chassis contact produces a positive roll correction");

    TireVerticalSolverConfiguration pitch_configuration = configuration;
    pitch_configuration.chassis_contact_count = 2;
    for (int contact = 0; contact < 2; ++contact)
    {
        pitch_configuration.chassis_contact[contact].x_meters =
            contact == 0 ? 1.0 : -1.0;
        pitch_configuration.chassis_contact[contact].y_meters = 0.0;
        pitch_configuration.chassis_contact[contact].z_meters = -0.05;
        pitch_configuration.chassis_contact[contact].group = 0;
        pitch_configuration.chassis_contact[contact].enabled = 1;
    }

    TireVerticalSolverState pitched_state = equilibrium;
    pitched_state.body_position[0] = 0.071;
    pitched_state.body_position[1] = 0.0;
    pitched_state.body_position[2] = 0.02;
    pitched_state.body_velocity[0] = -2.0;
    pitched_state.body_velocity[1] = 0.0;
    pitched_state.body_velocity[2] = 0.0;
    TireVerticalSolverState pitched_next = {};
    TireVerticalSolverDiagnostics pitched_diagnostics = {};
    const bool pitched_ok = TireVerticalSolverStep(pitch_configuration, pitched_state,
        pair_input, pitched_next, pitched_diagnostics);
    Check(pitched_ok, "solver pitched chassis contacts converge");
    Check(pitched_diagnostics.chassis_contact_impulse_newton_seconds[0]
        > pitched_diagnostics.chassis_contact_impulse_newton_seconds[1],
        "front chassis contact carries more impulse when the nose is down");
    Check(pitched_next.body_velocity[2] < 0.0,
        "front chassis contact produces a negative pitch correction");

    TireVerticalSolverConfiguration level_two = configuration;
    level_two.chassis_contact_count = 14;
    for (int contact = 0; contact < level_two.chassis_contact_count; ++contact)
    {
        char section[48] = {};
        std::snprintf(section, sizeof(section), "Formula Floor Point %d", contact);
        TireVerticalChassisContactConfiguration& chassis_contact =
            level_two.chassis_contact[contact];
        chassis_contact.x_meters = GfParmGetNum(parameters, section, "x", nullptr, 0.0);
        chassis_contact.y_meters = GfParmGetNum(parameters, section, "y", nullptr, 0.0);
        chassis_contact.z_meters = GfParmGetNum(parameters, section, "z", nullptr, 0.0);
        chassis_contact.group = std::strcmp(GfParmGetStr(parameters, section,
            "group", "floor"), "front_wing") == 0 ? 1 : 0;
        chassis_contact.enabled = GfParmGetNum(parameters, section, "enabled",
            nullptr, 0.0) > 0.0;
    }

    TireVerticalSolverState level_two_state = equilibrium;
    level_two_state.body_position[0] = 0.120;
    level_two_state.body_position[1] = 0.010;
    level_two_state.body_position[2] = -0.010;
    level_two_state.body_velocity[0] = -2.0;
    level_two_state.body_velocity[1] = 0.0;
    level_two_state.body_velocity[2] = 0.0;
    bool level_two_ok = true;
    int level_two_first_active = 0;
    double level_two_first_loss = 0.0;
    int level_two_first_constraints = 0;
    bool level_two_captured = false;
    for (int step = 0; step < 50; ++step)
    {
        TireVerticalSolverStepInput level_two_input = {};
        level_two_input.elapsed_seconds = 0.002;
        for (int wheel = 0; wheel < 4; ++wheel)
        {
            level_two_input.ground_height_meters[wheel] = -0.5;
            level_two_input.ground_height_velocity_meters_per_second[wheel] = 0.0;
        }
        for (int contact = 0;
            contact < level_two.chassis_contact_count; ++contact)
        {
            level_two_input.chassis_ground_height_meters[contact] = 0.0;
            level_two_input.chassis_ground_height_velocity_meters_per_second[contact] = 0.0;
        }
        TireVerticalSolverState level_two_next = {};
        TireVerticalSolverDiagnostics level_two_diagnostics = {};
        if (!TireVerticalSolverStep(level_two, level_two_state, level_two_input,
                level_two_next, level_two_diagnostics))
        {
            std::printf("level2 step %d failed: residual=%.9g iterations=%d reason=%d active=%d\n",
                step, level_two_diagnostics.residual_norm,
                level_two_diagnostics.newton_iterations, level_two_diagnostics.failure_reason,
                level_two_diagnostics.active_constraint_count);
            level_two_ok = false;
            for (int i = 0; i < level_two_diagnostics.active_constraint_count; ++i)
                std::printf("failed active kind=%d index=%d impulse=%.9g\n",
                    level_two_diagnostics.active_constraint_kind[i],
                    level_two_diagnostics.active_constraint_index[i],
                    level_two_diagnostics.constraint_impulse_newton_seconds[i]);
            break;
        }
        for (int contact = 0;
            contact < level_two.chassis_contact_count; ++contact)
        {
            if (level_two.chassis_contact[contact].enabled
                && level_two_diagnostics.chassis_contact_gap_meters[contact]
                    < -1.0e-7)
            {
                level_two_ok = false;
            }
        }
        int active_now = 0;
        for (int contact = 0; contact < level_two.chassis_contact_count; ++contact)
        {
            active_now += level_two_diagnostics.chassis_contact_active[contact] ? 1 : 0;
        }
        if (!level_two_captured && active_now > 0)
        {
            level_two_captured = true;
            level_two_first_active = active_now;
            level_two_first_loss =
                level_two_diagnostics.chassis_contact_energy_loss_joules;
            level_two_first_constraints =
                level_two_diagnostics.active_constraint_count;
        }
        level_two_state = level_two_next;
    }
    Check(level_two_ok, "level 2 chassis contacts remain converged for 50 steps");
    Check(level_two_first_active > 0,
        "level 2 chassis contacts activate at least one point");
    Check(level_two_first_loss > 0.0,
        "level 2 chassis contacts dissipate energy");
    Check(level_two_first_constraints <= TIRE_VERTICAL_MAX_ACTIVE_CONSTRAINTS,
        "level 2 chassis contacts remain within capacity");

    // Exercise the production whole-car transaction. None of the transmission
    // fixtures are needed: a failed vertical step must never reach them.
    car.ctrl = &vehicle.ctrl;
    car.formula_floor_contact_count = 0;
    car.formula_vertical_external_moment_source = 0;
    car.formula_vertical_failed = 0;
    car.formula_vertical_solver_initialized = 1;
    car.formula_vertical_substep_seconds = 0.0005;
    car.DynGCg.vel.x = car.DynGCg.vel.y = 0.0;
    car.DynGC.vel.x = car.DynGC.vel.y = 0.0;
    SimCarVerticalPublish(&car, equilibrium, equilibrium, equilibrium_diagnostics,
        ground_height, ground_velocity, 0.0);
    vehicle.pub.DynGC = car.DynGC;
    vehicle.pub.DynGCg = car.DynGCg;
    SimDeltaTime = 0.002f;
    tSituation situation = {};
    const tdble x_before = car.DynGCg.pos.x;
    const tdble spin_before = car.wheel[0].spinVel;
    const tdble ride_before = car.wheel[0].rideHeight;
    car.formula_vertical_state.body_position[0] -= 0.01;
    Check(!SimCarPhysicsStep(&car, &situation), "whole-car step rejects inconsistent initial contact");
    Check(car.formula_vertical_failed != 0, "whole-car failure stays visible");
    Check(car.DynGCg.pos.x == x_before && car.wheel[0].spinVel == spin_before,
        "failure cannot advance position or wheel rotation");
    Check(car.wheel[0].rideHeight == ride_before, "failure restores wheel views");
    Check(!SimCarPhysicsStep(&car, &situation), "next step stays frozen after failure");

    car.formula_vertical_failed = 0;
    SimCarVerticalPublish(&car, equilibrium, equilibrium, equilibrium_diagnostics,
        ground_height, ground_velocity, 0.0);
    vehicle.pub.DynGC = car.DynGC;
    vehicle.pub.DynGCg = car.DynGCg;
    car.DynGCg.vel.z = std::numeric_limits<tdble>::quiet_NaN();
    Check(!SimCarPhysicsStep(&car, &situation), "whole-car step rejects nonfinite dynamics before aero");
    Check(std::isfinite(car.DynGCg.vel.z), "nonfinite input cannot corrupt published dynamics");

    car.formula_vertical_failed = 0;
    SimCarVerticalPublish(&car, equilibrium, equilibrium, equilibrium_diagnostics,
        ground_height, ground_velocity, 0.0);
    SimCarVerticalCoupledUpdate(&car, &situation);
    Check(!car.formula_vertical_failed, "production vertical adapter accepts flat road");
    Check(car.formula_vertical_accepted_substeps == 4,
        "2ms frame respects configured 0.5ms vertical substeps");
    const double old_time = car.formula_vertical_time;
    SimCarVerticalCoupledUpdate(&car, &situation);
    Check(!car.formula_vertical_failed && car.formula_vertical_time > old_time,
        "production adapter advances repeated finite steps");
    for (int vertex = 0; vertex < 4; ++vertex)
        segment.vertex[vertex].z += 0.001f;
    SimCarVerticalCoupledUpdate(&car, &situation);
    Check(!car.formula_vertical_failed,
        "road sample interpolates from previous height instead of extrapolating twice");
    Check(TireVerticalStateIsFinite(car.formula_vertical_state),
        "changing road height keeps the accepted state finite");
    bool repeated_road_ok = true;
    double maximum_vertical_speed = 0.0;
    double maximum_load = 0.0;
    for (int step = 0; step < 5000; ++step)
    {
        const double road_height = 0.001 + 0.002 * std::sin(2.0 * 3.141592653589793
            * 8.0 * (step + 1) * static_cast<double>(SimDeltaTime));
        for (int vertex = 0; vertex < 4; ++vertex)
            segment.vertex[vertex].z = static_cast<tdble>(road_height);
        SimCarVerticalCoupledUpdate(&car, &situation);
        if (car.formula_vertical_failed || !TireVerticalStateIsFinite(car.formula_vertical_state))
        {
            repeated_road_ok = false;
            break;
        }
        maximum_vertical_speed = std::max(maximum_vertical_speed,
            std::fabs(car.formula_vertical_state.body_velocity[0]));
        for (const auto& wheel : car.wheel)
        {
            maximum_load = std::max(maximum_load, static_cast<double>(wheel.forces.z));
            repeated_road_ok = repeated_road_ok && std::isfinite(wheel.forces.z)
                && wheel.forces.z >= 0.0f;
        }
    }
    Check(repeated_road_ok, "ten seconds of 8Hz road excitation stays finite and admissible");
    Check(maximum_vertical_speed < 2.0, "small prescribed road excitation does not catapult the chassis");
    Check(maximum_load < 100000.0, "small prescribed road excitation has bounded physical loads");
    std::printf("road replay maximum_vz=%.9g maximum_N=%.9g\n", maximum_vertical_speed, maximum_load);

    std::printf("Summary: checks=%d failures=%d\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
