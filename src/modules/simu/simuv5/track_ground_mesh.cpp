#include "track_ground_mesh.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

TrackGroundMesh simulation_track_ground_mesh;

long long TrackGroundMesh::cellKey(int horizontal_cell, int lateral_cell)
{
    return static_cast<long long>(horizontal_cell) * 4294967296LL + static_cast<unsigned int>(lateral_cell);
}

bool TrackGroundMesh::load(const std::string& path)
{
    triangles.clear();
    cells.clear();
    if (path.empty())
        return true;
    std::ifstream input(path);
    std::string signature;
    size_t triangle_count;
    if (!(input >> signature >> triangle_count) || signature != "FORMULA_GROUND_1" || triangle_count > 1000000)
        return false;
    for (size_t triangle_index = 0; triangle_index < triangle_count; triangle_index++)
    {
        Triangle triangle;
        if (!(input >> triangle.friction >> triangle.rolling_resistance))
            return false;
        for (double& coordinate : triangle.coordinates)
            if (!(input >> coordinate) || !std::isfinite(coordinate))
                return false;
        const auto& coordinates = triangle.coordinates;
        triangles.push_back(triangle);
        const int horizontal_start = static_cast<int>(std::floor(std::min({coordinates[0], coordinates[3], coordinates[6]}) / 20.0));
        const int horizontal_end = static_cast<int>(std::floor(std::max({coordinates[0], coordinates[3], coordinates[6]}) / 20.0));
        const int lateral_start = static_cast<int>(std::floor(std::min({coordinates[1], coordinates[4], coordinates[7]}) / 20.0));
        const int lateral_end = static_cast<int>(std::floor(std::max({coordinates[1], coordinates[4], coordinates[7]}) / 20.0));
        for (int horizontal_cell = horizontal_start; horizontal_cell <= horizontal_end; horizontal_cell++)
            for (int lateral_cell = lateral_start; lateral_cell <= lateral_end; lateral_cell++)
                cells[cellKey(horizontal_cell, lateral_cell)].push_back(triangle_index);
    }
    return true;
}

bool TrackGroundMesh::sample(double horizontal_position, double lateral_position, double reference_height, TrackGroundSample& result) const
{
    const auto cell = cells.find(cellKey(static_cast<int>(std::floor(horizontal_position / 20.0)), static_cast<int>(std::floor(lateral_position / 20.0))));
    if (cell == cells.end())
        return false;
    double best_distance = std::numeric_limits<double>::infinity();
    for (size_t triangle_index : cell->second)
    {
        const Triangle& triangle = triangles[triangle_index];
        const auto& coordinates = triangle.coordinates;
        const double first_horizontal = coordinates[3] - coordinates[0];
        const double first_lateral = coordinates[4] - coordinates[1];
        const double second_horizontal = coordinates[6] - coordinates[0];
        const double second_lateral = coordinates[7] - coordinates[1];
        const double determinant = first_horizontal * second_lateral - first_lateral * second_horizontal;
        if (std::abs(determinant) < 0.00000001)
            continue;
        const double relative_horizontal = horizontal_position - coordinates[0];
        const double relative_lateral = lateral_position - coordinates[1];
        const double first_weight = (relative_horizontal * second_lateral - relative_lateral * second_horizontal) / determinant;
        const double second_weight = (first_horizontal * relative_lateral - first_lateral * relative_horizontal) / determinant;
        if (first_weight < -0.000001 || second_weight < -0.000001 || first_weight + second_weight > 1.000001)
            continue;
        const double height = coordinates[2] + first_weight * (coordinates[5] - coordinates[2]) + second_weight * (coordinates[8] - coordinates[2]);
        const double distance = std::abs(height - reference_height);
        if (distance >= best_distance)
            continue;
        const double first_height = coordinates[5] - coordinates[2];
        const double second_height = coordinates[8] - coordinates[2];
        std::array<double, 3> normal = {{first_lateral * second_height - first_height * second_lateral,
                                       first_height * second_horizontal - first_horizontal * second_height, determinant}};
        const double normal_length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        for (double& component : normal)
            component *= (normal[2] < 0.0 ? -1.0 : 1.0) / normal_length;
        result = {height, normal, triangle.friction, triangle.rolling_resistance};
        best_distance = distance;
    }
    return std::isfinite(best_distance);
}
