#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

class Light_pv {
public:
	glm::vec3 color{};
	Light_pv();
	Light_pv(glm::vec3 color_param);
};

class Point_light final :public Light_pv {
public:
	glm::vec3 pos{};
	Point_light();
	Point_light(glm::vec3 pos, glm::vec3 color);
};

class Dir_light final :public Light_pv {
public:
	glm::vec3 dir{};
	Dir_light();
	Dir_light(glm::vec3 dir, glm::vec3 color);
};

class Spot_light final :public Light_pv {
public:
	glm::vec3 pos{};
	glm::vec3 dir{};
	float inner_cone{};
	float outer_cone{};
	Spot_light();
	Spot_light(glm::vec3 pos_param, glm::vec3 dir_param,
		float inner_cone_param, float outer_cone_param, glm::vec3 color);
};

#endif