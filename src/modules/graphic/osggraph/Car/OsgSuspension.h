#ifndef _OSG_SUSPENSION_H_
#define _OSG_SUSPENSION_H_

#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Vec3d>
#include <osg/Matrixd>
#include <string>
#include <vector>

struct CarElt;
typedef struct CarElt tCarElt;

namespace osggraph {

class SDSuspension
{
public:
    enum Role
    {
        ROLE_INBOARD_STATIC = 0,
        ROLE_LOWER_WISHBONE = 1,
        ROLE_UPPER_WISHBONE = 2,
        ROLE_PUSHROD        = 3,
        ROLE_TRACKROD       = 4,
        ROLE_DRIVESHAFT     = 5,
        ROLE_UPRIGHT        = 6,
        ROLE_COUNT          = 7
    };

    SDSuspension();
    ~SDSuspension();

    // Initializes suspension for the car by extracting and splitting the authored
    // GEO_SUSPENSION_* sub-meshes from pCar into articulated osg::MatrixTransform nodes.
    osg::ref_ptr<osg::Group> initSuspension(tCarElt *car, void *handle, osg::Node *pCar);

    // Updates suspension linkage poses each frame based on car wheel telemetry
    void updateSuspension(tCarElt *car);

    bool isEnabled() const { return m_enabled; }

private:
    struct CornerData;
    struct CornerVisuals;

    bool loadProfile(tCarElt *car, void *handle);
    bool extractAuthoredMeshes(osg::Node *pCar);
    void solveCorner(int cornerIndex, double travel_m, double steer_rad, double spin_rad);

    bool m_enabled;
    tCarElt *m_car;
    double m_meshOriginHeight;
    osg::ref_ptr<osg::Group> m_suspensionRoot;

    // 4 corners: 0=FR, 1=FL, 2=RR, 3=RL (matching Speed Dreams wheel indices: FRNT_RGT=0, FRNT_LFT=1, REAR_RGT=2, REAR_LFT=3)
    CornerData* m_corners[4];
    CornerVisuals* m_visuals[4];
};

} // namespace osggraph

#endif // _OSG_SUSPENSION_H_
