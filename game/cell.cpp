#include <algorithm>
#include <cmath>

#include <lgo/arr/.h>
#include <lgo/vec/.h>
#include <lgo/mat/.h>
#include <lgo/line/.h>

#include "cell_d.h"
#include "block_d.h"
#include "surface_d.h"
#include "data_d.h"
#include "transform_f.h"

Cell::Cell(lgo::svec<int, 3> pos_param) : pos{ pos_param } {}

Cell& Cell::assign(Cell& cell) {
	if (cell.get_pos() != get_pos())
		handle_error(U"���i�H������m���P���椸�C");
	cell.child_surfaces = child_surfaces;
	return *this;
}

const lgo::svec<int, 3>& Cell::get_pos() const {
	return pos;
}

const std::vector<Cell_surface*> Cell::get_include_surfaces() const {
	return include_surfaces;
}

void Cell::update_surface() {
	get_actual_cell()->update_surface1(*this);
}

void Cell::add_placeholder(Cell* cell) {
	placeholders.push_back(cell);
}

void Cell::remove_placeholder(Cell* cell) {
	for (std::vector<Cell*>::iterator i{ placeholders.begin() }; i < placeholders.end(); ++i)
		if (*i == cell) {
			placeholders.erase(i);
			return;
		}
	handle_error(U"�䤣�����šC");
}

void Cell::add_placeholders() {
	for (int i{ 0 }; i < 2; ++i)
		for (int j{ 0 }; j < 2; ++j)
			for (int k{ 0 }; k < 2; ++k) {
				if (i == 0 && j == 0 && k == 0)
					continue;
				lgo::svec<int, 3> relative_pos{ i, j, k };
				Cell* cell{ get_cell(pos + relative_pos) };
				if (cell)
					cell->add_placeholder(this);
			}
}

void Cell::remove_placeholders() {
	for (int i{ 0 }; i < 2; ++i)
		for (int j{ 0 }; j < 2; ++j)
			for (int k{ 0 }; k < 2; ++k) {
				if (i == 0 && j == 0 && k == 0)
					continue;
				lgo::svec<int, 3> relative_pos{ i, j, k };
				Cell* cell{ get_cell(pos + relative_pos) };
				if (cell)
					cell->remove_placeholder(this);
			}
}

void Cell::add_child_surface(Cell_surface* surface) {
	child_surfaces.push_back(surface);
	std::vector<Cell*> include_cells{ surface->get_include_cells() };
	for (int i{ 0 }; i < include_cells.size(); ++i)
		include_cells[i]->include_surfaces.push_back(surface);
}

void Cell::remove_child_surface(Cell_surface* surface) {
	child_surfaces.erase(std::find(child_surfaces.begin(), child_surfaces.end(), surface));
	std::vector<Cell*> include_cells{ surface->get_include_cells() };
	for (int i{ 0 }; i < include_cells.size(); ++i) {
		std::vector<Cell_surface*>& include_surfaces_var{ include_cells[i]->include_surfaces };
		include_surfaces_var.erase(std::find(
			include_surfaces_var.begin(), include_surfaces_var.end(), surface));
	}
}

void Cell::remove_child_surfaces() {
	World_data* world_data{ get_world_data() };
	while (child_surfaces.size() != 0) {
		Cell_surface* child_surface{ child_surfaces[0] };
		if (child_surface == world_data->selected_surface)
			world_data->selected_surface = nullptr;
		remove_child_surface(child_surface);
		delete child_surface;
	}
}

void Cell::render() const {
	for (int i{ 0 }; i < child_surfaces.size(); ++i)
		child_surfaces[i]->render();
}

Air_cell::Air_cell(lgo::svec<int, 3> pos_param) : Cell{ pos_param } {}

Cell_type Air_cell::get_type() const {
	return Cell_type::Air;
}

Cell* Air_cell::get_actual_cell() {
	for (int i{ 0 }; i < placeholders.size(); ++i)
		if (placeholders[i]->get_type() != Cell_type::Air)
			return placeholders[i];
	return this;
}

void Air_cell::update_surface1(Cell&) {
	remove_child_surfaces();
}

Stone_cell::Stone_cell(lgo::svec<int, 3> pos_param) : Cell{ pos_param } {}

Cell_type Stone_cell::get_type() const {
	return Cell_type::Stone;
}

Cell* Stone_cell::get_actual_cell() {
	return this;
}

void Stone_cell::update_surface1(Cell& cell) {
	cell.remove_child_surfaces();
	auto get_neighbor_cells{
		[&cell] <bool find_air>
		()->std::vector<lgo::svec<int, 3>> {
			std::vector<lgo::svec<int, 3>> result{};
			for (int dir1{ 0 }; dir1 < 3; dir1++)
				for (int offset1{ -1 }; offset1 <= 1; offset1 += 2) {
					lgo::svec<int, 3> pos1{ cell.get_pos() };
					pos1[dir1] += offset1;
					Cell* cell1{ get_cell(pos1) };
					if ((!cell1 || cell1->get_actual_cell()->get_type() == Cell_type::Air) != find_air)
						continue;
					if (std::find(result.begin(), result.end(), pos1) == result.end())
						result.push_back(pos1);
					for (int dir2{ 0 }; dir2 < 3; dir2++) {
						if (dir2 == dir1)
							continue;
						for (int offset2{ -1 }; offset2 <= 1; offset2 += 2) {
							lgo::svec<int, 3> pos2{ pos1 };
							pos2[dir2] += offset2;
							Cell* cell2{ get_cell(pos2) };
							if ((!cell2 || cell2->get_actual_cell()->get_type() ==
								Cell_type::Air) != find_air)
								continue;
							if (std::find(result.begin(), result.end(), pos2) == result.end())
								result.push_back(pos2);
							for (int offset3{ -1 }; offset3 <= 1; offset3 += 2) {
								lgo::svec<int, 3> pos3{ pos2 };
								pos3[3 - dir1 - dir2] += offset3;
								Cell* cell3{ get_cell(pos3) };
								if ((!cell3 || cell3->get_actual_cell()->get_type() ==
									Cell_type::Air) == find_air && std::find(result.begin(),
									result.end(), pos3) == result.end())
									result.push_back(pos3);
							}
						}
					}
				}
			return result;
		}
	};
	std::vector<lgo::svec<int, 3>> diff_type{ get_neighbor_cells.operator()<true>() };
	std::vector<lgo::svec<int, 3>> same_type{ get_neighbor_cells.operator()<false>() };
	std::vector<lgo::svec<int, 3>> vertices{};
	if (same_type.size() == 0)
		return;
	lgo::svec<float, 3> camera_pos{};
	for (int i{ 0 }; i < same_type.size(); ++i)
		camera_pos -= lgo::sc<lgo::svec<float, 3>>(same_type[i] - cell.get_pos());
	for (std::int_fast16_t i{ 0 }; i < same_type.size(); ++i)
		for (std::int_fast16_t j{ 0 }; j < diff_type.size(); ++j)
			if (len(lgo::sc<lgo::svec<float, 3>>(same_type[i] - diff_type[j])) == 1.0f) {
				vertices.push_back(same_type[i]);
				break;
			}
	lgo::svec<float, 3> camera_up{};
	if (normalize(camera_pos) == lgo::svec<float, 3>{ 0.0f, 1.0f, 0.0f } ||
		normalize(camera_pos) == lgo::svec<float, 3>{ 0.0f, -1.0f, 0.0f })
		camera_up = lgo::svec<float, 3>{ 1.0f, 0.0f, 0.0f };
	else
		camera_up = lgo::svec<float, 3>{ 0.0f, 1.0f, 0.0f };
	lgo::smat<float, 4, 4> transform{ rotate(-camera_pos, camera_up) * move(camera_pos) };
	std::sort(vertices.begin(), vertices.end(),
		[transform, &cell](const lgo::svec<int, 3>& src, const lgo::svec<int, 3>& opd)->bool {
			auto cvrt_func{
				[transform, &cell](const lgo::svec<int, 3>& neighbor)->lgo::svec<float, 3> {
					lgo::svec<float, 3> related{ neighbor - cell.get_pos() };
					lgo::svec<float, 4> cvrted{ transform * lgo::sc<lgo::smat<float, 4, 1>>(
						lgo::svec<float, 4>{ related[0], related[1], related[2], 1.0f }) };
					lgo::svec<float, 3> out{};
					for (int i{ 0 }; i < 3; ++i)
						out[i] = cvrted[i] / cvrted[3];
					return out;
				}
			};
			lgo::svec<float, 3> src_proj{ cvrt_func(src) };
			lgo::svec<float, 3> opd_proj{ cvrt_func(opd) };
			return opd_proj[0] == 0.0f ? (opd_proj[1] <= 0.0f && (src_proj[0] > 0.0f
				|| (src_proj[0] == 0.0f && src_proj[1] > 0.0f))) :
				(opd_proj[0] > 0.0f ? (src_proj[0] == 0.0f ? (src_proj[1] > 0.0f) :
					(src_proj[0] > 0.0f && src_proj[1] / src_proj[0] > opd_proj[1] /
						opd_proj[0])) : (src_proj[0] >= 0.0f || src_proj[1] /
							src_proj[0] > opd_proj[1] / opd_proj[0]));
		}
	);
	for (int vertices_index{ 0 }; vertices_index < vertices.size(); vertices_index++) {
		lgo::svec<float, 3> center{ lgo::sc<lgo::svec<float, 3>>(cell.get_pos()) };
		lgo::svec<float, 3> vertex1{ lgo::sc<lgo::svec<float, 3>>(
			vertices[vertices_index]) };
		lgo::svec<float, 3> vertex2{ lgo::sc<lgo::svec<float, 3>>(vertices[
			lgo::sc<std::size_t>((vertices_index == 0) ? vertices.size() : vertices_index) -
			lgo::sc<std::size_t>(1)]) };
		if (lgo::line{ center, vertex1 }.angle({ center, vertex2 }) >
			std::numbers::pi_v<float> / 2) {
			continue;
		}
		lgo::arrs<lgo::svec<float, 3>, 3> triangle{ vertex1, vertex2, center };
		bool to_push{ true };
		std::vector<Cell*> include_cells{ get_include_cells(triangle) };
		for (int i{ 0 }; i < include_cells.size(); ++i) {
			for (int j{ 0 }; j < 3; ++j) {
				const std::vector<Cell_surface*>& include_surfaces_var{
					include_cells[i]->get_include_surfaces() };
				for (int k{ 0 }; k < include_surfaces_var.size(); ++k)
					for (int l{ 0 }; l < 3; ++l) {
						const lgo::arrs<lgo::svec<float, 3>, 3>& include_surface{
							include_surfaces_var[k]->get_vertices() };
						if (triangle[j] != include_surface[l])
							continue;
						for (int m{ 0 }; m < 3; ++m) {
							if (m == j)
								continue;
							for (int n{ 0 }; n < 3; ++n) {
								if (n == l || triangle[m] != include_surface[n])
									continue;
								int o{ 3 - j - m };
								int p{ 3 - l - n };
								lgo::line line{ triangle[j], triangle[m] };
								lgo::svec<float, 3> vec1{ triangle[o] -
									line.foot_of_perpendicular(triangle[o]) };
								lgo::svec<float, 3> vec2{ include_surface[p] -
									line.foot_of_perpendicular(include_surface[p]) };
								if (std::abs(dot(vec1, vec2) - len(vec1) * len(vec2)) <
									0.000001f)
									to_push = false;
							}
						}
					}
			}
		}
		if (to_push)
			cell.add_child_surface(new Stone_surface{ triangle, { lgo::svec<float, 2>{ 0.0f, 0.0f },
				lgo::svec<float, 2>{ 1.0f, 0.0f }, lgo::svec<float, 2>{ 0.5f, 1.0f } }, &cell });
	}
}