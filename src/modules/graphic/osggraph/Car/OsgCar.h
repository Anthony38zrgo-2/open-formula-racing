/***************************************************************************

    file                 : OsgCar.h
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C)2012 by Gaétan André, (C)2014 Xavier Bertaux
    email                : gaetan.andré@gmail.com

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGCAR_H_
#define _OSGCAR_H_

#include <vector>

#include <osg/TextureCubeMap>
#include <osg/LOD>

#include "OsgLoader.h"
#include "OsgWheel.h"
//#include "OsgBackFire.h"
#include "OsgCarLight.h"

struct CarElt;
typedef struct CarElt tCarElt;

namespace osggraph {

class SDCarShader;
class SDReflectionMapping;
class SDWheels;
class SDBackFire;
//class SDCarLight;

class SDCar
{
private :
    osg::ref_ptr<osg::MatrixTransform> carTransform;
    osg::ref_ptr<osg::MatrixTransform> lights_branch;
    osg::ref_ptr<osg::Switch>          LODSelector;
    osg::ref_ptr<osg::Switch>          DRMSelector;
    osg::ref_ptr<osg::Switch>          DRMSelector2;
    osg::ref_ptr<osg::Switch>          DriverSelector;
    osg::ref_ptr<osg::Switch>          SteerSelector;
    osg::ref_ptr<osg::MatrixTransform> SteerRot;
    osg::ref_ptr<osg::MatrixTransform> SteerRot2;
    osg::ref_ptr<osg::Switch>          RearWingSelector;
    osg::ref_ptr<osg::Group>           car_shaded_body;
    osg::ref_ptr<osg::Group>           carEntity;
    int                                nSteer;
    int                                DRMSelectMask[32];
    int                                nDRM;
    int                                nDRM2;
    float                              DRMThreshold[32];
    tdble                              steerWheelRot;

    //osg::ref_ptr<osg::Vec3Array> shadowVertices;
    //osg::ref_ptr<osg::Geometry> quad;
    //osg::ref_ptr<osg::Node> initOcclusionQuad(tCarElt *car);

    std::vector<SDCarLight> lights;

    tCarElt     *car;

    //SDWheels    wheels;
    SDCarShader *shader;
    SDReflectionMapping *reflectionMapping;
    //SDBackFire *backfire;

    int reflectionMappingMethod;

    void setReflectionMap(osg::ref_ptr<osg::Texture> map);
    void loadCarLights();
    int loadWing(void *handle, const char *path, const char *key,
        osgLoader &loader, const std::string &bCarName,
        const std::string &bSkinName, const char *name, bool value,
        osg::ref_ptr<osg::Switch> &wing, osgLoader::loadedModels &loaded)
        const;

public :
    SDCar(void);
    ~SDCar(void);

    osg::ref_ptr<osg::Node> loadCar(tCarElt *Car, bool tracktype, bool subcat,
        int carshader, osgLoader::loadedModels &loaded);
    bool isCar(const tCarElt*c) const;

    bool    _cockpit;
    bool    _driver;
    bool    _wing1;
    bool    _wing3;
    bool    _steer;
    bool    _carLOD;

    int     _carShader;

    SDReflectionMapping *getReflectionMap();

    int getReflectionMappingMethod() const;
    osg::ref_ptr<osg::Group> getCarOsgGroup() { return this->carTransform; }

    tCarElt *getCar() { return car; }
    const tCarElt *getCar() const { return car; }

    void markCarCurrent(const tCarElt *Car);
    void updateCar(tSituation *s, tCarElt *CurCar, int current, int driver);
    void updateShadingParameters(const osg::Matrixf &modelview);

    SDWheels    wheels;
};

class SDCars
{
private :
    std::vector<SDCar *> the_cars;
    osg::ref_ptr<osg::Group> cars_branch;
    osg::ref_ptr<osg::Group> shadow_branch;
    tSituation *situation;

    void addSDCar(SDCar * car);

public :
    SDCars(void);
    ~SDCars(void);

    void  loadCars(tSituation * pSituation, bool trackType, bool subCat,
        osgLoader::loadedModels &loaded);
    void  updateCars(tSituation *s, tCarElt *CurCar, int current, int driver);
    void  markCarCurrent(const tCarElt*car);
    SDCar *getCar(tCarElt*car);
    void  unLoad();
    void  updateShadingParameters(const osg::Matrixf &modelview);
    osg::ref_ptr<osg::Node> getShadowNode() { return shadow_branch.get(); }
    osg::ref_ptr<osg::Node> getCarsNode() { return cars_branch.get(); }

};

} // namespace osggraph

#endif /* _OSGCAR_H_ */
