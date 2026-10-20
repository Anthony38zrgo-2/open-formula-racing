/***************************************************************************

    file                 : OsgBrake.h
    created              : Mon Dec 31 10:24:02 CEST 2012
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

#ifndef _OSGBRAKE_H_
#define _OSGBRAKE_H_

#include <osg/Geometry>
#include <car.h>

namespace osggraph {

class SDBrakes
{
private :
    tCarElt *car;
    osg::ref_ptr<osg::Vec4Array> brake_colors[4];

public :
    SDBrakes() : car(nullptr) { }
    void setCar(tCarElt * car_elt);
    osg::Node *initBrake(int wheelIndex);
    void updateBrakes();
};

} // namespace osggraph

#endif /* _OSGBRAKE_H_ */
