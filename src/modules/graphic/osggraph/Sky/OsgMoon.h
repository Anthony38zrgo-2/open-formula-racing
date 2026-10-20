/***************************************************************************

    file                 : OsgMoon.h
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C)2012 by Xavier Bertaux
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

#ifndef _OSGMOON_H
#define _OSGMOON_H

#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osg/Material>

namespace osggraph {

class SDMoon : public osg::Referenced
{
    osg::ref_ptr<osg::MatrixTransform> moon_transform;
    osg::ref_ptr<osg::Material> orb_material;

    double prev_moon_angle;
    double moon_angle;
    double moon_rotation;
    double moon_size;
    double moon_dist;
    double moonAscension;
    double moondeclination;
    osg::Vec3f moon_position;

public:

    // Constructor
    SDMoon( void );

    // Destructor
    ~SDMoon( void );

    // build the moon object
    osg::Node *build( std::string path, double dist, double size );

    bool repaint( double angle );
    bool reposition(osg::Vec3d p, double angle  );

    void setMoonAngle (double angle) { moon_angle = angle; }
    double getMoonAngle() const { return moon_angle; }

    void setMoonRotation (double rotation) { moon_rotation = rotation; }
    double getMoonRotation() const { return moon_rotation; }

    void setMoonRightAscension ( double ra ) { moonAscension = ra; }
    double getMoonRightAscension() const { return moonAscension; }

    void setMoonDeclination ( double decl ) { moondeclination = decl; }
    double getMoonDeclination() const { return moondeclination; }

    void setMoonDist ( double dist ) { moon_dist = dist; }
    double getMoonDist() const { return moon_dist; }

    const osg::Vec3f & getMoonPosition() const { return moon_position; }

};

} // namespace osggraph

#endif // _OSGMOON_H
