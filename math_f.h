#ifndef MATH_F_H
#define MATH_F_H

#include <string>

#include <glm/glm.hpp>

enum class Alignment;

template<typename T1, typename T2>
class Vector;
template<typename T1, typename T2>
class Pair;
class Plane;
class Line;

std::string to_string8(float f, int precision);
std::string to_string8(double d, int precision);
std::string to_string8(std::u32string s);
std::u32string to_string32(float f, int precision);
std::u32string to_string32(double d, int precision);
std::u32string to_string32(std::string s);
glm::vec3 to_cell_side(glm::vec3 begin, glm::vec3 dir);
glm::ivec3 to_block(glm::ivec3 coord);
glm::ivec3 to_coord_in_block(glm::ivec3 coord);
std::string get_time();

#endif