#ifndef SURFACE_H
#define SURFACE_H

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "surface_f.h"
#include "cell_f.h"
#include "debug_f.h"
#include "value_f.h"
#include "object_f.h"

class Surface_pv {
public:
	glm::mat4 projection{};
	Surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3);
	virtual ~Surface_pv();
	void set_vertices(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3);
	void set_texcoord(glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3);
	void set_identity(Surface_pv** identity_param);
	std::array<glm::vec3, 3>* get_vertices();
	virtual void render() = 0;
protected:
	std::array<glm::vec3, 3> vertices{};
	std::array<glm::vec2, 3> texcoord{};
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
	Cell_surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3, Cell_pv* parent_cell_param);
	virtual ~Cell_surface_pv();
	virtual void render();
protected:
	Cell_pv* parent_cell{};
	Object* object_included{};
};

class Stone_surface final :public Cell_surface_pv {
public:
	Stone_surface(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3, Cell_pv* parent_cell_param);
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
unsigned int apply_tex(T1* data, GLenum format, unsigned int width, unsigned int height) {
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
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

#endif