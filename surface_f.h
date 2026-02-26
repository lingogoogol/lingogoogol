#ifndef SURFACE_F_H
#define SURFACE_F_H

#include <array>

#include <glm/glm.hpp>

#include "cell_f.h"
#include "data_f.h"

class Surface_pv;
class Cell_surface_pv;
class Stone_surface;
class Object_surface_pv;

Cell_pv* get_included_cell(const std::array<glm::vec3, 3>& vertices, World_data* data);

#endif