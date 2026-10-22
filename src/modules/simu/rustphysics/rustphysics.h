/*
    rustphysics.h -- Open Formula physics module backed by the Rust library.

    The module owns the simulation loop: it feeds driver commands, the track ground
    mesh and barrier contacts to the Rust library, and publishes the returned snapshot
    into CarElt. Rust is the single owner of pose, velocities, suspension, wheel spin,
    transmission, fuel, thermal state and contact state.
*/

#ifndef _RUSTPHYSICS_H_
#define _RUSTPHYSICS_H_

#include <string>
#include <vector>

#include <iphysicsengine.h>
#include <tgf.hpp>

#include "rust_abi.h"

#ifdef _WIN32
# ifdef RUSTPHYSICS_DLL
#  define RUSTPHYSICS_API __declspec(dllexport)
# else
#  define RUSTPHYSICS_API __declspec(dllimport)
# endif
#else
# define RUSTPHYSICS_API
#endif

extern "C" int RUSTPHYSICS_API openGfModule(const char* pszShLibName, void* hShLibHandle);
extern "C" int RUSTPHYSICS_API closeGfModule();

class RUSTPHYSICS_API RustPhysics : public GfModule, public IPhysicsEngine
{
public:
    virtual void initialize(int nCars, struct Track* pTrack);
    virtual void configureCar(struct CarElt* pCar);
    virtual void reconfigureCar(struct CarElt* pCar);
    virtual void toggleCarTelemetry(int nCarIndex, bool bOn = true);
    virtual void updateSituation(struct Situation* pSituation, double fDeltaTime);
    virtual void updateCar(struct Situation* pSituation, double fDeltaTime, int nCarIndex);
    virtual void setCar(const struct DynPt& dynGCG, int nCarIndex);
    virtual struct DynPt* getCar(int nCarIndex);
    virtual void shutdown();

    virtual bool ownsGarageSetup() const { return true; }
    virtual bool garageFields(int car_index, std::vector<PhysicsSetupField>& fields);
    virtual bool applyGarageFields(int car_index, const std::vector<std::pair<std::string, double> >& values, std::string& error);
    static RustPhysics& self();

    //! Applies a setup patch produced by the garage to the authoritative Rust car.
    //! Returns false and fills `error` when the patch is rejected; nothing is committed.
    bool applySetupPatch(int car_index, const std::string& patch_json, std::string& error);
    //! Serialized Rust setup document (current value, base value and field descriptor).
    bool setupDocument(int car_index, std::string& document);
    bool resetSetup(int car_index, std::string& error);

    virtual ~RustPhysics();

protected:
    RustPhysics(const std::string& strShLibName, void* hShLibHandle);

    friend int openGfModule(const char* pszShLibName, void* hShLibHandle);
    friend int closeGfModule();

    static RustPhysics* _pSelf;
};

#endif // _RUSTPHYSICS_H_
