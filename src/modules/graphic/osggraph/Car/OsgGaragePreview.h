#ifndef OPEN_FORMULA_OSG_GARAGE_PREVIEW_H
#define OPEN_FORMULA_OSG_GARAGE_PREVIEW_H

#include <osg/Group>
#include <osg/Vec3>
#include <vector>

namespace osggraph {
// CPU-only snapshot: owns no scene nodes, textures, materials or GL objects.
class GarageCarPreview
{
public:
    bool capture(osg::Group* carRoot);
    bool draw(int x, int y, int width, int height, bool sideView) const;
    void clear();
    std::size_t triangleCount() const { return vertices.size() / 3; }
    const osg::Vec3& minimum() const { return low; }
    const osg::Vec3& maximum() const { return high; }
private:
    std::vector<osg::Vec3> vertices;
    std::vector<osg::Vec3> normals;
    osg::Vec3 low, high;
};
}
#endif
