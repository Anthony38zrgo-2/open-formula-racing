/***************************************************************************

    file                 : OsgRender.h
    created              : Mon Aug 21 20:09:40 CEST 2012
    copyright            : (C) 2012 Xavier Bertaux
    email                : bertauxx@yahoo.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGRENDER_H_
#define _OSGRENDER_H_

#include <osg/Group>
#include <osg/Fog>

#include <osgShadow/ShadowedScene>

#include <track.h>	 //tTrack

struct Situation;
typedef Situation tSituation;

namespace osggraph {

class SDSky;
class SDScenery;

class SDRender
{
private:
    osg::ref_ptr<osg::Group> m_Root;
    osg::ref_ptr<osg::Group> m_Scene;
    osg::ref_ptr<osg::Group> m_ShadowSlot;
    osg::ref_ptr<osg::Group> m_ShadowRoot;
    osg::ref_ptr<osg::Group> m_NonShadowRoot;
    osg::ref_ptr<osg::Group> m_CarRoot;
    osg::ref_ptr<osg::Group> m_CarLightsRoot;

    osg::ref_ptr<osg::StateSet> stateSet;
    osg::ref_ptr<osg::StateSet> skySS;
    osg::ref_ptr<osg::Fog>      m_Fog;

    osg::ref_ptr<osg::LightSource> sunLight;

    osg::Vec3f BaseSkyColor;
    osg::Vec3f BaseFogColor;
    osg::Vec3f SkyColor;
    osg::Vec3f FogColor;
    osg::Vec3f CloudsColor;

    osg::Vec4f SceneAmbiant;
    osg::Vec4f SceneDiffuse;
    osg::Vec4f SceneSpecular;
    osg::Vec4f SceneFog;

    osg::Vec4f Scene_ambiant;
    osg::Vec4f Scene_Diffuse;
    osg::Vec4f Scene_Specular;
    osg::Vec4f Scene_Emit;

    unsigned SDSkyDomeDistance;
    unsigned SDNbCloudLayers;
    unsigned SDSkyDomeDistThresh;

    int carsShader;
    int SDDynamicWeather;
    bool SDDynamicSkyDome;

    int cloudsTextureIndex;
    int cloudsTextureIndex2;
    int cloudsTextureIndex3;
    float SDSunDeclination;
    float SDMoonDeclination;
    float SDMax_Visibility;
    double SDVisibility;
    unsigned int SDRain;
    unsigned ShadowIndex;
    unsigned TexSizeIndex;
    unsigned QualityIndex;
    int ShadowTexSize;

    osg::Vec3d *AStarsData;
    osg::Vec3d *APlanetsData;
    int NStars;
    int NPlanets;
    float sol_angle;
    float moon_angle;
    float sky_brightness;

    SDSky *thesky;
    SDScenery *scenery;
    const tTrack *track;

public:
    SDRender(void);
    ~SDRender(void);

    void Init(const tTrack *track);
    osg::ref_ptr< osg::StateSet> setFogState();
    osg::Vec4f getSceneColor(void);
    void UpdateTime(tSituation *s);
    void UpdateLight(const tTrack *track);
    void addCars(osg::Node* cars, osg::Node* carLights);
    void UpdateFogColor(double angle);
    void UpdateSky(double currentTime, double accelTime, double X, double Y);
    void ShadowedScene(void);
    void weather(const tTrack *track);

    inline SDSky * getSky() { return thesky; }
    inline int getShader() const { return carsShader; }
    inline int getShaowIndex() const { return ShadowIndex; }
    osg::ref_ptr<osg::Group> getRoot() { return m_Root.get(); }
    const osg::Vec3f &getFogColor() const { return FogColor; }
};

} // namespace osggraph

#endif //_OSGRENDER_H_
