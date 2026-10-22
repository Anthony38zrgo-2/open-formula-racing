/***************************************************************************

    file        : FormulaHudDraw.cpp
    created     : 2026-10-22
    copyright   : (C) 2026 Open Formula

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgText/Font>
#include <osgText/Text>

#include "FormulaHudDraw.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/LineWidth>
#include <osg/PrimitiveSet>

namespace osggraph
{
namespace formulahud
{

namespace
{
    const float kPi = 3.14159265358979323846f;
    const float kTwoPi = 6.28318530717958647692f;

    int hexDigit(char character)
    {
        if (character >= '0' && character <= '9')
            return character - '0';
        if (character >= 'a' && character <= 'f')
            return character - 'a' + 10;
        if (character >= 'A' && character <= 'F')
            return character - 'A' + 10;
        return -1;
    }

    float hexPair(const std::string& text, size_t offset)
    {
        const int high = hexDigit(text[offset]);
        const int low = hexDigit(text[offset + 1]);
        if (high < 0 || low < 0)
            return 0.0f;
        return (high * 16 + low) / 255.0f;
    }
}

osg::Vec4 colorFromHex(const std::string& text, float fallbackAlpha)
{
    if (text.size() >= 7 && text[0] == '#')
    {
        const float red = hexPair(text, 1);
        const float green = hexPair(text, 3);
        const float blue = hexPair(text, 5);
        float alpha = fallbackAlpha;
        if (text.size() >= 9)
            alpha = hexPair(text, 7);
        return osg::Vec4(red, green, blue, alpha);
    }
    return osg::Vec4(1.0f, 1.0f, 1.0f, fallbackAlpha);
}

osg::StateSet* createHudStateSet()
{
    osg::StateSet* stateset = new osg::StateSet;
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateset->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA),
                                   osg::StateAttribute::ON);
    stateset->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS, 0.0, 1.0, false),
                                   osg::StateAttribute::ON);
    stateset->setRenderBinDetails(10, "RenderBin");
    return stateset;
}

ShapeMesh::ShapeMesh()
{
    _vertices = new osg::Vec3Array;
    _vertexColors = new osg::Vec4Array;
    _geometry = new osg::Geometry;
    _geometry->setVertexArray(_vertices.get());
    _geometry->setColorArray(_vertexColors.get(), osg::Array::BIND_PER_VERTEX);
    _geometry->setUseDisplayList(false);
    _geometry->setUseVertexBufferObjects(true);
    _geometry->setDataVariance(osg::Object::DYNAMIC);
}

ShapeMesh::~ShapeMesh()
{
}

void ShapeMesh::begin()
{
    _positions.clear();
    _colors.clear();
}

void ShapeMesh::end()
{
    _vertices->assign(_positions.begin(), _positions.end());
    _vertexColors->assign(_colors.begin(), _colors.end());
    _geometry->removePrimitiveSet(0, _geometry->getNumPrimitiveSets());
    if (!_positions.empty())
        _geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0,
                                                       (GLsizei)_positions.size()));
    _vertices->dirty();
    _vertexColors->dirty();
    _geometry->dirtyBound();
}

void ShapeMesh::addTriangle(const osg::Vec3& a, const osg::Vec3& b, const osg::Vec3& c,
                            const osg::Vec4& color)
{
    _positions.push_back(a);
    _positions.push_back(b);
    _positions.push_back(c);
    _colors.push_back(color);
    _colors.push_back(color);
    _colors.push_back(color);
}

void ShapeMesh::addQuad(const osg::Vec3& a, const osg::Vec3& b, const osg::Vec3& c,
                        const osg::Vec3& d, const osg::Vec4& color)
{
    addTriangle(a, b, c, color);
    addTriangle(a, c, d, color);
}

void ShapeMesh::addRect(float x, float y, float width, float height, const osg::Vec4& color)
{
    if (width <= 0.0f || height <= 0.0f)
        return;
    const float z = 0.0f;
    addQuad(osg::Vec3(x, y, z), osg::Vec3(x + width, y, z),
            osg::Vec3(x + width, y + height, z), osg::Vec3(x, y + height, z), color);
}

void ShapeMesh::addRoundedRect(float x, float y, float width, float height, float radius,
                               const osg::Vec4& color)
{
    if (width <= 0.0f || height <= 0.0f)
        return;
    const float r = std::min(radius, std::min(width, height) * 0.5f);
    if (r <= 0.0f)
    {
        addRect(x, y, width, height, color);
        return;
    }
    addRect(x + r, y, width - 2.0f * r, height, color);
    addRect(x, y + r, r, height - 2.0f * r, color);
    addRect(x + width - r, y + r, r, height - 2.0f * r, color);
    const float centers[4][2] = {
        { x + r, y + r }, { x + width - r, y + r },
        { x + width - r, y + height - r }, { x + r, y + height - r }
    };
    const float startAngles[4] = { kPi, kPi * 1.5f, 0.0f, kPi * 0.5f };
    for (int corner = 0; corner < 4; ++corner)
    {
        const float base = startAngles[corner];
        const int segments = 6;
        for (int segment = 0; segment < segments; ++segment)
        {
            const float a0 = base + (float)segment / (float)segments * kPi * 0.5f;
            const float a1 = base + (float)(segment + 1) / (float)segments * kPi * 0.5f;
            addTriangle(osg::Vec3(centers[corner][0], centers[corner][1], 0.0f),
                        osg::Vec3(centers[corner][0] + std::cos(a0) * r,
                                  centers[corner][1] + std::sin(a0) * r, 0.0f),
                        osg::Vec3(centers[corner][0] + std::cos(a1) * r,
                                  centers[corner][1] + std::sin(a1) * r, 0.0f),
                        color);
        }
    }
}

void ShapeMesh::addRoundedOutline(float x, float y, float width, float height, float radius,
                                  float thickness, const osg::Vec4& color)
{
    if (width <= 0.0f || height <= 0.0f)
        return;
    // A one-pixel frame around a rounded rectangle: the straight sides plus four short
    // arcs approximated with quads, which is enough at HUD scales.
    const float r = std::min(radius, std::min(width, height) * 0.5f);
    const float t = std::min(thickness, std::min(width, height) * 0.25f);
    addRect(x + r, y + height - t, width - 2.0f * r, t, color);
    addRect(x + r, y, width - 2.0f * r, t, color);
    addRect(x, y + r, t, height - 2.0f * r, color);
    addRect(x + width - t, y + r, t, height - 2.0f * r, color);
    const float centers[4][2] = {
        { x + r, y + r }, { x + width - r, y + r },
        { x + width - r, y + height - r }, { x + r, y + height - r }
    };
    const float startAngles[4] = { kPi, kPi * 1.5f, 0.0f, kPi * 0.5f };
    for (int corner = 0; corner < 4; ++corner)
    {
        const float base = startAngles[corner];
        const int segments = 6;
        for (int segment = 0; segment < segments; ++segment)
        {
            const float a0 = base + (float)segment / (float)segments * kPi * 0.5f;
            const float a1 = base + (float)(segment + 1) / (float)segments * kPi * 0.5f;
            for (int ring = 0; ring < 1; ++ring)
            {
                const float inner = r - t;
                const float outer = r;
                addQuad(osg::Vec3(centers[corner][0] + std::cos(a0) * inner,
                                  centers[corner][1] + std::sin(a0) * inner, 0.0f),
                        osg::Vec3(centers[corner][0] + std::cos(a0) * outer,
                                  centers[corner][1] + std::sin(a0) * outer, 0.0f),
                        osg::Vec3(centers[corner][0] + std::cos(a1) * outer,
                                  centers[corner][1] + std::sin(a1) * outer, 0.0f),
                        osg::Vec3(centers[corner][0] + std::cos(a1) * inner,
                                  centers[corner][1] + std::sin(a1) * inner, 0.0f),
                        color);
            }
        }
    }
}

void ShapeMesh::addOutlineRect(float x, float y, float width, float height, float thickness,
                               const osg::Vec4& color)
{
    if (width <= 0.0f || height <= 0.0f)
        return;
    const float t = std::min(thickness, std::min(width, height) * 0.5f);
    addRect(x, y, width, t, color);
    addRect(x, y + height - t, width, t, color);
    addRect(x, y + t, t, height - 2.0f * t, color);
    addRect(x + width - t, y + t, t, height - 2.0f * t, color);
}

void ShapeMesh::addRotatedRect(float centerX, float centerY, float width, float height,
                               float angleRadians, const osg::Vec4& color)
{
    const float cosine = std::cos(angleRadians);
    const float sine = std::sin(angleRadians);
    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;
    const osg::Vec2 corners[4] = {
        osg::Vec2(-halfWidth, -halfHeight),
        osg::Vec2(halfWidth, -halfHeight),
        osg::Vec2(halfWidth, halfHeight),
        osg::Vec2(-halfWidth, halfHeight)
    };
    osg::Vec3 points[4];
    for (int corner = 0; corner < 4; ++corner)
    {
        const float localX = corners[corner].x() * cosine - corners[corner].y() * sine;
        const float localY = corners[corner].x() * sine + corners[corner].y() * cosine;
        points[corner] = osg::Vec3(centerX + localX, centerY + localY, 0.0f);
    }
    addQuad(points[0], points[1], points[2], points[3], color);
}

void ShapeMesh::addArcBand(float centerX, float centerY, float innerRadius, float outerRadius,
                           float startRadians, float endRadians, const osg::Vec4& color,
                           int segments)
{
    if (segments < 2 || outerRadius <= innerRadius)
        return;
    const float step = (endRadians - startRadians) / (float)segments;
    for (int segment = 0; segment < segments; ++segment)
    {
        const float angle0 = startRadians + step * (float)segment;
        const float angle1 = angle0 + step;
        const osg::Vec3 inner0(centerX + std::cos(angle0) * innerRadius,
                               centerY + std::sin(angle0) * innerRadius, 0.0f);
        const osg::Vec3 outer0(centerX + std::cos(angle0) * outerRadius,
                               centerY + std::sin(angle0) * outerRadius, 0.0f);
        const osg::Vec3 inner1(centerX + std::cos(angle1) * innerRadius,
                               centerY + std::sin(angle1) * innerRadius, 0.0f);
        const osg::Vec3 outer1(centerX + std::cos(angle1) * outerRadius,
                               centerY + std::sin(angle1) * outerRadius, 0.0f);
        addQuad(inner0, outer0, outer1, inner1, color);
    }
}

void ShapeMesh::addCircle(float centerX, float centerY, float radius, const osg::Vec4& color,
                          int segments)
{
    if (radius <= 0.0f || segments < 3)
        return;
    const osg::Vec3 center(centerX, centerY, 0.0f);
    for (int segment = 0; segment < segments; ++segment)
    {
        const float angle0 = (float)segment / (float)segments * kTwoPi;
        const float angle1 = (float)(segment + 1) / (float)segments * kTwoPi;
        addTriangle(center,
                    osg::Vec3(centerX + std::cos(angle0) * radius,
                              centerY + std::sin(angle0) * radius, 0.0f),
                    osg::Vec3(centerX + std::cos(angle1) * radius,
                              centerY + std::sin(angle1) * radius, 0.0f),
                    color);
    }
}

void ShapeMesh::addPolyline(const std::vector<osg::Vec2>& points, float width,
                            const osg::Vec4& color, bool closed)
{
    if (points.size() < 2 || width <= 0.0f)
        return;
    const float halfWidth = width * 0.5f;
    const size_t count = points.size();
    const size_t segments = closed ? count : count - 1;
    for (size_t segment = 0; segment < segments; ++segment)
    {
        const osg::Vec2& start = points[segment];
        const osg::Vec2& end = points[(segment + 1) % count];
        osg::Vec2 direction(end.x() - start.x(), end.y() - start.y());
        const float length = std::sqrt(direction.x() * direction.x()
                                       + direction.y() * direction.y());
        if (length < 1.0e-4f)
            continue;
        direction = osg::Vec2(direction.x() / length, direction.y() / length);
        const osg::Vec2 normal(-direction.y() * halfWidth, direction.x() * halfWidth);
        addQuad(osg::Vec3(start.x() + normal.x(), start.y() + normal.y(), 0.0f),
                osg::Vec3(end.x() + normal.x(), end.y() + normal.y(), 0.0f),
                osg::Vec3(end.x() - normal.x(), end.y() - normal.y(), 0.0f),
                osg::Vec3(start.x() - normal.x(), start.y() - normal.y(), 0.0f), color);
        // Round joins keep corners from pinching on the minimap outline.
        if (segment + 1 < segments || closed)
        {
            const osg::Vec2& corner = points[(segment + 1) % count];
            addCircle(corner.x(), corner.y(), halfWidth, color, 8);
        }
    }
}

void ShapeMesh::addPolygon(const std::vector<osg::Vec2>& points, const osg::Vec4& color)
{
    if (points.size() < 3)
        return;
    for (size_t index = 1; index + 1 < points.size(); ++index)
        addTriangle(osg::Vec3(points[0].x(), points[0].y(), 0.0f),
                    osg::Vec3(points[index].x(), points[index].y(), 0.0f),
                    osg::Vec3(points[index + 1].x(), points[index + 1].y(), 0.0f), color);
}

namespace
{
    osgText::Text::AlignmentType toOsgAlignment(HudTextAlignment alignment)
    {
        switch (alignment)
        {
        case HUD_TEXT_LEFT: return osgText::Text::LEFT_CENTER;
        case HUD_TEXT_CENTER: return osgText::Text::CENTER_CENTER;
        case HUD_TEXT_RIGHT_CENTER: return osgText::Text::RIGHT_CENTER;
        default: return osgText::Text::LEFT_CENTER;
        }
    }
}

osgText::Text* createHudText(osgText::Font* font, float sizePx, const osg::Vec4& color,
                             const std::string& text, HudTextAlignment alignment)
{
    osgText::Text* drawable = new osgText::Text;
    drawable->setFont(font);
    drawable->setFontResolution(200, 200);
    drawable->setCharacterSize(sizePx);
    drawable->setAlignment(toOsgAlignment(alignment));
    drawable->setColor(color);
    drawable->setBackdropType(osgText::Text::NONE);
    drawable->setDataVariance(osg::Object::DYNAMIC);
    drawable->setText(text, osgText::String::ENCODING_UTF8);
    return drawable;
}

void updateHudText(osgText::Text* text, const std::string& value, const osg::Vec4& color,
                   float x, float y)
{
    if (!text)
        return;
    if (text->getText().createUTF8EncodedString() != value)
        text->setText(value, osgText::String::ENCODING_UTF8);
    if (text->getColor() != color)
        text->setColor(color);
    const osg::Vec3 current = text->getPosition();
    if (current.x() != x || current.y() != y)
        text->setPosition(osg::Vec3(x, y, 0.0f));
}

osg::Geode* createShapeGeode(const std::string& name, ShapeMesh& mesh)
{
    osg::Geode* geode = new osg::Geode;
    geode->setName(name);
    geode->setStateSet(createHudStateSet());
    geode->addDrawable(mesh.geometry());
    return geode;
}

} // namespace formulahud
} // namespace osggraph
