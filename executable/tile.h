#ifndef TILE_H
#define TILE_H

#include <vector>

#include "light.h"
#include "tile_f.h"

class Tile {
public:
	Tile();
private:
	unsigned int tex{};
	std::vector<Light_pv*> light{};
};

#endif