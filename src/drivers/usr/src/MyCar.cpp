/***************************************************************************

    file        : MyCar.cpp
    created     : 8 Jun 2017
    copyright   : (C) 2017 D.Schellhammer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MyCar.h"
#include "Utils.h"

#include <iostream>

// The "AXIOM" logger instance.
extern GfLogger* PLogUSR;
#define LogUSR (*PLogUSR)

MyCar::MyCar() :
    HASTYC(false),
    HASCPD(false),
    HASABS(false),
    HASESP(false),
    HASTCL(false),
    mCar(NULL)
{
}

void MyCar::init(tCarElt* car, MyTrack* track)
{
    mCar = car;
    mTrack = track;
    mYaw = 0.0;
    mLastDamage = 0;
    mPrevGear = mCar->_gear;
    mGlobalPos = Vec3d(mCar->_pos_X, mCar->_pos_Y, mCar->_pos_Z);
    mFrontAxleOffset = mCar->priv.wheel[0].relPos.x;
    mTires.init(mCar);
    initVars();
    initCa();
    initCw();
    initBrakes();
}

void MyCar::setDefaults()
{
    mAbsSlip = 14.0;
    mSideSlipTCL = 25.0;
    mSideSlipTCLQualy = 40.0;
    mSideSlipTCLFactor = 0.2;
    mBrakeMuFactor = 0.88;
    mMuScaleLR = 0.86;
    mBumpSpeedFactor = 1000.0;
    mFuelPerMeter = 0.001;
    mFuelWeightFactor = 1.0;
    mTireWearPerMeter = 1.0;
    mRearWingAngle = 17.0 * M_PI / 180.0; /* rad */
    mShiftUpPoint = 0.98;
}

void MyCar::readPrivateSection(void *handle)
{
    LogUSR.info("Read private section ! \n");
    mAbsSlip = GfParmGetNum(handle, "private", "ABS slip", nullptr, mAbsSlip);
    mBrakeMuFactor = GfParmGetNum(handle, "private", "brake mu factor", nullptr, mBrakeMuFactor);
    mMuScaleLR = GfParmGetNum(handle, "private", "LR mu scale", nullptr, mMuScaleLR);
    mBumpSpeedFactor = GfParmGetNum(handle, "private", "bump speed factor", nullptr, mBumpSpeedFactor);
    mFuelPerMeter = GfParmGetNum(handle, "private", "fuel per meter", nullptr, mFuelPerMeter);
    mFuelWeightFactor = GfParmGetNum(handle, "private", "fuel weight factor", nullptr, mFuelWeightFactor);
    mTireWearPerMeter = GfParmGetNum(handle, "private", "tire wear per meter", nullptr, mTireWearPerMeter);
    mSideSlipTCL = GfParmGetNum(handle, "private", "TCL side slip", nullptr, mSideSlipTCL);
    mSideSlipTCLQualy = GfParmGetNum(handle, "private", "TCL side slip qualy", nullptr, mSideSlipTCLQualy);
    mSideSlipTCLFactor = GfParmGetNum(handle, "private", "TCL side slip factor", nullptr, mSideSlipTCLFactor);
    mShiftUpPoint = GfParmGetNum(handle, "private", "shift up point", nullptr, mShiftUpPoint);
}

void MyCar::readVarSpecs(void *handle)
{
    mRearWingAngle = GfParmGetNum(handle, SECT_REARWING, PRM_WINGANGLE, "deg", mRearWingAngle);
}

void MyCar::readConstSpecs(void* CarHandle)
{
    const char *enabling;
    enabling = GfParmGetStr(CarHandle, SECT_FEATURES, PRM_TIRETEMPDEG, VAL_NO);

    if (strcmp(enabling, VAL_YES) == 0)
    {
      HASTYC = true;
      LogUSR.info("#Car has TYC yes\n");
    }
    else
      LogUSR.info("#Car has TYC no\n");

    enabling = GfParmGetStr(CarHandle, SECT_FEATURES, PRM_TIRECOMPOUNDS, VAL_NO);

    if (strcmp(enabling, VAL_YES) == 0)
    {
      HASCPD = true;
      LogUSR.info("#Car has Compounds yes\n");
    }
    else
      LogUSR.info("#Car has Compounds no\n");

    enabling = GfParmGetStr(CarHandle, SECT_FEATURES, PRM_ABSINSIMU, VAL_NO);

    if (strcmp(enabling, VAL_YES) == 0)
    {
      HASABS = true;
      LogUSR.info("#Car has ABS yes\n");
    }
    else
      LogUSR.info("#Car has ABS no\n");

    enabling = GfParmGetStr(CarHandle, SECT_FEATURES, PRM_ESPINSIMU, VAL_NO);

    if (strcmp(enabling, VAL_YES) == 0)
    {
      HASESP = true;
      LogUSR.info("#Car has ESP yes\n");
    }
    else
      LogUSR.info("#Car has ESP no\n");

    enabling = GfParmGetStr(CarHandle, SECT_FEATURES, PRM_TCLINSIMU, VAL_NO);

    if (strcmp(enabling, VAL_YES) == 0)
    {
      HASTCL = true;
      LogUSR.info("#Car has TCL yes\n");
    }
    else
      LogUSR.info("#Car has TCL no\n");

    mCarMass = GfParmGetNum(CarHandle, SECT_CAR, PRM_MASS, NULL, 0);
    mTankVol = GfParmGetNum(CarHandle, SECT_CAR, PRM_TANK, (char*)NULL, 0);
    double tiremuFL = GfParmGetNum(CarHandle, SECT_FRNTLFTWHEEL, PRM_MU, NULL, 0);
    double tiremuFR = GfParmGetNum(CarHandle, SECT_FRNTRGTWHEEL, PRM_MU, NULL, 0);
    double tiremuRL = GfParmGetNum(CarHandle, SECT_REARLFTWHEEL, PRM_MU, NULL, 0);
    double tiremuRR = GfParmGetNum(CarHandle, SECT_REARRGTWHEEL, PRM_MU, NULL, 0);
    double tiremufront = MIN(tiremuFL, tiremuFR);
    double tiremurear = MIN(tiremuRL, tiremuRR);
    mTireMu = MIN(tiremufront, tiremurear);

    if (HASCPD)
    {
        char path[256];
        sprintf(path, "%s/%s/%s", SECT_FRNTRGTWHEEL, SECT_COMPOUNDS, SECT_SOFT);
        tiremuFR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_FRNTLFTWHEEL, SECT_COMPOUNDS, SECT_SOFT);
        tiremuFL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARRGTWHEEL, SECT_COMPOUNDS, SECT_SOFT);
        tiremuRR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARLFTWHEEL, SECT_COMPOUNDS, SECT_SOFT);
        tiremuRL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        tiremufront = MIN(tiremuFL, tiremuFR);
        tiremurear = MIN(tiremuRL, tiremuRR);
        mTireMuC[1] = MIN(tiremufront, tiremurear);

        sprintf(path, "%s/%s/%s", SECT_FRNTRGTWHEEL, SECT_COMPOUNDS, SECT_MEDIUM);
        tiremuFR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_FRNTLFTWHEEL, SECT_COMPOUNDS, SECT_MEDIUM);
        tiremuFL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARRGTWHEEL, SECT_COMPOUNDS, SECT_MEDIUM);
        tiremuRR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARLFTWHEEL, SECT_COMPOUNDS, SECT_MEDIUM);
        tiremuRL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        tiremufront = MIN(tiremuFL, tiremuFR);
        tiremurear = MIN(tiremuRL, tiremuRR);
        mTireMuC[2] = MIN(tiremufront, tiremurear);

        sprintf(path, "%s/%s/%s", SECT_FRNTRGTWHEEL, SECT_COMPOUNDS, SECT_HARD);
        tiremuFR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_FRNTLFTWHEEL, SECT_COMPOUNDS, SECT_HARD);
        tiremuFL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARRGTWHEEL, SECT_COMPOUNDS, SECT_HARD);
        tiremuRR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARLFTWHEEL, SECT_COMPOUNDS, SECT_HARD);
        tiremuRL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        tiremufront = MIN(tiremuFL, tiremuFR);
        tiremurear = MIN(tiremuRL, tiremuRR);
        mTireMuC[3] = MIN(tiremufront, tiremurear);

        sprintf(path, "%s/%s/%s", SECT_FRNTRGTWHEEL, SECT_COMPOUNDS, SECT_WET);
        tiremuFR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_FRNTLFTWHEEL, SECT_COMPOUNDS, SECT_WET);
        tiremuFL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARRGTWHEEL, SECT_COMPOUNDS, SECT_WET);
        tiremuRR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARLFTWHEEL, SECT_COMPOUNDS, SECT_WET);
        tiremuRL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        tiremufront = MIN(tiremuFL, tiremuFR);
        tiremurear = MIN(tiremuRL, tiremuRR);
        mTireMuC[4] = MIN(tiremufront, tiremurear);

        sprintf(path, "%s/%s/%s", SECT_FRNTRGTWHEEL, SECT_COMPOUNDS, SECT_EXTREM_WET);
        tiremuFR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_FRNTLFTWHEEL, SECT_COMPOUNDS, SECT_EXTREM_WET);
        tiremuFL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARRGTWHEEL, SECT_COMPOUNDS, SECT_EXTREM_WET);
        tiremuRR = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        sprintf(path, "%s/%s/%s", SECT_REARLFTWHEEL, SECT_COMPOUNDS, SECT_EXTREM_WET);
        tiremuRL = GfParmGetNum(CarHandle, path, PRM_MU, (char*)NULL, mTireMu);
        tiremufront = MIN(tiremuFL, tiremuFR);
        tiremurear = MIN(tiremuRL, tiremuRR);
        mTireMuC[5] = MIN(tiremufront, tiremurear);

        int compounds = GfParmGetNum(CarHandle, SECT_TIRESET, PRM_COMPOUNDS_SET, NULL, 1);
        mTireMu = mTireMuC[compounds];
        LogUSR.debug("# USR tire mu = %.2f\n", mTireMu);
    }

    mBrakePressMax = GfParmGetNum(CarHandle, SECT_BRKSYST, PRM_BRKPRESS, (char*)NULL, 0);
    mBrakeRepartition = GfParmGetNum(CarHandle, SECT_BRKSYST, PRM_BRKREP, (char*)NULL, 0);
    mFrontWingAngle = GfParmGetNum(CarHandle, SECT_FRNTWING, PRM_WINGANGLE, (char*)NULL, 0);
    mTires.setTYC(HASTYC);
}

void MyCar::initVars()
{
    mShiftTimer = 0;
    mAbsFactor = 1.0;
    mTclFactor = 1.0;
    mAccel = 0.0;
}

void MyCar::initCa()
{
    char* WheelSect[4] = {(char*)SECT_FRNTRGTWHEEL, (char*)SECT_FRNTLFTWHEEL, (char*)SECT_REARRGTWHEEL, (char*)SECT_REARLFTWHEEL};
    double frontwingarea = GfParmGetNum(mCar->_carHandle, SECT_FRNTWING, PRM_WINGAREA, (char*)NULL, 0);
    double rearwingarea = GfParmGetNum(mCar->_carHandle, SECT_REARWING, PRM_WINGAREA, (char*)NULL, 0);
    double frontclift = GfParmGetNum(mCar->_carHandle, SECT_AERODYNAMICS, PRM_FCL, (char*)NULL, 0);
    double rearclift = GfParmGetNum(mCar->_carHandle, SECT_AERODYNAMICS, PRM_RCL, (char*)NULL, 0);

    double frntwingca = 1.23 * frontwingarea * sin(mFrontWingAngle);
    double rearwingca = 1.23 * rearwingarea * sin(mRearWingAngle);
    double h = 0.0;

    for (int i = 0; i < 4; i++)
    {
        h += GfParmGetNum(mCar->_carHandle, WheelSect[i], PRM_RIDEHEIGHT, (char*)NULL, 0);
    }

    h*= 1.5; h = h * h; h = h * h; h = 2.0 * exp(-3.0 * h);
    mFrontCA = h * frontclift + 4.0 * frntwingca;
    mRearCA = h * rearclift + 4.0 * rearwingca;
    mCA = 2.0 * MIN(mFrontCA, mRearCA);
}

void MyCar::initCw()
{
    double cx = GfParmGetNum(mCar->_carHandle, SECT_AERODYNAMICS, PRM_CX, (char*)NULL, 0);
    double frontarea = GfParmGetNum(mCar->_carHandle, SECT_AERODYNAMICS, PRM_FRNTAREA, (char*)NULL, 0);

    mBodyCW = 0.645 * cx * frontarea;

    double frontwingarea = GfParmGetNum(mCar->_carHandle, SECT_FRNTWING, PRM_WINGAREA, (char*)NULL, 0);
    double rearwingarea = GfParmGetNum(mCar->_carHandle, SECT_REARWING, PRM_WINGAREA, (char*)NULL, 0);

    double frntwingca = 1.23 * frontwingarea * sin(mFrontWingAngle);
    double rearwingca = 1.23 * rearwingarea * sin(mRearWingAngle);
    mWingCW = frntwingca + rearwingca;

    mCW = mBodyCW + mWingCW;
}

void MyCar::initBrakes()
{
    double frontpistonarea = GfParmGetNum(mCar->_carHandle, SECT_FRNTRGTBRAKE, PRM_BRKAREA, (char*)NULL, 0);
    double rearpistonarea = GfParmGetNum(mCar->_carHandle, SECT_REARRGTBRAKE, PRM_BRKAREA, (char*)NULL, 0);
    double frontdiskmu = GfParmGetNum(mCar->_carHandle, SECT_FRNTRGTBRAKE, PRM_MU, (char*)NULL, 0);
    double reardiskmu = GfParmGetNum(mCar->_carHandle, SECT_REARRGTBRAKE, PRM_MU, (char*)NULL, 0);

    double maxf = 2.0 * mBrakeRepartition * mBrakePressMax * mCar->_brakeDiskRadius(0) * frontpistonarea * frontdiskmu / mCar->_wheelRadius(0);
    double maxr = 2.0 * (1 - mBrakeRepartition) * mBrakePressMax * mCar->_brakeDiskRadius(2) * rearpistonarea * reardiskmu / mCar->_wheelRadius(2);
    mBrakeForceMax = maxf + maxr;
}

double MyCar::calcFuel(double dist) const
{
    double tiredist = dist / mTireWearPerMeter;
    LogUSR.info("Tire distance : %.7f\n", tiredist);
    double mindist = MIN(dist, tiredist);
    LogUSR.info("Minimum distance : %.3f\n", mindist);
    double fuel = mindist * mFuelPerMeter;
    LogUSR.info("calcul fuel : %.3f\n", fuel);

    return Utils::clip(fuel, 0.0, mTankVol);
}

void MyCar::update(double dt)
{
    mDeltaTime = dt;
    mMass = mCarMass + mFuelWeightFactor * mCar->_fuel;
    mSpeedX = mCar->_speed_x;
    mTires.update();

    if(HASTYC)
    {
        LogUSR.debug("Friction : %.8f- Tyre temperature = %.3f\n", mTires.TyreCondition(), mCar->priv.wheel[0].temp_mid);
    }

    mSegMu = mTires.gripFactor() * mTireMu * mCar->_trkPos.seg->surface->kFriction;
    LogUSR.debug("# USR CarModel Tire Mu = %.3f - SegMu = %.3f\n", mTireMu, mSegMu);
    mCW = mBodyCW * (1.0 + mCar->_dammage / 10000.0) + mWingCW;
    mToMiddle = mCar->_trkPos.toMiddle;
    double yawdiff = Utils::normPiPi(mCar->_yaw - mYaw);
    mYaw = mCar->_yaw;
    mHeading = Vec3d(cos(mYaw), sin(mYaw), 0.0);
    Vec3d prevGlobalPos = mGlobalPos;
    mGlobalPos = Vec3d(mCar->_pos_X, mCar->_pos_Y, mCar->_pos_Z);
    mFrontAxleGlobalPos = mGlobalPos + mHeading * mFrontAxleOffset;
    Vec3d speedv = (mGlobalPos - prevGlobalPos) / dt;
    mSpeed = speedv.len();
    mSpeedYaw = speedv.getVec2().angle();
    double distdiff = (mGlobalPos - prevGlobalPos).len();

    if (distdiff > 0.05)
    {
        mYawRate = yawdiff / distdiff;
    }
    else
    {
        mYawRate = 0.0;
    }

    mAngleToTrack = Utils::normPiPi(mTrack->yaw(mCar->_distFromStartLine) - mYaw);
    mBorderDist = mCar->_trkPos.seg->width / 2.0 - fabs(mToMiddle) - mCar->_dimension_y / 2.0;
    mDamageDiff = mCar->_dammage - mLastDamage;
    mLastDamage = mCar->_dammage;
    bool mOnLeftSide = toMid() > 0.0 ? true : false;

    if (mCar->_trkPos.seg->side[mOnLeftSide] != NULL)
    {
        mBorderFriction = mCar->_trkPos.seg->side[mOnLeftSide]->surface->kFriction;
    }
    else
    {
        mBorderFriction = 1.0;
    }

    bool mAngleToLeft = mAngleToTrack < 0.0 ? true : false;

    if (gear() == -1)
    {
        mPointingToWall = (mAngleToLeft != mOnLeftSide) ? true : false;
    }
    else
    {
        mPointingToWall = (mAngleToLeft == mOnLeftSide) ? true : false;
    }

    mWallToMiddleAbs = mCar->_trkPos.seg->width / 2.0;

    if (mCar->_trkPos.seg->side[mOnLeftSide] != NULL)
    {
        int style = mCar->_trkPos.seg->side[mOnLeftSide]->style;

        if (style < 2)
        {
            mWallToMiddleAbs += mCar->_trkPos.seg->side[mOnLeftSide]->width;

            if (mCar->_trkPos.seg->side[mOnLeftSide]->side[mOnLeftSide] != NULL)
            {
                mWallToMiddleAbs += mCar->_trkPos.seg->side[mOnLeftSide]->side[mOnLeftSide]->width;
            }
        }
    }

    mWalldist = mWallToMiddleAbs - fabs(toMid());

    mAccelFilter.sample(20, mAccel);
    mMaxAccelForce = mCar->_gearRatio[mCar->_gear + mCar->_gearOffset] * mCar->_engineMaxTq / mCar->_wheelRadius(0);
    mSideSlip = mCar->_wheelSlipSide(0) + mCar->_wheelSlipSide(1) + mCar->_wheelSlipSide(2) + mCar->_wheelSlipSide(3);
}

void MyCar::setControls(double accel, double brake, double steer)
{
    mAccel = accel;

    mCar->_accelCmd = (tdble)accel;
    mCar->_brakeCmd = (tdble)brake;
    mCar->_steerCmd = (tdble)steer;
    mCar->_gearCmd = calcGear();
    mCar->_clutchCmd = (tdble)calcClutch();  // must be after gear
    mCar->_lightCmd = (RM_LIGHT_HEAD1 | RM_LIGHT_HEAD2);
}

double MyCar::brakeForce(double speed, double curvature, double curv_z, double mu, double pitchAngle, double rollAngle, PathType pathtype) const
{
    double friction = mu * mTires.gripFactor() * (mMass * 9.81 * (1.0 + sin(pitchAngle) + sin(rollAngle)) + mCA * speed * speed);
    double centrifugal = mMass * speed * speed * fabs(curvature) * (1.0 - sin(rollAngle));
    centrifugal = MIN(centrifugal, friction);
    double brakeforce = MAX(0.03 * maxBrakeForce(), sqrt(friction * friction - centrifugal * centrifugal));

    return std::min(brakeforce, maxBrakeForce());
}

double MyCar::curveSpeed(double curvature, double curv_z, double mu, double rollAngle, PathType pathtype) const
{
    curvature = fabs(curvature);

    if (curv_z < -0.002)
    {
        double factor = 1.8;

        if (pathtype != PATH_O)
        {
            factor = 2.5;
        }

        curvature = -curv_z * factor + curvature;
    }
    double radius = Utils::calcRadius(curvature);

    return sqrt(mu * 9.81 * (1.0 + sin(rollAngle)) * radius / (1.0 - MIN(0.99, radius * mCA * mu / mMass)));
}

double MyCar::bumpSpeed(double curv_z) const
{
    double speed_z = DBL_MAX;

    if (curv_z < -0.002)
    {
        speed_z = mBumpSpeedFactor * sqrt(9.81 / -(curv_z));
    }

    return speed_z;
}

int MyCar::calcGear()
{
    double SHIFT_UP = mShiftUpPoint;           // [-] (% of rpmredline)
    double SHIFT_DOWN_MARGIN = 130.0; // [rad/s] down from rpmredline
    int shifttime = 25;
    int MAX_GEAR = mCar->_gearNb - 1;
    const tCarCtrl &ctrl = mCar->ctrl;

    if (ctrl.rgcApplying)
    {
        return ctrl.gear;
    }

    if (mShiftUpPoint < 1.0  && mShiftUpPoint > 0.0)
    {
        SHIFT_DOWN_MARGIN = 130.0 / mShiftUpPoint;
    }

    if (v() < 1.0)
    {
        // For the start
        shifttime = 0;
    }

    if (mShiftTimer < shifttime)
    {
        mShiftTimer++;
    }

    if (mShiftTimer < shifttime)
    {
        return gear();
    }

    if (mGearDirection == -1)
    {
        return -1;
    }

    if (gear() <= 0)
    {
        return 1;
    }

    if (gear() < MAX_GEAR && mCar->_enginerpm / mCar->_enginerpmRedLine > SHIFT_UP)
    {
        mShiftTimer = 0;
        return gear() + 1;
    }
    else
    {
        double ratiodown = mCar->_gearRatio[gear() + mCar->_gearOffset - 1] / mCar->_gearRatio[gear() + mCar->_gearOffset];

        if (gear() > 1 && (mCar->_enginerpmRedLine - SHIFT_DOWN_MARGIN) / mCar->_enginerpm > ratiodown)
        {
            mShiftTimer = 0;

            return gear() - 1;
        }
    }

    return gear();
}

double MyCar::calcClutch()
{
    if (gear() > 1 || v() > 15.0)
    {
        if (gear() > mPrevGear)
        {
            mClutch = 0.3;
        }

        if (mCar->_enginerpm / mCar->_enginerpmRedLine > 0.2)
        {
            mClutch -= 0.04;
        }
        else
        {
            mClutch += 0.04;
        }

        if (gear() < mPrevGear)
        {
            mClutch = 0.0;
        }
    }
    else if (gear() == 1)
    {
        if (mCar->_enginerpm / mCar->_enginerpmRedLine > 0.2)
        {
            mClutch -= 0.04;
        }
        else
        {
            mClutch += 0.04;
        }

        if (fabs(mAngleToTrack) > 1.0 || mBorderDist < -2.0)
        {
            mClutch = 0.0;
        }
    }
    else if (gear() == 0)
    {
        // For a good start
        mClutch = 0.2;
    }
    else if (gear() == -1)
    {
        // For the reverse gear.
        if (mCar->_enginerpm > 500.0)
        {
            mClutch -= 0.01;
        }
        else
        {
            mClutch += 0.01;
        }
    }

    mPrevGear = gear();
    mClutch = Utils::clip(mClutch, 0.0, 1.0);

    return mClutch;
}

double MyCar::filterABS(double brake)
{
    double ABS_MINSPEED = 3.0;      // [m/s]

    if (mSpeed < ABS_MINSPEED)
    {
        return brake;
    }

    mSlip = slipFront() + slipRear();

    if (mAccel > 0.0)
    {
        mAbsFactor = 0.8;
    }

    if (fabs(mSideSlip) > 30.0)
    {
        mAbsFactor -= 0.1;
    }
    else if (mSlip < -mAbsSlip)
    {
        mAbsFactor -= 0.1;
    }
    else
    {
        mAbsFactor += 0.1;
    }

    mAbsFactor = Utils::clip(mAbsFactor, 0.1, 1.0);

    return brake *= mAbsFactor;
}

double MyCar::filterTCL(double accel)
{
    double factor = 3.1;

    if (fabs(mCar->_steerCmd) > 0.2)
    {
        factor = 2.5 * mTires.gripFactor();
    }

    double TCL_SLIP = factor * mTires.gripFactor();

    double diff = MAX(slipFront() - TCL_SLIP, slipRear() - TCL_SLIP);
    mTclController.mP = 0.19;
    mTclController.mD = 0.002;
    mTclFactor -= mTclController.sample(diff, mDeltaTime);
    mTclFactor = Utils::clip(mTclFactor, 0.0, 1.0);

    return accel *= mTclFactor;
}

double MyCar::slipFront() const
{
    return (mCar->_wheelSpinVel(FRNT_RGT) + mCar->_wheelSpinVel(FRNT_LFT)) * mCar->_wheelRadius(FRNT_LFT) / 2.0 - mSpeedX;
}

double MyCar::slipRear() const
{
    return (mCar->_wheelSpinVel(REAR_RGT) + mCar->_wheelSpinVel(REAR_LFT)) * mCar->_wheelRadius(REAR_LFT) / 2.0 - mSpeedX;
}

double MyCar::filterTCLSideSlip(double accel) const
{
    if (HASTYC == true && mCar->_remainingLaps < 5 && mTires.wear() < 0.8)
    {
        double slipQualy = MAX(mSideSlipTCL, mSideSlipTCLQualy);

        if (fabs(mSideSlip) > slipQualy * 2)
            return 0.0;
        else if (fabs(mSideSlip) > slipQualy)
            return accel * ((1.0 - MIN(0.7, (fabs(mSideSlip) - slipQualy) * mSideSlipTCLFactor)));
    }
    else
    {
        if (fabs(mSideSlip) > mSideSlipTCL * 2)
            return 0.0;
        else if (fabs(mSideSlip) > mSideSlipTCL)
            return accel * ((1.0 - MIN(0.7, (fabs(mSideSlip) - mSideSlipTCL) * mSideSlipTCLFactor)));
    }

    return accel;
}

bool MyCar::learningOfftrack()
{
    // Offtrack situations
    double offtrackmargin = 1.0;

    if (borderDist() < -offtrackmargin)
    {
        //std::cout << "offtrack "<< borderDist() << std::endl;
        return true;
    }

    // Barrier collisions
    if (damageDiff() > 0 && wallDist() - mCar->_dimension_y / 2.0 < 0.5)
    {
        LogUSR.info("barrier coll : %u\n", damageDiff());

        return true;
    }

    return false;
}
