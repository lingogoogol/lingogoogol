#include "block.h"
#include "cell.h"
#include "data.h"

Block::Block(glm::ivec3 pos_param) :pos{ pos_param } {
	for (int i{ 0 }; i < constant::cell_num; i++)
		for (int j{ 0 }; j < constant::cell_num; j++)
			for (int k{ 0 }; k < constant::cell_num; k++)
				if (i == 8 && j == 8 && k == 8)
					child_cells[i][j][k] = new Stone_cell{ glm::ivec3{i,j,k},this,&child_cells[i][j][k] };
				else
					child_cells[i][j][k] = new Air_cell{ glm::ivec3{i,j,k},this,&child_cells[i][j][k] };
	return;
}

const glm::ivec3& Block::get_pos() {
	return pos;
}

std::array<std::array<std::array<Cell_pv*, constant::cell_num>,
	constant::cell_num>, constant::cell_num>& Block::get_child_cells() {
	return child_cells;
}

void Block::update(World_data* data) {
	for (int i{ 0 }; i < constant::cell_num; i++)
		for (int j{ 0 }; j < constant::cell_num; j++)
			for (int k{ 0 }; k < constant::cell_num; k++)
				child_cells[i][j][k]->add_placeholders(data);
	for (int i{ 0 }; i < constant::cell_num; i++)
		for (int j{ 0 }; j < constant::cell_num; j++)
			for (int k{ 0 }; k < constant::cell_num; k++)
				child_cells[i][j][k]->update_surface(data);
	return;
}

void Block::destruct() {
	return;
}

void Block::render(World_data* data) {
	for (int i{ 0 }; i < constant::cell_num; i++)
		for (int j{ 0 }; j < constant::cell_num; j++)
			for (int k{ 0 }; k < constant::cell_num; k++)
				child_cells[i][j][k]->render(data);
	for (int i{ 0 }; i < child_objects.size(); i++)
		child_objects[i]->render();
	return;
}