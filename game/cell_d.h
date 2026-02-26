#ifndef CELL_H
#define CELL_H

#include <vector>

#include "cell_f.h"
#include "surface_d.h"
#include "block_d.h"
#include "light_f.h"

enum class Cell_type {
	Air,
	Stone,
	out_of_world,
	unknown
};

class Cell {
public:
	Cell(lgo::svec<int, 3> pos_param);
	virtual ~Cell() = default;
	virtual Cell& assign(Cell& cell);
	virtual Cell_type get_type() const = 0;
	virtual Cell* get_actual_cell() = 0;
	const lgo::svec<int, 3>& get_pos() const;
	const std::vector<Cell_surface*> get_include_surfaces() const;
	void update_surface();
	void add_placeholders();
	void remove_placeholders();
	void add_placeholder(Cell* cell);
	void remove_placeholder(Cell* cell);
	void add_child_surface(Cell_surface* surface);
	void remove_child_surface(Cell_surface* surface);
	void remove_child_surfaces();
	void render() const;
protected:
	lgo::svec<int, 3> pos{};
	std::vector<Cell*> placeholders{};
	std::vector<Cell_surface*> child_surfaces{};
	std::vector<Cell_surface*> include_surfaces{};
	std::vector<Point_light*> point_lights{};
	std::vector<Dir_light*> dir_lights{};
	std::vector<Spotlight*> spotlights{};
	virtual void update_surface1(Cell&) = 0;
	friend void Block::set_child_cell(Cell& cell);
};

class Air_cell final :public Cell {
public:
	Air_cell(lgo::svec<int, 3> pos_param);
	virtual ~Air_cell() = default;
	virtual Cell_type get_type() const;
	virtual Cell* get_actual_cell();
protected:
	virtual void update_surface1(Cell& cell);
};

class Stone_cell final :public Cell {
public:
	Stone_cell(lgo::svec<int, 3> pos_param);
	virtual ~Stone_cell() = default;
	virtual Cell_type get_type() const;
	virtual Cell* get_actual_cell();
protected:
	virtual void update_surface1(Cell& cell);
};

#endif