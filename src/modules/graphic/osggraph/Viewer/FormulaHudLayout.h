/***************************************************************************

    file        : FormulaHudLayout.h
    created     : 2026-10-22
    copyright   : (C) 2026 Open Formula

    Layout of the Formula HUD: anchors, margins, scale, minimum text sizes and block
    separation. The default values reproduce the Formula90s composition at a 1920x1080
    design viewport; `config/formula_hud.xml` can override them without recompiling.

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _FORMULA_HUD_LAYOUT_H_
#define _FORMULA_HUD_LAYOUT_H_

#include <string>

namespace osggraph
{
namespace formulahud
{

//! Axis aligned rectangle in HUD pixels. The origin is the bottom-left corner, matching
//! the orthographic HUD camera of osggraph (y grows upwards).
struct Rect
{
    float x;
    float y;
    float width;
    float height;

    Rect();
    Rect(float left, float bottom, float w, float h);

    float right() const { return x + width; }
    float top() const { return y + height; }
    float centerX() const { return x + width * 0.5f; }
    bool intersects(const Rect& other, float margin) const;
};

//! Tunable layout parameters, in design pixels of a 1920x1080 viewport.
struct LayoutConfig
{
    LayoutConfig();

    //! Reads `<local>/config/formula_hud.xml` then `<data>/config/formula_hud.xml`.
    //! Missing entries keep the built-in defaults; unreadable values are ignored.
    void load();

    bool enabled;
    float scale;                 //!< User multiplier applied to the computed scale.
    float minimumScale;
    float maximumScale;
    float leftMargin;
    float topMargin;
    float rightMargin;
    float bottomMargin;
    float blockPadding;
    float panelGap;
    float mapGap;
    float controlsBottomMargin;
    float controlsMinimumScale;
    float minimumLabelFontPx;
    float minimumValueFontPx;
    float fontSizeBoost;         //!< Global multiplier on every design font size.
};

//! Resolved rectangles of every migrated group for one viewport size.
struct Layout
{
    Layout();

    //! Recomputes every rectangle from the configuration and the visible area.
    void compute(const LayoutConfig& config, int viewportWidth, int viewportHeight);

    float screenWidth;
    float screenHeight;
    float uiScale;
    float labelFontPx;           //!< Smallest label size actually used (>= config minimum).
    float valueFontPx;           //!< Smallest value size actually used (>= config minimum).

    Rect timingPanel;            //!< Lap timing block (top-left).
    Rect minimapPanel;           //!< Track outline within the upper-right map/Air/Time panel.
    Rect minimapBackground;      //!< Shared map/Air/Time panel, inset from screen edges.
    Rect timingBackground;       //!< Shared translucent background of the two above.
    Rect rightPanel;             //!< Tachometer, speed and fuel.
    Rect enginePanel;            //!< Engine temperatures inside the tachometer.
    Rect tirePanel;              //!< Four wheel thermal panel, above the right panel.
    Rect controlsPanel;          //!< Steering wheel and pedals (bottom centre).
};

} // namespace formulahud
} // namespace osggraph

#endif // _FORMULA_HUD_LAYOUT_H_
