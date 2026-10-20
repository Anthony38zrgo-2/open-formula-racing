#include "track_ground_mesh.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argument_count, char** argument_values)
{
    if (argument_count != 2)
        return EXIT_FAILURE;
    TrackGroundMesh ground;
    if (!ground.load(argument_values[1]))
        return EXIT_FAILURE;
    std::ifstream input(argument_values[1]);
    std::string signature;
    size_t triangle_count;
    input >> signature >> triangle_count;
    size_t failures = 0;
    double maximum_error = 0.0;
    for (size_t triangle_index = 0; triangle_index < triangle_count; triangle_index++)
    {
        double friction, resistance;
        std::array<double, 9> triangle;
        input >> friction >> resistance;
        for (double& coordinate : triangle)
            input >> coordinate;
        for (double first_weight : {0.2, 0.4, 0.6})
        {
            const double second_weight = 0.2;
            const double last_weight = 1.0 - first_weight - second_weight;
            const double horizontal_position = triangle[0] * first_weight + triangle[3] * second_weight + triangle[6] * last_weight;
            const double lateral_position = triangle[1] * first_weight + triangle[4] * second_weight + triangle[7] * last_weight;
            const double expected_height = triangle[2] * first_weight + triangle[5] * second_weight + triangle[8] * last_weight;
            TrackGroundSample sample;
            if (!ground.sample(horizontal_position, lateral_position, expected_height, sample))
            {
                failures++;
                continue;
            }
            const double error = std::abs(sample.height - expected_height);
            maximum_error = std::max(maximum_error, error);
            const double normal_length = std::sqrt(sample.normal[0] * sample.normal[0] + sample.normal[1] * sample.normal[1] + sample.normal[2] * sample.normal[2]);
            failures += error > 0.000001 || std::abs(normal_length - 1.0) > 0.000001 || sample.normal[2] <= 0.0;
        }
    }
    TrackGroundSample outside_sample;
    failures += ground.sample(-1000000, -1000000, 0, outside_sample);
    std::cout << "Ground triangles: " << triangle_count << "; samples: " << triangle_count * 3 << "; failures: " << failures
              << "; maximum height error: " << maximum_error << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
