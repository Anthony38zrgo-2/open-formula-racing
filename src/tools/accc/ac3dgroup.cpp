/***************************************************************************

 file        : ac3dgroup.cpp
 created     : Fri Apr 18 23:11:36 CEST 2003
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
#include <cmath>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "accc.h"

void collapseTextures(std::list<ob_t> &ob0, std::list<ob_t> &ob1, std::list<ob_t> &ob2, std::list<ob_t> &ob3);

// check if materials need to be merged
bool materialNeedsMerge(const std::vector<mat_t> &mat1, const std::vector<mat_t> &mat2)
{
    if (mat2.size() > mat1.size())
        return true;
    for (size_t i = 0, end = mat2.size(); i < end; ++i)
    {
        if (mat1[i] != mat2[i])
            return true;
    }
    return false;
}

void loadAndGroup(const std::string &OutputFileName)
{
    std::list<ob_t> ob0;
    std::list<ob_t> ob1;
    std::list<ob_t> ob2;
    std::list<ob_t> ob3;
    std::vector<mat_t> mat0;
    std::vector<mat_t> mat1;
    std::vector<mat_t> mat2;
    std::vector<mat_t> mat3;
    FILE * ofile = NULL;
    int num_tkmn = 0;
    int good_group = 0;
    int i = 0;
    double dist = 0;

    /* disable object splitting during load. We split them
     * after merging the texture channels.
     */
    splitObjectsDuringLoad = false;

    if (fileL0)
    {
        fprintf(stderr, "\nloading file %s\n", fileL0);
        if (loadAC(fileL0, ob0, mat0) == -1)
        {
            fprintf(stderr, "\ncouldn't load file %s\n", fileL0);
            return;
        }
    }
    if (fileL1)
    {
        fprintf(stderr, "\nloading file %s\n", fileL1);
        if (loadAC(fileL1, ob1, mat1) == -1)
        {
            fprintf(stderr, "\ncouldn't load file %s\n", fileL1);
            return;
        }
    }
    if (fileL2)
    {
        fprintf(stderr, "\nloading file %s\n", fileL2);
        if (loadAC(fileL2, ob2, mat2) == -1)
        {
            fprintf(stderr, "\ncouldn't load file %s\n", fileL2);
            return;
        }
    }
    if (fileL3)
    {
        fprintf(stderr, "\nloading file %s\n", fileL3);
        if (loadAC(fileL3, ob3, mat3) == -1)
        {
            fprintf(stderr, "\ncouldn't load file %s\n", fileL3);
            return;
        }
    }

    printf("smoothing triangle normals\n");
    fprintf(stderr, "\nsmoothing triangle normals\n");

    smoothTriNorm(ob0);

    printf("collapsing textures\n");
    fprintf(stderr, "collapsing textures\n");

    /* now collapse the texture and texture arrays of 1 2 3 in 0 */
    collapseTextures(ob0, ob1, ob2, ob3);

    fprintf(stderr, "splitting objects\n");
    splitObjects(ob0);

    /* now make groups from ob0 */

    fprintf(stderr, "making groups\n");
    num_tkmn = 0;
    for (std::list<ob_t>::iterator it = ob0.begin(); it != ob0.end(); ++it)
    {
        if (it->canSkip())
            continue;
        if (it->nameStartsWith("tkmn") && it->type != "group")
        {
            num_tkmn++;
            continue;
        }
    }

    printf("found %d tkmn\n", num_tkmn);
    if (num_tkmn == 0)
    {
        fprintf(stderr,
                "\nERROR: cannot find any object tkmn for grouping\nAborting\n");
        exit(-1);
    }

    i = 0;
    std::vector<ob_groups_t> array_groups(num_tkmn);
    for (std::list<ob_t>::iterator it = ob0.begin(); it != ob0.end(); ++it)
    {
        if (it->canSkip())
            continue;
        if (it->nameStartsWith("tkmn") && it->type != "group")
        {
            array_groups[i].tkmn = &(*it);
            array_groups[i].name = it->name;
            array_groups[i].tkmnlabel = atoi(it->name.c_str() + 4);
            i++;
            continue;
        }
    }

    fprintf(stderr, "dispatching objects in groups\n");
    for (std::list<ob_t>::iterator it = ob0.begin(); it != ob0.end();)
    {
        if (it->canSkip() || it->nameStartsWith("tkmn"))
        {
            ++it;
            continue;
        }

        good_group = -1;
        it->dist_min = 1000000;
        for (i = 0; i < num_tkmn; i++)
        {
            dist = findDistmin(*array_groups[i].tkmn, *it);
            if (dist < it->dist_min)
            {
                it->dist_min = dist;
                good_group = i;
            }
            if (it->nameStartsWith("t0RB") ||
                it->nameStartsWith("t1RB") ||
                it->nameStartsWith("t2RB") ||
                it->nameStartsWith("tkRS") ||
                it->nameStartsWith("t0LB") ||
                it->nameStartsWith("t1LB") ||
                it->nameStartsWith("t2LB") ||
                it->nameStartsWith("tkLS") ||
                it->nameStartsWith("BOLt") ||
                it->nameStartsWith("BORt"))
            {
                if (atoi(it->name.c_str() + 4) == array_groups[i].tkmnlabel)
                {
                    printf("object %s is forced in group %d\n", it->name.c_str(),
                            array_groups[i].tkmnlabel);
                    good_group = i;
                    break;
                }
            }
        }
        if (good_group == -1)
        {
            printf("an object in no group %s\n", it->name.c_str());
            ++it;
            continue;
        }
        printf("object %s is going to group %s  at dist=%f\n", it->name.c_str(),
                array_groups[good_group].name.c_str(), sqrt(it->dist_min));

        // move object from ob0 to array_group
        std::list<ob_t>::iterator other = it++;
        array_groups[good_group].kids.splice(array_groups[good_group].kids.begin(), ob0, other);
    }
    /* now each tkmn group contains the list of its kids */

    /* for all tkmn groups the kids are dispatched in the different group levels */

    for (i = 0; i < num_tkmn; i++)
    {
        printf("grouping level for %s\n", array_groups[i].name.c_str());
        for (std::list<ob_t>::iterator it = array_groups[i].kids.begin(); it != array_groups[i].kids.end();)
        {
            std::list<ob_t>::iterator other = it++;
            if (!other->hasName())
                continue;
            if (other->nameStartsWith("tkrb"))
            {
                printf("inserting %s in group 0 of %s\n", other->name.c_str(),
                       array_groups[i].name.c_str());
                array_groups[i].kids0.splice(array_groups[i].kids0.begin(), array_groups[i].kids, other);
                continue;
            }
            if (other->nameStartsWith("tklb"))
            {
                printf("inserting %s in group 0 of %s\n", other->name.c_str(),
                       array_groups[i].name.c_str());
                array_groups[i].kids0.splice(array_groups[i].kids0.begin(), array_groups[i].kids, other);
                continue;
            }
            if (other->nameStartsWith("tkrs"))
            {
                printf("inserting %s in group 0 of %s\n", other->name.c_str(),
                        array_groups[i].name.c_str());
                array_groups[i].kids0.splice(array_groups[i].kids0.begin(), array_groups[i].kids, other);
                continue;
            }
            if (other->nameStartsWith("tkls"))
            {
                printf("inserting %s in group 0 of %s\n", other->name.c_str(),
                       array_groups[i].name.c_str());
                array_groups[i].kids0.splice(array_groups[i].kids0.begin(), array_groups[i].kids, other);
                continue;
            }

            if (other->dist_min < d1 * d1)
            {
                printf("inserting %s in group 1 of %s\n", other->name.c_str(),
                       array_groups[i].name.c_str());
                array_groups[i].kids1.splice(array_groups[i].kids1.begin(), array_groups[i].kids, other);
            }
            else if (other->dist_min < d2 * d2)
            {
                printf("inserting %s in group 2 of %s\n", other->name.c_str(),
                       array_groups[i].name.c_str());
                array_groups[i].kids2.splice(array_groups[i].kids2.begin(), array_groups[i].kids, other);
            }
            else if (other->dist_min < d3 * d3)
            {
                printf("inserting %s in group 3 of %s\n", other->name.c_str(),
                       array_groups[i].name.c_str());
                array_groups[i].kids3.splice(array_groups[i].kids3.begin(), array_groups[i].kids, other);
            }
            else
            {
                printf("cannot insert object %s in group %s\n", other->name.c_str(), array_groups[i].name.c_str());
            }
        }

        printf("in group %s\n", array_groups[i].name.c_str());
        printf("    found in l0  %d\n", static_cast<int>(array_groups[i].kids0.size()));
        printf("    found in l1  %d\n", static_cast<int>(array_groups[i].kids1.size()));
        printf("    found in l2  %d\n", static_cast<int>(array_groups[i].kids2.size()));
        printf("    found in l3  %d\n", static_cast<int>(array_groups[i].kids3.size()));
        printf("    staying kids  %d\n", static_cast<int>(array_groups[i].kids.size()) - 1); /* because of the tkmn not moved */
    }

    for (i = 0; i < num_tkmn; i++)
    {
        if (array_groups[i].kids3.size() > 0)
            mergeSplitted(array_groups[i].kids3);
        if (array_groups[i].kids2.size() > 0)
            mergeSplitted(array_groups[i].kids2);
        if (array_groups[i].kids1.size() > 0)
            mergeSplitted(array_groups[i].kids1);
    }

    fprintf(stderr, "writing destination file %s\n", OutputFileName.c_str());

    if ((ofile = fopen(OutputFileName.c_str(), "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFileName.c_str());
        return;
    }
    fprintf(ofile, "AC3Db\n");
    printMaterials(ofile, mat0);
    fprintf(ofile, "OBJECT world\n");
    fprintf(ofile, "kids %d\n", num_tkmn);

    for (i = 0; i < num_tkmn; i++)
    {
        int numg = 0;
        fprintf(ofile, "OBJECT group\n");
        fprintf(ofile, "name \"%s_g\"\n", array_groups[i].tkmn->name.c_str());
        numg = (array_groups[i].kids3.empty() ? 0 : 1) +
               (array_groups[i].kids2.empty() ? 0 : 1) +
               (array_groups[i].kids1.empty() ? 0 : 1) + 1;
        fprintf(ofile, "kids %d\n", numg);

        if (array_groups[i].kids3.size() > 0)
        {
            fprintf(ofile, "OBJECT group\n");
            fprintf(ofile, "name \"___%s_gl3\"\n", array_groups[i].tkmn->name.c_str());
            fprintf(ofile, "kids %d\n", static_cast<int>(array_groups[i].kids3.size()));
            printf("writing group: ___%s_gl3\n", array_groups[i].tkmn->name.c_str());
            for (std::list<ob_t>::iterator it = array_groups[i].kids3.begin(); it != array_groups[i].kids3.end(); ++it)
            {
                printOb(ofile, *it);
                printf("%s\n", it->name.c_str());
            }
        }

        if (array_groups[i].kids2.size() > 0)
        {
            fprintf(ofile, "OBJECT group\n");
            fprintf(ofile, "name \"%%___%s_gl2\"\n", array_groups[i].tkmn->name.c_str());
            fprintf(ofile, "kids %d\n", static_cast<int>(array_groups[i].kids2.size()));
            printf("writing group: ___%s_gl2\n", array_groups[i].tkmn->name.c_str());
            for (std::list<ob_t>::iterator it = array_groups[i].kids2.begin(); it != array_groups[i].kids2.end(); ++it)
            {
                printOb(ofile, *it);
                printf("%s\n", it->name.c_str());
            }
        }
        if (array_groups[i].kids1.size() > 0)
        {
            fprintf(ofile, "OBJECT group\n");
            fprintf(ofile, "name \"___%s_gl1\"\n", array_groups[i].tkmn->name.c_str());
            fprintf(ofile, "kids %d\n", static_cast<int>(array_groups[i].kids1.size()));
            printf("writing group: ___%s_gl1\n", array_groups[i].tkmn->name.c_str());
            for (std::list<ob_t>::iterator it = array_groups[i].kids1.begin(); it != array_groups[i].kids1.end(); ++it)
            {
                printOb(ofile, *it);
                printf("%s\n", it->name.c_str());
            }
        }

        /* there is always a group 0 with the tkmn at leat */
        fprintf(ofile, "OBJECT group\n");
        fprintf(ofile, "name \"___%s_gl0\"\n", array_groups[i].tkmn->name.c_str());
        fprintf(ofile, "kids %d\n", static_cast<int>(array_groups[i].kids0.size() + 1));
        printf("writing group: ___%s_gl0\n", array_groups[i].tkmn->name.c_str());
        for (std::list<ob_t>::iterator it = array_groups[i].kids0.begin(); it != array_groups[i].kids0.end(); ++it)
        {
            printOb(ofile, *it);
            printf("%s\n", it->name.c_str());
        }
        printOb(ofile, *array_groups[i].tkmn);
    }

    fclose(ofile);

    return;
}

void reorder(ob_t &ob, ob_t &ob2, std::vector<uv_t> &textarray, std::vector<tcoord_t> &vertexarray)
{
    int k = 0;

    for (int i = 0; i < ob.numvert; i++)
    {
        if (ob.vertex[i] != ob2.vertex[i])
        {
            for (int j = 0; j < ob.numvert; j++)
            {
                if (ob.vertex[i] == ob2.vertex[j])
                {
                    k++;

                    point_t p = ob2.vertex[i];
                    ob2.vertex[i] = ob2.vertex[j];
                    ob2.vertex[j] = p;

                    tcoord_t t = vertexarray[i];
                    vertexarray[i] = vertexarray[j];
                    vertexarray[j] = t;

                    uv_t text = textarray[i];
                    textarray[i] = textarray[j];
                    textarray[j] = text;
                }
            }
        }
    }
    printf("%s : reordered %d points\n", ob.name.c_str(), k);
    return;
}

/** collapse the given tiledob into the texture channel 1 of tarobj */
void collapseMapTiledTextures(ob_t &tarob, std::list<ob_t> &tiledob);
/** collapse the given skidsob into the texture channel 2 of tarobj */
void collapseSkidsGrassTextures(ob_t &tarob, std::list<ob_t> &skidsob);
/** collapse the given shadob into the texture channel 3 of tarobj */
void collapseShadowTextures(ob_t &tarob, std::list<ob_t> &shadob);

/** Match textures from ob1, ob2 and ob3 with ob0. In case a match is found
 *  add them as additional texture channels in ob0.
 */
void collapseTextures(std::list<ob_t> &ob0, std::list<ob_t> &ob1, std::list<ob_t> &ob2, std::list<ob_t> &ob3)
{
    for (std::list<ob_t>::iterator it = ob0.begin(); it != ob0.end(); ++it)
    {
        if (!it->isNamedAndPolygon())
            continue;

        collapseMapTiledTextures(*it, ob1);
        collapseSkidsGrassTextures(*it, ob2);
        collapseShadowTextures(*it, ob3);
    }
}

/** copy the texture, textarray and vertexarray properties of srcob
 *  into the corresponding places in destob based on the given channel.
 *  The channel may be 1,2 or 3.
 */
void copyTextureChannel(ob_t &destob, const ob_t &srcob, int channel)
{
    if (channel == 1)
    {
        destob.texture1 = srcob.texture;
        destob.textarray1 = srcob.textarray;
        destob.vertexarray1 = srcob.vertexarray;
    }
    else if (channel == 2)
    {
        destob.texture2 = srcob.texture;
        destob.textarray2 = srcob.textarray;
        destob.vertexarray2 = srcob.vertexarray;
    }
    else if (channel == 3)
    {
        destob.texture3 = srcob.texture;
        destob.textarray3 = srcob.textarray;
        destob.vertexarray3 = srcob.vertexarray;
    }
}

bool notInSameOrder(const ob_t &ob1, const ob_t &ob2)
{
    for (int i = 0; i < ob1.numvert; i++)
    {
        if (fabs(ob1.vertex[i].x - ob2.vertex[i].x) > MINVAL ||
            fabs(ob1.vertex[i].y - ob2.vertex[i].y) > MINVAL ||
            fabs(ob1.vertex[i].z - ob2.vertex[i].z) > MINVAL)
        {
            return true;
        }
    }
    return false;
}

void collapseMapTiledTextures(ob_t &tarob, std::list<ob_t> &tiledob)
{
    for (std::list<ob_t>::iterator it = tiledob.begin(); it != tiledob.end(); ++it)
    {
        if (it->isNamedAndPolygon())
        {
            if (it->isSamePoly(tarob))
            {
                copyTextureChannel(tarob, *it, 1);
                if (notInSameOrder(tarob, *it))
                {
                    printf("%s : points not in the same order, reordering ...\n", tarob.name.c_str());
                    reorder(tarob, *it, tarob.textarray1, tarob.vertexarray1);
                    printf("%s : reordering ... done\n", tarob.name.c_str());
                }
                break;
            }
        }
    }
}

void collapseSkidsGrassTextures(ob_t &tarob, std::list<ob_t> &skidsob)
{
    for (std::list<ob_t>::iterator it = skidsob.begin(); it != skidsob.end(); ++it)
    {
        if (it->isNamedAndPolygon())
        {
            if (it->isSamePoly(tarob))
            {
                copyTextureChannel(tarob, *it, 2);
                if (notInSameOrder(tarob, *it))
                {
                    printf("%s : points not in the same order\n", tarob.name.c_str());
                    reorder(tarob, *it, tarob.textarray2, tarob.vertexarray2);
                    printf("%s : reordering ... done\n", tarob.name.c_str());
                }
                break;
            }
        }
    }
}

void collapseShadowTextures(ob_t &tarob, std::list<ob_t> &shadob)
{
    for (std::list<ob_t>::iterator it = shadob.begin(); it != shadob.end(); ++it)
    {
        if (it->isNamedAndPolygon())
        {
            if (it->isSamePoly(tarob))
            {
                copyTextureChannel(tarob, *it, 3);
                if (notInSameOrder(tarob, *it))
                {
                    printf("%s : points not in the same order\n", tarob.name.c_str());
                    reorder(tarob, *it, tarob.textarray3, tarob.vertexarray3);
                    printf("%s : reordering ... done\n", tarob.name.c_str());
                }
                break;
            }
        }
    }
}
