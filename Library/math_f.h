#ifndef LIBRARY_MATH_F_H
#define LIBRARY_MATH_F_H

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
std::string get_time();

#endif