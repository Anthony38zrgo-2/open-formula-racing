/***************************************************************************

    file                 : ac3d.cpp
    created              : Wed May 29 22:11:21 CEST 2002
    copyright            : (C) 2001 by Eric Espie
    email                : eric.espie@torcs.org

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

    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
*/

#include "ac3d.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <set>
#include <cmath>
#include <cctype>

#include <tgf.hpp>

//------------------------------------- V3d -----------------------------------

Ac3d::V3d Ac3d::V3d::operator + (const V3d &other) const
{
    return V3d((*this)[0] + other[0], (*this)[1] + other[1], (*this)[2] + other[2]);
}

Ac3d::V3d Ac3d::V3d::operator - (const V3d &other) const
{
    return V3d((*this)[0] - other[0], (*this)[1] - other[1], (*this)[2] - other[2]);
}

Ac3d::V3d Ac3d::V3d::operator/(double scalar) const
{
    return V3d((*this)[0] / scalar, (*this)[1] / scalar, (*this)[2] / scalar);
}

Ac3d::V3d Ac3d::V3d::operator += (const V3d &other)
{
    (*this)[0] += other[0];
    (*this)[1] += other[1];
    (*this)[2] += other[2];
    return *this;
}

Ac3d::V3d Ac3d::V3d::operator /= (double scalar)
{
    (*this)[0] /= scalar;
    (*this)[1] /= scalar;
    (*this)[2] /= scalar;
    return *this;
}

double Ac3d::V3d::length() const
{
    return sqrt((*this)[0] * (*this)[0] + (*this)[1] * (*this)[1] + (*this)[2] * (*this)[2]);
}

void Ac3d::V3d::normalize()
{
    const double l = length();
    if (l != 0.0)
        *this = *this / length();
}

double Ac3d::V3d::dot(const V3d &other) const
{
    return (*this)[0] * other[0] + (*this)[1] * other[1] + (*this)[2] * other[2];
}

Ac3d::V3d Ac3d::V3d::cross(const V3d &other) const
{
    return V3d{ (*this)[1] * other[2] - (*this)[2] * other[1],
                (*this)[2] * other[0] - (*this)[0] * other[2],
                (*this)[0] * other[1] - (*this)[1] * other[0] };
}

//----------------------------------- Color -----------------------------------

//---------------------------------- Material ---------------------------------

Ac3d::Material::Material(const std::vector<std::string> &tokens)
{
    if (tokens.size() != 22)
        throw Exception("Invalid material");

    if (tokens[2] != "rgb" || tokens[6] != "amb" || tokens[10] != "emis" ||
        tokens[14] != "spec" || tokens[18] != "shi" || tokens[20] != "trans")
        throw Exception("Invalid material");

    name = tokens[1];
    rgb[0] = std::stod(tokens[3]);
    rgb[1] = std::stod(tokens[4]);
    rgb[2] = std::stod(tokens[5]);
    amb[0] = std::stod(tokens[7]);
    amb[1] = std::stod(tokens[8]);
    amb[2] = std::stod(tokens[9]);
    emis[0] = std::stod(tokens[11]);
    emis[1] = std::stod(tokens[12]);
    emis[2] = std::stod(tokens[13]);
    spec[0] = std::stod(tokens[15]);
    spec[1] = std::stod(tokens[16]);
    spec[2] = std::stod(tokens[17]);
    shi = std::stoi(tokens[19]);
    trans = std::stod(tokens[21]);
}

Ac3d::Material::Material(std::ifstream &fin, const std::string &name) : name(name)
{
    std::string line;

    while (std::getline(fin, line))
    {
        std::vector<std::string> tokens;

        tokenizeLine(line, tokens);

        if (tokens.empty())
            continue;
        if (tokens.at(0) == "ENDMAT")
            break;
        if (tokens.at(0) == "rgb")
        {
            rgb[0] = std::stod(tokens.at(1));
            rgb[1] = std::stod(tokens.at(2));
            rgb[2] = std::stod(tokens.at(3));
        }
        else if (tokens.at(0) == "amb")
        {
            amb[0] = std::stod(tokens.at(1));
            amb[1] = std::stod(tokens.at(2));
            amb[2] = std::stod(tokens.at(3));
        }
        else if (tokens.at(0) == "emis")
        {
            emis[0] = std::stod(tokens.at(1));
            emis[1] = std::stod(tokens.at(2));
            emis[2] = std::stod(tokens.at(3));
        }
        else if (tokens.at(0) == "spec")
        {
            spec[0] = std::stod(tokens.at(1));
            spec[1] = std::stod(tokens.at(2));
            spec[2] = std::stod(tokens.at(3));
        }
        else if (tokens.at(0) == "shi")
            shi = std::stoi(tokens.at(1));
        else if (tokens.at(0) == "trans")
            trans = std::stod(tokens.at(1));
        else if (tokens.at(0) == "data")
        {
            const size_t length = std::stoi(tokens.at(1));
            std::string text;
            while (getline(fin, text))
            {
                data += text;
                if (data.size() >= length)
                    break;
            }
        }
        else
            throw Exception("Invalid AC3D file");
    }
}

void Ac3d::Material::write(std::ofstream &fout, bool versionC) const
{
    if (versionC)
    {
        fout << "MAT \"" << name << "\"" << std::endl;
        fout << "rgb " << rgb[0] << " " << rgb[1] << " " << rgb[2] << std::endl;
        fout << "amb " << amb[0] << " " << amb[1] << " " << amb[2] << std::endl;
        fout << "emis " << emis[0] << " " << emis[1] << " " << emis[2] << std::endl;
        fout << "spec " << spec[0] << " " << spec[1] << " " << spec[2] << std::endl;
        fout << "shi " << shi << std::endl;
        fout << "trans " << trans << std::endl;
        fout << "ENDMAT" << std::endl;
    }
    else
    {
        fout << "MATERIAL \"" << name << "\""
             << " rgb " << rgb[0] << " " << rgb[1] << " " << rgb[2]
             << "  amb " << amb[0] << " " << amb[1] << " " << amb[2]
             << "  emis " << emis[0] << " " << emis[1] << " " << emis[2]
             << "  spec " << spec[0] << " " << spec[1] << " " << spec[2]
             << "  shi " << shi << "  trans " << trans << std::endl;
    }
}

bool Ac3d::Material::same(const Material &material) const
{
    return rgb == material.rgb &&
           amb == material.amb &&
           emis == material.emis &&
           spec == material.spec &&
           shi == material.shi &&
           trans == material.trans;
}

//-------------------------------- Surface ------------------------------------

Ac3d::Surface::Surface(std::ifstream &fin)
{
    std::string line;

    while (std::getline(fin, line))
    {
        std::vector<std::string> tokens;

        tokenizeLine(line, tokens);

        if (tokens.empty())
            continue;
        if (tokens.at(0) == "SURF")
            surf = static_cast<SURF>(std::stoi(tokens.at(1), nullptr, 16));
        else if (tokens.at(0) == "mat")
            mat = std::stoi(tokens.at(1));
        else if (tokens.at(0) == "refs")
        {
            const int numRefs = std::stoi(tokens.at(1));
            refs.resize(numRefs);
            for (int i = 0; i < numRefs; )
            {
                tokens.clear();
                std::getline(fin, line);
                if (line.empty())
                    continue;
                std::istringstream iss1(line);

                std::copy(std::istream_iterator<std::string>(iss1),
                          std::istream_iterator<std::string>(),
                          std::back_inserter(tokens));

                refs[i].index = std::stoi(tokens.at(0));
                double u = std::stod(tokens.at(1));
                double v = std::stod(tokens.at(2));
                refs[i].coords.emplace_back(u, v);

                if (tokens.size() >= 5)
                {
                    u = std::stod(tokens.at(1));
                    v = std::stod(tokens.at(2));
                    refs[i].coords.emplace_back(u, v);

                    if (tokens.size() >= 7)
                    {
                        u = std::stod(tokens.at(1));
                        v = std::stod(tokens.at(2));
                        refs[i].coords.emplace_back(u, v);

                        if (tokens.size() >= 9)
                        {
                            u = std::stod(tokens.at(1));
                            v = std::stod(tokens.at(2));
                            refs[i].coords.emplace_back(u, v);
                        }
                    }
                }
                i++;
            }
            break;
        }
        else
            throw Exception("Invalid AC3D file");
    }
}

void Ac3d::Surface::write(std::ofstream &fout, const Object &object) const
{
    fout << "SURF 0x" << std::hex << surf << std::dec << std::endl;
    fout << "mat " << mat << std::endl;
    fout << "refs " << refs.size() << std::endl;
    for (const auto &ref : refs)
    {
        if (!ref.coords.empty())
            fout << ref.index << " " << ref.coords[0][0] << " " << ref.coords[0][1];
        else
            fout << ref.index << " 0 0";

        if (ref.coords.size() >= 2)
        {
            fout << " " << ref.coords[1][0] << " " << ref.coords[1][1];

            if (ref.coords.size() >= 3)
            {
                fout << " " << ref.coords[2][0] << " " << ref.coords[2][1];

                if (ref.coords.size() == 4)
                {
                    fout << " " << ref.coords[3][0] << " " << ref.coords[3][1];
                }
            }
        }

        fout << std::endl;
    }
}

//--------------------------------- Matrix ------------------------------------

Ac3d::Matrix::Matrix()
{
    makeIdentity();
}

Ac3d::Matrix::Matrix(double m0, double m1, double m2, double m3,
    double m4, double m5, double m6, double m7,
    double m8, double m9, double m10, double m11,
    double m12, double m13, double m14, double m15)
{
    (*this)[0][0] = m0;  (*this)[0][1] = m1;  (*this)[0][2] = m2;  (*this)[0][3] = m3;
    (*this)[1][0] = m4;  (*this)[1][1] = m5;  (*this)[1][2] = m6;  (*this)[1][3] = m7;
    (*this)[2][0] = m8;  (*this)[2][1] = m9;  (*this)[2][2] = m10; (*this)[2][3] = m11;
    (*this)[3][0] = m12; (*this)[3][1] = m13; (*this)[3][2] = m14; (*this)[3][3] = m15;
}

void Ac3d::Matrix::setLocation(const v3 &location)
{
    (*this)[3][0] = location[0];
    (*this)[3][1] = location[1];
    (*this)[3][2] = location[2];
}

void Ac3d::Matrix::setLocation(double x, double y, double z)
{
    (*this)[3][0] = x;
    (*this)[3][1] = y;
    (*this)[3][2] = z;
}

void Ac3d::Matrix::setRotation(const std::array<double, 9> &rotation)
{
    (*this)[0][0] = rotation[0]; (*this)[0][1] = rotation[1]; (*this)[0][2] = rotation[2];
    (*this)[1][0] = rotation[3]; (*this)[1][1] = rotation[4]; (*this)[1][2] = rotation[5];
    (*this)[2][0] = rotation[6]; (*this)[2][1] = rotation[7]; (*this)[2][2] = rotation[8];
}

void Ac3d::Matrix::setRotation(double x, double y, double z)
{
    double cx, sx, cy, sy, cz, sz, szsy, czsy, szcy;

    if (x == 0)
    {
        cx = 1;
        sx = 0;
    }
    else
    {
        const double ax = x * M_PI / 180.0;
        sx = sin(ax);
        cx = cos(ax);
    }

    if (y == 0)
    {
        cy = 1;
        sy = 0;
    }
    else
    {
        const double ay = y * PI / 180.0;
        sy = sin(ay);
        cy = cos(ay);
    }

    if (z == 0)
    {
        cz = 1;
        sz = 0;
        szsy = 0;
        szcy = 0;
        czsy = sy;
    }
    else
    {
        const double az = z * PI / 180.0;
        sz = sin(az);
        cz = cos(az);
        szsy = sz * sy;
        czsy = cz * sy;
        szcy = sz * cy;
    }

    (*this)[0][0] = cx * cz - sx * szsy;
    (*this)[1][0] = -sx * cy;
    (*this)[2][0] = sz * cx + sx * czsy;
    (*this)[3][0] = 0;

    (*this)[0][1] = cz * sx + szsy * cx;
    (*this)[1][1] = cx * cy;
    (*this)[2][1] = sz * sx - czsy * cx;
    (*this)[3][1] = 0;

    (*this)[0][2] = -szcy;
    (*this)[1][2] = sy;
    (*this)[2][2] = cz * cy;
    (*this)[3][2] = 0;

    (*this)[0][3] = 0;
    (*this)[1][3] = 0;
    (*this)[2][3] = 0;
    (*this)[3][3] = 1;
}

void Ac3d::Matrix::setScale(double scale)
{
    (*this)[0][0] = scale;
    (*this)[1][1] = scale;
    (*this)[2][2] = scale;
    (*this)[3][2] = 1;
}

void Ac3d::Matrix::setScale(double x, double y, double z)
{
    (*this)[0][0] = x;
    (*this)[1][1] = y;
    (*this)[2][2] = z;
    (*this)[3][2] = 1;
}

void Ac3d::Matrix::makeIdentity()
{
    (*this)[0][0] = 1; (*this)[0][1] = 0; (*this)[0][2] = 0; (*this)[0][3] = 0;
    (*this)[1][0] = 0; (*this)[1][1] = 1; (*this)[1][2] = 0; (*this)[1][3] = 0;
    (*this)[2][0] = 0; (*this)[2][1] = 0; (*this)[2][2] = 1; (*this)[2][3] = 0;
    (*this)[3][0] = 0; (*this)[3][1] = 0; (*this)[3][2] = 0; (*this)[3][3] = 1;
}

void Ac3d::Matrix::makeLocation(const v3 &location)
{
    (*this)[0][0] = 1; (*this)[0][1] = 0; (*this)[0][2] = 0; (*this)[0][3] = 0;
    (*this)[1][0] = 0; (*this)[1][1] = 1; (*this)[1][2] = 0; (*this)[1][3] = 0;
    (*this)[2][0] = 0; (*this)[2][1] = 0; (*this)[2][2] = 1; (*this)[2][3] = 0;
    (*this)[3][0] = location[0];
    (*this)[3][1] = location[1];
    (*this)[3][2] = location[2];
    (*this)[3][3] = 1;
}

void Ac3d::Matrix::makeLocation(double x, double y, double z)
{
    (*this)[0][0] = 1; (*this)[0][1] = 0; (*this)[0][2] = 0; (*this)[0][3] = 0;
    (*this)[1][0] = 0; (*this)[1][1] = 1; (*this)[1][2] = 0; (*this)[1][3] = 0;
    (*this)[2][0] = 0; (*this)[2][1] = 0; (*this)[2][2] = 1; (*this)[2][3] = 0;
    (*this)[3][0] = x; (*this)[3][1] = y; (*this)[3][2] = z; (*this)[3][3] = 1;
}

void Ac3d::Matrix::makeRotation(const std::array<double, 9> &rotation)
{
    (*this)[0][0] = rotation[0]; (*this)[0][1] = rotation[1]; (*this)[0][2] = rotation[2]; (*this)[0][3] = 0;
    (*this)[1][0] = rotation[3]; (*this)[1][1] = rotation[4]; (*this)[1][2] = rotation[5]; (*this)[1][3] = 0;
    (*this)[2][0] = rotation[6]; (*this)[2][1] = rotation[7]; (*this)[2][2] = rotation[8]; (*this)[2][3] = 0;
    (*this)[3][0] = 0; (*this)[3][1] = 0; (*this)[3][2] = 0; (*this)[3][3] = 1;
}

void Ac3d::Matrix::makeRotation(double x, double y, double z)
{
    makeIdentity();
    setRotation(x, y, z);
}

void Ac3d::Matrix::makeScale(double scale)
{
    (*this)[0][0] = scale; (*this)[0][1] = 0; (*this)[0][2] = 0; (*this)[0][3] = 0;
    (*this)[1][0] = 0; (*this)[1][1] = scale; (*this)[1][2] = 0; (*this)[1][3] = 0;
    (*this)[2][0] = 0; (*this)[2][1] = 0; (*this)[2][2] = scale; (*this)[2][3] = 0;
    (*this)[3][0] = 0; (*this)[3][1] = 0; (*this)[3][2] = 0; (*this)[3][3] = 1;
}

void Ac3d::Matrix::makeScale(double x, double y, double z)
{
    (*this)[0][0] = x; (*this)[0][1] = 0; (*this)[0][2] = 0; (*this)[0][3] = 0;
    (*this)[1][0] = 0; (*this)[1][1] = y; (*this)[1][2] = 0; (*this)[1][3] = 0;
    (*this)[2][0] = 0; (*this)[2][1] = 0; (*this)[2][2] = z; (*this)[2][3] = 0;
    (*this)[3][0] = 0; (*this)[3][1] = 0; (*this)[3][2] = 0; (*this)[3][3] = 1;
}

void Ac3d::Matrix::transformPoint(V3d &point) const
{
    V3d dst;

    const double t0 = point[0];
    const double t1 = point[1];
    const double t2 = point[2];

    dst[0] = t0 * (*this)[0][0] + t1 * (*this)[1][0] + t2 * (*this)[2][0] + (*this)[3][0];
    dst[1] = t0 * (*this)[0][1] + t1 * (*this)[1][1] + t2 * (*this)[2][1] + (*this)[3][1];
    dst[2] = t0 * (*this)[0][2] + t1 * (*this)[1][2] + t2 * (*this)[2][2] + (*this)[3][2];

    point = dst;
}

void Ac3d::Matrix::transformNormal(V3d &normal) const
{
    V3d dst;

    const double t0 = normal[0];
    const double t1 = normal[1];
    const double t2 = normal[2];

    dst[0] = t0 * (*this)[0][0] + t1 * (*this)[1][0] + t2 * (*this)[2][0];
    dst[1] = t0 * (*this)[0][1] + t1 * (*this)[1][1] + t2 * (*this)[2][1];
    dst[2] = t0 * (*this)[0][2] + t1 * (*this)[1][2] + t2 * (*this)[2][2];

    normal = dst;
}

Ac3d::Matrix Ac3d::Matrix::multiply(const Matrix &matrix)
{
    Matrix result;

    for (size_t j = 0; j < 4; j++)
    {
        result[0][j] = matrix[0][0] * (*this)[0][j] +
                       matrix[0][1] * (*this)[1][j] +
                       matrix[0][2] * (*this)[2][j] +
                       matrix[0][3] * (*this)[3][j];

        result[1][j] = matrix[1][0] * (*this)[0][j] +
                       matrix[1][1] * (*this)[1][j] +
                       matrix[1][2] * (*this)[2][j] +
                       matrix[1][3] * (*this)[3][j];

        result[2][j] = matrix[2][0] * (*this)[0][j] +
                       matrix[2][1] * (*this)[1][j] +
                       matrix[2][2] * (*this)[2][j] +
                       matrix[2][3] * (*this)[3][j];

        result[3][j] = matrix[3][0] * (*this)[0][j] +
                       matrix[3][1] * (*this)[1][j] +
                       matrix[3][2] * (*this)[2][j] +
                       matrix[3][3] * (*this)[3][j];
    }

    return result;
}

//------------------------------- BoundingBox ---------------------------------

void Ac3d::BoundingBox::extend(const V3d &vertex)
{
    for (size_t i = 0; i < 3; i++)
    {
        if (vertex[i] > max[i])
            max[i] = vertex[i];
        if (vertex[i] < min[i])
            min[i] = vertex[i];
    }
}

void Ac3d::BoundingBox::extend(const BoundingBox &boundingBox)
{
    for (size_t i = 0; i < 3; i++)
    {
        if (boundingBox.max[i] > max[i])
            max[i] = boundingBox.max[i];
        if (boundingBox.min[i] < min[i])
            min[i] = boundingBox.min[i];
    }
}

bool Ac3d::BoundingBox::pointInside(double x, double y) const
{
    return x <= max[0] && x >= min[0] && y <= max[1] && y >= min[1];
}

//------------------------------- Boundingsphere ------------------------------

void Ac3d::BoundingSphere::extend(const BoundingBox &boundingBox)
{
    const V3d half = (boundingBox.max - boundingBox.min) / 2;
    center = boundingBox.min + half;
    radius = half.length();
}

//---------------------------------- Object -----------------------------------

Ac3d::Object::Object(std::ifstream &fin)
{
    std::string line;

    while (std::getline(fin, line))
    {
        std::vector<std::string> tokens;

        tokenizeLine(line, tokens);

        if (tokens.empty())
            continue;
        if (tokens.at(0) == "OBJECT")
        {
            parse(fin, tokens.at(1));
            return;
        }

        throw Exception("Invalid AC3D file");
    }
}

void Ac3d::Object::parse(std::ifstream &fin, const std::string &objType)
{
    type = objType;

    std::string line;

    while (std::getline(fin, line))
    {
        std::vector<std::string> tokens;

        tokenizeLine(line, tokens);

        if (tokens.empty())
            continue;
        if (tokens.at(0) == "name")
            name = tokens.at(1);
        else if (tokens.at(0) == "data")
        {
            const size_t length = std::stoi(tokens.at(1));
            std::string text;
            while (getline(fin, text))
            {
                data += text;
                if (data.size() >= length)
                    break;
            }
        }
        else if (tokens.at(0) == "texture")
        {
            if (tokens.size() > 2)
                textures.emplace_back(tokens.at(1), tokens.at(2));
            else
                textures.emplace_back(tokens.at(1));
        }
        else if (tokens.at(0) == "texrep")
        {
            texrep.initialized = true;
            texrep[0] = std::stod(tokens.at(1));
            texrep[1] = std::stod(tokens.at(2));
        }
        else if (tokens.at(0) == "texoff")
        {
            texoff.initialized = true;
            texoff[0] = std::stod(tokens.at(1));
            texoff[1] = std::stod(tokens.at(2));
        }
        else if (tokens.at(0) == "subdiv")
        {
            subdiv.initialized = true;
            subdiv = std::stoi(tokens.at(1));
        }
        else if (tokens.at(0) == "crease")
        {
            crease.initialized = true;
            crease = std::stod(tokens.at(1));
        }
        else if (tokens.at(0) == "rot")
        {
            rot.initialized = true;
            for (size_t i = 0; i < 9; i++)
                rot[i] = std::stod(tokens[i + 1]);
        }
        else if (tokens.at(0) == "loc")
        {
            loc.initialized = true;
            loc[0] = std::stod(tokens.at(1));
            loc[1] = std::stod(tokens.at(2));
            loc[2] = std::stod(tokens.at(3));
        }
        else if (tokens.at(0) == "url")
            url = tokens.at(1);
        else if (tokens.at(0) == "hidden")
            hidden = true;
        else if (tokens.at(0) == "locked")
            locked = true;
        else if (tokens.at(0) == "folded")
            folded = true;
        else if (tokens.at(0) == "numvert")
        {
            const int numvert = std::stoi(tokens.at(1));
            for (int i = 0; i < numvert; )
            {
                tokens.clear();
                std::getline(fin, line);
                if (line.empty())
                    continue;

                tokenizeLine(line, tokens);

                vertices.emplace_back(std::stod(tokens.at(0)), std::stod(tokens.at(1)), std::stod(tokens.at(2)));

                if (tokens.size() == 6)
                    normals.emplace_back(std::stod(tokens.at(3)), std::stod(tokens.at(4)), std::stod(tokens.at(5)));

                i++;
            }
        }
        else if (tokens.at(0) == "numsurf")
        {
            const int numsurf = std::stoi(tokens.at(1));
            for (int i = 0; i < numsurf; i++)
            {
                surfaces.emplace_back(fin);

                Surface &surface = surfaces.back();

                if (surface.isPolygon() && surface.refs.size() >= 3)
                {
                    const V3d &p0 = vertices[surface.refs[0].index];
                    const V3d &p1 = vertices[surface.refs[1].index];
                    const V3d &p2 = vertices[surface.refs[2].index];

                    surface.normal = ((p1 - p0).cross(p2 - p1));
                    surface.normal.normalize();
                }
            }
        }
        else if (tokens.at(0) == "kids")
        {
            const int numKids = std::stoi(tokens.at(1));
            for (int i = 0; i < numKids; i++)
            {
                const std::streambuf::pos_type len = fin.tellg();
                std::string peekLine;
                std::getline(fin, peekLine);
                std::vector<std::string> peekTokens;
                tokenizeLine(peekLine, peekTokens);
                fin.seekg(len, std::ios_base::beg);
                if (peekTokens.empty() || peekTokens[0] != "OBJECT")
                {
                    // this is a common problem with accc generated files so ignore it
                    // throw Exception("Invalid AC3D file: wrong number of kids");
                    return;
                }
                kids.emplace_back(fin);
            }
            return;
        }
    }
}

void Ac3d::Object::write(std::ofstream &fout, bool all, bool acc) const
{
    fout << "OBJECT " << type << std::endl;
    if (!name.empty())
    {
        fout << "name \"" << name << "\"" << std::endl;
    }
    if (all && !data.empty())
    {
        fout << "data " << data.length() << std::endl;
        fout << data << std::endl;
    }
    if (type == "poly")
    {
        if (!acc && !textures.empty())
        {
            fout << "texture \"" << textures[0].name << "\"" << std::endl;
        }
        else
        {
            for (size_t i = 0; i < textures.size(); i++)
            {
                const std::string types[4] = { "base", "tiled", "skids", "shad" };

                if (textures.size() > 2 && textures[i].name == "empty_texture_no_mapping")
                {
                    fout << "texture " << textures[i].name << " "
                         << (textures[i].type.empty() ? types[i] : textures[i].type) << std::endl;
                }
                else
                {
                    fout << "texture \"" << textures[i].name << "\" "
                         << (textures[0].type.empty() ? types[i] : textures[i].type) << std::endl;
                }
            }
        }
        if (texrep.initialized)
            fout << "texrep " << texrep[0] << " " << texrep[1] << std::endl;
        if (texoff.initialized)
            fout << "texoff " << texoff[0] << " " << texoff[1] << std::endl;
        if (all && subdiv.initialized)
            fout << "subdiv " << subdiv.value << std::endl;
        if (all && crease.initialized)
            fout << "crease " << crease.value << std::endl;
    }
    if (rot.initialized)
    {
        fout << "rot "
             << rot[0] << " " << rot[1] << " " << rot[2] << " "
             << rot[3] << " " << rot[4] << " " << rot[5] << " "
             << rot[6] << " " << rot[7] << " " << rot[8] << std::endl;
    }
    if (loc.initialized)
    {
        fout << "loc " << loc[0] << " " << loc[1] << " " << loc[2] << std::endl;
    }
    if (all && !url.empty())
        fout << "url " << url << std::endl;
    if (all && hidden)
        fout << "hidden" << std::endl;
    if (all && locked)
        fout << "locked" << std::endl;
    if (all && folded)
        fout << "folded" << std::endl;
    if (type == "poly")
    {
        if (!vertices.empty())
        {
            fout << "numvert " << vertices.size() << std::endl;
            for (size_t i = 0; i < vertices.size(); i++)
            {
                fout << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2];
                if (normals.size() == vertices.size())
                    fout << " " << normals[i][0] << " " << normals[i][1] << " " << normals[i][2];
                fout << std::endl;
            }
        }
        if (!surfaces.empty())
        {
            fout << "numsurf " << surfaces.size() << std::endl;
            for (const auto &surface : surfaces)
                surface.write(fout, *this);
        }
    }
    fout << "kids " << kids.size() << std::endl;
    for (const auto &kid : kids)
        kid.write(fout, all, acc);
}

void Ac3d::Object::transform(const Matrix &matrix)
{
    Matrix thisMatrix;

    if (loc.initialized)
    {
        thisMatrix.setLocation(loc);
        loc.initialized = false;
    }
    loc[0] = 0;
    loc[1] = 0;
    loc[2] = 0;

    if (rot.initialized)
    {
        thisMatrix.setRotation(rot);
        rot.initialized = false;
    }

    rot[0] = 1; rot[1] = 0; rot[2] = 0;
    rot[3] = 0; rot[4] = 1; rot[5] = 0;
    rot[6] = 0; rot[7] = 0; rot[8] = 1;

    const Matrix newMatrix = thisMatrix.multiply(matrix);

    if (type == "poly")
    {
        for (auto &vertex : vertices)
            newMatrix.transformPoint(vertex);
    }
    else
    {
        for (auto &kid : kids)
            kid.transform(newMatrix);
    }
}

void Ac3d::Object::flipAxes(bool in)
{
    if (type == "poly")
    {
        for (auto &vertex : vertices)
        {
            const double y = vertex[1];
            const double z = vertex[2];
            if (in)
            {
                vertex[1] = -z;
                vertex[2] = y;
            }
            else
            {
                vertex[1] = z;
                vertex[2] = -y;
            }
        }
    }
    else
    {
        for (auto &kid : kids)
            kid.flipAxes(in);
    }
}

void Ac3d::Object::removeSurfacesNotSURF(unsigned int SURF)
{
    for (std::list<Surface>::iterator it = surfaces.begin(); it != surfaces.end();)
    {
        if (it->surf != SURF)
            it = surfaces.erase(it);
        else
            ++it;
    }
}

void Ac3d::Object::removeSurfacesNotMaterial(int material)
{
    for (std::list<Surface>::iterator it = surfaces.begin(); it != surfaces.end();)
    {
        if (it->mat != material)
            it = surfaces.erase(it);
        else
            ++it;
    }
}

void Ac3d::Object::removeUnusedVertices()
{
    std::set<int> usedIndex;
    for (auto & surface: surfaces)
    {
        for (auto &ref : surface.refs)
        {
            usedIndex.insert(ref.index);
        }
    }
    int originalIndex = 0;
    for (size_t i = 0; i < vertices.size(); originalIndex++)
    {
        if (usedIndex.find(originalIndex) == usedIndex.end())
        {
            vertices.erase(vertices.begin() + i);
        }
        else
        {
            for (auto &surface: surfaces)
            {
                for (auto &ref : surface.refs)
                {
                    if (ref.index == originalIndex)
                        ref.index = static_cast<int>(i);
                }
            }
            i++;
        }
    }
}

void Ac3d::Object::splitBySURF()
{
    if (type == "poly")
        return;

    for (std::list<Object>::iterator it = kids.begin(); it != kids.end(); ++it)
    {
        Object &kid = *it;

        if (kid.type == "poly")
        {
            // get the different SURFs
            std::set<int> surfTypes;
            for (auto surface : kid.surfaces)
                surfTypes.insert(surface.surf);

            if (surfTypes.size() > 1)
            {
                const std::list<Object>::iterator last = it;
                std::list<Object>::iterator first = last;

                // add the new objects to the kids
                for (size_t i = 1; i < surfTypes.size(); i++)
                    first = kids.insert(first, Object(kid));

                std::set<int>::iterator it1 = surfTypes.begin();
                int i = 0;
                for (std::list<Object>::iterator it2 = first; it2 != std::next(last, 1); ++it2, ++it1)
                {
                    // make name unique
                    it2->name = it2->name + std::to_string(i++);

                    // remove all the others SURFs
                    it2->removeSurfacesNotSURF(*it1);

                    // remove the unused vertices
                    it2->removeUnusedVertices();
                }
                it = last;
            }
        }
        else
            kid.splitBySURF();
    }
}

void Ac3d::Object::splitByMaterial()
{
    if (type == "poly")
        return;

    for (std::list<Object>::iterator it = kids.begin(); it != kids.end(); ++it)
    {
        Object &kid = *it;

        if (kid.type == "poly")
        {
            // get the different SURFs
            std::set<int> materialTypes;
            for (auto surface : kid.surfaces)
                materialTypes.insert(surface.surf);

            if (materialTypes.size() > 1)
            {
                const std::list<Object>::iterator last = it;
                std::list<Object>::iterator first = last;

                // add the new objects to the kids
                for (size_t i = 1; i < materialTypes.size(); i++)
                    first = kids.insert(first, Object(kid));

                std::set<int>::iterator it1 = materialTypes.begin();
                int i = 0;
                for (std::list<Object>::iterator it2 = first; it2 != std::next(last, 1); ++it2, ++it1)
                {
                    // make name unique
                    it2->name = it2->name + std::to_string(i++);

                    // remove all the others SURFs
                    it2->removeSurfacesNotMaterial(*it1);

                    // remove the unused vertices
                    it2->removeUnusedVertices();
                }
                it = last;
            }
        }
        else
            kid.splitByMaterial();
    }
}

void Ac3d::Object::removeEmptyObjects()
{
    auto kid = kids.begin();
    while (kid != kids.end())
    {
        if (kid->type == "poly" && (kid->vertices.empty() || kid->surfaces.empty()))
            kid = kids.erase(kid);
        else if (kid->type == "group" && kid->kids.empty())
            kid = kids.erase(kid);
        else
            ++kid;
    }
}

void Ac3d::Object::removeBadTriangles()
{
    if (type == "poly")
    {
        auto surface = surfaces.begin();
        while (surface != surfaces.end())
        {
            if (surface->isBadTriangle(*this))
                surface = surfaces.erase(surface);
            else
                ++surface;
        }
        return;
    }
    for (auto &kid : kids)
        kid.removeBadTriangles();
}

void Ac3d::Object::splitByUV()
{
    if (type == "poly")
        return;

    for (std::list<Object>::iterator it = kids.begin(); it != kids.end(); ++it)
    {
        Object &kid = *it;
        if (kid.type == "poly")
        {
            bool needSplit = false;
            std::vector<std::set<V2d>> uvs(vertices.size());
            for (const auto &surface : surfaces)
            {
                for (const auto &ref : surface.refs)
                {
                    uvs[ref.index].insert(ref.coords[0]);

                    if (uvs[ref.index].size() > 1)
                        needSplit = true;
                }
            }

            if (needSplit)
            {
                // TODO
            }
        }
        else
            kid.splitByUV();
    }
}

const Ac3d::BoundingBox &Ac3d::Object::getBoundingBox() const
{
    if (type == "poly")
    {
        if (!hasBoundingBox)
        {
            for (const auto &vertex : vertices)
                boundingBox.extend(vertex);

            hasBoundingBox = true;
        }
    }
    else
    {
        for (const auto &kid : kids)
            boundingBox.extend(kid.getBoundingBox());
    }

    return boundingBox;
}

const Ac3d::BoundingSphere &Ac3d::Object::getBoundingSphere() const
{
    if (!hasBoundingSphere)
    {
        boundingSphere.extend(getBoundingBox());
        hasBoundingSphere = true;
    }

    return boundingSphere;
}

void Ac3d::Object::remapMaterials(bool mergeMaterials, const MaterialMap &materialMap)
{
    if (type == "poly")
    {
        for (auto &surface : surfaces)
        {
            if (mergeMaterials)
                surface.mat = static_cast<int>(materialMap.find(surface.mat)->second);
            else
                surface.mat = 0;
        }
    }
    else
    {
        for (auto &kid : kids)
            kid.remapMaterials(mergeMaterials, materialMap);
    }
}

void Ac3d::Object::generateTriangles()
{
    if (type == "poly")
    {
        for (std::list<Surface>::iterator it = surfaces.begin(); it != surfaces.end(); )
        {
            if (!it->isPolygon())
            {
                ++it;
                continue;
            }

            if (it->refs.size() > 3)
            {
                const std::list<Surface>::iterator original = it;
                const size_t count = it->refs.size() - 3;
                for (size_t i = 0; i < count; i++)
                {
                    Surface surface;
                    surface.mat = it->mat;
                    surface.surf = it->surf;
                    surface.refs.resize(3);
                    surface.refs[0] = it->refs[0];
                    surface.refs[1] = it->refs[i + 2];
                    surface.refs[2] = it->refs[i + 3];
                    surfaces.insert(++it, surface);
                }
                original->refs.resize(3);
            }
            else if (it->refs.size() < 3)
            {
                throw Exception("Invalid polygon");
            }
            else
                ++it;
        }
    }
    else
    {
        for (auto &kid : kids)
            kid.generateTriangles();
    }
}

Ac3d::V3d Ac3d::normalizedNormal(const V3d &p0, const V3d &p1, const V3d &p2)
{
    V3d  normal((p1 - p0).cross(p2 - p1));

    normal.normalize();

    return normal;
}

Ac3d::V3d Ac3d::unnormalizedNormal(const V3d &p0, const V3d &p1, const V3d &p2)
{
    return ((p1 - p0).cross(p2 - p1));
}

bool Ac3d::collinear(const V3d &p1, const V3d &p2, const V3d &p3)
{
    constexpr double epsilon = static_cast<double>(std::numeric_limits<float>::epsilon());
    const V3d v = V3d{ p2 - p1 }.cross(p3 - p1);
    return std::fabs(v.x()) < epsilon &&
           std::fabs(v.y()) < epsilon &&
           std::fabs(v.z()) < epsilon;
}

void Ac3d::Object::generateNormals()
{
    if (type == "poly")
    {
        struct Vertex
        {
            V3d vertex = { 0.0, 0.0, 0.0 };
            V3d normal = { 0.0, 0.0, 0.0 };

            bool equals(const Vertex &other) const
            {
                return (vertex.equals(other.vertex) && normal.equals(other.normal));
            }
        };
        struct Triangle
        {
            Triangle() = delete;
            Triangle(Surface::SURF surf, int mat,
                const V3d &v0, const V3d &v1, const V3d &v2,
                const std::vector<V2d> &uv0, const std::vector<V2d> &uv1, const std::vector<V2d> &uv2)
                : m_surf(surf), m_mat(mat)
            {
                m_vertices[0] = v0;
                m_vertices[1] = v1;
                m_vertices[2] = v2;
                m_coordinates[0] = uv0;
                m_coordinates[1] = uv1;
                m_coordinates[2] = uv2;
                if (isFlatShaded())
                {
                    m_normal = normalizedNormal(v0, v1, v2);
                    m_normals[0].emplace_back(m_normal);
                    m_normals[1].emplace_back(m_normal);
                    m_normals[2].emplace_back(m_normal);
                }
                else
                {
                    m_normal = unnormalizedNormal(v0, v1, v2);
                    const double a1 = (v1 - v0).angleRadians(v2 - v0);
                    const double a2 = (v2 - v1).angleRadians(v0 - v1);
                    const double a3 = (v0 - v2).angleRadians(v1 - v2);

                    m_normals[0].emplace_back(m_normal * a1);
                    m_normals[1].emplace_back(m_normal * a2);
                    m_normals[2].emplace_back(m_normal * a3);
                }
            }
            bool isFlatShaded() const
            {
                return (m_surf & Surface::ShadeMask) == 0;
            }
            bool isSmoothShaded() const
            {
                return (m_surf & Surface::ShadeMask) == Surface::Smooth;
            }
            bool good() const
            {
                if (m_vertices[0].equals(m_vertices[1]) || m_vertices[0].equals(m_vertices[2]) || m_vertices[1].equals(m_vertices[2]))
                    return false;
                return !collinear(m_vertices[0], m_vertices[1], m_vertices[2]);
            }
            void smooth(Triangle &other)
            {
                for (size_t i = 0; i < 3; i++)
                {
                    for (size_t j = 0; j < 3; j++)
                    {
                        if (m_vertices[i] == other.m_vertices[j])
                        {
                            m_normals[i].emplace_back(other.m_normals[j][0]);
                            other.m_normals[j].emplace_back(m_normals[i][0]);
                        }
                    }
                }
            }
            Vertex vertex(size_t index, const Object &object) const
            {
                Vertex  vertex;
                vertex.vertex = m_vertices[index];
                vertex.normal = m_normals[index][0];
                for (size_t i = 1; i < m_normals[index].size(); i++)
                {
                    const double angle = vertex.normal.angleDegrees(m_normals[index][i]);
                    if (object.crease.initialized && angle < object.crease.value)
                        vertex.normal += m_normals[index][i];
                    else if (!object.crease.initialized && angle < 179.999)
                        vertex.normal += m_normals[index][i];
                }
                vertex.normal.normalize();
                return vertex;
            }

            V3d m_vertices[3];
            std::vector<V2d> m_coordinates[3];
            std::vector<V3d> m_normals[3];
            V3d m_normal;
            Surface::SURF m_surf;
            int m_mat;
            int m_vertex_index[3]{ 0, 0, 0 };
        };

        std::vector<Triangle> triangles;

        for (const auto &surface : surfaces)
        {
            if (surface.isPolygon())
            {
                if (surface.refs.size() > 3)
                {
#if 0
                    if (surface.concave)
                    {
                        // TODO
                        return;
                    }
                    else // triangle fan
#endif
                    {
                        for (int i = 2; i < static_cast<int>(surface.refs.size()); i++)
                        {
                            const Triangle triangle(surface.surf, surface.mat,
                                vertices[surface.refs[0].index],
                                vertices[surface.refs[i - 1].index],
                                vertices[surface.refs[i].index],
                                surface.refs[0].coords,
                                surface.refs[i - 1].coords,
                                surface.refs[i].coords);
                            if (triangle.good())
                                triangles.emplace_back(triangle);
                            else
                                GfLogWarning("Bad triangle in %s\n", name.c_str());
                        }
                    }
                }
                else
                {
                    const Triangle triangle(surface.surf, surface.mat,
                        vertices[surface.refs[0].index],
                        vertices[surface.refs[1].index],
                        vertices[surface.refs[2].index],
                        surface.refs[0].coords,
                        surface.refs[1].coords,
                        surface.refs[2].coords);
                    if (triangle.good())
                        triangles.emplace_back(triangle);
                    else
                        GfLogWarning("Bad triangle in %s\n", name.c_str());
                }
            }
        }

        if (triangles.empty())
            return;

        size_t size = triangles.size();

        for (size_t i = 0, endi = size - 1; i < endi; i++)
        {
            for (size_t j = i + 1; j < size; j++)
            {
                if (triangles[i].isSmoothShaded() && triangles[j].isSmoothShaded())
                    triangles[i].smooth(triangles[j]);
            }
        }

        std::vector<Vertex> new_vertices;

        for (auto &triangle : triangles)
        {
            for (int i = 0; i < 3; i++)
            {
                bool found = false;
                const Vertex vertex = triangle.vertex(i, *this);
                for (size_t j = 0, end = new_vertices.size(); j < end; j++)
                {
                    if (new_vertices[j].equals(vertex))
                    {
                        triangle.m_vertex_index[i] = int(j);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    triangle.m_vertex_index[i] = int(new_vertices.size());
                    new_vertices.emplace_back(vertex);
                }
            }
        }

        size = new_vertices.size();
        vertices.resize(size);
        normals.resize(size);
        for (size_t i = 0; i < size; i++)
        {
            vertices[i] = new_vertices[i].vertex;
            normals[i] = new_vertices[i].normal;
        }

        surfaces.clear();
        for (const auto &triangle : triangles)
        {
            Surface surface;
            surface.surf = triangle.m_surf;
            surface.mat = triangle.m_mat;
            for (size_t i = 0; i < 3; i++)
            {
                Surface::Ref ref;
                ref.index = triangle.m_vertex_index[i];
                ref.coords = triangle.m_coordinates[i];
                surface.refs.emplace_back(ref);
            }
            surfaces.emplace_back(surface);
        }
    }
    else
    {
        for (auto &kid : kids)
            kid.generateNormals();
    }
}

double Ac3d::Object::getTerrainHeight(double x, double y) const
{
    double  terrainHeight = -1000000;
    V3d     terrainNormal;

    getTerrainHeight(x, y, terrainHeight, terrainNormal);

    return terrainHeight;

}

void Ac3d::Object::getTerrainHeight(double x, double y, double &terrainHeight, V3d &normal) const
{
    if (getBoundingBox().pointInside(x, y))
    {
        if (type == "poly")
        {
            for (const auto &surface : surfaces)
            {
                double z;
                V3d n;

                if (pointInside(surface, x, y, z, n))
                {
                    if (z > terrainHeight)
                    {
                        terrainHeight = z;
                        normal = n;
                    }
                }
            }
        }
        else
        {
            for (const auto &kid : kids)
                kid.getTerrainHeight(x, y, terrainHeight, normal);
        }
    }
}

bool Ac3d::Object::pointInside(const Surface &surface, double x, double y, double &z, V3d &normal) const
{
    if (surface.refs.size() != 3)
        return false;

    const double ax = vertices[surface.refs[0].index][0];
    const double ay = vertices[surface.refs[0].index][1];

    const double as_x = x - ax;
    const double as_y = y - ay;

    const double bx = vertices[surface.refs[1].index][0];
    const double by = vertices[surface.refs[1].index][1];

    const bool s_ab = (bx - ax) * as_y - (by - ay) * as_x > 0;

    const double cx = vertices[surface.refs[2].index][0];
    const double cy = vertices[surface.refs[2].index][1];

    if ((((cx - ax) * as_y - (cy - ay) * as_x) > 0) == s_ab)
        return false;

    if ((((cx - bx) * (y - by) - (cy - by) * (x - bx)) > 0) != s_ab)
        return false;

    const V3d v1 = vertices[surface.refs[1].index] - vertices[surface.refs[0].index];
    const V3d v2 = vertices[surface.refs[2].index] - vertices[surface.refs[0].index];

    const V3d n(v1.cross(v2));

    const double d = n.dot(vertices[surface.refs[0].index]);

    const V3d ray(0, 0, 1);

    const double ndr = n.dot(ray);

    if (ndr == 0)
        return false; // No intersection, the line is parallel to the plane

    const V3d rayOrigin(x, y, 0);

    z = (d - n.dot(rayOrigin)) / ndr;
    normal = n;

    return true;
}

//------------------------------------ Ac3d -----------------------------------

Ac3d::Ac3d()
{
    root.type = "world";
    stack.push(&root);
}

void Ac3d::addObject(const Object &object)
{
    stack.top()->kids.push_back(object);
    stack.push(&stack.top()->kids.back());
}

void Ac3d::addDefaultMaterial()
{
    Ac3d::Material mat;

    mat.name = "";
    mat.rgb.set(0.4, 0.4, 0.4);
    mat.amb.set(0.8, 0.8, 0.8);
    mat.emis.set(0.4, 0.4, 0.4);
    mat.spec.set(0.5, 0.5, 0.5);
    mat.shi = 50;
    mat.trans = 0;

    materials.push_back(mat);
}

void Ac3d::readFile(const std::string &fileName)
{
    std::ifstream   fin(fileName, std::ios::binary);

    if (!fin)
        throw Exception("Couldn't open file");

    std::string line;

    if (!std::getline(fin, line))
        throw Exception("Couldn't read file");

    if (line.substr(0, 5) == "AC3Db")
        versionC = false;
    else if (line.substr(0, 5) == "AC3Dc")
        versionC = true;
    else
        throw Exception("Not AC3D file");
    try
    {
        while (std::getline(fin, line))
        {
            std::vector<std::string> tokens;

            tokenizeLine(line, tokens);

            if (tokens.empty() || tokens[0].empty())
                continue;
            if (tokens.at(0) == "MATERIAL")
                materials.emplace_back(tokens);
            else if (tokens.at(0) == "MAT" && versionC)
                materials.emplace_back(fin, tokens.at(1));
            else if (tokens.at(0) == "OBJECT")
                root.parse(fin, tokens.at(1));
            else
                throw Exception("Invalid AC3D file");
        }
    }
    catch (std::out_of_range &)
    {
        throw Exception("Invalid AC3D file");
    }
}

void Ac3d::writeFile(const std::string &fileName, bool all) const
{
    const bool acc = fileName.substr(fileName.find_last_of('.') + 1) == "acc";

    std::ofstream   fout(fileName);

    if (!fout)
        throw Exception("Couldn't open file");

    if (versionC)
        fout << "AC3Dc" << std::endl;
    else
        fout << "AC3Db" << std::endl;

    for (const auto &material : materials)
        material.write(fout, versionC);

    root.write(fout, all, acc);
}

void Ac3d::transform(const Matrix &matrix)
{
    root.transform(matrix);
}

void Ac3d::flattenGeometry()
{
    const Matrix matrix;

    transform(matrix);
}

void Ac3d::merge(const Ac3d & ac3d, bool mergeMaterials)
{
    MaterialMap materialMap;

    if (mergeMaterials)
    {
        if (materials.empty())
        {
            materials = ac3d.materials;

            for (const auto &kid : ac3d.root.kids)
                root.kids.push_back(kid);

            return;
        }

        for (size_t i = 0; i < ac3d.materials.size(); i++)
        {
            bool found = false;
            for (size_t j = 0; j < materials.size(); j++)
            {
                if (ac3d.materials[i].same(materials[j]))
                {
                    materialMap[i] = j;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                materialMap[i] = static_cast<int>(materials.size());
                materials.push_back(ac3d.materials[i]);
            }
        }
    }

    for (const auto &kid : ac3d.root.kids)
    {
        root.kids.push_back(kid);
        root.kids.back().remapMaterials(mergeMaterials, materialMap);
    }
}

void Ac3d::flipAxes(bool in)
{
    root.flipAxes(in);
}

void Ac3d::generateTriangles()
{
    root.generateTriangles();
}

void Ac3d::generateNormals()
{
    root.generateNormals();
}

double Ac3d::getTerrainHeight(double x, double y) const
{
    double  terrainHeight = -1000000;
    V3d     terrainNormal;

    root.getTerrainHeight(x, y, terrainHeight, terrainNormal);

    return terrainHeight;
}

/*
 * calculates an angle based on plane equation (face normal) of the
 * terrain in this spot. * Angle is determined so that the x axis is
 * aligned to a horizontal intersection (i.e. height line) of the
 * terrain, with y axis pointing towards uphill
 */
double Ac3d::getTerrainAngle(double x, double y) const
{
    return root.getTerrainAngle(x, y);
}

double Ac3d::Object::getTerrainAngle(double x, double y) const
{
    double  terrainHeight = -1000000;
    V3d     terrainNormal;
    double  angle = 0;

    getTerrainHeight(x, y, terrainHeight, terrainNormal);

    if (terrainHeight != -1000000)
        angle = 180.0 - atan2(terrainNormal[0], terrainNormal[1]) * 180.0 / PI;

    return angle;
}

void Ac3d::tokenizeLine(const std::string &line, std::vector<std::string> &tokens)
{
    tokens.clear();
    size_t i = 0;
    while (i < line.length())
    {
        while (std::isspace(line[i]))
            i++;
        if (line[i] == '\"')
        {
            const size_t start = ++i;

            while (i < line.size() && line[i] != '\"')
                i++;

            if (i >= line.size())
                throw Exception("Invalid AC3D file");
            tokens.emplace_back(line.substr(start, i - start));
            i++;
        }
        else
        {
            const size_t start = i++;
            while (i < line.size() && !std::isspace(line[i]))
                i++;
            tokens.emplace_back(line.substr(start, i - start));
            i++;
        }
    }
}

void Ac3d::splitBySURF()
{
    root.splitBySURF();
}

void Ac3d::splitByMaterial()
{
    root.splitByMaterial();
}

void Ac3d::splitByUV()
{
    root.splitByUV();
}

void Ac3d::removeEmptyObjects()
{
    root.removeEmptyObjects();
}

void Ac3d::removeBadTriangles()
{
    root.removeBadTriangles();
}

void Ac3d::getPolys(Object *object, std::vector<Ac3d::Object *> &polys)
{
    if (object->type == "poly")
    {
        polys.push_back(object);
        return;
    }
    for (auto &kid : object->kids)
        getPolys(&kid, polys);
}

std::vector<Ac3d::Object *> &Ac3d::getPolys(std::vector<Ac3d::Object *> &polys)
{
    getPolys(&root, polys);

    return polys;
}
