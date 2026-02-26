#ifndef MATH_H
#define MATH_H

#include <vector>

#include <glm/glm.hpp>

#include "math_f.h"

enum class Alignment {
	Small,
	Large,
	Middle
};

template<typename T1, typename T2>
class Enum_vector :public std::vector<T1> {
public:
	Enum_vector();
	T1& operator[](T2 index);
	const T1& operator[](T2 index) const;
};

class Plane {
public:
	glm::vec3 coefficient{};
	float constant{};
	Plane(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3);
};

class Line {
public:
	glm::vec3 origin{};
	glm::vec3 dir{};
	Line(glm::vec3 origin_param, glm::vec3 point);
	glm::vec3 foot_of_perpendicular(glm::vec3 point);
};

template<typename T1, typename T2>
Enum_vector<T1, T2>::Enum_vector() :std::vector{} {
	return;
}

template<typename T1, typename T2>
T1& Enum_vector<T1, T2>::operator[](T2 index) {
	return std::vector<T1>[static_cast<std::size_t>(T2)];
}

template<typename T1, typename T2>
const T1& Enum_vector<T1, T2>::operator[](T2 index) const {
	return std::vector<T1>[static_cast<std::size_t>(T2)];
}

template<typename T1>
T1 operator++(T1& operand, int) {
	T1 copy{ operand };
	operand = static_cast<T1>(static_cast<unsigned int>(operand) + 1);
	return copy;
}

#endif