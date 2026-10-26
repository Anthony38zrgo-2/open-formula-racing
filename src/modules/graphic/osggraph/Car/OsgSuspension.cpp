#include "OsgSuspension.h"

#include <osg/Geometry>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/CullFace>
#include <osg/NodeVisitor>
#include <osgDB/FileUtils>

#include <tgf.h>
#include <portability.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <tuple>

#include <car.h>
#ifdef _name
#undef _name
#endif

namespace osggraph {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

inline osg::Vec3d rustToSdLocal(double x_g, double y_g, double z_g, double offset_z)
{
    return osg::Vec3d(-z_g, -x_g, y_g + offset_z);
}

inline osg::Vec3d rustToSdVec(double x_g, double y_g, double z_g)
{
    return osg::Vec3d(-z_g, -x_g, y_g);
}

inline osg::Vec3d accToRust(const osg::Vec3f& v, double offset_z)
{
    return osg::Vec3d((double)v.z(), (double)v.y() - offset_z, -(double)v.x());
}

inline osg::Vec3f accToSdPos(const osg::Vec3f& v)
{
    return osg::Vec3f(v.x(), -v.z(), v.y());
}

inline osg::Vec3d norm(const osg::Vec3d& v)
{
    double len = v.length();
    if (len < 1e-9)
        return osg::Vec3d(1.0, 0.0, 0.0);
    return v / len;
}

inline osg::Vec3d closestPointOnAxis(const osg::Vec3d& p, const osg::Vec3d& axisPoint, const osg::Vec3d& axisDir)
{
    double l = axisDir.length();
    if (l < 1e-9)
        return axisPoint;
    return axisPoint + axisDir * ((p - axisPoint) * axisDir);
}

inline osg::Vec3d perpVector(const osg::Vec3d& axis)
{
    // Speed Dreams car-local: +Z is Up, +X is Forward, +Y is Left
    osg::Vec3d u = axis ^ osg::Vec3d(0.0, 0.0, 1.0);
    if (u.length() < 1e-6)
        u = axis ^ osg::Vec3d(1.0, 0.0, 0.0);
    if (u.length() < 1e-6)
        u = axis ^ osg::Vec3d(0.0, 1.0, 0.0);
    return norm(u);
}

inline osg::Vec3d projectCircle(const osg::Vec3d& p, const osg::Vec3d& center, const osg::Vec3d& axis, double radius)
{
    osg::Vec3d v = p - center;
    osg::Vec3d v_perp = v - axis * (v * axis);
    if (v_perp.length() < 1e-9)
        v_perp = perpVector(axis);
    return center + norm(v_perp) * radius;
}

inline double signedAngle(const osg::Vec3d& a, const osg::Vec3d& b, const osg::Vec3d& axis)
{
    if (a.length() < 1e-9 || b.length() < 1e-9)
        return 0.0;
    osg::Vec3d c = a ^ b;
    double s = c * axis;
    double d = a * b;
    return std::atan2(s, d);
}

void constrainTriangle(osg::Vec3d& lbj, osg::Vec3d& ubj, osg::Vec3d& hub, double d_lu, double d_lh, double d_uh)
{
    osg::Vec3d dir_lu = lbj - ubj;
    if (dir_lu.length() > 1e-9)
    {
        osg::Vec3d mid = (lbj + ubj) * 0.5;
        osg::Vec3d d = norm(dir_lu);
        lbj = mid + d * (d_lu * 0.5);
        ubj = mid - d * (d_lu * 0.5);
    }
    osg::Vec3d dh = lbj - hub;
    if (dh.length() > 1e-9)
    {
        osg::Vec3d dir_h = norm(dh);
        osg::Vec3d corr = dir_h * (dh.length() - d_lh);
        lbj -= corr * 0.5;
        hub += corr * 0.5;
    }
    osg::Vec3d du = ubj - hub;
    if (du.length() > 1e-9)
    {
        osg::Vec3d dir_u = norm(du);
        osg::Vec3d corr2 = dir_u * (du.length() - d_uh);
        ubj -= corr2 * 0.5;
        hub += corr2 * 0.5;
    }
}

osg::Matrixd triangleBasis(const osg::Vec3d& lower, const osg::Vec3d& upper, const osg::Vec3d& hub)
{
    osg::Vec3d y = norm(upper - lower);
    osg::Vec3d x = (hub - lower) - y * ((hub - lower) * y);
    x = norm(x);
    osg::Vec3d z = norm(x ^ y);
    return osg::Matrixd(
        x.x(), x.y(), x.z(), 0.0,
        y.x(), y.y(), y.z(), 0.0,
        z.x(), z.y(), z.z(), 0.0,
        0.0,   0.0,   0.0,   1.0
    );
}

SDSuspension::Role classifyComponent(bool isRear,
                                     double minAbsX, double maxAbsX,
                                     double minY, double maxY,
                                     double minZ, double maxZ)
{
    double midY = 0.5 * (minY + maxY);
    double spanY = maxY - minY;
    double midZ = 0.5 * (minZ + maxZ);

    if (maxAbsX < 0.25)
        return SDSuspension::ROLE_INBOARD_STATIC;
    if (minAbsX > 0.50)
        return SDSuspension::ROLE_UPRIGHT;
    if (spanY > 0.20)
        return SDSuspension::ROLE_PUSHROD;
    if (isRear && midY > -0.27 && midY < -0.18)
    {
        if (midZ > 1.82)
            return SDSuspension::ROLE_TRACKROD;
        return SDSuspension::ROLE_DRIVESHAFT;
    }
    if (midY > -0.20)
        return SDSuspension::ROLE_UPPER_WISHBONE;
    return SDSuspension::ROLE_LOWER_WISHBONE;
}

class CollectSuspensionGeodesVisitor : public osg::NodeVisitor
{
public:
    CollectSuspensionGeodesVisitor() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN) {}

    void apply(osg::Geode& geode) override
    {
        const std::string& name = geode.getName();
        if (name.find("GEO_SUSPENSION_") != std::string::npos)
        {
            geodes.push_back(&geode);
        }
        traverse(geode);
    }

    std::vector<osg::Geode*> geodes;
};

} // anonymous namespace

struct SDSuspension::CornerData
{
    bool valid = false;
    osg::Vec3d hub_center;
    osg::Vec3d lw_if, lw_ir, lbj_rest;
    osg::Vec3d uw_if, uw_ir, ubj_rest;
    osg::Vec3d pushrod_outer_rest, pushrod_inner_rest;
    osg::Vec3d trackrod_inner, trackrod_outer;
    bool has_driveshaft = false;
    osg::Vec3d ds_inner, ds_outer_rest;

    // Precalculated constants in SD car-local coordinates
    osg::Vec3d l_dir, l_center;
    double l_radius = 0.0;
    osg::Vec3d u_dir, u_center;
    double u_radius = 0.0;
    double d_lu = 0.0, d_lh = 0.0, d_uh = 0.0;
};

struct SDSuspension::CornerVisuals
{
    osg::ref_ptr<osg::Group> cornerRoot;
    osg::ref_ptr<osg::MatrixTransform> roleTransforms[SDSuspension::ROLE_COUNT];
};

SDSuspension::SDSuspension() :
    m_enabled(false),
    m_car(nullptr),
    m_meshOriginHeight(0.48865)
{
    for (int i = 0; i < 4; ++i)
    {
        m_corners[i] = nullptr;
        m_visuals[i] = nullptr;
    }
}

SDSuspension::~SDSuspension()
{
    for (int i = 0; i < 4; ++i)
    {
        delete m_corners[i];
        delete m_visuals[i];
    }
}

bool SDSuspension::loadProfile(tCarElt *car, void *handle)
{
    if (!car) return false;

    // Read mesh origin height offset (defaults to 0.48865 for Williams FW19)
    m_meshOriginHeight = GfParmGetNum(handle, SECT_GROBJECTS, "source mesh origin offset z", nullptr, 0.48865);

    // Exact pivot anchors measured directly from the authored Williams FW19 3D mesh
    // in Godot/Rust frame (+X right, +Y up, -Z forward), converted to Speed Dreams
    // car-local frame: X_sd = -Z_g, Y_sd = -X_g, Z_sd = Y_g + offset_z.

    // Front Left (FL) - Speed Dreams wheel index 1 (FRNT_LFT)
    CornerData* fl = new CornerData;
    fl->hub_center         = rustToSdLocal(-0.834987, -0.226701, -1.146992, m_meshOriginHeight);
    fl->lw_if              = rustToSdLocal(-0.0677,   -0.3191,   -1.1933,   m_meshOriginHeight);
    fl->lw_ir              = rustToSdLocal(-0.1337,   -0.3218,   -0.5340,   m_meshOriginHeight);
    fl->lbj_rest           = rustToSdLocal(-0.6741,   -0.3327,   -1.1442,   m_meshOriginHeight);

    fl->uw_if              = rustToSdLocal(-0.1554,   -0.1442,   -1.1758,   m_meshOriginHeight);
    fl->uw_ir              = rustToSdLocal(-0.1919,   -0.1437,   -0.5461,   m_meshOriginHeight);
    fl->ubj_rest           = rustToSdLocal(-0.6565,   -0.1408,   -1.1515,   m_meshOriginHeight);

    fl->pushrod_outer_rest = rustToSdLocal(-0.6805,   -0.3289,   -1.1326,   m_meshOriginHeight);
    fl->pushrod_inner_rest = rustToSdLocal(-0.1819,    0.0096,   -0.9365,   m_meshOriginHeight);

    fl->trackrod_inner     = rustToSdLocal(-0.181461, -0.259653, -1.278319, m_meshOriginHeight);
    fl->trackrod_outer     = rustToSdLocal(-0.645358, -0.260107, -1.25807,  m_meshOriginHeight);
    fl->has_driveshaft     = false;

    // Rear Left (RL) - Speed Dreams wheel index 3 (REAR_LFT)
    CornerData* rl = new CornerData;
    rl->hub_center         = rustToSdLocal(-0.8000,   -0.216634,  1.743008, m_meshOriginHeight);
    rl->lw_if              = rustToSdLocal(-0.1463,   -0.3038,    1.4242,   m_meshOriginHeight);
    rl->lw_ir              = rustToSdLocal(-0.0849,   -0.3356,    1.7691,   m_meshOriginHeight);
    rl->lbj_rest           = rustToSdLocal(-0.6043,   -0.3454,    1.7305,   m_meshOriginHeight);

    rl->uw_if              = rustToSdLocal(-0.1243,   -0.1510,    1.3811,   m_meshOriginHeight);
    rl->uw_ir              = rustToSdLocal(-0.0765,   -0.1569,    1.8405,   m_meshOriginHeight);
    rl->ubj_rest           = rustToSdLocal(-0.5573,   -0.0847,    1.6877,   m_meshOriginHeight);

    rl->pushrod_outer_rest = rustToSdLocal(-0.5660,   -0.3454,    1.7218,   m_meshOriginHeight);
    rl->pushrod_inner_rest = rustToSdLocal(-0.1379,   -0.1195,    1.6985,   m_meshOriginHeight);

    rl->trackrod_inner     = rustToSdLocal(-0.0748,   -0.2562,    1.9095,   m_meshOriginHeight);
    rl->trackrod_outer     = rustToSdLocal(-0.6053,   -0.2553,    1.8902,   m_meshOriginHeight);

    rl->has_driveshaft     = true;
    rl->ds_inner           = rustToSdLocal(-0.0532,   -0.2311,    1.7582,   m_meshOriginHeight);
    rl->ds_outer_rest      = rustToSdLocal(-0.5952,   -0.2083,    1.7412,   m_meshOriginHeight);

    // Mirror across Y_sd (negating Y) for Right side corners (FR=0, RR=2)
    auto mirrorCorner = [](const CornerData* src) -> CornerData* {
        CornerData* dst = new CornerData(*src);
        auto mir = [](const osg::Vec3d& v) { return osg::Vec3d(v.x(), -v.y(), v.z()); };
        dst->hub_center         = mir(src->hub_center);
        dst->lw_if              = mir(src->lw_if);
        dst->lw_ir              = mir(src->lw_ir);
        dst->lbj_rest           = mir(src->lbj_rest);
        dst->uw_if              = mir(src->uw_if);
        dst->uw_ir              = mir(src->uw_ir);
        dst->ubj_rest           = mir(src->ubj_rest);
        dst->pushrod_outer_rest = mir(src->pushrod_outer_rest);
        dst->pushrod_inner_rest = mir(src->pushrod_inner_rest);
        dst->trackrod_inner     = mir(src->trackrod_inner);
        dst->trackrod_outer     = mir(src->trackrod_outer);
        if (src->has_driveshaft)
        {
            dst->ds_inner      = mir(src->ds_inner);
            dst->ds_outer_rest = mir(src->ds_outer_rest);
        }
        return dst;
    };

    CornerData* fr = mirrorCorner(fl);
    CornerData* rr = mirrorCorner(rl);

    // Map into m_corners by Speed Dreams wheel order: 0=FR, 1=FL, 2=RR, 3=RL
    m_corners[0] = fr;
    m_corners[1] = fl;
    m_corners[2] = rr;
    m_corners[3] = rl;

    for (int i = 0; i < 4; ++i)
    {
        CornerData* c = m_corners[i];
        c->l_dir    = norm(c->lw_ir - c->lw_if);
        c->l_center = closestPointOnAxis(c->lbj_rest, c->lw_if, c->l_dir);
        c->l_radius = (c->lbj_rest - c->l_center).length();

        c->u_dir    = norm(c->uw_ir - c->uw_if);
        c->u_center = closestPointOnAxis(c->ubj_rest, c->uw_if, c->u_dir);
        c->u_radius = (c->ubj_rest - c->u_center).length();

        c->d_lu = (c->lbj_rest - c->ubj_rest).length();
        c->d_lh = (c->lbj_rest - c->hub_center).length();
        c->d_uh = (c->ubj_rest - c->hub_center).length();

        c->valid = true;
    }

    return true;
}

bool SDSuspension::extractAuthoredMeshes(osg::Node *pCar)
{
    if (!pCar) return false;

    CollectSuspensionGeodesVisitor visitor;
    pCar->accept(visitor);
    if (visitor.geodes.empty())
        return false;

    m_suspensionRoot = new osg::Group;
    m_suspensionRoot->setName("SDSuspensionRoot");

    for (int i = 0; i < 4; ++i)
    {
        CornerVisuals* v = new CornerVisuals;
        v->cornerRoot = new osg::Group;
        v->cornerRoot->setName("Corner_" + std::to_string(i));
        for (int r = 0; r < ROLE_COUNT; ++r)
        {
            v->roleTransforms[r] = new osg::MatrixTransform;
            v->roleTransforms[r]->setName("Role_" + std::to_string(r));
            v->cornerRoot->addChild(v->roleTransforms[r].get());
        }
        m_visuals[i] = v;
        m_suspensionRoot->addChild(v->cornerRoot.get());
    }

    int extractedGeodes = 0;
    int extractedTriangles = 0;

    for (osg::Geode* origGeode : visitor.geodes)
    {
        if (!origGeode) continue;
        const std::string& name = origGeode->getName();

        int cornerIndex = -1;
        bool isRear = false;
        if (name == "GEO_SUSPENSION_FRONT_LINK_ASSEMBLY_RIGHT")
        {
            cornerIndex = 0; isRear = false;
        }
        else if (name == "GEO_SUSPENSION_FRONT_LINK_ASSEMBLY_LEFT")
        {
            cornerIndex = 1; isRear = false;
        }
        else if (name == "GEO_SUSPENSION_REAR_LINK_ASSEMBLY_RIGHT")
        {
            cornerIndex = 2; isRear = true;
        }
        else if (name == "GEO_SUSPENSION_REAR_LINK_ASSEMBLY_LEFT")
        {
            cornerIndex = 3; isRear = true;
        }
        else
        {
            continue;
        }

        unsigned int numDrawables = origGeode->getNumDrawables();
        for (unsigned int d = 0; d < numDrawables; ++d)
        {
            osg::Geometry* geom = origGeode->getDrawable(d) ? origGeode->getDrawable(d)->asGeometry() : nullptr;
            if (!geom) continue;

            const osg::Vec3Array* srcVerts = dynamic_cast<const osg::Vec3Array*>(geom->getVertexArray());
            if (!srcVerts || srcVerts->size() < 3) continue;

            const osg::Vec3Array* srcNorms = dynamic_cast<const osg::Vec3Array*>(geom->getNormalArray());
            const osg::Vec2Array* srcTex0  = dynamic_cast<const osg::Vec2Array*>(geom->getTexCoordArray(0));
            const osg::Vec2Array* srcTex1  = dynamic_cast<const osg::Vec2Array*>(geom->getTexCoordArray(1));
            const osg::Vec2Array* srcTex2  = dynamic_cast<const osg::Vec2Array*>(geom->getTexCoordArray(2));
            const osg::Vec2Array* srcTex3  = dynamic_cast<const osg::Vec2Array*>(geom->getTexCoordArray(3));

            struct TriRef
            {
                unsigned int i0, i1, i2;
            };
            std::vector<TriRef> tris;

            for (unsigned int p = 0; p < geom->getNumPrimitiveSets(); ++p)
            {
                const osg::PrimitiveSet* ps = geom->getPrimitiveSet(p);
                if (!ps) continue;
                if (ps->getMode() == osg::PrimitiveSet::TRIANGLES)
                {
                    const osg::DrawArrays* da = dynamic_cast<const osg::DrawArrays*>(ps);
                    if (da)
                    {
                        unsigned int first = da->getFirst();
                        unsigned int count = da->getCount();
                        for (unsigned int k = 0; k + 2 < count; k += 3)
                        {
                            tris.push_back({first + k, first + k + 1, first + k + 2});
                        }
                    }
                    else
                    {
                        for (unsigned int k = 0; k + 2 < ps->getNumIndices(); k += 3)
                        {
                            tris.push_back({ps->index(k), ps->index(k + 1), ps->index(k + 2)});
                        }
                    }
                }
            }

            if (tris.empty()) continue;

            // Group triangles into connected components by quantized vertex position (0.1 mm)
            std::vector<int> parent(tris.size());
            for (size_t t = 0; t < tris.size(); ++t) parent[t] = (int)t;

            auto findRoot = [&parent](int x) {
                while (parent[x] != x)
                {
                    parent[x] = parent[parent[x]];
                    x = parent[x];
                }
                return x;
            };

            using PosKey = std::tuple<int64_t, int64_t, int64_t>;
            std::map<PosKey, int> posMap;

            for (size_t t = 0; t < tris.size(); ++t)
            {
                unsigned int idxs[3] = {tris[t].i0, tris[t].i1, tris[t].i2};
                for (int c = 0; c < 3; ++c)
                {
                    const osg::Vec3f& v = (*srcVerts)[idxs[c]];
                    PosKey key(
                        (int64_t)std::llround((double)v.x() * 10000.0),
                        (int64_t)std::llround((double)v.y() * 10000.0),
                        (int64_t)std::llround((double)v.z() * 10000.0)
                    );
                    auto it = posMap.find(key);
                    if (it != posMap.end())
                    {
                        int ra = findRoot((int)t);
                        int rb = findRoot(it->second);
                        if (ra != rb) parent[ra] = rb;
                    }
                    else
                    {
                        posMap[key] = (int)t;
                    }
                }
            }

            std::map<int, std::vector<size_t>> compMap;
            for (size_t t = 0; t < tris.size(); ++t)
            {
                compMap[findRoot((int)t)].push_back(t);
            }

            std::vector<size_t> roleTris[ROLE_COUNT];
            for (const auto& kv : compMap)
            {
                const std::vector<size_t>& comp = kv.second;
                double minAbsX =  1e9, maxAbsX = -1e9;
                double minY    =  1e9, maxY    = -1e9;
                double minZ    =  1e9, maxZ    = -1e9;

                for (size_t t : comp)
                {
                    unsigned int idxs[3] = {tris[t].i0, tris[t].i1, tris[t].i2};
                    for (int c = 0; c < 3; ++c)
                    {
                        osg::Vec3d vg = accToRust((*srcVerts)[idxs[c]], m_meshOriginHeight);
                        double ax = std::abs(vg.x());
                        if (ax < minAbsX) minAbsX = ax;
                        if (ax > maxAbsX) maxAbsX = ax;
                        if (vg.y() < minY) minY = vg.y();
                        if (vg.y() > maxY) maxY = vg.y();
                        if (vg.z() < minZ) minZ = vg.z();
                        if (vg.z() > maxZ) maxZ = vg.z();
                    }
                }

                Role role = classifyComponent(isRear, minAbsX, maxAbsX, minY, maxY, minZ, maxZ);
                roleTris[role].insert(roleTris[role].end(), comp.begin(), comp.end());
            }

            for (int r = 0; r < ROLE_COUNT; ++r)
            {
                if (roleTris[r].empty()) continue;

                osg::ref_ptr<osg::Geode> newGeode = new osg::Geode;
                newGeode->setName(name + "_Role_" + std::to_string(r));
                if (origGeode->getStateSet())
                {
                    newGeode->setStateSet(origGeode->getStateSet());
                }

                osg::ref_ptr<osg::Geometry> newGeom = new osg::Geometry;
                newGeom->setDataVariance(osg::Object::STATIC);

                if (geom->getColorArray())
                {
                    newGeom->setColorArray(const_cast<osg::Array*>(geom->getColorArray()), osg::Array::BIND_OVERALL);
                }

                size_t vCount = roleTris[r].size() * 3;
                osg::ref_ptr<osg::Vec3Array> dstVerts = new osg::Vec3Array;
                dstVerts->reserve(vCount);

                osg::ref_ptr<osg::Vec3Array> dstNorms;
                if (srcNorms && srcNorms->size() >= srcVerts->size())
                {
                    dstNorms = new osg::Vec3Array;
                    dstNorms->reserve(vCount);
                }

                osg::ref_ptr<osg::Vec2Array> dstTex0, dstTex1, dstTex2, dstTex3;
                if (srcTex0 && srcTex0->size() >= srcVerts->size()) { dstTex0 = new osg::Vec2Array; dstTex0->reserve(vCount); }
                if (srcTex1 && srcTex1->size() >= srcVerts->size()) { dstTex1 = new osg::Vec2Array; dstTex1->reserve(vCount); }
                if (srcTex2 && srcTex2->size() >= srcVerts->size()) { dstTex2 = new osg::Vec2Array; dstTex2->reserve(vCount); }
                if (srcTex3 && srcTex3->size() >= srcVerts->size()) { dstTex3 = new osg::Vec2Array; dstTex3->reserve(vCount); }

                for (size_t t : roleTris[r])
                {
                    unsigned int idxs[3] = {tris[t].i0, tris[t].i1, tris[t].i2};
                    for (int c = 0; c < 3; ++c)
                    {
                        unsigned int idx = idxs[c];
                        dstVerts->push_back(accToSdPos((*srcVerts)[idx]));
                        if (dstNorms.valid()) dstNorms->push_back(accToSdPos((*srcNorms)[idx]));
                        if (dstTex0.valid())  dstTex0->push_back((*srcTex0)[idx]);
                        if (dstTex1.valid())  dstTex1->push_back((*srcTex1)[idx]);
                        if (dstTex2.valid())  dstTex2->push_back((*srcTex2)[idx]);
                        if (dstTex3.valid())  dstTex3->push_back((*srcTex3)[idx]);
                    }
                }

                newGeom->setVertexArray(dstVerts.get());
                if (dstNorms.valid())
                    newGeom->setNormalArray(dstNorms.get(), osg::Array::BIND_PER_VERTEX);
                if (dstTex0.valid()) newGeom->setTexCoordArray(0, dstTex0.get());
                if (dstTex1.valid()) newGeom->setTexCoordArray(1, dstTex1.get());
                if (dstTex2.valid()) newGeom->setTexCoordArray(2, dstTex2.get());
                if (dstTex3.valid()) newGeom->setTexCoordArray(3, dstTex3.get());

                newGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, (GLsizei)dstVerts->size()));
                newGeode->addDrawable(newGeom.get());

                m_visuals[cornerIndex]->roleTransforms[r]->addChild(newGeode.get());
                extractedTriangles += (int)roleTris[r].size();
            }
        }

        // Remove original static drawables and mask node so even NodeMaskOverride (~0u) in GaragePreview will not double-render
        origGeode->removeDrawables(0, origGeode->getNumDrawables());
        origGeode->setNodeMask(0);
        extractedGeodes++;
    }

    GfLogInfo("SDSuspension: Extracted %d triangles from %d authored suspension geodes\n",
              extractedTriangles, extractedGeodes);
    return extractedTriangles > 0;
}

osg::ref_ptr<osg::Group> SDSuspension::initSuspension(tCarElt *car, void *handle, osg::Node *pCar)
{
    m_car = car;
    if (!car || !pCar) return nullptr;

    std::string carName = car->_carName ? car->_carName : "";
    if (!loadProfile(car, handle))
    {
        return nullptr;
    }

    if (!extractAuthoredMeshes(pCar))
    {
        return nullptr;
    }

    // Solve rest pose (travel = 0, steer = 0, spin = 0 -> Identity transforms)
    for (int i = 0; i < 4; ++i)
    {
        solveCorner(i, 0.0, 0.0, 0.0);
    }

    m_enabled = true;
    GfLogInfo("SDSuspension: Initialized authored 3D suspension animation for %s\n", carName.c_str());
    return m_suspensionRoot;
}

void SDSuspension::solveCorner(int cornerIndex, double travel_m, double steer_rad, double spin_rad)
{
    CornerData* c = m_corners[cornerIndex];
    CornerVisuals* v = m_visuals[cornerIndex];
    if (!c || !v || !c->valid) return;

    // Speed Dreams car-local space: +Z is Up, +X is Forward, +Y is Left
    osg::Vec3d hubTarget = c->hub_center + osg::Vec3d(0.0, 0.0, travel_m);
    osg::Vec3d lbj = hubTarget + (c->lbj_rest - c->hub_center);
    osg::Vec3d ubj = hubTarget + (c->ubj_rest - c->hub_center);
    osg::Vec3d hub = hubTarget;

    // Step 1: PBD constraint solve for wishbones & upright triangle
    const int PBD_ITERATIONS = 12;
    for (int it = 0; it < PBD_ITERATIONS; ++it)
    {
        lbj = projectCircle(lbj, c->l_center, c->l_dir, c->l_radius);
        ubj = projectCircle(ubj, c->u_center, c->u_dir, c->u_radius);
        constrainTriangle(lbj, ubj, hub, c->d_lu, c->d_lh, c->d_uh);
        hub.z() += (hubTarget.z() - hub.z()) * 0.85;
    }

    // Final passes without external hub pull
    const int FINAL_PASSES = 4;
    for (int it = 0; it < FINAL_PASSES; ++it)
    {
        lbj = projectCircle(lbj, c->l_center, c->l_dir, c->l_radius);
        ubj = projectCircle(ubj, c->u_center, c->u_dir, c->u_radius);
        constrainTriangle(lbj, ubj, hub, c->d_lu, c->d_lh, c->d_uh);
    }

    // 0. Inboard static brackets remain fixed to chassis
    v->roleTransforms[ROLE_INBOARD_STATIC]->setMatrix(osg::Matrixd::identity());

    // 1. Lower wishbone rigid hinge rotation around (l_center, l_dir)
    double angleL = signedAngle(norm(c->lbj_rest - c->l_center), norm(lbj - c->l_center), c->l_dir);
    osg::Matrixd lowerPose = osg::Matrixd::translate(-c->l_center)
                           * osg::Matrixd::rotate(angleL, c->l_dir)
                           * osg::Matrixd::translate(c->l_center);
    v->roleTransforms[ROLE_LOWER_WISHBONE]->setMatrix(lowerPose);

    // 2. Upper wishbone rigid hinge rotation around (u_center, u_dir)
    double angleU = signedAngle(norm(c->ubj_rest - c->u_center), norm(ubj - c->u_center), c->u_dir);
    osg::Matrixd upperPose = osg::Matrixd::translate(-c->u_center)
                           * osg::Matrixd::rotate(angleU, c->u_dir)
                           * osg::Matrixd::translate(c->u_center);
    v->roleTransforms[ROLE_UPPER_WISHBONE]->setMatrix(upperPose);

    // 3. Upright / inner hub / brake duct / caliper assembly
    osg::Matrixd transport = osg::Matrixd::inverse(triangleBasis(c->lbj_rest, c->ubj_rest, c->hub_center))
                           * triangleBasis(lbj, ubj, hub);
    osg::Matrixd steerRot  = (cornerIndex < 2)
                           ? osg::Matrixd::rotate(steer_rad, osg::Vec3d(0.0, 0.0, 1.0))
                           : osg::Matrixd::identity();
    osg::Matrixd uprightPose = osg::Matrixd::translate(-c->hub_center)
                             * transport
                             * steerRot
                             * osg::Matrixd::translate(hub);
    v->roleTransforms[ROLE_UPRIGHT]->setMatrix(uprightPose);

    // 4. Pushrod (attached to lower wishbone outboard on both front and rear of the authored Williams mesh)
    osg::Vec3d pushOut  = c->pushrod_outer_rest * lowerPose;
    osg::Vec3d pDir0    = norm(c->pushrod_outer_rest - c->pushrod_inner_rest);
    osg::Vec3d pDir1    = norm(pushOut - c->pushrod_inner_rest);
    osg::Matrixd pushPose = osg::Matrixd::translate(-c->pushrod_outer_rest)
                          * osg::Matrixd::rotate(pDir0, pDir1)
                          * osg::Matrixd::translate(pushOut);
    v->roleTransforms[ROLE_PUSHROD]->setMatrix(pushPose);

    // 5. Trackrod (pivots at trackrod_inner and follows upright steer arm)
    osg::Vec3d trackOut = c->trackrod_outer * uprightPose;
    osg::Vec3d tDir0    = norm(c->trackrod_outer - c->trackrod_inner);
    osg::Vec3d tDir1    = norm(trackOut - c->trackrod_inner);
    osg::Matrixd trackPose = osg::Matrixd::translate(-c->trackrod_inner)
                           * osg::Matrixd::rotate(tDir0, tDir1)
                           * osg::Matrixd::translate(c->trackrod_inner);
    v->roleTransforms[ROLE_TRACKROD]->setMatrix(trackPose);

    // 6. Driveshaft (rear wheels only: spins around its axis and tilts from ds_inner to upright hub)
    if (c->has_driveshaft)
    {
        osg::Vec3d dsOut  = c->ds_outer_rest * uprightPose;
        osg::Vec3d dsDir0 = norm(c->ds_outer_rest - c->ds_inner);
        osg::Vec3d dsDir1 = norm(dsOut - c->ds_inner);
        osg::Matrixd dsPose = osg::Matrixd::translate(-c->ds_inner)
                            * osg::Matrixd::rotate(spin_rad, dsDir0)
                            * osg::Matrixd::rotate(dsDir0, dsDir1)
                            * osg::Matrixd::translate(c->ds_inner);
        v->roleTransforms[ROLE_DRIVESHAFT]->setMatrix(dsPose);
    }
}

void SDSuspension::updateSuspension(tCarElt *car)
{
    if (!m_enabled || !car) return;

    for (int i = 0; i < 4; ++i)
    {
        CornerData* c = m_corners[i];
        if (!c || !c->valid) continue;

        // Vertical displacement of the wheel hub relative to authored rest center
        double wheelZ = (double)car->priv.wheel[i].relPos.z;
        double travel_m = wheelZ - c->hub_center.z();
        travel_m = std::max(-0.08, std::min(0.08, travel_m));

        double steer_rad = (double)car->priv.wheel[i].relPos.az;
        double spin_rad  = (double)car->priv.wheel[i].relPos.ay;

        solveCorner(i, travel_m, steer_rad, spin_rad);
    }
}

} // namespace osggraph
