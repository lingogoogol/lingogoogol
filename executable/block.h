#ifndef BLOCK_H
#define BLOCK_H

#include <array>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "block_f.h"
#include "cell_f.h"
#include "object_f.h"
#include "value.h"

class Block {
public:
	Block();
	Block(glm::ivec3 pos_param);
	const glm::ivec3& get_pos();
	std::array<std::array<std::array<Cell_pv*, constant::block_side_length>,
		constant::block_side_length>, constant::block_side_length>& get_child_cells();
	void update();
	void destruct();
	void render();
private:
	glm::ivec3 pos{};
	std::array<std::array<std::array<Cell_pv*, constant::block_side_length>,
		constant::block_side_length>, constant::block_side_length> child_cells{};
	std::vector<Object*> child_objects{};
};

#endif