#ifndef SHADER_H
#define SHADER_H

#include "shader_f.h"

struct Shader {
    unsigned int text{};
    unsigned int button{};
    unsigned int transparent_cell{};
    unsigned int transparent_object{};
    unsigned int tile_ambient{};
    unsigned int tile_dir{};
    unsigned int tile_point{};
    unsigned int tile_spot{};
    unsigned int opaque_cell{};
    unsigned int opaque_object{};
};

#endif