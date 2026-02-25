#ifndef CELL_H
#define CELL_H

#include <glm/glm.hpp>

#include "object.h"
#include "cell_f.h"

enum class Cell_material {
	Stone
};

class Cell {
	Cell();
	Cell(Cell_material material_param, glm::ivec3 pos_param);
public:
	bool belonged_to_world{};
	Object* belonged_object{};
	Cell_material material{};
	glm::ivec3 pos{};
};

#endif