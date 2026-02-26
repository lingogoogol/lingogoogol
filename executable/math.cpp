#include <algorithm>

#include "math.h"

Plane::Plane(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3) :
    coefficient{ (point2 - point1) * (point3 - point1) }, constant{ glm::dot(coefficient, point1) } {
    return;
}

Line::Line(glm::vec3 origin_param, glm::vec3 point) :
    origin{ origin_param }, dir{ glm::normalize(point - origin_param) } {
    return;
}

glm::vec3 Line::foot_of_perpendicular(glm::vec3 point) {
    return origin + dir * glm::dot(point - origin, dir) / glm::dot(dir, dir);
}

glm::vec3 to_cell_side(glm::vec3 begin, glm::vec3 dir) {
    glm::vec3 coefficient{};
    for (int i{ 0 }; i < 3; i++)
        coefficient[i] = dir[i] == 0 ? std::numeric_limits<float>::infinity() :
        (((dir[i] > 0 ? std::floor(begin[i] + 1) : std::ceil(begin[i] - 1)) - begin[i]) / dir[i]);
    float dist_to_cell_side{ std::min(coefficient.x, std::min(coefficient.y, coefficient.z)) };
    for (int i{ 0 }; i < 3; i++)
        begin[i] = coefficient[i] == dist_to_cell_side ? (dir[i] > 0 ? std::floor(begin[i] + 1) :
            std::ceil(begin[i] - 1)) : (begin[i] + dir[i] * dist_to_cell_side);
    return begin;
};