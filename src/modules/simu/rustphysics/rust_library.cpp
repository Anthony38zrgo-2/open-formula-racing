/*
    rust_library.cpp -- run-time loader for the Open Formula Rust physics DLL.
*/

#include "rust_library.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
# include <windows.h>
#else
# include <dlfcn.h>
#endif

RustPhysicsLibrary rustPhysicsLibrary;

RustPhysicsLibrary::RustPhysicsLibrary()
    : abiVersion(nullptr), abiSignature(nullptr), buildProvenance(nullptr), loadTrackGroundMesh(nullptr),
      trackGroundTriangles(nullptr), create(nullptr), destroy(nullptr), reset(nullptr),
      setPose(nullptr), step(nullptr), snapshot(nullptr), getLastError(nullptr),
      backendMask(nullptr), setupDocument(nullptr), setupFields(nullptr), fuelDensity(nullptr), vehicleSpecs(nullptr), applySetup(nullptr),
      resetSetup(nullptr), pitFuel(nullptr), pitTires(nullptr), vehicleGeometry(nullptr),
      traceShutdown(nullptr), traceConfigure(nullptr), traceDump(nullptr), traceBuffered(nullptr), effectiveSetup(nullptr),
      _module(nullptr)
{
}

RustPhysicsLibrary::~RustPhysicsLibrary()
{
    unload();
}

namespace
{
    void* openLibrary(const std::string& path, std::string& error)
    {
#ifdef _WIN32
        HMODULE module = ::LoadLibraryA(path.c_str());
        if (!module)
        {
            error = "LoadLibrary failed for " + path + " (error " +
                    std::to_string(static_cast<long long>(::GetLastError())) + ")";
        }
        return reinterpret_cast<void*>(module);
#else
        void* module = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!module)
            error = std::string("dlopen failed for ") + path + ": " + ::dlerror();
        return module;
#endif
    }

    void closeLibrary(void* module)
    {
        if (!module)
            return;
#ifdef _WIN32
        ::FreeLibrary(reinterpret_cast<HMODULE>(module));
#else
        ::dlclose(module);
#endif
    }

    void* resolve(void* module, const char* name)
    {
#ifdef _WIN32
        return reinterpret_cast<void*>(
            ::GetProcAddress(reinterpret_cast<HMODULE>(module), name));
#else
        return ::dlsym(module, name);
#endif
    }
}

namespace
{
    void mixLayout(std::uint64_t& hash, std::uint64_t value)
    {
        hash ^= value;
        hash *= 0x00000100000001b3ull;
    }

    //! Mirrors `ofp_abi_signature` in the Rust library: sizes and offsets that define
    //! the ABI. Equality proves both sides were compiled from the same declarations.
    std::uint64_t expectedAbiSignature()
    {
        std::uint64_t hash = 0xcbf29ce484222325ull;
        mixLayout(hash, (std::uint64_t)sizeof(OfpVehicleSpecs));
        mixLayout(hash, (std::uint64_t)offsetof(OfpVehicleSpecs, tire_width_m));
        mixLayout(hash, (std::uint64_t)offsetof(OfpVehicleSpecs, tread_optimal_temperature_c));
        mixLayout(hash, (std::uint64_t)sizeof(OfpSnapshot));
        mixLayout(hash, (std::uint64_t)sizeof(OfpWheelState));
        mixLayout(hash, (std::uint64_t)sizeof(OfpBodyContact));
        mixLayout(hash, (std::uint64_t)sizeof(OfpControlInput));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, wheels));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, floor_clearance));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, water_temperature_celsius));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, body_contact_impulse_ns));
        mixLayout(hash, (std::uint64_t)offsetof(OfpWheelState, tread_temperature_c));
        mixLayout(hash, (std::uint64_t)offsetof(OfpWheelState, contact_point));
        mixLayout(hash, (std::uint64_t)offsetof(OfpWheelState, ray_distance));
        mixLayout(hash, (std::uint64_t)offsetof(OfpWheelState, brake_disc_temperature_c));
        // ABI v6 tail (appended; v5 prefix untouched).
        mixLayout(hash, (std::uint64_t)offsetof(OfpVehicleSpecs, max_torque_nm));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, clutch_torque_nm));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, sequence));
        // ABI v7 tail (appended; every v6 offset untouched).
        mixLayout(hash, (std::uint64_t)sizeof(OfpWheelDiagnostics));
        mixLayout(hash, (std::uint64_t)sizeof(OfpFailureDiagnostics));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, angular_acceleration));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, wheel_diagnostics));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, axle_diagnostics));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, failure));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, trace_records));
        mixLayout(hash, (std::uint64_t)offsetof(OfpWheelDiagnostics, anchor_position));
        return hash;
    }
}

bool RustPhysicsLibrary::load(const std::string& library_path)
{
    unload();
    _path = library_path;
    std::string error;
    _module = openLibrary(library_path, error);
    if (!_module)
    {
        _last_error = error;
        return false;
    }

    struct Symbol
    {
        const char* name;
        void** target;
    };
    const Symbol symbols[] = {
        { "ofp_abi_version", reinterpret_cast<void**>(&abiVersion) },
        { "ofp_abi_signature", reinterpret_cast<void**>(&abiSignature) },
        { "ofp_build_provenance", reinterpret_cast<void**>(&buildProvenance) },
        { "ofp_load_track_ground_mesh", reinterpret_cast<void**>(&loadTrackGroundMesh) },
        { "ofp_track_ground_triangles", reinterpret_cast<void**>(&trackGroundTriangles) },
        { "ofp_create", reinterpret_cast<void**>(&create) },
        { "ofp_destroy", reinterpret_cast<void**>(&destroy) },
        { "ofp_reset", reinterpret_cast<void**>(&reset) },
        { "ofp_set_pose", reinterpret_cast<void**>(&setPose) },
        { "ofp_step", reinterpret_cast<void**>(&step) },
        { "ofp_snapshot", reinterpret_cast<void**>(&snapshot) },
        { "ofp_last_error", reinterpret_cast<void**>(&getLastError) },
        { "ofp_backend_mask", reinterpret_cast<void**>(&backendMask) },
        { "ofp_setup_document", reinterpret_cast<void**>(&setupDocument) },
        { "ofp_setup_fields", reinterpret_cast<void**>(&setupFields) },
        { "ofp_fuel_density", reinterpret_cast<void**>(&fuelDensity) },
        { "ofp_vehicle_specs", reinterpret_cast<void**>(&vehicleSpecs) },
        { "ofp_apply_setup", reinterpret_cast<void**>(&applySetup) },
        { "ofp_reset_setup", reinterpret_cast<void**>(&resetSetup) },
        { "ofp_pit_fuel", reinterpret_cast<void**>(&pitFuel) },
        { "ofp_pit_tires", reinterpret_cast<void**>(&pitTires) },
        { "ofp_vehicle_geometry", reinterpret_cast<void**>(&vehicleGeometry) },
    };
    for (const Symbol& symbol : symbols)
    {
        *symbol.target = resolve(_module, symbol.name);
        if (!*symbol.target)
        {
            _last_error = std::string("missing Rust physics symbol: ") + symbol.name;
            unload();
            return false;
        }
    }
    // ABI v7 diagnostics entry points. A v7 library always exports them; resolving
    // them without failing keeps the bridge usable with a v7 build that dropped the
    // optional dumper, and the call sites guard on null.
    traceShutdown = reinterpret_cast<TraceShutdownFn>(resolve(_module, "ofp_trace_shutdown"));
    traceConfigure = reinterpret_cast<TraceConfigureFn>(resolve(_module, "ofp_trace_configure"));
    traceDump = reinterpret_cast<TraceDumpFn>(resolve(_module, "ofp_trace_dump"));
    traceBuffered = reinterpret_cast<TraceBufferedFn>(resolve(_module, "ofp_trace_buffered"));
    effectiveSetup = reinterpret_cast<EffectiveSetupFn>(resolve(_module, "ofp_effective_setup"));
    // Session collision-world entry points: optional, so a library built before the
    // refactor still loads and the adapter falls back to the single-argument loader.
    loadTrackGroundMeshV2 = reinterpret_cast<LoadTrackGroundMeshV2Fn>(
        resolve(_module, "ofp_load_track_ground_mesh_v2"));
    releaseTrackGroundMesh = reinterpret_cast<ReleaseTrackGroundMeshFn>(
        resolve(_module, "ofp_release_track_ground_mesh"));
    worldGeneration = reinterpret_cast<WorldGenerationFn>(resolve(_module, "ofp_world_generation"));
    worldInstallCount = reinterpret_cast<WorldInstallCountFn>(
        resolve(_module, "ofp_world_install_count"));
    worldIdentityJson = reinterpret_cast<WorldIdentityJsonFn>(
        resolve(_module, "ofp_world_identity_json"));
    worldLastError = reinterpret_cast<WorldLastErrorFn>(resolve(_module, "ofp_world_last_error"));
    contactReport = reinterpret_cast<ContactReportFn>(resolve(_module, "ofp_contact_report"));
    if (abiVersion() != OFP_ABI_VERSION)
    {
        _last_error = "Rust physics ABI mismatch: library reports " +
                      std::to_string(abiVersion()) + ", host expects " +
                      std::to_string(OFP_ABI_VERSION);
        unload();
        return false;
    }
    if (!abiSignature || abiSignature() != expectedAbiSignature())
    {
        _last_error = "Rust physics ABI layout mismatch: the bridge and the library were built "
                      "from different structure definitions";
        unload();
        return false;
    }
    return true;
}

void RustPhysicsLibrary::unload()
{
    if (_module) {
        if (traceShutdown) traceShutdown();
        closeLibrary(_module);
    }
    _module = nullptr;
    abiVersion = nullptr;
    buildProvenance = nullptr;
    loadTrackGroundMesh = nullptr;
    trackGroundTriangles = nullptr;
    create = nullptr;
    destroy = nullptr;
    reset = nullptr;
    setPose = nullptr;
    step = nullptr;
    snapshot = nullptr;
    getLastError = nullptr;
    backendMask = nullptr;
    setupDocument = nullptr;
    setupFields = nullptr;
    fuelDensity = nullptr;
    vehicleSpecs = nullptr;
    applySetup = nullptr;
    resetSetup = nullptr;
    pitFuel = nullptr;
    pitTires = nullptr;
    vehicleGeometry = nullptr;
    traceShutdown = nullptr;
    traceConfigure = nullptr;
    traceDump = nullptr;
    traceBuffered = nullptr;
    effectiveSetup = nullptr;
    loadTrackGroundMeshV2 = nullptr;
    releaseTrackGroundMesh = nullptr;
    worldGeneration = nullptr;
    worldInstallCount = nullptr;
    worldIdentityJson = nullptr;
    worldLastError = nullptr;
    contactReport = nullptr;
}
