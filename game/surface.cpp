#include <iostream>
#include <type_traits>
#include <cmath>

#include <lgo/lib/glad.h>
#include <lgo/data/.h>

#include "data_d.h"
#include "surface_d.h"
#include "cell_d.h"
#include "transform_i.h"

Surface::Surface(lgo::arrs<lgo::svec<float, 3>, 3> vertices_param,
	lgo::arrs<lgo::svec<float, 2>, 3> texcoords_param) :
	vertices{ vertices_param }, texcoords{ texcoords_param } {
	allocate_VAO();
	current_VBO();
}

Surface::Surface(lgo::svec<float, 3> vertex1, lgo::svec<float, 3> vertex2,
	lgo::svec<float, 3> vertex3, lgo::svec<float, 2> texcoord1,
	lgo::svec<float, 2> texcoord2, lgo::svec<float, 2> texcoord3) :
	Surface{ { vertex1, vertex2, vertex3 }, { texcoord1, texcoord2, texcoord3 } } {}

const lgo::arrs<lgo::svec<float, 3>, 3>& Surface::get_vertices() const {
	return vertices;
}

void Surface::allocate_VAO() {
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 3, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, lgo::rc<void*>(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, lgo::rc<void*>(sizeof(float) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Surface::current_VBO() {
	float vertices_var[3][5]{};
	for (int i{ 0 }; i < 3; ++i) {
		for (int j{ 0 }; j < 3; ++j)
			vertices_var[i][j] = vertices[i][j];
		for (int j{ 0 }; j < 2; ++j)
			vertices_var[i][j + 3] = texcoords[i][j];
	}
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 5 * 3, vertices_var);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Cell_surface::Cell_surface(lgo::arrs<lgo::svec<float, 3>, 3> vertices_param,
	lgo::arrs<lgo::svec<float, 2>, 3> texcoords_param, Cell* parent_cell_param) :
	Surface{ vertices_param,texcoords_param },
	parent_cell{ parent_cell_param } {}

Cell_surface::Cell_surface(lgo::svec<float, 3> vertex1,
	lgo::svec<float, 3> vertex2, lgo::svec<float, 3> vertex3,
	lgo::svec<float, 2> texcoord1, lgo::svec<float, 2> texcoord2,
	lgo::svec<float, 2> texcoord3, Cell* parent_cell_param) :
	Cell_surface{ { vertex1, vertex2, vertex3 },
		{ texcoord1, texcoord2, texcoord3 }, parent_cell_param } {}

std::vector<Cell*> Cell_surface::get_include_cells() {
	return ::get_include_cells(vertices);
}

void Cell_surface::render() const {
	Data* data{ get_data() };
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(data->shader->transparent_cell);
	float* view_pointer{ lgo::make_gl_matrix(data->view) };
	glUniformMatrix4fv(glGetUniformLocation(data->shader->transparent_cell,
		"view"), 1, GL_FALSE, view_pointer);
	lgo::free_gl_matrix(view_pointer);
	float* projection_pointer{ lgo::make_gl_matrix(data->projection) };
	glUniformMatrix4fv(glGetUniformLocation(data->shader->transparent_cell,
		"projection"), 1, GL_FALSE, projection_pointer);
	lgo::free_gl_matrix(projection_pointer);
	glUniform1i(glGetUniformLocation(data->shader->transparent_cell, "normal"), 0);
	glUniform1i(glGetUniformLocation(data->shader->transparent_cell, "color"), 1);
	glUniform1i(glGetUniformLocation(data->shader->transparent_cell, "shininess"), 2);
	glUniform1i(glGetUniformLocation(data->shader->transparent_cell, "specular_strength"), 3);
	glUniform3iv(glGetUniformLocation(data->shader->transparent_cell,
		"parent_cell"), 1, parent_cell->get_pos().data());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, normal);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, color);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shininess);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, specular_strength);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

Stone_surface::Stone_surface(lgo::arrs<lgo::svec<float, 3>, 3> vertices_param,
	lgo::arrs<lgo::svec<float, 2>, 3> texcoords_param, Cell* parent_cell_param) :
	Cell_surface{ vertices_param,texcoords_param,parent_cell_param } {
	gen_normal();
	gen_color();
	gen_shininess();
	gen_specular_strength();
}

Stone_surface::Stone_surface(lgo::svec<float, 3> vertex1,
	lgo::svec<float, 3> vertex2, lgo::svec<float, 3> vertex3,
	lgo::svec<float, 2> texcoord1, lgo::svec<float, 2> texcoord2,
	lgo::svec<float, 2> texcoord3, Cell* parent_cell_param) :
	Stone_surface{ { vertex1, vertex2, vertex3 },
		{ texcoord1, texcoord2, texcoord3 }, parent_cell_param } {}

void Stone_surface::select() {
	unsigned char tex_data[16]{
		255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255
	};
	color = lgo::create_tex(tex_data, GL_RGBA,
		constant::surface_tex_precision, constant::surface_tex_precision);
}

void Stone_surface::unselect() {
	gen_color();
}

void Stone_surface::right_click() {
	World_data* world_data{ get_world_data() };
	std::vector<lgo::svec<int, 3>> poss{};
	lgo::svec<int, 3> center{ parent_cell->get_pos() };
	for (int i{ -1 }; i <= 1; ++i)
		for (int j{ -1 }; j <= 1; ++j)
			for (int k{ -1 }; k <= 1; ++k)
				poss.push_back(lgo::svec<int, 3>{ center[0] + i, center[1] + j, center[2] + k });
	lgo::svec<float, 3> selected_pos{ world_data->selected_pos };
	std::sort(poss.begin(), poss.end(),
		[selected_pos](const lgo::svec<int, 3>& comp, const lgo::svec<int, 3>& comped)->bool {
			return lgo::len(lgo::sc<lgo::svec<float, 3>>(comp) - selected_pos) <
				lgo::len(lgo::sc<lgo::svec<float, 3>>(comped) - selected_pos);
		}
	);
	for (int i{ 0 }; i < poss.size(); ++i) {
		Cell* cell{ get_cell(poss[i]) };
		if (!cell || cell->get_type() != Cell_type::Air)
			continue;
		set_cell(poss[i], Cell_type::Stone);
		return;
	}
}

bool Stone_surface::get_property(Surface_property property) const {
	switch (property) {
	case Surface_property::Selectable:
		return true;
	default:
		handle_error(U"�����������ݩʡC");
		return false;
	}
}

void Stone_surface::gen_normal() {
	float tex_data[12]{
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	};
	normal = lgo::create_tex(tex_data, GL_RGB,
		constant::surface_tex_precision, constant::surface_tex_precision);
}

void Stone_surface::gen_color() {
	unsigned char tex_data[16]{
		0, 255, 255, 255, 255, 0, 255, 255,
		255, 255, 0, 255, 0, 255, 255, 255
	};
	color = lgo::create_tex(tex_data, GL_RGBA,
		constant::surface_tex_precision, constant::surface_tex_precision);
}

void Stone_surface::gen_shininess() {
	float tex_data[4]{
		32.0f, 32.0f,
		32.0f, 32.0f
	};
	shininess = lgo::create_tex(tex_data, GL_RED,
		constant::surface_tex_precision, constant::surface_tex_precision);
}

void Stone_surface::gen_specular_strength() {
	float tex_data[4]{
		0.5f, 0.5f,
		0.5f, 0.5f
	};
	specular_strength = lgo::create_tex(tex_data, GL_RED,
		constant::surface_tex_precision, constant::surface_tex_precision);
}