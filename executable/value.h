#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <fstream>
#include <map>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "surface.h"
#include "window_f.h"
#include "value_f.h"
#include "text_f.h"

enum class State {
    Init,
    Home,
    World
};

struct Character {
    int advance{};
    int left_pos{};
    int top_pos{};
    int width{};
    int height{};
    unsigned int tex{};
    unsigned int count{};
};

struct Shader {
    unsigned int text{};
    unsigned int button{};
    unsigned int cell_surface{};
    unsigned int object_surface{};
};

struct Setting {
    unsigned int start_width;
    unsigned int start_height;
    float acceleration;
    float max_speed;
    float angle_of_view;
    float friction;
    float cursor_sensitivity;
    glm::vec3 environment_color;
    float shininess;
    float specular_strength;
    int text_resolution;
};

#endif