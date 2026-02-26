#include "transform_f.h"
#include "const_f.h"
#include "surface.h"
#include "data.h"
#include "block.h"

glm::vec3 to_point_in_cell(glm::vec3 point) {
	for (std::int_fast8_t i{ 0 }; i < 3; i++)
		point[i] -= std::floor(point[i]);
	return point;
}

glm::ivec3 to_coord(glm::ivec3 point) {
	glm::ivec3 coord{};
	for (std::int_fast8_t i{ 0 }; i < 3; i++)
		coord[i] = static_cast<int>(std::floor(point[i]));
	return coord;
}

glm::ivec3 to_coord_in_block(glm::ivec3 coord) {
	for (int i{ 0 }; i < 3; i++)
		coord[i] = coord[i] % constant::cell_num + (coord[i] >= 0 ? 0 : 1);
	return coord;
}

glm::ivec3 to_block(glm::ivec3 coord) {
	for (int i{ 0 }; i < 3; i++)
		coord[i] = coord[i] / constant::cell_num - (coord[i] >= 0 ? 0 :
			(coord[i] % constant::cell_num == 0 ? 0 : 1));
	return coord;
}

bool point_at_edge(glm::vec3 point) {
	int side{ 0 };
	point = to_point_in_cell(point);
	for (std::int_fast8_t i{ 0 }; i < 3; i++)
		if (point[i] == 0.0f)
			side++;
	return side >= 2;
}

bool point_at_edge_meet_surface(glm::vec3 point, Surface_pv* surface) {
	point = to_point_in_cell(point);
	if (point == glm::vec3{ 0.0f,0.0f,0.0f }) {
		for (int i{ 0 }; i < 3; i++) {
			if (to_point_in_cell(surface->get_vertices()[i]) == glm::vec3{ 0.0f,0.0f,0.0f })
				return true;
		}
		return false;
	}
	bool smaller{ false };
	bool bigger{ false };
	int axis{};
	for (int i{ 0 }; i < 3; i++)
		if (point[i] != 0)
			axis = i;
	for (int i{ 0 }; i < 3; i++) {
		bool same_axis{ true };
		glm::vec3 vertex{ to_point_in_cell(surface->get_vertices()[i]) };
		for (int j{ 0 }; j < 3; j++)
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

glm::vec3 to_cell_side(glm::vec3 begin, glm::vec3 dir) {
	glm::vec3 coefficient{};
	for (int i{ 0 }; i < 3; i++)
		coefficient[i] = dir[i] == 0 ? std::numeric_limits<float>::infinity() :
		(((dir[i] > 0 ? std::floor(begin[i] + 1) : std::ceil(begin[i] - 1)) - begin[i]) / dir[i]);
	float dist_to_cell_side{ std::min(coefficient.x, std::min(coefficient.y, coefficient.z)) };
	for (int i{ 0 }; i < 3; i++)
		begin[i] = coefficient[i] == dist_to_cell_side ? (dir[i] > 0 ? std::floor(begin[i] + 1) :
			std::ceil(begin[i] - 1)) : (begin[i] + dir[i] * dist_to_cell_side);
	return begin;
};

Cell_pv* get_cell(glm::ivec3 pos, World_data* data) {
	glm::ivec3 block{ to_block(pos) };
	glm::ivec3 coord_in_block{ to_coord_in_block(pos) };
	glm::ivec3 first_block_pos{ (*data->blocks[0][0][0])->get_pos() };
	std::size_t index_y{ static_cast<std::size_t>(block[0]) - static_cast<std::size_t>(first_block_pos[0]) };
	std::size_t index_x{ static_cast<std::size_t>(block[1]) - static_cast<std::size_t>(first_block_pos[1]) };
	std::size_t index_z{ static_cast<std::size_t>(block[2]) - static_cast<std::size_t>(first_block_pos[2]) };
	if (index_y >= constant::block_num ||
		index_x >= constant::block_num ||
		index_z >= constant::block_num)
		return nullptr;
	else
		return (*data->blocks[index_y][index_x][index_z])->get_child_cells()
		[coord_in_block.y][coord_in_block.x][coord_in_block.z];
}

Cell_pv* get_included_cell(const std::array<glm::vec3, 3>& vertices, World_data* data) {
	glm::vec3 cell_pos_f{ vertices[0] };
	for (int i{ 1 }; i < 3; i++)
		for (int j{ 0 }; j < 3; j++)
			if (vertices[i][j] < cell_pos_f[j])
				cell_pos_f[j] = vertices[i][j];
	glm::ivec3 cell_pos_i{};
	for (int i{ 0 }; i < 3; i++)
		cell_pos_i[i] = static_cast<int>(std::floor(cell_pos_f[i]));
	return get_cell(cell_pos_i, data);
}