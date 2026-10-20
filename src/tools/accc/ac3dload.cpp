/***************************************************************************

 file        : ac3dload.cpp
 created     : Fri Apr 18 23:00:28 CEST 2003
 copyright   : (C) 2003 by Christophe Guionneau

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file

 @author Christophe Guionneau
 */

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cmath>
#include <cfloat>
#ifndef _WIN32
// std::isnan should be available as of C++11 (201103L) but some compiler
// vendors set this even though support is incomplete
#if __cplusplus>=201402L // compiler claims to be C++14 compliant
#define isnan std::isnan
#endif
#endif // _WIN32

#include "accc.h"

#if _MSC_VER && _MSC_VER < 1700
#define to_string(x) to_string(static_cast<long long>(x))
#endif

#define AC3D     "AC3Db"
#define MATERIAL "MATERIAL"
#define OBJECT   "OBJECT"
#define KIDS     "Kids"
#define NAME     "name"
#define LOC      "loc"
#define DATA     "data"
#define TEXTURE  "texture"
#define TEXREP   "texrep"
#define NUMVERT  "numvert"
#define NUMSURF  "numsurf"
#define SURF     "SURF"
#define MAT      "mat"
#define REFS     "refs"
#define CREASE   "crease"
#define SUBDIV   "subdiv"
#define FOLDED   "folded"
#define HIDDEN   "hidden"
#define LOCKED   "locked"

ob_t::ob_t() :
kids(0),
loc(0.0, 0.0, 0.0),
attrSurf(0),
attrMat(0),
texrep_x(0.0),
texrep_y(0.0),
numvert(0),
numsurf(0),
numvertice(0),
x_min(0.0),
y_min(0.0),
z_min(0.0),
x_max(0.0),
y_max(0.0),
z_max(0.0),
dist_min(0.0),
saved(false),
kids_o(0),
inkids_o(false)
{
}

ob_t::~ob_t()
{
}

void ob_t::initSpacialExtend()
{
    x_min = y_min = z_min = DBL_MAX;
    x_max = y_max = z_max = DBL_MIN;

    for (int v = 0; v < numvertice; v++)
    {
        if (vertex[v].x > x_max)
            x_max = vertex[v].x;
        if (vertex[v].x < x_min)
            x_min = vertex[v].x;

        if (vertex[v].y > y_max)
            y_max = vertex[v].y;
        if (vertex[v].y < y_min)
            y_min = vertex[v].y;

        if (vertex[v].z > z_max)
            z_max = vertex[v].z;
        if (vertex[v].z < z_min)
            z_min = vertex[v].z;
    }
}

void ob_t::createTextArrays()
{
    if (!vertexarray.empty())
    {
        textarray.resize(numvertice);
        for (int i = 0; i < numsurf * 3; i++)
            textarray[vertexarray[i].indice] = vertexarray[i].uv;
    }

    if (!vertexarray1.empty())
    {
        textarray1.resize(numvertice);
        for (int i = 0; i < numsurf * 3; i++)
            textarray1[vertexarray1[i].indice] = vertexarray1[i].uv;
    }

    if (!vertexarray2.empty())
    {
        textarray2.resize(numvertice);
        for (int i = 0; i < numsurf * 3; i++)
            textarray2[vertexarray2[i].indice] = vertexarray2[i].uv;
    }

    if (!vertexarray3.empty())
    {
        textarray3.resize(numvertice);
        for (int i = 0; i < numsurf * 3; i++)
            textarray3[vertexarray3[i].indice] = vertexarray3[i].uv;
    }
}

void ob_t::createVertexArrays()
{
    int numEls = numsurf * 3;

    if (hasTexture())
        vertexarray.resize(numEls);

    if (hasTexture1())
        vertexarray1.resize(numEls);

    if (hasTexture2())
        vertexarray2.resize(numEls);

    if (hasTexture3())
        vertexarray3.resize(numEls);
}

void ob_t::copyTextureNames(const ob_t &srcob)
{
    texture = srcob.texture;
    texture1 = srcob.texture1;
    texture2 = srcob.texture2;
    texture3 = srcob.texture3;
}

void ob_t::setVertexArraysIndex(int vaIdx, int newIndex)
{
    vertexarray[vaIdx].indice = newIndex;

    if (!vertexarray1.empty())
        vertexarray1[vaIdx].indice = newIndex;
    if (!vertexarray2.empty())
        vertexarray2[vaIdx].indice = newIndex;
    if (!vertexarray3.empty())
        vertexarray3[vaIdx].indice = newIndex;
}

void ob_t::computeObSurfCentroid(int obsurf, point_t &out) const
{
    int firstIdx = obsurf * 3;

    out.set(0, 0, 0);

    for (int curVert = 0; curVert < 3; curVert++)
        out += vertex[vertexarray[firstIdx + curVert].indice];

    out /= 3;
}

#ifndef M_PI
#define M_PI 3.14159267
#endif
void computeTriNorm(std::list<ob_t> &objects);
void computeObjectTriNorm(ob_t &object);
void smoothFaceTriNorm(ob_t &object);
void smoothObjectTriNorm(ob_t &object);
void normalize(point_t &t);
bool checkMustSmoothVector(point_t &n1, point_t &n2, point_t &t1, point_t &t2);
void mapNormalToSphere(std::list<ob_t> &objects);
void mapNormalToSphere2(std::list<ob_t> &objects);
void normalMap(std::list<ob_t> &objects);
void mapTextureEnv(std::list<ob_t> &objects);
point_t tmpPoint[500000];
tcoord_t tmpva[500000];
uv_t tmptexa[500000];
int refs = 0;
const char * const shadowtexture = "shadow2.png";

//int numob = 0;
int nummaterial = 0;
int numvertex = 0;
int numsurf = 0;
int dataSize = 0;
int dataSizeRead = 0;
bool numvertFound = false;
bool numrefsFound = false;
bool dataFound = false;
unsigned int attrSurf = 0;
int attrMat = 0;
int numrefs = 0;
int numrefstotal = 0;
char tmpname[256];
int tmpIndice = 0;
int tmpIndice2 = 0;
int tmpIndice3 = 0;
int vert;
int numvertice = 0;
char tex[256][256];
int texnum = 0;
struct verbaction_t
{
    const char * verb;
    int (*doVerb)(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
};

int doMaterial(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doObject(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doKids(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doName(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doLoc(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doData(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doTexture(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doTexrep(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doNumvert(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doNumsurf(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doSurf(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doMat(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doRefs(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doCrease(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doSubdiv(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doFolded(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doHidden(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
int doLocked(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);

void computeSaveAC3D(const std::string & OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials);
void computeSaveOBJ(const std::string & OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials);
void computeSaveAC3DM(const std::string & OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials);
void computeSaveAC3DStrip(const std::string & OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials);
void stripifyOb(FILE * ofile, ob_t * object, bool writeit);

verbaction_t verbTab[] =
{
{ MATERIAL, doMaterial },
{ OBJECT, doObject },
{ KIDS, doKids },
{ NAME, doName },
{ LOC, doLoc },
{ DATA, doData },
{ TEXTURE, doTexture },
{ TEXREP, doTexrep },
{ NUMVERT, doNumvert },
{ NUMSURF, doNumsurf },
{ SURF, doSurf },
{ MAT, doMat },
{ REFS, doRefs },
{ CREASE, doCrease },
{ SUBDIV, doSubdiv },
{ FOLDED, doFolded },
{ HIDDEN, doHidden },
{ LOCKED, doLocked },
{ "END", NULL } };

/** Copies a single surface from the "vertexarray" attributes of srcob to the ones of destob.
 *  It decides whether to copy multitexture data based on srcob's "vertexarray" attributes.
 *
 *  In particular it copies 3 entries starting at srcSurfIdx * 3 from srcob->vertexarray
 *  to entries starting at destSurfIdx * 3 in destob->vertexarray. The same goes for the
 *  multitexture entries.
 */
void copyVertexArraysSurface(ob_t &destob, int destSurfIdx, const ob_t &srcob, int srcSurfIdx)
{
    const int firstDestIdx = destSurfIdx * 3;
    const int firstSrcIdx = srcSurfIdx * 3;

    for (int off = 0; off < 3; off++)
    {
        destob.vertexarray[firstDestIdx + off] = srcob.vertexarray[firstSrcIdx + off];

        if (!srcob.vertexarray1.empty())
            destob.vertexarray1[firstDestIdx + off] = srcob.vertexarray1[firstSrcIdx + off];
        if (!srcob.vertexarray2.empty())
            destob.vertexarray2[firstDestIdx + off] = srcob.vertexarray2[firstSrcIdx + off];
        if (!srcob.vertexarray3.empty())
            destob.vertexarray3[firstDestIdx + off] = srcob.vertexarray3[firstSrcIdx + off];
    }
}

/** copy the (u,v) coords from srcidxarr to the corresponding position in destarr.
 *  destarr needs to have 2 * number of vertices entries.
 */
void createTexCoordArray(uv_t * destarr, tcoord_t * srcidxarr, int numidx)
{
    tcoord_t * curidxobj = NULL;

    for (int curidx = 0; curidx < numidx; curidx++)
    {
        curidxobj = &srcidxarr[curidx];

        destarr[curidxobj->indice] = curidxobj->uv;
    }
}

/** Creates a new object from the given data. This function is used during object splitting.
 *
 *  @param splitid the id of this split object
 *  @param srcobj the original object, which was split
 *  @param tmpob the temporary storage object using in splitting
 *  @param retob the split object
 *
 *  In the tmpob the following should be set: numvertice, numsurf, vertex, snorm,
 *  vertexarray, textarray
 */
void createObjectSplitCopy(int splitid, const ob_t &srcobj, const ob_t &tmpob, ob_t &retob)
{
    /* special handling of name */
    retob.name = srcobj.name + "_s_" + std::to_string(splitid);

    retob.type = srcobj.type;
    retob.attrSurf = srcobj.attrSurf;
    retob.attrMat = srcobj.attrMat;
    retob.texture = srcobj.texture;
    retob.texture1 = srcobj.texture1;
    retob.texture2 = srcobj.texture2;
    retob.texture3 = srcobj.texture3;
    retob.data = srcobj.data;
    retob.numvert = tmpob.numvert;
    retob.numsurf = tmpob.numsurf;
    retob.numvertice = tmpob.numvertice;

    retob.vertex.resize(retob.numvert);
    std::copy_n(tmpob.vertex.begin(), retob.numvert, retob.vertex.begin());

    retob.norm.resize(retob.numvert);
    std::copy_n(tmpob.norm.begin(), retob.numvert, retob.norm.begin());

    retob.snorm.resize(retob.numvert);
    std::copy_n(tmpob.snorm.begin(), retob.numvert, retob.snorm.begin());

    if (srcobj.hasTexture())
    {
        retob.vertexarray.resize(retob.numvertice);
        retob.textarray.resize(retob.numvertice);
        std::copy_n(tmpob.vertexarray.begin(), retob.numvertice, retob.vertexarray.begin());
        std::copy_n(tmpob.textarray.begin(), retob.numvertice, retob.textarray.begin());
    }
    if (srcobj.hasTexture1())
    {
        retob.vertexarray1.resize(retob.numvertice);
        retob.textarray1.resize(retob.numvertice);
        std::copy_n(tmpob.vertexarray1.begin(), retob.numvertice, retob.vertexarray1.begin());
        std::copy_n(tmpob.textarray1.begin(), retob.numvertice, retob.textarray1.begin());
    }
    if (srcobj.hasTexture2())
    {
        retob.vertexarray2.resize(retob.numvertice);
        retob.textarray2.resize(retob.numvertice);
        std::copy_n(tmpob.vertexarray2.begin(), retob.numvertice, retob.vertexarray2.begin());
        std::copy_n(tmpob.textarray2.begin(), retob.numvertice, retob.textarray2.begin());
    }
    if (srcobj.hasTexture3())
    {
        retob.vertexarray3.resize(retob.numvertice);
        retob.textarray3.resize(retob.numvertice);
        std::copy_n(tmpob.vertexarray3.begin(), retob.numvertice, retob.vertexarray3.begin());
        std::copy_n(tmpob.textarray3.begin(), retob.numvertice, retob.textarray3.begin());
    }
}

void copyTexChannel(std::vector<uv_t> &desttextarray, std::vector<tcoord_t> &destvertexarray, const tcoord_t &srcvert,
    int storedptidx, int destptidx, int destvertidx)
{
    desttextarray[destptidx] = srcvert.uv;

    destvertexarray[destvertidx].set(storedptidx, srcvert.uv, false);
}

void copySingleVertexData(ob_t &destob, const ob_t &srcob,
    int storedptidx, int destptidx, int destvertidx, int srcvertidx)
{
    /* channel 0 */
    if (!destob.textarray.empty())
    {
        copyTexChannel(destob.textarray, destob.vertexarray, srcob.vertexarray[srcvertidx],
                storedptidx, destptidx, destvertidx);
    }

    /* channel 1 */
    if (!destob.textarray1.empty())
    {
        copyTexChannel(destob.textarray1, destob.vertexarray1, srcob.vertexarray1[srcvertidx],
                storedptidx, destptidx, destvertidx);
    }

    /* channel 2 */
    if (!destob.textarray2.empty())
    {
        copyTexChannel(destob.textarray2, destob.vertexarray2, srcob.vertexarray2[srcvertidx],
                storedptidx, destptidx, destvertidx);
    }

    /* channel 3 */
    if (!destob.textarray3.empty())
    {
        copyTexChannel(destob.textarray3, destob.vertexarray3, srcob.vertexarray3[srcvertidx],
                storedptidx, destptidx, destvertidx);
    }
}

int computeNorm(const point_t &pv1, const point_t &pv2, const point_t &pv3, point_t &norm)
{
    double p1, p2, p3, q1, q2, q3, dd;
    double x1, y1, z1, x2, y2, z2, x3, y3, z3;

    x1 = pv1.x;
    y1 = pv1.y;
    z1 = pv1.z;

    x2 = pv2.x;
    y2 = pv2.y;
    z2 = pv2.z;

    x3 = pv3.x;
    y3 = pv3.y;
    z3 = pv3.z;

    if (((x1 == x2) && (y1 == y2) && (z1 == z2)) ||
        ((x1 == x3) && (y1 == y3) && (z1 == z3)) ||
        ((x2 == x3) && (y2 == y3) && (z2 == z3)))
    {
        norm.set(0.0, 1.0, 0.0);
        return 0;
    }

    p1 = x2 - x1;
    p2 = y2 - y1;
    p3 = z2 - z1;

    q1 = x3 - x1;
    q2 = y3 - y1;
    q3 = z3 - z1;

    dd = sqrt((p2 * q3 - q2 * p3) * (p2 * q3 - q2 * p3) +
              (p3 * q1 - q3 * p1) * (p3 * q1 - q3 * p1) +
              (p1 * q2 - q1 * p2) * (p1 * q2 - q1 * p2));
    if (dd == 0.0)
    {
        norm.set(0.0, 1.0, 0.0);
        return 0;
    }

    norm.x = (p2 * q3 - q2 * p3) / dd;
    norm.y = (p3 * q1 - q3 * p1) / dd;
    norm.z = (p1 * q2 - q1 * p2) / dd;

    if (isnan(norm.x) || isnan(norm.y) || isnan(norm.z))
    {
        norm.set(0.0, 1.0, 0.0);
        return 0;
    }

    return 0;
}

int doMaterial(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    mat_t material;
    char *p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown MATERIAL format %s\n", Line);
        return (-1);
    }

    // skip whitespace
    while (*p != '\0' && std::isspace(*p))
        p++;

    if (*p == '\"')
    {
        p++;

        while (*p != '\0' && *p != '\"')
        {
            material.name.push_back(p[0]);
            p++;
        }
    }
    else
    {
        while (*p != '\0' && !std::isspace(*p))
        {
            material.name.push_back(p[0]);
            p++;
        }
    }

    if (*p == '\0')
    {
        fprintf(stderr, "invalid MATERIAL format %s\n", Line);
        return (-1);
    }

    p++;

    // skip whitespace
    while (*p != '\0' && std::isspace(*p))
        p++;

    if (sscanf(p,
            "rgb %lf %lf %lf amb %lf %lf %lf emis %lf %lf %lf spec %lf %lf %lf shi %lf trans %lf",
            &(material.rgb.r), &(material.rgb.g), &(material.rgb.b),
            &(material.amb.r), &(material.amb.g), &(material.amb.b),
            &(material.emis.r), &(material.emis.g), &(material.emis.b),
            &(material.spec.r), &(material.spec.g), &(material.spec.b),
            &(material.shi), &(material.trans)) != 14)
    {
        fprintf(stderr, "invalid MATERIAL format %s\n", p);
        return (-1);
    }

    // append to list
    materials.push_back(material); // use emplace_back someday

    nummaterial++;

    return (0);
}

int doObject(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char name[256] = { 0 };
    char *p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown OBJECT format %s\n", Line);
        return (-1);
    }
    if (sscanf(p, "%255s", name) != 1)
    {
        fprintf(stderr, "invalid OBJECT format %s\n", p);
        return (-1);
    }

    objects.push_front(ob_t());

    ob_t &objectt = objects.front();

    objectt.x_min = 1000000;
    objectt.y_min = 1000000;
    objectt.z_min = 1000000;
    objectt.type = name;
    objectt.texrep_x = 1.0;
    objectt.texrep_y = 1.0;

    //numob++;
    numrefs = 0;
    numsurf = 0;
    numvertFound = false;
    dataFound = false;

    return (0);
}

int findIndice(int indice, const std::vector<int> &oldva, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (oldva[i] == indice)
            return i;
    }
    return -1;
}

void terrainSplitOb(std::list<ob_t> &objects, std::list<ob_t>::iterator &it)
{
    std::list<ob_t> split;
    const ob_t &object = *it;

    printf("terrain splitting %s\n", object.name.c_str());
    if ((object.x_max - object.x_min) < 2 * distSplit)
        return;
    if ((object.y_max - object.y_min) < 2 * distSplit)
        return;
    printf("terrain splitting %s started\n", object.name.c_str());

    std::vector<int> oldSurfToNewObjMap(object.numsurf, 0);

    int numNewObjs = 0;

    for (double curXPos = object.x_min; curXPos < object.x_max; curXPos += distSplit)
    {
        for (double curYPos = object.y_min; curYPos < object.y_max; curYPos += distSplit)
        {
            int numTriFound = 0;
            bool found_a_tri = false;

            for (int curObjSurf = 0; curObjSurf < object.numsurf; curObjSurf++)
            {
                point_t surfCentroid;
                object.computeObSurfCentroid(curObjSurf, surfCentroid);

                if (surfCentroid.x >= curXPos && surfCentroid.x < curXPos + distSplit)
                {
                    if (surfCentroid.y >= curYPos && surfCentroid.y < curYPos + distSplit)
                    {
                        found_a_tri = true;
                        oldSurfToNewObjMap[curObjSurf] = numNewObjs;
                        numTriFound++;
                    }
                }
            }

            if (found_a_tri)
            {
                printf("surface num %d : numtri : %d\n", numNewObjs, numTriFound);
                numNewObjs++;
            }
        }
    }
    printf("found in %s : %d subsurfaces\n", object.name.c_str(), numNewObjs);

    for (int curNewObj = 0; curNewObj < numNewObjs; curNewObj++)
    {
        int numNewSurf = 0;
        /* find the number of surface */
        for (int curSurf = 0; curSurf < object.numsurf; curSurf++)
        {
            if (oldSurfToNewObjMap[curSurf] != curNewObj)
                continue;
            numNewSurf++;
        }

        /* initial creation of tob */

        ob_t tob;

        tob.numsurf = numNewSurf;
        tob.attrSurf = object.attrSurf;
        tob.attrMat = object.attrMat;
        tob.data = object.data;
        tob.type = object.type;

        /* special name handling */
        tob.name = object.name + "__split__" + std::to_string(curNewObj);

        tob.copyTextureNames(object);

        /* store the index data in tob's vertexarray */

        tob.createVertexArrays();

        int curNewSurf = 0;
        for (int curSurf = 0; curSurf < object.numsurf; curSurf++)
        {
            if (oldSurfToNewObjMap[curSurf] != curNewObj)
                continue;

            copyVertexArraysSurface(tob, curNewSurf, object, curSurf);

            curNewSurf++;
        }

        /* create a list with temporal points and smoothed normals and store the index
         * to them in tob's vertexarray.indice property.
         */

        /* temporal storage for points and smoothed normals. Temporal because
         * we don't know the size, so we allocate the same number as in the
         * source object.
         */
        std::vector<point_t> pttmp(object.numvertice, point_t(0.0, 0.0, 0.0));
        std::vector<point_t> snorm(object.numvertice, point_t(0.0, 0.0, 0.0));

        /* storedPtIdxArr: keep a list of the indices of points stored in the new object.
         * If an index is contained in storedPtIdxArr we don't store the point itself,
         * but only the index in the vertexarray of the new object.
         */
        std::vector<int> storedPtIdxArr(object.numvertice, 0);

        int curNewPtIdx = 0;
        for (int curNewIdx = 0; curNewIdx < numNewSurf * 3; curNewIdx++)
        {
            int idx = tob.vertexarray[curNewIdx].indice;

            int storedIdx = findIndice(idx, storedPtIdxArr, curNewPtIdx);
            if (storedIdx == -1)
            {
                storedPtIdxArr[curNewPtIdx] = idx;
                storedIdx = curNewPtIdx;
                pttmp[curNewPtIdx] = object.vertex[idx];
                snorm[curNewPtIdx] = object.norm[idx];
                curNewPtIdx++;
            }

            tob.setVertexArraysIndex(curNewIdx, storedIdx);
        }

        int numNewPts = curNewPtIdx;

        tob.numvert = numNewPts;
        tob.numvertice = numNewPts;

        /* create and store tob's norm, snorm, vertex and textarray data */

        tob.vertex = pttmp;
        tob.norm = snorm;
        tob.snorm = snorm;

        tob.createTextArrays();

        tob.initSpacialExtend();

        // prepend the new object to the list
        split.push_front(tob);
    }

    if (!split.empty())
    {
        // move new objects in front of old object in list
        objects.splice(it, split);
        // remove old object from list
        objects.erase(it);
    }
}

void splitOb(std::list<ob_t> &objects, std::list<ob_t>::iterator &it)
{
    int oldnumptstored = 0; /* temporary placeholder for numptstored */

    /* The object we use as storage during splitting.
     * Following attribs will be used: vertexarray, vertex, norn, snorm, textarray,
     * numvert, numvertice, numsurf
     */
    ob_t workob;
    std::list<ob_t> split;

    tcoord_t curvertex[3];
    int curstoredidx[3];

    bool touse = false;
    int orignumtris = it->numsurf; /* number of surfaces/triangles in the source object */
    int orignumverts = orignumtris * 3; /* number of vertices in the source object: orignumtris * 3 */
    bool mustcontinue = true;
    int numobject = 0;
    int curvert = 0;

    std::vector<bool> tri(orignumtris, false);
    std::vector<int> oldva(orignumverts, 0);

    workob.vertex.resize(orignumverts);
    workob.norm.resize(orignumverts);
    workob.snorm.resize(orignumverts);
    if (it->hasTexture())
    {
        workob.vertexarray.resize(orignumverts);
        workob.textarray.resize(orignumverts);
    }
    if (it->hasTexture1())
    {
        workob.vertexarray1.resize(orignumverts);
        workob.textarray1.resize(orignumverts);
    }
    if (it->hasTexture2())
    {
        workob.vertexarray2.resize(orignumverts);
        workob.textarray2.resize(orignumverts);
    }
    if (it->hasTexture3())
    {
        workob.vertexarray3.resize(orignumverts);
        workob.textarray3.resize(orignumverts);
    }

    while (mustcontinue)
    {
        mustcontinue = false;

        int numvertstored = 0; /* number of vertices stored in the object */
        int numtristored = 0; /* number of triangles stored in the object: numvertstored/3 */
        int numptstored = 0; /* number of vertices stored */
        bool firstTri = false;
        bool atleastone = true;

        while (atleastone)
        {
            atleastone = false;
            for (int curtri = 0; curtri < orignumtris; curtri++)
            {
                touse = false;
                if (tri[curtri])
                    continue;
                mustcontinue = 1;

                curvert = curtri * 3;

                /** find vertices of the triangle */
                for (int i = 0; i < 3; i++)
                {
                    curvertex[i] = it->vertexarray[curvert+i];

                    curstoredidx[i] = findIndice(curvertex[i].indice, oldva, numptstored);
                }

                if (curstoredidx[0] == -1 && curstoredidx[1] == -1 && curstoredidx[2] == -1)
                {
                    if (!firstTri)
                        touse = true;
                    else
                        touse = false;
                    /* triangle is ok */
                }
                else
                {
                    touse = true;

                    for (int i = 0; i < 3; i++)
                    {
                        if (curstoredidx[i] != -1)
                        {
                            if (workob.textarray.empty() || workob.textarray[curstoredidx[i]] != curvertex[i].uv)
                            {
                                touse = false;
                                /* triangle is not ok */
                            }
                        }
                    }
                }

                if (touse)
                {
                    firstTri = true;
                    /* triangle is ok */

                    tri[curtri] = true; /* mark this triangle */

                    /* store the vertices of the triangle with new indice */
                    /* not yet in the array : store it at the current position */
                    for (int i = 0; i < 3; i++)
                    {
                        oldnumptstored = numptstored;

                        if (curstoredidx[i] == -1)
                        {
                            workob.vertex[numptstored] = it->vertex[curvertex[i].indice];
                            workob.norm[numptstored] = it->norm[curvertex[i].indice];
                            workob.snorm[numptstored] = it->snorm[curvertex[i].indice];

                            // clear saved in vertex array entry
                            it->vertexarray[curvert + i].saved = false;

                            oldva[numptstored] = curvertex[i].indice; /* remember the value of the vertice already saved */
                            curstoredidx[i] = numptstored;
                            numptstored++;
                        }

                        copySingleVertexData(workob, *it, curstoredidx[i], oldnumptstored, numvertstored, curvert+i);

                        numvertstored++;
                    }

                    numtristored++;
                    atleastone = true;

                } // if (touse)

            } // for (curtri = 0; curtri < orignumtris; curtri++)
        } // while (atleastone)

        if (numtristored == 0)
            continue;

        /* must saved the object */
        workob.numvert = numptstored;
        workob.numvertice = numvertstored;
        workob.numsurf = numvertstored/3;

        split.push_front(ob_t());

        createObjectSplitCopy(numobject++, *it, workob, split.front());

        attrSurf = it->attrSurf;
        attrMat = it->attrMat;

        printf("numtri = %d on orignumtris = %d\n", numtristored, orignumtris);
    } // while (mustcontinue == 1)

    if (!split.empty())
    {
        // move new objects in front of old object in list
        objects.splice(it, split);
        // remove old object from list
        objects.erase(it);
    }
}

int doKids(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    int kids;
    char *p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Kids format %s\n", Line);
        return (-1);
    }
    if (sscanf(p, "%d", &kids) != 1)
    {
        fprintf(stderr, "invalid Kids format %s\n", p);
        return (-1);
    }

    ob_t &object = objects.front();
    if (kids == 0)
    {
        if (numsurf != object.numsurf)
        {
            fprintf(stderr, "only %d of %d SURF found in %s\n", numsurf, object.numsurf, object.name.c_str());
            return (-1);
        }
        object.vertexarray.resize(numrefstotal);
        object.textarray.resize(numrefstotal);
        object.norm.assign(numrefstotal * 3, point_t(0.0, 0.0, 0.0));
        object.snorm.assign(numrefstotal * 3, point_t(0.0, 0.0, 0.0));
        object.attrSurf = attrSurf;
        object.attrMat = attrMat;
        attrSurf = 0x20;

        for (int i = 0; i < numrefstotal; i++)
        {
            object.vertexarray[i] = tmpva[i];
            object.textarray[i] = tmptexa[i];
        }

        object.numvertice = numvertice;

        if (!object.hasName())
        {
            object.name = tmpname + std::to_string(tmpIndice);

            tmpIndice++;
        }

        if ((typeConvertion == _AC3DTOAC3DS
                && (extendedStrips || extendedTriangles))
                || typeConvertion == _AC3DTOAC3DGROUP
                || (typeConvertion == _AC3DTOAC3D && extendedTriangles))
        {
            printf("Computing normals for %s\n", object.name.c_str());
            computeObjectTriNorm(object);
            //smoothObjectTriNorm(object);
        }

        numrefs = numrefstotal = 0;
        numvertFound = false;
        numrefsFound = false;
        dataFound = false;
        numvertex = 0;
        numvertice = 0;
    }
    else
        object.kids = kids;

    return (0);
}

int doName(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char * p;
    char *q;
    char name[256];
    char name2[256];
    p = strstr(Line, "\"");
    if (p == NULL)
    {
        fprintf(stderr, "unknown name format %s\n", Line);
        return (-1);
    }
    else
        p++;
    sprintf(name, "%s", p);
    p = strstr(name, "\n");
    if (p != NULL)
        *p = '\0';

    if (!strcmp("\"n\"", name))
    {
        sprintf(name, "terrain%d", tmpIndice2++);
    }
    if (!strcmp("\"NoName\"", name))
    {
        sprintf(name, "ob%d", tmpIndice3++);
    }
    p = name;
    q = name2;
    while (*p)
    {
        if ((*p <= 'z' && *p >= 'a'))
        {
            *p = (*p - 'a') + 'A';
        }
        if ((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
        {
            *q = *p;
            q++;
            *q = '\0';
        }
        p++;
    }
    sprintf(name, "%s", name2);

    /*sprintf(name,"terrain%d",tmpIndice2++);*/
    ob_t &object = objects.front();
    object.name = name;
    sprintf(tmpname, "%s", name);

    fprintf(stderr, "loading  %s object                             \r", name);
    printf("loading  %s object\n", name);
    tmpIndice = 0;
    dataFound = false;
    return (0);
}

int doLoc(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Loc format %s\n", Line);
        return (-1);
    }
    ob_t &object = objects.front();
    if (sscanf(p, "%lf %lf %lf", &object.loc.x, &object.loc.y, &object.loc.z) != 3)
    {
        fprintf(stderr, "invalid Loc format %s\n", p);
        return (-1);
    }

    dataFound = false;
    return (0);
}

int doData(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown data format %s\n", Line);
        return (-1);
    }
    if (sscanf(p, "%d", &dataSize) != 1)
    {
        fprintf(stderr, "invalid data format %s\n", p);
        return (-1);
    }
    dataFound = true;
    dataSizeRead = 0;
    return (0);
}

int doGetData(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    int lineSize = (int)strlen(Line);
    // the '\n' of the last line is not included
    if ((dataSizeRead + lineSize) > dataSize && Line[lineSize - 1] == '\n')
    {
        Line[lineSize - 1] = 0;
        lineSize--;
    }
    dataSizeRead += lineSize;
    ob_t &object = objects.front();
    object.data += Line;
    return (0);
}

int doCrease(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    dataFound = false;
    return (0);
}

int doSubdiv(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    dataFound = false;
    return (0);
}

int doFolded(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    dataFound = false;
    return (0);
}

int doHidden(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    dataFound = false;
    return (0);
}

int doLocked(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    dataFound = false;
    return (0);
}

int doTexture(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char name[256] = { 0 };
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown texture format %s\n", Line);
        return (-1);
    }
    if (sscanf(p, "%255s", name) != 1)
    {
        fprintf(stderr, "invalid texture format %s\n", p);
        return (-1);
    }

    ob_t &object = objects.front();
    p = strstr(name, "\"");
    if (p != NULL)
    {
        p++;
        char *q = strstr(p, "\"");
        if (q != NULL)
            *q = '\0';

        object.texture = p;
    }
    else
        object.texture = name;
    dataFound = false;
    return (0);
}

int doTexrep(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Texrep format %s\n", Line);
        return (-1);
    }
    ob_t &object = objects.front();
    if (sscanf(p, "%lf %lf", &object.texrep_x, &object.texrep_y) != 2)
    {
        fprintf(stderr, "invalid Texrep format %s\n", p);
        return (-1);
    }

    dataFound = false;
    return (0);
}

int doNumvert(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown numvert format %s\n", Line);
        return (-1);
    }
    ob_t &object = objects.front();
    if (sscanf(p, "%d", &object.numvert) != 1)
    {
        fprintf(stderr, "invalid numvert format %s\n", p);
        return (-1);
    }
    object.vertex.resize(object.numvert);
    numvertex = 0;
    numvertFound = true;
    dataFound = false;
    return (0);
}

int doNumsurf(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown numsurf format %s\n", Line);
        return (-1);
    }
    ob_t &object = objects.front();
    if (sscanf(p, "%d", &object.numsurf) != 1)
    {
        fprintf(stderr, "invalid numsurf format %s\n", p);
        return (-1);
    }
    numvertFound = false;
    dataFound = false;
    return (0);
}

int doGetVertex(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    ob_t &object = objects.front();
    if (sscanf(Line, "%lf %lf %lf ",
        &object.vertex[numvertex].x,
        &object.vertex[numvertex].z,
        &object.vertex[numvertex].y) != 3)
    {
        fprintf(stderr, "invalid vertex format %s\n", Line);
        return (-1);
    }
    object.vertex[numvertex].x += object.loc.x;
    object.vertex[numvertex].y += object.loc.z;
    object.vertex[numvertex].z += object.loc.y;
    object.vertex[numvertex].y = -object.vertex[numvertex].y;
    /* compute min/max of the vertex for this object */
    if (object.vertex[numvertex].x > object.x_max)
        object.x_max = object.vertex[numvertex].x;
    if (object.vertex[numvertex].x < object.x_min)
        object.x_min = object.vertex[numvertex].x;

    if (object.vertex[numvertex].y > object.y_max)
        object.y_max = object.vertex[numvertex].y;
    if (object.vertex[numvertex].y < object.y_min)
        object.y_min = object.vertex[numvertex].y;

    if (object.vertex[numvertex].z > object.z_max)
        object.z_max = object.vertex[numvertex].z;
    if (object.vertex[numvertex].z < object.z_min)
        object.z_min = object.vertex[numvertex].z;

    numvertex++;
    /*fprintf(stderr,"numvertex = %d\n",numvertex);*/
    return (0);
}

int doGetSurf(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    const ob_t &object = objects.front();

    if (sscanf(Line, "%d %lf %lf ", &tmpva[numvertice].indice,
        &tmpva[numvertice].uv.u, &tmpva[numvertice].uv.v) != 3)
    {
        fprintf(stderr, "invalid surf format %s\n", Line);
        return (-1);
    }
    /*fprintf(stderr,"numrefs = %d\n",numrefs);*/
    /*printf("%.2lf %.2lf\n",tmpva[numvertice].uv.u,tmpva[numvertice].uv.v);*/
    tmpva[numvertice].saved = false;
    tmptexa[tmpva[numvertice].indice].u = tmpva[numvertice].uv.u * object.texrep_x;
    tmptexa[tmpva[numvertice].indice].v = tmpva[numvertice].uv.v * object.texrep_y;
    numvertice++;
    return (0);
}

int doSurf(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    unsigned int surf = 0;
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown SURF format %s\n", Line);
        return (-1);
    }
    if (sscanf(p, "%x", &surf) != 1)
    {
        fprintf(stderr, "invalid SURF format %s\n", p);
        return (-1);
    }
    // Check for an object with multiple surfaces with different SURF types.
    // Can't convert multiple triangles with different SURF types into a triangle strip with a single SURF type.
    if ((typeConvertion == _AC3DTOAC3DS || typeConvertion == _AC3DTOAC3DGROUP) && extendedStrips && numrefs && surf != attrSurf)
    {
        fprintf(stderr, "multiple SURF in object 0x%x and 0x%x (OBJECT needs splitting by SURF type?)\n", surf, attrSurf);
        return (-1);
    }
    numsurf++;
    attrSurf = surf;
    numvertFound = false;
    dataFound = false;
    return (0);
}

int doMat(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    int mat = 0;
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown mat format %s\n", Line);
        return (-1);
    }
    if (sscanf(p, "%d", &mat) != 1)
    {
        fprintf(stderr, "invalid mat format %s\n", p);
        return (-1);
    }
    // Check for an object with multiple surfaces with different material types.
    // Can't convert multiple triangles with different material types into a triangle strip with a single material type.
    if ((typeConvertion == _AC3DTOAC3DS || typeConvertion == _AC3DTOAC3DGROUP) && extendedStrips && numrefs && mat != attrMat)
    {
        fprintf(stderr, "multiple mat in object %d and %d (OBJECT needs splitting by material type?)\n", mat, attrMat);
        return (-1);
    }
    attrMat = mat;
    numvertFound = false;
    return (0);
}

int doRefs(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials)
{
    char * p = strstr(Line, " ");
    if (p == NULL)
    {
        fprintf(stderr, "unknown Refs format %s\n", Line);
        return (-1);
    }
    if (sscanf(p, "%d", &refs) != 1)
    {
        fprintf(stderr, "invalid Refs format %s\n", p);
        return (-1);
    }
    if (refs != 3)
    {
        fprintf(stderr, "invalid number of Refs %d\n", refs);
        return (-1);
    }

    numrefstotal += refs;
    numrefsFound = true;
    numrefs++;
    return (0);
}

/* We need to split an object face in more faces
 * if there are common points with different texture coordinates.
 */
bool isObjectSplit(const ob_t &object)
{
    if (notexturesplit)
        return false;

    int numverts = object.numvertice;

    for (int i = 0; i < numverts; i++)
    {
        for (int j = i + 1; j < numverts; j++)
        {
            bool same_pt = (object.vertexarray[i].indice == object.vertexarray[j].indice);
            bool diff_u = (object.vertexarray[i].uv.u != object.vertexarray[j].uv.u);
            bool diff_v = (object.vertexarray[i].uv.v != object.vertexarray[j].uv.v);

            if (same_pt && (diff_u || diff_v))
                return true;
        }
    }

    if (collapseObject)
        return true;

    return false;
}

bool isTerrainSplit(const ob_t &object)
{
    /* general showstoppers */
    if (typeConvertion == _AC3DTOAC3DS)
        return false;

    if (distSplit <= 0)
        return false;

    if (object.hasName())
    {
        /* denied prefixes */
        const int num_prefixes = 17;
        const char* denied_prefixes[num_prefixes] =
        { "tkrb", "tkmn", "tkrs", "tklb", "brlt", "brrt", "tkls", "t0RB",
          "t1RB", "t2RB", "tkRS", "t0LB", "t1LB", "t2LB", "tkLS", "BOLt",
          "BORt" };

        for (int i = 0; i < num_prefixes; i++)
        {
            if (object.nameStartsWith(denied_prefixes[i]))
                return false;
        }

        /* name contains terrain or ground */
        if (object.nameHasStr("terrain") || object.nameHasStr("TERRAIN") ||
            object.nameHasStr("GROUND") || object.nameHasStr("ground"))
            return true;
    }

    /* dimension within splitting distance */
    if (((object.x_max - object.x_min) > 1.5 * distSplit) ||
        ((object.y_max - object.y_min) > 1.5 * distSplit))
        return true;

    return false;
}

void splitObjects(std::list<ob_t> &objects)
{
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end();)
    {
        if (isObjectSplit(*it))
        {
            printf("Found in %s, a duplicate coord with different u,v, split is required\n",
                   it->name.c_str());

            std::list<ob_t>::iterator other = it++;
            splitOb(objects, other);
        }
        else if (isTerrainSplit(*it))
        {
            printf("Splitting surfaces of %s\n", it->name.c_str());

            std::list<ob_t>::iterator other = it++;
            terrainSplitOb(objects, other);
        }
        else
            ++it;
    }
}

int loadAC(const std::string &inputFilename, std::list<ob_t> &objects, std::vector<mat_t> &materials, const std::string &outputFilename)
{
    /* saveIn : 0= 3ds , 1= obj , 2=ac3d grouped (track) , 3 = ac3d strips (cars) */
    char Line[256];
    int ret = 0;
    int (*doVerb)(char *Line, std::list<ob_t> &objects, std::vector<mat_t> &materials);
    FILE * file;

    if ((file = fopen(inputFilename.c_str(), "r")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", inputFilename.c_str());
        return (-1);
    }
    if (fgets(Line, 256, file) == NULL)
    {
        fprintf(stderr, "failed to read first line of the file\n");
        fclose(file);
        return (-1);
    }
    if (strnicmp(Line, AC3D, strlen(AC3D)))
    {
        fprintf(stderr, "unknown format %s\n", Line);
        fclose(file);
        return (-1);
    }

    fprintf(stderr, "starting loading ...\n");

    while (fgets(Line, sizeof(Line), file))
    {
        int i = 0;
        /*fprintf(stderr,"parsing line: %s", Line);*/
        doVerb = NULL;
        while (1)
        {
            if (stricmp("END", verbTab[i].verb) == 0)
                break;
            if (strnicmp(Line, verbTab[i].verb, strlen(verbTab[i].verb)) == 0)
            {
                doVerb = verbTab[i].doVerb;
                break;
            }
            i++;
        }
        if (numvertFound && doVerb == NULL)
        {
            ret = doGetVertex(Line, objects, materials);
            if(ret != 0)
                break;
        }
        else if (numrefsFound && doVerb == NULL)
        {
            ret = doGetSurf(Line, objects, materials);
            if(ret != 0)
                break;
        }
        else if (dataFound && doVerb == NULL)
        {
            ret = doGetData(Line, objects, materials);
            if (ret != 0)
                break;
        }
        else
        {
            if (doVerb == NULL)
            {
                fprintf(stderr, " Unknown verb %s\n", Line);
                continue;
            }
            numvertFound = false;
            numrefsFound = false;
            dataFound = false;
            ret = doVerb(Line, objects, materials);
            if(ret != 0)
                break;
        }
    }
    fclose(file);
    if(ret != 0)
        return ret;

    if (splitObjectsDuringLoad != 0)
        splitObjects(objects);

    // --- perform file output ---

    if(outputFilename.empty())
        return 0;

    if (typeConvertion == _AC3DTOOBJ)
    {
        computeSaveOBJ(outputFilename, objects, materials);
    }
    else if (typeConvertion == _AC3DTOAC3DM)
    {
        computeSaveAC3DM(outputFilename, objects, materials);
    }
    else if (typeConvertion == _AC3DTOAC3DS)
    {
        computeSaveAC3DStrip(outputFilename, objects, materials);
    }
    else if (typeConvertion == _AC3DTOAC3D)
    {
        computeSaveAC3D(outputFilename, objects, materials);
    }

    return 0;
}

int printOb(FILE *ofile, ob_t &object)
{
    bool multitex = false;

    if (object.numsurf == 0)
        return 0;

    object.saved = true;
    fprintf(ofile, "OBJECT poly\n");
    fprintf(ofile, "name \"%s\"\n", object.name.c_str());
    if (object.hasMultiTexture())
    {
        multitex = true;
        fprintf(ofile, "texture \"%s\" base\n", object.texture.c_str());
        if (object.hasTexture1())
            fprintf(ofile, "texture \"%s\" tiled\n", object.texture1.c_str());
        else
            fprintf(ofile, "texture empty_texture_no_mapping tiled\n");
        if (object.hasTexture2())
            fprintf(ofile, "texture \"%s\" skids\n", object.texture2.c_str());
        else
            fprintf(ofile, "texture empty_texture_no_mapping skids\n");
        if (object.hasTexture3())
            fprintf(ofile, "texture \"%s\" shad\n", object.texture3.c_str());
        else
            fprintf(ofile, "texture empty_texture_no_mapping shad\n");
    }
    else
    {
        fprintf(ofile, "texture \"%s\"\n", object.texture.c_str());
    }
    fprintf(ofile, "numvert %d\n", object.numvert);
    for (int i = 0; i < object.numvert; i++)
    {
        if (extendedStrips || extendedTriangles)
        {
            fprintf(ofile, "%lf %lf %lf %lf %lf %lf\n", object.vertex[i].x,
                object.vertex[i].z, -object.vertex[i].y, object.snorm[i].x,
                object.snorm[i].z, -object.snorm[i].y);
        }
        else
        {
            fprintf(ofile, "%lf %lf %lf\n", object.vertex[i].x, object.vertex[i].z, -object.vertex[i].y);
        }
    }
    if (!extendedStrips)
    {
        fprintf(ofile, "numsurf %d\n", object.numsurf);
        for (int i = 0; i < object.numsurf; i++)
        {
            if (object.attrSurf != 0)
                fprintf(ofile, "SURF 0x%02x\n", object.attrSurf);
            else
                fprintf(ofile, "SURF 0x20\n");
            fprintf(ofile, "mat %d\n", object.attrMat);
            fprintf(ofile, "refs %zu\n", object.vertexarray.size());
            if (object.vertexarray.size() == 3)
            {
                /* GUIONS */
                if (!multitex)
                {
                    fprintf(ofile, "%d %.5f %.5f\n",
                        object.vertexarray[i * 3].indice,
                        object.textarray[object.vertexarray[i * 3].indice].u,
                        object.textarray[object.vertexarray[i * 3].indice].v);
                    fprintf(ofile, "%d %.5f %.5f\n",
                        object.vertexarray[i * 3 + 1].indice,
                        object.textarray[object.vertexarray[i * 3 + 1].indice].u,
                        object.textarray[object.vertexarray[i * 3 + 1].indice].v);
                    fprintf(ofile, "%d %.5f %.5f\n",
                        object.vertexarray[i * 3 + 2].indice,
                        object.textarray[object.vertexarray[i * 3 + 2].indice].u,
                        object.textarray[object.vertexarray[i * 3 + 2].indice].v);
                }
                else
                {
                    fprintf(ofile, "%d %.5f %.5f",
                        object.vertexarray[i * 3].indice,
                        object.textarray[object.vertexarray[i * 3].indice].u,
                        object.textarray[object.vertexarray[i * 3].indice].v);

                    if (object.hasTexture1())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray1[object.vertexarray[i * 3].indice].u,
                            object.textarray1[object.vertexarray[i * 3].indice].v);
                    else if (object.hasTexture2() || object.hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object.hasTexture2())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray2[object.vertexarray[i * 3].indice].u,
                            object.textarray2[object.vertexarray[i * 3].indice].v);
                    else if (object.hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object.hasTexture3())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray3[object.vertexarray[i * 3].indice].u,
                            object.textarray3[object.vertexarray[i * 3].indice].v);
                    fprintf(ofile, "\n");

                    fprintf(ofile, "%d %.5f %.5f",
                        object.vertexarray[i * 3 + 1].indice,
                        object.textarray[object.vertexarray[i * 3 + 1].indice].u,
                        object.textarray[object.vertexarray[i * 3 + 1].indice].v);

                    if (object.hasTexture1())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray1[object.vertexarray[i * 3 + 1].indice].u,
                            object.textarray1[object.vertexarray[i * 3 + 1].indice].v);
                    else if (object.hasTexture2() || object.hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object.hasTexture2())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray2[object.vertexarray[i * 3 + 1].indice].u,
                            object.textarray2[object.vertexarray[i * 3 + 1].indice].v);
                    else if (object.hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object.hasTexture3())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray3[object.vertexarray[i * 3 + 1].indice].u,
                            object.textarray3[object.vertexarray[i * 3 + 1].indice].v);

                    fprintf(ofile, "\n");

                    fprintf(ofile, "%d %.5f %.5f",
                        object.vertexarray[i * 3 + 2].indice,
                        object.textarray[object.vertexarray[i * 3 + 2].indice].u,
                        object.textarray[object.vertexarray[i * 3 + 2].indice].v);

                    if (object.hasTexture1())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray1[object.vertexarray[i * 3 + 2].indice].u,
                            object.textarray1[object.vertexarray[i * 3 + 2].indice].v);
                    else if (object.hasTexture2() || object.hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object.hasTexture2())
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray2[object.vertexarray[i * 3 + 2].indice].u,
                            object.textarray2[object.vertexarray[i * 3 + 2].indice].v);
                    else if (object.hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object.hasTexture3())
                    {
                        fprintf(ofile, " %.5f %.5f",
                            object.textarray3[object.vertexarray[i * 3 + 2].indice].u,
                            object.textarray3[object.vertexarray[i * 3 + 2].indice].v);
                        if (object.textarray3[object.vertexarray[i * 3 + 2].indice].u
                            != object.textarray1[object.vertexarray[i * 3 + 2].indice].u)
                        {
                            printf("error in text\n");
                        }
                    }

                    fprintf(ofile, "\n");
                }
            }
        }
    }
    else
    {
        stripifyOb(ofile, &object, true);
    }
    fprintf(ofile, "kids 0\n");
    return 0;
}

int foundNear(FILE * ofile, ob_t &object, std::list<ob_t> &objects, double dist, bool print)
{
    double x;
    double y;
    int numfound = 0;

    x = (object.x_max - object.x_min) / 2 + object.x_min;
    y = (object.y_max - object.y_min) / 2 + object.y_min;

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        if (it->nameStartsWith("tkmn"))
            continue;

        if (it->inkids_o)
            continue;

        if (it->numsurf == 0)
            continue;

        if ((it->x_min - x) * (it->x_min - x) + (it->y_min - y) * (it->y_min - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d)\n", it->name , object.name, dist);*/
            numfound++;
            it->inkids_o = true;
            if (print)
                printOb(ofile, *it);
            continue;
        }
        if ((it->x_max - x) * (it->x_max - x) + (it->y_max - y) * (it->y_max - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d)\n", it->name , object.name, dist);*/
            numfound++;
            it->inkids_o = true;
            if (print)
                printOb(ofile, *it);
            continue;
        }
        if ((it->x_min - x) * (it->x_min - x) + (it->y_max - y) * (it->y_max - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d)\n", it->name , object.name, dist);*/
            numfound++;
            it->inkids_o = true;
            if (print)
                printOb(ofile, *it);
            continue;
        }
        if ((it->x_max - x) * (it->x_max - x) + (it->y_min - y) * (it->y_min - y) < dist * dist)
        {
            /*printf("object %s near object %s (dist=%d)\n", it->name , object.name, dist);*/
            numfound++;
            it->inkids_o = true;
            if (print)
                printOb(ofile, *it);
            continue;
        }
    }

    object.kids_o = numfound++;

    /*printf(" object %s (dist=%d) found %d objects\n", object.name, dist, numfound);*/
    return (0);
}

void normalize(point_t &t)
{
    double dd;
    dd = sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
    if (dd != 0.0)
        t /= dd;
    else
        t.set(0.0, 1.0, 0.0);
}

void computeTriNorm(std::list<ob_t> &objects)
{
    point_t norm;

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numsurf; i++)
        {
            /* compute the same normal for each points in the surface */
            computeNorm(it->vertex[it->vertexarray[i * 3].indice],
                        it->vertex[it->vertexarray[i * 3 + 1].indice],
                        it->vertex[it->vertexarray[i * 3 + 2].indice],
                        norm);
            it->norm[it->vertexarray[i * 3].indice] += norm;
            it->norm[it->vertexarray[i * 3 + 1].indice] += norm;
            it->norm[it->vertexarray[i * 3 + 2].indice] += norm;
        }
        for (int i = 0; i < it->numsurf; i++)
        {
            normalize(it->norm[it->vertexarray[i * 3].indice]);
            normalize(it->norm[it->vertexarray[i * 3 + 1].indice]);
            normalize(it->norm[it->vertexarray[i * 3 + 2].indice]);
        }
    }
    return;
}

void computeObjectTriNorm(ob_t &object)
{
    point_t norm;

    if (object.canSkip())
        return;

    for (int i = 0; i < object.numsurf; i++)
    {
        /* compute the same normal for each points in the surface */
        computeNorm(object.vertex[object.vertexarray[i * 3].indice],
                    object.vertex[object.vertexarray[i * 3 + 1].indice],
                    object.vertex[object.vertexarray[i * 3 + 2].indice], norm);

        object.norm[object.vertexarray[i * 3].indice] += norm;
        object.norm[object.vertexarray[i * 3 + 1].indice] += norm;
        object.norm[object.vertexarray[i * 3 + 2].indice] += norm;
    }
    for (int i = 0; i < object.numsurf; i++)
    {
        normalize(object.norm[object.vertexarray[i * 3].indice]);
        normalize(object.norm[object.vertexarray[i * 3 + 1].indice]);
        normalize(object.norm[object.vertexarray[i * 3 + 2].indice]);
    }

    return;
}

bool checkMustSmoothVector(const point_t &n1, const point_t &n2, const point_t &t1, const point_t &t2)
{
    return false;
#if 0
    double dot, cos_angle;
    cos_angle = cos(smooth_angle * M_PI / 180.0);
    if (fabs(t1.x - t2.x) <= 0.05 && fabs(t1.y - t2.y) <= 0.05 && fabs(t1.z - t2.z) <= 0.05)
    {
        // GUIONS
        dot = n1.x * n2.x + n1.y * n2.y + n1.z * n2.z;
        if (dot > cos_angle)
        {
            return true;
        }

    }
    return false;
#endif
}

void smoothTriNorm(std::list<ob_t> &objects)
{
    printf("Smooth called on %s\n", objects.front().name.c_str());
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip() || it->type == "group")
            continue;

        if (it->numsurf == 0)
        {
            printf("Skipping %s because there are no surfaces\n", it->name.c_str());
            continue;
        }

        for (int i = 0; i < it->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            it->snorm[i] = it->norm[i];
        }
    }

#if 0 // this does nothing
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip() || it->hasNoSmooth())
            continue;
        for (std::list<ob_t>::iterator it1 = objects.begin(); it1 != objects.end(); ++it1)
        {
            if (it1->canSkip() || it1->hasNoSmooth())
                continue;
            for (int i = 0; i < it->numvert; i++)
            {
                for (int j = 0; j < it1->numvert; j++)
                {
                    if (checkMustSmoothVector(it->norm[i], it1->norm[j], it->vertex[i], it1->vertex[j]))
                    {
                        point_t p = it1->norm[j] + it->norm[i];
                        normalize(p);

                        it->snorm[i] = p;
                        it1->snorm[j] = p;
                    }
                }
            }
        }
    }
#endif

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip() || it->type == "group" || it->numsurf == 0)
            continue;
        for (int i = 0; i < it->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            double nx = it->snorm[i].x;
            double ny = it->snorm[i].y;
            double nz = it->snorm[i].z;
            double dd = sqrt(nx * nx + ny * ny + nz * nz);
            if (dd != 0.0)
            {
                it->snorm[i].x = nx / dd;
                it->snorm[i].y = ny / dd;
                it->snorm[i].z = nz / dd;
            }
            else
                it->snorm[i].set(0, 0, 1);
        }
    }

    return;
}

void smoothFaceTriNorm(ob_t &object)
{
    if (object.canSkip())
        return;

    for (int i = 0; i < object.numvert; i++)
    {
        /* compute the same normal for each points in the surface */
        for (int j = 0; j < object.numvert; j++)
        {
            if ((object.vertex[i].x - object.vertex[j].x) <= 0.01 &&
                (object.vertex[i].y - object.vertex[j].y) <= 0.01 &&
                (object.vertex[i].z - object.vertex[j].z) <= 0.1)
            {
                /*same point */
                object.snorm[i] += object.norm[j];
                object.snorm[j] = object.snorm[i];
            }
        }
    }

    for (int i = 0; i < object.numvert; i++)
        normalize(object.snorm[i]);

    return;
}

void smoothObjectTriNorm(ob_t &object)
{
    for (int i = 0; i < object.numvert; i++)
    {
        /* compute the same normal for each points in the surface */
        for (int j = 0; j < object.numvert; j++)
        {
            if ((object.vertex[i].x - object.vertex[j].x) <= 0.001 &&
                (object.vertex[i].y - object.vertex[j].y) <= 0.001 &&
                (object.vertex[i].z - object.vertex[j].z) <= 0.001)
            {
                /*same point */
                object.snorm[i] += object.norm[j];
                object.snorm[j] = object.snorm[i];
            }
        }
    }
    for (int i = 0; i < object.numvert; i++)
        normalize(object.snorm[i]);

    return;
}

void computeSaveAC3D(const std::string &OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials)
{
    char name2[256];
    char *p, *q;
    int numg = 0;
    bool lastpass = false;
    int nborder = 0;
    bool ordering = false;
    FILE * ofile = NULL;

    if (normalMapping)
        normalMap(objects);

    if ((ofile = fopen(OutputFilename.c_str(), "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename.c_str());
        return;
    }
    if (extendedTriangles)
    {
        smoothTriNorm(objects);
        if (isobjectacar)
        {
            mapNormalToSphere2(objects);
            if (extendedEnvCoord)
                mapTextureEnv(objects);
        }
        if (collapseObject)
            mergeSplitted(objects);
    }

    fprintf(ofile, "AC3Db\n");
    printMaterials(ofile, materials);
    fprintf(ofile, "OBJECT world\n");

    if (OrderString && isobjectacar)
    {
        fprintf(stderr, "ordering objects according to  %s\n", OrderString);
        p = OrderString;
        ordering = true;
        nborder = 1;
        while (true)
        {
            q = strstr(p, ";");
            if (q != NULL)
                nborder++;
            else
                break;
            p = q + 1;
            if (*p == '\0')
            {
                nborder--;
                break;
            }
        }
    }
    else
    {
        ordering = false;
        nborder = 0;
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (!it->hasName())
            continue;

        if (!isobjectacar)
        {
            if (it->nameStartsWith("tkmn"))
            {
                numg++;
                continue;
            }
        }
        else
        {
            if (it->type == "group")
                continue;
            numg++;
        }
    }

    fprintf(ofile, "kids %d\n", numg);

    texnum = 0;
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        int texnofound = 0;
        if (it->canSkip())
            continue;

        texnofound = 1;
        for (int i = 0; i < texnum; i++)
        {
            if (!it->hasTexture())
            {
                texnofound = 0;
                break;
            }
            if (!strncmp(tex[i], it->texture.c_str(), 13))
            {
                texnofound = 0;
                break;
            }
            else
                texnofound = 1;
        }
        if (texnofound == 1)
        {
            if (it->hasTexture())
            {
                strcpy(tex[texnum], it->texture.c_str());
                tex[texnum][13] = '\0';
                /*sprintf(tex[texnum],"%s",it->texture);*/
            }
            texnum++;
        }
        it->saved = false;
        printf("name=%s x_min=%.1f y_min=%.1f x_max=%.1f y_max=%.1f\n",
                it->name.c_str(), it->x_min, it->y_min, it->x_max, it->y_max);
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        it->kids_o = 0;
        if (it->canSkip())
            continue;

        if (it->nameStartsWith("tkmn"))
        {
            foundNear(ofile, *it, objects, far_dist, false);
            printf("object =%s num kids_o=%d\n", it->name.c_str(), it->kids_o);
        }
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        it->inkids_o = false;
    }

    p = OrderString;
    q = OrderString;
    nborder++;
    for (int ik = 0; ik < nborder; ik++)
    {
        if (ordering)
        {
            /* look to the current object name to save */
            if (p == NULL)
                lastpass = true;
            else
            {
                q = p;
                p = strstr(p, ";");
                if (p != NULL)
                {
                    *p = '\0';
                    p++;
                }
            }
        }
        for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
        {
            if (it->canSkip())
                continue;

            if (!isobjectacar)
            {
                if (it->nameStartsWith("tkmn"))
                {
                    fprintf(ofile, "OBJECT group\n");
                    fprintf(ofile, "name \"%s_g\"\n", it->name.c_str());
                    fprintf(ofile, "kids %d\n", it->kids_o + 1);
                    printOb(ofile, *it);
                    foundNear(ofile, *it, objects, far_dist, true);
                    printf("object =%s num kids_o=%d\n", it->name.c_str(), it->kids_o);
                }
            }
            else
            {
                if (!it->saved)
                {
                    if (ordering && !lastpass)
                    {
                        if (it->name == q)
                        {
                            printOb(ofile, *it);
                            printf("object =%s num kids_o=%d test with %s\n", it->name.c_str(), it->kids_o, q);
                        }
                        else
                        {
                            char nameBuf[1024];
                            sprintf(nameBuf, "%ss", q);
                            if (it->name == nameBuf)
                            {
                                printOb(ofile, *it);
                                printf("object =%s num kids_o=%d\n", it->name.c_str(), it->kids_o);
                            }

                        }
                    }
                    else
                    {
                        printOb(ofile, *it);
                        printf("object =%s num kids_o=%d\n", it->name.c_str(), it->kids_o);
                    }
                }
            }
        }
    }

    for (int i = 0; i < texnum; i++)
    {
        printf("analysing  %s\n", tex[i]);
        p = tex[i];
        q = name2;
        while (*p)
        {
            if ((*p <= 'Z' && *p >= 'A'))
            {
                *p = (*p - 'A') + 'a';
            }

            if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')
                    || (*p == '.'))
            {
                *q = *p;
                q++;
                *q = '\0';
            }
            p++;
        }
        int j = 0;
        while (name2[j] != '\0')
        {
            if (name2[j] == '.')
            {
                name2[j] = '\0';
                break;
            }
            j++;
            if (j == 8)
            {
                name2[j] = '\0';
                break;
            }
        }

        printf("texture file %s will be stored as %s.png\n", tex[i], name2);
    }

    tmpIndice = 0;
    /* do the job */
    printf("\nend\n");

    fclose(ofile);
}

void computeSaveOBJ(const std::string &OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials)
{
    char name2[256];
    int deltav = 1;
    int ind = 0;
    char tname[256];
    FILE * ofile;
    FILE * tfile;

    if ((ofile = fopen(OutputFilename.c_str(), "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename.c_str());
        return;
    }

    fprintf(ofile, "mtllib ./%s.mtl\n", OutputFilename.c_str());
    sprintf(tname, "%s.mtl", OutputFilename.c_str());

    if ((tfile = fopen(tname, "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", tname);
        fclose(ofile);
        return;
    }

    for (size_t i = 0, end = materials.size(); i < end; ++i)
    {
	for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
        {
            if (it->canSkip())
                continue;

            if (it->hasTexture())
            {
                fprintf(tfile, "newmtl default\n");
                fprintf(tfile, "Ka %lf %lf %lf\n", materials[i].amb.r,
                        materials[i].amb.g, materials[i].amb.b);
                fprintf(tfile, "Kd %lf %lf %lf\n", materials[i].emis.r,
                        materials[i].emis.g, materials[i].emis.b);
                fprintf(tfile, "Ks %lf %lf %lf\n", materials[i].spec.r,
                        materials[i].spec.g, materials[i].spec.b);
                fprintf(tfile, "Ns %d\n", (int)materials[i].shi);
                fprintf(tfile, "map_kd %s\n", it->texture.c_str());
                break;
            }
        }
    }

    texnum = 0;
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        int texnofound = 0;
        if (it->canSkip())
            continue;

        texnofound = 1;
        for (int i = 0; i < texnum; i++)
        {
            if (!it->hasTexture())
            {
                texnofound = 0;
                break;
            }
            if (!strncmp(tex[i], it->texture.c_str(), 13))
            {
                texnofound = 0;
                break;
            }
            else
                texnofound = 1;
        }
        if (texnofound == 1)
        {
            if (it->hasTexture())
            {
                strcpy(tex[texnum], it->texture.c_str());
                tex[texnum][13] = '\0';
            }
            texnum++;
        }
        printf("name=%s x_min=%.1f y_min=%.1f x_max=%.1f y_max=%.1f\n",
                it->name.c_str(), it->x_min, it->y_min, it->x_max, it->y_max);
    }

    for (int i = 0; i < texnum; i++)
    {
        printf("analysing  %s\n", tex[i]);
        char *p = tex[i];
        char *q = name2;
        while (*p)
        {
            if ((*p <= 'Z' && *p >= 'A'))
            {
                *p = (*p - 'A') + 'a';
            }

            if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')
                    || (*p == '.'))
            {
                *q = *p;
                q++;
                *q = '\0';
            }
            p++;
        }
        int j = 0;
        while (name2[j] != '\0')
        {
            if (name2[j] == '.')
            {
                name2[j] = '\0';
                break;
            }
            j++;
            if (j == 8)
            {
                name2[j] = '\0';
                break;
            }
        }

        printf("texture file %s will be stored as %s.png\n", tex[i], name2);
    }

    computeTriNorm(objects);
    smoothTriNorm(objects);
    fprintf(ofile, "g\n");

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
        {
            fprintf(ofile, "v %lf %lf %lf\n", it->vertex[i].x, it->vertex[i].y, it->vertex[i].z);
        }
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
            fprintf(ofile, "vt %lf %lf 0.0\n", it->textarray[i].u, it->textarray[i].v);
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
        {
            fprintf(ofile, "vn %lf %lf %lf\n", it->snorm[i].x, it->snorm[i].y, it->snorm[i].z);
        }
    }
    fprintf(ofile, "g OB1\n");
    fprintf(ofile, "s 1\n");
    fprintf(ofile, "usemtl default\n");
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        ind = it->numvert;
        printf("making obj face for %s\n", it->name.c_str());

        for (int i = 0; i < it->numsurf; i++)
        {
            int v1, v2, v3;
            v1 = it->vertexarray[i * 3].indice;
            v2 = it->vertexarray[i * 3 + 1].indice;
            v3 = it->vertexarray[i * 3 + 2].indice;
            fprintf(ofile, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", v1 + deltav,
                    v1 + deltav, v1 + deltav, v2 + deltav, v2 + deltav,
                    v2 + deltav, v3 + deltav, v3 + deltav, v3 + deltav);
        }
        deltav += ind;
    }
    fprintf(ofile, "end\n");

    fclose(ofile);
	fclose(tfile);
}

void stripifyOb(FILE * ofile, ob_t * object, bool writeit)
{
    FILE *stripeout, *stripein;
    char line[256];
    char filename[50];
    char command[256];
    unsigned int NumStrips;
    unsigned int NumStripPoints;
    unsigned int CurrentStripNumber;
    unsigned int CurrentStripStart;
    unsigned int CurrentStripLength;
    unsigned int CurrentStripPoint;
    int debj = 0;
    int dege = 0;
    int k, v1, v2, v0;
    k = 0;
    int tri = 0;
    int tritotal = 0;
    bool multitex = false;

    if (object->numsurf < 3 && !writeit)
        return;
    if (object->vertexarray.size() < 3)
        return;
    fprintf(stderr, "stripifying %s                    \r", object->name.c_str());
    sprintf(filename, "temp.obj");
    stripeout = fopen(filename, "w");
    for (int i = 0; i < object->numvert; i++)
        fprintf(stripeout, "v 0.0 0.0 0.0\n");

    for (int i = 0; i < object->numsurf; i++)
    {
        fprintf(stripeout, "f %d %d %d\n",
                object->vertexarray[i * 3].indice + 1,
                object->vertexarray[i * 3 + 1].indice + 1,
                object->vertexarray[i * 3 + 2].indice + 1);
    }

    fclose(stripeout);
#ifdef _WIN32
    sprintf(command, "stripe.exe %s >shudup", filename);
    system(command);
    sprintf(command, "erase shudup");
    system(command);
    strcat(filename, "f");
    stripein = fopen(filename,"r");
#else
    sprintf(command, "stripe %s >/dev/null", filename);
    if (system(command) < 0)
        printf("Calling stripe failed");
    sprintf(command, "rm %s", filename);
    if (system(command) < 0)
        printf("Calling stripe failed");
    strcat(filename, "f");
    stripein = fopen("temp.objf", "r");
#endif

    /* Count the number of strip points and initialize PolyList */
    NumStrips = 0;
    NumStripPoints = 0;

    while (fscanf(stripein, "%255s", line) != EOF)
    {
        switch (line[0])
        {
        case '#':
        case 'v':
            /* # is a comment, v is a vertex, we ignore both */
            while (fgetc(stripein) != '\n')
                ;
            break;

        case 't':
            /* t is the start of a new triangle strip */
            NumStrips++;
            break;

        case 'q':
        case '%':
            /* q is another entry in the current strip, % is a STRIPE type */
            break;

        default:
            /* Anything else is a point in the current triangle strip */
            NumStripPoints++;
            break;
        }
    }

    if (object->hasName())
        printf("name=%s stripnumber =%u\n", object->name.c_str(), NumStrips);
    /* Allocate enough memory for what we just read */
    std::vector<unsigned int> StripPoint(NumStripPoints);
    std::vector<unsigned int> StripStart(NumStrips);
    std::vector<unsigned int> StripLength(NumStrips);

    /* Fill the triangle strip lists with the STRIPE data */
    rewind(stripein);

    CurrentStripNumber = 0;
    CurrentStripStart = 0;
    CurrentStripLength = 0;
    CurrentStripPoint = 0;

    for (unsigned int j = 0; j < NumStrips; j++)
    {
        StripStart[j] = 0;
        StripLength[j] = 0;
    }

    while (fscanf(stripein, "%255s", line) != EOF)
    {
        switch (line[0])
        {
        case '#':
        case 'v':
            /* # is a comment, v is a vertex, we ignore both */
            while (fgetc(stripein) != '\n')
                ;
            break;

        case 't':
            /* t is the start of a new triangle strip */
            if (CurrentStripNumber > 0)
            {
                StripStart[CurrentStripNumber - 1] = CurrentStripStart;
                StripLength[CurrentStripNumber - 1] = CurrentStripLength;
                printf("striplength %u\n",
                        StripLength[CurrentStripNumber - 1]);
            }
            CurrentStripNumber++;
            CurrentStripStart = CurrentStripPoint;
            CurrentStripLength = 0;
            printf("new strip\n");
            break;

        case 'q':
        case '%':
            /* q is another entry in the current strip, % is a STRIPE type */
            break;

        default:
            /* Anything else is a point in the current triangle strip */
            StripPoint[CurrentStripPoint] = (unsigned int) (atoi(line) - 1);
            CurrentStripPoint++;
            CurrentStripLength++;
            break;
        }
    }
    if (CurrentStripNumber > 0)
    {
        StripStart[CurrentStripNumber - 1] = CurrentStripStart;
        StripLength[CurrentStripNumber - 1] = CurrentStripLength;
    }

    fclose(stripein);

    std::vector<tcoord_t> stripvertexarray(object->numvertice * 10);
    k = 0;
    dege = 0;
    if (writeit)
    {
        fprintf(ofile, "numsurf %u\n", NumStrips);

    }
    if (object->hasMultiTexture())
        multitex = true;
    else
        multitex = false;

    for (unsigned int i = 0; i < NumStrips; i++)
    {
        /* get the first triangle */
        v1 = StripPoint[StripStart[i]];
        v2 = StripPoint[StripStart[i] + 1];
        debj = 2;
        tri = 0;
        if (writeit)
        {
            /** For some reason the surf attribute is modified for the output.
             *  The surfaces are made double-sided, although stripification doesn't
             *  introduce this property.
             *  Thus, instead of the whole if-condition the actual code for outputting
             *  this attribute should simply be:
             *
             *  fprintf(ofile, "SURF 0x%02x\n", object->attrSurf)
             *
             *  However this causes huge artifacts in the generated tracks. Thus, the
             *  following is not correct behavior but works for whatever reason.
             *  It is a legacy from TORCS and no details are known why it is done.
             *
             *  A proper solution would be to remove the attribute modification and
             *  rework all tracks to fit the correct behavior.
             */
            if (object->attrSurf)
            {
                fprintf(ofile, "SURF 0x%02x\n",
                        (object->attrSurf & 0xF0) | 0x04);
            }
            else
            {
                fprintf(ofile, "SURF 0x24\n");
            }
            fprintf(ofile, "mat %d\n", object->attrMat);
            fprintf(ofile, "refs %u\n", StripLength[i]);
            if (!multitex)
                fprintf(ofile, "%d %.5f %.5f\n", v1, object->textarray[v1].u, object->textarray[v1].v);
            else
            {
                fprintf(ofile, "%d %.5f %.5f", v1, object->textarray[v1].u, object->textarray[v1].v);

                if (object->hasTexture1())
                    fprintf(ofile, " %.5f %.5f", object->textarray1[v1].u, object->textarray1[v1].v);
                else if (object->hasTexture2() || object->hasTexture3())
                    fprintf(ofile, " 0.000000 0.000000");

                if (object->hasTexture2())
                    fprintf(ofile, " %.5f %.5f", object->textarray2[v1].u, object->textarray2[v1].v);
                else if (object->hasTexture3())
                    fprintf(ofile, " 0.000000 0.000000");

                if (object->hasTexture3())
                    fprintf(ofile, " %.5f %.5f", object->textarray3[v1].u, object->textarray3[v1].v);

                fprintf(ofile, "\n");
            }
            if (!multitex)
                fprintf(ofile, "%d %.5f %.5f\n", v2, object->textarray[v2].u, object->textarray[v2].v);
            else
            {
                fprintf(ofile, "%d %.5f %.5f", v2, object->textarray[v2].u, object->textarray[v2].v);

                if (object->hasTexture1())
                     fprintf(ofile, " %.5f %.5f", object->textarray1[v2].u, object->textarray1[v2].v);
                else if (object->hasTexture2() || object->hasTexture3())
                    fprintf(ofile, " 0.000000 0.000000");

                if (object->hasTexture2())
                    fprintf(ofile, " %.5f %.5f", object->textarray2[v2].u, object->textarray2[v2].v);
                else if (object->hasTexture3())
                    fprintf(ofile, " 0.000000 0.000000");

                if (object->hasTexture3())
                    fprintf(ofile, " %.5f %.5f", object->textarray3[v2].u, object->textarray3[v2].v);

                fprintf(ofile, "\n");
            }
        }
        for (unsigned int j = debj; j < StripLength[i]; j++)
        {
            v0 = StripPoint[StripStart[i] + j];
            /*printf("adding point %d\n",v0);*/

            if (!writeit)
            {
                stripvertexarray[k].indice = v1;
                stripvertexarray[k].uv = object->textarray[v1];
                stripvertexarray[k].saved = false;
                k++;
                stripvertexarray[k].indice = v2;
                stripvertexarray[k].uv = object->textarray[v2];
                stripvertexarray[k].saved = false;
                k++;
                stripvertexarray[k].indice = v0;
                stripvertexarray[k].uv = object->textarray[v0];
                stripvertexarray[k].saved = false;
                k++;
                if ((tri % 2) == 0)
                {
                    v1 = v0;
                }
                else
                {
                    v2 = v0;
                }
            }
            else
            {
                if (!multitex)
                    fprintf(ofile, "%d %.5f %.5f\n", v0, object->textarray[v0].u, object->textarray[v0].v);
                else
                {
                    fprintf(ofile, "%d %.5f %.5f", v0, object->textarray[v0].u, object->textarray[v0].v);

                    if (object->hasTexture1())
                        fprintf(ofile, " %.5f %.5f", object->textarray1[v0].u, object->textarray1[v0].v);
                    else if (object->hasTexture2() || object->hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object->hasTexture2())
                        fprintf(ofile, " %.5f %.5f", object->textarray2[v0].u, object->textarray2[v0].v);
                    else if (object->hasTexture3())
                        fprintf(ofile, " 0.000000 0.000000");

                    if (object->hasTexture3())
                        fprintf(ofile, " %.5f %.5f", object->textarray3[v0].u, object->textarray3[v0].v);

                    fprintf(ofile, "\n");
                }
            }

            tri++;
        }
        tritotal += tri;
    }

    printf("strips for %s : number of strips %u : average of points triangles by strips %.2f\n",
           object->name.c_str(), NumStrips,
           (float) ((float) tritotal - (float) dege) / ((float) NumStrips));
    if (!writeit)
    {
        if (tritotal != object->numsurf)
        {
            printf("warning: error nb surf= %d != %d  degenerated triangles %d  tritotal=%d for %s\n",
                   tritotal, object->numsurf, dege, tritotal - dege,
                   object->name.c_str());
        }
        object->vertexarray = stripvertexarray;
        object->numvertice = k;
        object->numsurf = k / 3;
    }
}

void computeSaveAC3DM(const std::string &OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials)
{
    char name2[256] = { 0 };
    int deltav = 1;
    int ind = 0;
    FILE * ofile;

    if ((ofile = fopen(OutputFilename.c_str(), "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename.c_str());
        return;
    }

    printMaterials(ofile, materials);

    for (int i = 0; i < texnum; i++)
    {
        printf("analysing  %s\n", tex[i]);
        char *p = tex[i];
        char *q = name2;
        while (*p)
        {
            if ((*p <= 'Z' && *p >= 'A'))
            {
                *p = (*p - 'A') + 'a';
            }

            if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')
                    || (*p == '.'))
            {
                *q = *p;
                q++;
                *q = '\0';
            }
            p++;
        }
        int j = 0;
        while (name2[j] != '\0')
        {
            if (name2[j] == '.')
            {
                name2[j] = '\0';
                break;
            }
            j++;
            if (j == 8)
            {
                name2[j] = '\0';
                break;
            }
        }

        printf("texture file %s will be stored as %s.png\n", tex[i], name2);
    }

    computeTriNorm(objects);
    smoothTriNorm(objects);
    fprintf(ofile, "g\n");

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
            fprintf(ofile, "v %lf %lf %lf\n", it->vertex[i].x, it->vertex[i].y, it->vertex[i].z);
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
            fprintf(ofile, "vt %lf %lf 0.0\n", it->textarray[i].u, it->textarray[i].v);
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
            fprintf(ofile, "vn %lf %lf %lf\n", it->snorm[i].x, it->snorm[i].y, it->snorm[i].z);
    }
    fprintf(ofile, "g OB1\n");
    fprintf(ofile, "s 1\n");
    fprintf(ofile, "usemtl default\n");

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        ind = it->numvert;
        printf("making obj face for %s\n", it->name.c_str());

        for (int i = 0; i < it->numsurf; i++)
        {
            const int v1 = it->vertexarray[i * 3].indice;
            const int v2 = it->vertexarray[i * 3 + 1].indice;
            const int v3 = it->vertexarray[i * 3 + 2].indice;
            fprintf(ofile, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", v1 + deltav,
                    v1 + deltav, v1 + deltav, v2 + deltav, v2 + deltav,
                    v2 + deltav, v3 + deltav, v3 + deltav, v3 + deltav);
        }
        deltav += ind;
    }
    fprintf(ofile, "end\n");
    fclose(ofile);
}

void mapNormalToSphere(std::list<ob_t> &objects)
{
    double xmin = 9999;
    double ymin = 9999;
    double zmin = 9999;
    double xmax = -9999;
    double ymax = -9999;
    double zmax = -9999;
    double pospt = 0;
    double ddmax = 0;
    double ddmin = 10000;

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        if (it->x_min < xmin)
            xmin = it->x_min;
        if (it->y_min < ymin)
            ymin = it->y_min;
        if (it->z_min < zmin)
            zmin = it->z_min;

        if (it->x_max > xmax)
            xmax = it->x_max;
        if (it->y_max > ymax)
            ymax = it->y_max;
        if (it->z_max > zmax)
            zmax = it->z_max;

        for (int i = 0; i < it->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            pospt = sqrt(it->vertex[i].x * it->vertex[i].x +
                         it->vertex[i].y * it->vertex[i].y +
                         it->vertex[i].z * it->vertex[i].z);
            if (pospt > ddmax)
                ddmax = pospt;
            if (pospt < ddmin)
                ddmin = pospt;
        }
    }
    ddmin = (ddmax - ddmin) / 2 + ddmin;
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
        {
            double fact = 0;
            /* compute the same normal for each points in the surface */
            pospt = sqrt(it->vertex[i].x * it->vertex[i].x +
                         it->vertex[i].y * it->vertex[i].y +
                         it->vertex[i].z * it->vertex[i].z);
            fact = ddmin / pospt;
            if (fact > 1.0)
                fact = 1.0;
            it->snorm[i] *= fact;
        }
    }
}

void mapTextureEnv(std::list<ob_t> &objects)
{
    double x, y, z, zt, lg;
    double z_min = 10000;
    double z_max = -10000;

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int j = 0; j < it->numvert; j++)
        {
            z = it->vertex[j].z + it->snorm[j].z / 3.0;
            if (z > z_max)
                z_max = z;
            if (z < z_min)
                z_min = z;
        }
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        /* create the new vertex array */
        it->textarray1 = it->textarray;
        it->textarray2 = it->textarray;
        it->texture1 = it->texture;
        it->texture2 = it->texture;
        for (int i = 0; i < it->numvert; i++)
        {
            x = it->vertex[i].x;
            y = it->vertex[i].y;
            z = it->vertex[i].z;
            lg = sqrt(x * x + y * y + z * z);
            if (lg != 0.0)
            {
                x /= lg;
                y /= lg;
                z /= lg;
            }
            else
            {
                x = 0;
                y = 0;
                z = 1;
            }
            //z_min = 0;
            it->textarray1[i].u = 0.5 + x / 2.0;
            zt = (z + it->snorm[i].z / 3.0 - z_min) / (z_max - z_min);
            it->textarray1[i].v = zt;

            if (it->textarray1[i].v > 1.0)
                it->textarray1[i].v = 0.999;
            else if (it->textarray1[i].v < 0.0)
                it->textarray1[i].v = 0.001;

            it->textarray2[i].u = 0.5 + y / 2.0;
            it->textarray2[i].v = z;
        }
    }
}

void mapTextureEnvOld(std::list<ob_t> &objects)
{
    double x_min = 10000;
    double x_max = -10000;
    double y_min = 10000;
    double y_max = -10000;
    double z_min = 10000;
    double z_max = -10000;
    double u_min = 10000;
    double u_max = -10000;
    double v_min = 10000;
    double v_max = -10000;

    double u2_min = 10000;
    double u2_max = -10000;
    double v2_min = 10000;
    double v2_max = -10000;

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int j = 0; j < it->numvert; j++)
        {
            if (it->vertex[j].x > x_max)
                x_max = it->vertex[j].x;
            if (it->vertex[j].x < x_min)
                x_min = it->vertex[j].x;

            if (it->vertex[j].y > y_max)
                y_max = it->vertex[j].y;
            if (it->vertex[j].y < y_min)
                y_min = it->vertex[j].y;

            if (it->vertex[j].z > z_max)
                z_max = it->vertex[j].z;
            if (it->vertex[j].z < z_min)
                z_min = it->vertex[j].z;
        }
    }
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        /* create the new vertex array */
        it->textarray1 = it->textarray;
        it->textarray2 = it->textarray;
        it->texture1 = it->texture;
        it->texture2 = it->texture;
        for (int i = 0; i < it->numvert; i++)
        {
            it->textarray1[i].u = (it->vertex[i].x - x_min)
                    / (x_max - x_min) + (it->snorm[i].x) / 2;
            it->textarray1[i].v = ((it->vertex[i].z - z_min)
                    / (z_max - z_min)) + (it->snorm[i].z) / 2;
            it->textarray2[i].u = ((it->vertex[i].x - x_min)
                    / (x_max - x_min)) + (it->snorm[i].x) / 2;
            it->textarray2[i].v = ((it->vertex[i].y - y_min)
                    / (x_max - x_min)) + (it->snorm[i].y) / 2;

            if (it->textarray1[i].u > u_max)
                u_max = it->textarray1[i].u;

            if (it->textarray1[i].v > v_max)
                v_max = it->textarray1[i].v;

            if (it->textarray1[i].u < u_min)
                u_min = it->textarray1[i].u;

            if (it->textarray1[i].v < v_min)
                v_min = it->textarray1[i].v;

            if (it->textarray2[i].u > u2_max)
                u2_max = it->textarray2[i].u;

            if (it->textarray2[i].v > v2_max)
                v2_max = it->textarray2[i].v;

            if (it->textarray2[i].u < u2_min)
                u2_min = it->textarray2[i].u;

            if (it->textarray2[i].v < v2_min)
                v2_min = it->textarray2[i].v;
        }
    }

    /* clamp the texture coord */
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
        {
            it->textarray1[i].u = (it->textarray1[i].u - u_min) / (u_max - u_min);
            it->textarray1[i].v = (it->textarray1[i].v - v_min) / (v_max - v_min);

            it->textarray2[i].u = (it->textarray2[i].u - u2_min) / (u2_max - u2_min) - 0.5;
            it->textarray2[i].v = (it->textarray2[i].v - v2_min) / (v2_max - v2_min) - 0.5;
        }
    }
}

void mapNormalToSphere2(std::list<ob_t> &objects)
{
    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int i = 0; i < it->numvert; i++)
        {
            /* compute the same normal for each points in the surface */
            /*      tmpob->norm[i] = tmpob->vertex[i]; */
            /*      normalize(&tmpob->norm[i]); */
            /*      tmpob->snorm[i] += tmpob->norm[i]; */
            normalize(it->snorm[i]);
        }
    }
}

void normalMap(std::list<ob_t> &objects)
{
    double x_min = 99999;
    double y_min = 99999;
    double z_min = 99999;
    double x_max = -99999;
    double y_max = -99999;
    double z_max = -99999;

    for (const auto &object : objects)
    {
        if (object.type != "poly" || object.numvert < 3)
            continue;

        for (int j = 0; j < object.numvert; j++)
        {
            if (object.vertex[j].x > x_max)
                x_max = object.vertex[j].x;
            if (object.vertex[j].x < x_min)
                x_min = object.vertex[j].x;

            if (object.vertex[j].y > y_max)
                y_max = object.vertex[j].y;
            if (object.vertex[j].y < y_min)
                y_min = object.vertex[j].y;

            if (object.vertex[j].z > z_max)
                z_max = object.vertex[j].z;
            if (object.vertex[j].z < z_min)
                z_min = object.vertex[j].z;
        }
    }

    for (auto &object : objects)
    {
        if (object.type != "poly" || object.numvert < 3)
            continue;

        printf("normalMap : handling %s\n", object.name.c_str());
        for (int i = 0; i < object.numvert; i++)
        {
            object.textarray[i].u = (object.vertex[i].x - x_min) / (x_max - x_min);
            object.textarray[i].v = (object.vertex[i].y - y_min) / (y_max - y_min);
        }
        object.texture = shadowtexture;
    }
}

void normalMap01(std::list<ob_t> &objects)
{
    double x_min = 99999;
    double y_min = 99999;
    double z_min = 99999;
    double x_max = -99999;
    double y_max = -99999;
    double z_max = -99999;

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        for (int j = 0; j < it->numvert; j++)
        {
            if (it->vertex[j].x > x_max)
                x_max = it->vertex[j].x;
            if (it->vertex[j].x < x_min)
                x_min = it->vertex[j].x;

            if (it->vertex[j].y > y_max)
                y_max = it->vertex[j].y;
            if (it->vertex[j].y < y_min)
                y_min = it->vertex[j].y;

            if (it->vertex[j].z > z_max)
                z_max = it->vertex[j].z;
            if (it->vertex[j].z < z_min)
                z_min = it->vertex[j].z;
        }
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->canSkip())
            continue;

        it->textarray3.resize(it->numvert);
        printf("normalMap : handling %s\n", it->name.c_str());
        for (int i = 0; i < it->numvert; i++)
        {
            it->textarray3[i].u = (it->vertex[i].x - x_min) / (x_max - x_min) - 0.5;
            it->textarray3[i].v = (it->vertex[i].y - y_min) / (y_max - y_min) - 0.5;
        }
        it->texture3 = it->texture;
    }
}

void computeSaveAC3DStrip(const std::string &OutputFilename, std::list<ob_t> &objects, const std::vector<mat_t> &materials)
{
    int numg = 0;
    char *p = NULL;
    char *q = NULL;
    bool lastpass = false;
    int nborder = 0;
    bool ordering = false;
    FILE * ofile = NULL;

    if ((ofile = fopen(OutputFilename.c_str(), "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFilename.c_str());
        return;
    }
    smoothTriNorm(objects);
    if (isobjectacar)
    {
        mapNormalToSphere2(objects);
        normalMap01(objects);

        if (extendedEnvCoord)
            mapTextureEnv(objects);
        if (collapseObject)
            mergeSplitted(objects);
    }
    fprintf(ofile, "AC3Db\n");
    printMaterials(ofile, materials);
    fprintf(ofile, "OBJECT world\n");

    if (OrderString)
    {
        fprintf(stderr, "ordering objects according to  %s\n", OrderString);
        p = OrderString;
        ordering = true;
        nborder = 1;
        while (true)
        {
            q = strstr(p, ";");
            if (q != NULL)
                nborder++;
            else
                break;
            p = q + 1;
            if (*p == '\0')
            {
                nborder--;
                break;
            }
        }
    }
    else
    {
        ordering = false;
        nborder = 0;
    }

    for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if (!it->hasName())
            continue;

        if (it->name == "world" || (it->type == "world" && it->numvert == 0 && it->numsurf == 0))
            continue;

        if (it->name == "group" || (it->type == "group" && it->numvert == 0 && it->numsurf == 0))
            continue;

        /* don't count empty objects */
        if (it->type == "poly" && it->numvert == 0 && it->numsurf == 0 && it->kids == 0)
            continue;

        numg++;
        it->saved = false;
    }

    fprintf(ofile, "kids %d\n", numg);

    texnum = 0;

    p = OrderString;
    q = OrderString;
    nborder++;
    for (int ik = 0; ik < nborder; ik++)
    {
        if (ordering)
        {
            /* look to the current object name to save */
            if (p == NULL)
                lastpass = true;
            else
            {
                q = p;
                p = strstr(p, ";");
                if (p != NULL)
                {
                    *p = '\0';
                    p++;
                }
            }
        }
        for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
        {
            int texnofound = 0;
            if (it->canSkip())
                continue;

            texnofound = 1;
            for (int i = 0; i < texnum; i++)
            {
                if (!it->hasTexture())
                {
                    texnofound = 0;
                    break;
                }
                if (!strncmp(tex[i], it->texture.c_str(), 13))
                {
                    texnofound = 0;
                    break;
                }
                else
                    texnofound = 1;
            }
            if (texnofound == 1)
            {
                if (it->hasTexture())
                {
                    strcpy(tex[texnum], it->texture.c_str());
                    tex[texnum][13] = '\0';
                    /*sprintf(tex[texnum],"%s",it->texture);*/
                }
                texnum++;
            }
            printf("name=%s x_min=%.1f y_min=%.1f x_max=%.1f y_max=%.1f\n",
                    it->name.c_str(), it->x_min, it->y_min, it->x_max, it->y_max);
        }

#if 0 // ????????????
        tmpob->kids_o = 0;
#endif

        for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
            it->inkids_o = false;

        for (std::list<ob_t>::iterator it = objects.begin(); it != objects.end(); ++it)
        {
            if (it->canSkip())
                continue;

            if (!it->saved)
            {
                if (ordering && !lastpass)
                {
                    if (it->name == q)
                    {
                        printOb(ofile, *it);
                        printf("object =%s num kids_o=%d test with %s\n", it->name.c_str(), it->kids_o, q);
                    }
                    else
                    {
                        std::string nameBuf(q);
                        nameBuf += 's';
                        if (it->name == nameBuf)
                        {
                            printOb(ofile, *it);
                            printf("object =%s num kids_o=%d\n", it->name.c_str(), it->kids_o);
                        }
                    }
                }
                else
                {
                    printOb(ofile, *it);
                    printf("object =%s num kids_o=%d\n", it->name.c_str(), it->kids_o);
                }
            }
        }
    }
    tmpIndice = 0;
    /* do the job */
    printf("\nend\n");

    fclose(ofile);
}

void mergeObject(ob_t &ob1, const ob_t &ob2, char * nameS)
{
    ob_t tobS;
    static std::vector<int> oldva;
    int n = 0;
    const int numtri = ob1.numsurf + ob2.numsurf;

    printf("merging %s with %s  tri=%d\n", ob1.name.c_str(), ob2.name.c_str(), numtri);
    oldva.resize(ob1.vertex.size() + ob2.vertex.size());
    std::fill(oldva.begin(), oldva.end(), -1);
    tobS.numsurf = ob1.numsurf;
    tobS.vertexarray.resize(numtri * 3);
    tobS.vertex.resize(ob1.vertex.size() + ob2.vertex.size());
    tobS.norm.resize(ob1.norm.size() + ob2.norm.size());
    tobS.snorm.resize(ob1.snorm.size() + ob2.snorm.size());
    tobS.textarray.resize(numtri * 3);
    tobS.textarray1.resize(numtri * 3);
    tobS.textarray2.resize(numtri * 3);
    tobS.textarray3.resize(numtri * 3);

    std::copy_n(ob1.vertex.begin(), ob1.vertex.size(), tobS.vertex.begin());
    std::copy_n(ob1.norm.begin(), ob1.norm.size(), tobS.norm.begin());
    std::copy_n(ob1.snorm.begin(), ob1.snorm.size(), tobS.snorm.begin());
    std::copy_n(ob1.vertexarray.begin(), ob1.vertexarray.size(), tobS.vertexarray.begin());
    std::copy_n(ob1.textarray.begin(), ob1.textarray.size(), tobS.textarray.begin());

    if (ob1.hasTexture1())
        std::copy_n(ob1.textarray1.begin(), ob1.textarray1.size(), tobS.textarray1.begin());

    if (ob1.hasTexture2())
        std::copy_n(ob1.textarray2.begin(), ob1.textarray2.size(), tobS.textarray2.begin());

    if (ob1.hasTexture3())
        std::copy_n(ob1.textarray3.begin(), ob1.textarray3.size(), tobS.textarray3.begin());

    n = ob1.numvert;
    for (int i = 0; i < ob2.numvert; i++)
    {
        for (int j = 0; j < ob1.numvert; j++)
        {
            if (ob2.vertex[i] == ob1.vertex[j] && ob2.textarray[i] == ob1.textarray[j])
            {
                oldva[i] = j;
            }
        }
    }

    for (int i = 0; i < ob2.numvert; i++)
    {
        if (oldva[i] == -1)
        {
            oldva[i] = n;
            tobS.textarray[n] = ob2.textarray[i];
            if (ob2.hasTexture1())
                tobS.textarray1[n] = ob2.textarray1[i];
            if (ob2.hasTexture2())
                tobS.textarray2[n] = ob2.textarray2[i];
            if (ob2.hasTexture3())
                tobS.textarray3[n] = ob2.textarray3[i];
            tobS.snorm[n] = ob2.snorm[i];
            tobS.norm[n] = ob2.norm[i];
            tobS.vertex[n] = ob2.vertex[i];

            n++;
        }
    }
    tobS.numvert = n;
    for (int i = 0; i < ob2.numsurf; i++)
    {
        bool found = false;
        for (int j = 0; j < ob1.numsurf; j++)
        {
            if (tobS.vertexarray[j * 3].indice == oldva[ob2.vertexarray[i * 3].indice] &&
                tobS.vertexarray[j * 3 + 1].indice == oldva[ob2.vertexarray[i * 3 + 1].indice] &&
                tobS.vertexarray[j * 3 + 2].indice == oldva[ob2.vertexarray[i * 3 + 2].indice])
            {
                /* this face is OK */
                found = true;
                break;
            }
        }
        if (!found)
        {
            const int k = tobS.numsurf;
            /* add the triangle */
            tobS.vertexarray[k * 3].indice = oldva[ob2.vertexarray[i * 3].indice];
            tobS.vertexarray[k * 3 + 1].indice = oldva[ob2.vertexarray[i * 3 + 1].indice];
            tobS.vertexarray[k * 3 + 2].indice = oldva[ob2.vertexarray[i * 3 + 2].indice];
            tobS.numsurf++;
        }
    }

    ob1.numsurf = tobS.numsurf;
    ob1.numvert = tobS.numvert;
    ob1.vertex = tobS.vertex;
    ob1.norm = tobS.norm;
    ob1.snorm = tobS.snorm;
    ob1.vertexarray = tobS.vertexarray;
    ob1.textarray = tobS.textarray;
    ob1.textarray1 = tobS.textarray1;
    ob1.textarray2 = tobS.textarray2;
    ob1.textarray3 = tobS.textarray3;
}

int mergeSplitted(std::list<ob_t> &objects)
{
    int k = 0;
    int reduced = 0;

    for (std::list<ob_t>::iterator tob = objects.begin(); tob != objects.end(); ++tob)
    {
        // skip objects that are not split
        if (isobjectacar)
        {
            if (!tob->nameHasStr("_s_"))
                continue;
        }
        else if (!tob->nameHasStr("__split__"))
            continue;

        // get base name of split object
        char nameS[256];
        char* p;
        sprintf(nameS, "%s", tob->name.c_str());
        if (isobjectacar)
            p = strstr(nameS, "_s_");
        else
            p = strstr(nameS, "__split__");
        if (p == NULL)
            continue;
        printf("looking for merge : %s\n", nameS);
        if (isobjectacar)
            p = p + strlen("_s_");
        else
            p = p + strlen("__split__");
        *p = '\0';

        k = 0;
        std::list<ob_t>::iterator tob0 = tob;
        for (++tob0; tob0 != objects.end();)
        {
            if (tob0->canSkip() || tob0->type == "group")
            {
                ++tob0;
                continue;
            }

            if (tob0->nameStartsWith(nameS))
            {
                mergeObject(*tob, *tob0, nameS);
                printf("merging %s with %s\n",nameS, tob0->name.c_str());
                reduced++;
                tob0 = objects.erase(tob0);
                k++;
                continue;
            }
            ++tob0;
        }

        if (k == 0)
            continue;

        /* we know that nameS has k+1 objects and need to be merged */
        printf("need merge for %s : %d objects found\n", tob->name.c_str(), k + 1);
    }

    return reduced;
}

#define P2(x) ((x)*(x))

double findDistmin(const ob_t &ob1, const ob_t &ob2)
{
    double di;
    double d = 100000;

    for (int i = 0; i < ob1.numvert; i++)
    {
        for (int j = 0; j < ob2.numvert; j++)
        {
            const double a1 = ob1.vertex[i].x;
            const double b1 = ob1.vertex[i].y;
            const double a2 = ob2.vertex[j].x;
            const double b2 = ob2.vertex[j].y;
            di = P2(a1-a2) + P2(b1-b2);
            if (di < d)
                d = di;
        }
    }

    return d;
}

void printMaterials(FILE *file, const std::vector<mat_t> &materials)
{
    for (size_t j = 0, end = materials.size(); j < end; j++) //for (const auto & material : materials)
    {
        const mat_t &material = materials[j];
        fprintf(file,
            "MATERIAL \"%s\" rgb %1.2f %1.2f %1.2f  amb %1.2f %1.2f %1.2f  emis %1.2f %1.2f %1.2f  spec %1.2f %1.2f %1.2f  shi %d  trans 0\n",
            material.name.c_str(), material.rgb.r, material.rgb.g, material.rgb.b, material.amb.r,
            material.amb.g, material.amb.b, material.emis.r, material.emis.g,
            material.emis.b, material.spec.r, material.spec.g, material.spec.b,
            (int)material.shi);
    }
}
