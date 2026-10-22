/***************************************************************************

    file        : FormulaHudLayout.cpp
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

#include "FormulaHudLayout.h"

#include <algorithm>
#include <cmath>

#include <tgf.h>

namespace osggraph
{
namespace formulahud
{

namespace
{
    //! Design viewport the default rectangles were measured on. Every default below is
    //! expressed in these pixels and scaled to the visible area at run time.
    const float kDesignWidth = 1920.0f;
    const float kDesignHeight = 1080.0f;

    //! Formula90s geometry, converted from its 1080p composition.
    const float kRightPanelWidth = 541.35f; // preserve content scale with 24 px side padding
    const float kRightPanelHeight = 370.0125f;
    // The wheel panel expands with its content in the original; these are the measured
    // laid-out sizes (402x297 design pixels at scale 1.188), not the nominal minimum.
    const float kTirePanelWidth = 330.0f;
    const float kTirePanelHeight = 230.0f;
    const float kTimingPanelWidth = 261.36f;   // 220 * 1.188
    // The original panel is 132 design pixels tall for the title plus LAP/LAST/BEST.
    // The migration adds the requested current-lap row, so the block is 167.
    const float kTimingPanelHeight = 198.4f;   // 167 * 1.188
    const float kMinimapWidth = 243.0f;        // 108 * 2.25
    const float kMinimapHeight = 288.0f;       // 128 * 2.25
    const float kControlsWidth = 435.6f;
    const float kControlsHeight = 127.6f;

    float readNumber(void* handle, const char* section, const char* name, float fallback)
    {
        if (!handle || !section || !name)
            return fallback;
        const double value = GfParmGetNum(handle, section, name, nullptr, fallback);
        if (!std::isfinite(value))
            return fallback;
        return (float)value;
    }
}

Rect::Rect()
    : x(0.0f), y(0.0f), width(0.0f), height(0.0f)
{
}

Rect::Rect(float left, float bottom, float w, float h)
    : x(left), y(bottom), width(w), height(h)
{
}

bool Rect::intersects(const Rect& other, float margin) const
{
    return x - margin < other.right() && other.x - margin < right()
        && y - margin < other.top() && other.y - margin < top();
}

LayoutConfig::LayoutConfig()
    : enabled(true)
    , scale(1.0f)
    , minimumScale(0.72f)
    , maximumScale(2.50f)
    , leftMargin(14.0f)
    , topMargin(10.0f)
    , rightMargin(16.0f)
    , bottomMargin(48.0f)
    , blockPadding(12.0f)
    , panelGap(10.0f)
    , mapGap(12.0f)
    , controlsBottomMargin(18.0f)
    , controlsMinimumScale(0.65f)
    , minimumLabelFontPx(10.0f)
    , minimumValueFontPx(13.0f)
    , fontSizeBoost(1.0f)
{
}

void LayoutConfig::load()
{
    void* handle = GfParmReadFileBoth("config/formula_hud.xml", GFPARM_RMODE_STD);
    if (!handle)
    {
        GfLogInfo("Formula HUD: config/formula_hud.xml not found, using built-in layout\n");
        return;
    }
    const char* section = "FormulaHud";
    enabled = readNumber(handle, section, "enabled", enabled ? 1.0f : 0.0f) > 0.5f;
    scale = std::max(0.10f, readNumber(handle, section, "scale", scale));
    minimumScale = std::max(0.10f, readNumber(handle, section, "minimum scale", minimumScale));
    maximumScale = std::max(minimumScale, readNumber(handle, section, "maximum scale", maximumScale));
    leftMargin = std::max(0.0f, readNumber(handle, section, "left margin", leftMargin));
    topMargin = std::max(0.0f, readNumber(handle, section, "top margin", topMargin));
    rightMargin = std::max(0.0f, readNumber(handle, section, "right margin", rightMargin));
    bottomMargin = std::max(0.0f, readNumber(handle, section, "bottom margin", bottomMargin));
    blockPadding = std::max(0.0f, readNumber(handle, section, "block padding", blockPadding));
    panelGap = std::max(0.0f, readNumber(handle, section, "panel gap", panelGap));
    mapGap = std::max(0.0f, readNumber(handle, section, "map gap", mapGap));
    controlsBottomMargin = std::max(0.0f, readNumber(handle, section, "controls bottom margin", controlsBottomMargin));
    controlsMinimumScale = std::max(0.10f, readNumber(handle, section, "controls minimum scale", controlsMinimumScale));
    minimumLabelFontPx = std::max(6.0f, readNumber(handle, section, "minimum label font", minimumLabelFontPx));
    minimumValueFontPx = std::max(6.0f, readNumber(handle, section, "minimum value font", minimumValueFontPx));
    fontSizeBoost = std::min(2.0f, std::max(0.5f, readNumber(handle, section, "font size boost", fontSizeBoost)));
    GfLogInfo("Formula HUD: layout scale=%.3f min=%.3f margins=(%.1f,%.1f,%.1f,%.1f) gaps=(%.1f,%.1f)\n",
              scale, minimumScale, leftMargin, topMargin, rightMargin, bottomMargin,
              panelGap, mapGap);
    GfParmReleaseHandle(handle);
}

Layout::Layout()
    : screenWidth(0.0f)
    , screenHeight(0.0f)
    , uiScale(1.0f)
    , labelFontPx(10.0f)
    , valueFontPx(13.0f)
{
}

void Layout::compute(const LayoutConfig& config, int viewportWidth, int viewportHeight)
{
    screenWidth = (float)std::max(1, viewportWidth);
    screenHeight = (float)std::max(1, viewportHeight);

    // Uniform scale from the design viewport, clamped so text never becomes unreadable
    // and never grows without bound on very large displays.
    const float widthScale = screenWidth / kDesignWidth;
    const float heightScale = screenHeight / kDesignHeight;
    const float baseScale = std::min(widthScale, heightScale);
    uiScale = std::min(config.maximumScale,
                       std::max(config.minimumScale, baseScale * config.scale));

    // The wheel panel and the upper-left map must remain separate on short viewports.
    const float rightGroupDesignHeight = kTirePanelHeight + 310.0f
        + config.panelGap + config.topMargin + config.bottomMargin;
    if (rightGroupDesignHeight > 0.0f)
    {
        const float scaleThatFits = screenHeight / rightGroupDesignHeight;
        if (scaleThatFits < uiScale)
            uiScale = std::max(0.20f, scaleThatFits);
    }

    const float marginLeft = std::max(24.0f, config.leftMargin * uiScale);
    const float marginRight = std::max(24.0f, config.rightMargin * uiScale);
    const float marginBottom = std::max(24.0f, config.bottomMargin * uiScale);
    const float panelGap = config.panelGap * uiScale;

    // --- right block: tachometer/speed/fuel/engine temperatures at the bottom-right ---
    // The HUD canvas has its origin at the bottom-left corner, so `y` is the bottom edge.
    Rect right(marginLeft, marginBottom, kRightPanelWidth * uiScale,
               kRightPanelHeight * uiScale);
    right.x = std::max(marginLeft, screenWidth - marginRight - right.width);
    rightPanel = right;

    // Requested composition: wheels bottom-left; oil/water in the free upper-right
    // corner of the tachometer. Engine is a subrectangle of rightPanel.
    tirePanel = Rect(marginLeft, marginBottom, kTirePanelWidth * uiScale,
                     kTirePanelHeight * uiScale);
    enginePanel = Rect(right.right() - 158.0f * uiScale, right.top() - 90.0f * uiScale,
                       130.0f * uiScale, 74.0f * uiScale);

    // One panel combines the map and Air/Time at the upper-right, inset from both edges.
    timingPanel = Rect();
    timingBackground = Rect();
    const float mapBlockHeight = 240.0f * uiScale;
    const float mapMargin = std::max(24.0f, 32.0f * uiScale);
    const float mapBlockLeft = screenWidth - mapMargin - 300.0f * uiScale;
    const float mapBlockBottom = screenHeight - mapMargin - mapBlockHeight;
    minimapBackground = Rect(mapBlockLeft, mapBlockBottom, 300.0f * uiScale, mapBlockHeight);
    minimapPanel = Rect(mapBlockLeft + 12.0f * uiScale, mapBlockBottom + 56.0f * uiScale,
                        276.0f * uiScale, 180.0f * uiScale);

    // --- bottom centre: steering wheel and pedal indicators ---
    const float controlsScale = std::min(1.0f, std::max(config.controlsMinimumScale,
                                                        screenWidth / 1280.0f)) * uiScale;
    // Every bottom panel shares the same raised baseline.
    Rect controls((screenWidth - kControlsWidth * controlsScale) * 0.5f,
                  marginBottom,
                  kControlsWidth * controlsScale,
                  kControlsHeight * controlsScale);
    // Never let the controls crawl into the tachometer: keep a gap to the right block.
    if (controls.right() + panelGap > rightPanel.x)
        controls.x = std::max(marginLeft, rightPanel.x - panelGap - controls.width);
    controlsPanel = controls;

    // Minimum text sizes are enforced in device pixels, so a smaller window reduces the
    // layout but never the legibility of the labels.
    labelFontPx = std::max(config.minimumLabelFontPx, 13.0f * uiScale * config.fontSizeBoost);
    valueFontPx = std::max(config.minimumValueFontPx, 15.0f * uiScale * config.fontSizeBoost);

    GfLogDebug("Formula HUD: viewport=%.0fx%.0f uiScale=%.3f right=(%.1f,%.1f %.1fx%.1f) "
               "tires=(%.1f,%.1f %.1fx%.1f) timing=(%.1f,%.1f %.1fx%.1f) map=(%.1f,%.1f %.1fx%.1f) "
               "controls=(%.1f,%.1f %.1fx%.1f)\n",
               screenWidth, screenHeight, uiScale,
               rightPanel.x, rightPanel.y, rightPanel.width, rightPanel.height,
               tirePanel.x, tirePanel.y, tirePanel.width, tirePanel.height,
               timingPanel.x, timingPanel.y, timingPanel.width, timingPanel.height,
               minimapPanel.x, minimapPanel.y, minimapPanel.width, minimapPanel.height,
               controlsPanel.x, controlsPanel.y, controlsPanel.width, controlsPanel.height);
}

} // namespace formulahud
} // namespace osggraph
