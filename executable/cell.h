#ifndef CELL_H
#define CELL_H

#include <glm/glm.hpp>
#include <vector>

#include "object.h"
#include "cell_f.h"
#include "surface.h"

enum class Cell_material {
	Air,
	Stone,
	out_of_world,
	unknown
};

class Cell_pv {
public:
	Cell_pv(glm::ivec3 pos_param, Block* parent_block_param, Cell_pv** identity_param);
	virtual Cell_material get_material() = 0;
	virtual Cell_pv* get_actual_cell() = 0;
	const glm::ivec3* get_pos();
	void update_surface();
	void render();
	void add_placeholders();
	void remove_placeholders();
	void add_placeholder(Cell_pv* cell);
	void remove_placeholder(Cell_pv* cell);
protected:
	glm::ivec3 pos{};
	std::vector<Cell_pv*> placeholders{};
	std::vector<Surface_pv*> child_surfaces{};
	std::vector<Surface_pv*> include_surfaces{};
	Block* parent_block{};
	Cell_pv** identity{};
};

class Air_cell final :public Cell_pv {
public:
	Air_cell(glm::vec3 pos_param, Block* parent_block_param, Cell_pv** identity_param);
	virtual Cell_material get_material();
	virtual Cell_pv* get_actual_cell();
	void place(Cell_material material);
private:
};

class Stone_cell final :public Cell_pv {
public:
	Stone_cell(glm::ivec3 pos_param, Block* parent_block_param, Cell_pv** identity_param);
	virtual Cell_pv* get_actual_cell();
	virtual Cell_material get_material();
	void broke();
protected:
};

#endif