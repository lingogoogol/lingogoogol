#include "light.h"

Light_pv::Light_pv(glm::vec3 pos_param, glm::vec3 color_param) :pos{ pos_param }, color { color_param } {
	return;
}

Point_light::Point_light(glm::vec3 pos_param, glm::vec3 color_param) : Light_pv{ pos_param, color_param } {
	return;
}

Dir_light::Dir_light(glm::vec3 pos_param, glm::vec3 dir_param, glm::vec3 color_param) :
	Light_pv{ pos_param, color_param }, dir{ dir_param } {
	return;
}

Spotlight::Spotlight(glm::vec3 pos_param, glm::vec3 dir_param,
	float inner_cone_param, float outer_cone_param, glm::vec3 color_param):
	Light_pv{ pos_param, color_param }, dir{ dir_param },
	inner_cone{ inner_cone_param }, outer_cone{ outer_cone_param }{
	return;
}