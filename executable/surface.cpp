#include <iostream>
#include <type_traits>
#include <cmath>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "value.h"
#include "surface.h"
#include "debug_f.h"
#include "cell.h"

Surface_pv::Surface_pv(std::array<glm::vec3, 3> vertices_param,
	std::array<glm::vec2, 3> texcoords_param, World_data* data_param) :
	vertices{ vertices_param }, texcoords{ texcoords_param }, data{ data_param } {
	allocate_VAO();
	current_VBO();
	return;
}

Surface_pv::Surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3, World_data* data_param) :
	Surface_pv{ {vertex1,vertex2,vertex3},{texcoord1,texcoord2,texcoord3},data_param } {
	return;
}

Surface_pv::~Surface_pv() {
	return;
}

void Surface_pv::set_vertices(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3) {
	vertices[0] = vertex1;
	vertices[1] = vertex2;
	vertices[2] = vertex3;
	current_VBO();
	return;
}

void Surface_pv::set_texcoord(glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3) {
	texcoords[0] = texcoord1;
	texcoords[1] = texcoord2;
	texcoords[2] = texcoord3;
	current_VBO();
	return;
}

void Surface_pv::set_identity(Surface_pv** identity_param) {
	identity = identity_param;
	return;
}

const std::array<glm::vec3, 3>& Surface_pv::get_vertices() {
	return vertices;
}

void Surface_pv::allocate_VAO() {
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 3, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return;
}

void Surface_pv::current_VBO() {
	float vertices_var[3][5]{};
	for (int i{ 0 }; i < 3; i++) {
		for (int j{ 0 }; j < 3; j++)
			vertices_var[i][j] = vertices[i][j];
		for (int j{ 0 }; j < 2; j++)
			vertices_var[i][j + 3] = texcoords[i][j];
	}
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 5 * 3, vertices_var);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return;
}

Cell_surface_pv::Cell_surface_pv(std::array<glm::vec3, 3> vertices_param, std::array<glm::vec2, 3> texcoords_param,
	Cell_pv* parent_cell_param, World_data* data_param) :
	Surface_pv{ vertices_param,texcoords_param,data_param }, parent_cell{ parent_cell_param } {
	included_cell = get_included_cell(vertices_param, data);
	if (!included_cell) {
		handle_error(U"表面存在世界外，無法儲存其位置。", data);
		return;
	}
	included_cell->add_include_surface(this);
	return;
}

Cell_surface_pv::Cell_surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3,
	Cell_pv* parent_cell_param, World_data* data_param) :
	Cell_surface_pv{ {vertex1,vertex2,vertex3},{texcoord1,texcoord2,texcoord3},parent_cell_param,data_param } {
}

Cell_surface_pv::~Cell_surface_pv() {
	included_cell->remove_include_surface(this, data);
	return;
}

void Cell_surface_pv::render() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(data->shader.transparent_cell);
	glUniformMatrix4fv(glGetUniformLocation(data->shader.transparent_cell, "view"), 1, GL_FALSE, glm::value_ptr(data->view));
	glUniformMatrix4fv(glGetUniformLocation(data->shader.transparent_cell, "projection"), 1, GL_FALSE, glm::value_ptr(data->projection));
	glUniform1i(glGetUniformLocation(data->shader.transparent_cell, "normal"), 0);
	glUniform1i(glGetUniformLocation(data->shader.transparent_cell, "color"), 1);
	glUniform1i(glGetUniformLocation(data->shader.transparent_cell, "shininess"), 2);
	glUniform1i(glGetUniformLocation(data->shader.transparent_cell, "specular_strength"), 3);
	glm::ivec3 posss{ 0,0,0 };
	glUniform3iv(glGetUniformLocation(data->shader.transparent_cell, "parent_cell"), 1, glm::value_ptr(posss));
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
	check_GL_error(data);
	return;
}

Stone_surface::Stone_surface(std::array<glm::vec3, 3> vertices_param, std::array<glm::vec2, 3> texcoords_param,
	Cell_pv* parent_cell_param, World_data* data_param):
	Cell_surface_pv{ vertices_param,texcoords_param,parent_cell_param,data_param } {
	gen_normal();
	gen_color();
	gen_shininess();
	gen_specular_strength();
	return;
}

Stone_surface::Stone_surface(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3,
	Cell_pv* parent_cell_param, World_data* data_param):
	Stone_surface{ {vertex1,vertex2,vertex3},{texcoord1,texcoord2,texcoord3},parent_cell_param,data_param } {
	return;
}

Stone_surface::~Stone_surface() {
	return;
}

void Stone_surface::gen_normal() {
	float tex_data[12]{
		0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,
		0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,
	};
	normal = create_tex(tex_data, GL_RGB,
		constant::surface_tex_precision, constant::surface_tex_precision, data);
}

void Stone_surface::gen_color() {
	unsigned char tex_data[16]{
		0,255,255,255,255,0,255,255,
		255,255,0,255,0,255,255,255
	};
	color = create_tex(tex_data, GL_RGBA,
		constant::surface_tex_precision, constant::surface_tex_precision, data);
	return;
}

void Stone_surface::gen_shininess() {
	float tex_data[4]{
		32.0f,32.0f,
		32.0f,32.0f
	};
	shininess = create_tex(tex_data, GL_RED,
		constant::surface_tex_precision, constant::surface_tex_precision, data);
}

void Stone_surface::gen_specular_strength() {
	float tex_data[4]{
		0.5f,0.5f,
		0.5f,0.5f
	};
	specular_strength = create_tex(tex_data, GL_RED,
		constant::surface_tex_precision, constant::surface_tex_precision, data);
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