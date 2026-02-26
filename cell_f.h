#ifndef CELL_F_H
#define CELL_F_H

#include "data.h"

enum class Cell_type;

class Cell_pv;
class Air_cell;
class Stone_cell;

Cell_pv* get_cell(glm::ivec3 pos, World_data* data);

#endif