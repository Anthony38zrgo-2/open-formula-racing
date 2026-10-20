/***************************************************************************

    file                 : OsgScenery.h
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

#ifndef _OSGSCENERY_H_
#define _OSGSCENERY_H_

#include <string>

#include <track.h>	//tTrack
#include <raceman.h> // tSituation

#include <osg/Group>

namespace osggraph {

class	SDBackground;
class   SDPit;
//class   SDTrackLights;
class	SDScenery;
class   osgLoader;

#define SKYBIN 1
#define TRACKBIN 2

class SDBackground
{
    osg::ref_ptr<osg::Group>				_background;
    osg::ref_ptr < osg::MatrixTransform>	_backgroundTransform;

    bool	_type;

public:
    // Constructor
    SDBackground(void);

    // Destructor
    ~SDBackground(void);

    void build(int X, int Y, int Z, const std::string& strTrack);
    void reposition(double X, double Y, double Z);

    osg::ref_ptr<osg::Group> getBackground() { return _background.get(); }
};

class SDPit
{
private:
    osg::ref_ptr<osg::Group> pit_root;

public:
    // Constructor
    SDPit(void);

    // Destructor
    ~SDPit(void);

    void build(const tTrack *track);

    osg::ref_ptr<osg::Group> getPit() const { return pit_root; }
};

class SDTrackLights
{
private:
    class Internal;
    Internal *internal;

    osg::ref_ptr<osg::Group> _osgtracklight;

public:
    // Constructor
    SDTrackLights(void);

    // Destructor
    ~SDTrackLights(void);

    void build(const tTrack *track);
    void update(double currentTime, double totTime, int raceType);

    osg::ref_ptr<osg::Group> getTrackLight() { return _osgtracklight.get(); }
};

class SDScenery
{
private:
    SDBackground	*m_background;
    SDPit           *m_pit;
    SDTrackLights   *m_tracklights;

    osg::ref_ptr<osg::Group> _scenery;

    int _max_visibility;
    int _nb_cloudlayer;
    int _DynamicSkyDome;
    int _SkyDomeDistance;
    int _SkyDomeDistThresh;

    bool _bgsky;
    bool _speedWay;
    bool _speedWayLong;

    static double grWrldX;
    static double grWrldY;
    static double grWrldZ;
    static double grWrldMaxSize;

    void LoadGraphicsOptions();
    void LoadSkyOptions();
    void CustomizePits(void);
    bool LoadTrack(const std::string &dir, const std::string &file);

public:
    /* Constructor */
    SDScenery(void);

    /* Destructor */
    ~SDScenery(void);

    int LoadScene(const tTrack *track);
    void ShutdownScene(void);
    void reposition(double X, double Y, double Z);
    void update_tracklights(double currentTime, double totTime, int raceType);

    inline static double getWorldX(){return grWrldX;}
    inline static double getWorldY(){return grWrldY;}
    inline static double getWorldZ(){return grWrldZ;}
    inline static double getWorldMaxSize(){return grWrldMaxSize;}
    bool getSpeedWay() const { return _speedWay; }
    bool getSpeedWayLong() const { return _speedWayLong; }

    osg::ref_ptr<osg::Group> getScene() { return _scenery.get(); }
    osg::ref_ptr<osg::Group> getBackground() { return m_background->getBackground(); }
    osg::ref_ptr<osg::Group> getPit() { return m_pit->getPit(); }
    osg::ref_ptr<osg::Group> getTracklight() { return m_tracklights->getTrackLight(); }
};

} // namespace osggraph

#endif //_OSGSCENERY_H_
