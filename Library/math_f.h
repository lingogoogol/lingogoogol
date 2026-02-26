#ifndef LIBRARY_MATH_F_H
#define LIBRARY_MATH_F_H

#include <string>
#include <cstdint>

#include <glm/glm.hpp>

enum class Alignment;

template<typename T1, typename T2>
class Vector;
template<typename T1, typename T2>
class Pair;
class Plane;
class Line;

std::int_fast8_t to_intfast8(char8_t ch);
char8_t to_char8(int i);
template<typename T1, int T2>
std::string to_string8(T1 f, int precision);
std::string to_string8(std::u32string s);
template<typename T1, int T2>
std::u32string to_string32(T1 d, int precision);
std::u32string to_string32(std::string s);
std::string get_time();

#endif