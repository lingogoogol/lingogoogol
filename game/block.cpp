#include "block_d.h"
#include "cell_d.h"
#include "data_d.h"
#include "transform_i.h"

Block::Block(lgo::svec<int, 3> pos_param) :pos{ pos_param } {}

void Block::gen_cell() {
	for (int i{ 0 }; i < constant::cell_num; ++i)
		for (int j{ 0 }; j < constant::cell_num; ++j)
			for (int k{ 0 }; k < constant::cell_num; ++k)
				if ((i == 8 && j == 8 && k == 8) ||
					(i == 3 && j == 6 && k == 3) ||
					(i == 4 && j == 5 && k == 3) ||
					(i == 4 && j == 5 && k == 4) ||
					(i == 4 && j == 6 && k == 3) ||
					(i == 4 && j == 6 && k == 4) ||
					(i == 4 && j == 7 && k == 3) ||
					(i == 4 && j == 9 && k == 4) ||
					(i == 5 && j == 6 && k == 3) ||
					(i == 5 && j == 6 && k == 4) ||
					(i == 5 && j == 6 && k == 5) ||
					(i == 5 && j == 7 && k == 4) ||
					(i == 5 && j == 7 && k == 5) ||
					(i == 5 && j == 7 && k == 6) ||
					(i == 5 && j == 8 && k == 4) ||
					(i == 5 && j == 9 && k == 4) ||
					(i == 5 && j == 9 && k == 5) ||
					(i == 5 && j == 10 && k == 4) ||
					(i == 5 && j == 10 && k == 5) ||
					(i == 6 && j == 6 && k == 5) ||
					(i == 6 && j == 6 && k == 6) ||
					(i == 6 && j == 7 && k == 5) ||
					(i == 6 && j == 7 && k == 6) ||
					(i == 6 && j == 8 && k == 6) ||
					(i == 6 && j == 8 && k == 7) ||
					(i == 6 && j == 11 && k == 5) ||
					(i == 6 && j == 12 && k == 6) ||
					(i == 6 && j == 13 && k == 6) ||
					(i == 7 && j == 7 && k == 6) ||
					(i == 7 && j == 7 && k == 7) ||
					(i == 7 && j == 8 && k == 6) ||
					(i == 7 && j == 8 && k == 7) ||
					(i == 7 && j == 8 && k == 8) ||
					(i == 7 && j == 11 && k == 6) ||
					(i == 7 && j == 12 && k == 6) ||
					(i == 7 && j == 12 && k == 7) ||
					(i == 7 && j == 14 && k == 7) ||
					(i == 7 && j == 15 && k == 7) ||
					(i == 8 && j == 7 && k == 8) ||
					(i == 8 && j == 8 && k == 7) ||
					(i == 8 && j == 8 && k == 8) ||
					(i == 8 && j == 9 && k == 8) ||
					(i == 8 && j == 11 && k == 7) ||
					(i == 8 && j == 11 && k == 8) ||
					(i == 9 && j == 8 && k == 8) ||
					(i == 9 && j == 9 && k == 8) ||
					(i == 9 && j == 11 && k == 9) ||
					(i == 10 && j == 11 && k == 10))
					set_child_cell(*new Stone_cell{ lgo::svec<int, 3>{ pos[0] * constant::cell_num + i,
						pos[1] * constant::cell_num + j, pos[2] * constant::cell_num + k } });
				else
					set_child_cell(*new Air_cell{ lgo::svec<int, 3>{ pos[0] * constant::cell_num + i,
						pos[1] * constant::cell_num + j, pos[2] * constant::cell_num + k } });
}

const lgo::svec<int, 3>& Block::get_pos() {
	return pos;
}

Cell* Block::get_child_cell(lgo::svec<int, 3> pos_param) {
	lgo::svec<int, 3> indices{ to_coord_in_block(pos_param) };
	return child_cells[indices[1]][indices[0]][indices[2]];
}

void Block::set_child_cell(Cell& cell) {
	lgo::svec<int, 3> indices{ to_coord_in_block(cell.get_pos()) };
	Cell*& target{ child_cells[indices[1]][indices[0]][indices[2]] };
	if (target) {
		target->remove_placeholders();
		target->remove_child_surfaces();
		cell.placeholders = target->placeholders;
		cell.include_surfaces = target->include_surfaces;
		cell.point_lights = target->point_lights;
		cell.dir_lights = target->dir_lights;
		cell.spotlights = target->spotlights;
	}
	target = &cell;
}

void Block::add_placeholders() {
	for (int i{ 0 }; i < constant::cell_num; ++i)
		for (int j{ 0 }; j < constant::cell_num; ++j)
			for (int k{ 0 }; k < constant::cell_num; ++k)
				child_cells[i][j][k]->add_placeholders();
}

void Block::update_surface() {
	for (int i{ 0 }; i < constant::cell_num; ++i)
		for (int j{ 0 }; j < constant::cell_num; ++j)
			for (int k{ 0 }; k < constant::cell_num; ++k)
				child_cells[i][j][k]->update_surface();
}

void Block::destruct() {}

void Block::render() {
	for (int i{ 0 }; i < constant::cell_num; ++i)
		for (int j{ 0 }; j < constant::cell_num; ++j)
			for (int k{ 0 }; k < constant::cell_num; ++k)
				child_cells[i][j][k]->render();
}