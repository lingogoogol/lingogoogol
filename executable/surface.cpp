#include <iostream>
#include <type_traits>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "value.h"
#include "surface.h"
#include "debug_f.h"
#include "cell.h"

Surface_pv::Surface_pv() {
	allocate_VAO();
	return;
}

Surface_pv::Surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3) {
	vertices[0] = vertex1;
	vertices[1] = vertex2;
	vertices[2] = vertex3;
	texcoord[0] = texcoord1;
	texcoord[1] = texcoord2;
	texcoord[2] = texcoord3;
	allocate_VAO();
	current_VBO();
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
	texcoord[0] = texcoord1;
	texcoord[1] = texcoord2;
	texcoord[2] = texcoord3;
	current_VBO();
	return;
}

std::array<glm::vec3, 3>* Surface_pv::get_vertices() {
	return &vertices;
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
			vertices_var[i][j + 3] = texcoord[i][j];
	}
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 5 * 3, vertices_var);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return;
}

Cell_surface_pv::Cell_surface_pv() :Surface_pv{} {
	return;
}

Cell_surface_pv::Cell_surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3) :
	Surface_pv{ vertex1,vertex2,vertex3,
	texcoord1,texcoord2,texcoord3 } {
	return;
}

Cell_surface_pv::~Cell_surface_pv() {
	return;
}

void Cell_surface_pv::render() {
	glEnable(GL_DEPTH_TEST);
	glUseProgram(object::shader.transparent_cell);
	glUniformMatrix4fv(glGetUniformLocation(object::shader.transparent_cell, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(object::shader.transparent_cell, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform1i(glGetUniformLocation(object::shader.transparent_cell, "normal"), 0);
	glUniform1i(glGetUniformLocation(object::shader.transparent_cell, "color"), 1);
	glUniform1i(glGetUniformLocation(object::shader.transparent_cell, "shininess"), 2);
	glUniform1i(glGetUniformLocation(object::shader.transparent_cell, "specular_strength"), 3);
	glm::ivec3 posss{ 0,0,0 };
	glUniform3iv(glGetUniformLocation(object::shader.transparent_cell, "parent_cell"), 1, glm::value_ptr(posss));
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
	check_GL_error();
	return;
}

Stone_surface::Stone_surface() :Cell_surface_pv{} {
	gen_normal();
	gen_color();
	gen_shininess();
	gen_specular_strength();
	return;
}

Stone_surface::Stone_surface(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3) :
	Cell_surface_pv{ vertex1,vertex2,vertex3,
	texcoord1,texcoord2,texcoord3 } {
	gen_normal();
	gen_color();
	gen_shininess();
	gen_specular_strength();
	return;
}

Stone_surface::~Stone_surface() {
	return;
}

void Stone_surface::gen_normal() {
	float data[12]{
		0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,
		0.0f,0.0f,1.0f,0.0f,0.0f,1.0f,
	};
	normal = apply_tex(data, GL_RGB, constant::surface_tex_precision, constant::surface_tex_precision);
}

void Stone_surface::gen_color() {
	unsigned char data[16]{
		0,255,0,255,255,0,0,255,
		0,0,255,255,0,255,0,255
	};
	color = apply_tex(data, GL_RGBA, constant::surface_tex_precision, constant::surface_tex_precision);
	return;
}

void Stone_surface::gen_shininess() {
	float data[4]{
		32.0f,32.0f,
		32.0f,32.0f
	};
	shininess = apply_tex(data, GL_RED, constant::surface_tex_precision, constant::surface_tex_precision);
}

void Stone_surface::gen_specular_strength() {
	float data[4]{
		0.5f,0.5f,
		0.5f,0.5f
	};
	specular_strength = apply_tex(data, GL_RED, constant::surface_tex_precision, constant::surface_tex_precision);
}