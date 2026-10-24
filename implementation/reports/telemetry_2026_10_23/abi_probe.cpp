// Proves the host's load-time ABI gate accepts the installed Rust library: the same
// FNV-1a over the same sizes and offsets the loader computes, compared against the
// value the library reports.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <windows.h>

#include "rust_abi.h"

namespace
{
    void mixLayout(std::uint64_t& hash, std::uint64_t value)
    {
        hash ^= value;
        hash *= 0x00000100000001b3ull;
    }

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
        mixLayout(hash, (std::uint64_t)offsetof(OfpVehicleSpecs, max_torque_nm));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, clutch_torque_nm));
        mixLayout(hash, (std::uint64_t)offsetof(OfpSnapshot, sequence));
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

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("usage: abi_probe <vehicle_physics_engine.dll>\n");
        return 2;
    }
    HMODULE module = ::LoadLibraryA(argv[1]);
    if (!module)
    {
        std::printf("FAIL cannot load %s (error %lu)\n", argv[1], ::GetLastError());
        return 1;
    }
    typedef std::uint32_t (*VersionFn)();
    typedef std::uint64_t (*SignatureFn)();
    VersionFn version = reinterpret_cast<VersionFn>(
        reinterpret_cast<void*>(::GetProcAddress(module, "ofp_abi_version")));
    SignatureFn signature = reinterpret_cast<SignatureFn>(
        reinterpret_cast<void*>(::GetProcAddress(module, "ofp_abi_signature")));
    if (!version || !signature)
    {
        std::printf("FAIL the library does not export the ABI identity symbols\n");
        return 1;
    }
    const std::uint64_t host = expectedAbiSignature();
    const std::uint64_t library = signature();
    std::printf("host_version=%u library_version=%u\n", OFP_ABI_VERSION, version());
    std::printf("host_signature=0x%016llx library_signature=0x%016llx\n",
                (unsigned long long)host, (unsigned long long)library);
    const bool ok = version() == OFP_ABI_VERSION && host == library;
    std::printf("%s\n", ok ? "PASS the loader accepts this library"
                           : "FAIL the loader would refuse this library");
    return ok ? 0 : 1;
}
