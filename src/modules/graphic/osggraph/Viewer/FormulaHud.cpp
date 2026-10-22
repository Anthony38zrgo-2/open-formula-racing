/***************************************************************************

    file        : FormulaHud.cpp
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

#include "FormulaHud.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

#include <osg/Geode>
#include <osg/StateSet>

#include <tgf.h>
#include <track.h>

namespace osggraph
{
// Provided by OsgMain.cpp: the track model of the running session.
extern const tTrack* getTrackModel();

namespace formulahud
{

namespace
{
    //! Colours of the Formula90s HUD, kept exactly as published by its own configuration
    //! (`retro_hud.json`, `hud_config.json`) so the migrated components keep their look.
    struct Palette
    {
        osg::Vec4 background;
        osg::Vec4 dial;
        osg::Vec4 digital;
        osg::Vec4 inactive;
        osg::Vec4 redline;
        osg::Vec4 redlineInactive;
        osg::Vec4 alert;
        osg::Vec4 divider;
        osg::Vec4 fuelContainer;
        osg::Vec4 fuelSegmentOff;
        osg::Vec4 fuelSegmentOn;
        osg::Vec4 cold;
        osg::Vec4 optimal;
        osg::Vec4 warm;
        osg::Vec4 hot;
        osg::Vec4 critical;
        osg::Vec4 pending;
        osg::Vec4 lastLap;
        osg::Vec4 bestLap;
        osg::Vec4 controlsBackground;
        osg::Vec4 controlsBorder;
        osg::Vec4 throttleColor;
        osg::Vec4 brakeColor;
        osg::Vec4 steeringColor;
        osg::Vec4 label;
        osg::Vec4 pedalLabel;
        osg::Vec4 barBackground;
        osg::Vec4 wheelRing;
        osg::Vec4 wheelSpoke;
        osg::Vec4 wheelHub;
        osg::Vec4 mapOutline;
        osg::Vec4 mapShadow;
        osg::Vec4 mapStart;
        osg::Vec4 mapPlayer;
        osg::Vec4 mapPlayerOutline;
    };

    const Palette& palette()
    {
        static const Palette colors = {
            colorFromHex("#06152566"),          // background (alpha 0x66)
            colorFromHex("#f4f5f6"),            // dial
            colorFromHex("#f4f5f6"),            // digital
            colorFromHex("#183149"),            // inactive
            colorFromHex("#f0182d"),            // redline
            colorFromHex("#67202b"),            // inactive redline
            colorFromHex("#f0182d"),            // alert
            colorFromHex("#ffffff"),            // divider
            colorFromHex("#9aa5ae"),            // fuel container
            colorFromHex("#4a555e"),            // inactive fuel segment
            colorFromHex("#f2f3f1"),            // active fuel segment
            colorFromHex("#73b8ff"),            // cold
            colorFromHex("#a6ff9e"),            // optimal
            colorFromHex("#ffd16b"),            // warm
            colorFromHex("#ff6b61"),            // hot
            colorFromHex("#d91f1f"),            // critical
            colorFromHex("#cfd6e4"),            // pending (timing)
            colorFromHex("#ffffff"),            // last lap
            colorFromHex("#a6ff9e"),            // best lap
            osg::Vec4(0.025f, 0.035f, 0.05f, 0.90f),
            osg::Vec4(0.45f, 0.52f, 0.62f, 0.45f),
            colorFromHex("#48df94"),            // throttle
            colorFromHex("#ff665c"),            // brake
            colorFromHex("#ffd477"),            // steering
            osg::Vec4(0.78f, 0.83f, 0.90f, 1.0f),
            osg::Vec4(0.88f, 0.92f, 0.98f, 1.0f),
            osg::Vec4(0.16f, 0.20f, 0.26f, 1.0f),
            osg::Vec4(0.92f, 0.95f, 1.00f, 1.0f),
            osg::Vec4(0.70f, 0.76f, 0.85f, 1.0f),
            osg::Vec4(0.45f, 0.51f, 0.61f, 1.0f),
            osg::Vec4(0.97f, 0.98f, 0.94f, 0.98f),
            osg::Vec4(0.01f, 0.02f, 0.03f, 0.92f),
            osg::Vec4(0.28f, 1.00f, 0.34f, 1.0f),
            osg::Vec4(0.20f, 0.50f, 1.00f, 1.0f),
            osg::Vec4(0.01f, 0.02f, 0.08f, 0.98f)
        };
        return colors;
    }

    //! Design canvases of the migrated panels, in the units of the original scripts.
    const float kRightDesignWidth = 1448.0f;
    const float kRightDesignHeight = 1086.0f;
    const float kTireDesignWidth = 402.0f;
    const float kTireDesignHeight = 297.0f;
    const float kTimingDesignWidth = 220.0f;
    const float kTimingDesignHeight = 167.0f;
    const float kControlsDesignWidth = 396.0f;
    const float kControlsDesignHeight = 116.0f;

    const float kPi = 3.14159265358979323846f;
    const float kDegToRad = kPi / 180.0f;

    osg::Vec4 thermalColor(ThermalState state)
    {
        const Palette& colors = palette();
        switch (state)
        {
        case THERMAL_COLD: return colors.cold;
        case THERMAL_OPTIMAL: return colors.optimal;
        case THERMAL_WARM: return colors.warm;
        case THERMAL_HOT: return colors.hot;
        case THERMAL_CRITICAL: return colors.critical;
        default: return colors.pending;
        }
    }

    //! Hottest of three readings, ignoring non-finite samples so one unavailable sensor
    //! does not blank the state colour of the other two.
    double hottestOfThree(double a, double b, double c)
    {
        double hottest = 0.0;
        bool found = false;
        const double values[3] = { a, b, c };
        for (int index = 0; index < 3; ++index)
        {
            if (!std::isfinite(values[index]))
                continue;
            if (!found || values[index] > hottest)
            {
                hottest = values[index];
                found = true;
            }
        }
        return found ? hottest : std::numeric_limits<double>::quiet_NaN();
    }

    float fitFontSize(const Fonts& fonts, bool bold, const std::string& text, float maxWidth,
                      float desiredSize)
    {
        if (maxWidth <= 0.0f)
            return desiredSize;
        const float width = fonts.measure(bold, text, desiredSize);
        if (width <= maxWidth || width <= 0.0f)
            return desiredSize;
        return std::max(6.0f, desiredSize * maxWidth / width);
    }
}

HudPanel::HudPanel()
{
}

PanelXform::PanelXform()
    : scale(1.0f), left(0.0f), bottom(0.0f), designWidth(1.0f), designHeight(1.0f)
{
}

PanelXform::PanelXform(const Rect& rect, float designWidthIn, float designHeightIn)
    : scale(1.0f), left(rect.x), bottom(rect.y)
    , designWidth(designWidthIn), designHeight(designHeightIn)
{
    if (designWidthIn <= 0.0f || designHeightIn <= 0.0f)
        return;
    scale = std::min(rect.width / designWidthIn, rect.height / designHeightIn);
    left = rect.x + (rect.width - designWidthIn * scale) * 0.5f;
    bottom = rect.y + (rect.height - designHeightIn * scale) * 0.5f;
}

osg::Vec2 PanelXform::point(float x, float yDown) const
{
    return osg::Vec2(left + x * scale, bottom + (designHeight - yDown) * scale);
}

FormulaHud::FormulaHud()
    : _timingTitle(nullptr)
    , _gearText(nullptr)
    , _speedText(nullptr)
    , _speedUnitText(nullptr)
    , _rpmText(nullptr)
    , _rpmUnitText(nullptr)
    , _oilLabel(nullptr)
    , _oilValue(nullptr)
    , _oilUnit(nullptr)
    , _waterLabel(nullptr)
    , _waterValue(nullptr)
    , _waterUnit(nullptr)
    , _fuelLabel(nullptr)
    , _fuelValue(nullptr)
    , _fuelUnit(nullptr)
    , _fuelAverageLabel(nullptr)
    , _fuelAverageValue(nullptr)
    , _fuelAverageUnit(nullptr)
    , _fuelDeltaLabel(nullptr)
    , _fuelDeltaValue(nullptr)
    , _fuelDeltaUnit(nullptr)
    , _tireTitle(nullptr)
    , _steeringLabel(nullptr)
    , _steeringValue(nullptr)
    , _throttleLabel(nullptr)
    , _throttleValue(nullptr)
    , _brakeLabel(nullptr)
    , _brakeValue(nullptr)
    , _minimapReady(false)
    , _minimapTrack(nullptr)
    , _minimapMin(0.0f, 0.0f)
    , _minimapMax(0.0f, 0.0f)
    , _minimapStart(0.0f, 0.0f)
    , _minimapStartForward(0.0f, -1.0f)
    , _measuredSpeedWidth(0.0f)
    , _measuredRpmWidth(0.0f)
    , _speedFontPx(0.0f)
    , _rpmFontPx(0.0f)
    , _diagnostics(false)
    , _diagnosticClock(0.0)
    , _diagnosticCount(0)
    , _fontsReady(false)
{
    for (int row = 0; row < 4; ++row)
    {
        _timingCaption[row] = nullptr;
        _timingValue[row] = nullptr;
    }
    for (int corner = 0; corner < WHEEL_COUNT; ++corner)
    {
        _tireCells[corner].header = nullptr;
        _tireCells[corner].pressure = nullptr;
        _tireCells[corner].zones = nullptr;
        _tireCells[corner].carcass = nullptr;
        _tireCells[corner].gas = nullptr;
        _tireCells[corner].rim = nullptr;
        _tireCells[corner].brake = nullptr;
    }
}

FormulaHud::~FormulaHud()
{
}

void FormulaHud::setDiagnosticsEnabled(bool enabled)
{
    _diagnostics = enabled;
}

std::vector<std::pair<std::string, Rect> > FormulaHud::groupRects() const
{
    std::vector<std::pair<std::string, Rect> > rects;
    rects.push_back(std::make_pair(std::string("formulaMapAirTime"), _layout.minimapBackground));
    rects.push_back(std::make_pair(std::string("formulaRight"), _layout.rightPanel));
    rects.push_back(std::make_pair(std::string("formulaTires"), _layout.tirePanel));
    rects.push_back(std::make_pair(std::string("formulaEngine"), _layout.enginePanel));
    rects.push_back(std::make_pair(std::string("formulaControls"), _layout.controlsPanel));
    return rects;
}

namespace
{
    osg::Geode* createTextGeode(const std::string& name)
    {
        osg::Geode* geode = new osg::Geode;
        geode->setName(name);
        geode->setStateSet(createHudStateSet());
        return geode;
    }

    void attachText(osg::Geode* geode, osgText::Text* text)
    {
        text->setNodeMask(0xFFFFFFFFu);
        geode->addDrawable(text);
    }
}

bool FormulaHud::create()
{
    _config.load();
    if (!_config.enabled)
    {
        GfLogInfo("Formula HUD: disabled by configuration\n");
        return false;
    }
    if (!_fonts.load())
    {
        GfLogError("Formula HUD: Libron is required; the migrated HUD is not created\n");
        return false;
    }
    _fontsReady = true;

    _root = new osg::Group;
    _root->setName("formulaHudWidget");
    _root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    createMinimapPanel();
    createRightPanel();
    createTirePanel();
    createControlsPanel();

    GfLogInfo("Formula HUD: created with Libron Regular/Bold (panels=5)\n");
    return true;
}

void FormulaHud::setVisible(bool visible)
{
    if (_root.valid())
        _root->setNodeMask(visible ? 0xFFFFFFFFu : 0u);
}

void FormulaHud::setViewport(int width, int height)
{
    _layout.compute(_config, width, height);
    for (const std::pair<std::string, Rect>& group : groupRects())
    {
        GfLogInfo("Formula HUD group %s rect=(%.1f,%.1f %.1fx%.1f)\n", group.first.c_str(),
                  group.second.x, group.second.y, group.second.width, group.second.height);
    }
}

// ---------------------------------------------------------------------------
// Lap timing (top left, Formula90s LapTimingPanel)
// ---------------------------------------------------------------------------

void FormulaHud::createTimingPanel()
{
    _timing.shapeGeode = createShapeGeode("formulaTimingShapes", _timing.shapes);
    _timing.textGeode = createTextGeode("formulaTimingText");
    _root->addChild(_timing.shapeGeode.get());
    _root->addChild(_timing.textGeode.get());

    osgText::Font* bold = _fonts.bold();
    osgText::Font* regular = _fonts.regular();
    const Palette& colors = palette();

    _timingTitle = createHudText(bold, 18.0f, colors.dial, "LAP TIMING",
                                 HUD_TEXT_CENTER);
    attachText(_timing.textGeode.get(), _timingTitle);

    static const char* captions[4] = { "LAP", "CUR", "LAST", "BEST" };
    for (int row = 0; row < 4; ++row)
    {
        _timingCaption[row] = createHudText(regular, 15.0f, colors.label, captions[row],
                                            HUD_TEXT_CENTER);
        attachText(_timing.textGeode.get(), _timingCaption[row]);
        _timingValue[row] = createHudText(bold, 24.0f, colors.pending, "--",
                                          HUD_TEXT_RIGHT_CENTER);
        attachText(_timing.textGeode.get(), _timingValue[row]);
    }
}

void FormulaHud::updateTimingPanel(const CarReadout& readout)
{
    const PanelXform xform(_layout.timingPanel, kTimingDesignWidth, kTimingDesignHeight);
    const Palette& colors = palette();
    const float uiScale = _layout.uiScale;

    // Translucent backdrop shared with the minimap, as in the original composition.
    _timing.shapes.begin();
    _timing.shapes.addRect(_layout.timingBackground.x, _layout.timingBackground.y,
                           _layout.timingBackground.width, _layout.timingBackground.height,
                           colors.background);
    _timing.shapes.end();

    const float marginLeft = 14.0f;
    const float marginRight = 14.0f;
    const float titleSize = std::max(_layout.valueFontPx, 18.0f * xform.scale);
    const float captionSize = std::max(_layout.labelFontPx, 15.0f * xform.scale);
    const float valueSize = std::max(_layout.valueFontPx, 24.0f * xform.scale);

    if (_timingTitle->getCharacterHeight() != titleSize)
        _timingTitle->setCharacterSize(titleSize);
    const osg::Vec2 titlePoint = xform.point(kTimingDesignWidth * 0.5f, 20.0f);
    updateHudText(_timingTitle, "LAP TIMING", colors.dial, titlePoint.x(), titlePoint.y());

    static const float rowCenters[4] = { 48.0f, 78.0f, 108.0f, 138.0f };
    const std::string values[4] = {
        readout.lapValid
            // A practice session reports a placeholder lap total; only a real race total
            // is shown next to the current lap.
            ? ((readout.totalLaps > 0 && readout.totalLaps < 999)
                ? formatLapNumber(readout.lapNumber) + "/" + std::to_string(readout.totalLaps)
                : formatLapNumber(readout.lapNumber))
            : std::string("--"),
        formatLapTime(readout.currentLapValid ? readout.currentLapTime : -1.0),
        formatLapTime(readout.lastLapValid ? readout.lastLapTime : -1.0),
        formatLapTime(readout.bestLapValid ? readout.bestLapTime : -1.0)
    };
    osg::Vec4 valueColors[4];
    valueColors[0] = readout.bestLapValid ? colors.bestLap : colors.pending;
    valueColors[1] = readout.currentLapValid ? colors.lastLap : colors.pending;
    valueColors[2] = readout.lastLapValid ? colors.lastLap : colors.pending;
    valueColors[3] = readout.bestLapValid ? colors.bestLap : colors.pending;

    static const char* captionText[4] = { "LAP", "CUR", "LAST", "BEST" };
    for (int row = 0; row < 4; ++row)
    {
        if (_timingCaption[row]->getCharacterHeight() != captionSize)
            _timingCaption[row]->setCharacterSize(captionSize);
        if (_timingValue[row]->getCharacterHeight() != valueSize)
            _timingValue[row]->setCharacterSize(valueSize);
        const osg::Vec2 captionPoint = xform.point(marginLeft, rowCenters[row]);
        const osg::Vec2 valuePoint = xform.point(kTimingDesignWidth - marginRight, rowCenters[row]);
        updateHudText(_timingCaption[row], captionText[row], colors.label, captionPoint.x(),
                      captionPoint.y());
        updateHudText(_timingValue[row], values[row], valueColors[row], valuePoint.x(),
                      valuePoint.y());
    }
    (void)uiScale;
}

// ---------------------------------------------------------------------------
// Upper-right minimap with Air/Time, using the shared HUD background and Libron.
// ---------------------------------------------------------------------------

void FormulaHud::createMinimapPanel()
{
    _minimap.shapeGeode = createShapeGeode("formulaMinimapShapes", _minimap.shapes);
    _minimap.textGeode = createTextGeode("formulaMinimapText");
    _root->addChild(_minimap.shapeGeode.get());
    _root->addChild(_minimap.textGeode.get());
    _mapAirLabel = createHudText(_fonts.regular(), 16.0f, palette().dial, "Air", HUD_TEXT_LEFT);
    _mapAirValue = createHudText(_fonts.bold(), 18.0f, palette().dial, "--", HUD_TEXT_RIGHT_CENTER);
    _mapTimeLabel = createHudText(_fonts.regular(), 16.0f, palette().dial, "Time", HUD_TEXT_LEFT);
    _mapTimeValue = createHudText(_fonts.bold(), 18.0f, palette().dial, "--:--:--", HUD_TEXT_RIGHT_CENTER);
    for (osgText::Text* text : { _mapAirLabel.get(), _mapAirValue.get(),
                                _mapTimeLabel.get(), _mapTimeValue.get() })
        attachText(_minimap.textGeode.get(), text);
}

void FormulaHud::buildMinimapGeometry(const tTrack* track)
{
    // The geometry is rebuilt whenever the session track changes, so switching circuit in
    // the same process never keeps the previous layout.
    _minimapTrack = track;
    _minimapCenterline.clear();
    _minimapMin = osg::Vec2(0.0f, 0.0f);
    _minimapMax = osg::Vec2(0.0f, 0.0f);
    _minimapStart = osg::Vec2(0.0f, 0.0f);
    _minimapStartForward = osg::Vec2(0.0f, -1.0f);
    if (!track || !track->seg || track->nseg <= 0)
    {
        _minimapReady = false;
        GfLogInfo("Formula HUD minimap: no track geometry available yet\n");
        return;
    }
    _minimapReady = true;

    const tTrackSeg* segment = track->seg;
    const tTrackSeg* first = segment;
    int guard = 0;
    bool startFound = false;
    const int limit = track->nseg + 8;
    for (; segment && guard < limit; ++guard)
    {
        const osg::Vec2 startCenter(
            ((float)segment->vertex[TR_SL].x + (float)segment->vertex[TR_SR].x) * 0.5f,
            ((float)segment->vertex[TR_SL].y + (float)segment->vertex[TR_SR].y) * 0.5f);
        _minimapCenterline.push_back(startCenter);
        if (!startFound && (segment->raceInfo & TR_START) != 0)
        {
            const osg::Vec2 endCenter(
                ((float)segment->vertex[TR_EL].x + (float)segment->vertex[TR_ER].x) * 0.5f,
                ((float)segment->vertex[TR_EL].y + (float)segment->vertex[TR_ER].y) * 0.5f);
            _minimapStart = startCenter;
            osg::Vec2 forward(endCenter.x() - startCenter.x(), endCenter.y() - startCenter.y());
            const float length = std::sqrt(forward.x() * forward.x() + forward.y() * forward.y());
            if (length > 1.0e-3f)
                _minimapStartForward = osg::Vec2(forward.x() / length, forward.y() / length);
            startFound = true;
        }
        segment = segment->next;
        if (segment == first)
            break;
    }
    if (!startFound && _minimapCenterline.size() >= 2)
    {
        // Fall back to the first recorded centreline point: never invent a start line.
        _minimapStart = _minimapCenterline[0];
        osg::Vec2 forward(_minimapCenterline[1].x() - _minimapStart.x(),
                          _minimapCenterline[1].y() - _minimapStart.y());
        const float length = std::sqrt(forward.x() * forward.x() + forward.y() * forward.y());
        if (length > 1.0e-3f)
            _minimapStartForward = osg::Vec2(forward.x() / length, forward.y() / length);
    }
    // One point per track segment is far more than the minimap can show; the outline is
    // decimated to a bounded number of samples so the geometry stays cheap to rebuild.
    const size_t kMaxMinimapPoints = 600;
    if (_minimapCenterline.size() > kMaxMinimapPoints)
    {
        const size_t stride = _minimapCenterline.size() / kMaxMinimapPoints + 1;
        std::vector<osg::Vec2> decimated;
        decimated.reserve(_minimapCenterline.size() / stride + 1);
        for (size_t index = 0; index < _minimapCenterline.size(); index += stride)
            decimated.push_back(_minimapCenterline[index]);
        _minimapCenterline.swap(decimated);
    }

    if (!_minimapCenterline.empty())
    {
        _minimapMin = _minimapCenterline[0];
        _minimapMax = _minimapCenterline[0];
        for (size_t index = 1; index < _minimapCenterline.size(); ++index)
        {
            const osg::Vec2& point = _minimapCenterline[index];
            _minimapMin.x() = std::min(_minimapMin.x(), point.x());
            _minimapMin.y() = std::min(_minimapMin.y(), point.y());
            _minimapMax.x() = std::max(_minimapMax.x(), point.x());
            _minimapMax.y() = std::max(_minimapMax.y(), point.y());
        }
    }
    GfLogInfo("Formula HUD minimap: track=%s segments=%d start=(%.2f,%.2f) forward=(%.3f,%.3f) "
              "bounds=(%.1f,%.1f)-(%.1f,%.1f)\n",
              track->name ? track->name : "?", (int)_minimapCenterline.size(),
              _minimapStart.x(), _minimapStart.y(), _minimapStartForward.x(),
              _minimapStartForward.y(), _minimapMin.x(), _minimapMin.y(), _minimapMax.x(),
              _minimapMax.y());
}

void FormulaHud::updateMinimapPanel(const CarReadout& readout, const tCarElt* car, double timeOfDay)
{
    _minimap.shapes.begin();
    const Rect& rect = _layout.minimapPanel;
    const Palette& colors = palette();
    const float uiScale = _layout.uiScale;
    const Rect& background = _layout.minimapBackground;
    _minimap.shapes.addRect(background.x, background.y, background.width, background.height,
                            colors.background);
    const float left = background.x + 16.0f * uiScale;
    const float right = background.right() - 16.0f * uiScale;
    const float airY = background.y + 45.0f * uiScale;
    const float timeY = background.y + 20.0f * uiScale;
    _mapAirLabel->setCharacterSize(std::max(10.0f, 16.0f * uiScale));
    _mapTimeLabel->setCharacterSize(std::max(10.0f, 16.0f * uiScale));
    _mapAirValue->setCharacterSize(std::max(12.0f, 18.0f * uiScale));
    _mapTimeValue->setCharacterSize(std::max(12.0f, 18.0f * uiScale));
    char air[48] = "--";
    if (car && std::isfinite((double)car->_airtemp))
        std::snprintf(air, sizeof(air), "%.1f\u00b0C", (double)car->_airtemp - 273.15);
    char clock[32] = "--:--:--";
    if (std::isfinite(timeOfDay))
    {
        const int seconds = (int)(std::fmod(std::max(0.0, timeOfDay), 86400.0));
        std::snprintf(clock, sizeof(clock), "%02d:%02d:%02d", seconds / 3600,
                      (seconds / 60) % 60, seconds % 60);
    }
    updateHudText(_mapAirLabel.get(), "Air", colors.dial, left, airY);
    updateHudText(_mapAirValue.get(), air, colors.dial, right, airY);
    updateHudText(_mapTimeLabel.get(), "Time", colors.dial, left, timeY);
    updateHudText(_mapTimeValue.get(), clock, colors.dial, right, timeY);

    // The HUD is created before the track is known, and a session can load another track
    // in the same process: rebuild whenever the track model differs, including a return
    // to "no track" so a stale outline is never drawn.
    {
        const tTrack* track = getTrackModel();
        if (track != _minimapTrack || (!_minimapReady && track != nullptr))
            buildMinimapGeometry(track);
    }

    if (!_minimapCenterline.empty())
    {
        // Uniform aspect fit with a 10% inset, as in the original controller.
        const float inset = std::min(rect.width, rect.height) * 0.10f;
        const float availableWidth = std::max(1.0f, rect.width - inset * 2.0f);
        const float availableHeight = std::max(1.0f, rect.height - inset * 2.0f);
        const float sourceWidth = std::max(0.001f, _minimapMax.x() - _minimapMin.x());
        const float sourceHeight = std::max(0.001f, _minimapMax.y() - _minimapMin.y());
        const float mapScale = std::min(availableWidth / sourceWidth, availableHeight / sourceHeight);
        const float renderedWidth = sourceWidth * mapScale;
        const float renderedHeight = sourceHeight * mapScale;
        const float originX = rect.x + (rect.width - renderedWidth) * 0.5f;
        const float originY = rect.y + (rect.height - renderedHeight) * 0.5f;

        std::vector<osg::Vec2> screenPoints;
        screenPoints.reserve(_minimapCenterline.size() + 1);
        for (size_t index = 0; index < _minimapCenterline.size(); ++index)
        {
            const osg::Vec2& world = _minimapCenterline[index];
            screenPoints.push_back(osg::Vec2(originX + (world.x() - _minimapMin.x()) * mapScale,
                                             originY + (world.y() - _minimapMin.y()) * mapScale));
        }
        screenPoints.push_back(screenPoints.front());

        _minimap.shapes.addPolyline(screenPoints, std::max(2.0f, 4.0f * uiScale), colors.mapShadow,
                                    false);
        _minimap.shapes.addPolyline(screenPoints, std::max(1.0f, 1.8f * uiScale), colors.mapOutline,
                                    false);

        // Start/finish tick, perpendicular to the track direction at the start line.
        const osg::Vec2 start(originX + (_minimapStart.x() - _minimapMin.x()) * mapScale,
                              originY + (_minimapStart.y() - _minimapMin.y()) * mapScale);
        const osg::Vec2 heading = _minimapStartForward;
        const osg::Vec2 side(-heading.y(), heading.x());
        const float tickHalf = 6.0f * uiScale;
        std::vector<osg::Vec2> tick;
        tick.push_back(osg::Vec2(start.x() - side.x() * tickHalf, start.y() - side.y() * tickHalf));
        tick.push_back(osg::Vec2(start.x() + side.x() * tickHalf, start.y() + side.y() * tickHalf));
        _minimap.shapes.addPolyline(tick, 5.0f * uiScale, colors.mapShadow, false);
        _minimap.shapes.addPolyline(tick, 2.6f * uiScale, colors.mapStart, false);
        _minimap.shapes.addCircle(start.x(), start.y(), 3.7f * uiScale, colors.mapStart, 20);

        // Car marker: an arrow pointing along the published yaw.
        if (car && readout.connected)
        {
            const float worldX = (float)car->_pos_X;
            const float worldY = (float)car->_pos_Y;
            // The marker is clamped to the map rectangle: a car in the pit lane or off the
            // recorded centreline must never be drawn outside the minimap.
            const float markerX = std::min(rect.right(), std::max(rect.x,
                originX + (worldX - _minimapMin.x()) * mapScale));
            const float markerY = std::min(rect.top(), std::max(rect.y,
                originY + (worldY - _minimapMin.y()) * mapScale));
            const float yaw = (float)car->_yaw;
            const osg::Vec2 forward(std::cos(yaw), std::sin(yaw));
            const osg::Vec2 side(-forward.y(), forward.x());
            const float size = std::max(5.0f, 6.0f * uiScale);
            std::vector<osg::Vec2> marker;
            marker.push_back(osg::Vec2(markerX + forward.x() * size,
                                       markerY + forward.y() * size));
            marker.push_back(osg::Vec2(markerX - forward.x() * (size * 0.66f) + side.x() * size * 0.58f,
                                       markerY - forward.y() * (size * 0.66f) + side.y() * size * 0.58f));
            marker.push_back(osg::Vec2(markerX - forward.x() * (size * 0.33f),
                                       markerY - forward.y() * (size * 0.33f)));
            marker.push_back(osg::Vec2(markerX - forward.x() * (size * 0.66f) - side.x() * size * 0.58f,
                                       markerY - forward.y() * (size * 0.66f) - side.y() * size * 0.58f));
            _minimap.shapes.addPolygon(marker, colors.mapPlayer);
            std::vector<osg::Vec2> outline(marker);
            outline.push_back(marker.front());
            _minimap.shapes.addPolyline(outline, 1.8f * uiScale, colors.mapPlayerOutline, false);
        }
    }
    _minimap.shapes.end();
}

// ---------------------------------------------------------------------------
// Right block: tachometer, gear, speed, engine temperatures and fuel
// ---------------------------------------------------------------------------

namespace
{
    //! Godot `draw_arc` angles are measured clockwise from +x on a y-down canvas; the HUD
    //! draws with y up, so every angle is negated before building the band.
    void addBandGodot(ShapeMesh& mesh, const PanelXform& xform, float centerXDown,
                      float centerYDown, float innerRadius, float outerRadius,
                      float startDegrees, float endDegrees, const osg::Vec4& color, int segments)
    {
        const osg::Vec2 center = xform.point(centerXDown, centerYDown);
        mesh.addArcBand(center.x(), center.y(), innerRadius * xform.scale,
                        outerRadius * xform.scale, -startDegrees * kDegToRad,
                        -endDegrees * kDegToRad, color, segments);
    }

    osg::Vec2 godotPoint(const PanelXform& xform, float centerXDown, float centerYDown,
                         float radius, float degrees)
    {
        const float radians = degrees * kDegToRad;
        return xform.point(centerXDown + std::cos(radians) * radius,
                           centerYDown + std::sin(radians) * radius);
    }
}

void FormulaHud::createRightPanel()
{
    _right.shapeGeode = createShapeGeode("formulaRightShapes", _right.shapes);
    _right.textGeode = createTextGeode("formulaRightText");
    _root->addChild(_right.shapeGeode.get());
    _root->addChild(_right.textGeode.get());

    osgText::Font* bold = _fonts.bold();
    osgText::Font* regular = _fonts.regular();
    const Palette& colors = palette();

    _gearText = createHudText(bold, 400.0f, colors.digital, "N", HUD_TEXT_CENTER);
    _speedText = createHudText(bold, 160.0f, colors.digital, "0", HUD_TEXT_CENTER);
    _speedUnitText = createHudText(regular, 52.0f, colors.dial, "KM/H",
                                   HUD_TEXT_CENTER);
    _rpmText = createHudText(bold, 180.0f, colors.digital, "0", HUD_TEXT_RIGHT_CENTER);
    _rpmUnitText = createHudText(regular, 52.0f, colors.dial, "RPM",
                                 HUD_TEXT_RIGHT_CENTER);

    _oilLabel = createHudText(regular, 52.0f, colors.dial, "OIL TEMP",
                              HUD_TEXT_CENTER);
    _oilValue = createHudText(bold, 94.0f, colors.digital, "--", HUD_TEXT_CENTER);
    _oilUnit = createHudText(regular, 52.0f, colors.dial, "\u00b0C",
                             HUD_TEXT_CENTER);
    _waterLabel = createHudText(regular, 52.0f, colors.dial, "WATER TEMP",
                                HUD_TEXT_CENTER);
    _waterValue = createHudText(bold, 94.0f, colors.digital, "--", HUD_TEXT_CENTER);
    _waterUnit = createHudText(regular, 52.0f, colors.dial, "\u00b0C",
                               HUD_TEXT_CENTER);

    _fuelLabel = createHudText(regular, 52.0f, colors.dial, "FUEL", HUD_TEXT_CENTER);
    _fuelValue = createHudText(bold, 64.0f, colors.dial, "--", HUD_TEXT_CENTER);
    _fuelUnit = createHudText(regular, 52.0f, colors.dial, "KG", HUD_TEXT_CENTER);
    _fuelAverageLabel = createHudText(regular, 52.0f, colors.dial, "AVG",
                                      HUD_TEXT_CENTER);
    _fuelAverageValue = createHudText(bold, 64.0f, colors.inactive, "--.--",
                                      HUD_TEXT_CENTER);
    _fuelAverageUnit = createHudText(regular, 52.0f, colors.dial, "KG/LAP",
                                     HUD_TEXT_CENTER);
    _fuelDeltaLabel = createHudText(regular, 64.0f, colors.dial, "DELTA",
                                    HUD_TEXT_CENTER);
    _fuelDeltaValue = createHudText(bold, 108.0f, colors.inactive, "--.--",
                                    HUD_TEXT_CENTER);
    _fuelDeltaUnit = createHudText(regular, 52.0f, colors.dial, "LAPS",
                                   HUD_TEXT_CENTER);

    osgText::Text* all[] = {
        _gearText, _speedText, _speedUnitText, _rpmText, _rpmUnitText,
        _oilLabel, _oilValue, _oilUnit, _waterLabel, _waterValue, _waterUnit,
        _fuelLabel, _fuelValue, _fuelUnit, _fuelAverageLabel, _fuelAverageValue,
        _fuelAverageUnit, _fuelDeltaLabel, _fuelDeltaValue, _fuelDeltaUnit
    };
    for (size_t index = 0; index < sizeof(all) / sizeof(all[0]); ++index)
        attachText(_right.textGeode.get(), all[index]);
}

void FormulaHud::updateRightPanel(const CarReadout& readout)
{
    const PanelXform xform(_layout.rightPanel, kRightDesignWidth, kRightDesignHeight);
    const Palette& colors = palette();
    _right.shapes.begin();

    const float panelLeft = xform.point(0.0f, 0.0f).x();
    const float panelBottom = xform.point(0.0f, kRightDesignHeight).y();
    _right.shapes.addRect(panelLeft, panelBottom, kRightDesignWidth * xform.scale,
                          kRightDesignHeight * xform.scale, colors.background);
    const Rect& engine = _layout.enginePanel;

    // --- tachometer dial: 180 deg -> 328 deg (Godot angles), same geometry as the
    //     original, driven by the active car's own revolution limit. ---
    const float centerX = 650.0f;
    const float centerY = 760.0f;
    const float bandInner = 444.0f;
    const float bandOuter = 496.0f;
    const float startDegrees = 180.0f;
    const float endDegrees = 328.0f;
    const double rpmMaximum = readout.rpmValid && readout.rpmMax > 0.0 ? readout.rpmMax : 0.0;
    const double rpmValue = rpmMaximum > 0.0 ? std::min(readout.rpm, rpmMaximum) : 0.0;
    const double rpmFraction = rpmMaximum > 0.0 ? rpmValue / rpmMaximum : 0.0;
    // The original marks its last 6.3% as the redline band (17800 of 19000).
    const double redlineFraction = 0.937;
    const bool overRedline = rpmFraction >= redlineFraction;

    addBandGodot(_right.shapes, xform, centerX, centerY, bandInner, bandOuter, startDegrees,
                 endDegrees, colors.inactive, 64);
    if (rpmMaximum > 0.0 && rpmFraction > 0.0)
    {
        const float fillDegrees = startDegrees + (endDegrees - startDegrees) * (float)rpmFraction;
        addBandGodot(_right.shapes, xform, centerX, centerY, bandInner, bandOuter, startDegrees,
                     fillDegrees, overRedline ? colors.alert : colors.dial, 64);
    }
    // Redline band last, so it stays visible over the filled arc.
    const float redlineDegrees = startDegrees + (endDegrees - startDegrees) * (float)redlineFraction;
    addBandGodot(_right.shapes, xform, centerX, centerY, bandInner, bandOuter, redlineDegrees,
                 endDegrees, overRedline ? colors.redline : colors.redlineInactive, 24);

    // Shift lights (top-left of the design canvas).
    if (rpmMaximum > 0.0)
    {
        const double idealShift = rpmMaximum * 0.916;
        if (readout.rpm >= idealShift)
        {
            const osg::Vec2 center = xform.point(130.0f, 130.0f);
            _right.shapes.addCircle(center.x(), center.y(), 57.0f * xform.scale, colors.inactive, 40);
            _right.shapes.addCircle(center.x(), center.y(), 48.0f * xform.scale, colors.dial, 40);
        }
        if (readout.rpm >= rpmMaximum * redlineFraction)
        {
            const osg::Vec2 center = xform.point(285.0f, 130.0f);
            _right.shapes.addCircle(center.x(), center.y(), 57.0f * xform.scale, colors.inactive, 40);
            _right.shapes.addCircle(center.x(), center.y(), 48.0f * xform.scale, colors.alert, 40);
        }
    }

    // --- fuel: icon, twelve-segment level bar and remaining mass in kilograms ---
    {
        const osg::Vec2 iconCenter = xform.point(328.0f, 783.0f);
        const float iconScale = xform.scale;
        _right.shapes.addRoundedRect(iconCenter.x() - 20.0f * iconScale,
                                     iconCenter.y() - 30.0f * iconScale,
                                     34.0f * iconScale, 60.0f * iconScale, 5.0f * iconScale,
                                     colors.dial);
        _right.shapes.addRect(iconCenter.x() + 14.0f * iconScale,
                              iconCenter.y() - 16.0f * iconScale, 22.0f * iconScale,
                              8.0f * iconScale, colors.dial);
        _right.shapes.addRect(iconCenter.x() + 30.0f * iconScale,
                              iconCenter.y() - 18.0f * iconScale, 8.0f * iconScale,
                              14.0f * iconScale, colors.dial);
        _right.shapes.addRect(iconCenter.x() - 26.0f * iconScale,
                              iconCenter.y() - 39.0f * iconScale, 14.0f * iconScale,
                              9.0f * iconScale, colors.dial);

        const float trackLeft = 383.0f;
        const float trackTop = 751.0f;
        const float trackWidth = 690.0f;
        const float trackHeight = 66.0f;
        _right.shapes.addRect(xform.point(trackLeft, trackTop + trackHeight).x(),
                              xform.point(trackLeft, trackTop + trackHeight).y(),
                              trackWidth * xform.scale, trackHeight * xform.scale,
                              colors.fuelContainer);

        const int segmentCount = 12;
        const float segmentGap = 7.0f;
        const float segmentWidth = (trackWidth - 34.0f - segmentGap * (float)(segmentCount - 1))
            / (float)segmentCount;
        const float firstSegmentX = trackLeft + 17.0f;
        const float segmentHeight = trackHeight - 16.0f;
        const osg::Vec4 fuelColor = readout.fuelValid && readout.fuelFraction <= 0.05
            ? colors.alert : colors.fuelSegmentOn;
        for (int segment = 0; segment < segmentCount; ++segment)
        {
            const float segmentX = firstSegmentX + (segmentWidth + segmentGap) * (float)segment;
            const osg::Vec2 bottomLeft = xform.point(segmentX, trackTop + (trackHeight + segmentHeight) * 0.5f);
            _right.shapes.addRect(bottomLeft.x(), bottomLeft.y(), segmentWidth * xform.scale,
                                  segmentHeight * xform.scale, colors.fuelSegmentOff);
            if (!readout.fuelValid)
                continue;
            const double fill = std::min(1.0, std::max(0.0,
                readout.fuelFraction * (double)segmentCount - (double)segment));
            if (fill <= 0.0)
                continue;
            _right.shapes.addRect(bottomLeft.x(), bottomLeft.y(),
                                  (float)fill * segmentWidth * xform.scale,
                                  segmentHeight * xform.scale, fuelColor);
        }
    }

    // --- dividers between the fuel summary cells ---
    {
        const osg::Vec2 leftDivider = xform.point(509.0f, 879.0f);
        _right.shapes.addRect(leftDivider.x(), leftDivider.y(), 4.0f * xform.scale,
                              123.0f * xform.scale, colors.divider);
        const osg::Vec2 rightDivider = xform.point(943.0f, 879.0f);
        _right.shapes.addRect(rightDivider.x(), rightDivider.y(), 4.0f * xform.scale,
                              123.0f * xform.scale, colors.divider);
    }

    _right.shapes.end();

    // --- text ---
    static const float kScaleLabelRadius = 540.0f;
    const int segmentCount = rpmMaximum > 0.0
        ? (int)std::min(32.0, std::max(5.0, std::floor(rpmMaximum / 1000.0 + 0.5)))
        : 0;
    const float scaleLabelSize = std::max(_layout.labelFontPx, 88.0f * xform.scale);
    // One label per dial segment, created up front: the label index is then identical to
    // its segment, so hiding the odd thousands can never desynchronise the pool from the
    // indices used below.
    while (_rpmScaleLabels.size() < (size_t)segmentCount)
    {
        osgText::Text* label = createHudText(_fonts.bold(), scaleLabelSize, colors.dial,
                                             "0", HUD_TEXT_CENTER);
        attachText(_right.textGeode.get(), label);
        _rpmScaleLabels.push_back(label);
    }
    for (int segment = 0; segment < segmentCount; ++segment)
    {
        const float ratio = segmentCount > 1
            ? (float)segment / (float)(segmentCount - 1) : 0.0f;
        const float degrees = startDegrees + (endDegrees - startDegrees) * ratio;
        const double thousand = (double)segment * 1000.0;
        const int displayed = (int)std::floor(thousand / 1000.0 + 0.5);
        // Every second thousand carries a label, exactly like the original dial.
        const bool labelled = (displayed % 2 == 0);
        osgText::Text* label = _rpmScaleLabels[(size_t)segment];
        label->setNodeMask(labelled ? 0xFFFFFFFFu : 0u);
        if (!labelled)
            continue;
        if (label->getCharacterHeight() != scaleLabelSize)
            label->setCharacterSize(scaleLabelSize);
        const osg::Vec2 point = godotPoint(xform, centerX, centerY, kScaleLabelRadius, degrees);
        updateHudText(label, std::to_string(displayed), colors.dial, point.x(), point.y());
    }
    // Labels left over from a car with more dial segments stay hidden.
    for (size_t index = (size_t)segmentCount; index < _rpmScaleLabels.size(); ++index)
        _rpmScaleLabels[index]->setNodeMask(0);

    const osg::Vec4 engineRpmColor = overRedline ? colors.alert : colors.digital;
    {
        const std::string gear = readout.gearValid ? formatGear(readout.gear) : std::string("--");
        // Libron is wider than the condensed face of the original: the gear, the speed and
        // the revolution readout are measured and fitted into three separate columns of the
        // dial so a wide glyph ('R', 'N') can never collide with its neighbour.
        const float gearSize = fitFontSize(_fonts, true, gear, 300.0f * xform.scale,
                                           std::max(_layout.valueFontPx, 400.0f * xform.scale));
        if (_gearText->getCharacterHeight() != gearSize)
            _gearText->setCharacterSize(gearSize);
        const osg::Vec2 gearPoint = xform.point(560.0f, 540.0f);
        updateHudText(_gearText, gear, engineRpmColor, gearPoint.x(), gearPoint.y());
    }
    {
        const std::string speed = readout.speedValid ? formatSpeed(readout.speedKph) : "--";
        // Libron is wider than the condensed face of the original, so the speed and the
        // revolution readout are measured and fitted into their own design columns; they
        // must never overlap each other.
        const float speedDesired = std::max(_layout.valueFontPx, 160.0f * xform.scale);
        const float speedSize = fitFontSize(_fonts, true, speed, 300.0f * xform.scale, speedDesired);
        _measuredSpeedWidth = _fonts.measure(true, speed, speedDesired);
        _speedFontPx = speedSize;
        if (_speedText->getCharacterHeight() != speedSize)
            _speedText->setCharacterSize(speedSize);
        const osg::Vec2 speedPoint = xform.point(870.0f, 594.0f);
        updateHudText(_speedText, speed, colors.digital, speedPoint.x(), speedPoint.y());

        const float unitSize = std::max(_layout.labelFontPx, 52.0f * xform.scale);
        if (_speedUnitText->getCharacterHeight() != unitSize)
            _speedUnitText->setCharacterSize(unitSize);
        const osg::Vec2 unitPoint = xform.point(870.0f, 694.0f);
        updateHudText(_speedUnitText, "KM/H", colors.dial, unitPoint.x(), unitPoint.y());
    }
    {
        const std::string rpmText = readout.rpmValid ? formatRpm(readout.rpm) : "--";
        // Libron is wider than the condensed face of the original; the value is right
        // aligned to its design column and shrunk only as much as needed.
        const float rpmDesired = std::max(_layout.valueFontPx, 180.0f * xform.scale);
        const float rpmSize = fitFontSize(_fonts, true, rpmText, 355.0f * xform.scale, rpmDesired);
        _measuredRpmWidth = _fonts.measure(true, rpmText, rpmDesired);
        _rpmFontPx = rpmSize;
        if (_rpmText->getCharacterHeight() != rpmSize)
            _rpmText->setCharacterSize(rpmSize);
        const osg::Vec2 rpmPoint = xform.point(1440.0f, 575.0f);
        updateHudText(_rpmText, rpmText, engineRpmColor, rpmPoint.x(), rpmPoint.y());

        const float unitSize = std::max(_layout.labelFontPx, 52.0f * xform.scale);
        if (_rpmUnitText->getCharacterHeight() != unitSize)
            _rpmUnitText->setCharacterSize(unitSize);
        const osg::Vec2 unitPoint = xform.point(1440.0f, 700.0f);
        updateHudText(_rpmUnitText, "RPM", colors.dial, unitPoint.x(), unitPoint.y());
    }
    {
        const float labelSize = std::max(10.0f, 12.0f * _layout.uiScale);
        const float valueSize = std::max(13.0f, 20.0f * _layout.uiScale);
        const osg::Vec4 oilColor = readout.engineValid
            ? thermalColor(classifyEngine(readout.oilC, readout.oilOptimalMinC,
                                          readout.oilOptimalMaxC, readout.oilDeratingC,
                                          readout.oilCriticalC))
            : colors.pending;
        const osg::Vec4 waterColor = readout.engineValid
            ? thermalColor(classifyEngine(readout.waterC, readout.waterOptimalMinC,
                                          readout.waterOptimalMaxC, readout.waterDeratingC,
                                          readout.waterCriticalC))
            : colors.pending;

        if (_oilLabel->getCharacterHeight() != labelSize)
            _oilLabel->setCharacterSize(labelSize);
        if (_oilValue->getCharacterHeight() != valueSize)
            _oilValue->setCharacterSize(valueSize);
        if (_oilUnit->getCharacterHeight() != labelSize)
            _oilUnit->setCharacterSize(labelSize);
        if (_waterLabel->getCharacterHeight() != labelSize)
            _waterLabel->setCharacterSize(labelSize);
        if (_waterValue->getCharacterHeight() != valueSize)
            _waterValue->setCharacterSize(valueSize);
        if (_waterUnit->getCharacterHeight() != labelSize)
            _waterUnit->setCharacterSize(labelSize);

        const osg::Vec2 oilLabelPoint(engine.centerX(), engine.y + engine.height * 0.87f);
        const osg::Vec2 oilValuePoint(engine.x + engine.width * 0.40f, engine.y + engine.height * 0.65f);
        const osg::Vec2 oilUnitPoint(engine.x + engine.width * 0.77f, oilValuePoint.y());
        const osg::Vec2 waterLabelPoint(engine.centerX(), engine.y + engine.height * 0.38f);
        const osg::Vec2 waterValuePoint(engine.x + engine.width * 0.40f, engine.y + engine.height * 0.16f);
        const osg::Vec2 waterUnitPoint(engine.x + engine.width * 0.77f, waterValuePoint.y());
        updateHudText(_oilLabel, "OIL TEMP", colors.dial, oilLabelPoint.x(), oilLabelPoint.y());
        updateHudText(_oilValue, readout.engineValid ? formatTemperature(readout.oilC) : "--",
                      oilColor, oilValuePoint.x(), oilValuePoint.y());
        updateHudText(_oilUnit, "\u00b0C", colors.dial, oilUnitPoint.x(), oilUnitPoint.y());
        updateHudText(_waterLabel, "WATER TEMP", colors.dial, waterLabelPoint.x(),
                      waterLabelPoint.y());
        updateHudText(_waterValue, readout.engineValid ? formatTemperature(readout.waterC) : "--",
                      waterColor, waterValuePoint.x(), waterValuePoint.y());
        updateHudText(_waterUnit, "\u00b0C", colors.dial, waterUnitPoint.x(), waterUnitPoint.y());
    }
    {
        const float labelSize = std::max(_layout.labelFontPx, 52.0f * xform.scale);
        const float valueSize = std::max(_layout.valueFontPx, 64.0f * xform.scale);
        const float deltaLabelSize = std::max(_layout.labelFontPx, 64.0f * xform.scale);
        const float deltaValueSize = std::max(_layout.valueFontPx, 108.0f * xform.scale);
        osgText::Text* labels[] = { _fuelLabel, _fuelUnit, _fuelAverageLabel, _fuelAverageUnit,
                                    _fuelDeltaUnit };
        for (size_t index = 0; index < sizeof(labels) / sizeof(labels[0]); ++index)
        {
            if (labels[index]->getCharacterHeight() != labelSize)
                labels[index]->setCharacterSize(labelSize);
        }
        if (_fuelDeltaLabel->getCharacterHeight() != deltaLabelSize)
            _fuelDeltaLabel->setCharacterSize(deltaLabelSize);
        if (_fuelValue->getCharacterHeight() != valueSize)
            _fuelValue->setCharacterSize(valueSize);
        if (_fuelAverageValue->getCharacterHeight() != valueSize)
            _fuelAverageValue->setCharacterSize(valueSize);
        if (_fuelDeltaValue->getCharacterHeight() != deltaValueSize)
            _fuelDeltaValue->setCharacterSize(deltaValueSize);

        const osg::Vec4 fuelColor = readout.fuelValid
            ? (readout.fuelFraction <= 0.05 ? colors.alert : colors.dial)
            : colors.pending;
        const osg::Vec2 fuelLabelPoint = xform.point(290.0f, 887.0f);
        const osg::Vec2 fuelValuePoint = xform.point(292.0f, 960.0f);
        const osg::Vec2 fuelUnitPoint = xform.point(408.0f, 968.0f);
        const osg::Vec2 averageLabelPoint = xform.point(728.0f, 887.0f);
        const osg::Vec2 averageValuePoint = xform.point(725.0f, 960.0f);
        const osg::Vec2 averageUnitPoint = xform.point(866.0f, 968.0f);
        const osg::Vec2 deltaLabelPoint = xform.point(1154.0f, 887.0f);
        const osg::Vec2 deltaValuePoint = xform.point(1078.0f, 960.0f);
        const osg::Vec2 deltaUnitPoint = xform.point(1305.0f, 968.0f);
        updateHudText(_fuelLabel, "FUEL", colors.dial, fuelLabelPoint.x(), fuelLabelPoint.y());
        updateHudText(_fuelValue, readout.fuelValid ? formatFuelMass(readout.fuelKg) : "--.--",
                      fuelColor, fuelValuePoint.x(), fuelValuePoint.y());
        updateHudText(_fuelUnit, "KG", colors.dial, fuelUnitPoint.x(), fuelUnitPoint.y());
        updateHudText(_fuelAverageLabel, "AVG", colors.dial, averageLabelPoint.x(),
                      averageLabelPoint.y());
        // No per-lap consumption authority exists in the destination; the cell keeps the
        // original geometry and shows the explicit unknown marker instead of an estimate.
        updateHudText(_fuelAverageValue, "--.--", colors.inactive, averageValuePoint.x(),
                      averageValuePoint.y());
        updateHudText(_fuelAverageUnit, "KG/LAP", colors.dial, averageUnitPoint.x(),
                      averageUnitPoint.y());
        updateHudText(_fuelDeltaLabel, "DELTA", colors.dial, deltaLabelPoint.x(),
                      deltaLabelPoint.y());
        updateHudText(_fuelDeltaValue, "--.--", colors.inactive, deltaValuePoint.x(),
                      deltaValuePoint.y());
        updateHudText(_fuelDeltaUnit, "LAPS", colors.dial, deltaUnitPoint.x(),
                      deltaUnitPoint.y());
    }
}

// ---------------------------------------------------------------------------
// Four wheel thermal sets and brake discs
// ---------------------------------------------------------------------------

void FormulaHud::createTirePanel()
{
    _tires.shapeGeode = createShapeGeode("formulaTireShapes", _tires.shapes);
    _tires.textGeode = createTextGeode("formulaTireText");
    _root->addChild(_tires.shapeGeode.get());
    _root->addChild(_tires.textGeode.get());

    const Palette& colors = palette();
    _tireTitle = createHudText(_fonts.bold(), 18.0f, colors.dial, "TYRES",
                               HUD_TEXT_CENTER);
    attachText(_tires.textGeode.get(), _tireTitle);

    static const char* headers[WHEEL_COUNT] = { "FR", "FL", "RR", "RL" };
    for (int corner = 0; corner < WHEEL_COUNT; ++corner)
    {
        TireCellText& cell = _tireCells[corner];
        cell.header = createHudText(_fonts.bold(), 20.0f, colors.dial, headers[corner],
                                    HUD_TEXT_CENTER);
        cell.pressure = createHudText(_fonts.regular(), 15.0f, colors.dial, "-- kPa",
                                      HUD_TEXT_CENTER);
        cell.zones = createHudText(_fonts.bold(), 18.0f, colors.dial, "-- -- --\u00b0",
                                   HUD_TEXT_CENTER);
        cell.carcass = createHudText(_fonts.regular(), 13.0f, colors.dial, "CAR --\u00b0",
                                     HUD_TEXT_CENTER);
        cell.gas = createHudText(_fonts.regular(), 13.0f, colors.dial, "GAS --\u00b0",
                                 HUD_TEXT_CENTER);
        cell.rim = createHudText(_fonts.regular(), 13.0f, colors.dial, "RIM --\u00b0",
                                 HUD_TEXT_CENTER);
        cell.brake = createHudText(_fonts.regular(), 13.0f, colors.dial, "BRK --\u00b0",
                                   HUD_TEXT_CENTER);
        osgText::Text* all[] = { cell.header, cell.pressure, cell.zones, cell.carcass,
                                 cell.gas, cell.rim, cell.brake };
        for (size_t index = 0; index < sizeof(all) / sizeof(all[0]); ++index)
            attachText(_tires.textGeode.get(), all[index]);
    }
}

void FormulaHud::updateTirePanel(const CarReadout& readout)
{
    const PanelXform xform(_layout.tirePanel, kTireDesignWidth, kTireDesignHeight);
    const Palette& colors = palette();

    _tires.shapes.begin();
    const osg::Vec2 panelBottomLeft = xform.point(0.0f, kTireDesignHeight);
    _tires.shapes.addRect(panelBottomLeft.x(), panelBottomLeft.y(),
                          kTireDesignWidth * xform.scale, kTireDesignHeight * xform.scale,
                          colors.background);
    _tires.shapes.end();

    const float titleSize = std::max(_layout.valueFontPx, 18.0f * xform.scale);
    if (_tireTitle->getCharacterHeight() != titleSize)
        _tireTitle->setCharacterSize(titleSize);
    const osg::Vec2 titlePoint = xform.point(kTireDesignWidth * 0.5f, 20.0f);
    updateHudText(_tireTitle, "TYRES", colors.dial, titlePoint.x(), titlePoint.y());

    const float headerSize = std::max(_layout.valueFontPx, 20.0f * xform.scale);
    const float pressureSize = std::max(_layout.labelFontPx, 15.0f * xform.scale);
    const float zonesSize = std::max(_layout.valueFontPx, 18.0f * xform.scale);
    const float detailSize = std::max(_layout.labelFontPx, 13.0f * xform.scale);

    const float margin = 16.0f;
    const float gridGap = 10.0f;
    const float cellWidth = (kTireDesignWidth - margin * 2.0f - gridGap) * 0.5f;
    const float cellHeight = (kTireDesignHeight - 24.0f - 28.0f - 6.0f) * 0.5f;
    const float firstRowTop = 12.0f + 28.0f;

    for (int corner = 0; corner < WHEEL_COUNT; ++corner)
    {
        const WheelReadout& wheel = readout.wheel[corner];
        TireCellText& cell = _tireCells[corner];
        // The original grid reads FL, FR on the first row and RL, RR on the second; the
        // corner index keeps Speed Dreams' order (FR, FL, RR, RL), so each corner is
        // placed in the cell that matches its side of the car.
        const int column = (corner == WHEEL_FR || corner == WHEEL_RR) ? 1 : 0;
        const int row = (corner == WHEEL_RR || corner == WHEEL_RL) ? 1 : 0;
        const float cellLeft = margin + (cellWidth + gridGap) * (float)column;
        const float cellTop = firstRowTop + (cellHeight + 6.0f) * (float)row;
        const float centerX = cellLeft + cellWidth * 0.5f;

        if (cell.header->getCharacterHeight() != headerSize)
            cell.header->setCharacterSize(headerSize);
        if (cell.pressure->getCharacterHeight() != pressureSize)
            cell.pressure->setCharacterSize(pressureSize);
        if (cell.zones->getCharacterHeight() != zonesSize)
            cell.zones->setCharacterSize(zonesSize);
        osgText::Text* details[] = { cell.carcass, cell.gas, cell.rim, cell.brake };
        for (size_t index = 0; index < sizeof(details) / sizeof(details[0]); ++index)
        {
            if (details[index]->getCharacterHeight() != detailSize)
                details[index]->setCharacterSize(detailSize);
        }

        const osg::Vec2 headerPoint = xform.point(centerX, cellTop + 12.0f);
        updateHudText(cell.header, cell.header->getText().createUTF8EncodedString(), colors.dial,
                      headerPoint.x(), headerPoint.y());

        const std::string pressure = wheel.valid && wheel.pressureValid
            ? formatPressure(wheel.pressureKPaGauge) + " kPa" : std::string("-- kPa");
        const osg::Vec2 pressurePoint = xform.point(centerX, cellTop + 30.0f);
        updateHudText(cell.pressure, pressure, colors.dial, pressurePoint.x(), pressurePoint.y());

        std::string zones;
        if (wheel.valid)
        {
            zones = formatTemperature(wheel.treadInnerC) + " "
                + formatTemperature(wheel.treadCenterC) + " "
                + formatTemperature(wheel.treadOuterC) + "\u00b0";
        }
        else
        {
            zones = "-- -- --\u00b0";
        }
        const double hottest = hottestOfThree(wheel.treadInnerC, wheel.treadCenterC,
                                               wheel.treadOuterC);
        const osg::Vec4 zonesColor = wheel.valid ? thermalColor(classifyTread(wheel, hottest))
                                                 : colors.pending;
        const osg::Vec2 zonesPoint = xform.point(centerX, cellTop + 50.0f);
        updateHudText(cell.zones, zones, zonesColor, zonesPoint.x(), zonesPoint.y());

        // No profile publishes an optimal window for the carcass, the inflation gas or
        // the rim, so those three are shown with the neutral digital colour rather than
        // with an invented threshold.
        // Seven rows inside one cell; the offsets are the compact vertical rhythm of the
        // original grid, adapted so the last row never touches the next cell.
        const osg::Vec2 carcassPoint = xform.point(centerX, cellTop + 68.0f);
        const osg::Vec2 gasPoint = xform.point(centerX, cellTop + 82.0f);
        const osg::Vec2 rimPoint = xform.point(centerX, cellTop + 96.0f);
        const osg::Vec2 brakePoint = xform.point(centerX, cellTop + 110.0f);
        updateHudText(cell.carcass,
                      wheel.valid ? "CAR " + formatTemperatureWithUnit(wheel.carcassC)
                                  : std::string("CAR --"),
                      colors.dial, carcassPoint.x(), carcassPoint.y());
        updateHudText(cell.gas,
                      wheel.valid ? "GAS " + formatTemperatureWithUnit(wheel.gasC)
                                  : std::string("GAS --"),
                      colors.dial, gasPoint.x(), gasPoint.y());
        updateHudText(cell.rim,
                      wheel.valid ? "RIM " + formatTemperatureWithUnit(wheel.rimC)
                                  : std::string("RIM --"),
                      colors.dial, rimPoint.x(), rimPoint.y());
        const osg::Vec4 brakeColor = wheel.valid && wheel.brakeValid
            ? thermalColor(classifyBrake(readout, wheel.brakeDiscC)) : colors.pending;
        updateHudText(cell.brake,
                      wheel.valid ? "BRK " + formatTemperatureWithUnit(wheel.brakeDiscC)
                                  : std::string("BRK --"),
                      brakeColor, brakePoint.x(), brakePoint.y());
    }
}

// ---------------------------------------------------------------------------
// Bottom centre: steering wheel in degrees and pedal indicators
// ---------------------------------------------------------------------------

void FormulaHud::createControlsPanel()
{
    _controls.shapeGeode = createShapeGeode("formulaControlsShapes", _controls.shapes);
    _controls.textGeode = createTextGeode("formulaControlsText");
    _root->addChild(_controls.shapeGeode.get());
    _root->addChild(_controls.textGeode.get());

    const Palette& colors = palette();
    _steeringLabel = createHudText(_fonts.bold(), 13.0f, colors.label, "VOLANTE",
                                   HUD_TEXT_LEFT_CENTER);
    _steeringValue = createHudText(_fonts.bold(), 13.0f, colors.steeringColor, "0\u00b0",
                                   HUD_TEXT_CENTER);
    _throttleLabel = createHudText(_fonts.regular(), 14.0f, colors.pedalLabel, "ACELERADOR",
                                   HUD_TEXT_LEFT_CENTER);
    _throttleValue = createHudText(_fonts.bold(), 15.0f, colors.throttleColor, "0%",
                                   HUD_TEXT_RIGHT_CENTER);
    _brakeLabel = createHudText(_fonts.regular(), 14.0f, colors.pedalLabel, "FRENO",
                                HUD_TEXT_LEFT_CENTER);
    _brakeValue = createHudText(_fonts.bold(), 15.0f, colors.brakeColor, "0%",
                                HUD_TEXT_RIGHT_CENTER);

    osgText::Text* all[] = { _steeringLabel, _steeringValue, _throttleLabel, _throttleValue,
                             _brakeLabel, _brakeValue };
    for (size_t index = 0; index < sizeof(all) / sizeof(all[0]); ++index)
        attachText(_controls.textGeode.get(), all[index]);
}

void FormulaHud::updateControlsPanel(const CarReadout& readout)
{
    const PanelXform xform(_layout.controlsPanel, kControlsDesignWidth, kControlsDesignHeight);
    const Palette& colors = palette();
    _controls.shapes.begin();

    const osg::Vec2 bottomLeft = xform.point(0.0f, kControlsDesignHeight);
    _controls.shapes.addRoundedRect(bottomLeft.x(), bottomLeft.y(),
                                    kControlsDesignWidth * xform.scale,
                                    kControlsDesignHeight * xform.scale, 12.0f * xform.scale,
                                    colors.controlsBackground);
    _controls.shapes.addRoundedOutline(bottomLeft.x(), bottomLeft.y(),
                                       kControlsDesignWidth * xform.scale,
                                       kControlsDesignHeight * xform.scale, 12.0f * xform.scale,
                                       1.0f * xform.scale, colors.controlsBorder);

    // Steering wheel: the ring rotates with the effective steering fraction, exactly like
    // the original wheel graphic, while the readout states the physical wheel angle.
    const float steeringFraction = readout.steeringValid ? (float)readout.steerWheelFraction : 0.0f;
    const float visualRotation = -steeringFraction * kPi * 0.75f;
    const osg::Vec2 wheelCenter = xform.point(62.0f, 64.0f);
    // Ring drawn as an annulus so the panel background stays visible through the wheel.
    _controls.shapes.addArcBand(wheelCenter.x(), wheelCenter.y(), 24.0f * xform.scale,
                                27.0f * xform.scale, 0.0f, kPi * 2.0f, colors.wheelRing, 48);
    const float spokeAngles[3] = { -kPi * 0.5f, kPi / 6.0f, kPi * 5.0f / 6.0f };
    for (int spoke = 0; spoke < 3; ++spoke)
    {
        const float angle = spokeAngles[spoke] + visualRotation;
        // Design space uses a downwards y axis; the screen is y-up, so the vertical
        // component of every spoke direction is negated.
        const float directionX = std::cos(angle);
        const float directionY = -std::sin(angle);
        const osg::Vec3 from(wheelCenter.x() + directionX * 6.0f * xform.scale,
                             wheelCenter.y() + directionY * 6.0f * xform.scale, 0.0f);
        const osg::Vec3 to(wheelCenter.x() + directionX * 25.0f * xform.scale,
                           wheelCenter.y() + directionY * 25.0f * xform.scale, 0.0f);
        const osg::Vec3 perpendicular(-directionY * 1.5f * xform.scale,
                                      directionX * 1.5f * xform.scale, 0.0f);
        _controls.shapes.addQuad(from - perpendicular, to - perpendicular, to + perpendicular,
                                 from + perpendicular,
                                 spoke == 0 ? colors.steeringColor : colors.wheelSpoke);
    }
    _controls.shapes.addCircle(wheelCenter.x(), wheelCenter.y(), 6.0f * xform.scale,
                               colors.wheelHub, 24);

    // Pedal bars.
    const float barWidth = 250.0f;
    const float barHeight = 15.0f;
    for (int pedal = 0; pedal < 2; ++pedal)
    {
        const float baseline = pedal == 0 ? 30.0f : 77.0f;
        const float fraction = pedal == 0 ? (float)readout.throttle : (float)readout.brake;
        // Rectangles grow upwards in OSG; transform the design-space bottom edge.
        const osg::Vec2 barTopLeft = xform.point(126.0f, baseline + 9.0f + barHeight);
        _controls.shapes.addRect(barTopLeft.x(), barTopLeft.y(), barWidth * xform.scale,
                                 barHeight * xform.scale, colors.barBackground);
        if (readout.pedalsValid && fraction > 0.0f)
        {
            _controls.shapes.addRect(barTopLeft.x(), barTopLeft.y(),
                                     barWidth * xform.scale * std::min(1.0f, fraction),
                                     barHeight * xform.scale,
                                     pedal == 0 ? colors.throttleColor : colors.brakeColor);
        }
    }
    _controls.shapes.end();

    const float labelSize = std::max(_layout.labelFontPx, 13.0f * xform.scale);
    const float pedalLabelSize = std::max(_layout.labelFontPx, 14.0f * xform.scale);
    const float valueSize = std::max(_layout.valueFontPx, 15.0f * xform.scale);

    if (_steeringLabel->getCharacterHeight() != labelSize)
        _steeringLabel->setCharacterSize(labelSize);
    if (_steeringValue->getCharacterHeight() != labelSize)
        _steeringValue->setCharacterSize(labelSize);
    if (_throttleLabel->getCharacterHeight() != pedalLabelSize)
        _throttleLabel->setCharacterSize(pedalLabelSize);
    if (_brakeLabel->getCharacterHeight() != pedalLabelSize)
        _brakeLabel->setCharacterSize(pedalLabelSize);
    if (_throttleValue->getCharacterHeight() != valueSize)
        _throttleValue->setCharacterSize(valueSize);
    if (_brakeValue->getCharacterHeight() != valueSize)
        _brakeValue->setCharacterSize(valueSize);

    const osg::Vec2 steeringLabelPoint = xform.point(26.0f, 21.0f);
    updateHudText(_steeringLabel, "VOLANTE", colors.label, steeringLabelPoint.x(),
                  steeringLabelPoint.y());

    std::string steeringText = "--";
    if (readout.steeringValid)
    {
        const std::string direction = formatSteeringDirection(readout.steerWheelDeg);
        steeringText = direction.empty()
            ? formatSteeringDegrees(readout.steerWheelDeg)
            : direction + " " + formatSteeringDegrees(readout.steerWheelDeg);
    }
    const osg::Vec2 steeringValuePoint = xform.point(62.0f, 106.0f);
    updateHudText(_steeringValue, steeringText, colors.steeringColor, steeringValuePoint.x(),
                  steeringValuePoint.y());

    const osg::Vec2 throttleLabelPoint = xform.point(126.0f, 30.0f);
    const osg::Vec2 throttleValuePoint = xform.point(376.0f, 30.0f);
    const osg::Vec2 brakeLabelPoint = xform.point(126.0f, 77.0f);
    const osg::Vec2 brakeValuePoint = xform.point(376.0f, 77.0f);
    updateHudText(_throttleLabel, "ACELERADOR", colors.pedalLabel, throttleLabelPoint.x(),
                  throttleLabelPoint.y());
    updateHudText(_throttleValue,
                  readout.pedalsValid ? formatPercent(readout.throttle) : std::string("--"),
                  colors.throttleColor, throttleValuePoint.x(), throttleValuePoint.y());
    updateHudText(_brakeLabel, "FRENO", colors.pedalLabel, brakeLabelPoint.x(),
                  brakeLabelPoint.y());
    updateHudText(_brakeValue,
                  readout.pedalsValid ? formatPercent(readout.brake) : std::string("--"),
                  colors.brakeColor, brakeValuePoint.x(), brakeValuePoint.y());
}

// ---------------------------------------------------------------------------
// Frame update
// ---------------------------------------------------------------------------

void FormulaHud::update(const tCarElt* car, const tSituation* situation, double timeOfDay)
{
    if (!_root.valid())
        return;
    const CarReadout readout = formulahud::readCar(car, situation);
    // `OPEN_FORMULA_HUD_SKIP` disables named panels for diagnostics (comma separated:
    // timing, minimap, right, tires, controls), so a defect can be attributed to one
    // component without rebuilding the module.
    static const std::string skipped = []() {
        const char* configured = std::getenv("OPEN_FORMULA_HUD_SKIP");
        return std::string(configured ? configured : "");
    }();
    if (skipped.find("minimap") == std::string::npos)
        updateMinimapPanel(readout, car, timeOfDay);
    if (skipped.find("right") == std::string::npos)
        updateRightPanel(readout);
    if (skipped.find("tires") == std::string::npos)
        updateTirePanel(readout);
    if (skipped.find("controls") == std::string::npos)
        updateControlsPanel(readout);
    logDiagnostics(readout, car);
}

void FormulaHud::logDiagnostics(const CarReadout& readout, const tCarElt* car)
{
    if (!_diagnostics || !car)
        return;
    _diagnosticClock += 1.0;
    if (_diagnosticClock < 60.0)
        return;
    _diagnosticClock = 0.0;
    ++_diagnosticCount;
    const char* cornerNames[WHEEL_COUNT] = { "FR", "FL", "RR", "RL" };
    char wheels[512];
    wheels[0] = '\0';
    for (int corner = 0; corner < WHEEL_COUNT; ++corner)
    {
        const WheelReadout& wheel = readout.wheel[corner];
        char buffer[160];
        std::snprintf(buffer, sizeof(buffer), " %s=(in %.1f mid %.1f out %.1f car %.1f gas %.1f "
                      "rim %.1f brk %.1f)", cornerNames[corner], wheel.treadInnerC,
                      wheel.treadCenterC, wheel.treadOuterC, wheel.carcassC, wheel.gasC,
                      wheel.rimC, wheel.brakeDiscC);
        strncat(wheels, buffer, sizeof(wheels) - strlen(wheels) - 1);
    }
    GfLogInfo("FORMULAHUD n=%d t=%.3f speed=%.1f rpm=%.0f/%.0f gear=%d water=%.1f oil=%.1f "
              "fuel=%.1f/%.1f steer=%.1fdeg(%.3f) throttle=%.2f brake=%.2f lap=%d cur=%.3f "
              "last=%.3f best=%.3f pos=(%.2f,%.2f) yaw=%.4f valid=0x%x\n",
              _diagnosticCount, readout.sampleTime, readout.speedKph, readout.rpm,
              readout.rpmMax, readout.gear, readout.waterC, readout.oilC, readout.fuelKg,
              readout.fuelCapacityKg, readout.steerWheelDeg, readout.steerWheelFraction,
              readout.throttle, readout.brake, readout.lapNumber, readout.currentLapTime,
              readout.lastLapTime, readout.bestLapTime, car->_pos_X, car->_pos_Y, car->_yaw,
              car->formulaHud.validMask);
    GfLogInfo("FORMULAHUD wheels%s\n", wheels);
    GfLogInfo("FORMULAHUD text scale=%.4f speed='%s' measured=%.1f font=%.1f | rpm='%s' measured=%.1f font=%.1f\n",
              _layout.uiScale, formatSpeed(readout.speedKph).c_str(), _measuredSpeedWidth,
              _speedFontPx, formatRpm(readout.rpm).c_str(), _measuredRpmWidth, _rpmFontPx);
}

} // namespace formulahud
} // namespace osggraph
