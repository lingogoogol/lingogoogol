#include <algorithm>

#include "cell.h"
#include "math.h"
#include "block.h"
#include "holder.h"
#include "surface.h"
#include "value_f.h"

Cell_pv::Cell_pv(glm::ivec3 pos_param, Block* parent_block_param, Cell_pv** identity_param) :
	pos{ pos_param }, parent_block{ parent_block_param }, identity{ identity_param } {
	return;
}

const glm::ivec3* Cell_pv::get_pos() {
	return &pos;
}

void Cell_pv::update_surface() {
	switch (get_actual_cell()->get_material()) {
	case Cell_material::Air:
		break;
	case Cell_material::Stone: {
		glm::ivec3 air{};
		for (int i{ 0 }; i < 3; i++)
			for (int j{ -1 }; j <= 1; j += 2) {
				glm::ivec3 relative_pos{ 0,0,0 };
				relative_pos[i] += j;
				Cell_pv* cell{ get_cell(pos + relative_pos) };
				if (!cell)
					air += relative_pos;
				else if (cell->get_material() == Cell_material::Air)
					air += relative_pos;
			}
		std::vector<Pair<glm::ivec3, glm::vec3>> vertices{};
		for (int i{ 0 }; i < 3; i++)
			for (int j{ -1 }; j <= 1; j += 2) {
				glm::ivec3 relative_pos_j{ 0,0,0 };
				relative_pos_j[i] += j;
				Cell_pv* cell_j{ get_cell(pos + relative_pos_j) };
				if (!cell_j)
					continue;
				else if (cell_j->get_actual_cell()->get_material() != Cell_material::Stone)
					continue;
				bool next_to_diff_j{ false };
				for (int k{ 0 }; k < 3; k++) {
					if (k == i)
						continue;
					for (int l{ -1 }; l <= 1; l += 2) {
						glm::ivec3 relative_pos_l{ relative_pos_j };
						relative_pos_l[k] += l;
						Cell_pv* cell_l{ get_cell(pos + relative_pos_l) };
						if (!cell_l) {
							next_to_diff_j = true;
							continue;
						}
						else if (cell_l->get_actual_cell()->get_material() != Cell_material::Stone) {
							next_to_diff_j = true;
							continue;
						}
						glm::ivec3 relative_pos_temp_l{ relative_pos_l };
						relative_pos_temp_l[i] -= j;
						Cell_pv* cell_temp_l{ get_cell(pos + relative_pos_temp_l) };
						bool next_to_diff_l{};
						if (!cell_temp_l)
							next_to_diff_l = true;
						else
							next_to_diff_l = cell_temp_l->get_actual_cell()->get_material() != Cell_material::Stone;
						for (int m{ 0 }; m < 3; m++) {
							if (m == i || m == k)
								continue;
							for (int n{ -1 }; n <= 1; n += 2) {
								glm::ivec3 relative_pos_n{ relative_pos_l };
								relative_pos_n[m] += n;
								Cell_pv* cell_n{ get_cell(pos + relative_pos_n) };
								if (!cell_n) {
									next_to_diff_l = true;
									continue;
								}
								else if (cell_n->get_actual_cell()->get_material() != Cell_material::Stone) {
									next_to_diff_l = true;
									continue;
								}
								if (std::find(vertices.begin(), vertices.end(),
									Pair{ relative_pos_n, glm::vec3{} }) != vertices.end())
									continue;
								glm::ivec3 relative_pos_temp_n{ relative_pos_n };
								relative_pos_temp_n[i] -= j;
								bool next_to_diff_n{};
								Cell_pv* cell_temp_n{ get_cell(pos + relative_pos_temp_n) };
								if (!cell_temp_n)
									next_to_diff_n = true;
								else
									next_to_diff_n = cell_temp_n->get_actual_cell()->get_material() != Cell_material::Stone;
								relative_pos_temp_n = relative_pos_n;
								relative_pos_temp_n[k] -= l;
								cell_temp_n = get_cell(pos + relative_pos_temp_n);
								if (!cell_temp_n)
									next_to_diff_n = true;
								else
									next_to_diff_n = next_to_diff_n || cell_temp_n->get_actual_cell()->get_material() != Cell_material::Stone;
								if (next_to_diff_n)
									vertices.push_back(Pair{relative_pos_n, glm::vec3{}});
							}
						}
						if (next_to_diff_l && std::find(vertices.begin(), vertices.end(),
							Pair{ relative_pos_l, glm::vec3{} }) == vertices.end())
							vertices.push_back(Pair{relative_pos_l, glm::vec3{}});
					}
				}
				if (next_to_diff_j && std::find(vertices.begin(), vertices.end(),
					Pair{ relative_pos_j, glm::vec3{} }) == vertices.end())
					vertices.push_back(Pair{relative_pos_j, glm::vec3{}});
			}
		glm::vec3 up{};
		if (glm::normalize(static_cast<glm::vec3>(air)) == glm::vec3{ 0.0f,1.0f,0.0f } ||
			glm::normalize(static_cast<glm::vec3>(air)) == glm::vec3{ 0.0f,-1.0f,0.0f })
			up = glm::vec3{ 1.0f,0.0f,0.0f };
		else
			up = glm::vec3{ 0.0f,1.0f,0.0f };
		glm::mat4 transform{ glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f) *
			glm::lookAt(static_cast<glm::vec3>(air), glm::vec3{0.0f,0.0f,0.0f}, up) };
		for (int i{ 0 }; i < vertices.size(); i++) {
			glm::vec4 vec4{ transform * glm::vec4{static_cast<glm::vec3>(vertices[i].first),1.0f} };
			for (int j{ 0 }; j < 3; j++)
				vertices[i].second[j] = vec4[j] / vec4[3];
		}
		std::sort(vertices.begin(), vertices.end(),
			[](const Pair<glm::ivec3, glm::vec3>& comp, const Pair<glm::ivec3, glm::vec3>& comped)->bool {
				if (comped.second.x == 0.0f) {
					if (comped.second.y > 0.0f)
						return false;
					else {
						if (comp.second.x > 0.0f)
							return true;
						else
							return false;
					}
				}
				else if (comped.second.x > 0.0f) {
					if (comp.second.x == 0.0f) {
						if (comp.second.y > 0.0f)
							return true;
						else
							return false;
					}
					else if (comp.second.x > 0.0f)
						return glm::dot(glm::vec3{ 0.0f,1.0f,0.0f }, comp.second) >
						glm::dot(glm::vec3{ 0.0f,1.0f,0.0f }, comped.second);
					else
						return false;
				}
				else {
					if (comp.second.x >= 0.0f)
						return true;
					else
						return glm::dot(glm::vec3{ 0.0f,1.0f,0.0f }, comp.second) <
						glm::dot(glm::vec3{ 0.0f,1.0f,0.0f }, comped.second);
				}
			}
		);
		if (vertices.size() != 0) {
			for (int i{ 0 }; i < vertices.size() - 1; i++)
				child_surfaces.push_back(new Stone_surface{ static_cast<glm::vec3>(pos + vertices[i].first),
					static_cast<glm::vec3>(pos + vertices[static_cast<std::size_t>(i) +
					static_cast<std::size_t>(1)].first), static_cast<glm::vec3>(pos),
					glm::vec2{0.0f,0.0f}, glm::vec2{1.0f,0.0f}, glm::vec2{0.5f,1.0f}, this });
			child_surfaces.push_back(new Stone_surface{ static_cast<glm::vec3>(pos +
				vertices[vertices.size() - 1].first),static_cast<glm::vec3>(pos +
				vertices[0].first), static_cast<glm::vec3>(pos),
				glm::vec2{0.0f,0.0f}, glm::vec2{1.0f,0.0f}, glm::vec2{0.5f,1.0f}, this });
		}
		break;
	}
	default:
		break;
	}
}

void Cell_pv::add_placeholder(Cell_pv* cell) {
	placeholders.push_back(cell);
	return;
}

void Cell_pv::remove_placeholder(Cell_pv* cell) {
	for (std::vector<Cell_pv*>::iterator i{ placeholders.begin() }; i < placeholders.end(); i++)
		if (*i == cell) {
			placeholders.erase(i);
			return;
		}
	throw 0;
}

void Cell_pv::add_placeholders() {
	for (int i{ 0 }; i < 2; i++)
		for (int j{ 0 }; j < 2; j++)
			for (int k{ 0 }; k < 2; k++) {
				if (i == 0 && j == 0 && k == 0)
					continue;
				glm::ivec3 relative_pos{ i,j,k };
				Cell_pv* cell{ get_cell(pos + relative_pos) };
				if (cell)
					cell->add_placeholder(this);
			}
	return;
}

void Cell_pv::remove_placeholders() {
	for (int i{ 0 }; i < 2; i++)
		for (int j{ 0 }; j < 2; j++)
			for (int k{ 0 }; k < 2; k++) {
				if (i == 0 && j == 0 && k == 0)
					continue;
				glm::ivec3 relative_pos{ i,j,k };
				Cell_pv* cell{ get_cell(pos + relative_pos) };
				if (cell)
					cell->remove_placeholder(this);
			}
	return;
}

void Cell_pv::render() {
	for (int i{ 0 }; i < child_surfaces.size(); i++)
		child_surfaces[i]->render();
	return;
}

Air_cell::Air_cell(glm::vec3 pos_param, Block* parent_block_param, Cell_pv** identity_param) :
	Cell_pv{ pos_param,parent_block_param,identity_param } {
	return;
}

void Air_cell::place(Cell_material material) {
	return;
}

Cell_material Air_cell::get_material() {
	return Cell_material::Air;
}

Cell_pv* Air_cell::get_actual_cell() {
	for (int i{ 0 }; i < placeholders.size(); i++)
		if (placeholders[i]->get_material() != Cell_material::Air)
			return placeholders[i];
	return this;
}

Stone_cell::Stone_cell(glm::ivec3 pos_param, Block* parent_block_param, Cell_pv** identity_param) :
	Cell_pv{ pos_param,parent_block_param,identity_param } {
	return;
}


Cell_pv* Stone_cell::get_actual_cell() {
	return this;
}

Cell_material Stone_cell::get_material() {
	return Cell_material::Stone;
}

void Stone_cell::broke() {
	return;
}

Cell_pv* get_cell(glm::ivec3 coord) {
	glm::ivec3 block{ to_block(coord) };
	glm::ivec3 coord_in_block{ to_coord_in_block(coord) };
	glm::ivec3 first_block_pos{ (*object::blocks[0][0][0])->get_pos() };
	std::size_t index_y{ static_cast<std::size_t>(block[0]) - static_cast<std::size_t>(first_block_pos[0]) };
	std::size_t index_x{ static_cast<std::size_t>(block[1]) - static_cast<std::size_t>(first_block_pos[1]) };
	std::size_t index_z{ static_cast<std::size_t>(block[2]) - static_cast<std::size_t>(first_block_pos[2]) };
	if (index_y >= constant::load_block_side_length ||
		index_x >= constant::load_block_side_length ||
		index_z >= constant::load_block_side_length)
		return nullptr;
	else
		return (*object::blocks[index_y][index_x][index_z])->get_child_cells()
		    [coord_in_block.y][coord_in_block.x][coord_in_block.z];
}