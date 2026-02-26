#ifndef LIGHT_H
#define LIGHT_H

#include <lgo/vec/s/.h>

class Light_pv {
public:
	lgo::svec<float, 3> pos{};
	lgo::svec<float, 3> color{};
	Light_pv(lgo::svec<float, 3> pos_param, lgo::svec<float, 3> color_param);
};

class Point_light final :public Light_pv {
public:
	Point_light(lgo::svec<float, 3> pos_param, lgo::svec<float, 3> color_param);
};

class Dir_light final :public Light_pv {
public:
	lgo::svec<float, 3> dir{};
	Dir_light(lgo::svec<float, 3> pos_param,
		lgo::svec<float, 3> dir_param, lgo::svec<float, 3> color_param);
};

class Spotlight final :public Light_pv {
public:
	lgo::svec<float, 3> dir{};
	float inner_cone{};
	float outer_cone{};
	Spotlight(lgo::svec<float, 3> pos_param, lgo::svec<float, 3> dir_param,
		float inner_cone_param, float outer_cone_param, lgo::svec<float, 3> color_param);
};

#endif