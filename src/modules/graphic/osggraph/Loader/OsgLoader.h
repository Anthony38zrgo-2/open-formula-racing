/***************************************************************************

    file                 : OsgLoader.h
    created              : Mon Aug 21 18:24:02 CEST 2012
    copyright            : (C)2013 by Xavier Bertaux
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

#ifndef OSGLOADER_H
#define OSGLOADER_H

#include <osgDB/ReadFile>

#include "ReaderWriterACC.h"
#include <vector>
#include <string>
//#include "ReaderWriterACC2.h"

namespace osggraph {

class osgLoader
{
protected:
    struct loadedModel
    {
        std::string strFile, skinname;
        osg::ref_ptr<osg::Node> node;
    };

public:
    typedef std::vector<loadedModel> loadedModels;
    osgLoader(void);
    ~osgLoader(void);

    osg::Node *Load3dFile(loadedModels &loaded,
        const std::string &strFile, bool car = false,
        const std::string &carname = "", const std::string& name = "");
    osg::ref_ptr<osg::Image> LoadImageFile(const std::string &strFile);
    void AddSearchPath(const std::string &strPath);

protected:
    osg::ref_ptr<osgDB::ReaderWriter::Options> m_pOpt;
    ReaderWriterACC m_ACCReader;
};

} // namespace osggraph

#endif
