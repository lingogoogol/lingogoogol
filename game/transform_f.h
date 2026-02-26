#ifndef TRANSFORM_F_H
#define TRANSFORM_F_H

#include "cell_f.h"
#include "data_f.h"
#include "surface_f.h"
#include "block_f.h"

lgo::svec<float, 3> to_point_in_cell(lgo::svec<float, 3> point);
lgo::svec<int, 3> to_coord(lgo::svec<float, 3> point);
lgo::svec<int, 3> to_coord_in_block(lgo::svec<int, 3> coord);
lgo::svec<int, 3> to_block(lgo::svec<int, 3> coord);
bool point_at_edge(lgo::svec<float, 3> point);
bool point_at_edge_meet_surface(lgo::svec<float, 3> point, Surface* surface);
lgo::svec<float, 3> to_cell_side(lgo::svec<float, 3> begin, lgo::svec<float, 3> dir);
Block* get_block(lgo::svec<int, 3> pos);
Cell* get_cell(lgo::svec<int, 3> pos);
template<typename... T1>
Cell* set_cell(lgo::svec<int, 3> pos, Cell_type type, T1... params);
std::vector<Cell*> get_include_cells(const lgo::arrs<lgo::svec<float, 3>, 3>& vertices);

#endif