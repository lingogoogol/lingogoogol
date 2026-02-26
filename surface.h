#ifndef SURFACE_H
#define SURFACE_H

#include <array>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "surface_f.h"
#include "cell_f.h"
#include "lib.h"
#include "data_f.h"
#include "object_f.h"

class Surface_pv {
public:
	glm::mat4 projection{};
	Surface_pv(std::array<glm::vec3, 3> vertices,
		std::array<glm::vec2, 3> texcoords, World_data* data_param);
	Surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3, World_data* data_param);
	virtual ~Surface_pv();
	void set_vertices(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3);
	void set_texcoord(glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3);
	void set_identity(Surface_pv** identity_param);
	const std::array<glm::vec3, 3>& get_vertices();
	virtual void render() = 0;
protected:
	World_data* data{};
	std::array<glm::vec3, 3> vertices{};
	std::array<glm::vec2, 3> texcoords{};
	Surface_pv** identity{};
	unsigned int VAO{};
	unsigned int VBO{};
	unsigned int normal{};
	unsigned int color{};
	unsigned int shininess{};
	unsigned int specular_strength{};
	void allocate_VAO();
	void current_VBO();
	virtual void gen_color() = 0;
	virtual void gen_normal() = 0;
	virtual void gen_shininess() = 0;
	virtual void gen_specular_strength() = 0;
};

class Cell_surface_pv :public Surface_pv {
public:
	Cell_surface_pv(std::array<glm::vec3, 3> vertices_param, std::array<glm::vec2, 3> texcoords_param,
		Cell_pv* parent_cell_param, World_data* data_param);
	Cell_surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3,
		Cell_pv* parent_cell_param, World_data* data_param);
	virtual ~Cell_surface_pv();
	virtual void render();
protected:
	Cell_pv* parent_cell{};
	Cell_pv* included_cell{};
	Object* include_object{};
};

class Stone_surface final :public Cell_surface_pv {
public:
	Stone_surface(std::array<glm::vec3, 3> vertices_param, std::array<glm::vec2, 3> texcoords_param,
		Cell_pv* parent_cell_param, World_data* data_param);
	Stone_surface(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3,
		Cell_pv* parent_cell_param, World_data* data_param);
	virtual ~Stone_surface();
protected:
private:
	virtual void gen_normal();
	virtual void gen_color();
	virtual void gen_shininess();
	virtual void gen_specular_strength();
};

class Object_surface_pv {
public:
protected:
private:
};

template<typename T1>
unsigned int create_tex(T1* tex_data, GLenum format, unsigned int width, unsigned int height) {
	unsigned int tex{};
	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLenum type{};
	if constexpr (std::is_same_v<T1, unsigned char>)
		type = GL_UNSIGNED_BYTE;
	else if constexpr (std::is_same_v<T1, int>)
		type = GL_INT;
	else if constexpr (std::is_same_v<T1, unsigned int>)
		type = GL_UNSIGNED_INT;
	else if constexpr (std::is_same_v<T1, float>)
		type = GL_FLOAT;
	else
		handle_error(U"未知的資料類型。");
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, tex_data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

template<typename... T1>
unsigned int create_FBO1(int color_attachment_num, unsigned int attachment, T1... attachments) {
	if constexpr (sizeof...(T1) == 0) {
		unsigned int FBO{};
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, attachment, 0);
		return FBO;
	}
	else {
		unsigned int FBO{ create_FBO1(color_attachment_num, attachments...) };
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (color_attachment_num - sizeof...(T1)), GL_TEXTURE_2D, attachment, 0);
		return FBO;
	}
}

template<typename... T1>
unsigned int create_FBO(T1... attachments) {
	unsigned int FBO{ create_FBO1(sizeof...(T1) - 1,attachments...) };
	std::array<unsigned int, sizeof...(T1)> buffers{};
	for (int i{ 0 }; i < sizeof...(T1) - 1; i++)
		buffers[i] = GL_COLOR_ATTACHMENT0 + i;
	glDrawBuffers(sizeof...(T1) - 1, buffers.data());
	return FBO;
}

#endif