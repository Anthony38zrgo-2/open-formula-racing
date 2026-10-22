/*
    rust_library.h -- run-time loader for the Open Formula Rust physics DLL.
*/

#ifndef RUST_LIBRARY_H
#define RUST_LIBRARY_H

#include <string>

#include "rust_abi.h"

class RustPhysicsLibrary
{
public:
    typedef std::uint32_t (*AbiVersionFn)();
    typedef std::uint64_t (*AbiSignatureFn)();
    typedef const char* (*BuildProvenanceFn)();
    typedef int (*LoadTrackGroundMeshFn)(const char* path);
    typedef int (*TrackGroundTrianglesFn)();
    typedef OfpHandle (*CreateFn)(const char* profile_path, OfpVec3 origin, double yaw_radians, double fuel_kg);
    typedef void (*DestroyFn)(OfpHandle handle);
    typedef int (*ResetFn)(OfpHandle handle, OfpVec3 origin, double yaw_radians);
    typedef int (*SetPoseFn)(OfpHandle handle, OfpVec3 origin, OfpVec3 basis_x, OfpVec3 basis_y, OfpVec3 basis_z);
    typedef int (*StepFn)(OfpHandle handle, const OfpControlInput* input,
                          const OfpBodyContact* contacts, std::uint32_t contact_count,
                          double duration_seconds);
    typedef int (*SnapshotFn)(const OfpHandle handle, OfpSnapshot* out);
    typedef int (*LastErrorFn)(const OfpHandle handle, char* buffer, std::uint32_t capacity);
    typedef std::uint32_t (*BackendMaskFn)(const OfpHandle handle);
    typedef int (*SetupDocumentFn)(const OfpHandle handle, char* buffer, std::uint32_t capacity);
    typedef int (*VehicleSpecsFn)(const OfpHandle handle, OfpVehicleSpecs* out);
    typedef double (*FuelDensityFn)(const OfpHandle handle);
    typedef int (*ApplySetupFn)(OfpHandle handle, const char* text);
    typedef int (*ResetSetupFn)(OfpHandle handle);
    typedef int (*PitFuelFn)(OfpHandle handle, double kilograms);
    typedef int (*PitTiresFn)(OfpHandle handle);
    typedef int (*VehicleGeometryFn)(const OfpHandle handle, OfpVec3* center_of_mass, OfpVec3* inertia,
                                     double* spawn_height, double* mass);

    RustPhysicsLibrary();
    ~RustPhysicsLibrary();

    bool load(const std::string& library_path);
    void unload();
    bool isLoaded() const { return _module != nullptr; }
    const std::string& path() const { return _path; }
    const std::string& lastError() const { return _last_error; }

    AbiVersionFn abiVersion;
    AbiSignatureFn abiSignature;
    BuildProvenanceFn buildProvenance;
    LoadTrackGroundMeshFn loadTrackGroundMesh;
    TrackGroundTrianglesFn trackGroundTriangles;
    CreateFn create;
    DestroyFn destroy;
    ResetFn reset;
    SetPoseFn setPose;
    StepFn step;
    SnapshotFn snapshot;
    LastErrorFn getLastError;
    BackendMaskFn backendMask;
    SetupDocumentFn setupDocument;
    SetupDocumentFn setupFields;
    FuelDensityFn fuelDensity;
    VehicleSpecsFn vehicleSpecs;
    ApplySetupFn applySetup;
    ResetSetupFn resetSetup;
    PitFuelFn pitFuel;
    PitTiresFn pitTires;
    VehicleGeometryFn vehicleGeometry;

private:
    void* _module;
    std::string _path;
    std::string _last_error;
};

// Process-wide library instance (the DLL is loaded once per session).
extern RustPhysicsLibrary rustPhysicsLibrary;

#endif // RUST_LIBRARY_H
