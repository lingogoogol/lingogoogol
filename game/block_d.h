#ifndef BLOCK_H
#define BLOCK_H

#include <lgo/lib/glad.h>

#include "block_f.h"
#include "cell_f.h"
#include "const_f.h"

class Block {
public:
	Block(lgo::svec<int, 3> pos_param);
	void gen_cell();
	const lgo::svec<int, 3>& get_pos();
	Cell* get_child_cell(lgo::svec<int, 3> pos_param);
	void set_child_cell(Cell& cell);
	void add_placeholders();
	void update_surface();
	void destruct();
	void render();
private:
	lgo::svec<int, 3> pos{};
	lgo::arrs<lgo::arrs<lgo::arrs<Cell*, constant::cell_num>,
		constant::cell_num>, constant::cell_num> child_cells{};
};

#endif