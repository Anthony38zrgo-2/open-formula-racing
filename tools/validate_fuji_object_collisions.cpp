#include <SOLID/solid.h>
#include <array>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void recordCollision(void* context, DtObjectRef, DtObjectRef, const DtCollData* collision)
{
    *static_cast<bool*>(context) = collision && std::hypot(collision->normal[0], collision->normal[1]) > 0.000001;
}

static void ignoreStaticCollision(void*, DtObjectRef, DtObjectRef, const DtCollData*)
{
}

static bool validateStaticWorld(const char* path)
{
    std::ifstream input(path);
    std::string signature;
    size_t group_count;
    input >> signature >> group_count;
    std::vector<DtShapeRef> shapes(group_count);
    for (size_t group_index = 0; group_index < group_count; group_index++)
    {
        std::string name;
        size_t triangle_count;
        input >> name >> triangle_count;
        shapes[group_index] = dtNewComplexShape();
        for (size_t triangle_index = 0; triangle_index < triangle_count; triangle_index++)
        {
            dtBegin(DT_POLYGON);
            for (int vertex_index = 0; vertex_index < 3; vertex_index++)
            {
                double x, y, z;
                input >> x >> y >> z;
                dtVertex(x, y, z);
            }
            dtEnd();
        }
        dtEndComplexShape();
        dtCreateObject(&shapes[group_index], shapes[group_index]);
        dtSetObjectResponse(&shapes[group_index], ignoreStaticCollision, DT_SMART_RESPONSE, nullptr);
    }
    const auto unfiltered_contacts = dtTest();
    for (size_t first_index = 0; first_index < group_count; first_index++)
        for (size_t second_index = 0; second_index < first_index; second_index++)
            dtSetPairResponse(&shapes[first_index], &shapes[second_index], nullptr, DT_NO_RESPONSE, nullptr);
    const auto filtered_contacts = dtTest();
    std::cout << "Static-only contacts before filtering: " << unfiltered_contacts << "; after: " << filtered_contacts << '\n';
    for (size_t group_index = 0; group_index < group_count; group_index++)
    {
        dtDeleteObject(&shapes[group_index]);
        dtDeleteShape(shapes[group_index]);
    }
    return filtered_contacts == 0;
}

int main(int argument_count, char** argument_values)
{
    if (argument_count != 2)
        return EXIT_FAILURE;
    if (!validateStaticWorld(argument_values[1]))
        return EXIT_FAILURE;
    std::ifstream input(argument_values[1]);
    std::string signature;
    size_t group_count;
    if (!(input >> signature >> group_count) || signature != "FORMULA_STATIC_WALLS_1")
        return EXIT_FAILURE;
    size_t detected_groups = 0;
    for (size_t group_index = 0; group_index < group_count; group_index++)
    {
        std::string name;
        size_t triangle_count;
        input >> name >> triangle_count;
        std::vector<std::array<double, 9> > triangles(triangle_count);
        DtShapeRef wall_shape = dtNewComplexShape();
        for (std::array<double, 9>& triangle : triangles)
        {
            dtBegin(DT_POLYGON);
            for (size_t coordinate_index = 0; coordinate_index < 9; coordinate_index++)
                input >> triangle[coordinate_index];
            for (size_t vertex_index = 0; vertex_index < 9; vertex_index += 3)
                dtVertex(triangle[vertex_index], triangle[vertex_index + 1], triangle[vertex_index + 2]);
            dtEnd();
        }
        dtEndComplexShape();
        int wall_identifier = 0;
        int probe_identifier = 1;
        DtShapeRef probe_shape = dtBox(4.44427, 2.0, 1.154363);
        dtCreateObject(&wall_identifier, wall_shape);
        dtCreateObject(&probe_identifier, probe_shape);
        bool collision_detected = false;
        dtSetPairResponse(&wall_identifier, &probe_identifier, recordCollision, DT_SMART_RESPONSE, &collision_detected);
        for (const std::array<double, 9>& triangle : triangles)
        {
            const double first_x = triangle[3] - triangle[0];
            const double first_y = triangle[4] - triangle[1];
            const double first_z = triangle[5] - triangle[2];
            const double second_x = triangle[6] - triangle[0];
            const double second_y = triangle[7] - triangle[1];
            const double second_z = triangle[8] - triangle[2];
            const double normal_x = first_y * second_z - first_z * second_y;
            const double normal_y = first_z * second_x - first_x * second_z;
            const double normal_length = std::hypot(normal_x, normal_y);
            if (normal_length < 0.000001)
                continue;
            const double center_x = (triangle[0] + triangle[3] + triangle[6]) / 3.0;
            const double center_y = (triangle[1] + triangle[4] + triangle[7]) / 3.0;
            const double center_z = (triangle[2] + triangle[5] + triangle[8]) / 3.0;
            dtSelectObject(&probe_identifier);
            dtLoadIdentity();
            dtTranslate(center_x + normal_x / normal_length * 5.0, center_y + normal_y / normal_length * 5.0, center_z);
            dtTestObjects(&wall_identifier, &probe_identifier);
            if (collision_detected)
            {
                collision_detected = false;
                continue;
            }
            dtProceed();
            for (int step_index = 1; step_index <= 150; step_index++)
            {
                const double distance = 5.0 - step_index * 0.05;
                dtSelectObject(&probe_identifier);
                dtLoadIdentity();
                dtTranslate(center_x + normal_x / normal_length * distance, center_y + normal_y / normal_length * distance, center_z);
                dtTestObjects(&wall_identifier, &probe_identifier);
                if (collision_detected)
                    break;
                dtProceed();
            }
            if (collision_detected)
                break;
        }
        std::cout << name << (collision_detected ? " PASS" : " FAIL") << '\n';
        detected_groups += collision_detected;
        dtClearPairResponse(&wall_identifier, &probe_identifier);
        dtDeleteObject(&probe_identifier);
        dtDeleteObject(&wall_identifier);
        dtDeleteShape(probe_shape);
        dtDeleteShape(wall_shape);
    }
    std::cout << "Detected collision groups: " << detected_groups << '/' << group_count << '\n';
    return input.good() && detected_groups == group_count ? EXIT_SUCCESS : EXIT_FAILURE;
}
