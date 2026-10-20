#ifndef TRACK_GROUND_MESH_H
#define TRACK_GROUND_MESH_H

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

struct TrackGroundSample
{
    double height;
    std::array<double, 3> normal;
    double friction;
    double rolling_resistance;
};

class TrackGroundMesh
{
public:
    bool load(const std::string& path);
    bool sample(double horizontal_position, double lateral_position, double reference_height, TrackGroundSample& result) const;

private:
    struct Triangle
    {
        std::array<double, 9> coordinates;
        double friction;
        double rolling_resistance;
    };
    std::vector<Triangle> triangles;
    std::unordered_map<long long, std::vector<size_t> > cells;
    static long long cellKey(int horizontal_cell, int lateral_cell);
};

extern TrackGroundMesh simulation_track_ground_mesh;

#endif
