#ifndef TILE_H
#define TILE_H

#include <vector>

#include "light_f.h"
#include "tile_f.h"
#include "value_f.h"

class Tile {
public:
	Tile();
	void render(World_data* data);
private:
	unsigned int VAO{};
	unsigned int VBO{};
	std::vector<Dir_light*> dir_lights{};
	std::vector<Point_light*> point_lights{};
	std::vector<Spotlight*> spotlights{};
};

#endif