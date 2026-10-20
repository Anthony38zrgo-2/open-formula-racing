/***************************************************************************

    file                     : OsgOptions.h
    created                  : Thu Mar 31 00:00:41 CEST 2015
    copyright                : (C) 2015 by Xavier Bertaux
    email                    : bertauxx@yahoo.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>

#ifndef _OSGOPTIONS_H_
#define _OSGOPTIONS_H_

namespace osggraph {

class SDOptions
{
public:
    SDOptions( void );
    ~SDOptions( void );

private:
    int     _SmokeValue;
    int     _SmokeDuration;
    int     _SmokeInterval;

    int     _SkidValue;
    int     _SkidLength;
    int     _SkidInterval;

    int     _LOD;

    bool    _bgsky;
    int     _DynamicSkyDome;
    int     _SkyDomeDistance;
    int     _Max_Visibility;
    int     _PrecipitationDensity;
    int     _Rain;

    int     _CloudLayer;

    int     _SceneLOD;
    bool    _Cockpit3D;

    int     _ShadowType;
    int     _ShadowSize;
    int     _ShadowQuality;

    int     _Shaders;
    int     _ShadersQuality;
    int     _ShadersSize;

    bool    _NormalMap;
};

} // namespace osggraph

#endif /* _OSGOPTIONS_H_ */
