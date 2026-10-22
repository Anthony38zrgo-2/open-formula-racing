/***************************************************************************

    file        : FormulaHud.h
    created     : 2026-10-22
    copyright   : (C) 2026 Open Formula

    Formula HUD migrated from Formula90s: tachometer and speedometer, fuel level and
    remaining mass, engine water/oil temperatures, the four wheel thermal sets, the four
    brake discs, the minimap, lap timing, the steering-wheel angle in degrees and the
    throttle/brake pedal indicators.

    The component owns its scene graph and layout; it reads telemetry and race timing
    from tCarElt/tSituation and never advances a simulation of its own.

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _FORMULA_HUD_H_
#define _FORMULA_HUD_H_

#include <memory>
#include <string>
#include <vector>

#include <osg/Group>
#include <osg/Vec2>
#include <osg/ref_ptr>

#include "FormulaHudData.h"
#include "FormulaHudDraw.h"
#include "FormulaHudFonts.h"
#include "FormulaHudLayout.h"

namespace osgText
{
    class Text;
}

namespace osggraph
{
namespace formulahud
{

//! One screen-space panel: a shape geometry, a shape geode and its text drawables.
struct HudPanel
{
    HudPanel();

    ShapeMesh shapes;
    osg::ref_ptr<osg::Geode> shapeGeode;
    osg::ref_ptr<osg::Geode> textGeode;
};

//! Coordinates of the Formula90s design space (y grows downwards) mapped to HUD pixels.
struct PanelXform
{
    PanelXform();
    PanelXform(const Rect& rect, float designWidth, float designHeight);

    osg::Vec2 point(float x, float yDown) const;
    float scale;        //!< Design pixels to HUD pixels.
    float left;
    float bottom;
    float designWidth;
    float designHeight;
};

class FormulaHud
{
public:
    FormulaHud();
    ~FormulaHud();

    //! Builds the scene graph. Returns false when the Libron faces are unavailable.
    bool create();
    //! Recomputes the layout for a new visible area (window resize or screen mode).
    void setViewport(int width, int height);
    //! Reads the current car and redraws every migrated component.
    void update(const tCarElt* car, const tSituation* situation, double timeOfDay);

    osg::Group* group() const { return _root.get(); }
    void setVisible(bool visible);
    bool isVisible() const { return _root.valid() && _root->getNodeMask() != 0; }

    const Layout& layout() const { return _layout; }
    const LayoutConfig& configuration() const { return _config; }

    //! Rectangles of the migrated groups, for coexistence checks against the legacy HUD.
    std::vector<std::pair<std::string, Rect> > groupRects() const;

    //! Enables the once-per-second diagnostic line (position on the minimap, timing and
    //! temperatures) used to validate the migration against the runtime log.
    void setDiagnosticsEnabled(bool enabled);

private:
    void createTimingPanel();
    void createMinimapPanel();
    void createRightPanel();
    void createTirePanel();
    void createControlsPanel();

    void updateTimingPanel(const CarReadout& readout);
    void updateMinimapPanel(const CarReadout& readout, const tCarElt* car, double timeOfDay);
    void updateRightPanel(const CarReadout& readout);
    void updateTirePanel(const CarReadout& readout);
    void updateControlsPanel(const CarReadout& readout);

    void buildMinimapGeometry(const tTrack* track);
    void logDiagnostics(const CarReadout& readout, const tCarElt* car);

    LayoutConfig _config;
    Layout _layout;
    Fonts _fonts;
    osg::ref_ptr<osg::Group> _root;

    HudPanel _timing;
    HudPanel _minimap;
    HudPanel _right;
    HudPanel _tires;
    HudPanel _controls;
    osg::ref_ptr<osgText::Text> _mapAirLabel;
    osg::ref_ptr<osgText::Text> _mapAirValue;
    osg::ref_ptr<osgText::Text> _mapTimeLabel;
    osg::ref_ptr<osgText::Text> _mapTimeValue;

    // Timing panel text.
    osgText::Text* _timingTitle;
    osgText::Text* _timingCaption[4];
    osgText::Text* _timingValue[4];

    // Right panel text.
    std::vector<osgText::Text*> _rpmScaleLabels;
    osgText::Text* _gearText;
    osgText::Text* _speedText;
    osgText::Text* _speedUnitText;
    osgText::Text* _rpmText;
    osgText::Text* _rpmUnitText;
    osgText::Text* _oilLabel;
    osgText::Text* _oilValue;
    osgText::Text* _oilUnit;
    osgText::Text* _waterLabel;
    osgText::Text* _waterValue;
    osgText::Text* _waterUnit;
    osgText::Text* _fuelLabel;
    osgText::Text* _fuelValue;
    osgText::Text* _fuelUnit;
    osgText::Text* _fuelAverageLabel;
    osgText::Text* _fuelAverageValue;
    osgText::Text* _fuelAverageUnit;
    osgText::Text* _fuelDeltaLabel;
    osgText::Text* _fuelDeltaValue;
    osgText::Text* _fuelDeltaUnit;

    // Tire panel text.
    osgText::Text* _tireTitle;
    struct TireCellText
    {
        osgText::Text* header;
        osgText::Text* pressure;
        osgText::Text* zones;
        osgText::Text* carcass;
        osgText::Text* gas;
        osgText::Text* rim;
        osgText::Text* brake;
    };
    TireCellText _tireCells[WHEEL_COUNT];

    // Controls panel text.
    osgText::Text* _steeringLabel;
    osgText::Text* _steeringValue;
    osgText::Text* _throttleLabel;
    osgText::Text* _throttleValue;
    osgText::Text* _brakeLabel;
    osgText::Text* _brakeValue;

    // Minimap track data, built once per track from the destination's own segments.
    bool _minimapReady;
    const tTrack* _minimapTrack;
    std::vector<osg::Vec2> _minimapCenterline;
    osg::Vec2 _minimapMin;
    osg::Vec2 _minimapMax;
    osg::Vec2 _minimapStart;
    osg::Vec2 _minimapStartForward;

    //! Last measured text metrics, logged by the diagnostics to prove the Libron fit.
    float _measuredSpeedWidth;
    float _measuredRpmWidth;
    float _speedFontPx;
    float _rpmFontPx;
    bool _diagnostics;
    double _diagnosticClock;
    int _diagnosticCount;
    bool _fontsReady;
};

} // namespace formulahud
} // namespace osggraph

#endif // _FORMULA_HUD_H_
