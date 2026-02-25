#include "light.h"

Light_pv::Light_pv() {
	return;
}

Light_pv::Light_pv(glm::vec3 color_param) :color{ color_param } {
	return;
}

Point_light::Point_light() : Light_pv{} {
	return;
}

Point_light::Point_light(glm::vec3 pos_param, glm::vec3 color) : Light_pv{ color }, pos{ pos_param } {
	return;
}

Dir_light::Dir_light() : Light_pv{} {
	return;
}

Dir_light::Dir_light(glm::vec3 dir_param, glm::vec3 color) : Light_pv{ color }, dir{ dir_param } {
	return;
}

Spot_light::Spot_light() : Light_pv{} {
	return;
}

Spot_light::Spot_light(glm::vec3 pos_param, glm::vec3 dir_param,
	float inner_cone_param, float outer_cone_param, glm::vec3 color):
	Light_pv{ color }, pos{ pos_param }, dir{ dir_param },
	inner_cone{ inner_cone_param }, outer_cone{ outer_cone_param }{
	return;
}