/***************************************************************************

    file                 : ac3d.h
    created              : Wed May 29 22:15:37 CEST 2002
    copyright            : (C) 2001 by Eric Espie
    email                : Eric.Espie@torcs.org

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

    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
*/

#ifndef _AC3D_H_
#define _AC3D_H_

#include <portability.h>

#include <array>
#include <stack>
#include <string>
#include <vector>
#include <list>
#include <fstream>
#include <limits>
#include <map>

struct Ac3d
{
    struct V2d : public std::array<double, 2>
    {
        V2d()
        {
            at(0) = 0;
            at(1) = 0;
        }
        V2d(double x, double y)
        {
            at(0) = x;
            at(1) = y;
        }
    };

    struct V3d : public std::array<double, 3>
    {
        V3d()
        {
            at(0) = 0;
            at(1) = 0;
            at(2) = 0;
        }
        V3d(double x, double y, double z)
        {
            at(0) = x;
            at(1) = y;
            at(2) = z;
        }

        double x() const { return (*this)[0]; }
        double y() const { return (*this)[1]; }
        double z() const { return (*this)[2]; }
        void x(double value) { (*this)[0] = value; }
        void y(double value) { (*this)[1] = value; }
        void z(double value) { (*this)[2] = value; }
        V3d operator+(const V3d &other) const;
        V3d operator-(const V3d &other) const;
        V3d operator/(double scalar) const;
        V3d operator += (const V3d &other);
        V3d operator /= (double scalar);
        V3d operator * (double value) const
        {
            return V3d{ x() * value, y() * value, z() * value };
        }
        bool operator == (const V3d &other)
        {
            return x() == other.x() && y() == other.y() && z() == other.z();
        }
        double dot(const V3d &other) const;
        V3d cross(const V3d &other) const;
        double length() const;
        void normalize();
        double angleRadians(const V3d &other) const
        {
            return acos(dot(other) / (length() * other.length()));
        }
        double angleDegrees(const V3d &other) const
        {
            return angleRadians(other) * 180.0 / M_PI;
        }
        bool equals(const V3d &other) const
        {
            static constexpr double  SMALL_NUM = static_cast<double>(std::numeric_limits<float>::epsilon());

            return std::abs(x() - other.x()) < SMALL_NUM &&
                   std::abs(y() - other.y()) < SMALL_NUM &&
                   std::abs(z() - other.z()) < SMALL_NUM;
        }
    };

    struct Color : public std::array<double, 3>
    {
        Color()
        {
            at(0) = 0;
            at(1) = 0;
            at(2) = 0;
        }
        void set(double r, double g, double b)
        {
            at(0) = r;
            at(1) = g;
            at(2) = b;
        }
    };

    class Exception : public std::exception
    {
    private:
        const char *message;

    public:
        explicit Exception(const char *msg) : message(msg)
        {
        }
        const char *what() const noexcept
        {
            return message;
        }
    };

    struct Material
    {
        std::string	name;
        Color       rgb;
        Color       amb;
        Color       emis;
        Color       spec;
        int         shi = 0;
        double      trans = 0;
        std::string	data;

        Material() = default;
        explicit Material(const std::vector<std::string> &tokens);
        Material(std::ifstream &fin, const std::string &name);
        void write(std::ofstream &fout, bool versionC) const;
        bool same(const Material &material) const;
    };

    struct Object;

    struct Surface
    {
        struct Ref
        {
            int                 index = 0;
            std::vector<V2d>    coords;

            Ref() = default;
            Ref(int index, double u, double v) : index(index)
            {
                coords.emplace_back(u, v);
            }
            Ref(int index, double u, double v, double u1, double v1) : index(index)
            {
                coords.emplace_back(u, v);
                coords.emplace_back(u1, v1);
            }
            Ref(int index, double u, double v, double u1, double v1, double u2, double v2) : index(index)
            {
                coords.emplace_back(u, v);
            coords.emplace_back(u1, v1);
            coords.emplace_back(u2, v2);
            }
            Ref(int index, double u, double v, double u1, double v1, double u2, double v2, double u3, double v3) : index(index)
            {
                coords.emplace_back(u, v);
                coords.emplace_back(u1, v1);
                coords.emplace_back(u2, v2);
                coords.emplace_back(u3, v3);
            }
        };

        enum SURF : unsigned int {
            Polygon                         = 0x00,
            ClosedLine                      = 0x01,
            OpenLine                        = 0x02,
            TriangleStrip                   = 0x04,
            TypeMask                        = 0x0f,
            SingleSided                     = 0x00,
            DoubleSided                     = 0x20,
            Flat                            = 0x00,
            Smooth                          = 0x10,
            ShadeMask                       = 0x10,
            PolygonSingleSidedFlat          = Polygon       | SingleSided | Flat,      // 0x00
            ClosedLineSingleSidedFlat       = ClosedLine    | SingleSided | Flat,      // 0x01
            OpenLineSingleSidedFlat         = OpenLine      | SingleSided | Flat,      // 0x02
            TriangleStripSingleSidedFlat    = TriangleStrip | SingleSided | Flat,      // 0x04
            PolygonSingleSidedSmooth        = Polygon       | SingleSided | Smooth,    // 0x10
            ClosedLineSingleSidedSmooth     = ClosedLine    | SingleSided | Smooth,    // 0x11
            OpenLineSingleSidedSmooth       = OpenLine      | SingleSided | Smooth,    // 0x12
            TriangleStripSingleSidedSmooth  = TriangleStrip | SingleSided | Smooth,    // 0x14
            PolygonDoubleSidedFlat          = Polygon       | DoubleSided | Flat,      // 0x20
            ClosedLineDoubleSidedFlat       = ClosedLine    | DoubleSided | Flat,      // 0x21
            OpenLineDoubleSidedFlat         = OpenLine      | DoubleSided | Flat,      // 0x22
            TriangleStripDoubleSidedFlat    = TriangleStrip | DoubleSided | Flat,      // 0x24
            PolygonDoubleSidedSmooth        = Polygon       | DoubleSided | Smooth,    // 0x30
            ClosedLineDoubleSidedSmooth     = ClosedLine    | DoubleSided | Smooth,    // 0x31
            OpenLineDoubleSidedSmooth       = OpenLine      | DoubleSided | Smooth,    // 0x32
            TriangleStripDoubleSidedSmooth  = TriangleStrip | DoubleSided | Smooth,    // 0x34
        };
        SURF                surf = PolygonSingleSidedFlat;
        int                 mat = 0;
        std::vector<Ref>    refs;
        V3d                 normal = { 0, 0, 0 };

        Surface() = default;
        explicit Surface(std::ifstream &fin);
        void write(std::ofstream &fout, const Object &object) const;
        bool isPolygon() const
        {
            return (surf & TypeMask) == Polygon;
        }
        bool isTriangleStrip() const
        {
            return (surf & TypeMask) == TriangleStrip;
        }
        bool isBadTriangle(const Object &object)
        {
            if (isPolygon() && refs.size() == 3)
            {
                if (object.vertices[refs[0].index].equals(object.vertices[refs[1].index]) ||
                    object.vertices[refs[0].index].equals(object.vertices[refs[2].index]) ||
                    object.vertices[refs[1].index].equals(object.vertices[refs[2].index]))
                    return true;
                return collinear(object.vertices[refs[0].index],
                                 object.vertices[refs[1].index],
                                 object.vertices[refs[2].index]);
            }
            return false;
        }
    };

    class v2 : public std::array<double, 2>
    {
    public:
        bool initialized = false;
    };
    class v3 : public V3d
    {
    public:
        bool initialized = false;
    };
    class v9 : public std::array<double, 9>
    {
    public:
        bool initialized = false;
    };
    template <typename T> struct Value
    {
        T       value = 0;
        bool    initialized = false;

        operator T() const { return value; }
        T operator = (T i) { value = i; return value; }
    };

    class Matrix : public std::array<std::array<double, 4>, 4>
    {
    public:
        Matrix();
        Matrix(double m0, double m1, double m2, double m3,
               double m4, double m5, double m6, double m7,
               double m8, double m9, double m10, double m11,
               double m12, double m13, double m14, double m15);
        void setLocation(const v3 &location);
        void setLocation(double x, double y, double z);
        void setRotation(const std::array<double, 9> &rotation);
        void setRotation(double x, double y, double z);
        void setScale(double scale);
        void setScale(double x, double y, double z);
        void makeIdentity();
        void makeLocation(const v3 &location);
        void makeLocation(double x, double y, double z);
        void makeRotation(const std::array<double, 9> &rotation);
        void makeRotation(double x, double y, double z);
        void makeScale(double scale);
        void makeScale(double x, double y, double z);
        void transformPoint(V3d &point) const;
        void transformNormal(V3d &normal) const;
        Matrix multiply(const Matrix &matrix);
    };

    struct BoundingBox
    {
        V3d min{ std::numeric_limits<double>::max(),
                 std::numeric_limits<double>::max(),
                 std::numeric_limits<double>::max() };
        V3d max{ -std::numeric_limits<double>::max(),
                 -std::numeric_limits<double>::max(),
                 -std::numeric_limits<double>::max() };

        void extend(const V3d &vertex);
        void extend(const BoundingBox &boundingBox);
        bool pointInside(double x, double y) const;
    };

    struct BoundingSphere
    {
        V3d     center{ 0, 0, 0 };
        double  radius = 0;

        void extend(const BoundingBox &boundingBox);
    };

    typedef std::map<size_t, size_t> MaterialMap;

    struct Texture
    {
        explicit Texture(const std::string &name) : name(name) { }
        Texture(const std::string &name, const std::string &type) : name(name), type(type) { }
        std::string name;
        std::string type;
    };

    struct Object
    {
        std::string		            type;
        std::string		            name;
        std::string		            data;
        std::vector<Texture>        textures;
        v2                          texrep;
        v2                          texoff;
        Value<int>				    subdiv;
        Value<double>			    crease;
        v9                          rot;
        v3			                loc;
        std::string				    url;
        bool				        hidden = false;
        bool				        locked = false;
        bool				        folded = false;
        std::vector<V3d>            vertices;
        std::vector<V3d>            normals;
        std::list<Surface>          surfaces;
        std::list<Object>           kids;

        // not part of AC3D file
        mutable bool                hasBoundingBox = false;
        mutable BoundingBox         boundingBox;
        mutable bool                hasBoundingSphere = false;
        mutable BoundingSphere      boundingSphere;

        Object() = default;
        Object(const std::string &type, const std::string &name) : type(type), name(name) { }
        explicit Object(std::ifstream &fin);
        void parse(std::ifstream &fin, const std::string &objType);
        void write(std::ofstream &fout, bool all, bool acc) const;
        void transform(const Matrix &matrix);
        void flipAxes(bool in);
        void splitBySURF();
        void splitByMaterial();
        void splitByUV();
        void removeEmptyObjects();
        void removeBadTriangles();
        void removeSurfacesNotSURF(unsigned int SURF);
        void removeSurfacesNotMaterial(int material);
        void removeUnusedVertices();
        const BoundingBox &getBoundingBox() const;
        const BoundingSphere &getBoundingSphere() const;
        void remapMaterials(bool mergeMaterials, const MaterialMap &materialMap);
        void generateTriangles();
        void generateNormals();
        void getTerrainHeight(double x, double y, double &terrainHeight, V3d &normal) const;
        double getTerrainHeight(double x, double y) const;
        double getTerrainAngle(double x, double y) const;
        bool pointInside(const Surface &surface, double x, double y, double &z, V3d &normal) const;
    };

    bool                    versionC = false;
    std::vector<Material>   materials;
    Object                  root;
    std::stack<Object *>    stack;

    Ac3d();
    void addObject(const Object &object);
    void addDefaultMaterial();
    void readFile(const std::string &fileName);
    void writeFile(const std::string &fileName, bool all) const;
    void flattenGeometry();
    void transform(const Matrix &matrix);
    void flipAxes(bool in);
    void generateTriangles();
    void generateNormals();
    void splitBySURF();
    void splitByMaterial();
    void splitByUV();
    void removeEmptyObjects();
    void removeBadTriangles();
    void merge(const Ac3d &ac3d, bool mergeMaterials);
    double getTerrainHeight(double x, double y) const;
    double getTerrainAngle(double x, double y) const;
    static void tokenizeLine(const std::string &line, std::vector<std::string> &tokens);
    std::vector<Ac3d::Object *> &getPolys(std::vector<Ac3d::Object *> &polys);
    static void getPolys(Object *object, std::vector<Ac3d::Object *> &polys);
    static V3d normalizedNormal(const V3d &p0, const V3d &p1, const V3d &p2);
    static V3d unnormalizedNormal(const V3d &p0, const V3d &p1, const V3d &p2);
    static bool collinear(const V3d &p1, const V3d &p2, const V3d &p3);
};

#endif /* _AC3D_H_ */
