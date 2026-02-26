#include <algorithm>
#include <array>
#include <cmath>

#include "cell.h"
#include "math.h"
#include "block.h"
#include "lib.h"
#include "surface.h"
#include "data.h"
#include "transform_f.h"

Cell_pv::Cell_pv(glm::ivec3 pos_param, Block* parent_block_param, Cell_pv** identity_param) :
	pos{ pos_param }, parent_block{ parent_block_param }, identity{ identity_param } {
	return;
}

const glm::ivec3& Cell_pv::get_pos() {
	return pos;
}

const std::vector<Surface_pv*> Cell_pv::get_include_surfaces() {
	return include_surfaces;
}

void Cell_pv::update_surface(World_data* data) {
	switch (get_actual_cell()->get_type()) {
	case Cell_type::Air:
		break;
	case Cell_type::Stone: {
		glm::ivec3 air{};
		for (int i{ -1 }; i <= 1; i++)
			for (int j{ -1 }; j <= 1; j++)
				for (int k{ -1 }; k <= 1; k++) {
					glm::ivec3 relative_pos{ i,j,k };
					Cell_pv* cell{ get_cell(pos + relative_pos, data) };
					if (!cell)
						air += relative_pos;
					else if (cell->get_actual_cell()->get_type() == Cell_type::Air)
						air += relative_pos;
				}
		std::vector<Pair<glm::ivec3, glm::vec3>> vertices{};
		for (int i{ 0 }; i < 3; i++)
			for (int j{ -1 }; j <= 1; j += 2) {
				glm::ivec3 relative_pos_j{ 0,0,0 };
				relative_pos_j[i] += j;
				Cell_pv* cell_j{ get_cell(pos + relative_pos_j, data) };
				if (!cell_j)
					continue;
				else if (cell_j->get_actual_cell()->get_type() != Cell_type::Stone)
					continue;
				bool next_to_diff_j{ false };
				for (int k{ 0 }; k < 3; k++) {
					if (k == i)
						continue;
					for (int l{ -1 }; l <= 1; l += 2) {
						glm::ivec3 relative_pos_l{ relative_pos_j };
						relative_pos_l[k] += l;
						Cell_pv* cell_l{ get_cell(pos + relative_pos_l, data) };
						if (!cell_l) {
							next_to_diff_j = true;
							continue;
						}
						else if (cell_l->get_actual_cell()->get_type() != Cell_type::Stone) {
							next_to_diff_j = true;
							continue;
						}
						glm::ivec3 relative_pos_temp_l{ relative_pos_l };
						relative_pos_temp_l[i] -= j;
						Cell_pv* cell_temp_l{ get_cell(pos + relative_pos_temp_l, data) };
						bool next_to_diff_l{};
						if (!cell_temp_l)
							next_to_diff_l = true;
						else
							next_to_diff_l = cell_temp_l->get_actual_cell()->get_type() != Cell_type::Stone;
						for (int m{ 0 }; m < 3; m++) {
							if (m == i || m == k)
								continue;
							for (int n{ -1 }; n <= 1; n += 2) {
								glm::ivec3 relative_pos_n{ relative_pos_l };
								relative_pos_n[m] += n;
								Cell_pv* cell_n{ get_cell(pos + relative_pos_n, data) };
								if (!cell_n) {
									next_to_diff_l = true;
									continue;
								}
								else if (cell_n->get_actual_cell()->get_type() != Cell_type::Stone) {
									next_to_diff_l = true;
									continue;
								}
								if (std::find(vertices.begin(), vertices.end(),
									Pair{ relative_pos_n, glm::vec3{} }) != vertices.end())
									continue;
								glm::ivec3 relative_pos_temp_n{ relative_pos_n };
								relative_pos_temp_n[i] -= j;
								bool next_to_diff_n{};
								Cell_pv* cell_temp_n{ get_cell(pos + relative_pos_temp_n, data) };
								if (!cell_temp_n)
									next_to_diff_n = true;
								else
									next_to_diff_n = cell_temp_n->get_actual_cell()->get_type() != Cell_type::Stone;
								relative_pos_temp_n = relative_pos_n;
								relative_pos_temp_n[k] -= l;
								cell_temp_n = get_cell(pos + relative_pos_temp_n, data);
								if (!cell_temp_n)
									next_to_diff_n = true;
								else
									next_to_diff_n = next_to_diff_n || cell_temp_n->get_actual_cell()->get_type() != Cell_type::Stone;
								if (next_to_diff_n)
									vertices.push_back(Pair{ relative_pos_n, glm::vec3{} });
							}
						}
						if (next_to_diff_l && std::find(vertices.begin(), vertices.end(),
							Pair{ relative_pos_l, glm::vec3{} }) == vertices.end())
							vertices.push_back(Pair{ relative_pos_l, glm::vec3{} });
					}
				}
				if (next_to_diff_j && std::find(vertices.begin(), vertices.end(),
					Pair{ relative_pos_j, glm::vec3{} }) == vertices.end())
					vertices.push_back(Pair{ relative_pos_j, glm::vec3{} });
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
		if (vertices.size() == 0)
			break;
		for (int i{ 0 }; i < vertices.size(); i++) {
			std::array<glm::vec3, 3> triangle{ static_cast<glm::vec3>(pos + vertices[i].first),
				static_cast<glm::vec3>(pos + vertices[static_cast<std::size_t>((i == 0) ? vertices.
				size() : i) - static_cast<std::size_t>(1)].first), static_cast<glm::vec3>(pos) };
			Cell_pv* included_cell{ get_included_cell(triangle, data) };
			if (!included_cell) {
				handle_error(U"無法存取同一單元的表面，因為該單元不在已讀取區塊的範圍內。");
				return;
			}
			bool to_push{ true };
			for (int j{ 0 }; j < 3; j++) {
				const std::vector<Surface_pv*>& include_surfaces_var{ included_cell->get_include_surfaces() };
				for (int k{ 0 }; k < include_surfaces_var.size(); k++)
					for (int l{ 0 }; l < 3; l++) {
						const std::array<glm::vec3, 3>& include_surface{ include_surfaces_var[k]->get_vertices() };
						if (triangle[j] != include_surface[l])
							continue;
						for (int m{ 0 }; m < 3; m++) {
							if (m == j)
								continue;
							for (int n{ 0 }; n < 3; n++) {
								if (n == l || triangle[m] != include_surface[n])
									continue;
								for (int o{ 0 }; o < 3; o++) {
									if (o == j || o == m)
										continue;
									for (int p{ 0 }; p < 3; p++) {
										if (p == l || p == n)
											continue;
										Line line{ triangle[j], triangle[m] };
										glm::vec3 vec1{ triangle[o] - line.foot_of_perpendicular(triangle[o]) };
										glm::vec3 vec2{ include_surface[p] - line.foot_of_perpendicular(include_surface[p]) };
										if (std::abs(glm::dot(vec1, vec2) - glm::length(vec1) * glm::length(vec2)) < 0.000001f)
											to_push = false;
									}
								}
							}
						}
					}
			}
			if (to_push)
				child_surfaces.push_back(new Stone_surface{ triangle,
					{glm::vec2{0.0f,0.0f}, glm::vec2{1.0f,0.0f}, glm::vec2{0.5f,1.0f}}, this, data });
		}
		break;
	}
	default:
		break;
	}
	return;
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

void Cell_pv::add_placeholders(World_data* data) {
	for (int i{ 0 }; i < 2; i++)
		for (int j{ 0 }; j < 2; j++)
			for (int k{ 0 }; k < 2; k++) {
				if (i == 0 && j == 0 && k == 0)
					continue;
				glm::ivec3 relative_pos{ i,j,k };
				Cell_pv* cell{ get_cell(pos + relative_pos, data) };
				if (cell)
					cell->add_placeholder(this);
			}
	return;
}

void Cell_pv::remove_placeholders(World_data* data) {
	for (int i{ 0 }; i < 2; i++)
		for (int j{ 0 }; j < 2; j++)
			for (int k{ 0 }; k < 2; k++) {
				if (i == 0 && j == 0 && k == 0)
					continue;
				glm::ivec3 relative_pos{ i,j,k };
				Cell_pv* cell{ get_cell(pos + relative_pos, data) };
				if (cell)
					cell->remove_placeholder(this);
			}
	return;
}

void Cell_pv::add_include_surface(Surface_pv* surface) {
	include_surfaces.push_back(surface);
	return;
}

void Cell_pv::remove_include_surface(Surface_pv* surface, World_data* data) {
	for (std::vector<Surface_pv*>::iterator i{ include_surfaces.begin() }; i < include_surfaces.end(); i++)
		if (*i == surface) {
			include_surfaces.erase(i);
			return;
		}
	handle_error(U"移除包含的表面時在該單元找不到。");
	return;
}

void Cell_pv::render(Data_pv* data) {
	for (int i{ 0 }; i < child_surfaces.size(); i++)
		child_surfaces[i]->render();
	return;
}

Air_cell::Air_cell(glm::vec3 pos_param, Block* parent_block_param, Cell_pv** identity_param) :
	Cell_pv{ pos_param,parent_block_param,identity_param } {
	return;
}

void Air_cell::place(Cell_type type) {
	return;
}

Cell_type Air_cell::get_type() {
	return Cell_type::Air;
}

Cell_pv* Air_cell::get_actual_cell() {
	for (int i{ 0 }; i < placeholders.size(); i++)
		if (placeholders[i]->get_type() != Cell_type::Air)
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

Cell_type Stone_cell::get_type() {
	return Cell_type::Stone;
}

void Stone_cell::broke() {
	return;
}