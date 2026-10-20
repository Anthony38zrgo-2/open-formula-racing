/***************************************************************************

    file        : OsgHUD.h
    created     : Sun Nov 23 20:12:19 CEST 2014
    copyright   : (C) 2014 by Xavier Bertaux
    email       : Xavier Bertaux
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _OSGHUD_H_
#define _OSGHUD_H_

#include <vector>
#include <list>

#include <car.h>        // tCarElt
#include <raceman.h>    // tSituation

#include <osg/Camera>

namespace osgText
{
    class Text;
}

namespace osggraph {

class SDFrameInfo;

class OSGPLOT
{
public:
    struct PlotLineConfig
    {
        bool reference;
        float referenceLineAtValue;
        osg::Vec4 color;
        float maxValue;
        float minValue;
        float timeFrame;
        std::string Xdata;
        std::string Ydata;
    };

private:
    struct PlotLine : public PlotLineConfig
    {
    public:
        osg::Vec3Array *dataPoints;
        osg::Geometry *geometry;
        osg::Vec3Array *vertices;

    public:
        PlotLine(const PlotLineConfig &config);

        void appendDataPoint(float x, float y, float z)
        {
            //add the new element (as last of our vector)
            dataPoints->push_back(osg::Vec3(x, y, z));
        }
        void recalculateDrawnPoint(float currentTime, float positionX, float positionY, float width, float height);
        void clearDataPoints()
        {
            dataPoints->clear();
        }
    };

public:
    OSGPLOT(float positionX,
            float positionY,
            float width,
            float height,
            const std::string &title,
            std::vector<PlotLineConfig> lines);

    ~OSGPLOT();
    float positionX;
    float positionY;
    float width;
    float height;
    std::string title;
    std::list<PlotLine> plotLines;
    osg::ref_ptr<osg::Group> osgGroup;

    osg::ref_ptr <osg::Group> getGroup();
    void drawBackground();
    void update(tSituation *s, const SDFrameInfo* frameInfo,const tCarElt *currCar);
    void clearDataPoints();
};

class SDHUD
{
    private:
        osg::ref_ptr<osg::Camera> _cameraHUD;
        tCarElt *_car;

        //car data that need to be remembered between frames for the hud
        struct CarData {
            int carLaps;
            float remainingFuelForLaps;
            float laptimeFreezeCountdown;//keep display for x seconds
            float laptimeFreezeTime;
            float timeDiffFreezeCountdown;//keep display for x seconds
            float timeDiffFreezeTime;
            int oldSector;
            int oldLapNumber;
            std::string name;

            CarData() : carLaps(0), remainingFuelForLaps(0), laptimeFreezeCountdown(3.0f),
            laptimeFreezeTime(0.0f), timeDiffFreezeCountdown(8.0f), timeDiffFreezeTime(0.0f),
            oldSector(0), oldLapNumber(0)
            {
            }
        };
        const tCarElt *lastCar;
        std::map<const tCarElt *, CarData> carData;

        float hudScale;
        osg::ref_ptr <osg::Group> osgGroupHud;
        std::map<std::string,osg::Geode* > hudWidgets;
        std::map<std::string,osg::Geometry* > hudImgElements;
        std::map<std::string,osg::Geometry* > hudImgVertexElements;
        std::map<std::string,osg::MatrixTransform* > hudImgRotableElements;
        std::map<std::string,OSGPLOT* > hudGraphElements;

        float prevSteerAngle;

        std::map<std::string, osgText::Text * > hudTextElements;
        std::map<std::string, int> hudElementsVisibilityStatus;
        bool hudElementsVisibilityStatusEnabled;
        bool hudEditModeEnabled;
        std::string selectedWidgetGroup;

        static osg::Vec3 calculatePosition(osg::BoundingBox mybb, const std::string &objPoint,
                                           osg::BoundingBox bb, const std::string &referenceObjPoint,
                                           float verticalModifier, float horizontalModifier);
        static std::string formatLaptime(tdble sec, int sgn);
        static void changeImageSize(osg::Geometry *geom,
                                    float newSize,
                                    const std::string &resizeFrom,
                                    float hudScale);
        static void changeImagePosition(osg::Geometry *geom,
                                        float newX,
                                        float newY,
                                        float hudScale);
        static void changeImageAlpha(osg::Geometry *geom, float newAlpha);
        static void changeImageTexture(osg::Geometry *geom, const std::string &newTextureUrl);
        static void changeImageVertex(osg::Geometry *geom,
                                        float angleFrom,
                                        float angleTo,
                                        float currValue,
                                        float newX,
                                        float newY,
                                        float hudScale);
        osg::BoundingBox getBoundigBoxFromWidgetName(std::string widgetName);
        osg::BoundingBox getBoundigBoxFromWidgetGroupName(std::string widgetGroupName);
        void recalculateImageWidgetPosition(std::string widgetGroupName, std::string widgetName, float hudScale);
        void recalculateTextWidgetPosition(std::string widgetGroupName, std::string widgetName, float hudScale);
        bool isMouseOverWidgetGroup (std::string widgetGroupName);
        void selectWidgetGroupByName(std::string widgetGroupName);
        void setWidgetsGroupsVisibilityForcedON();
        void setWidgetsGroupsVisibilityNormal();
        void saveWidgetGroupPosition(std::string widgetGroupName);
        std::string &GetCarName(const tCarElt *currCar);

public:
        SDHUD();
        ~SDHUD();

        void ToggleHUD();
        void ToggleHUDwidget(const std::string &widget);
        void ToggleHUDwidgets(const std::string &widgets);

        void ToggleHUDdriverinput();

        void ToggleHUDeditmode();

        osg::ref_ptr<osg::Camera> camera;

        void CreateHUD( int scrH, int scrW);
        void DispDebug(const tSituation *s, const SDFrameInfo* frame);
        void Refresh(tSituation *s, const SDFrameInfo* frameInfo, const tCarElt *currCar,
                     int SimClouds, int SimRain, float SimTimeOfDay);

        osg::ref_ptr <osg::Group> generateHudFromXmlFile( int scrH, int scrW);

        inline osg::ref_ptr<osg::Camera> getRootCamera()
        {
            return this->camera;
        }
};

} // namespace osggraph

#endif //_OSGHUD_H_
