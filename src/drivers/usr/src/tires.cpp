/***************************************************************************

    file                 : tires.cpp
    created              : 2011-03-21 07:39:00 UTC
    copyright            : (C) Daniel Schellhammer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "tires.h"

#include <algorithm>

Tires::Tires() :
    mCar(NULL),
    mAvgWearPerMeter(0.00001)
{
}

void Tires::init(const tCarElt* car)
{
    mCar = car;
    mDistWhenNew = 0;
    mWear = 0.0;
    mHotTemp = 273.15 + GfParmGetNum(mCar->_carHandle, "private", "hottemp", (char*)NULL, 120.0);
    mMuScale = GfParmGetNum(mCar->_carHandle, "private", "mu scale", (char*)NULL, 1.0);
}

void Tires::update()
{
    double minWear = 10.0;

    for (int i = 0; i < 4; i++)
    {
        if (mCar->priv.wheel[i].treadDepth < mWear)
        {
            mWear = mCar->priv.wheel[i].treadDepth;
        }

        if (mCar->priv.wheel[i].treadDepth < minWear)
            minWear = mCar->priv.wheel[i].treadDepth;
    }

    // Check if tires were changed
    if (mWear < 99.99)
    {
        mDistWhenNew = mCar->_distRaced;
    }

    // Calc dist to tire change
    double distused = mCar->_distRaced - mDistWhenNew;
    if (distused > 1000.0)
    {
        mAvgWearPerMeter = mWear / distused;
    }

    double wearuntilchange = 100.0 - mWear;
    mDistLeft = wearuntilchange / mAvgWearPerMeter;

    mGripFactor = updateGripFactor();
}

double Tires::updateGripFactor() const
{
    double gripFactor = 1.0 * mMuScale;

    if(HasTYC)
    {
        gripFactor *= TyreCondition();
    }

    return gripFactor;
}

double Tires::frictionBalanceLR() const
{
    double left = mCar->_wheelSeg(FRNT_LFT)->surface->kFriction + mCar->_wheelSeg(REAR_LFT)->surface->kFriction;
    double right = mCar->_wheelSeg(FRNT_RGT)->surface->kFriction + mCar->_wheelSeg(REAR_RGT)->surface->kFriction;

    return 1.0 - left / right;
}

double Tires::TyreConditionFront() const
{
    return MIN(mCar->_tyreCondition(0), mCar->_tyreCondition(1));
}

double Tires::TyreConditionRear() const
{
    return MIN(mCar->_tyreCondition(2), mCar->_tyreCondition(3));
}

double Tires::TyreCondition() const
{
    return MIN(TyreConditionFront(), TyreConditionRear());
}

double Tires::TyreTreadDepthFront() const
{
    double Right = mCar->_tyreTreadDepth(0);
    double Left = mCar->_tyreTreadDepth(1);

    return 100 * MIN(Right,Left);
}

double Tires::TyreTreadDepthRear() const
{
    double Right = mCar->_tyreTreadDepth(2);
    double Left = mCar->_tyreTreadDepth(3);

    return 100 * MIN(Right,Left);
}

double Tires::TyreTreadDepth() const
{
    return MIN(TyreTreadDepthFront(), TyreTreadDepthRear());
}
