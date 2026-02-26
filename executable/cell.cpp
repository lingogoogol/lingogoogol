#include "cell.h"

Cell_pv::Cell_pv() {
	return;
}

Cell_pv::Cell_pv(Cell_material material_param, glm::ivec3 pos_param) :material{ material_param }, pos{ pos_param } {
	return;
}

const glm::ivec3* Cell_pv::get_pos() {
	return &pos;
}