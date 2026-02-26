#ifndef BLOCK_H
#define BLOCK_H

#include <array>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "block_f.h"
#include "cell_f.h"
#include "object_f.h"
#include "data_f.h"
#include "const_f.h"

class Block {
public:
	Block(glm::ivec3 pos_param);
	const glm::ivec3& get_pos();
	std::array<std::array<std::array<Cell_pv*, constant::cell_num>,
		constant::cell_num>, constant::cell_num>& get_child_cells();
	void update(World_data* data);
	void destruct();
	void render(World_data* data);
private:
	glm::ivec3 pos{};
	std::array<std::array<std::array<Cell_pv*, constant::cell_num>,
		constant::cell_num>, constant::cell_num> child_cells{};
	std::vector<Object*> child_objects{};
};

#endif