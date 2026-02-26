#include <functional>
#include <cmath>

#include <lgo/data/.h>

#include "transform_i.h"
#include "const_f.h"
#include "surface_d.h"
#include "data_d.h"
#include "block_d.h"
#include "cell_d.h"

lgo::svec<float, 3> to_point_in_cell(lgo::svec<float, 3> point) {
	for (std::int_fast8_t i{ 0 }; i < 3; ++i)
		point[i] -= std::floor(point[i]);
	return point;
}

lgo::svec<int, 3> to_coord(lgo::svec<float, 3> point) {
	lgo::svec<int, 3> coord{};
	for (std::int_fast8_t i{ 0 }; i < 3; ++i)
		coord[i] = lgo::sc<int>(std::floor(point[i]));
	return coord;
}

lgo::svec<int, 3> to_coord_in_block(lgo::svec<int, 3> coord) {
	for (int i{ 0 }; i < 3; ++i)
		coord[i] = coord[i] % constant::cell_num + (coord[i] >= 0 ? 0 : 1);
	return coord;
}

lgo::svec<int, 3> to_block(lgo::svec<int, 3> coord) {
	for (int i{ 0 }; i < 3; ++i)
		coord[i] = coord[i] / constant::cell_num - (coord[i] >= 0 ? 0 :
			(coord[i] % constant::cell_num == 0 ? 0 : 1));
	return coord;
}

bool point_at_edge(lgo::svec<float, 3> point) {
	int side{ 0 };
	point = to_point_in_cell(point);
	for (std::int_fast8_t i{ 0 }; i < 3; ++i)
		if (point[i] == 0.0f)
			side++;
	return side >= 2;
}

bool point_at_edge_meet_surface(lgo::svec<float, 3> point, Surface* surface) {
	point = to_point_in_cell(point);
	if (point == lgo::svec<float, 3>{ 0.0f,0.0f,0.0f }) {
		for (int i{ 0 }; i < 3; ++i) {
			if (to_point_in_cell(surface->get_vertices()[i]) ==
				lgo::svec<float, 3>{ 0.0f, 0.0f, 0.0f })
				return true;
		}
		return false;
	}
	bool smaller{ false };
	bool bigger{ false };
	int axis{};
	for (int i{ 0 }; i < 3; ++i)
		if (point[i] != 0)
			axis = i;
	for (int i{ 0 }; i < 3; ++i) {
		bool same_axis{ true };
		lgo::svec<float, 3> vertex{ to_point_in_cell(surface->get_vertices()[i]) };
		for (int j{ 0 }; j < 3; ++j)
			if (j != axis && vertex[j] != 0) {
				same_axis = false;
				break;
			}
		if (same_axis)
			if (vertex[axis] == point[axis])
				return true;
			else if (vertex[axis] < point[axis])
				smaller = true;
			else if (vertex[axis] > point[axis])
				bigger = true;
	}
	return smaller && bigger;
}

lgo::svec<float, 3> to_cell_side(lgo::svec<float, 3> begin, lgo::svec<float, 3> dir) {
	lgo::svec<float, 3> coefficient{};
	for (int i{ 0 }; i < 3; ++i)
		coefficient[i] = dir[i] == 0 ? std::numeric_limits<float>::infinity() :
		(((dir[i] > 0 ? std::floor(begin[i] + 1) : std::ceil(begin[i] - 1)) - begin[i]) / dir[i]);
	float dist_to_cell_side{ std::min(coefficient[0], std::min(coefficient[1], coefficient[2])) };
	for (int i{ 0 }; i < 3; ++i)
		begin[i] = coefficient[i] == dist_to_cell_side ? (dir[i] > 0 ? std::floor(begin[i] + 1) :
			std::ceil(begin[i] - 1)) : (begin[i] + dir[i] * dist_to_cell_side);
	return begin;
};

Block* get_block(lgo::svec<int, 3> pos) {
	World_data* world_data{ get_world_data() };
	lgo::svec<int, 3> block{ to_block(pos) };
	lgo::svec<int, 3> first_block_pos{ world_data->blocks[0][0][0]->get_pos() };
	int index_x{ block[1] - first_block_pos[1] };
	int index_y{ block[0] - first_block_pos[0] };
	int index_z{ block[2] - first_block_pos[2] };
	if (index_x >= constant::block_num || index_x < 0 ||
		index_y >= constant::block_num || index_y < 0 ||
		index_z >= constant::block_num || index_z < 0)
		return nullptr;
	else
		return world_data->blocks[index_y][index_x][index_z];
}

Cell* get_cell(lgo::svec<int, 3> pos) {
	Block* block{ get_block(pos) };
	if (block)
		return block->get_child_cell(pos);
	else
		return nullptr;
}

std::vector<Cell*> get_include_cells(const lgo::arrs<lgo::svec<float, 3>, 3>& vertices) {
	auto func{
		[&vertices](std::function<bool(float, float)> comp_func,
			std::function<int(float)> final_func)->lgo::svec<int, 3> {
			lgo::svec<float, 3> pos_f{ vertices[0] };
			for (int i{ 1 }; i < 3; ++i)
				for (int j{ 0 }; j < 3; ++j)
					if (comp_func(vertices[i][j], pos_f[j]))
						pos_f[j] = vertices[i][j];
			lgo::svec<int, 3> pos_i{};
			for (int i{ 0 }; i < 3; ++i)
				pos_i[i] = final_func(pos_f[i]);
			return pos_i;
		}
	};
	lgo::svec<int, 3> min_pos{ func(
		[](float comp, float comped)->bool { return comp < comped; },
		[](float before)->int { return lgo::sc<int>(std::floor(before)); }
	) };
	lgo::svec<int, 3> max_pos{ func(
		[](float comp, float comped)->bool { return comp > comped; },
		[](float before)->int { return lgo::sc<int>(std::ceil(before)); }
	) };
	for (int i{ 0 }; i < 3; ++i)
		if (max_pos[i] == min_pos[i])
			max_pos[i]++;
	std::vector<Cell*> result{};
	for (int x{ min_pos[0] }; x < max_pos[0]; x++)
		for (int y{ min_pos[1] }; y < max_pos[1]; y++)
			for (int z{ min_pos[2] }; z < max_pos[2]; z++) {
				result.push_back(get_cell({ x, y, z }));
				if (!*(result.end() - 1))
					handle_error(U"�����s�b�@�ɥ~�A�L�k�x�s���m�C");
			}
	return result;
}