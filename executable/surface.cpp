#include <iostream>

#include <glad/glad.h>

#include "value.h"
#include "surface.h"
#include "debug_f.h"

Surface_pv::Surface_pv() {
	allocate_VAO();
	return;
}

Surface_pv::Surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3) {
	vertices.push_back(vertex1);
	vertices.push_back(vertex2);
	vertices.push_back(vertex3);
	texcoord.push_back(texcoord1);
	texcoord.push_back(texcoord2);
	texcoord.push_back(texcoord3);
	normal = glm::normalize(glm::cross(vertex3 - vertex1, vertex2 - vertex1));
	allocate_VAO();
	current_VBO();
	return;
}

Surface_pv::~Surface_pv() {
	return;
}

void Surface_pv::set_vertices(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3) {
	vertices.clear();
	vertices.push_back(vertex1);
	vertices.push_back(vertex2);
	vertices.push_back(vertex3);
	current_VBO();
	return;
}

void Surface_pv::set_texcoord(glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3) {
	texcoord.clear();
	texcoord.push_back(texcoord1);
	texcoord.push_back(texcoord2);
	texcoord.push_back(texcoord3);
	current_VBO();
	return;
}

std::vector<glm::vec3>* Surface_pv::get_vertices() {
	return &vertices;
}

void Surface_pv::print() {
	glEnable(GL_DEPTH_TEST);
	glUseProgram(object::shader.cell_surface);
	glUniformMatrix4fv(glGetUniformLocation(object::shader.cell_surface, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(object::shader.cell_surface, "view"), 1, GL_FALSE, &view[0][0]);
	glm::vec3 light_pos{ 10.0f, 13.0f, -6.0f };
	glUniform3fv(glGetUniformLocation(object::shader.cell_surface, "point_light.pos"), 1, &light_pos[0]);
	glm::vec3 light_color{ 1.0f, 1.0f, 1.0f };
	glUniform3fv(glGetUniformLocation(object::shader.cell_surface, "point_light.color"), 1, &light_color[0]);
	glUniform3fv(glGetUniformLocation(object::shader.cell_surface, "camera_pos"), 1, &object::camera_pos[0]);
	glUniform3fv(glGetUniformLocation(object::shader.cell_surface, "ambient_color"), 1, &object::setting.environment_color[0]);
	glUniform1f(glGetUniformLocation(object::shader.cell_surface, "shininess"), object::setting.shininess);
	glUniform1f(glGetUniformLocation(object::shader.cell_surface, "specular_strength"), object::setting.specular_strength);
	glBindVertexArray(VAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	check_GL_error();
	return;
}

void Surface_pv::allocate_VAO() {
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 3, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 5));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return;
}

void Surface_pv::current_VBO() {
	float vertices_var[3][8]{};
	for (int i{ 0 }; i < 3; i++) {
		for (int j{ 0 }; j < 3; j++)
			vertices_var[i][j] = vertices[i][j];
		for (int j{ 0 }; j < 2; j++)
			vertices_var[i][j + 3] = texcoord[i][j];
		for (int j{ 0 }; j < 3; j++)
			vertices_var[i][j + 5] = normal[j];
	}
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 8 * 3, vertices_var);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return;
}

void Surface_pv::gen_tex1(unsigned char* tex_data) {
	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
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

Stone_surface::Stone_surface() :Cell_surface_pv{} {
	gen_tex();
	return;
}

Stone_surface::Stone_surface(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
	glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3) :
	Cell_surface_pv{ vertex1,vertex2,vertex3,
	texcoord1,texcoord2,texcoord3 } {
	gen_tex();
	return;
}

Stone_surface::~Stone_surface() {
	return;
}

void Stone_surface::gen_tex() {
	unsigned char tex_data[16]{
		0,255,0,255,255,0,0,255,
		0,0,255,255,0,255,0,255
	};
	gen_tex1(tex_data);
	return;
}