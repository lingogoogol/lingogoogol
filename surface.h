#ifndef SURFACE_H
#define SURFACE_H

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "surface_f.h"
#include "cell_f.h"

class Surface_pv {
public:
	glm::mat4 view{};
	glm::mat4 projection{};
	Surface_pv();
	Surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3);
	virtual ~Surface_pv();
	void set_vertices(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3);
	void set_texcoord(glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3);
	std::vector<glm::vec3>* get_vertices();
	void print();
protected:
	std::vector<glm::vec3> vertices{};
	std::vector<glm::vec2> texcoord{};
	glm::vec3 normal{};
	unsigned int tex{};
	unsigned int VAO{};
	unsigned int VBO{};
	void allocate_VAO();
	void current_VBO();
	virtual void gen_tex() = 0;
	void gen_tex1(unsigned char* tex_data);
};

class Cell_surface_pv :public Surface_pv {
public:
	Cell_surface_pv();
	Cell_surface_pv(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3);
	virtual ~Cell_surface_pv();
protected:
	Cell* belonged_cell{};
};

class Stone_surface final :public Cell_surface_pv {
public:
	Stone_surface();
	Stone_surface(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3,
		glm::vec2 texcoord1, glm::vec2 texcoord2, glm::vec2 texcoord3);
	virtual ~Stone_surface();
protected:
private:
	virtual void gen_tex();
};

class Object_surface_pv {
public:
protected:
private:
};

#endif