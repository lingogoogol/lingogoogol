#include "block.h"
#include "cell.h"

Block::Block() {
	return;
}

Block::Block(glm::ivec3 pos_param) :pos{ pos_param } {
	for (int i{ 0 }; i < constant::block_side_length; i++)
		for (int j{ 0 }; j < constant::block_side_length; j++)
			for (int k{ 0 }; k < constant::block_side_length; k++)
				if (i == 8 && j == 8 && k == 8)
					child_cells[i][j][k] = new Stone_cell{ glm::ivec3{i,j,k},this,&child_cells[i][j][k] };
				else
					child_cells[i][j][k] = new Air_cell{ glm::ivec3{i,j,k},this,&child_cells[i][j][k] };
	return;
}

const glm::ivec3& Block::get_pos() {
	return pos;
}

std::array<std::array<std::array<Cell_pv*, constant::block_side_length>,
	constant::block_side_length>, constant::block_side_length>& Block::get_child_cells() {
	return child_cells;
}

void Block::update() {
	for (int i{ 0 }; i < constant::block_side_length; i++)
		for (int j{ 0 }; j < constant::block_side_length; j++)
			for (int k{ 0 }; k < constant::block_side_length; k++)
				child_cells[i][j][k]->add_placeholders();
	for (int i{ 0 }; i < constant::block_side_length; i++)
		for (int j{ 0 }; j < constant::block_side_length; j++)
			for (int k{ 0 }; k < constant::block_side_length; k++)
				child_cells[i][j][k]->update_surface();
	return;
}

void Block::destruct() {
	return;
}

void Block::render() {
	for (int i{ 0 }; i < constant::block_side_length; i++)
		for (int j{ 0 }; j < constant::block_side_length; j++)
			for (int k{ 0 }; k < constant::block_side_length; k++)
				child_cells[i][j][k]->render();
	for (int i{ 0 }; i < child_objects.size(); i++)
		child_objects[i]->render();
	return;
}