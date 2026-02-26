#ifndef CELL_H
#define CELL_H

#include <glm/glm.hpp>

#include "object.h"
#include "cell_f.h"

enum class Cell_material {
	Stone
};

class Cell_pv {
public:
	Cell_pv();
	Cell_pv(Cell_material material_param, glm::ivec3 pos_param);
	const glm::ivec3* get_pos();
protected:
	Cell_material material{};
	glm::ivec3 pos{};
};

#endif