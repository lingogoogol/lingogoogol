#ifndef MATH_F_H
#define MATH_F_H

#include <glm/glm.hpp>

enum class Alignment;

template<typename T1, typename T2>
class Enum_vector;
class Plane;
class Line;

glm::vec3 to_cell_side(glm::vec3 begin, glm::vec3 dir);
template<typename T1>
T1 operator++(T1& operand, int);

#endif