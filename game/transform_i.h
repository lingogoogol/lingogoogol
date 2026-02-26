#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <lgo/vec/s/.h>
#include <lgo/debug/.h>

#include "cell_d.h"
#include "transform_f.h"
#include "block_d.h"

template<typename... T1>
Cell* set_cell(lgo::svec<int, 3> pos, Cell_type type, T1... params) {
	Cell* cell{};
	switch (type) {
	case Cell_type::Stone:
		cell = new Stone_cell{ pos, params... };
		break;
	case Cell_type::Air:
		cell = new Air_cell{ pos, params... };
		break;
	default:
		handle_error(U"�������椸�����C");
		return nullptr;
	}
	Block* block{ get_block(pos) };
	if (!block)
		return nullptr;
	block->set_child_cell(*cell);
	cell->add_placeholders();
	for (int i{ -1 }; i <= 2; ++i)
		for (int j{ -1 }; j <= 2; ++j)
			for (int k{ -1 }; k <= 2; ++k) {
				Cell* neighbor_cell{ get_cell({ cell->get_pos()[0] + i, cell->get_pos()[1] + j, cell->get_pos()[2] + k }) };
				if (neighbor_cell)
					neighbor_cell->update_surface();
			}
	return cell;
}

#endif