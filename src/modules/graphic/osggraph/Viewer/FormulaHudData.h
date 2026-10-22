/***************************************************************************

    file        : FormulaHudData.h
    created     : 2026-10-22
    copyright   : (C) 2026 Open Formula

    Presentation model of the Formula HUD migrated from Formula90s.

    Every value here is either read from the physics publication of the accepted step
    (`tCarElt::formulaHud`, filled by the active simu module) or from the race engine's
    timing authority (`tCarRaceInfo`). Nothing is estimated, extrapolated or defaulted:
    a value the model does not publish stays invalid and is rendered as "--".

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _FORMULA_HUD_DATA_H_
#define _FORMULA_HUD_DATA_H_

#include <string>

#include <car.h>
#include <raceman.h>

namespace osggraph
{
namespace formulahud
{

//! Speed Dreams wheel order used by every readout and widget of this HUD.
enum WheelCorner
{
    WHEEL_FR = 0,
    WHEEL_FL = 1,
    WHEEL_RR = 2,
    WHEEL_RL = 3,
    WHEEL_COUNT = 4
};

//! Thermal state of a measured node, classified with active-profile thresholds.
enum ThermalState
{
    THERMAL_UNKNOWN = 0,
    THERMAL_COLD,
    THERMAL_OPTIMAL,
    THERMAL_WARM,
    THERMAL_HOT,
    THERMAL_CRITICAL
};

struct WheelReadout
{
    WheelReadout();

    bool valid;              //!< Per-wheel thermal nodes were published.
    bool brakeValid;         //!< The wheel brake disc can be classified.
    double treadInnerC;
    double treadCenterC;
    double treadOuterC;
    double carcassC;
    double gasC;
    double rimC;
    double brakeDiscC;
    double pressureKPaGauge;
    bool pressureValid;      //!< Gauge pressure was measured, not defaulted.
    bool wearValid;
    double wearRemaining;    //!< Remaining tread fraction from the physics wheel view.
    double brakeEfficiency;
    double treadOptimalC;
    double treadWindowC;
    double treadOverheatC;
    double carcassOptimalC;
};

//! One coherent presentation snapshot: car telemetry plus race timing of the same frame.
struct CarReadout
{
    CarReadout();

    bool connected;             //!< The physics published at least one accepted step.
    double sampleTime;          //!< Simulation time of the published step.

    bool rpmValid;
    double rpm;
    double rpmMax;
    double rpmIdle;

    bool gearValid;
    int gear;                   //!< -1 reverse, 0 neutral, >0 forward gear.

    bool speedValid;
    double speedKph;

    bool engineValid;
    double waterC;
    double oilC;
    double waterOptimalMinC;
    double waterOptimalMaxC;
    double waterDeratingC;
    double waterCriticalC;
    double oilOptimalMinC;
    double oilOptimalMaxC;
    double oilDeratingC;
    double oilCriticalC;

    bool brakeLimitsValid;
    double brakeOptimalMinC;
    double brakeOptimalMaxC;
    double brakeFadeStartC;
    double brakeCriticalC;

    bool fuelValid;
    double fuelKg;
    double fuelCapacityKg;
    double fuelDensityKgPerL;
    double fuelFraction;

    bool steeringValid;
    double steerWheelDeg;
    double steerWheelFraction;  //!< Effective applied steering, -1..1, positive = left.

    bool pedalsValid;
    double throttle;
    double brake;

    bool limitsValid;
    WheelReadout wheel[WHEEL_COUNT];

    bool lapValid;
    int lapNumber;              //!< 1-based current lap, from the race engine.
    int totalLaps;
    bool currentLapValid;
    double currentLapTime;
    bool lastLapValid;
    double lastLapTime;
    bool bestLapValid;
    double bestLapTime;
};

//! Builds the readout of the active car. Invalid data stays explicitly invalid.
CarReadout readCar(const tCarElt* car, const tSituation* situation);

//! Thermal classification using the active profile thresholds only.
ThermalState classifyTread(const WheelReadout& wheel, double temperatureC);
ThermalState classifyBrake(const CarReadout& readout, double temperatureC);
ThermalState classifyEngine(double temperatureC,
                            double optimalMinimumC,
                            double optimalMaximumC,
                            double deratingC,
                            double criticalC);

//! Text formatting. An unavailable value becomes "--" or "--:--.---".
std::string formatTemperature(double temperatureC);          //!< "92"
std::string formatTemperatureWithUnit(double temperatureC);  //!< "92°"
std::string formatPressure(double kilopascalsGauge);         //!< "148"
std::string formatSpeed(double kilometersPerHour);           //!< "287"
std::string formatRpm(double revolutionsPerMinute);          //!< "11250"
std::string formatFuelMass(double kilograms);                //!< "7.6"
std::string formatLapNumber(int lap);
std::string formatLapTime(double seconds);                   //!< "1:07.482"
std::string formatSteeringDegrees(double degrees);           //!< "24°"
std::string formatSteeringDirection(double degrees);         //!< "IZQ" / "DER" / ""
std::string formatPercent(double fraction);                  //!< "47%"
std::string formatGear(int gear);

//! True when the value can be shown (finite).
bool isUsable(double value);

} // namespace formulahud
} // namespace osggraph

#endif // _FORMULA_HUD_DATA_H_
