/***************************************************************************

    file        : OsgHUD.cpp
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

#include <osg/Material>
#include <osg/Geode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Camera>
#include <osg/RenderInfo>
#include <osg/Projection>

#include <osgDB/WriteFile>

#include <osgText/Text>

#include <osg/TextureRectangle>
#include <osgDB/ReadFile>
#include <osg/TexMat>

#include <osg/BlendFunc>

#include "OsgHUD.h"
#include "OsgNodeMask.h"
#include "OsgMain.h"
#include "guimenu.h"
#include "translation.h"
#include "tgfclient.h"

#include "forcefeedback.h"
extern TGFCLIENT_API ForceFeedbackManager forceFeedback;

#include <sstream>
#include <iomanip> //setprecision

#include <algorithm> //tolower
#include <string> //tolower
#include <cctype> //tolower

namespace osggraph {

//string constants for dashboard
static std::string strEmpty;
static std::string strBrakeRep("F/R Brake Rep.");
static std::string strFrontARB("Front ARB");
static std::string strRearARB("Rear ARB");
static std::string strFDiffMSB("F Pow Max Slip");
static std::string strFDiffCMSB("F Coa Max Slip");
static std::string strRDiffMSB("R Pow Max Slip");
static std::string strRDiffCMSB("R Coa Max Slip");
static std::string strCDiffMSB("C Pow Max Slip");
static std::string strCDiffCMSB("C Coa Max Slip");
static std::string strFuel("Fuel");
static std::string strRepair("Repair");
static std::string strTireSet("New tires");
static std::string strFrontWing("Front wing");
static std::string strRearWing("Rear wing");
static std::string strPenalty("Next pit type");
static std::string strTireCompounds("Tyre compound");

static const osg::Vec4 colorRed(1.0, 0.0, 0.0, 1.0);
static const osg::Vec4 colorYellow(1.0, 0.878, 0.0, 1.0);
static const osg::Vec4 colorCyan(0.31, 0.968, 0.933, 1.0);

//mouse
int prevMouseButtonState = 0;
int mouseStartDragX = 0;
int mouseStartDragY = 0;
int mouseTotalDragX = 0;
int mouseTotalDragY= 0;
int mouseDragX = 0;
int mouseDragY = 0;
int mousePrevPosX = 0;
int mousePrevPosY = 0;
int hudScreenH = 0;
int hudScreenW = 0;

//edithud
bool hudEditModeEnabled = false;

extern tdble SimTimeOfDay;
extern int SimClouds;
extern int SimRain;

std::vector<float> horsepowerPoints;

// TODO[START]: move this to utils? /src/modules/graphic/osggraph/Utils
static void split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;

    while (getline(ss, item, delim))
    {
        elems.push_back(item);
    }
}

double getCathetusTwoLength(double cateto1, double angoloVertice1) {
    double angoloVertice2 = 45 - angoloVertice1;
    double ipotenusa = cateto1 / cos(angoloVertice1 * M_PI / 180);
    double cateto2 = ipotenusa * tan(angoloVertice2 * M_PI / 180);
    return cateto2;
}

static std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

static osg::Vec4 colorStringToVec4(const std::string & colorString)
{
    std::vector<std::string> colorStringVector = split(colorString, '#');

    return osg::Vec4(
        std::atof(colorStringVector[0].c_str()),
        std::atof(colorStringVector[1].c_str()),
        std::atof(colorStringVector[2].c_str()),
        std::atof(colorStringVector[3].c_str())
        );
}

OSGPLOT::OSGPLOT(
    float positionX,
    float positionY,
    float width,
    float height,
    const std::string &title,
    std::vector<PlotLineConfig> lines) :
    positionX(positionX),
    positionY(positionY),
    width(width),
    height(height),
    title(title)
{
    osgGroup = new osg::Group;

    for (size_t i = 0; i < lines.size(); i++)
    {
        plotLines.push_back(lines[i]);

        osg::Geode *geode = new osg::Geode;

        // add the points geometry to the geode.
        geode->addDrawable(plotLines.back().geometry);
        osgGroup->addChild(geode);
    }

    osgText::Text* osgTitle = new osgText::Text;

    //draw the background of the chart
    drawBackground();

    //prepare the geode for the title
    osg::Geode* geode = new osg::Geode;

    //color
    osg::Vec4 color = osg::Vec4(0.0f,0.0f,0.0f,0.9f);
    osgTitle->setColor(color);

    std::string fontFileUrl = "/vera/Vera.ttf";
    std::string fontsMainDirectory = GetDataDir();
    fontsMainDirectory = fontsMainDirectory+"data/fonts";
    fontFileUrl = fontsMainDirectory+fontFileUrl;
    osgTitle->setFont(fontFileUrl);

    //font resolution
    osgTitle->setFontResolution(200,200);

    //set the font size
    osgTitle->setCharacterSize(20);
    osgTitle->setAlignment(osgText::Text::LEFT_BOTTOM_BASE_LINE );

    //asign the position
    osgTitle->setPosition(osg::Vec3(positionX + 5.0f, positionY + height - 25.0f, 0.05f));

    osgTitle->setText(title);
    osgTitle->setNodeMask(1);

    geode->addDrawable(osgTitle);
    osgGroup->addChild(geode);
}

OSGPLOT::~OSGPLOT()
{
}

osg::ref_ptr <osg::Group> OSGPLOT::getGroup()
{
    return (*osgGroup).asGroup();
}

void OSGPLOT::update(tSituation *s, const SDFrameInfo* frameInfo, const tCarElt *currCar)
{
    float currentTime = GfTimeClock();

    for (std::list<PlotLine>::iterator it = plotLines.begin(); it != plotLines.end(); ++it)
    {
        if (!it->reference)
        {
            //get x value
            float x = 0;
            if (it->Xdata == "time") x = currentTime;

            //get y value
            float y = 0;
            if (it->Ydata == "fps")                    y = (float)frameInfo->fInstFps;
            else if (it->Ydata == "carspeed")          y = (float)currCar->_speed_x * 3.6;
            else if (it->Ydata == "fpsavverrange")     y = (float)frameInfo->fAvgFps;
            else if (it->Ydata == "carbracketemp")     y = (float)currCar->_brakeTemp(0);
            else if (it->Ydata == "forcefeedback")     y = fabs((float)forceFeedback.force);
            else if (it->Ydata == "brake")             y = (float)currCar->_brakeCmd;
            else if (it->Ydata == "accel")             y = (float)currCar->_accelCmd;
            else if (it->Ydata == "clutch")            y = (float)currCar->_clutchCmd;
            else if (it->Ydata == "steering")          y = (float)currCar->_steerCmd;

            //get z value
            float z = 0.1f;

            //add the new point
            it->appendDataPoint(x, y, z);
        }

        //redraw
        it->recalculateDrawnPoint(currentTime, positionX, positionY, width, height);
    }
}

void OSGPLOT::clearDataPoints()
{
    for (std::list<PlotLine>::iterator it = plotLines.begin(); it != plotLines.end(); ++it)
    {
        if (!it->reference && it->Ydata != "fps")
            it->clearDataPoints();
    }
}

OSGPLOT::PlotLine::PlotLine(const PlotLineConfig &config) :
    PlotLineConfig(config),
    dataPoints(nullptr),
    geometry(nullptr),
    vertices(nullptr)
{
    geometry = new osg::Geometry();
    vertices = new osg::Vec3Array(2);
    dataPoints = new osg::Vec3Array(0);

    // pass the created vertex array to the points geometry object.
    geometry->setVertexArray(vertices);
    geometry->setUseDisplayList(false);
    if (config.reference)
        geometry->setUseVertexBufferObjects(true);

    // set the same color for the reference plot line
    osg::Vec4Array *plotColors = new osg::Vec4Array;
    plotColors->push_back(color);
    geometry->setColorArray(plotColors, osg::Array::BIND_OVERALL);

    // set the normal
    osg::Vec3Array *normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0.0f, -1.0f, 0.0f));
    geometry->setNormalArray(normals, osg::Array::BIND_OVERALL);

    // tell osg to draw our geometry as lines
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size()));

    // disable lighting (light is always on) and enalbe transparency
    osg::StateSet *stateset = geometry->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
}

void OSGPLOT::PlotLine::recalculateDrawnPoint(float currentTime, float positionX, float positionY, float width, float height)
{
    //recalculate main plot line
    if (!reference)
    {
        //find max and min values for our plot
        //just draw point that are in our range of time
        for(osg::Vec3Array::iterator it = dataPoints->begin(); it != dataPoints->end();)
        {
            if ((*it).x() <= (currentTime - timeFrame) || (*it).x() <= 0)
            {
                it = dataPoints->erase(it);
            }
            else
            {
                //find max
                if ((*it).y() > maxValue)
                {
                    maxValue = (float)(*it).y();
                }

                //find min
                if ((*it).y() < minValue)
                {
                    minValue = (float)(*it).y();
                }

                ++it;
            }
        }

        //cicle trounght data point and calculate correct display position
        int counter = 0;
        vertices->resize(dataPoints->size());

        for(osg::Vec3Array::iterator it = dataPoints->begin(); it != dataPoints->end(); ++it, counter++)
        {
            //copy data
            (*vertices)[counter].set(
                ((float)(*it).x() - (currentTime - timeFrame))* (width / timeFrame),
                (float)(*it).y(),
                (float)(*it).z()
            );

            //scale to fit plot area
            (*vertices)[counter].set(
                (*vertices)[counter].x(),
                ((*vertices)[counter].y() - minValue) / (maxValue - minValue) * height,
                (*vertices)[counter].z()
            );

            //move to correct position
            (*vertices)[counter].set(
                (*vertices)[counter].x() + positionX,
                (*vertices)[counter].y() + positionY,
                (*vertices)[counter].z()
            );
        }

        //pass the new vertices to the geometry
        geometry->setVertexArray(vertices);

        //update the drawing instructions (for the different number of vertices)
        geometry->removePrimitiveSet(0,1);
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size()));
    }

    //recalculate reference plot line
    else
    {
        // note, anticlockwise ordering.
        osg::Vec3 myCoords[] =
        {
            osg::Vec3(
                positionX,
                ((referenceLineAtValue - minValue) / (maxValue - minValue) * height) + positionY,
                0.1f
            ),
            osg::Vec3(
                positionX + width,
                ((referenceLineAtValue - minValue) / (maxValue - minValue) * height) + positionY,
                0.1f
            ),
        };

        int numCoords = sizeof(myCoords)/sizeof(osg::Vec3);

        vertices = new osg::Vec3Array(numCoords,myCoords);

        //tell osg that our vertices has changed
        vertices->dirty();
        geometry->setVertexArray(vertices);
    }
}

void OSGPLOT::drawBackground()
{
    // create Geometry object
    osg::Geode* geode = new osg::Geode;
    osg::Geometry* bgGeometry = new osg::Geometry();

    // create the vertices of the geometry
    /*
     * the numbers rappresent the order of the vertices iside the vec array (took me some time to figure out the order was anticlock wise)
     * x and y are the axes
     * assuming this square is the screen
     *
     *      4_______3
     *      |       |
     *    y |       |
     *      |       |
     *      1_______2
     *          x
    * */
    osg::Vec3 myCoords[] =
    {
           osg::Vec3(positionX, positionY, 0.0f),
           osg::Vec3(positionX + width, positionY, 0.0f),
           osg::Vec3(positionX + width, positionY + height, 0.0f),
           osg::Vec3(positionX, positionY+height, 0.0f),
    };

        int numCoords = sizeof(myCoords)/sizeof(osg::Vec3);
       osg::Vec3Array* vertices = new osg::Vec3Array(numCoords,myCoords);

       // pass the created vertex array to the points geometry object.
       bgGeometry->setVertexArray(vertices);

       // apply the same color to the whole geometry
       osg::Vec4Array* colors = new osg::Vec4Array;
       colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,0.5f));
       bgGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);

       // setup normals
       osg::Vec3Array* normals = new osg::Vec3Array;
       normals->push_back(osg::Vec3(0.0f,-1.0f,0.0f));
       bgGeometry->setNormalArray(normals, osg::Array::BIND_OVERALL);

       // tell osg to draw this geometry as quads
       bgGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,numCoords));

       // disable lighting (light is always on) and enable transparency
       osg::StateSet* stateset = bgGeometry->getOrCreateStateSet();
       stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
       stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
       stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

       // add the points geometry to the geode.
       geode->addDrawable(bgGeometry);

      // add the geode to the graph group
      osgGroup->addChild(geode);
}

osg::Vec3 SDHUD::calculatePosition(osg::BoundingBox mybb, const std::string &objPoint,
                                   osg::BoundingBox bb,  const std::string &referenceObjPoint,
                                   float verticalModifier, float horizontalModifier)
{
    /*
    Possible positioning values:
    tl  //top left
    tr  //top right
    tc  //top center
    bl  //bottom left
    br  //bottom right
    bc  //bottom center
    ml  //middle left
    mr  //middle right
    mc  //middle center
    */

    float vPoint = 0;
    float hPoint = 0;
    float vSign = 0;

    //my starting point
    osg::Vec3 position = osg::Vec3(0.0f,0.0f,0.0f);

    //ref object
    //vertical
    if (referenceObjPoint.find('t') == 0)
    {
        vPoint += bb.yMax();
        vSign = 1;
    }
    else if (referenceObjPoint.find('b') == 0)
    {
        vPoint += bb.yMin();
        vSign = -1;
    }
    else if (referenceObjPoint.find('m') == 0)
    {
        vPoint += (bb.yMax() - bb.yMin()) / 2;
        vSign = 1;
    }

    //horizontal
    if (referenceObjPoint.find('l') == 1)
    {
        hPoint += bb.xMin();
    }
    else if (referenceObjPoint.find('r') == 1)
    {
        hPoint += bb.xMax();
    }
    else if (referenceObjPoint.find('c') == 1)
    {
        hPoint += (bb.xMax() - bb.xMin()) / 2;
    }

    //my obj /*todo check medium vertical alignment*/
    //vertical
    if (objPoint.find('t') == 0)
    {
        vPoint -= (mybb.yMax() - mybb.yMin()) * vSign;//height
    }
    else if(objPoint.find('b') == 0)
    {
        //do nothing
    }
    else if (objPoint.find('m') == 0)
    {
        vPoint -= (mybb.yMax() - mybb.yMin()) * vSign  /2;
    }

    //horizontal
    if (objPoint.find('l') == 1)
    {
        //nothing to do
    }
    else if (objPoint.find('r') == 1)
    {
        hPoint -= (mybb.xMax() - mybb.xMin());//width
    }
    else if(objPoint.find('c') == 1)
    {
        hPoint -= (mybb.xMax() - mybb.xMin()) / 2;
    }

    //modifier
    hPoint += horizontalModifier;
    vPoint += verticalModifier;

    // apply the modifiers
    position += osg::Vec3(hPoint,vPoint,0.0f);

    return position;
}

std::string SDHUD::formatLaptime(tdble sec, int sgn)
{
    std::ostringstream lapTimeString;

    if(sec < 0)
    {
        lapTimeString << "-";
    }
    else
    {
        lapTimeString << "+";
    }

    sec = abs(sec);

    const int m = (int)(sec / 60.0);
    sec -= 60 * m;
    const int s = (int)(sec);
    sec -= s;
    const int ms = (int)floor(sec * 1000.0);

    //minutes
    if( m < 10)
    {
        lapTimeString << "0";
    }

    lapTimeString << m;

    lapTimeString << ":";

    //seconds
    if( s < 10)
    {
        lapTimeString << "0";
    }

    lapTimeString << s;

    lapTimeString << ".";

    //decimals
    if( ms < 100)
    {
        lapTimeString << "0";
    }

    if( ms < 10)
    {
        lapTimeString << "0";
    }

    lapTimeString << ms;

    return lapTimeString.str();
}

void SDHUD::changeImageSize(osg::Geometry *geom,
                            float newSize/*where 1.0 full image size (width or height) and 0.0 no size (width or height)*/,
                            const std::string &resizeFrom/*left|right|top|bottom == this is the place that will be fixed(not modified), the other one ill be moved to fit the new size*/,
                            float hudScale)
{
    osg::TextureRectangle* texture;

    //get the texture data of this object
    texture = dynamic_cast<osg::TextureRectangle*>(geom->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXTURE));

    //get the image from the texture data
    osg::Image* img;
    img = texture->getImage();

    //get image dimensions
    float imgWidth = img->s() * hudScale;
    float imgHeight = img->t() * hudScale;

    //adapt the geometry
    {
        //osg::Vec3Array* vertices = new osg::Vec3Array;
        osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());

        /*
         * how vertices are arranged:
         *      3_______2
         *      |       |
         *    y |       |
         *      |       |
         *      0_______1
         *          x
         *
         * [vertices(0-3)][0]=x
         * [vertices(0-3)][1]=y
        * */

        //change the width
        if(resizeFrom =="left")
        {
            //change right vertices
            (*vertices)[1][0] = (*vertices)[0][0]+(imgWidth * newSize);
            (*vertices)[2][0] = (*vertices)[0][0]+(imgWidth * newSize);
        }
        else if(resizeFrom =="right")
        {
            //change left vertices
            (*vertices)[0][0] = (*vertices)[1][0]-imgWidth+(imgWidth * (1.0f-newSize));
            (*vertices)[3][0] = (*vertices)[1][0]-imgWidth+(imgWidth * (1.0f-newSize));
        }
        else if(resizeFrom =="top")
        {
            //change bottom vertices
            (*vertices)[0][1] = (*vertices)[2][1]-imgHeight+(imgHeight * (1.0f-newSize));
            (*vertices)[1][1] = (*vertices)[2][1]-imgHeight+(imgHeight * (1.0f-newSize));
        }
        else if(resizeFrom =="bottom")
        {
            //change top vertices
            (*vertices)[2][1] = (*vertices)[0][1]+(imgHeight * newSize);
            (*vertices)[3][1] = (*vertices)[0][1]+(imgHeight * newSize);
        }

        vertices->dirty();

        geom->setVertexArray(vertices);
    }

    //adapt the texture
    {

        osg::Vec2Array* texcoords = dynamic_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));

        if(resizeFrom =="left")
        {
            (*texcoords)[1][0]= newSize;
            (*texcoords)[2][0]= newSize;
        }
        else if(resizeFrom =="right")
        {
            (*texcoords)[0][0] = 1.0f - newSize;
            (*texcoords)[3][0] = 1.0f - newSize;
        }
        else if(resizeFrom =="top")
        {
            (*texcoords)[0][1] = 1.0f - newSize;
            (*texcoords)[1][1] = 1.0f - newSize;
        }
        else if(resizeFrom =="bottom")
        {
            (*texcoords)[2][1] = newSize;
            (*texcoords)[3][1] = newSize;
        }

        geom->setTexCoordArray(0,texcoords);
    }
}

void SDHUD::changeImageVertex(osg::Geometry *geom,
                            float fromAngle,
                            float toAngle,
                            float currValue,
                            float newPosX,
                            float newPosY,
                            float hudScale)

{
    osg::TextureRectangle* texture;

    //get the texture data of this object
    texture = dynamic_cast<osg::TextureRectangle*>(geom->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXTURE));

    //get the image from the texture data
    osg::Image* img;
    img = texture->getImage();

    //get image dimensions
    float width = img->s();
    float height = img->t();

    //rerieve to start modifing vertex and UVs
    osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    osg::Vec2Array* texcoords = dynamic_cast<osg::Vec2Array*>(geom->getTexCoordArray(0));

    //determine the start and end triangle
    int startTriangle = ceil((fromAngle+0.0001)/45.0)-1;
    int endTriangle = ceil((toAngle)/45.0)-1;

    if(startTriangle < 0){
        startTriangle = startTriangle + 8;
    }

    //determine the range of angle we will be using to cover 100% of the value
    float angleRange = 0.0f;
    if(fromAngle > toAngle){
        angleRange = (360-fromAngle)+toAngle;
    }else{
        angleRange = toAngle-fromAngle;
    }

    //what angle we need to add to the start angle to reach the current value angle
    float currValueAngle = angleRange*currValue;

    //determine the angle we can use for the first triangle (in case the start is not at his 0 angle)
    float startTriangleStartAngle = std::fmod( fromAngle, 45.0f );
    float startTriangleAngleCapacity = 45.0f - std::fmod( fromAngle, 45.0f );

    //determine the angle we can use for final triangle (in case the start is not at his 0 angle)
    //determine how many triangles we will need to process
    int trianglesNeeded = ceil((currValueAngle - startTriangleAngleCapacity) / 45)+1;

    //determine what will be the last triangle given the current value
    int currValueLastTriangle = ((startTriangle + trianglesNeeded) % 8 ) - 1;
    if(currValueLastTriangle < 0){
        currValueLastTriangle = currValueLastTriangle + 8;
    }

    //determine a modifier for the triangle index that we will use in the next for cicle, so that we will start processing from the start to the final triangle
    //despite the starting triangle being greater than the final triangle
    int indexModifier = 0;
    if(startTriangle <= endTriangle){
        indexModifier = 0;
    }else{
        indexModifier = startTriangle;
    }

    float angleRangeToBeUsed = currValueAngle;

    for (int triangle = 0; triangle <= 7; triangle++) {
        int triangleIdx = triangle + indexModifier;
        if(triangleIdx > 7){
            triangleIdx = triangleIdx - 8;
        }
        float newAngle=0.0;

        //se è il primo triangolo e ci sta tutto il valore uso il valore necessario
        if(triangleIdx == startTriangle){
            if(angleRangeToBeUsed <= startTriangleAngleCapacity){
                newAngle = angleRangeToBeUsed;
                angleRangeToBeUsed = angleRangeToBeUsed - newAngle;
            }else{
                newAngle = startTriangleAngleCapacity;
                angleRangeToBeUsed = angleRangeToBeUsed - newAngle;
            }
            //if it's the first triangle the initial start angle may not be the local 0 so
            //we need to add to the startingPointAngle to the desidered value
            //we will fix this when calculating vertex 2
        }

        //vale per tutti gli altri angoli (se è zero resta zero, se è maggiore di 45 uso 45)
        if( ((startTriangle <= endTriangle ) && (triangleIdx > startTriangle))
            || ((startTriangle > endTriangle )  && (triangleIdx > startTriangle || triangleIdx <= endTriangle)) )
        {
            if(angleRangeToBeUsed <= 45.0)
            {
                newAngle = angleRangeToBeUsed;
                angleRangeToBeUsed = angleRangeToBeUsed - newAngle;
            }
            else
            {
                newAngle = 45.0;
                angleRangeToBeUsed = angleRangeToBeUsed - newAngle;
            }
        }

        /*
        GfLogDebug("TriangleIndex %i  (%i)", triangleIdx, triangle);
        GfLogDebug("   CurrValueAngle %f\n", currValueAngle);
        GfLogDebug("   Angoli richiesti %i \n",trianglesNeeded);
        GfLogDebug("   Triangle index (Start/End) %i ## %i\n", startTriangle, currValueLastTriangle);
        GfLogDebug("   Angle to be used %f \n",angleRangeToBeUsed);
        GfLogDebug("   My angle %f \n",newAngle );
        GfLogDebug("   Start Triangle start angle %f \n",startTriangleStartAngle );
        */

        for (int vertex = 0;vertex <= 2; vertex++ )
        {
            int vertexIndex= (triangleIdx*3)+vertex;
            float CathetusTwoLength = 0.0;
            float newX = 0.0;
            float newY = 0.0;

            //GfLogDebug("VertexIndex %i \n", vertexIndex);
            /*----------------------------------------------------------------------------------------------------------------------------*/
            //for all vertex start by restoring their original status

            if (vertex == 0){
                newX = 0.5f;
                newY = 0.5f;
            }
            if (vertex == 1){
                switch (triangleIdx) {
                    case 0:
                        newX = 0.0;
                        newY = 0.5;
                        break;
                    case 2:
                        newX = 0.5;
                        newY = 1.0;
                        break;
                    case 4:
                        newX = 1.0;
                        newY = 0.5;
                        break;
                    case 6:
                        newX = 0.5;
                        newY = 0.0;
                        break;
                    //------------------------------------------------------------------------------
                    case 1:
                        newX = 0.0;
                        newY = 1.0;
                        break;
                    case 3:
                        newX = 1.0;
                        newY = 1.0;
                        break;
                    case 5:
                        newX = 1.0;
                        newY = 0.0;
                        break;
                    case 7:
                        newX = 0.0;
                        newY = 0.0;
                        break;
                    default:
                        break;
                }
            }

            if (vertex == 2){
                switch (triangleIdx) {
                    case 0:
                        newX = 0.0;
                        newY = 1.0;
                        break;
                    case 2:
                        newX = 1.0;
                        newY = 1.0;
                        break;
                    case 4:
                        newX = 1.0;
                        newY = 0.0;
                        break;
                    case 6:
                        newX = 0.0;
                        newY = 0.0;
                        break;
                     //-------------------------------------------------------------
                    case 1:
                        newX = 0.5;
                        newY = 1.0;
                        break;
                    case 3:
                        newX = 1.0;
                        newY = 0.5;
                        break;
                    case 5:
                        newX = 0.5;
                        newY = 0.0;
                        break;
                    case 7:
                        newX = 0.0;
                        newY = 0.5;
                        break;
                    default:
                        break;
                }
            }
            (*vertices)[vertexIndex][0] = newPosX + newX * width * hudScale; //imgWidth; //x
            (*vertices)[vertexIndex][1] = newPosY + newY * height * hudScale; //imgHeight; //y
            //update uvs
            (*texcoords)[vertexIndex][0] = newX;
            (*texcoords)[vertexIndex][1] = newY;
            /*----------------------------------------------------------------------------------------------------------------------------*/

            //now adjust what is needed:
            //for start triangle we need to update the second vertice
            //for end triangle we need to update the third vertice
            //for hidden triangles we just hide everything

            //start triangle
            if (triangleIdx == startTriangle && vertex ==1){
                switch (triangleIdx) {
                    case 0:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = 0.0;
                        newY = 1.0 - CathetusTwoLength;
                        break;
                    case 2:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = 1.0-CathetusTwoLength;
                        newY = 1.0;
                        break;
                    case 4:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = 1.0;
                        newY = CathetusTwoLength;
                        break;
                    case 6:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = CathetusTwoLength;
                        newY = 0.0;
                        break;
                    //------------------------------------------------------------------------------
                    case 1:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = 0.5-CathetusTwoLength;
                        newY = 1.0;
                        break;
                    case 3:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = 1.0;
                        newY = 0.5+CathetusTwoLength;
                        break;
                    case 5:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = 0.5+CathetusTwoLength;
                        newY = 0.0;
                        break;
                    case 7:
                        CathetusTwoLength = getCathetusTwoLength(0.5, startTriangleStartAngle);
                        newX = 0.0;
                        newY = 0.5-CathetusTwoLength;
                        break;
                    default:
                        break;
                }
                //update vertices
                (*vertices)[vertexIndex][0] = newPosX + newX * width * hudScale; //imgWidth; //x
                (*vertices)[vertexIndex][1] = newPosY + newY * height * hudScale; //imgHeight; //y
                //update uvs
                (*texcoords)[vertexIndex][0]= newX;
                (*texcoords)[vertexIndex][1]= newY;
            }

            //end triangle
            if (triangleIdx == currValueLastTriangle && vertex == 2){
                //we need to fix a special case in case the first triangle is also the last triangle
                //in this case we want the startingAngle to be added to the desiredValueAngle
                if (triangleIdx == startTriangle){
                    newAngle = newAngle + startTriangleStartAngle;
                }

                switch (triangleIdx) {
                    case 0:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = 0.0;
                        newY = 0.5+CathetusTwoLength;
                        break;
                    case 2:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = 0.5+CathetusTwoLength;
                        newY = 1.0;
                        break;
                    case 4:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = 1.0;
                        newY = 0.5-CathetusTwoLength;
                        break;
                    case 6:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = 0.5-CathetusTwoLength;
                        newY = 0.0;
                        break;
                     //-------------------------------------------------------------
                    case 1:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = CathetusTwoLength;
                        newY = 1.0;
                        break;
                    case 3:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = 1.0;
                        newY = 1.0-CathetusTwoLength;
                        break;
                    case 5:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = 1.0-CathetusTwoLength;
                        newY = 0.0;
                        break;
                    case 7:
                        CathetusTwoLength = getCathetusTwoLength(0.5, 45.0-newAngle);
                        newX = 0.0;
                        newY = CathetusTwoLength;
                        break;
                    default:
                        break;
                }
                (*vertices)[vertexIndex][0] = newPosX + newX * width * hudScale; //imgWidth; //x
                (*vertices)[vertexIndex][1] = newPosY + newY * height * hudScale; //imgHeight; //y
                //update uvs
                (*texcoords)[vertexIndex][0]= newX;
                (*texcoords)[vertexIndex][1]= newY;
            }

            //if hidden triangle
            if( newAngle <= 0.0 ){
                newX = 0.0f;
                newY = 0.0f;
                (*vertices)[vertexIndex][0] = newX * width * hudScale; //imgWidth; //x
                (*vertices)[vertexIndex][1] = newY * height * hudScale; //imgHeight; //y
                //update uvs
                (*texcoords)[vertexIndex][0]= newX;
                (*texcoords)[vertexIndex][1]= newY;
            }
        }
    }
    //adapt the geometry
    vertices->dirty();
    geom->setVertexArray(vertices);
    //adapt the texture
    geom->setTexCoordArray(0,texcoords);
}


void SDHUD::changeImagePosition(osg::Geometry *geom,
                                float newX,
                                float newY,
                                float hudScale)
{
    osg::TextureRectangle* texture;

    //get the texture data of this object
    texture = dynamic_cast<osg::TextureRectangle*>(geom->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXTURE));

    //get the image from the texture data
    osg::Image* img;
    img = texture->getImage();

    //get image dimensions
    float imgWidth = img->s() * hudScale;
    float imgHeight = img->t() * hudScale;

    //adapt the geometry
    {
        osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());

        //change the position
        (*vertices)[0][0] = newX;
        (*vertices)[0][1] = newY;

        (*vertices)[1][0] = newX+imgWidth;
        (*vertices)[1][1] = newY;

        (*vertices)[2][0] = newX+imgWidth;
        (*vertices)[2][1] = newY+imgHeight;

        (*vertices)[3][0] = newX;
        (*vertices)[3][1] = newY+imgHeight;

        vertices->dirty();

        geom->setVertexArray(vertices);
    }
}

void SDHUD::changeImageAlpha(osg::Geometry *geom,
                            float newAlpha) /*where 1.0 fully visible and 0.0 completely hidden*/
{
    // assign colors
    osg::Vec4Array* colors = new osg::Vec4Array(1);
    (*colors)[0].set(1.0f,1.0f,1.0f,newAlpha);
    geom->setColorArray(colors);
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);

}

void SDHUD::changeImageTexture(osg::Geometry *geom,
                            const std::string &newTextureUrl)
{
    osg::TextureRectangle* texture;

    //get the texture data of this object
    texture = dynamic_cast<osg::TextureRectangle*>(geom->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXTURE));

    // setup texture
    osg::Image* myImg = osgDB::readImageFile(newTextureUrl);
    texture->setImage(myImg);
}

// TODO[END]: move this to utils? /src/modules/graphic/osggraph/Utils

SDHUD::SDHUD() :
    _car(nullptr),
    lastCar(nullptr),
    hudScale(1.0f),
    prevSteerAngle(0.0f),
    hudElementsVisibilityStatusEnabled(true)
{
}

void SDHUD::CreateHUD(int scrH, int scrW)
{
    hudScreenH = scrH;
    hudScreenW = scrW;
    // create a camera to set up the projection and model view matrices, and the subgraph to draw in the HUD
    camera = new osg::Camera;

    // set the projection matrix
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0,scrW,0,scrH));

    // set the view matrix
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());

    // only clear the depth buffer
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    // draw subgraph after main camera view.
    camera->setRenderOrder(osg::Camera::POST_RENDER);

    // we don't want the camera to grab event focus from the viewers main camera(s).
    camera->setAllowEventFocus(false);

    //calculate optimal hud scale (choose the minor scale from the vertical and horizontal scale)
    float scaleH = (float)scrH/1024;
    float scaleW = (float)scrW/1280;

    if(scaleH < scaleW){
        hudScale = scaleH;
    }
    else
    {
        hudScale = scaleW;
    }

    GfLogDebug("OSGHUD: Hud Scale is: %f\n", hudScale);

    GfLogDebug("OSGHUD: Generating the HUD from the xml file.\n");

    //generate the hud from the relative xml file
    camera->addChild(generateHudFromXmlFile(scrH, scrW));

}

void SDHUD::DispDebug(const tSituation *s, const SDFrameInfo* frame)
{
}  // grDispDebug

std::string &SDHUD::GetCarName(const tCarElt *currCar)
{
    CarData &d = carData[currCar];
    std::string &s = d.name;

    if (s.empty())
    {
        std::string p = "cars/models/";

        p += currCar->_carName;
        p += "/";
        p += currCar->_carName;
        p += PARAMEXT;

        const char *path = p.c_str();
        void *h = GfParmReadFileBoth(path, GFPARM_RMODE_STD);

        if (!h)
        {
            GfLogError("GfParmReadFileBoth %s failed\n", path);
            return s;
        }

        s = GfParmGetName(h);
        GfParmReleaseHandle(h);
    }

    return s;
}

void SDHUD::Refresh(tSituation *s, const SDFrameInfo* frameInfo,
                        const tCarElt *currCar, int SimClouds, int SimRain, float SimTimeOfDay)
{
    typedef std::map<std::string,OSGPLOT* >::iterator it_type;
    bool carHasChanged = true;

    CarData &data = carData[currCar];

    // reset timers if car changes
    if (currCar != lastCar)
    {
        data.oldSector = currCar->_currentSector;
        data.oldLapNumber = currCar->_laps;
        data.laptimeFreezeTime = 0.0;
        data.timeDiffFreezeTime = 0.0;

        for(it_type iterator = hudGraphElements.begin(); iterator != hudGraphElements.end(); ++iterator)
            iterator->second->clearDataPoints();

        lastCar = currCar;
        carHasChanged = true;
    }else{
        carHasChanged = false;
    }

    //update all the graphs

    for(it_type iterator = hudGraphElements.begin(); iterator != hudGraphElements.end(); ++iterator)
    {
           //iterator->first = key
           //iterator->second = value
           iterator->second->update(s,frameInfo,currCar);
    }

//board
    tCarElt *firstAheadCar;
    tCarElt *secondAheadCar;
    tCarElt *thirdAheadCar;
    tCarElt *firstBehindCar;
    tCarElt *secondBehindCar;
    tCarElt *thirdBehindCar;

    std::vector<tCarElt *> boardCars;

    // get pointers for previous and behind cars from us
    if (currCar->_pos > 3)
    {
        thirdAheadCar = s->cars[currCar->_pos - 4];
        boardCars.push_back(thirdAheadCar);
    }
    if (currCar->_pos > 2)
    {
        secondAheadCar = s->cars[currCar->_pos - 3];
        boardCars.push_back(secondAheadCar);
    }
    if (currCar->_pos > 1) {
        firstAheadCar = s->cars[currCar->_pos - 2];
        boardCars.push_back(firstAheadCar);
    }
    //always add our car
    boardCars.push_back(s->cars[currCar->_pos-1]);

    if (currCar->_pos < s->_ncars )
    {
        firstBehindCar = s->cars[currCar->_pos + 0];
        boardCars.push_back(firstBehindCar);
    }

    if (currCar->_pos < (s->_ncars-1) )
    {
        secondBehindCar = s->cars[currCar->_pos + 1];
        boardCars.push_back(secondBehindCar);
    }

    if (currCar->_pos < (s->_ncars - 2))
    {
        thirdBehindCar = s->cars[currCar->_pos + 2];
        boardCars.push_back(thirdBehindCar);
    }

    //hide all board slots... we will enable later what we will use
    for (int id = 1; id <= 7; id++)
    {
        std::ostringstream mapKey;
        //hide board number
        mapKey.str("");
        mapKey << "board-player" << id << "-background";
        hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);
        mapKey.str("");
        mapKey << "board-player" << id << "-background-first";
        hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);
        mapKey.str("");
        mapKey << "board-player" << id << "-background-current";
        hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);

        mapKey.str("");
        mapKey << "board-player" << id << "-number";
        hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);

        mapKey.str("");
        mapKey << "board-player" << id << "-name";
        hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);

        mapKey.str("");
        mapKey << "board-player" << id << "-timediff";
        hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);
    }

    int id = 0;
    for(std::vector<tCarElt *>::iterator car = boardCars.begin(); car != boardCars.end(); ++car) {

        std::ostringstream mapKey;
        id++;

        mapKey.str("");
        mapKey << "board-player" << id << "-background";
        hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_ALL);

        //select special background for current and/or first player
        if ((*car) == currCar)
        {
            hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);
            mapKey << "-current";
            hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_ALL);
        }
        else if ((*car)->_pos == 1)
        {
            hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);
            mapKey << "-first";
            hudImgElements[mapKey.str()]->setNodeMask(NODE_MASK_ALL);
        }

        std::ostringstream position;
        position.str("");
        position << (*car)->_pos;
        mapKey.str("");
        mapKey << "board-player" << id << "-number";
        hudTextElements[mapKey.str()]->setText(position.str());
        hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_ALL);

        //update board names texts
        mapKey.str("");
        mapKey << "board-player" << id << "-name";
        hudTextElements[mapKey.str()]->setText((*car)->_name);
        hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_ALL);

        //update time diff texts
        mapKey.str("");
        mapKey << "board-player" << id << "-timediff";

        if( data.oldLapNumber != currCar->_laps)
        {
            data.timeDiffFreezeTime = GfTimeClock();
        }

        if ( GfTimeClock() < (data.timeDiffFreezeTime + data.timeDiffFreezeCountdown))
        {
            data.oldLapNumber = currCar->_laps;

            std::ostringstream tempStr;
            tempStr.str("");
            if((*car)->_laps == currCar->_laps)
            {
                tempStr << formatLaptime(((*car)->_curTime - currCar->_curTime),1);
            }
            else if ((*car)->_laps > currCar->_laps)
            {
                tempStr << ((*car)->_laps - currCar->_laps);
                tempStr << " laps";
            }

            hudTextElements[mapKey.str()]->setText(tempStr.str());
            hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_ALL);
            //hide time diff for our car
            if ((*car) == currCar)
            {
                hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);
            }
        }
        else
        {
            hudTextElements[mapKey.str()]->setNodeMask(NODE_MASK_NONE);
        }
    }

//position box
    std::ostringstream temp;
    temp.str("");
    temp << s->_ncars;
    hudTextElements["position-container-totalplayers"]->setText(temp.str());
    temp.str("");
    temp << currCar->_pos;
    hudTextElements["position-container-currentposition"]->setText(temp.str());


//laps box
    temp.str("");
    temp << s->_totLaps;
    hudTextElements["lap-container-totallaps"]->setText(temp.str());
    temp.str("");
    temp << currCar->_laps;
    hudTextElements["lap-container-currentlap"]->setText(temp.str());


//laptime
    if (data.oldSector != currCar->_currentSector)
    {
        data.laptimeFreezeTime = GfTimeClock();
        data.oldSector = currCar->_currentSector;

        if (currCar->_currentSector == 0)
        {
            hudTextElements["laptime-last-time"]->setText(formatLaptime(currCar->_lastLapTime,0));

            if (currCar->_lastLapTime == currCar->_bestLapTime)
            {
                hudImgElements["laptime-last-background-green"]->setNodeMask(NODE_MASK_ALL);
                hudImgElements["laptime-last-background-normal"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-grey"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-violet"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-red"]->setNodeMask(NODE_MASK_NONE);
            }
            else
            {
                hudImgElements["laptime-last-background-normal"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-grey"]->setNodeMask(NODE_MASK_ALL);
                hudImgElements["laptime-last-background-violet"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-green"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-red"]->setNodeMask(NODE_MASK_NONE);
            }
        }
        else
        {
            float currentPrevSectorSplitTime = currCar->_curSplitTime[currCar->_currentSector - 1]; // our time in the sector we have "just" run over
            float bestPrevSectorSplitTime = currCar->_bestSplitTime[currCar->_currentSector-1]; // the best split time of the sector we are in this moment

            if (currentPrevSectorSplitTime < bestPrevSectorSplitTime)
            {
                hudImgElements["laptime-last-background-normal"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-grey"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-violet"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-green"]->setNodeMask(NODE_MASK_ALL);
                hudImgElements["laptime-last-background-red"]->setNodeMask(NODE_MASK_NONE);
            }
            else
            {
                hudImgElements["laptime-last-background-normal"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-grey"]->setNodeMask(NODE_MASK_ALL);
                hudImgElements["laptime-last-background-violet"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-green"]->setNodeMask(NODE_MASK_NONE);
                hudImgElements["laptime-last-background-red"]->setNodeMask(NODE_MASK_NONE);
            }
        }
    }

    if (GfTimeClock() > (data.laptimeFreezeTime + data.laptimeFreezeCountdown))
    {
        //remember our current sector
        data.oldSector = currCar->_currentSector;

        temp.str("");
        temp << "S" << (data.oldSector+1);
        hudTextElements["laptime-sector-description"]->setText(temp.str());

        hudImgElements["laptime-last-background-normal"]->setNodeMask(NODE_MASK_ALL);
        hudImgElements["laptime-last-background-grey"]->setNodeMask(NODE_MASK_NONE);
        hudImgElements["laptime-last-background-violet"]->setNodeMask(NODE_MASK_NONE);
        hudImgElements["laptime-last-background-green"]->setNodeMask(NODE_MASK_NONE);
        hudImgElements["laptime-last-background-red"]->setNodeMask(NODE_MASK_NONE);

        //show laptime
        hudTextElements["laptime-last-time"]->setText(formatLaptime(currCar->_curLapTime,0));

        //on the last sector show the total lap time
        if (currCar->_currentSector == currCar->_nbSectors-1)
        {
            hudTextElements["laptime-best-time"]->setText(formatLaptime(currCar->_bestLapTime,0));
        }
        else
        {
            tdble bestSplitTime = currCar->_bestSplitTime[currCar->_currentSector]; // the best split time of the sector we are in this moment
            hudTextElements["laptime-best-time"]->setText(formatLaptime(bestSplitTime,0));
        }
    }

//fuel
    temp.str("");
    temp << std::fixed << std::setprecision(1) << currCar->_fuel;
    hudTextElements["fuel-info-quantity-number"]->setText(temp.str());

    temp.str("");

    //when we have done at least one lap calculate remaining fuel
    if (currCar->_laps > data.carLaps && currCar->_laps > 1)
    {
        float fuelConsumpionPerLap = currCar->_fuelTotal / (float)(currCar->_laps-1);
        data.remainingFuelForLaps = currCar->_fuel / fuelConsumpionPerLap;
        data.carLaps = currCar->_laps;
    }

    //if we have fuel for more than one lap display how many
    if (data.remainingFuelForLaps > 0 )
    {
        temp << std::fixed << std::setprecision(0) << data.remainingFuelForLaps;
    }
    else
    {
        temp << "---";
    }

    hudTextElements["fuel-info-laps-number"]->setText(temp.str());

    float carFuel = (float)((float)currCar->_fuel / (float)currCar->_tank);

    //update fuel bar
    changeImageSize(hudImgElements["fuel-icon-full"], carFuel, "bottom", hudScale);


//abs, tcs, speed limiter
    bool abs = false;   // Show ABS indicator?
    bool tcs = false;   // Show TCS indicator?
    bool spd = false;   // Show speed limiter indicator?

    // Parse control messages if they include ABS / TCS / SPD
    for (int i = 0; i < 4; i++)
    {
        abs = abs || strstr(currCar->_msgCmd[i], "ABS");
        tcs = tcs || strstr(currCar->_msgCmd[i], "TCS");
        spd = spd || strstr(currCar->_msgCmd[i], "Speed Limiter On");
    }

    hudImgElements["abs-icon"]->setNodeMask(abs ? NODE_MASK_ALL : NODE_MASK_NONE);
    hudImgElements["tcs-icon"]->setNodeMask(tcs ? NODE_MASK_ALL : NODE_MASK_NONE);

//gear
    // show gear: "N" for neutral, "R" for retro, "gearnumber" for all the others
    temp.str("");
    if(currCar->_gear == 0)
    {
        temp << "N";
    }
    else if (currCar->_gear == -1)
    {
        temp << "R";
    }
    else
    {
        temp << currCar->_gear;
    }

    hudTextElements["gear-number"]->setText(temp.str());

//speed
    temp.str("");
    //get speed in km/h
    temp << (int)(currCar->_speed_x * 3.6);
    hudTextElements["speed-number"]->setText(temp.str());

    //set speed unit
    temp.str("");
    temp << "kph"; //mph
    hudTextElements["speed-number-unit"]->setText(temp.str());

//rpm number
    temp.str("");
    //convert Radian/s to RPM
    temp << (int) RADS2RPM(currCar->_enginerpm);
    hudTextElements["rpm-number"]->setText(temp.str());

//damage
    float carDamage = (float)((currCar->_dammage) / (float)s->_maxDammage);
    changeImageSize(hudImgElements["engine-icon-damaged"], carDamage, "bottom", hudScale);


//rpm
    float rpmWidth = 1.0 / currCar->_enginerpmMax * currCar->_enginerpm;
    changeImageSize(hudImgElements["rpm-on"], rpmWidth, "left", hudScale);

// dash items
    std::string &description = strEmpty;
    if (currCar->_dashboardActiveItem < currCar->_dashboardInstantNb)
    {
        const tDashboardItem *item = &(currCar->_dashboardInstant[currCar->_dashboardActiveItem]);
        temp.str("");
        switch (item->type)
        {
        case DI_BRAKE_REPARTITION:
            description = strBrakeRep;
            temp << std::fixed << std::setprecision(1) << (100.0 * item->setup->value) << " %";
            break;
        case DI_FRONT_ANTIROLLBAR:
            description = strFrontARB;
            temp << std::fixed << std::setprecision(1) << (item->setup->value / 1000.0) << " kN/m";
            break;
        case DI_REAR_ANTIROLLBAR:
            description = strRearARB;
            temp << std::fixed << std::setprecision(1) << (item->setup->value / 1000.0) << " kN/m";
            break;
        case DI_FRONT_DIFF_MAX_SLIP_BIAS:
            description = strFDiffMSB;
            temp << std::fixed << std::setprecision(1) << (100.0 * item->setup->value) << " %";
            break;
        case DI_FRONT_DIFF_COAST_MAX_SLIP_BIAS:
            description = strFDiffCMSB;
            temp << std::fixed << std::setprecision(1) << (100.0 * item->setup->value) << " %";
            break;
        case DI_REAR_DIFF_MAX_SLIP_BIAS:
            description = strRDiffMSB;
            temp << std::fixed << std::setprecision(1) << (100.0 * item->setup->value) << " %";
            break;
        case DI_REAR_DIFF_COAST_MAX_SLIP_BIAS:
            description = strRDiffCMSB;
            temp << std::fixed << std::setprecision(1) << (100.0 * item->setup->value) << " %";
            break;
        case DI_CENTRAL_DIFF_MAX_SLIP_BIAS:
            description = strCDiffMSB;
            temp << std::fixed << std::setprecision(1) << (100.0 * item->setup->value) << " %";
            break;
        case DI_CENTRAL_DIFF_COAST_MAX_SLIP_BIAS:
            description = strCDiffCMSB;
            temp << std::fixed << std::setprecision(1) << (100.0 * item->setup->value) << " %";
            break;
        }
        hudTextElements["dash-items-type"]->setText(description);
        hudTextElements["dash-items-value1"]->setColor(colorRed);
        hudTextElements["dash-items-value1"]->setText(temp.str());
        hudTextElements["dash-items-value2"]->setText(std::string(" "));
    }
    else
    {
        const tDashboardItem *item = &(currCar->_dashboardRequest[currCar->_dashboardActiveItem - currCar->_dashboardInstantNb]);
        std::ostringstream value1;
        std::ostringstream value2;
        value1 << " ";
        value2 << " ";
        switch (item->type)
        {
        case DI_FUEL:
            description = strFuel;
            value1 << std::fixed << std::setprecision(1) << item->setup->desired_value << " l";
            value2 << std::fixed << std::setprecision(1) << item->setup->value << " l";
            break;
        case DI_REPAIR:
            description = strRepair;
            value1 << std::fixed << std::setprecision(1) << item->setup->desired_value;
            value2 << std::fixed << std::setprecision(1) << item->setup->value;
            break;
        case DI_TYRE_SET:
            description = strTireSet;
            if (item->setup->desired_value > 0.9)
            {
                value1 << "YES";
            }
            else
            {
                value1 << "NO";
            }
            break;
        case DI_COMPOUND_SET:
            description = strTireCompounds;
            if(item->setup->desired_value == 1)
            {
                value1 << "SOFT";
            }
            else if (item->setup->desired_value == 2)
            {
                value1 << "MEDIUM";
            }
            else if (item->setup->desired_value == 3)
            {
                value1 << "HARD";
            }
            else if (item->setup->desired_value == 4)
            {
                value1 << "WET";
            }
            else if (item->setup->desired_value == 5)
            {
                value1 << "EXTREME WET";
            }
            break;
        case DI_FRONT_WING_ANGLE:
            description = strFrontWing;
            value1 << std::fixed << std::setprecision(1) << RAD2DEG(item->setup->desired_value);
            value2 << std::fixed << std::setprecision(1) << RAD2DEG(item->setup->value);
            break;
        case DI_REAR_WING_ANGLE:
            description = strRearWing;
            value1 << std::fixed << std::setprecision(1) << RAD2DEG(item->setup->desired_value);
            value2 << std::fixed << std::setprecision(1) << RAD2DEG(item->setup->value);
            break;
        case DI_PENALTY:
            description = strPenalty;

            if (item->setup->desired_value > 0.9)
            {
                value1 << "PENALTY";
            }
            else
            {
                value1 << "REPAIR";
            }
            break;
        }
        hudTextElements["dash-items-type"]->setText(description);
        hudTextElements["dash-items-value1"]->setColor(colorYellow);
        hudTextElements["dash-items-value1"]->setText(value1.str());
        hudTextElements["dash-items-value2"]->setColor(colorCyan);
        hudTextElements["dash-items-value2"]->setText(value2.str());
    }

// driver inputs
    changeImageSize(hudImgElements["driverinput-clutch"], currCar->_clutchCmd, "bottom", hudScale);
    changeImageSize(hudImgElements["driverinput-brake"], currCar->_brakeCmd, "bottom", hudScale);
    changeImageSize(hudImgElements["driverinput-throttle"], currCar->_accelCmd, "bottom", hudScale);

    const osg::Vec3d axis(0, 0, 1);
    //wheel

    double angle = currCar->_steerCmd * currCar->_steerWheelRot / 2.0f;

    osg::Vec3d center = hudImgRotableElements["driverinput-wheel"]->getBound().center();//Save the center point coordinates of the object first
    osg::Matrix curMatrix = hudImgRotableElements["driverinput-wheel"]->getMatrix();

    curMatrix *= osg::Matrix::translate(-center);//Move the object to the origin of the world coordinate system
    curMatrix *= osg::Matrix::rotate(-prevSteerAngle, axis);//rotate
    curMatrix *= osg::Matrix::rotate(angle, axis);//rotate
    curMatrix *= osg::Matrix::translate(center);//move back to the original position of the object

    hudImgRotableElements["driverinput-wheel"]->setMatrix(curMatrix);

    prevSteerAngle = angle;

// tires temps
    for (int i = 0; i < 4; i++) { //for each tires

        std::ostringstream tireName;
        switch(i) {
            case 0:
                tireName << "fr";
            break;
            case 1:
                tireName << "fl";
            break;
            case 2:
                tireName << "rr";
            break;
            case 3:
                tireName <<  "rl";
            break;
            default:
                tireName << "";
        }

        float currentTemp = 0;
        std::string tirePartName;

        currentTemp = currCar->_tyreT_out(i);

        std::ostringstream tireNameCold;
        std::ostringstream tireNameOptimal;
        std::ostringstream tireNameHot;

        tireNameCold << "tire-" << tireName.str().c_str() << "-cold";
        tireNameOptimal << "tire-" << tireName.str().c_str() << "-optimal";
        tireNameHot << "tire-" << tireName.str().c_str() << "-hot";

        float optimalAlpha = 0.0f;
        float hotAlpha = 0.0f;

        float tempOptimal = currCar->_tyreT_opt(i);
        float tempMaxCold = tempOptimal - ( tempOptimal * 10 / 100 ); //temp at witch we will conside the tire maximun cold
        float tempMaxHot = tempOptimal + ( tempOptimal * 10 / 100 ); //temp at witch we will conside the tire maximun hot

        //update the cold part
        changeImageAlpha(hudImgElements[tireNameCold.str().c_str()], 1.0f);

        //update the optimal part
        optimalAlpha = (currentTemp-tempMaxCold) / (tempOptimal-tempMaxCold);
        if (optimalAlpha > 1.0f){
            optimalAlpha = 1.0f;
        }
        if (optimalAlpha < 0.0f){
            optimalAlpha = 0.0f;
        }
        changeImageAlpha(hudImgElements[tireNameOptimal.str().c_str()], optimalAlpha);

        //update the hot part
        hotAlpha = (tempMaxHot-currentTemp) / (tempMaxHot-tempOptimal);
        if (hotAlpha > 1.0f){
            hotAlpha = 1.0f;
        }
        if (hotAlpha < 0.0f){
            hotAlpha = 0.0f;
        }
        changeImageAlpha(hudImgElements[tireNameHot.str().c_str()], 1.0-hotAlpha);

        //temps string
        temp.str("");
        //set the current tire temp (internally the tire temp is in KELVIN so we convert it to Centrigrade)
        temp << K2C(currCar->_tyreT_mid(i));
        std::ostringstream tireNameText;
        tireNameText << "tire-" << tireName.str().c_str()  << "-temps";
        hudTextElements[tireNameText.str().c_str()]->setText(temp.str());

        //set the temperature unit
        temp.str("");
        temp << "C";
        tireNameText.str("");
        tireNameText << "tire-" << tireName.str().c_str()  << "-temps-unit";
        hudTextElements[tireNameText.str().c_str()]->setText(temp.str());

        //pressures string
        temp.str("");
        tireNameText.str("");
        //set the current tire temp (internally the tire temp is in KPA *1000 so we wanto to convert it ot PSI)
        temp << currCar->_tyreCurrentPressure(i) * 0.145038 / 1000;
        tireNameText << "tire-" << tireName.str().c_str()  << "-pressures";
        hudTextElements[tireNameText.str().c_str()]->setText(temp.str());

        //set the temperature unit
        temp.str("");
        temp << "PSI";
        tireNameText.str("");
        tireNameText << "tire-" << tireName.str().c_str()  << "-pressures-unit";
        hudTextElements[tireNameText.str().c_str()]->setText(temp.str());

        // the brakes section has been commented out both here and in the osghud config file since the current phisics implementation is too unrealistic for this part of the hud to be useful
        // but please leave this here for when it will be made better in the future
        /*
        //set brake temps
        float brakeTemp = currCar->_brakeTemp(i);

        tireNameText.str("");
        tireNameText << "brake-" << tireName.str().c_str()  << "-cold";
        changeImageAlpha(hudImgElements[tireNameText.str().c_str()], 1.0);

        tireNameText.str("");
        tireNameText << "brake-" << tireName.str().c_str()  << "-optimal";
        changeImageAlpha(hudImgElements[tireNameText.str().c_str()], (brakeTemp*2));

        tireNameText.str("");
        tireNameText << "brake-" << tireName.str().c_str()  << "-hot";
        changeImageAlpha(hudImgElements[tireNameText.str().c_str()], ((brakeTemp-0.5)*2));
        */
    }



// tire wear
    changeImageSize(hudImgElements["tire-fr-cold"], currCar->_tyreTreadDepth(0), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-fr-optimal"], currCar->_tyreTreadDepth(0), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-fr-hot"], currCar->_tyreTreadDepth(0), "bottom", hudScale);

    changeImageSize(hudImgElements["tire-fl-cold"], currCar->_tyreTreadDepth(1), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-fl-optimal"], currCar->_tyreTreadDepth(1), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-fl-hot"], currCar->_tyreTreadDepth(1), "bottom", hudScale);

    changeImageSize(hudImgElements["tire-rr-cold"], currCar->_tyreTreadDepth(2), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-rr-optimal"], currCar->_tyreTreadDepth(2), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-rr-hot"], currCar->_tyreTreadDepth(2), "bottom", hudScale);

    changeImageSize(hudImgElements["tire-rl-cold"], currCar->_tyreTreadDepth(3), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-rl-optimal"], currCar->_tyreTreadDepth(3), "bottom", hudScale);
    changeImageSize(hudImgElements["tire-rl-hot"], currCar->_tyreTreadDepth(3), "bottom", hudScale);

//tire slip
    float slip = 0.0f;
    slip = currCar->_wheelSlipNorm(0)/currCar->_wheelSlipOpt(0);
    changeImageAlpha(hudImgElements["tire-fr-slip"], slip);
    slip = currCar->_wheelSlipNorm(1)/currCar->_wheelSlipOpt(1);
    changeImageAlpha(hudImgElements["tire-fl-slip"], slip);
    slip = currCar->_wheelSlipNorm(2)/currCar->_wheelSlipOpt(2);
    changeImageAlpha(hudImgElements["tire-rr-slip"], slip);
    slip = currCar->_wheelSlipNorm(3)/currCar->_wheelSlipOpt(3);
    changeImageAlpha(hudImgElements["tire-rl-slip"], slip);

//tire compound
    for (int i = 0; i < 4; i++) { //for each tires
		temp.str("");
		switch((int)currCar->_tyreCompound(i))
		{
		case 1:
			temp << "S"; //SOFT
			break;
		case 2:
			temp << "M"; //MEDIUM
			break;
		case 3:
			temp << "H"; //HARD
			break;
		case 4:
			temp << "R"; //WET
			break;
		case 5:
			temp << "R+"; //EXTREME WET
			break;
		default:
			temp << ""; //no valid value found, leave empty
		}
		if (i==0) hudTextElements["tire-fr-compound"]->setText(temp.str());
		if (i==1) hudTextElements["tire-fl-compound"]->setText(temp.str());
		if (i==2) hudTextElements["tire-rr-compound"]->setText(temp.str());
		if (i==3) hudTextElements["tire-rl-compound"]->setText(temp.str());
	}



//gforces
    osg::BoundingBox gforcegraphbb =hudImgElements["gforces-graph"]->getBoundingBox();
    osg::BoundingBox gforcedotbb = hudImgElements["gforces-dot"]->getBoundingBox();
    osg::Vec3f position = calculatePosition(gforcedotbb,"mc",gforcegraphbb,"mc", 0.0f, 0.0f);
    changeImagePosition(
        hudImgElements["gforces-dot"],
        gforcegraphbb.xMin()+position.x()+currCar->_DynGC.acc.y * 3.5 * 1,//horizontal
        gforcegraphbb.yMin()+position.y()-currCar->_DynGC.acc.x * 3.5,//vertical
        hudScale
    );
//track conditions
    //temperature
    temp.str("");
    temp << K2C(currCar->_airtemp);
    hudTextElements["trackdata-airtemp-data"]->setText(temp.str());
    temp.str("C");
    hudTextElements["trackdata-airtemp-unit"]->setText(temp.str());

    //timeof day
    temp.str("");
    double timeofday = SimTimeOfDay + s->currentTime;

    int hours = (int)(timeofday/60/60);
    int minutes = (int)((timeofday - (hours*60*60))/60);
    int seconds = timeofday - (hours*60*60) - (minutes*60);

    if(hours < 10){
        temp << "0" << hours << ":";
    }else{
        temp << hours << ":";
    }
    if(minutes < 10){
        temp << "0" << minutes << ".";
    }else{
        temp << minutes << ":";
    }
    if (seconds < 10){
        temp << "0" << seconds;
    }else{
        temp << seconds;
    }
    hudTextElements["trackdata-time-data"]->setText(temp.str());

    //SimClouds 0=none, 1=cirrus, 2=few, 3=many, 7=full
    //SimRain  0=none, 1=little, 2=medium, 3=heavy
    //disable all, we will enable only the one needed later
    changeImageAlpha(hudImgElements["trackdata-weathericon-sunny"],    0.0f);
    changeImageAlpha(hudImgElements["trackdata-weathericon-overcast"], 0.0f);
    changeImageAlpha(hudImgElements["trackdata-weathericon-cloudy"],   0.0f);
    changeImageAlpha(hudImgElements["trackdata-weathericon-rainy"],    0.0f);

    //sunny
    if(SimClouds == 0 && SimRain == 0){
        changeImageAlpha(hudImgElements["trackdata-weathericon-sunny"],    1.0f);
    }
    //overcast
    if(SimClouds >= 1 && SimClouds <= 2 && SimRain == 0){
        changeImageAlpha(hudImgElements["trackdata-weathericon-overcast"], 1.0f);
    }
    //cloudy
    if(SimClouds > 2 && SimRain == 0){
        changeImageAlpha(hudImgElements["trackdata-weathericon-cloudy"],   1.0f);
    }
    //rainy
    if(SimRain > 0){
        changeImageAlpha(hudImgElements["trackdata-weathericon-rainy"],    1.0f);
    }
//shifting leds
    if (hudWidgets["shiftlightsWidget"]->getNodeMask()){
        //read the car handle //engine section//torque data points
         GfParmGetNum(currCar->_carHandle, SECT_ENGINE, ARR_DATAPTS, NULL, 0.0f);

        //get the number of datapoint in the engine section
        std::string dataPointsPath = SECT_ENGINE "/" ARR_DATAPTS;
        int dataPointsCount = GfParmGetEltNb(currCar->_carHandle, dataPointsPath.c_str());

        //engine data point values will be stored here
        float rpm = 0.0f;
        float torque = 0.0f;

        //values from previous datapoint
        float prevrpm = 0.0f;
        float prevtorque = 0.0f;

        float intermediaterpm = 0.0f;
        float intermediatetorque = 0.0f;

        //get the data points
        //we do this one time (when a new car is selected)
        if (carHasChanged)
        {
            //empty the vector to
            horsepowerPoints.clear();

            //fill it with data, for each datapoint
            for (int i = 1; i <= dataPointsCount; i++)
            {
                std::string dataPointPath = dataPointsPath+"/"+ std::to_string(i);

                rpm = GfParmGetNum(currCar->_carHandle, dataPointPath.c_str(), PRM_RPM, (char*) NULL, 0.0f/*RevsMax*/);
                torque = GfParmGetNum(currCar->_carHandle, dataPointPath.c_str(), PRM_TQ, (char*) NULL, 0.0f);

                //if the current data point has the same value of the previous datapoint; skip it
                if (rpm > 0.0f && rpm == prevrpm){
                    continue;
                }

                // insert the horsepower value at the given rpm
                //auto position = horsepowerPoints.begin() + rpm;
                float value = torque * rpm;

                //calculate intermediate point/values from previous to current datapoin
                for( int h = 1+prevrpm; h < (int)rpm; h++)
                {
                    if(h > (int)currCar->_enginerpmMax) {break;}//quit the for cicle when we reach the rpmMax
                    intermediaterpm = h;
                    intermediatetorque =  prevtorque + (((torque-prevtorque) / (rpm - prevrpm)) * (h-prevrpm));
                    //GfLogInfo("Intermediate Point %i - rpm %i - torque %f \n", 0, (int)intermediaterpm, intermediatetorque);

                    // insert the horsepower value at the given rpm
                    float value = intermediatetorque * intermediaterpm;
                    horsepowerPoints.push_back(value);
                }
                //insert the poin value
                horsepowerPoints.push_back(value);

                prevrpm = rpm;
                prevtorque = torque;
            }
        }
        /*
        GfLogInfo("RPM %i \n", (int)currCar->_enginerpm);
        GfLogInfo("Gear %f - GearRation %f \n", (float)currCar->_gear, (float)currCar->_gearRatio[(int) (currCar->_gear + currCar->_gearOffset)]);
        GfLogInfo("nextGear %f - GearRation %f \n", (float)(currCar->_gear + 1.0), (float)currCar->_gearRatio[(int) (currCar->_gear + currCar->_gearOffset +1)]);
        */
        //only do this when engine rpm is not 0 and the gear is not neutral
        //otherwise we get infinite values
        if(currCar->_enginerpm > 0 && currCar->_gear > 0 ){
            float nextRpm = currCar->_enginerpm / currCar->_gearRatio[(int) (currCar->_gear + currCar->_gearOffset)] * currCar->_gearRatio[(int) (currCar->_gear + currCar->_gearOffset +1)];

            //reset all the led to off, we will turn on what's needed later
            hudImgElements["light1-on"]->setNodeMask(0);
            hudImgElements["light2-on"]->setNodeMask(0);
            hudImgElements["light3-on"]->setNodeMask(0);
            hudImgElements["light4-on"]->setNodeMask(0);
            hudImgElements["light5-on"]->setNodeMask(0);
            hudImgElements["light6-on"]->setNodeMask(0);
            hudImgElements["light7-on"]->setNodeMask(0);
            hudImgElements["light8-on"]->setNodeMask(0);
            hudImgElements["light9-on"]->setNodeMask(0);
            hudImgElements["light10-on"]->setNodeMask(0);
            hudImgElements["light11-on"]->setNodeMask(0);
            hudImgElements["light12-on"]->setNodeMask(0);

            float horsepowerCurr = horsepowerPoints.at((int)currCar->_enginerpm);
            float horsepowerNext = horsepowerPoints.at((int)nextRpm);
            //GfLogInfo("Curr gear %i (%f) - RPM %i - CurrPower %f \n", currCar->_gear, currCar->_gearRatio[(int) (currCar->_gear + currCar->_gearOffset)], (int)RADS2RPM(currCar->_enginerpm), horsepowerCurr);
            //GfLogInfo("Next gear %i (%f)- RPM %i - nextPower %f \n", currCar->_gear+1, currCar->_gearRatio[(int) (currCar->_gear + currCar->_gearOffset+1)], (int)RADS2RPM(nextRpm), horsepowerNext);

            //green leds
            if( horsepowerNext > horsepowerCurr * 80/100){
                hudImgElements["light1-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 82/100){
                hudImgElements["light2-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 84/100){
                hudImgElements["light3-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 86/100){
                hudImgElements["light4-on"]->setNodeMask(1);
            }
            //red leds
            if( horsepowerNext > horsepowerCurr * 88/100){
                hudImgElements["light5-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 90/100){
                hudImgElements["light6-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 92/100){
                hudImgElements["light7-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 94/100){
                hudImgElements["light8-on"]->setNodeMask(1);
            }
            //purple leds
            if( horsepowerNext > horsepowerCurr * 96/100){
                hudImgElements["light9-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 98/100){
                hudImgElements["light10-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 100/100){
                hudImgElements["light11-on"]->setNodeMask(1);
            }
            if( horsepowerNext > horsepowerCurr * 102/100){
                hudImgElements["light12-on"]->setNodeMask(1);
            }
        }
    }

// debug info
    if (frameInfo->fInstFps != 0)
    {
        temp.str("");
        temp << "FPS: " << frameInfo->fInstFps << " (AVG: " << frameInfo->fAvgFps
            << " MIN: " << frameInfo->fMinFps << " MAX: " << frameInfo->fMaxFps << ")";
        hudTextElements["debug-info"]->setText(temp.str());
    }
    else
        hudTextElements["debug-info"]->setText("");

// delta best
    if (currCar->_bestLapTime != 0){
        float deltabest = currCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine] - currCar->_bestLapTimeAtTrackPosition[(int)currCar->_distFromStartLine];

        GfLogDebug("OSGHUD curr: %f \n", currCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine]);
        GfLogDebug("OSGHUD best: %f \n", currCar->_bestLapTimeAtTrackPosition[(int)currCar->_distFromStartLine]);

        if(deltabest > 0){//we are slower
            float scale = 0.0f;
            if (deltabest > 10.0f){
                scale = 1.0f;
            }else{
                scale = deltabest/10;
            }
            changeImageSize(hudImgElements["delta-gaining"], 0, "left", hudScale);
            changeImageSize(hudImgElements["delta-losing"], scale, "right", hudScale);
        }else if(deltabest < 0){//we are faster
            float scale = 0.0f;
            if (deltabest < -10.0f){
                scale = 1.0f;
            }else{
                scale = -1*deltabest/10;
            }
            changeImageSize(hudImgElements["delta-gaining"], scale, "left", hudScale);
            changeImageSize(hudImgElements["delta-losing"], 0, "right", hudScale);
        }
        temp.str("");
        temp << std::setprecision(3) << deltabest;
        hudTextElements["delta-time"]->setText(temp.str());
    }else{
        changeImageSize(hudImgElements["delta-gaining"], 0, "left", hudScale);
        changeImageSize(hudImgElements["delta-losing"], 0, "right", hudScale);
    }

//tachometer
        osg::BoundingBox newtachobb =hudImgElements["tachometer"]->getBoundingBox();
        //let the final 15% be redline
        float redLinePercent = 0.15f; //15%
        float whiteLinePercent = 1.0f-redLinePercent; //85%
        float rpmVal1 = 0.0f;
        float rpmVal2 = 0.0f;
        float startAngle = 291.0f;
        float endAngle = 180.0f;
        float totalAngleRange = 0.0f;
        float endOfWhiteLineAngle = 0.0f;

        if(startAngle < endAngle){
            totalAngleRange = endAngle - startAngle;
            endOfWhiteLineAngle = (totalAngleRange * whiteLinePercent) + startAngle;
        }else{
            totalAngleRange = (360.0f - startAngle) + endAngle;
            endOfWhiteLineAngle = (totalAngleRange * whiteLinePercent) - (360.0f - startAngle);
            if(endOfWhiteLineAngle < 0){
                endOfWhiteLineAngle = endOfWhiteLineAngle + 360.0f;
            }
        }

        if(currCar->_enginerpm <= (currCar->_enginerpmRedLine * whiteLinePercent)){
            rpmVal1 = currCar->_enginerpm / (currCar->_enginerpmRedLine * whiteLinePercent);
            rpmVal2 = 0.0;
        }else{
            rpmVal1 = 1.0;
            rpmVal2 = (currCar->_enginerpm - (currCar->_enginerpmRedLine * whiteLinePercent)) / (currCar->_enginerpmRedLine * redLinePercent);
        }

        changeImageVertex(hudImgVertexElements["newtacho-rpmon"], startAngle, endOfWhiteLineAngle, rpmVal1, newtachobb.xMin(), newtachobb.yMin(), hudScale);
        changeImageVertex(hudImgVertexElements["newtacho-rpmonred"], endOfWhiteLineAngle, endAngle, rpmVal2, newtachobb.xMin(), newtachobb.yMin(), hudScale);

//driverinfo
        double deltaAhead =0.0f;
        double deltaBehind =0.0f;

        //if we are not first get the gap to the car directly ahead of us
        if(currCar->_pos > 1){
            //if he is in my same lap
            if (firstAheadCar->_laps == currCar->_laps){
                //get how much time it took him to reach his position from my position
                deltaAhead = firstAheadCar->_currLapTimeAtTrackPosition[(int)firstAheadCar->_distFromStartLine] - firstAheadCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine];
                deltaAhead = deltaAhead * -1;
            }else{
                // if he is on the next lap or many laps ahead
                int lapsAhead = firstAheadCar->_laps - currCar->_laps;
                //if he has not lapped me
                if (currCar->_distFromStartLine > firstAheadCar->_distFromStartLine){
                    deltaAhead = (lapsAhead -1) * firstAheadCar->_lastLapTime; //for simplicity we consider his last laptime (this may be totally wrong if he for example pitted in the last lap)
                    deltaAhead += firstAheadCar->_lastLapTime - firstAheadCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine];//how much time took him to reach the startline from my position
                    deltaAhead += firstAheadCar->_currLapTimeAtTrackPosition[(int)firstAheadCar->_distFromStartLine];//how much time took him to reach his position from the startline
                    deltaAhead = deltaAhead * -1;
                //he has lapped me
                }else{
                    deltaAhead += lapsAhead * firstAheadCar->_lastLapTime; //for simplicity we consider his last laptime (this may be totally wrong if he for example pitted in the last lap)
                    deltaAhead += firstAheadCar->_currLapTimeAtTrackPosition[(int)firstAheadCar->_distFromStartLine] - firstAheadCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine];//how much time took him to reach his position from my position in this lap
                    deltaAhead = deltaAhead * -1;
                }
            }
        }

        //if we are not last get the gap to the car directly behind us
        if(currCar->_pos < s->_ncars){
            //if he is in my same lap
            if (firstBehindCar->_laps == currCar->_laps){
                //get how much time it took me to reach my position from my position
                deltaBehind = currCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine] - currCar->_currLapTimeAtTrackPosition[(int)firstBehindCar->_distFromStartLine];
            }else{
                // if he is on the prev lap or many laps Behind
                int lapsBehind = currCar->_laps - firstBehindCar->_laps;
                //if we have not lapped him
                if (currCar->_distFromStartLine < firstBehindCar->_distFromStartLine){
                    deltaBehind = (lapsBehind -1) * currCar->_lastLapTime; //for simplicity we consider our last laptime (this may be totally wrong if we for example have pitted in the last lap)
                    deltaBehind += currCar->_lastLapTime - currCar->_currLapTimeAtTrackPosition[(int)firstBehindCar->_distFromStartLine];//how much time took us to reach the startline from his position
                    deltaBehind += currCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine];//how much time took us to reach our position from the startline
                    deltaBehind = deltaBehind;
                //we have lapped him
                }else{
                    deltaBehind += lapsBehind * currCar->_lastLapTime; //for simplicity we consider his last laptime (this may be totally wrong if he for example pitted in the last lap)
                    deltaBehind += currCar->_currLapTimeAtTrackPosition[(int)firstBehindCar->_distFromStartLine] - currCar->_currLapTimeAtTrackPosition[(int)currCar->_distFromStartLine];//how much time took us to reach our position from his position in this lap
                    deltaBehind = deltaBehind;
                }
            }
        }
        //get the actual name of the car
        temp.str("");

        //update the hud strings
        temp.str("");
        temp << currCar->_pos;
        hudTextElements["driverinfo-position"]->setText(temp.str());
        hudTextElements["driverinfo-name"]->setText(currCar->_name);
        hudTextElements["driverinfo-team"]->setText(currCar->_teamname);

        hudTextElements["driverinfo-carclass"]->setText(currCar->_category);
        hudTextElements["driverinfo-carmodel"]->setText(GetCarName(currCar));

        hudTextElements["driverinfo-bestlap"]->setText(formatLaptime(currCar->_bestLapTime,0));
        hudTextElements["driverinfo-lastlap"]->setText(formatLaptime(currCar->_lastLapTime,0));

        if( s->_ncars > 1 ){
			//if we are not first
			if(currCar->_pos > 1){
				temp.str("");
				temp << "P" << firstAheadCar->_pos;
				hudTextElements["driverinfo-posahead"]->setText(temp.str());
				hudTextElements["driverinfo-gapahead"]->setText(formatLaptime(deltaAhead,-1));
			}else{
				temp.str("");
				hudTextElements["driverinfo-posahead"]->setText(temp.str());
				hudTextElements["driverinfo-gapahead"]->setText(temp.str());
			}
			//if we are not last
			if(currCar->_pos < s->_ncars){
				temp.str("");
				temp << "P" << firstBehindCar->_pos;
				hudTextElements["driverinfo-posbehind"]->setText(temp.str());
				hudTextElements["driverinfo-gapbehind"]->setText(formatLaptime(deltaBehind,-1));
			}else{
				temp.str("");
				hudTextElements["driverinfo-posbehind"]->setText(temp.str());
				hudTextElements["driverinfo-gapbehind"]->setText(temp.str());
			}
		}else{
			temp.str("");
			hudTextElements["driverinfo-posahead"]->setText(temp.str());
			hudTextElements["driverinfo-posbehind"]->setText(temp.str());
			hudTextElements["driverinfo-gapahead"]->setText(temp.str());
			hudTextElements["driverinfo-gapbehind"]->setText(temp.str());
		}
        //special case for human drivers (since they have no team and nation assigned (for now)
        if(currCar->_driverType == RM_DRV_HUMAN){
            hudTextElements["driverinfo-team"]->setText("");
            hudImgElements["driverinfo-flag"]->setNodeMask(0);
        }else{
            hudImgElements["driverinfo-flag"]->setNodeMask(1);
        }
        //update the flag texture
        std::string nationname = currCar->_nationname;
        //to lowercase
        std::transform(nationname.begin(), nationname.end(), nationname.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        temp.str("");
        std::string uk = "uk";
        if (nationname == uk){
			nationname = "gb";
		}
        temp << "data/img/flags/4x3png/" << nationname.c_str() <<".png";
        changeImageTexture(hudImgElements["driverinfo-flag"], temp.str());

//edithud
    if (hudEditModeEnabled){
        //if there is some widgetGroup selected display the edithud and relative controls around it else keep it hidden
        if (!selectedWidgetGroup.empty()){
            hudWidgets["edithudWidget"]->setNodeMask(1);
        }else{
            hudWidgets["edithudWidget"]->setNodeMask(0);
        }

        tMouseInfo    *mouse;
        mouse = GfuiMouseInfo();

        //mouse started to be pressed
        if (prevMouseButtonState == 0  && mouse->button[0] == 1){
            mouseStartDragX = mouse->X;
            mouseStartDragY = mouse->Y;
            prevMouseButtonState = 1;

            //get the toggle bonding box and detect clicks on it
            osg::BoundingBox toggleOnOffBB = hudImgElements["edithud-toggleoff"]->getBoundingBox();
            float mousePosX = mouse->X * hudScreenW /640;
            float mousePosY = mouse->Y * hudScreenH /480;
            if (mousePosX >= toggleOnOffBB.xMin() && mousePosX <= toggleOnOffBB.xMax()
            && mousePosY >= toggleOnOffBB.yMin() && mousePosY <= toggleOnOffBB.yMax()){
                GfLogInfo("Clicked toggle\n");
                //toggle the widget enabled/disabled status
                ToggleHUDwidget(selectedWidgetGroup);
                hudWidgets[selectedWidgetGroup.c_str()]->setNodeMask(1);

                if(hudImgElements["edithud-toggleon"]->getNodeMask()==0){
                    hudImgElements["edithud-toggleoff"]->setNodeMask(0);
                    hudImgElements["edithud-toggleon"]->setNodeMask(1);
                    GfLogInfo("OSGHUD: done");
                }else{
                    hudImgElements["edithud-toggleoff"]->setNodeMask(1);
                    hudImgElements["edithud-toggleon"]->setNodeMask(0);
                }
            }


            //check mouse widgets collisions
            //open the osghud config file file
            void *paramHandle2 = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);

            //cicle throught each element of the widgetGroup
            if (GfParmListSeekFirst(paramHandle2, "widgets") == 0)
            {
                do
                {
                    std::string widgetGroupName = GfParmListGetCurEltName(paramHandle2,"widgets");
                    if (widgetGroupName.find("edithudWidget")!=std::string::npos){
                        continue;
                    }
                    if (widgetGroupName.find("mouseWidget")!=std::string::npos){
                        continue;
                    }

                    bool collision = isMouseOverWidgetGroup(widgetGroupName);
                        //GfLogInfo("OSGHUD: %i :  %s \n", collision, widgetGroupName.c_str());
                    if (collision == true){
                        GfLogInfo("OSGHUD: mouse has clicked on:  %s \n", widgetGroupName.c_str());
                        selectWidgetGroupByName(widgetGroupName);
                    }

                } while (GfParmListSeekNext(paramHandle2, "widgets") == 0);
            }
            //release the config file
            GfParmReleaseHandle(paramHandle2);
        }
        //mouse stopped to be pressed
        if (prevMouseButtonState == 1  && mouse->button[0] == 0){
            mouseTotalDragX = (mouse->X - mouseStartDragX) * (float)hudScreenW /640;
            mouseTotalDragY = (mouse->Y - mouseStartDragY) * (float)hudScreenH /480;
            mouseStartDragX = 0;
            mouseStartDragY = 0;
            prevMouseButtonState = 0;
            GfLogInfo("Mouse was dragged for x:%i y:%i \n", mouseDragX, mouseDragY); //0 left button 1 wheelbutton 2 right button//default mouse resolution is set to be 640x480 (must be scaled to match the actual resolution)
            GfLogInfo("Mouse total drag x:%i y:%i \n", mouseTotalDragX, mouseTotalDragY); //0 left button 1 wheelbutton 2 right button//default mouse resolution is set to be 640x480 (must be scaled to match the actual resolution)

            if (!selectedWidgetGroup.empty()){
                saveWidgetGroupPosition(selectedWidgetGroup);
            }
        }else{
            mouseDragX = 0;
            mouseDragY = 0;
        }

        if (mouse->button[0] == 1){
            mouseDragX =  (mouse->X - mousePrevPosX);
            mouseDragY =  (mouse->Y - mousePrevPosY);
        }
        mousePrevPosX = mouse->X;
        mousePrevPosY = mouse->Y;

        //move it to the correct position
        float moveX = (float)mouseDragX * (float)hudScreenW /640;
        float moveY = (float)mouseDragY * (float)hudScreenH /480;

    //start doing hud things
        std::string selectedWidgetGroupPath = "widgets/" + selectedWidgetGroup;

        //start
        //move the widget
        //open the osghud config file file
        void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);

        //cicle throught each element of the widgetGroup
        if (GfParmListSeekFirst(paramHandle, selectedWidgetGroupPath.c_str()) == 0)
        {
            do
            {
                std::string widgetName = GfParmListGetCurEltName(paramHandle,selectedWidgetGroupPath.c_str());

                if ( hudTextElements.find(widgetName) != hudTextElements.end() )
                {
                    osg::Vec3 textPosition = hudTextElements[widgetName.c_str()]->getPosition();
                    textPosition[0]=textPosition[0]+moveX;
                    textPosition[1]=textPosition[1]+moveY;
                    hudTextElements[widgetName.c_str()]->setPosition(textPosition);
                }
                else if ( hudImgElements.find(widgetName) != hudImgElements.end() )
                {
                    osg::BoundingBox editedwidgetBB = hudImgElements[widgetName.c_str()]->getBoundingBox();
                    changeImagePosition(
                        hudImgElements[widgetName.c_str()],
                        editedwidgetBB.xMin()+moveX,
                        editedwidgetBB.yMin()+moveY,
                        hudScale
                    );
                }

                else if ( hudGraphElements.find(widgetName) != hudGraphElements.end() )
                {
                    //todo: I havent figured out how to move these without messing them out
                    //widgetBoundingBox = hudGraphElements[positionRefObj]->getBoundingBox();
                }

                else
                {
                    GfLogDebug("OSGHUD: This is not a recognized widget type. I dont know what to do with this.");
                }
            } while (GfParmListSeekNext(paramHandle, selectedWidgetGroupPath.c_str()) == 0);
        }

        //release the config file
        GfParmReleaseHandle(paramHandle);
        //end



        //mouse update the mouse pointer position
        changeImagePosition(
            hudImgElements["mouse-normal"],
            mouse->X * (float)hudScreenW /640,
            (mouse->Y * (float)hudScreenH /480)-(128*hudScale),/*todo*/
            hudScale
        );



        //if there is no WidgetGroup selected we have nothing to do
        if (selectedWidgetGroup.empty()){
            return;
        }


        //get the new widgetGroup bounding box
        osg::BoundingBox targetWidgetGroupBoundingBox = getBoundigBoxFromWidgetGroupName(selectedWidgetGroup);
        /*
        GfLogInfo("OSGHUD: boundingbox: %s\n", selectedWidgetGroup.c_str());
        GfLogInfo("OSGHUD: boundingbox: xmin %f\n", targetWidgetGroupBoundingBox.xMin());
        GfLogInfo("OSGHUD: boundingbox: ymin %f\n", targetWidgetGroupBoundingBox.yMin());
        GfLogInfo("OSGHUD: boundingbox: xmax %f\n", targetWidgetGroupBoundingBox.xMax());
        GfLogInfo("OSGHUD: boundingbox: ymax %f\n", targetWidgetGroupBoundingBox.yMax());
        */

        /*
         * how vertices are arranged:
         *      3_______2
         *      |       |
         *    y |       |
         *      |       |
         *      0_______1
         *          x
         *
         * [vertices(0-3)][0]=x
         * [vertices(0-3)][1]=y
        * */

        //resize the edithudWidget to match the size of targetWidgetGroup
        //create the vertices for the image geometry and assign them

        //move the background
        osg::Vec3Array* vertices = new osg::Vec3Array;
        float depth = 0.0f-0.1f;
        vertices->push_back(osg::Vec3( targetWidgetGroupBoundingBox.xMin(),targetWidgetGroupBoundingBox.yMin(),depth)); //bottom left
        vertices->push_back(osg::Vec3( targetWidgetGroupBoundingBox.xMin(),targetWidgetGroupBoundingBox.yMax(),depth)); //bottom right
        vertices->push_back(osg::Vec3( targetWidgetGroupBoundingBox.xMax(),targetWidgetGroupBoundingBox.yMax(),depth)); //top right
        vertices->push_back(osg::Vec3( targetWidgetGroupBoundingBox.xMax(),targetWidgetGroupBoundingBox.yMin(),depth)); //topleft
        vertices->dirty();

        hudImgElements["edithud-background"]->setVertexArray(vertices);
        hudImgElements["edithud-background"]->setUseDisplayList(false);

        //move all the other pieces of the edithudWidget
        //osg::Vec3Array* targetVertices = dynamic_cast<osg::Vec3Array*>(hudImgElements["edithud-background"]->getVertexArray());
        std::vector<std::string> edithudWidgets;
        std::string currWidget;
        //edithudWidgets.push_back("edithud-background");
        edithudWidgets.push_back("edithud-titlebar");
        edithudWidgets.push_back("edithud-dragicon");
        edithudWidgets.push_back("edithud-resizeicon");
        edithudWidgets.push_back("edithud-toggleoff");
        edithudWidgets.push_back("edithud-toggleon");
        edithudWidgets.push_back("edithud-titletext");

        for (size_t i = 0; i < edithudWidgets.size(); ++i){
            if ( hudTextElements.find(edithudWidgets[i]) != hudTextElements.end() )
            {
                recalculateTextWidgetPosition("edithudWidget",edithudWidgets[i],hudScale);
            }
            else if ( hudImgElements.find(edithudWidgets[i]) != hudImgElements.end() )
            {
                recalculateImageWidgetPosition("edithudWidget",edithudWidgets[i],hudScale);
            }
        }
    }
}

void SDHUD::ToggleHUD()
{
    if (hudElementsVisibilityStatusEnabled)
    {
        hudElementsVisibilityStatus["boardWidget"] =        (int)hudWidgets["boardWidget"]->getNodeMask();
        hudElementsVisibilityStatus["racepositionWidget"] = (int)hudWidgets["racepositionWidget"]->getNodeMask();
        hudElementsVisibilityStatus["racelapsWidget"] =     (int)hudWidgets["racelapsWidget"]->getNodeMask();
        hudElementsVisibilityStatus["laptimeWidget"] =      (int)hudWidgets["laptimeWidget"]->getNodeMask();
        hudElementsVisibilityStatus["carinfoWidget"] =      (int)hudWidgets["carinfoWidget"]->getNodeMask();
        hudElementsVisibilityStatus["newtacho-rpmon"]=      (int)hudImgVertexElements["newtacho-rpmon"]->getNodeMask();
        hudElementsVisibilityStatus["carstatusWidget"] =    (int)hudWidgets["carstatusWidget"]->getNodeMask();
        hudElementsVisibilityStatus["driverinputWidget"] =  (int)hudWidgets["driverinputWidget"]->getNodeMask();
        hudElementsVisibilityStatus["driverinput-wheel"] =  (int)hudImgRotableElements["driverinput-wheel"]->getNodeMask();
        hudElementsVisibilityStatus["textFPSWidget"] =      (int)hudWidgets["textFPSWidget"]->getNodeMask();
        hudElementsVisibilityStatus["tiresWidget"] =        (int)hudWidgets["tiresWidget"]->getNodeMask();
        hudElementsVisibilityStatus["gforceWidget"] =       (int)hudWidgets["gforceWidget"]->getNodeMask();
        hudElementsVisibilityStatus["dashitemsWidget"] =    (int)hudWidgets["dashitemsWidget"]->getNodeMask();
        hudElementsVisibilityStatus["graphFPSWidget"] =     (int)hudWidgets["graphFPSWidget"]->getNodeMask();
        hudElementsVisibilityStatus["graphSpeedWidget"] =   (int)hudWidgets["graphSpeedWidget"]->getNodeMask();
        hudElementsVisibilityStatus["graphFFBWidget"] =     (int)hudWidgets["graphFFBWidget"]->getNodeMask();
        hudElementsVisibilityStatus["graphInputsWidget"] =  (int)hudWidgets["graphInputsWidget"]->getNodeMask();
        hudElementsVisibilityStatus["deltaWidget"] =        (int)hudWidgets["deltaWidget"]->getNodeMask();
        hudElementsVisibilityStatus["rpmWidget"] =          (int)hudWidgets["rpmWidget"]->getNodeMask();
        hudElementsVisibilityStatus["trackdataWidget"] =    (int)hudWidgets["trackdataWidget"]->getNodeMask();
        hudElementsVisibilityStatus["shiftlightsWidget"] =  (int)hudWidgets["shiftlightsWidget"]->getNodeMask();
        hudElementsVisibilityStatus["driverinfoWidget"] =   (int)hudWidgets["driverinfoWidget"]->getNodeMask();

        hudWidgets["boardWidget"]->setNodeMask(0);
        hudWidgets["racepositionWidget"]->setNodeMask(0);
        hudWidgets["racelapsWidget"]->setNodeMask(0);
        hudWidgets["laptimeWidget"]->setNodeMask(0);
        hudWidgets["carinfoWidget"]->setNodeMask(0);
        hudImgVertexElements["newtacho-rpmon"]->setNodeMask(0);
        hudImgVertexElements["newtacho-rpmonred"]->setNodeMask(0);
        hudWidgets["carstatusWidget"]->setNodeMask(0);
        hudWidgets["driverinputWidget"]->setNodeMask(0);
        hudImgRotableElements["driverinput-wheel"]->setNodeMask(0);
        hudWidgets["textFPSWidget"]->setNodeMask(0);
        hudWidgets["tiresWidget"]->setNodeMask(0);
        hudWidgets["gforceWidget"]->setNodeMask(0);
        hudWidgets["dashitemsWidget"]->setNodeMask(0);
        hudWidgets["graphFPSWidget"]->setNodeMask(0);
        hudWidgets["graphSpeedWidget"]->setNodeMask(0);
        hudWidgets["graphFFBWidget"]->setNodeMask(0);
        hudWidgets["graphInputsWidget"]->setNodeMask(0);
        hudWidgets["deltaWidget"]->setNodeMask(0);
        hudWidgets["rpmWidget"]->setNodeMask(0);
        hudWidgets["trackdataWidget"]->setNodeMask(0);
        hudWidgets["shiftlightsWidget"]->setNodeMask(0);
        hudWidgets["driverinfoWidget"]->setNodeMask(0);
        hudElementsVisibilityStatusEnabled = false;
    }else{
        hudWidgets["boardWidget"]->setNodeMask(hudElementsVisibilityStatus["boardWidget"]);
        hudWidgets["racepositionWidget"]->setNodeMask(hudElementsVisibilityStatus["racepositionWidget"]);
        hudWidgets["racelapsWidget"]->setNodeMask(hudElementsVisibilityStatus["racelapsWidget"]);
        hudWidgets["laptimeWidget"]->setNodeMask(hudElementsVisibilityStatus["laptimeWidget"]);
        hudWidgets["carinfoWidget"]->setNodeMask(hudElementsVisibilityStatus["carinfoWidget"]);
        hudImgVertexElements["newtacho-rpmon"]->setNodeMask(hudElementsVisibilityStatus["newtacho-rpmon"]);
        hudImgVertexElements["newtacho-rpmonred"]->setNodeMask(hudElementsVisibilityStatus["newtacho-rpmonred"]);
        hudWidgets["carstatusWidget"]->setNodeMask(hudElementsVisibilityStatus["carstatusWidget"]);
        hudWidgets["driverinputWidget"]->setNodeMask(hudElementsVisibilityStatus["driverinputWidget"]);
        hudImgRotableElements["driverinput-wheel"]->setNodeMask(hudElementsVisibilityStatus["driverinput-wheel"]);
        hudWidgets["textFPSWidget"]->setNodeMask(hudElementsVisibilityStatus["textFPSWidget"]);
        hudWidgets["tiresWidget"]->setNodeMask(hudElementsVisibilityStatus["tiresWidget"]);
        hudWidgets["gforceWidget"]->setNodeMask(hudElementsVisibilityStatus["gforceWidget"]);
        hudWidgets["dashitemsWidget"]->setNodeMask(hudElementsVisibilityStatus["dashitemsWidget"]);
        hudWidgets["graphFPSWidget"]->setNodeMask(hudElementsVisibilityStatus["graphFPSWidget"]);
        hudWidgets["graphSpeedWidget"]->setNodeMask(hudElementsVisibilityStatus["graphSpeedWidget"]);
        hudWidgets["graphFFBWidget"]->setNodeMask(hudElementsVisibilityStatus["graphFFBWidget"]);
        hudWidgets["graphInputsWidget"]->setNodeMask(hudElementsVisibilityStatus["graphInputsWidget"]);
        hudWidgets["deltaWidget"]->setNodeMask(hudElementsVisibilityStatus["deltaWidget"]);
        hudWidgets["rpmWidget"]->setNodeMask(hudElementsVisibilityStatus["rpmWidget"]);
        hudWidgets["trackdataWidget"]->setNodeMask(hudElementsVisibilityStatus["trackdataWidget"]);
        hudWidgets["shiftlightsWidget"]->setNodeMask(hudElementsVisibilityStatus["shiftlightsWidget"]);
        hudWidgets["driverinfoWidget"]->setNodeMask(hudElementsVisibilityStatus["driverinfoWidget"]);
        hudElementsVisibilityStatusEnabled = true;
    }
}

void SDHUD::ToggleHUDwidget(const std::string &widget)
{
    //read the previous status from the config file
    std::string path = "widgets/" + widget;
    std::string attribute = "enabled";
    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);
    int widgetEnabled = GfParmGetNum (paramHandle, path.c_str(), attribute.c_str(), "",0);

    //reverse the value
    widgetEnabled = !widgetEnabled;

    //apply to the current screen
    hudWidgets[widget]->setNodeMask(widgetEnabled);

    //save the value back in the config file
    GfParmSetNum(paramHandle, path.c_str(), attribute.c_str(), NULL, (int)widgetEnabled);
    GfParmWriteFile(NULL, paramHandle, "osghudconfig");

    //release the config file
    GfParmReleaseHandle(paramHandle);
}

void SDHUD::ToggleHUDwidgets(const std::string &widgets)
{
    std::vector<std::string> split_widgets;

    split(widgets, ',', split_widgets);

    for (size_t i = 0; i < split_widgets.size(); ++i)
        ToggleHUDwidget(split_widgets[i]);
}

void SDHUD::ToggleHUDdriverinput()
{
    ToggleHUDwidget("driverinputWidget");
    hudImgRotableElements["driverinput-wheel"]->setNodeMask(hudWidgets["driverinputWidget"]->getNodeMask());
}

osg::ref_ptr <osg::Group> SDHUD::generateHudFromXmlFile(int scrH, int scrW)
{
    osgGroupHud = new osg::Group;
    osg::ref_ptr<osg::Group> osgGroupWidgets = new osg::Group;

    osgGroupHud->addChild(osgGroupWidgets);

    //screen bounding box
    osg::BoundingBox screenBB;
    screenBB.expandBy(osg::Vec3(0.0f,0.0f,0.0f));
    screenBB.expandBy(osg::Vec3(scrW,scrH,0.0f));

    std::string mainSection= "widgets";
    std::string widgetsSectionPath;
    std::string widgetsSectionName;
    std::string subSectionPath;
    std::string subSectionName;

    //open the file
    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);

    //GfLogInfo("OSGHUD: Try to find all mains sections: %s \n", subSectionName.c_str());

    //cicle throught the widget sections
    if (GfParmListSeekFirst(paramHandle, mainSection.c_str()) == 0)
    {
        do
        {
            widgetsSectionName = GfParmListGetCurEltName(paramHandle, mainSection.c_str());
            widgetsSectionPath = mainSection + "/" + widgetsSectionName;
            GfLogDebug("OSGHUD: Generating hud: %s \n", widgetsSectionName.c_str());

            osg::ref_ptr<osg::Geode> geode = new osg::Geode;
            geode->setName(widgetsSectionName);
            osgGroupWidgets->addChild(geode);
            hudWidgets[widgetsSectionName.c_str()] = geode;

            // turn lighting off for the text and disable depth test to ensure it's always ontop.
            osg::StateSet* stateset = geode->getOrCreateStateSet();
            stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

            //read enabled disabled status for this widget from the config file
            int widgetEnabled = GfParmGetNum (paramHandle, widgetsSectionPath.c_str(),"enabled", "",0);

            //cicle throught each element of the widget
            if (GfParmListSeekFirst(paramHandle, widgetsSectionPath.c_str()) == 0)
            {
                do
                {
                        subSectionName = GfParmListGetCurEltName(paramHandle, widgetsSectionPath.c_str());
                        subSectionPath = widgetsSectionPath + "/" + subSectionName;

                        //get a list of the params in this section
                        std::vector<std::string> paramsInSection = GfParmListGetParamsNamesList(paramHandle, subSectionPath.c_str());
                        std::string type = GfParmGetStr (paramHandle, subSectionPath.c_str(),"type", "no default value" );

                        /* ============================
                             CREATE OSG TEXT
                           ============================*/
                        if (type == GFMNU_ATTR_TEXT)
                        {
                            //read data into local variables
                            const std::string &elementId = subSectionName;
                            std::string textStr =          GfuiGetTranslatedText (paramHandle, subSectionPath.c_str(), GFMNU_ATTR_TEXT, "" );
                            std::string fontFileUrl =      GfParmGetStr (paramHandle, subSectionPath.c_str(),"fontFileUrl", "" );
                            std::string colorString =      GfParmGetStr (paramHandle, subSectionPath.c_str(),"color", "" );
                            float fontSize =               GfParmGetNum (paramHandle, subSectionPath.c_str(),"fontSize", "",0 ) * hudScale;
                            std::string textAlign =        GfParmGetStr (paramHandle, subSectionPath.c_str(),"textAlign", "" );

                            std::string positionRefObj =       GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObj", "" );
                            std::string positionRefObjPoint =  GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObjPoint", "tl" );
                            std::string positionMyPoint =      GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-myPoint", "tl" );
                            float positionVerticalModifier =   GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-verticalModifier", "",0 ) * hudScale;
                            float positionHorizontalModifier = GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-horizontalModifier", "",0 ) * hudScale;

                            GfLogDebug("OSGHUD: Generate text object: %s \n", elementId.c_str());

                            //create the osg::text object
                            osgText::Text* text = new  osgText::Text;

                            //add the text obj to our pool (we willl need it later)
                            hudTextElements[elementId] = text;

                            //extract and apply the color
                            text->setColor(colorStringToVec4(colorString));

                            //set the font
                            {
                                std::string fontsMainDirectory = GetDataDir();
                                fontsMainDirectory = fontsMainDirectory+"data/fonts";
                                fontFileUrl = fontsMainDirectory+fontFileUrl;
                                text->setFont(fontFileUrl);

                                //font resolution
                                text->setFontResolution(200,200);

                                //set the font size
                                text->setCharacterSize(fontSize);
                            }

                            //set alignement
                            if (textAlign.empty() || textAlign=="LEFT_BOTTOM")
                            {
                                text->setAlignment(osgText::Text::LEFT_BOTTOM_BASE_LINE );
                            }
                            else if (textAlign=="RIGHT_BOTTOM")
                            {
                                text->setAlignment(osgText::Text::RIGHT_BOTTOM_BASE_LINE );
                            }
                            else if (textAlign=="CENTER_BOTTOM")
                            {
                                text->setAlignment(osgText::Text::CENTER_BOTTOM_BASE_LINE );
                            }

                            //set the text string
                            text->setText(textStr, osgText::String::Encoding::ENCODING_UTF8);

                            //set the position
                            //find the referenceObj bounding box
                            osg::BoundingBox refObjBb = getBoundigBoxFromWidgetName(positionRefObj);

                            //calculate the positioning
                            osg::Vec3 position = calculatePosition(text->getBoundingBox(),positionMyPoint,refObjBb,positionRefObjPoint, positionVerticalModifier, positionHorizontalModifier);

                            //asign the position
                            text->setPosition(position);

                            //TODO: enable shadows for texts?
                            //text->setBackdropType(osgText::Text::DROP_SHADOW_BOTTOM_RIGHT); //OUTLINE //DROP_SHADOW_BOTTOM_RIGHT
                            /*
                            text->setBackdropOffset(0.07f);
                            color = osg::Vec4(0.57f,0.57f,0.57f,0.6f);
                            text->setBackdropColor(color);
                            */

                            //add the text to the hud
                            geode->addDrawable( text );

                        }
                        else if( type == "image" || type == "imagerotable")
                        {
                            /* ============================
                                 CREATE OSG IMAGE
                               ============================*/
                            //read data into local variables
                            const std::string &elementId =      subSectionName;
                            std::string url =                   GfParmGetStr (paramHandle, subSectionPath.c_str(),"url", "" );

                            std::string positionRefObj =        GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObj", "" );
                            std::string positionRefObjPoint =   GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObjPoint", "tl" );
                            std::string positionMyPoint =       GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-myPoint", "tl" );
                            float positionVerticalModifier =    GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-verticalModifier", "",0 ) * hudScale;
                            float positionHorizontalModifier =  GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-horizontalModifier", "",0 ) * hudScale;

                            float forcedImageWidth = GfParmGetNum (paramHandle, subSectionPath.c_str(),"width", "",0 );
                            float forcedImageHeight = GfParmGetNum (paramHandle, subSectionPath.c_str(),"height", "",0 );

                            GfLogDebug("OSGHUD: Generate image object: %s \n", elementId.c_str());

                            //start preparing the image
                            std::string filename = GetDataDir();
                            filename = filename+url;

                            //get the bounding box
                            osg::BoundingBox bb;

                            for(unsigned int i=0;i<geode->getNumDrawables();++i)
                            {
                                bb.expandBy(geode->getDrawable(i)->getBoundingBox());
                            }

                            //check that the image file exist
                            if (!GfFileExists(filename.c_str()))
                            {
                                GfLogError ("OSGHUD: Specified image file does not exist: %s.\n", filename.c_str());
                            }

                            osg::Image* img = osgDB::readImageFile(filename);

                            //correct the image size to match the hud scale
                            float imgWidth = img->s() *  hudScale;
                            float imgHeight = img->t() * hudScale;

                            //if we specified a different size use that
                           // GfLogError ("OSGHUD: Forced image width %f: \n", forcedImageWidth);
                           // GfLogError ("for %s: \n", url.c_str());
                            if (forcedImageWidth > 0.0f){
                                GfLogError ("OSGHUD: Force a different image size for image: %s\n", url.c_str());
                                imgWidth = forcedImageWidth * hudScale;
                                imgHeight = forcedImageHeight * hudScale;
                            }

                            // create geometry
                            osg::Geometry* geom = new osg::Geometry;
                            hudImgElements[elementId] =  geom;

                            //set the position
                            //find the referenceObj bounding box
                            osg::BoundingBox refObjBb = getBoundigBoxFromWidgetName(positionRefObj);

                            //get object bounding box
                            osg::BoundingBox myObjBb;
                            myObjBb.expandBy(osg::Vec3(0.0f,0.0f,0.0f));
                            myObjBb.expandBy(osg::Vec3(imgWidth,imgHeight,0.0f));

                            //calculate the positioning
                            osg::Vec3 position = calculatePosition(myObjBb,positionMyPoint,refObjBb,positionRefObjPoint, positionVerticalModifier, positionHorizontalModifier);

                            //asign the position
                            float positionLeft =   position.x();
                            float positionBottom = position.y();

                            //create the vertices for the image geometry and assign them
                            osg::Vec3Array* vertices = new osg::Vec3Array;
                            float depth = 0.0f-0.1f;
                            vertices->push_back(osg::Vec3( positionLeft            ,positionBottom           ,depth)); //bottom left
                            vertices->push_back(osg::Vec3( positionLeft+imgWidth   ,positionBottom           ,depth)); //bottom right
                            vertices->push_back(osg::Vec3( positionLeft+imgWidth   ,positionBottom+imgHeight ,depth)); //top right
                            vertices->push_back(osg::Vec3( positionLeft            ,positionBottom+imgHeight ,depth)); //topleft
                            geom->setVertexArray(vertices);

                            // texture the geometry and apply material
                            // calculate textures coordinates
                            osg::Vec2Array* texcoords = new osg::Vec2Array(4);
                            (*texcoords)[0].set(0.0f, 0.0f);
                            (*texcoords)[1].set(1.0f, 0.0f);
                            (*texcoords)[2].set(1.0f, 1.0f);
                            (*texcoords)[3].set(0.0f, 1.0f);
                            geom->setTexCoordArray(0,texcoords);

                            // calculate normals
                            osg::Vec3Array* normals = new osg::Vec3Array(1);
                            (*normals)[0].set(0.0f,-1.0f,0.0f);
                            geom->setNormalArray(normals);
                            geom->setNormalBinding(osg::Geometry::BIND_OVERALL);

                            // assign colors
                            osg::Vec4Array* colors = new osg::Vec4Array(1);
                            (*colors)[0].set(1.0f,1.0f,1.0f,1.0f);
                            geom->setColorArray(colors);
                            geom->setColorBinding(osg::Geometry::BIND_OVERALL);

                            // draw the vertices as quads
                            geom->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

                            // disable display list so our modified tex coordinates show up
                            geom->setUseDisplayList(false);

                            // setup texture
                            osg::TextureRectangle* texture = new osg::TextureRectangle;

                            texture->setImage(img);

                            // setup stateset
                            osg::StateSet* state = geom->getOrCreateStateSet();
                            state->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

                            //Conversely, disable write depth cache,
                            //Make objects behind transparent polygons visible
                            //OSG draws transparent polygons first, then draws opaque polygons
                            osg :: Depth * imgDepth = new osg :: Depth;
                            imgDepth-> setWriteMask (false);
                            state-> setAttributeAndModes (imgDepth, osg :: StateAttribute :: ON);

                            // setup material
                            osg::TexMat* texmat = new osg::TexMat;
                            texmat->setScaleByTextureRectangleSize(true);
                            state->setTextureAttributeAndModes(0, texmat, osg::StateAttribute::ON);

                            //enable gl_blending (for texture transparency)
                            state->setMode(GL_BLEND, osg::StateAttribute::ON);
                            osg::BlendFunc* blend = new osg::BlendFunc;
                            blend->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_DST_ALPHA);

                            // turn off lighting (light always on)
                            state->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

                            if (type == "image")
                            {
                                //add the image geometry to the hud
                                geode->addDrawable(geom);
                            }
                            else if(type == "imagerotable")
                            {
                                osg::MatrixTransform* transform = new osg::MatrixTransform;
                                const double angle = 0.0;
                                const osg::Vec3d axis(0, 0, 1);
                                transform->setMatrix(osg::Matrix::rotate(angle, axis));
                                transform->addChild(geom);
                                osgGroupHud->addChild(transform);
                                hudImgRotableElements[elementId] =  transform;
                                if(widgetEnabled>0){
                                    hudImgRotableElements["driverinput-wheel"]->setNodeMask(1);
                                }else{
                                    hudImgRotableElements["driverinput-wheel"]->setNodeMask(0);
                                }
                            }
                        }
                        else if( type == "imagevertex")
                        {
                            /* ============================
                                 CREATE OSG IMAGEVERTEX
                               ============================*/
                            //read data into local variables
                            const std::string &elementId =      subSectionName;
                            std::string url =                   GfParmGetStr (paramHandle, subSectionPath.c_str(),"url", "" );
                            std::string positionRefObj =        GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObj", "" );
                            std::string positionRefObjPoint =   GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObjPoint", "tl" );
                            std::string positionMyPoint =       GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-myPoint", "tl" );
                            float positionVerticalModifier =    GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-verticalModifier", "",0 ) * hudScale;
                            float positionHorizontalModifier =  GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-horizontalModifier", "",0 ) * hudScale;

                            GfLogDebug("OSGHUD: Generate imagevertex object: %s \n", elementId.c_str());

                            osg::ref_ptr<osg::Geode>      myGeode = new osg::Geode;
                            osg::ref_ptr<osg::Geometry>   myGeometry = new osg::Geometry;
                            osg::ref_ptr<osg::Vec3Array>  myVertices = new osg::Vec3Array;
                            osg::ref_ptr<osg::Vec3Array>  myNormals = new osg::Vec3Array;
                            osg::ref_ptr<osg::Vec2Array>  myTexCoords = new osg::Vec2Array; //(UV)

                            //start preparing the image
                            std::string myFilename = GetDataDir();
                            myFilename.append(url);

                            //check that the image file exist
                            if (!GfFileExists(myFilename.c_str()))
                            {
                                GfLogError ("OSGHUD: Specified image file does not exist: %s.\n", myFilename.c_str());
                            }

                            // setup texture
                            osg::Image* myImg = osgDB::readImageFile(myFilename);
                            osg::TextureRectangle* myTexture = new osg::TextureRectangle;
                            myTexture->setImage(myImg);

                            //get image dimensions
                            float width = myImg->s();
                            float height = myImg->t();

                            //set the position
                            //find the referenceObj bounding box
                            osg::BoundingBox refObjBb = getBoundigBoxFromWidgetName(positionRefObj);

                            //get object bounding box
                            osg::BoundingBox myObjBb;
                            myObjBb.expandBy(osg::Vec3(0.0f,0.0f,0.0f));
                            myObjBb.expandBy(osg::Vec3(width,height,0.0f));

                            //calculate the positioning
                            osg::Vec3 position = calculatePosition(myObjBb,positionMyPoint,refObjBb,positionRefObjPoint, positionVerticalModifier, positionHorizontalModifier);

                            //asign the position
                            float positionLeft =   position.x();
                            float positionBottom = position.y();

                            //vertices
                            //triangle 1
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(0.0*width+positionLeft,  0.5*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(0.0*width+positionLeft,  1.0*height+positionBottom,  1.0)); //3
                            //triangle 2
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(0.0*width+positionLeft,  1.0*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  1.0*height+positionBottom,  1.0)); //3
                            //triangle 3
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  1.0*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(1.0*width+positionLeft,  1.0*height+positionBottom,  1.0)); //3
                            //triangle 4
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(1.0*width+positionLeft,  1.0*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(1.0*width+positionLeft,  0.5*height+positionBottom,  1.0)); //3
                            //triangle 5
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(1.0*width+positionLeft,  0.5*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(1.0*width+positionLeft,  0.0*height+positionBottom,  1.0)); //3
                            //triangle 6
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(1.0*width+positionLeft,  0.0*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.0*height+positionBottom,  1.0)); //3
                            //triangle 7
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.0*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(0.0*width+positionLeft,  0.0*height+positionBottom,  1.0)); //3
                            //triangle 8
                            myVertices->push_back ( osg::Vec3(0.5*width+positionLeft,  0.5*height+positionBottom,  1.0)); //1
                            myVertices->push_back ( osg::Vec3(0.0*width+positionLeft,  0.0*height+positionBottom,  1.0)); //2
                            myVertices->push_back ( osg::Vec3(0.0*width+positionLeft,  0.5*height+positionBottom,  1.0)); //3

                            myGeometry->setVertexArray  ( myVertices  );

                            //UV's
                            //triangle 1
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(0.0,  0.5)); //2
                            myTexCoords->push_back( osg::Vec2(0.0,  1.0)); //3
                            //triangle 2
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(0.0,  1.0)); //2
                            myTexCoords->push_back( osg::Vec2(0.5,  1.0)); //3
                            //triangle 3
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(0.5,  1.0)); //2
                            myTexCoords->push_back( osg::Vec2(1.0,  1.0)); //3
                            //triangle 4
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(1.0,  1.0)); //2
                            myTexCoords->push_back( osg::Vec2(1.0,  0.5)); //3
                            //triangle 5
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(1.0,  0.5)); //2
                            myTexCoords->push_back( osg::Vec2(1.0,  0.0)); //3
                            //triangle 6
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(1.0,  0.0)); //2
                            myTexCoords->push_back( osg::Vec2(0.5,  0.0)); //3
                            //triangle 7
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(0.5,  0.0)); //2
                            myTexCoords->push_back( osg::Vec2(0.0,  0.0)); //3
                            //triangle 8
                            myTexCoords->push_back( osg::Vec2(0.5,  0.5)); //1
                            myTexCoords->push_back( osg::Vec2(0.0,  0.0)); //2
                            myTexCoords->push_back( osg::Vec2(0.0,  0.5)); //3

                            myGeometry->setTexCoordArray( 0, myTexCoords );

                            // calculate normals
                            osg::Vec3Array* normals = new osg::Vec3Array(8);
                            (*normals)[0].set(0.0f,-1.0f,0.0f);
                            (*normals)[1].set(0.0f,-1.0f,0.0f);
                            (*normals)[2].set(0.0f,-1.0f,0.0f);
                            (*normals)[3].set(0.0f,-1.0f,0.0f);
                            (*normals)[4].set(0.0f,-1.0f,0.0f);
                            (*normals)[5].set(0.0f,-1.0f,0.0f);
                            (*normals)[6].set(0.0f,-1.0f,0.0f);
                            (*normals)[7].set(0.0f,-1.0f,0.0f);
                            myGeometry->setNormalArray(normals);
                            myGeometry->setNormalBinding(osg::Geometry::BIND_OVERALL);

                            // assign colors
                            osg::Vec4Array* colors = new osg::Vec4Array(8);
                            (*colors)[0].set(1.0f,1.0f,1.0f,1.0f);
                            (*colors)[1].set(1.0f,1.0f,1.0f,1.0f);
                            (*colors)[2].set(1.0f,1.0f,1.0f,1.0f);
                            (*colors)[3].set(1.0f,1.0f,1.0f,1.0f);
                            (*colors)[4].set(1.0f,1.0f,1.0f,1.0f);
                            (*colors)[5].set(1.0f,1.0f,1.0f,1.0f);
                            (*colors)[6].set(1.0f,1.0f,1.0f,1.0f);
                            (*colors)[7].set(1.0f,1.0f,1.0f,1.0f);
                            myGeometry->setColorArray(colors);
                            myGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

                            // draw the vertices as quads
                            myGeometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, myVertices->size()));

                            // disable display list so our modified tex coordinates show up
                            myGeometry->setUseDisplayList(false);

                            // setup stateset
                            osg::StateSet* myState = myGeometry->getOrCreateStateSet();
                            myState->setTextureAttributeAndModes(0, myTexture, osg::StateAttribute::ON);

                            //Conversely, disable write depth cache,
                            //Make objects behind transparent polygons visible
                            //OSG draws transparent polygons first, then draws opaque polygons
                            osg :: Depth * myImgDepth = new osg :: Depth;
                            myImgDepth-> setWriteMask (false);
                            myState-> setAttributeAndModes (myImgDepth, osg :: StateAttribute :: ON);

                            // setup material
                            osg::TexMat* myTexmat = new osg::TexMat;
                            myTexmat->setScaleByTextureRectangleSize(true);
                            myState->setTextureAttributeAndModes(0, myTexmat, osg::StateAttribute::ON);

                            //enable gl_blending (for texture transparency)
                            myState->setMode(GL_BLEND, osg::StateAttribute::ON);
                            osg::BlendFunc* myBlend = new osg::BlendFunc;
                            myBlend->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_DST_ALPHA);

                            // turn off lighting (light always on)
                            myState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

                            //myGeode->setName("mytachograph");
                            myGeode->addDrawable( myGeometry );
                            osgGroupWidgets->addChild(myGeode);
                            hudImgVertexElements[elementId] =  myGeometry;
                        }
                        else if( type == "graph")
                        {
                            /* ============================
                                 CREATE OSG GRAPH
                               ============================*/
                            //read data into local variables
                            const std::string &elementId = subSectionName;

                            //positioning variables
                            std::string positionRefObj =       GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObj", "" );
                            std::string positionRefObjPoint =  GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-refObjPoint", "tl" );
                            std::string positionMyPoint =      GfParmGetStr (paramHandle, subSectionPath.c_str(),"position-myPoint", "tl" );
                            float positionVerticalModifier =   GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-verticalModifier", "",0 ) * hudScale;
                            float positionHorizontalModifier = GfParmGetNum (paramHandle, subSectionPath.c_str(),"position-horizontalModifier", "",0 ) * hudScale;

                            //graph variables
                            float width =                      GfParmGetNum (paramHandle, subSectionPath.c_str(),"width", "",0 ) * hudScale;
                            float height =                     GfParmGetNum (paramHandle, subSectionPath.c_str(),"height", "",0 ) * hudScale;
                            std::string title =                GfParmGetStr (paramHandle, subSectionPath.c_str(),"title", "" );

                            // line variables
                            std::vector<OSGPLOT::PlotLineConfig> lines;
#if _MSC_VER &&_MSC_VER < 1700
                            long long lineNum = 1;
#else
                            int lineNum = 1;
#endif

                            //get a list of the sections in this section
                            std::string linesSectionPath = subSectionPath + "/lines/";
                            std::string lineSectionPath = linesSectionPath + "1";

                            //cicle throught each element of the widget
                            while (GfParmExistsSection(paramHandle, lineSectionPath.c_str()))
                            {
                                 OSGPLOT::PlotLineConfig config;

                                 config.reference = GfParmGetNum(paramHandle, lineSectionPath.c_str(), "reference", "", 0) != 0;
                                 config.maxValue = GfParmGetNum(paramHandle, lineSectionPath.c_str(), "maxValue", "", 0);
                                 config.minValue = GfParmGetNum(paramHandle, lineSectionPath.c_str(), "minValue", "", 0);
                                 config.timeFrame = GfParmGetNum(paramHandle, lineSectionPath.c_str(), "timeFrame", "", 0);
                                 config.referenceLineAtValue = GfParmGetNum(paramHandle, lineSectionPath.c_str(), "referenceLineAtValue", "", 0);
                                 config.Xdata = GfParmGetStr(paramHandle, lineSectionPath.c_str(), "Xdata", "");
                                 config.Ydata = GfParmGetStr(paramHandle, lineSectionPath.c_str(), "Ydata", "");
                                 config.color = colorStringToVec4(GfParmGetStr(paramHandle, lineSectionPath.c_str(), "color", ""));

                                 lines.push_back(config);
                                 lineSectionPath = linesSectionPath + std::to_string(++lineNum);
                            }

                            GfLogDebug("OSGHUD: Generate graph object: %s \n", elementId.c_str());

                            //calculate position
                            //find the referenceObj bounding box
                            osg::BoundingBox refObjBb = getBoundigBoxFromWidgetName(positionRefObj);

                            //calculate our bounding box
                            osg::BoundingBox plotBB;
                            plotBB.expandBy(osg::Vec3(0.0f,0.0f,0.0f));
                            plotBB.expandBy(osg::Vec3(width,height,0.0f));

                              //calculate the positioning
                            osg::Vec3 position = calculatePosition(plotBB,positionMyPoint,refObjBb,positionRefObjPoint, positionVerticalModifier, positionHorizontalModifier);

                            float positionX = position.x();
                            float positionY = position.y();

                            //istantiate the graph
                            hudGraphElements[elementId] = new OSGPLOT(positionX, positionY, width, height, title, lines);
                            geode->addChild(hudGraphElements[elementId]->getGroup());
                        }
                        else
                        {
                        /* ============================
                             NO MORE HUD TYPE OPTIONS...??
                           ============================*/
                            GfLogDebug("OSG-HUD object type not recognized: %s", type.c_str());
                        }

                } while (GfParmListSeekNext(paramHandle, widgetsSectionPath.c_str()) == 0);
            }

            //only display the widget element if the widget is enabled
            if(widgetEnabled > 0){
                geode->setNodeMask(1);
            }else{
                geode->setNodeMask(0);
            }

        } while (GfParmListSeekNext(paramHandle, mainSection.c_str()) == 0);
    }

    //release the xml file
    GfParmReleaseHandle(paramHandle);

    //return the hud object
    return (*osgGroupHud).asGroup();
}

osg::BoundingBox SDHUD::getBoundigBoxFromWidgetName(std::string widgetName)
{
    //find the referenceObj pointer and then get his bounding box
    osg::BoundingBox widgetBoundingBox;
    if ( hudTextElements.find(widgetName) != hudTextElements.end() )
    {
        widgetBoundingBox = hudTextElements[widgetName]->getBoundingBox();
    }
    else if ( hudImgElements.find(widgetName) != hudImgElements.end() )
    {
        widgetBoundingBox = hudImgElements[widgetName]->getBoundingBox();
    }

    else if ( hudGraphElements.find(widgetName) != hudGraphElements.end() )
    {
        //widgetBoundingBox = hudGraphElements[positionRefObj]->getBoundingBox();
    }

    else
    {
        GfLogDebug("OSGHUD: No (valid) reference object given for the current element alignement: Assuming Screen!\n");
        osg::BoundingBox screenBB;
        screenBB.expandBy(osg::Vec3(0.0f,0.0f,0.0f));
        screenBB.expandBy(osg::Vec3(hudScreenW,hudScreenH,0.0f));
        widgetBoundingBox = screenBB;
    }
    return widgetBoundingBox;
}

osg::BoundingBox SDHUD::getBoundigBoxFromWidgetGroupName(std::string widgetGroupName)
{
    osg::BoundingBox widgetGroupBoundingBox;
    std::string widgetGroupNamePath = "widgets/"+widgetGroupName;

    //open the osghud config file file
    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);

    //cicle throught each element of the widgetGroup
    if (GfParmListSeekFirst(paramHandle, widgetGroupNamePath.c_str()) == 0)
    {
        do
        {
            std::string widgetsName = GfParmListGetCurEltName(paramHandle,widgetGroupNamePath.c_str());

            //get the bounding box of the single widget
            osg::BoundingBox widgetBoundingBox = getBoundigBoxFromWidgetName(widgetsName);

            //add the single widget bounding box to the widgetgroup bounding box
            widgetGroupBoundingBox.expandBy(widgetBoundingBox);
        } while (GfParmListSeekNext(paramHandle, widgetGroupNamePath.c_str()) == 0);
    }

    //release the config file
    GfParmReleaseHandle(paramHandle);

    return widgetGroupBoundingBox;
}
void SDHUD::recalculateImageWidgetPosition(std::string widgetGroupName, std::string widgetName, float hudScale)
{
    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);
    std::string imageWidgetPath = "widgets/"+widgetGroupName +"/"+widgetName;
    osg::Geometry* imageWidget = hudImgElements[widgetName];

    std::string positionRefObj =        GfParmGetStr (paramHandle, imageWidgetPath.c_str(),"position-refObj", "" );
    std::string positionRefObjPoint =   GfParmGetStr (paramHandle, imageWidgetPath.c_str(),"position-refObjPoint", "tl" );
    std::string positionMyPoint =       GfParmGetStr (paramHandle, imageWidgetPath.c_str(),"position-myPoint", "tl" );
    float positionVerticalModifier =    GfParmGetNum (paramHandle, imageWidgetPath.c_str(),"position-verticalModifier", "",0 ) * hudScale;
    float positionHorizontalModifier =  GfParmGetNum (paramHandle, imageWidgetPath.c_str(),"position-horizontalModifier", "",0 ) * hudScale;

    GfParmReleaseHandle(paramHandle);

    osg::TextureRectangle* texture;

    //get the texture data of this object
    texture = dynamic_cast<osg::TextureRectangle*>(imageWidget->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXTURE));

    //get the image from the texture data
    osg::Image* img;
    img = texture->getImage();

    //get image dimensions
    float imgWidth = img->s() * hudScale;
    float imgHeight = img->t() * hudScale;

    //set the position
    osg::BoundingBox refObjBb = getBoundigBoxFromWidgetName(positionRefObj);

    //get object bounding box
    osg::BoundingBox myObjBb = imageWidget->getBoundingBox();

    //calculate the positioning
    osg::Vec3 position = calculatePosition(myObjBb,positionMyPoint,refObjBb,positionRefObjPoint, positionVerticalModifier, positionHorizontalModifier);

    //asign the position
    float positionLeft =   position.x();
    float positionBottom = position.y();

    //create the vertices for the image geometry and assign them
    osg::Vec3Array* vertices = new osg::Vec3Array;
    float depth = 0.0f-0.1f;

    vertices->push_back(osg::Vec3( positionLeft            ,positionBottom           ,depth)); //bottom left
    vertices->push_back(osg::Vec3( positionLeft+imgWidth   ,positionBottom           ,depth)); //bottom right
    vertices->push_back(osg::Vec3( positionLeft+imgWidth   ,positionBottom+imgHeight ,depth)); //top right
    vertices->push_back(osg::Vec3( positionLeft            ,positionBottom+imgHeight ,depth)); //topleft
    vertices->dirty();
    imageWidget->setVertexArray(vertices);
    imageWidget->setUseDisplayList(false);
}

void SDHUD::recalculateTextWidgetPosition(std::string widgetGroupName, std::string widgetName, float hudScale)
{

    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);
    std::string textWidgetPath = "widgets/"+widgetGroupName +"/"+widgetName;
    osgText::Text* textWidget = hudTextElements[widgetName];


    std::string positionRefObj =        GfParmGetStr (paramHandle, textWidgetPath.c_str(),"position-refObj", "" );
    std::string positionRefObjPoint =   GfParmGetStr (paramHandle, textWidgetPath.c_str(),"position-refObjPoint", "tl" );
    std::string positionMyPoint =       GfParmGetStr (paramHandle, textWidgetPath.c_str(),"position-myPoint", "tl" );
    float positionVerticalModifier =    GfParmGetNum (paramHandle, textWidgetPath.c_str(),"position-verticalModifier", "",0 ) * hudScale;
    float positionHorizontalModifier =  GfParmGetNum (paramHandle, textWidgetPath.c_str(),"position-horizontalModifier", "",0 ) * hudScale;

    GfParmReleaseHandle(paramHandle);

    //set the position
    osg::BoundingBox refObjBb = getBoundigBoxFromWidgetName(positionRefObj);

    //get object bounding box
    osg::BoundingBox myObjBb = textWidget->getBoundingBox();

    //calculate the positioning
    osg::Vec3 position = calculatePosition(myObjBb,positionMyPoint,refObjBb,positionRefObjPoint, positionVerticalModifier, positionHorizontalModifier);

    textWidget->setPosition(position);
}

void SDHUD::saveWidgetGroupPosition(std::string widgetGroupName)
{
    GfLogInfo("Saving position: %s\n", widgetGroupName.c_str());

    std::string widgetGroupNamePath = "widgets/"+widgetGroupName;

    //open the osghud config file file
    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);

    //cicle throught each element of the widgetGroup
    if (GfParmListSeekFirst(paramHandle, widgetGroupNamePath.c_str()) == 0)
    {
        do
        {
            std::string widgetsName = GfParmListGetCurEltName(paramHandle,widgetGroupNamePath.c_str());
            std::string widgetPath = widgetGroupNamePath +"/"+ widgetsName;

            //leggo la posizione dal file
            std::string positionRefObj =        GfParmGetStr (paramHandle, widgetPath.c_str(),"position-refObj", "" );
            std::string positionRefObjPoint =   GfParmGetStr (paramHandle, widgetPath.c_str(),"position-refObjPoint", "tl" );
            std::string positionMyPoint =       GfParmGetStr (paramHandle, widgetPath.c_str(),"position-myPoint", "tl" );
            float positionVerticalModifier =    GfParmGetNum (paramHandle, widgetPath.c_str(),"position-verticalModifier", "",0 );
            float positionHorizontalModifier =  GfParmGetNum (paramHandle, widgetPath.c_str(),"position-horizontalModifier", "",0 );

            if ( positionRefObj.find("screen") == 0 ){

                //convert the modification to the resolution used in the config file
                float modifierVertical = ((float)mouseTotalDragY / (float)hudScreenH * 1024);
                float modifierHorizontal = ((float)mouseTotalDragX / (float)hudScreenW * 1280);

                // I still dont completely understand the math or the reasons for this, but "it works"
                float horizontalScale = (((float)hudScreenW / (float)hudScreenH) / (1280.00/1024.00));

                modifierHorizontal = modifierHorizontal * horizontalScale;

                float newPositionVerticalModifier = positionVerticalModifier + modifierVertical;
                float newPositionHorizontalModifier = positionHorizontalModifier + modifierHorizontal;

                GfLogInfo("OSGHUD: Hud Scale is: %f\n", hudScale);

                GfLogInfo("Resolution: %f %f\n", (float)hudScreenW, (float)hudScreenH);
                GfLogInfo("Horizontal Scale: %f\n", horizontalScale);

                GfLogInfo("Original Horizontal was: %f\n", positionHorizontalModifier);

                GfLogInfo("Original Vertical was: %f\n", positionVerticalModifier);
                GfLogInfo("Original Horizontal was: %f\n", positionHorizontalModifier);

                GfLogInfo("Mouse drag was Vertical: %f\n", (float)mouseTotalDragY);
                GfLogInfo("Mouse drag was Horizontal: %f\n", (float)mouseTotalDragX);

                GfLogInfo("Modifier Vertical by: %f\n", modifierVertical);
                GfLogInfo("Modifier Horizontal by: %f\n", modifierHorizontal);

                GfLogInfo("Modified Vertical is: %f\n", newPositionVerticalModifier);
                GfLogInfo("Modified Horizontal is: %f\n", newPositionHorizontalModifier);

                //save the value back in the config file
                GfParmSetStr (paramHandle, widgetPath.c_str(), "position-refObjPoint", positionRefObjPoint.c_str() );
                GfParmSetStr (paramHandle, widgetPath.c_str(), "position-myPoint", positionMyPoint.c_str() );
                GfParmSetNum (paramHandle, widgetPath.c_str(), "position-verticalModifier", NULL, (int)newPositionVerticalModifier);
                GfParmSetNum (paramHandle, widgetPath.c_str(), "position-horizontalModifier", NULL, (int)newPositionHorizontalModifier);
                GfParmWriteFile(NULL, paramHandle, "osghudconfig");
            }
        } while (GfParmListSeekNext(paramHandle, widgetGroupNamePath.c_str()) == 0);
    }

    //release the config file
    GfParmReleaseHandle(paramHandle);
}



bool SDHUD::isMouseOverWidgetGroup (std::string widgetGroupName)
{
    tMouseInfo    *mouse;
    mouse = GfuiMouseInfo();
    osg::BoundingBox targetWidgetGroupBoundingBox = getBoundigBoxFromWidgetGroupName(widgetGroupName);

    float mousePosX = mouse->X * hudScreenW /640;
    float mousePosY = mouse->Y * hudScreenH /480;

    if (mousePosX >= targetWidgetGroupBoundingBox.xMin() && mousePosX <= targetWidgetGroupBoundingBox.xMax()
    && mousePosY >= targetWidgetGroupBoundingBox.yMin() && mousePosY <= targetWidgetGroupBoundingBox.yMax()){
        return true;
    }else{
        return false;
    }
}
void SDHUD::selectWidgetGroupByName(std::string widgetGroupName)
{
    //remember the selection
    selectedWidgetGroup = widgetGroupName;
    std::string selectedWidgetGroupPath = "widgets/"+widgetGroupName;

    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);
    int widgetEnabled = GfParmGetNum (paramHandle, selectedWidgetGroupPath.c_str(), "enabled", "",0);

    //set the edithud title according to the selection
    hudTextElements["edithud-titletext"]->setText(widgetGroupName);
    hudImgElements["edithud-toggleoff"]->setNodeMask(!widgetEnabled);
    hudImgElements["edithud-toggleon"]->setNodeMask(widgetEnabled);

    //release the config file
    GfParmReleaseHandle(paramHandle);
}
void SDHUD::setWidgetsGroupsVisibilityForcedON()
{
    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);

    //cicle throught each element of the widgetGroup
    if (GfParmListSeekFirst(paramHandle, "widgets") == 0)
    {
        do
        {
            std::string widgetGroupName = GfParmListGetCurEltName(paramHandle,"widgets");
            if (widgetGroupName.find("edithudWidget")!=std::string::npos){
                continue;
            }
            if (widgetGroupName.find("mouseWidget")!=std::string::npos){
                continue;
            }
            hudWidgets[widgetGroupName]->setNodeMask(1);

        } while (GfParmListSeekNext(paramHandle, "widgets") == 0);
    }
    //release the config file
    GfParmReleaseHandle(paramHandle);
}
void SDHUD::setWidgetsGroupsVisibilityNormal()
{
    void *paramHandle = GfParmReadFileLocal("config/osghudconfig.xml", GFPARM_RMODE_STD);

    //cicle throught each element of the widgetGroup
    if (GfParmListSeekFirst(paramHandle, "widgets") == 0)
    {
        do
        {
            std::string widgetGroupName = GfParmListGetCurEltName(paramHandle,"widgets");
            if (widgetGroupName.find("edithudWidget")!=std::string::npos){
                continue;
            }
            if (widgetGroupName.find("mouseWidget")!=std::string::npos){
                continue;
            }
            std::string selectedWidgetGroupPath = "widgets/"+widgetGroupName;
            int widgetEnabled = GfParmGetNum (paramHandle, selectedWidgetGroupPath.c_str(), "enabled", "",0);
            hudWidgets[widgetGroupName]->setNodeMask(widgetEnabled);

        } while (GfParmListSeekNext(paramHandle, "widgets") == 0);
    }
    //release the config file
    GfParmReleaseHandle(paramHandle);
}

void SDHUD::ToggleHUDeditmode()
{
    //toggle the edit mode status
    hudEditModeEnabled = !hudEditModeEnabled;
    GfLogInfo("OSGHUD editmode toggled to %i \n", hudEditModeEnabled);

    if (hudEditModeEnabled){
        //we are entering edithud mode
        //force all widgets to be visible, even if disabled
        setWidgetsGroupsVisibilityForcedON();
        //display the mouse pointer
        hudWidgets["mouseWidget"]->setNodeMask(1);
    }else{
        //we are going back to normal game mode
        //restore normal widgets visibility (visible if enabled or invisible if disabled)
        setWidgetsGroupsVisibilityNormal();
        //hide the mouse pointer
        hudWidgets["mouseWidget"]->setNodeMask(0);
        //hide the edithud widget
        hudWidgets["edithudWidget"]->setNodeMask(0);
    }
}

SDHUD::~SDHUD()
{
    for (std::map<std::string, OSGPLOT * >::iterator it = hudGraphElements.begin(); it != hudGraphElements.end(); ++it)
        delete(it->second);
}

} // namespace osggraph
