#ifndef CELL_F_H
#define CELL_F_H

#include "data.h"

enum class Cell_type;

class Cell_pv;
class Air_cell;
class Stone_cell;

Cell_pv* get_cell(glm::ivec3 pos, World_data* data);
glm::vec3 to_cell_side(glm::vec3 begin, glm::vec3 dir);
glm::ivec3 to_block(glm::ivec3 coord);
glm::ivec3 to_coord_in_block(glm::ivec3 coord);

#endif