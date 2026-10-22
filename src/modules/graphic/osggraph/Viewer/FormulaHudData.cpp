/***************************************************************************

    file        : FormulaHudData.cpp
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

#include "FormulaHudData.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace osggraph
{
namespace formulahud
{

namespace
{
    //! Text used when a value was not published for this step.
    const char* const kUnknownNumber = "--";
    const char* const kUnknownLapTime = "--:--.---";

}

CarReadout::CarReadout()
    : connected(false)
    , sampleTime(0.0)
    , rpmValid(false)
    , rpm(0.0)
    , rpmMax(0.0)
    , rpmIdle(0.0)
    , gearValid(false)
    , gear(0)
    , speedValid(false)
    , speedKph(0.0)
    , engineValid(false)
    , waterC(0.0)
    , oilC(0.0)
    , waterOptimalMinC(0.0)
    , waterOptimalMaxC(0.0)
    , waterDeratingC(0.0)
    , waterCriticalC(0.0)
    , oilOptimalMinC(0.0)
    , oilOptimalMaxC(0.0)
    , oilDeratingC(0.0)
    , oilCriticalC(0.0)
    , brakeLimitsValid(false)
    , brakeOptimalMinC(0.0)
    , brakeOptimalMaxC(0.0)
    , brakeFadeStartC(0.0)
    , brakeCriticalC(0.0)
    , fuelValid(false)
    , fuelKg(0.0)
    , fuelCapacityKg(0.0)
    , fuelDensityKgPerL(0.0)
    , fuelFraction(0.0)
    , steeringValid(false)
    , steerWheelDeg(0.0)
    , steerWheelFraction(0.0)
    , pedalsValid(false)
    , throttle(0.0)
    , brake(0.0)
    , limitsValid(false)
    , lapValid(false)
    , lapNumber(0)
    , totalLaps(0)
    , currentLapValid(false)
    , currentLapTime(0.0)
    , lastLapValid(false)
    , lastLapTime(0.0)
    , bestLapValid(false)
    , bestLapTime(0.0)
{
    for (int corner = 0; corner < WHEEL_COUNT; ++corner)
        wheel[corner] = WheelReadout();
}

WheelReadout::WheelReadout()
    : valid(false)
    , brakeValid(false)
    , treadInnerC(0.0)
    , treadCenterC(0.0)
    , treadOuterC(0.0)
    , carcassC(0.0)
    , gasC(0.0)
    , rimC(0.0)
    , brakeDiscC(0.0)
    , pressureKPaGauge(0.0)
    , pressureValid(false)
    , brakeEfficiency(0.0)
    , treadOptimalC(0.0)
    , treadWindowC(0.0)
    , treadOverheatC(0.0)
    , carcassOptimalC(0.0)
{
}

CarReadout readCar(const tCarElt* car, const tSituation* situation)
{
    CarReadout readout;
    if (!car)
        return readout;

    const tFormulaHudTelemetry& published = car->formulaHud;
    readout.connected = published.validMask != 0u;
    // Published values are copied verbatim: a non-finite measurement must reach the
    // formatter as "unknown" instead of being turned into a plausible zero.
    readout.sampleTime = published.sampleTime;

    readout.limitsValid = (published.validMask & FHT_VALID_LIMITS) != 0u;

    readout.rpmValid = (published.validMask & FHT_VALID_RPM) != 0u
        && std::isfinite((double)published.engineRpm);
    readout.rpm = published.engineRpm;
    readout.rpmMax = published.engineRpmMax;
    readout.rpmIdle = published.engineRpmIdle;
    if (!std::isfinite((double)readout.rpmMax) || readout.rpmMax <= 0.0)
        readout.rpmValid = false;

    // Gear belongs to the physics publication as well: the simu module writes it into
    // the shared car state, the HUD only reads it.
    readout.gearValid = readout.connected;
    readout.gear = car->_gear;

    // Speed is the magnitude of the published world velocity: the physics owns it and
    // the HUD never integrates position to rebuild it.
    const double vx = (double)car->pub.DynGCg.vel.x;
    const double vy = (double)car->pub.DynGCg.vel.y;
    const double vz = (double)car->pub.DynGCg.vel.z;
    const double speed = std::sqrt(vx * vx + vy * vy + vz * vz);
    readout.speedValid = std::isfinite(speed);
    readout.speedKph = readout.speedValid ? speed * 3.6 : 0.0;

    readout.engineValid = (published.validMask & FHT_VALID_ENGINE_THERMAL) != 0u;
    readout.waterC = published.waterC;
    readout.oilC = published.oilC;
    readout.waterOptimalMinC = published.waterOptimalMinC;
    readout.waterOptimalMaxC = published.waterOptimalMaxC;
    readout.waterDeratingC = published.waterDeratingC;
    readout.waterCriticalC = published.waterCriticalC;
    readout.oilOptimalMinC = published.oilOptimalMinC;
    readout.oilOptimalMaxC = published.oilOptimalMaxC;
    readout.oilDeratingC = published.oilDeratingC;
    readout.oilCriticalC = published.oilCriticalC;
    if (!std::isfinite((double)readout.waterC) || !std::isfinite((double)readout.oilC)
        || !std::isfinite((double)readout.waterCriticalC)
        || !std::isfinite((double)readout.oilCriticalC)
        || readout.waterCriticalC <= readout.waterOptimalMaxC
        || readout.waterOptimalMaxC <= readout.waterOptimalMinC
        || readout.oilCriticalC <= readout.oilOptimalMaxC
        || readout.oilOptimalMaxC <= readout.oilOptimalMinC)
        readout.engineValid = false;

    readout.brakeLimitsValid = readout.limitsValid
        && std::isfinite((double)published.brakeCriticalC)
        && std::isfinite((double)published.brakeOptimalMinC)
        && published.brakeCriticalC > published.brakeOptimalMaxC
        && published.brakeOptimalMaxC > published.brakeOptimalMinC;
    readout.brakeOptimalMinC = published.brakeOptimalMinC;
    readout.brakeOptimalMaxC = published.brakeOptimalMaxC;
    readout.brakeFadeStartC = published.brakeFadeStartC;
    readout.brakeCriticalC = published.brakeCriticalC;

    readout.fuelValid = (published.validMask & FHT_VALID_FUEL) != 0u
        && std::isfinite((double)published.fuelKg)
        && std::isfinite((double)published.fuelCapacityKg)
        && published.fuelCapacityKg > 0.0;
    readout.fuelKg = published.fuelKg;
    readout.fuelCapacityKg = published.fuelCapacityKg;
    readout.fuelDensityKgPerL = published.fuelDensityKgPerL;
    readout.fuelFraction = readout.fuelValid
        ? std::min(1.0, std::max(0.0, readout.fuelKg / readout.fuelCapacityKg))
        : 0.0;

    readout.steeringValid = (published.validMask & FHT_VALID_STEERING) != 0u
        && std::isfinite((double)published.steerWheelDeg)
        && std::isfinite((double)published.steerWheelFraction);
    readout.steerWheelDeg = published.steerWheelDeg;
    readout.steerWheelFraction = readout.steeringValid
        ? std::min(1.0, std::max(-1.0, (double)published.steerWheelFraction))
        : 0.0;

    readout.pedalsValid = (published.validMask & FHT_VALID_PEDALS) != 0u
        && std::isfinite((double)published.throttleApplied)
        && std::isfinite((double)published.brakeApplied);
    readout.throttle = readout.pedalsValid
        ? std::min(1.0, std::max(0.0, (double)published.throttleApplied)) : 0.0;
    readout.brake = readout.pedalsValid
        ? std::min(1.0, std::max(0.0, (double)published.brakeApplied)) : 0.0;

    const bool wheelThermal = (published.validMask & FHT_VALID_WHEEL_THERMAL) != 0u;
    for (int corner = 0; corner < WHEEL_COUNT; ++corner)
    {
        const tFormulaHudWheel& source = published.wheel[corner];
        WheelReadout& target = readout.wheel[corner];
        target.valid = wheelThermal;
        target.brakeValid = wheelThermal && readout.brakeLimitsValid
            && std::isfinite((double)source.brakeDiscC);
        target.treadInnerC = source.treadInnerC;
        target.treadCenterC = source.treadCenterC;
        target.treadOuterC = source.treadOuterC;
        target.carcassC = source.carcassC;
        target.gasC = source.gasC;
        target.rimC = source.rimC;
        target.brakeDiscC = source.brakeDiscC;
        target.pressureKPaGauge = source.pressureKPaGauge;
        target.pressureValid = std::isfinite((double)source.pressureKPaGauge);
        target.brakeEfficiency = source.brakeEfficiency;
        target.treadOptimalC = published.treadOptimalC[corner];
        target.treadWindowC = published.treadWindowC[corner];
        target.treadOverheatC = published.treadOverheatC[corner];
        target.carcassOptimalC = published.carcassOptimalC[corner];
        if (!std::isfinite((double)target.treadOptimalC)
            || !std::isfinite((double)target.treadOverheatC)
            || target.treadOptimalC <= 0.0
            || target.treadOverheatC <= target.treadOptimalC)
            target.valid = false;
    }

    // Race timing authority: the race engine writes these fields, never the HUD.
    // Speed Dreams counts start-line crossings exactly like the original controller:
    // 0 before the first crossing, 1 when the first lap is completed.
    readout.lapValid = car->_laps >= 0;
    readout.lapNumber = car->_laps;
    readout.totalLaps = situation ? situation->_totLaps : 0;
    readout.currentLapValid = std::isfinite((double)car->_curLapTime) && car->_curLapTime > 0.0;
    readout.currentLapTime = readout.currentLapValid ? (double)car->_curLapTime : 0.0;
    readout.lastLapValid = std::isfinite((double)car->_lastLapTime) && car->_lastLapTime > 0.0;
    readout.lastLapTime = readout.lastLapValid ? (double)car->_lastLapTime : 0.0;
    readout.bestLapValid = std::isfinite((double)car->_bestLapTime) && car->_bestLapTime > 0.0;
    readout.bestLapTime = readout.bestLapValid ? (double)car->_bestLapTime : 0.0;

    return readout;
}

bool isUsable(double value)
{
    return std::isfinite(value);
}

ThermalState classifyTread(const WheelReadout& wheel, double temperatureC)
{
    if (!wheel.valid || !isUsable(temperatureC))
        return THERMAL_UNKNOWN;
    const double coldBelow = wheel.treadOptimalC - wheel.treadWindowC;
    const double warmAbove = wheel.treadOptimalC + wheel.treadWindowC;
    if (temperatureC < coldBelow)
        return THERMAL_COLD;
    if (temperatureC <= warmAbove)
        return THERMAL_OPTIMAL;
    if (temperatureC <= wheel.treadOverheatC)
        return THERMAL_WARM;
    return THERMAL_HOT;
}

ThermalState classifyBrake(const CarReadout& readout, double temperatureC)
{
    if (!readout.brakeLimitsValid || !isUsable(temperatureC))
        return THERMAL_UNKNOWN;
    if (temperatureC < readout.brakeOptimalMinC)
        return THERMAL_COLD;
    if (temperatureC <= readout.brakeOptimalMaxC)
        return THERMAL_OPTIMAL;
    if (temperatureC < readout.brakeFadeStartC)
        return THERMAL_WARM;
    if (temperatureC < readout.brakeCriticalC)
        return THERMAL_HOT;
    return THERMAL_CRITICAL;
}

ThermalState classifyEngine(double temperatureC,
                            double optimalMinimumC,
                            double optimalMaximumC,
                            double deratingC,
                            double criticalC)
{
    if (!isUsable(temperatureC) || criticalC <= optimalMaximumC
        || optimalMaximumC <= optimalMinimumC)
        return THERMAL_UNKNOWN;
    if (temperatureC < optimalMinimumC)
        return THERMAL_COLD;
    if (temperatureC <= optimalMaximumC)
        return THERMAL_OPTIMAL;
    if (temperatureC < deratingC)
        return THERMAL_WARM;
    if (temperatureC < criticalC)
        return THERMAL_HOT;
    return THERMAL_CRITICAL;
}

namespace
{
    std::string formatFinite(double value, int decimals)
    {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
        return std::string(buffer);
    }
}

std::string formatTemperature(double temperatureC)
{
    if (!isUsable(temperatureC) || temperatureC < -40.0)
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f", temperatureC);
    return std::string(buffer);
}

std::string formatTemperatureWithUnit(double temperatureC)
{
    if (!isUsable(temperatureC) || temperatureC < -40.0)
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f\u00b0", temperatureC);
    return std::string(buffer);
}

std::string formatPressure(double kilopascalsGauge)
{
    if (!isUsable(kilopascalsGauge))
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f", kilopascalsGauge);
    return std::string(buffer);
}

std::string formatSpeed(double kilometersPerHour)
{
    if (!isUsable(kilometersPerHour))
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f", std::max(0.0, kilometersPerHour));
    return std::string(buffer);
}

std::string formatRpm(double revolutionsPerMinute)
{
    if (!isUsable(revolutionsPerMinute) || revolutionsPerMinute < 0.0)
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f", revolutionsPerMinute);
    return std::string(buffer);
}

std::string formatFuelMass(double kilograms)
{
    if (!isUsable(kilograms) || kilograms < 0.0)
        return std::string(kUnknownNumber);
    return formatFinite(kilograms, 1);
}

std::string formatLapNumber(int lap)
{
    if (lap <= 0)
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d", lap);
    return std::string(buffer);
}

std::string formatLapTime(double seconds)
{
    if (!isUsable(seconds) || seconds <= 0.0)
        return std::string(kUnknownLapTime);
    const long long totalMilliseconds = (long long)std::llround(seconds * 1000.0);
    const long long minutes = totalMilliseconds / 60000;
    const long long remaining = totalMilliseconds % 60000;
    const long long wholeSeconds = remaining / 1000;
    const long long milliseconds = remaining % 1000;
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%lld:%02lld.%03lld", minutes, wholeSeconds,
                  milliseconds);
    return std::string(buffer);
}

std::string formatSteeringDegrees(double degrees)
{
    if (!isUsable(degrees))
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f\u00b0", std::fabs(degrees));
    return std::string(buffer);
}

std::string formatSteeringDirection(double degrees)
{
    if (!isUsable(degrees) || std::fabs(degrees) < 1.0)
        return std::string();
    // The physics convention is positive = left (see the steering sign contract).
    return degrees > 0.0 ? std::string("IZQ") : std::string("DER");
}

std::string formatPercent(double fraction)
{
    if (!isUsable(fraction))
        return std::string(kUnknownNumber);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f%%",
                  std::min(100.0, std::max(0.0, fraction * 100.0)));
    return std::string(buffer);
}

std::string formatGear(int gear)
{
    if (gear < 0)
        return std::string("R");
    if (gear == 0)
        return std::string("N");
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%d", gear);
    return std::string(buffer);
}

} // namespace formulahud
} // namespace osggraph
