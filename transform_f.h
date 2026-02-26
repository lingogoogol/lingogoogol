#ifndef TRANSFORM_F_H
#define TRANSFORM_F_H

#include <array>

#include <glm/glm.hpp>

#include "cell_f.h"
#include "data_f.h"
#include "surface_f.h"

glm::vec3 to_point_in_cell(glm::vec3 point);
glm::ivec3 to_coord(glm::ivec3 point);
glm::ivec3 to_coord_in_block(glm::ivec3 coord);
glm::ivec3 to_block(glm::ivec3 coord);
bool point_at_edge(glm::vec3 point);
bool point_at_edge_meet_surface(glm::vec3 point, Surface_pv* surface);
glm::vec3 to_cell_side(glm::vec3 begin, glm::vec3 dir);
Cell_pv* get_cell(glm::ivec3 pos, World_data* data);
Cell_pv* get_included_cell(const std::array<glm::vec3, 3>& vertices, World_data* data);

#endif