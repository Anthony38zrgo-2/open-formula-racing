/***************************************************************************

    file                 : OsgWheel.cpp
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C) 2012 by Gaëtan André
    email                : gaetan.andre@gmail.com

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Portability don't build with MingW 4.8 */
#ifdef _MSC_VER
#include <portability.h>
#endif // _MSC_VER

#include "OsgLoader.h"
#include "OsgWheel.h"
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>

namespace osggraph {

osg::ref_ptr<osg::Node> SDWheels::initWheels(tCarElt *car_elt,void *handle,
    osgLoader::loadedModels &loaded)
{
    this->car = car_elt;

    this->brakes.setCar(car);

    const char *wheelSoft3DModNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_WHEELSOFT_3D, "");
    const char *wheelMedium3DModNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_WHEELMEDIUM_3D, "");
    const char *wheelHard3DModNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_WHEELHARD_3D, "");
    const char *wheelWet3DModNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_WHEELWET_3D, "");
    const char *wheelExtremWet3DModNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_WHEELEXTWET_3D, "");

    const char *wheelFront3DModFileNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_FRONT_WHEEL_3D, "");
    const char *wheelRear3DModFileNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_REAR_WHEEL_3D, "");
    const char *wheel3DModFileNamePrfx =
            GfParmGetStr(handle, SECT_GROBJECTS, PRM_WHEEL_3D, "wheel");

    if (*wheelSoft3DModNamePrfx)
    {
        wheels[FRNT_RGT] =initWheel(FRNT_RGT, true, wheelSoft3DModNamePrfx, wheelMedium3DModNamePrfx,
                                    wheelHard3DModNamePrfx, wheelWet3DModNamePrfx, wheelExtremWet3DModNamePrfx,
                                    loaded);
        wheels[FRNT_LFT] =initWheel(FRNT_LFT, true, wheelSoft3DModNamePrfx, wheelMedium3DModNamePrfx,
                                    wheelHard3DModNamePrfx, wheelWet3DModNamePrfx, wheelExtremWet3DModNamePrfx,
                                    loaded);

        wheels[REAR_RGT] =initWheel(REAR_RGT, true, wheelSoft3DModNamePrfx, wheelMedium3DModNamePrfx,
                                    wheelHard3DModNamePrfx, wheelWet3DModNamePrfx, wheelExtremWet3DModNamePrfx,
                                    loaded);
        wheels[REAR_LFT] =initWheel(REAR_LFT, true, wheelSoft3DModNamePrfx, wheelMedium3DModNamePrfx,
                                    wheelHard3DModNamePrfx, wheelWet3DModNamePrfx, wheelExtremWet3DModNamePrfx,
                                    loaded);
    }
    else
    {
        if (*wheelFront3DModFileNamePrfx)
        {
            wheels[FRNT_RGT] = initWheel(FRNT_RGT, false, wheelFront3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
            wheels[FRNT_LFT] = initWheel(FRNT_LFT, false, wheelFront3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
        }
        else
        {
            wheels[FRNT_RGT] =initWheel(FRNT_RGT, false, wheel3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
            wheels[FRNT_LFT] =initWheel(FRNT_LFT, false, wheel3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
        }

        if (*wheelRear3DModFileNamePrfx)
        {
            wheels[REAR_RGT] =initWheel(REAR_RGT, false, wheelRear3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
            wheels[REAR_LFT] =initWheel(REAR_LFT, false, wheelRear3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
        }
        else
        {
            wheels[REAR_RGT] =initWheel(REAR_RGT, false, wheel3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
            wheels[REAR_LFT] =initWheel(REAR_LFT, false, wheel3DModFileNamePrfx, NULL, NULL, NULL, NULL, loaded);
        }
    }

    osg::ref_ptr<osg::Group> group = new osg::Group;

    for(int i=0;i<4;i++)
    {
        group->addChild(wheels[i]);
    }

    return group.get();
}

osg::ref_ptr<osg::MatrixTransform> SDWheels::initWheel(int wheelIndex, bool compound, const char *wheel_mod_name,
                                                       const char *medium_mod_name, const char *hard_mod_name,
                                                       const char *wet_mod_name, const char *extwet_mod_name,
                                                       osgLoader::loadedModels &loaded)
{
    osgLoader loader;
    static const char* wheel_model_attributes[] = {
        "front right wheel model", "front left wheel model",
        "rear right wheel model", "rear left wheel model"
    };
    const char* configured_wheel_model = GfParmGetStr(car->_carHandle, SECT_GROBJECTS,
        wheel_model_attributes[wheelIndex], "");
    if (*configured_wheel_model)
        wheel_mod_name = configured_wheel_model;
    char wheel_file_name[32];
    static const int MaxPathSize = 512;
    char buf[MaxPathSize];
    compounds = compound;

    const bool bCustomSkin = strlen(this->car->_skinName) != 0;

    std::string bSkinName;
    std::string TmpPath;

    if (bCustomSkin)
    {
        std::string dataDir = GfDataDir(), localDir = GfLocalDir();
        snprintf(buf, MaxPathSize, "cars/models/%s/wheel3d-%s.png", car->_carName, car->_skinName);
        std::string dataSkin = TmpPath + buf,
            localSkin = localDir + buf;

        if (osgDB::fileExists(localSkin))
        {
            TmpPath = localDir;
            snprintf(buf, MaxPathSize, "%s", car->_skinName);
            bSkinName = buf;
        }
        else if (osgDB::fileExists(dataSkin))
        {
            TmpPath = dataDir;
            snprintf(buf, MaxPathSize, "%s", car->_skinName);
            bSkinName = buf;
        }

        GfLogInfo("Car Texture = %s\n", bSkinName.c_str());
    }
    std::string strTPath;
    snprintf(buf, MaxPathSize, "drivers/%s/%d/", car->_modName, car->_driverIndex);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    snprintf(buf, MaxPathSize, "cars/models/%s/", car->_carName);
    strTPath = TmpPath+buf;
    loader.AddSearchPath(strTPath);

    snprintf(buf, MaxPathSize, "%scars/models/%s/", GfLocalDir(), car->_carName);
    loader.AddSearchPath(buf);
    GfLogDebug("Chemin Textures : %s\n", strTPath.c_str());

    loader.AddSearchPath("data/objects/");
    loader.AddSearchPath("data/textures/");

    wheels_switches[wheelIndex] = new osg::Switch;

    if (compounds)
    {
        for(int j=0; j<4; j++)
        {
            osg::ref_ptr<osg::Node> wheel = 0;
            if (wheel_mod_name && strlen(wheel_mod_name))
            {
                snprintf(wheel_file_name, 32, "%s%d.acc", wheel_mod_name, j);
                wheel = loader.Load3dFile(loaded, wheel_file_name, true,
                    car->_carName, bSkinName);
                wheels_switches[wheelIndex]->addChild(wheel.get(), false);
                GfLogDebug("Loading compound %s\n", wheel_mod_name);
            }
        }

        for(int j=0; j<4; j++)
        {
            osg::ref_ptr<osg::Node> wheel = 0;
            if (medium_mod_name && strlen(medium_mod_name))
            {
                snprintf(wheel_file_name, 32, "%s%d.acc", medium_mod_name, j);
                wheel = loader.Load3dFile(loaded, wheel_file_name, true,
                    car->_carName, bSkinName);
                wheels_switches[wheelIndex]->addChild(wheel.get(), false);
                GfLogDebug("Loading compound %s\n", medium_mod_name);
            }
        }

        for(int j=0; j<4; j++)
        {
            osg::ref_ptr<osg::Node> wheel = 0;
            if (hard_mod_name && strlen(hard_mod_name))
            {
                snprintf(wheel_file_name, 32, "%s%d.acc", hard_mod_name, j);
                wheel = loader.Load3dFile(loaded, wheel_file_name, true,
                    car->_carName, bSkinName);
                wheels_switches[wheelIndex]->addChild(wheel.get(), false);
                GfLogDebug("Loading compound %s\n", hard_mod_name);
            }
        }

        for(int j=0; j<4; j++)
        {
            osg::ref_ptr<osg::Node> wheel = 0;
            if (wet_mod_name && strlen(wet_mod_name))
            {
                snprintf(wheel_file_name, 32, "%s%d.acc", wet_mod_name, j);
                wheel = loader.Load3dFile(loaded, wheel_file_name, true,
                    car->_carName, bSkinName);
                wheels_switches[wheelIndex]->addChild(wheel.get(), false);
                GfLogDebug("Loading compound %s\n", wet_mod_name);
            }
        }

        for(int j=0; j<4; j++)
        {
            osg::ref_ptr<osg::Node> wheel = 0;
            if (extwet_mod_name && strlen(extwet_mod_name))
            {
                snprintf(wheel_file_name, 32, "%s%d.acc", extwet_mod_name, j);
                wheel = loader.Load3dFile(loaded, wheel_file_name, true,
                    car->_carName, bSkinName);
                wheels_switches[wheelIndex]->addChild(wheel.get(), false);
                GfLogDebug("Loading compound %s\n", extwet_mod_name);
            }
        }
    }
    else
    {

        // Load speed-dependant 3D wheel model if available
        for(int j=0; j<4; j++)
        {
            osg::ref_ptr<osg::Node> wheel = 0;
            if (wheel_mod_name && strlen(wheel_mod_name))
            {
                snprintf(wheel_file_name, 32, "%s%d.acc", wheel_mod_name, j);
                wheel = loader.Load3dFile(loaded, wheel_file_name, true,
                    car->_carName, bSkinName);
                wheels_switches[wheelIndex]->addChild(wheel.get(), false);
            }
        }
    }

    osg::ref_ptr<osg::MatrixTransform> whlsize = new osg::MatrixTransform;
    float wheelRadius = car->_rimRadius(wheelIndex) + car->_tireHeight(wheelIndex);
    float w = car->_tireWidth(wheelIndex);
    float dia = wheelRadius * 2;

    osg::Matrix wheelsize = osg::Matrix::scale(dia, w, dia);

    whlsize->setMatrix(wheelsize);
    whlsize->addChild(wheels_switches[wheelIndex]);

    if(wheelIndex == FRNT_RGT || wheelIndex == REAR_RGT )
    {
        osg::ref_ptr<osg::MatrixTransform> flipright = new osg::MatrixTransform;
        flipright->setMatrix(osg::Matrix::rotate(osg::PI, osg::Z_AXIS));
        flipright->addChild(whlsize.get());
        whlsize = flipright.get();
    }

    osg::ref_ptr<osg::MatrixTransform> transform1 = new osg::MatrixTransform;
    transform1->addChild(whlsize.get());

    osg::ref_ptr<osg::MatrixTransform> transform2 = new osg::MatrixTransform;
    transform2->addChild(transform1.get());

    //initiating brakes
    transform2->addChild(this->brakes.initBrake(wheelIndex));

    return transform2.get();
}

void SDWheels::updateWheels()
{
    int j;
    static const float maxVel[3] = { 20.0, 40.0, 70.0 };

    brakes.updateBrakes();

    for(int i=0; i<4; i++)
    {
        osg::Matrix spinMatrix = osg::Matrix::rotate(car->priv.wheel[i].relPos.ay, osg::Y_AXIS);

        osg::Matrix posMatrix = osg::Matrix::translate(car->priv.wheel[i].relPos.x, car->priv.wheel[i].relPos.y, car->priv.wheel[i].relPos.z);

        osg::Matrix camberDirMatrix = osg::Matrix::rotate(car->priv.wheel[i].relPos.ax, osg::X_AXIS, //camber
                                                          0.0, osg::Y_AXIS,
                                                          car->priv.wheel[i].relPos.az, osg::Z_AXIS ); //direction

        posMatrix = camberDirMatrix * posMatrix;
        osg::MatrixTransform * trans = dynamic_cast<osg::MatrixTransform *>(wheels[i]->getChild(0));
        trans->setMatrix(spinMatrix);
        wheels[i]->setMatrix(posMatrix);


        for (j = 0; j < 3; j++)
        {
            if (fabs(car->_wheelSpinVel(i)) < maxVel[j])
                break;
        }

        int compoundSelector = (car->priv.wheel[i].compound - 1);
        int selectchild = compoundSelector * 4;
        int v = j + selectchild;

        if (compounds)
        {
            this->wheels_switches[i]->setSingleChildOn(v);
            GfLogDebug("Drawing Compounds = %i\n", v);
        }
        else
            this->wheels_switches[i]->setSingleChildOn(j);
    }
}

} // namespace osggraph
