#ifndef SURFACE_H
#define SURFACE_H

#include <lgo/lib/glad.h>
#include <lgo/mat/s/.h>

#include "surface_f.h"
#include "cell_f.h"

enum class Surface_property {
	Selectable
};

class Surface {
public:
	lgo::smat<float, 4, 4> projection{};
	Surface(lgo::arrs<lgo::svec<float, 3>, 3> vertices,
		lgo::arrs<lgo::svec<float, 2>, 3> texcoords);
	Surface(lgo::svec<float, 3> vertex1, lgo::svec<float, 3> vertex2,
		lgo::svec<float, 3> vertex3, lgo::svec<float, 2> texcoord1,
		lgo::svec<float, 2> texcoord2, lgo::svec<float, 2> texcoord3);
	virtual ~Surface() = default;
	const lgo::arrs<lgo::svec<float, 3>, 3>& get_vertices() const;
	virtual void render() const = 0;
	virtual void select() = 0;
	virtual void unselect() = 0;
	virtual void right_click() = 0;
	virtual bool get_property(Surface_property property) const = 0;
protected:
	lgo::arrs<lgo::svec<float, 3>, 3> vertices{};
	lgo::arrs<lgo::svec<float, 2>, 3> texcoords{};
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

class Cell_surface :public Surface {
public:
	Cell_surface(lgo::arrs<lgo::svec<float, 3>, 3> vertices_param,
		lgo::arrs<lgo::svec<float, 2>, 3> texcoords_param, Cell* parent_cell_param);
	Cell_surface(lgo::svec<float, 3> vertex1, lgo::svec<float, 3> vertex2,
		lgo::svec<float, 3> vertex3, lgo::svec<float, 2> texcoord1,
		lgo::svec<float, 2> texcoord2, lgo::svec<float, 2> texcoord3, Cell* parent_cell_param);
	virtual ~Cell_surface() = default;
	std::vector<Cell*> get_include_cells();
	virtual void render() const;
protected:
	Cell* parent_cell{};
};

class Stone_surface final :public Cell_surface {
public:
	Stone_surface(lgo::arrs<lgo::svec<float, 3>, 3> vertices_param,
		lgo::arrs<lgo::svec<float, 2>, 3> texcoords_param, Cell* parent_cell_param);
	Stone_surface(lgo::svec<float, 3> vertex1, lgo::svec<float, 3> vertex2,
		lgo::svec<float, 3> vertex3, lgo::svec<float, 2> texcoord1,
		lgo::svec<float, 2> texcoord2, lgo::svec<float, 2> texcoord3, Cell* parent_cell_param);
	virtual ~Stone_surface() = default;
	virtual void select();
	virtual void unselect();
	virtual void right_click();
	virtual bool get_property(Surface_property property) const;
protected:
private:
	virtual void gen_normal();
	virtual void gen_color();
	virtual void gen_shininess();
	virtual void gen_specular_strength();
};

#endif