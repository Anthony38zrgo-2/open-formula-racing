#include "OsgGaragePreview.h"
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/Switch>
#include <osg/TriangleFunctor>
#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace osggraph {
namespace {
struct Triangles
{
    osg::Matrix matrix;
    std::vector<osg::Vec3>* vertices = nullptr;
    std::vector<osg::Vec3>* normals = nullptr;
    void operator()(const osg::Vec3& a, const osg::Vec3& b, const osg::Vec3& c)
    {
        osg::Vec3 p[3] = {a * matrix, b * matrix, c * matrix};
        for (const auto& v : p)
            if (!std::isfinite(v.x()) || !std::isfinite(v.y()) || !std::isfinite(v.z())) return;
        osg::Vec3 normal = (p[1] - p[0]) ^ (p[2] - p[0]);
        if (normal.normalize() < 1e-10f) return;
        for (const auto& v : p) { vertices->push_back(v); normals->push_back(normal); }
    }
};
class Capture : public osg::NodeVisitor
{
public:
    std::vector<osg::Vec3>& vertices;
    std::vector<osg::Vec3>& normals;
    Capture(std::vector<osg::Vec3>& v, std::vector<osg::Vec3>& n)
        : osg::NodeVisitor(TRAVERSE_ACTIVE_CHILDREN), vertices(v), normals(n)
    { setNodeMaskOverride(~0u); }
    void apply(osg::Switch& node) override
    {
        if (node.getName() == "DRIVER") return;
        // The racing camera can select a separate cockpit. Always take the
        // exterior branch for this view, without changing the live switch.
        if (node.getName() == "COCK" && node.getNumChildren() > 1)
            node.getChild(1)->accept(*this);
        else traverse(node);
    }
    void apply(osg::Geode& node) override
    {
        osg::TriangleFunctor<Triangles> triangles;
        triangles.matrix = osg::computeLocalToWorld(getNodePath());
        triangles.vertices = &vertices; triangles.normals = &normals;
        for (unsigned i = 0; i < node.getNumDrawables(); ++i)
            node.getDrawable(i)->accept(triangles);
    }
};
}
void GarageCarPreview::clear() { vertices.clear(); normals.clear(); }
bool GarageCarPreview::capture(osg::Group* root)
{
    clear();
    if (!root) return false;
    Capture visitor(vertices, normals);
    // Exclude only the root's world placement. Keep all child transforms,
    // including wheel scale, steering, camber and suspension placement.
    for (unsigned i = 0; i < root->getNumChildren(); ++i) root->getChild(i)->accept(visitor);
    if (vertices.empty()) return false;
    low.set(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    high = -low;
    for (const auto& v : vertices) for (int axis = 0; axis < 3; ++axis)
    { low[axis] = std::min(low[axis], v[axis]); high[axis] = std::max(high[axis], v[axis]); }
    return (high - low).length() > 1e-5f;
}
bool GarageCarPreview::draw(int x, int y, int width, int height, bool side) const
{
    if (vertices.empty() || width <= 0 || height <= 0) return false;
    using UseProgram = void (APIENTRY*)(GLuint);
    using ActiveTexture = void (APIENTRY*)(GLenum);
    static const auto useProgram = reinterpret_cast<UseProgram>(SDL_GL_GetProcAddress("glUseProgram"));
    static const auto activeTexture = reinterpret_cast<ActiveTexture>(SDL_GL_GetProcAddress("glActiveTexture"));
    GLint program = 0, textureUnit = GL_TEXTURE0, matrixMode = GL_MODELVIEW;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program); glGetIntegerv(GL_ACTIVE_TEXTURE, &textureUnit);
    glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if (useProgram) useProgram(0);
    GLint units = 1; glGetIntegerv(GL_MAX_TEXTURE_UNITS, &units);
    for (int i = 0; i < units; ++i)
    {
        if (activeTexture) activeTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_1D); glDisable(GL_TEXTURE_2D); glDisable(GL_TEXTURE_CUBE_MAP);
        glDisable(GL_TEXTURE_3D);
    }
    if (activeTexture) activeTexture(GL_TEXTURE0);
    glViewport(x, y, width, height);
    glEnable(GL_SCISSOR_TEST); glScissor(x, y, width, height);
    glDepthMask(GL_TRUE); glClearDepth(1); glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND); glDisable(GL_ALPHA_TEST); glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    for (int i = 0; i < 6; ++i) glDisable(GL_CLIP_PLANE0 + i);
    glDisable(GL_FOG); glDisable(GL_COLOR_MATERIAL); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (int i = 0; i < 8; ++i) glDisable(GL_LIGHT0 + i);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    const osg::Vec3 center = (low + high) * .5f;
    const osg::Vec3 span = high - low;
    // Coordinates of the native model: X forward, Y left, Z up.
    const double modelWidth = side ? span.x() : span.y();
    const double modelHeight = side ? span.z() : span.x();
    const double aspect = (double)width / height;
    const double halfHeight = std::max(modelHeight, modelWidth / aspect) * .56;
    const double depth = std::max(1.0, (double)span.length() * 2);
    glOrtho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, 0, depth * 3);
    glMatrixMode(GL_MODELVIEW); glPushMatrix();
    const osg::Matrix view = side
        ? osg::Matrix::lookAt(center + osg::Vec3(0, -depth, 0), center, osg::Vec3(0, 0, 1))
        : osg::Matrix::lookAt(center + osg::Vec3(0, 0, depth), center, osg::Vec3(1, 0, 0));
    glLoadMatrixd(view.ptr());
    const GLfloat light[] = {-.4f, -.6f, 1.f, 0.f};
    const GLfloat ambient[] = {.40f, .40f, .40f, 1};
    const GLfloat diffuse[] = {.75f, .75f, .75f, 1};
    const GLfloat material[] = {.56f, .59f, .61f, 1};
    const GLfloat specular[] = {.10f, .10f, .10f, 1};
    const GLfloat black[] = {0, 0, 0, 1};
    // Eye-space light, independent of the selected orthographic view.
    glPushMatrix(); glLoadIdentity(); glLightfv(GL_LIGHT0, GL_POSITION, light); glPopMatrix();
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient); glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse); glLightfv(GL_LIGHT0, GL_SPECULAR, diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, black); glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24);
    glBegin(GL_TRIANGLES);
    for (std::size_t i = 0; i < vertices.size(); ++i)
    { glNormal3fv(normals[i].ptr()); glVertex3fv(vertices[i].ptr()); }
    glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(matrixMode); glPopAttrib();
    if (activeTexture) activeTexture(textureUnit);
    if (useProgram) useProgram(program);
    return true;
}
}
