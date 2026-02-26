#ifndef LIBRARY_MATH_H
#define LIBRARY_MATH_H

#include <vector>
#include <array>

#include <glm/glm.hpp>

#include "math_f.h"
#include "concept.h"

enum class Alignment {
	Small,
	Large,
	Middle
};

template<typename T1, typename T2>
class Vector :public std::vector<T1> {
public:
	Vector();
	T1& operator[](T2 index);
	const T1& operator[](T2 index) const;
};

class Universal {
	template<typename T1>
	operator T1();
};

template<typename T1, typename T2>
class Pair {
public:
	T1 first{};
	T2 second{};
	Pair();
	Pair(T1 first_param, T2 second_param);
	bool operator==(Pair<T1, T2> pair);
};

class Plane {
public:
	glm::vec3 coefficient{};
	float constant{};
	Plane(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3);
	Plane(std::array<glm::vec3, 3> points);
};

class Line {
public:
	glm::vec3 origin{};
	glm::vec3 dir{};
	Line(glm::vec3 origin_param, glm::vec3 point);
	glm::vec3 foot_of_perpendicular(glm::vec3 point);
	float meet_plane(Plane plane);
};

template<typename T1, typename T2>
Vector<T1, T2>::Vector() :std::vector{} {
	return;
}

template<typename T1, typename T2>
T1& Vector<T1, T2>::operator[](T2 index) {
	return std::vector<T1>[static_cast<std::size_t>(T2)];
}

template<typename T1, typename T2>
const T1& Vector<T1, T2>::operator[](T2 index) const {
	return std::vector<T1>[static_cast<std::size_t>(T2)];
}

template<typename T1>
T1 operator++(T1& operand, int) {
	T1 copy{ operand };
	operand = static_cast<T1>(static_cast<unsigned int>(operand) + 1);
	return copy;
}

template<typename T1>
Universal::operator T1() {
	return T1{};
}

template<typename T1, typename T2>
Pair(T1, T2)->Pair<T1, T2>;

template<typename T1, typename T2>
Pair<T1, T2>::Pair() {
	return;
}

template<typename T1, typename T2>
Pair<T1, T2>::Pair(T1 first_param, T2 second_param) :first{ first_param }, second{ second_param } {
	return;
}

template<typename T1, typename T2>
bool Pair<T1, T2>::operator==(Pair<T1, T2> pair) {
	return first == pair.first && second == pair.second;
}

template<typename T1, typename... T2>
int mem_num1(T2... universal) {
	if constexpr (initializeble<T1, universal...>)
		return mem_num1(universal..., Universal);
	else
		return sizeof...(T2) - 1;
}

template<typename T1>
int mem_num() {
	return mem_num1<T1>(Universal{});
}

#endif