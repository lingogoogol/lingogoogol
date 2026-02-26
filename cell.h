#ifndef CELL_H
#define CELL_H

#include <glm/glm.hpp>
#include <vector>

#include "object.h"
#include "cell_f.h"
#include "surface.h"
#include "block_f.h"
#include "light_f.h"

enum class Cell_type {
	Air,
	Stone,
	out_of_world,
	unknown
};

class Cell_pv {
public:
	Cell_pv(glm::ivec3 pos_param, Block* parent_block_param, Cell_pv** identity_param);
	virtual Cell_type get_type() = 0;
	virtual Cell_pv* get_actual_cell() = 0;
	const glm::ivec3& get_pos();
	const std::vector<Surface_pv*> get_include_surfaces();
	void update_surface(World_data* data);
	void add_placeholders(World_data* data);
	void remove_placeholders(World_data* data);
	void add_placeholder(Cell_pv* cell);
	void remove_placeholder(Cell_pv* cell);
	void add_include_surface(Surface_pv* surface);
	void remove_include_surface(Surface_pv* surface, World_data* data);
	void render(Data_pv* data);
protected:
	glm::ivec3 pos{};
	std::vector<Cell_pv*> placeholders{};
	std::vector<Surface_pv*> child_surfaces{};
	std::vector<Surface_pv*> include_surfaces{};
	std::vector<Point_light*> point_lights{};
	std::vector<Dir_light*> dir_lights{};
	std::vector<Spotlight*> spotlights{};
	Block* parent_block{};
	Cell_pv** identity{};
};

class Air_cell final :public Cell_pv {
public:
	Air_cell(glm::vec3 pos_param, Block* parent_block_param, Cell_pv** identity_param);
	virtual Cell_type get_type();
	virtual Cell_pv* get_actual_cell();
	void place(Cell_type type);
private:
};

class Stone_cell final :public Cell_pv {
public:
	Stone_cell(glm::ivec3 pos_param, Block* parent_block_param, Cell_pv** identity_param);
	virtual Cell_pv* get_actual_cell();
	virtual Cell_type get_type();
	void broke();
protected:
};

#endif