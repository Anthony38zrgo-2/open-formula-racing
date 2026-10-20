/***************************************************************************

 file        : accc.cpp
 created     : Fri Apr 18 23:10:53 CEST 2003
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

    @author	Christophe Guionneau
*/


#include <cstdio>
#include <cctype>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "accc.h"

/* -g  g-track-2.ac -l0 g2.ac -d3 200900 -d2 200 -d1 100 -S 200 >t */
/* +es 80 p-gt1.ac porsche-gt1.ac>t */
double distSplit = 0;
static std::string InputFileName;
static std::string OutputFileName;
char * fileL0 = NULL;
char * fileL1 = NULL;
char * fileL2 = NULL;
char * fileL3 = NULL;
char * OrderString = NULL;
double d1 = 0;
double d2 = 0;
double d3 = 0;
bool extendedStrips = false;
bool extendedEnvCoord = false;
bool isobjectacar = false;
bool extendedTriangles = false;
bool notexturesplit = false;
bool collapseObject = false;
bool splitObjectsDuringLoad = true;
conv_t typeConvertion = _UNSPECIFIED;
double far_dist = 10000;
bool normalMapping = false;
double smooth_angle = 70.0;

void usage(void)
{
    fprintf(stderr, "Ac3d Compiler $Revision: 1.4.10 $\n");
    fprintf(stderr, "Usage: accc [option] <inputfile> <outputfile> [dist][-order \"orderstring\"]<\n");
    fprintf(stderr, "+o: ac3d to ac3d : the result file is optimized\n");
    fprintf(stderr, "                   in using groups by zone of track\n");
    fprintf(stderr, "                   track must be named TKMNxxx\n");
    fprintf(stderr, "                   dist is used to determine the distance of the objects\n");
    fprintf(stderr, "+om: ac3d to ac3d : the result is a vertex merged unique obj\n" );
    fprintf(stderr, "                    for smooth car only\n");
    fprintf(stderr, "-o: ac3d to obj\n");
    fprintf(stderr, "+s: ac3d to ac3d with strips (used for cars)\n");
    fprintf(stderr, "+es <angle_of_smooth>: ac3d to ac3d with strips (used for cars) and extended ac3d file output\n");
    fprintf(stderr, "    (computed normals) angle is in degrees : example : +es 80.0 clkdtm_o.ac clkdtm.ac\n");
    fprintf(stderr, "    the output file will also contains the secondary textcoord for the env sliding\n");
    fprintf(stderr, "+et: ac3d to ac3d with triangles (used for cars) and extended ac3d file output (normals normals)\n");
    fprintf(stderr, "+etn: ac3d to ac3d with triangles no split and extended ac3d file output (normals normals)\n");
    fprintf(stderr, "+shad: ac3d to ac3d with triangles, create a new ac3d file used for track shading\n");
    fprintf(stderr, "    vertical mapping of a single texture\n");
    fprintf(stderr, "<inputfile>:\n");
    fprintf(stderr, "<oututfile>:\n");
    fprintf(stderr, "-order \"orderstring\": only used with +es , +s and  +et\n");
    fprintf(stderr, "       \"orderstring\" is the order of the objects during the save\n");
    fprintf(stderr, "        for example : \"HB;OB;OBM\" is the names of the objet separate\n");
    fprintf(stderr, "        by a semicolon\n");
    fprintf(stderr, "        for imported NFS cars the object name \"HB\" must be used instead\n");
    fprintf(stderr, "        of \":HB\"\n");
    fprintf(stderr, "-g <outputfile>: for grouping multilayer ac3d track\n");
    fprintf(stderr, "     -l0 <input filename> : the base geometry\n");
    fprintf(stderr, "     -l1 <input filename> : the base geometry map-tiled\n");
    fprintf(stderr, "     -l2 <input filename> : the base geometry skids/and grass details\n");
    fprintf(stderr, "     -l3 <input filename> : the base geometry shadows\n");
    fprintf(stderr, "     -d3 <val> : the max distance of groups in tkmn\n");
    fprintf(stderr, "     -d2 <val> : the distance of group 2 in tkmn\n");
    fprintf(stderr, "     -d1 <val> : the distance of group 1 in tkmn\n");
    fprintf(stderr, "     -S <val> : the distance of splitting for TERRAIN* objects\n");
    fprintf(stderr, "     -es : for extended ac3d file format for strips\n");
    fprintf(stderr, "     -et : for extended ac3d file format for triangles\n");
    fprintf(stderr, "     -nts : no texture split (for debug)\n");
}

void init_args(int argc, char **argv)
{
    if (argc < 4)
    {
        usage();
        exit(1);
    }
    if (!strcmp(argv[1], "-o"))
    {
        typeConvertion = _AC3DTOOBJ;
    }
    else if (!strcmp(argv[1], "+o"))
    {
        typeConvertion = _AC3DTOAC3D;
    }
    else if (!strcmp(argv[1], "+om"))
    {
        typeConvertion = _AC3DTOAC3DM;
    }
    else if (!strcmp(argv[1], "+s"))
    {
        typeConvertion = _AC3DTOAC3DS;
    }
    else if (!strcmp(argv[1], "+es"))
    {
        typeConvertion = _AC3DTOAC3DS;
        isobjectacar = true;
        extendedStrips = true;
        extendedEnvCoord = true;
        collapseObject = true;
        if (sscanf(argv[2], "%lf", &smooth_angle) != 1)
        {
            fprintf(stderr, "invalid +es %s\n", argv[2]);
            exit(1);
        }
        if (argc < 4)
        {
            fprintf(stderr, "missing input file name\n");
            exit(1);
        }
        InputFileName = argv[3];
        if (argc < 5)
        {
            fprintf(stderr, "missing output file name\n");
            exit(1);
        }
        OutputFileName = argv[4];
    }
    else if (!strcmp(argv[1], "+et"))
    {
        typeConvertion = _AC3DTOAC3D;
        extendedStrips = false;
        isobjectacar = true;
        extendedEnvCoord = true;
        extendedTriangles = true;
        collapseObject = true;
    }
    else if (!strcmp(argv[1], "+etn"))
    {
        typeConvertion = _AC3DTOAC3D;
        extendedStrips = false;
        isobjectacar = true;
        extendedEnvCoord = true;
        notexturesplit = false;
        collapseObject = true;
        extendedTriangles = true;
    }
    else if (!strcmp(argv[1], "+shad"))
    {
        typeConvertion = _AC3DTOAC3D;
        extendedStrips = false;
        isobjectacar = false;
        notexturesplit = true;
        extendedTriangles = false;
        normalMapping = true;
        collapseObject = true;
    }
    else if (!strcmp(argv[1], "-g"))
    {
        typeConvertion = _AC3DTOAC3DGROUP;
    }
    else
    {
        usage();
        exit(1);
    }

    if (typeConvertion == _AC3DTOAC3DGROUP)
    {
        int i = 3;
        /* read other arguments */
        OutputFileName = argv[2];
        for (i = 3; i < argc; i++)
        {
            if (!strcmp(argv[i], "-es"))
            {
                extendedStrips = true;
                continue;
            }

            if (!strcmp(argv[i], "-et"))
            {
                extendedTriangles = true;
                continue;
            }

            if (!strcmp(argv[i], "-nts"))
            {
                notexturesplit = true;
                continue;
            }

            if (!strcmp(argv[i], "-S"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                if (sscanf(argv[i], "%lf", &distSplit) != 1)
                {
                    fprintf(stderr, "invalid -S %s\n", argv[i]);
                    exit(1);
                }
                continue;
            }

            if (!strcmp(argv[i], "-l0"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                fileL0 = argv[i];
                continue;
            }
            if (!strcmp(argv[i], "-l1"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                fileL1 = argv[i];
                continue;
            }
            if (!strcmp(argv[i], "-l2"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                fileL2 = argv[i];
                continue;
            }
            if (!strcmp(argv[i], "-l3"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                fileL3 = argv[i];
                continue;
            }
            if (!strcmp(argv[i], "-d3"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                if (sscanf(argv[i], "%lf", &d3) != 1)
                {
                    fprintf(stderr, "invalid -d3 %s\n", argv[i]);
                    exit(1);
                }
                continue;
            }
            if (!strcmp(argv[i], "-d2"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                if (sscanf(argv[i], "%lf", &d2) != 1)
                {
                    fprintf(stderr, "invalid -d2 %s\n", argv[i]);
                    exit(1);
                }
                continue;
            }
            if (!strcmp(argv[i], "-d1"))
            {
                if (argc <= i + 1)
                {
                    usage();
                    exit(-1);
                }
                i++;
                if (sscanf(argv[i], "%lf", &d1) != 1)
                {
                    fprintf(stderr, "invalid -d1 %s\n", argv[i]);
                    exit(1);
                }
                continue;
            }
        }
    }
    else
    {
        if (InputFileName.empty())
            InputFileName = argv[2];
        if (OutputFileName.empty())
            OutputFileName = argv[3];

        if (argc == 7)
        {
            if (!strcmp(argv[5], "-order"))
                OrderString = argv[6];
        }
        else if (typeConvertion == _AC3DTOAC3D)
        {
            if (argc >= 5)
            {
                if (!strcmp(argv[4], "-order"))
                    OrderString = argv[5];
                else if (sscanf(argv[4], "%lf", &far_dist) != 1)
                {
                    fprintf(stderr, "invalid -order %s\n", argv[4]);
                    exit(1);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    init_args(argc, argv);

    if (typeConvertion == _AC3DTOAC3DGROUP)
    {
        loadAndGroup(OutputFileName);
    }
    else
    {
        std::list<ob_t> objects;
        std::vector<mat_t> materials;

        loadAC(InputFileName, objects, materials, OutputFileName);
    }

    return 0;
}
