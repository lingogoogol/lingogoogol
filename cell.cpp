#include "cell.h"

Cell::Cell() {
	return;
}

Cell::Cell(Cell_material material_param, glm::ivec3 pos_param) :material{ material_param }, pos{ pos_param } {
	return;
}