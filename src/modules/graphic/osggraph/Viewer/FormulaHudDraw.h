/***************************************************************************

    file        : FormulaHudDraw.h
    created     : 2026-10-22
    copyright   : (C) 2026 Open Formula

    Small immediate-mode drawing helpers for the Formula HUD. Every shape is emitted as
    triangles into one per-panel geometry, so a panel costs one draw call for its shapes
    plus one per text drawable, and no line width or core-profile extension is required.

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _FORMULA_HUD_DRAW_H_
#define _FORMULA_HUD_DRAW_H_

#include <string>
#include <vector>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/ref_ptr>

namespace osgText
{
    class Font;
    class Text;
}

namespace osggraph
{
namespace formulahud
{

//! Alignment of a HUD text drawable, expressed without the osgText types.
enum HudTextAlignment
{
    HUD_TEXT_LEFT,
    HUD_TEXT_LEFT_CENTER,
    HUD_TEXT_CENTER,
    HUD_TEXT_RIGHT_CENTER
};

//! Parses "#rrggbb" or "#rrggbbaa" into a linear-free HUD colour.
osg::Vec4 colorFromHex(const std::string& text, float fallbackAlpha = 1.0f);

//! State set shared by every Formula HUD drawable: unlit, blended, depth independent.
osg::StateSet* createHudStateSet();

//! Immediate-mode triangle sink bound to one osg::Geometry.
class ShapeMesh
{
public:
    ShapeMesh();
    ~ShapeMesh();

    void begin();
    void end();

    void addTriangle(const osg::Vec3& a, const osg::Vec3& b, const osg::Vec3& c,
                     const osg::Vec4& color);
    void addQuad(const osg::Vec3& a, const osg::Vec3& b, const osg::Vec3& c,
                 const osg::Vec3& d, const osg::Vec4& color);
    void addRect(float x, float y, float width, float height, const osg::Vec4& color);
    void addRoundedRect(float x, float y, float width, float height, float radius,
                        const osg::Vec4& color);
    void addRoundedOutline(float x, float y, float width, float height, float radius,
                           float thickness, const osg::Vec4& color);
    void addOutlineRect(float x, float y, float width, float height, float thickness,
                        const osg::Vec4& color);
    void addRotatedRect(float centerX, float centerY, float width, float height,
                        float angleRadians, const osg::Vec4& color);
    //! Annular band from `innerRadius` to `outerRadius` between two angles (radians,
    //! counter-clockwise from +x). Used by the tachometer scale.
    void addArcBand(float centerX, float centerY, float innerRadius, float outerRadius,
                    float startRadians, float endRadians, const osg::Vec4& color,
                    int segments = 64);
    void addCircle(float centerX, float centerY, float radius, const osg::Vec4& color,
                   int segments = 32);
    //! Polyline drawn as a quad strip, so the width is honoured on every GL profile.
    void addPolyline(const std::vector<osg::Vec2>& points, float width,
                     const osg::Vec4& color, bool closed = false);
    //! Convex polygon (triangle fan around the first point).
    void addPolygon(const std::vector<osg::Vec2>& points, const osg::Vec4& color);

    osg::Geometry* geometry() const { return _geometry.get(); }

private:
    std::vector<osg::Vec3> _positions;
    std::vector<osg::Vec4> _colors;
    osg::ref_ptr<osg::Vec3Array> _vertices;
    osg::ref_ptr<osg::Vec4Array> _vertexColors;
    osg::ref_ptr<osg::Geometry> _geometry;
};

//! Creates a HUD text drawable with a Libron face and the given alignment.
osgText::Text* createHudText(osgText::Font* font, float sizePx, const osg::Vec4& color,
                             const std::string& text = std::string(),
                             HudTextAlignment alignment = HUD_TEXT_LEFT_CENTER);

//! Updates text, colour and position only when something changed.
void updateHudText(osgText::Text* text, const std::string& value, const osg::Vec4& color,
                   float x, float y);

//! Creates the geode that carries the shapes of one panel.
osg::Geode* createShapeGeode(const std::string& name, ShapeMesh& mesh);

} // namespace formulahud
} // namespace osggraph

#endif // _FORMULA_HUD_DRAW_H_
