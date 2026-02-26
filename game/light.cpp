#include "light_d.h"

Light_pv::Light_pv(lgo::svec<float, 3> pos_param, lgo::svec<float, 3> color_param) :
	pos{ pos_param }, color{ color_param } {}

Point_light::Point_light(lgo::svec<float, 3> pos_param, lgo::svec<float, 3> color_param) :
	Light_pv{ pos_param, color_param } {}

Dir_light::Dir_light(lgo::svec<float, 3> pos_param,
	lgo::svec<float, 3> dir_param, lgo::svec<float, 3> color_param) :
	Light_pv{ pos_param, color_param }, dir{ dir_param } {}

Spotlight::Spotlight(lgo::svec<float, 3> pos_param, lgo::svec<float, 3> dir_param,
	float inner_cone_param, float outer_cone_param, lgo::svec<float, 3> color_param) :
	Light_pv{ pos_param, color_param }, dir{ dir_param },
	inner_cone{ inner_cone_param }, outer_cone{ outer_cone_param } {}